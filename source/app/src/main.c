#include <stdint.h>
#include "at32f413.h"
#include "system_clock.h"
#include "systick.h"
#include "hw_config.h"
#include "w5500.h"
#include "modbus_tcp_server.h"
#include "modbus_reg_model.h"
#include "app_bridge.h"
#include "i2c_hw.h"
#include "settings.h"

/******************************************************************************/

static w5500_t w5500_device;

uint8_t g_net_mac[6];
uint8_t g_net_subnet[4];
uint8_t g_net_ip[4];
uint8_t g_net_gateway[4];

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

  hw_i2c_init();

  settings_load();

  w5500_init(&w5500_device, 0);
  delay_ms(1000);
  wiznet_net_config();

  modbus_tcp_serv_init(&w5500_device);

  ModbusRegModelInit();

  /// Инициализация UART<->TCP моста для UART1. При добавлении UART2 -
  /// еще один вызов с APP_BRIDGE_CHANNEL_UART2 и его sock_id/tcp_port.
  app_bridge_init(&w5500_device, APP_BRIDGE_CHANNEL_1, 1, 950);

  for (;;)
  {
    modbus_tcp_serv_process();
    app_bridge_process();
  }
}