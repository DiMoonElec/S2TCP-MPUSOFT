#include "app_bridge.h"
#include "at32f413.h"
#include "SerialRingBuffer\public-api.h"
#include "serial_ring_buffer_hal.h"
#include "tcp_uart_bridge.h"

/////////////////////////////////////////////////////////////////////////
// Размеры аппаратных кольцевых буферов UART (HAL-уровень).
//
// Это НЕ то же самое, что TCP_UART_BRIDGE_BUFFER_SIZE/QUEUE_SIZE внутри
// моста (те уже дают 2048 байт программной очереди) - этот буфер должен
// продержаться только на интервале между приемом байта по прерыванию
// UART и следующим вызовом app_bridge_process() из главного цикла (там
// нет задержек, но modbus_tcp_serv_process() может занимать заметное
// время на SPI-обмене с W5500).
//
// Ориентировочная формула: размер >= (байт/сек UART) * (худшее время
// одной итерации главного цикла) * запас. Например, для 115200 бод
// (~87 мкс/байт) и 1-2 мс на итерацию это ~12-23 байта - 256 байт дают
// солидный запас. Если у вас более высокая скорость UART или
// modbus_tcp_serv_process() может блокироваться дольше - пересчитайте
// и поднимите значение (для UART2 в будущем можно завести свой define).
/////////////////////////////////////////////////////////////////////////
#ifndef UART1_RING_BUFFER_SIZE
#define UART1_RING_BUFFER_SIZE 256
#endif

#define UART1_DEFAULT_BAUDRATE 115200u

/////////////////////////////////////////////////////////////////////////
// Канал 1: UART1 <-> TCP:950
//
// SerialBuffer_UART1 намеренно НЕ static: USART1_IRQHandler в
// hw_interrupts.c обращается к ней напрямую через
//   extern SerialRingBuffer_t SerialBuffer_UART1;
//   SerialRingBufferHAL_IRQHandler(&SerialBuffer_UART1);
// Это осознанное исключение из инкапсуляции модуля - в обработчике
// прерывания нежелательно добавлять лишнюю косвенность (поиск канала по
// IRQn, вызов через API), поэтому ISR трогает буфер конкретного канала
// напрямую. Имя переменной и её внешнее связывание - часть контракта
// с hw_interrupts.c, при переименовании поправьте оба места.
/////////////////////////////////////////////////////////////////////////
SerialRingBuffer_t SerialBuffer_UART1 =
    {
        .hal =
            {
                .IRQn = USART1_IRQn,
                .usart_x = USART1}};

static uint8_t uart1_rx_ring_buffer[UART1_RING_BUFFER_SIZE];
static uint8_t uart1_tx_ring_buffer[UART1_RING_BUFFER_SIZE];
static TcpUartBridge_t tcpUartBridge_1;

/////////////////////////////////////////////////////////////////////////
// Заготовка под канал 2. Когда понадобится - раскомментировать блок,
// завести соответствующий #define UART2_RING_BUFFER_SIZE, добавить в
// hw_interrupts.c обработчик USART2_IRQHandler с
// extern SerialRingBuffer_t SerialBuffer_UART2; (по аналогии с UART1,
// поэтому SerialBuffer_UART2 тоже объявлена без static), и добавить
// строку в таблицу g_channels[] ниже (APP_BRIDGE_CHANNEL_2).
/////////////////////////////////////////////////////////////////////////
#if 0
#define UART2_RING_BUFFER_SIZE 256
#define UART2_DEFAULT_BAUDRATE 115200u

SerialRingBuffer_t SerialBuffer_UART2 =
    {
        .hal =
            {
                .IRQn = USART2_IRQn,
                .usart_x = USART2}};

static uint8_t uart2_rx_ring_buffer[UART2_RING_BUFFER_SIZE];
static uint8_t uart2_tx_ring_buffer[UART2_RING_BUFFER_SIZE];
static TcpUartBridge_t tcpUartBridge_2;
#endif

/////////////////////////////////////////////////////////////////////////
// Таблица каналов. Один канал = один UART + один TCP-сокет + один мост.
// app_bridge_init()/process() и все геттеры/сеттеры работают через эту
// таблицу по AppBridgeChannelId_t, не зная деталей конкретного UART -
// поэтому добавление канала не требует правок в этой логике.
/////////////////////////////////////////////////////////////////////////
typedef struct
{
  SerialRingBuffer_t *srb;
  TcpUartBridge_t *bridge;
  uint8_t *rx_buf;
  uint16_t rx_buf_size;
  uint8_t *tx_buf;
  uint16_t tx_buf_size;
  uint8_t sock_id;
  uint16_t tcp_port;
  uint32_t baudrate; /* кэш текущей скорости - см. app_bridge_set_uart_baudrate() */
} AppBridgeChannel_t;

static AppBridgeChannel_t g_channels[APP_BRIDGE_CHANNEL_COUNT] =
    {
        [APP_BRIDGE_CHANNEL_1] =
            {
                .srb = &SerialBuffer_UART1,
                .bridge = &tcpUartBridge_1,
                .rx_buf = uart1_rx_ring_buffer,
                .rx_buf_size = UART1_RING_BUFFER_SIZE,
                .tx_buf = uart1_tx_ring_buffer,
                .tx_buf_size = UART1_RING_BUFFER_SIZE,
                .baudrate = UART1_DEFAULT_BAUDRATE,
            },
        /*
        [APP_BRIDGE_CHANNEL_2] =
            {
                .srb = &SerialBuffer_UART2,
                .bridge = &tcpUartBridge_2,
                .rx_buf = uart2_rx_ring_buffer,
                .rx_buf_size = UART2_RING_BUFFER_SIZE,
                .tx_buf = uart2_tx_ring_buffer,
                .tx_buf_size = UART2_RING_BUFFER_SIZE,
                .baudrate = UART2_DEFAULT_BAUDRATE,
            },
        */
};

static AppBridgeChannel_t *get_channel(AppBridgeChannelId_t id)
{
  if (id >= APP_BRIDGE_CHANNEL_COUNT)
    return 0;
  return &g_channels[id];
}

/////////////////////////////////////////////////////////////////////////
// Инициализация / главный цикл
/////////////////////////////////////////////////////////////////////////
int8_t app_bridge_init(w5500_t *net_dev, AppBridgeChannelId_t id, uint8_t sock_id, uint16_t tcp_port)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return -1;

  ch->sock_id = sock_id;
  ch->tcp_port = tcp_port;

  SerialRingBufferHAL_Init(ch->srb);
  SerialRingBuffer_Init(ch->srb,
                        ch->rx_buf, ch->rx_buf_size,
                        ch->tx_buf, ch->tx_buf_size);

  return tcp_uart_bridge_init(ch->bridge, net_dev, ch->srb, ch->sock_id, ch->tcp_port);
}

void app_bridge_process(void)
{
  for (uint8_t i = 0; i < APP_BRIDGE_CHANNEL_COUNT; i++)
  {
    tcp_uart_bridge_process(g_channels[i].bridge);
  }
}

uint8_t app_bridge_is_connected(AppBridgeChannelId_t id)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return 0;

  return tcp_uart_bridge_isConnected(ch->bridge);
}

/////////////////////////////////////////////////////////////////////////
// Параметры батчинга UART -> TCP - тонкие обертки над tcp_uart_bridge_*()
/////////////////////////////////////////////////////////////////////////
int8_t app_bridge_set_flush_delay_ms(AppBridgeChannelId_t id, uint32_t delay_ms)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return -1;

  return tcp_uart_bridge_set_flush_delay_ms(ch->bridge, delay_ms);
}

uint32_t app_bridge_get_flush_delay_ms(AppBridgeChannelId_t id)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return 0;

  return tcp_uart_bridge_get_flush_delay_ms(ch->bridge);
}

int8_t app_bridge_set_flush_size(AppBridgeChannelId_t id, uint16_t flush_size)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return -1;

  return tcp_uart_bridge_set_flush_size(ch->bridge, flush_size);
}

uint16_t app_bridge_get_flush_size(AppBridgeChannelId_t id)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return 0;

  return tcp_uart_bridge_get_flush_size(ch->bridge);
}

/////////////////////////////////////////////////////////////////////////
// Скорость UART.
//
// ВНИМАНИЕ: usart_baud_rate_set() - функция из AT32 firmware library по
// предположению об их типовом нейминге (usart_init/usart_baud_rate_set
// и т.п.). У меня нет содержимого вашего hw_config.h, поэтому:
//  - сверьте точное имя функции для вашей версии библиотеки;
//  - проверьте, не требует ли смена скорости "на лету" дополнительных
//    действий (например, usart_enable(usart_x, FALSE) перед и
//    usart_enable(usart_x, TRUE) после).
// ch->srb->hal.usart_x уже хранит указатель на нужный USARTx с тем
// типом, который использует ваш HAL - поэтому его тип здесь трогать
// не нужно.
/////////////////////////////////////////////////////////////////////////
int8_t app_bridge_set_uart_baudrate(AppBridgeChannelId_t id, uint32_t baudrate)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return -1;

  // Валидация: отсекаем заведомо нерабочие значения. Конкретные
  // границы поправьте под реально поддерживаемые вашим UART скорости.
  if (baudrate < 1200u || baudrate > 3000000u)
    return -2;

  usart_baud_rate_set(ch->srb->hal.usart_x, baudrate); /* TODO: сверить с hw_config.h */

  ch->baudrate = baudrate;
  return 0;
}

uint32_t app_bridge_get_uart_baudrate(AppBridgeChannelId_t id)
{
  AppBridgeChannel_t *ch = get_channel(id);
  if (!ch)
    return 0;

  // Возвращается закэшированное значение (то, что последним успешно
  // установлено через app_bridge_set_uart_baudrate() либо значение
  // по умолчанию из таблицы) - фактическое считывание из регистров
  // USART здесь не делается.
  return ch->baudrate;
}