#ifndef __APP_BRIDGE_H__
#define __APP_BRIDGE_H__

#include <stdint.h>
#include "w5500.h"

/////////////////////////////////////////////////////////////////////////
// Модуль владеет всеми TCP<->UART мостами приложения (буферы UART,
// SerialRingBuffer_t и TcpUartBridge_t на каждый канал), инициализирует
// их и прокручивает их process() из главного цикла. Наружу отдает
// компактный API по идентификатору канала - вызывающему коду (main.c,
// модбас-регистрам настроек и т.п.) не нужно знать про внутренние
// структуры конкретного моста.
//
// Добавление нового моста (например, UART2):
//   1) добавить идентификатор в AppBridgeChannelId_t ниже;
//   2) в app_bridge.c завести буферы/SerialRingBuffer_t/TcpUartBridge_t
//      и добавить запись в таблицу g_channels[] (по аналогии с
//      APP_BRIDGE_CHANNEL_1);
//   3) если у нового UART есть прерывание (см. hw_interrupts.c) - убрать
//      static у его SerialBuffer_UARTx и добавить туда extern (см.
//      комментарий в app_bridge.c про SerialBuffer_UART1);
//   4) init/process/геттеры/сеттеры ничего менять не требуют - они уже
//      работают по идентификатору и переберают все каналы сами.
/////////////////////////////////////////////////////////////////////////

typedef enum
{
  APP_BRIDGE_CHANNEL_1 = 0,
  /* APP_BRIDGE_CHANNEL_2, */

  APP_BRIDGE_CHANNEL_COUNT
} AppBridgeChannelId_t;

/* Инициализирует один канал: UART (HAL + кольцевые буферы), TCP-сокет
(sock_id/tcp_port) и сам мост. Вызывать по одному разу на каждый канал
(при добавлении UART2 - еще один вызов с APP_BRIDGE_CHANNEL_2 и его
sock_id/tcp_port). net_dev - уже инициализированное устройство w5500
(w5500_init() и сетевые настройки должны быть выполнены до вызова этой
функции, как и раньше в main()). Возврат: 0 - ок, -1 - неверный id. */
int8_t app_bridge_init(w5500_t *net_dev, AppBridgeChannelId_t id, uint8_t sock_id, uint16_t tcp_port);

/* Вызывать из главного цикла на каждой итерации - прокручивает
tcp_uart_bridge_process() всех каналов. */
void app_bridge_process(void);

uint8_t app_bridge_is_connected(AppBridgeChannelId_t id);

/////////////////////////////////////////////////////////////////////////
// Параметры батчинга UART -> TCP (см. tcp_uart_bridge.h).
// Возврат как у tcp_uart_bridge_set_*(): 0 - ок, -1 - неверный id,
// -2 - значение не прошло валидацию (не применено).
/////////////////////////////////////////////////////////////////////////
int8_t app_bridge_set_flush_delay_ms(AppBridgeChannelId_t id, uint32_t delay_ms);
uint32_t app_bridge_get_flush_delay_ms(AppBridgeChannelId_t id);
int8_t app_bridge_set_flush_size(AppBridgeChannelId_t id, uint16_t flush_size);
uint16_t app_bridge_get_flush_size(AppBridgeChannelId_t id);

/////////////////////////////////////////////////////////////////////////
// Скорость UART.
//
// ВНИМАНИЕ: реализация в app_bridge.c дергает usart_baud_rate_set() из
// AT32 firmware library - это предположение по типовому неймингу
// библиотек Artery/AT32 (у меня нет содержимого вашего hw_config.h,
// чтобы проверить точное имя функции и нужна ли остановка/перезапуск
// USART вокруг смены скорости). Сверьте с вашей версией библиотеки и
// поправьте вызов внутри app_bridge_set_uart_baudrate() при необходимости.
/////////////////////////////////////////////////////////////////////////
int8_t app_bridge_set_uart_baudrate(AppBridgeChannelId_t id, uint32_t baudrate);
uint32_t app_bridge_get_uart_baudrate(AppBridgeChannelId_t id);

#endif /* __APP_BRIDGE_H__ */