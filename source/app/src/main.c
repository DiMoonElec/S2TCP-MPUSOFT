#include <stdint.h>
#include "at32f413.h"
#include "system_clock.h"
#include "systick.h"
#include "hw_config.h"
#include "w5500.h"
#include "modbus_tcp_server.h"
#include "modbus_reg_model.h"
#include "SerialRingBuffer\public-api.h"
#include "serial_ring_buffer_hal.h"
#include "tcp_uart_bridge.h"

/******************************************************************************/

static w5500_t w5500_device;

static uint8_t g_net_mac[6] = {0x02, 0x00, 0xAA, 0xBB, 0xCC, 0xDD};
static uint8_t g_net_subnet[4] = {255, 255, 255, 0};
static uint8_t g_net_ip[4] = {192, 168, 88, 128};
static uint8_t g_net_gateway[4] = {192, 168, 88, 1};

/******************************************************************************/

#define UART1_RING_BUFFER_SIZE 2048

SerialRingBuffer_t SerialBuffer_UART1 =
    {
        .hal =
            {
                .IRQn = USART1_IRQn,
                .usart_x = USART1}};

static uint8_t uart1_rx_ring_buffer[UART1_RING_BUFFER_SIZE];
static uint8_t uart1_tx_ring_buffer[UART1_RING_BUFFER_SIZE];

/******************************************************************************/

static TcpUartBridge_t tcpUartBridge_1;

/******************************************************************************/

static void wiznet_net_config(void)
{
  /// Сетевые настройки w5500
  w5500_set_mac(&w5500_device, g_net_mac);
  w5500_set_subnet(&w5500_device, g_net_subnet);
  w5500_set_ip(&w5500_device, g_net_ip);
  w5500_set_gateway(&w5500_device, g_net_gateway);
}

/******************************************************************************/

void main(void)
{
  wk_system_clock_config();
  hw_config();

  w5500_init(&w5500_device, 0);
  delay_ms(1000);
  wiznet_net_config();

  modbus_tcp_serv_init(&w5500_device);

  ModbusRegModelInit();

  /// Инициализация драйвера UART1
  SerialRingBufferHAL_Init(&SerialBuffer_UART1);
  SerialRingBuffer_Init(&SerialBuffer_UART1,
                        uart1_rx_ring_buffer, UART1_RING_BUFFER_SIZE,
                        uart1_tx_ring_buffer, UART1_RING_BUFFER_SIZE);

  tcp_uart_bridge_init(&tcpUartBridge_1, &w5500_device, &SerialBuffer_UART1, 1, 950);

  for (;;)
  {
    modbus_tcp_serv_process();
    tcp_uart_bridge_process(&tcpUartBridge_1);
  }
}