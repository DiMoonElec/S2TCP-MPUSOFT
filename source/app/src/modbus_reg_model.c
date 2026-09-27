#include "modbus_reg_model.h"
#include "ModbusSlave/public-api.h"
#include <stdint.h>
#include "app_bridge.h"
#include "main.h"
#include "settings.h"
#include "at32f413_misc.h"

/********************************************************************/

// Сетевые регистры
#define REG_SAVE_EEPROM 0

#define REG_REBOOT 2

#define REG_NET_IP_0 4
#define REG_NET_IP_1 5
#define REG_NET_IP_2 6
#define REG_NET_IP_3 7

#define REG_NET_SUBNET_0 8
#define REG_NET_SUBNET_1 9
#define REG_NET_SUBNET_2 10
#define REG_NET_SUBNET_3 11

#define REG_NET_GATEWAY_0 12
#define REG_NET_GATEWAY_1 13
#define REG_NET_GATEWAY_2 14
#define REG_NET_GATEWAY_3 15

#define REG_BRIGE_1_FLUSH_DELAY 16
#define REG_BRIGE_1_FLUSH_SIZE 18

/********************************************************************/

/*
  Запрос на чтение от master
*/
static uint16_t reg_model_get(uint16_t reg)
{
  switch (reg)
  {
    // --- Сетевые регистры ---
  case REG_NET_IP_0:
    return g_net_ip[0];
  case REG_NET_IP_1:
    return g_net_ip[1];
  case REG_NET_IP_2:
    return g_net_ip[2];
  case REG_NET_IP_3:
    return g_net_ip[3];
  ////////////////////////////////////
  case REG_NET_SUBNET_0:
    return g_net_subnet[0];
  case REG_NET_SUBNET_1:
    return g_net_subnet[1];
  case REG_NET_SUBNET_2:
    return g_net_subnet[2];
  case REG_NET_SUBNET_3:
    return g_net_subnet[3];
  ////////////////////////////////////
  case REG_NET_GATEWAY_0:
    return g_net_gateway[0];
  case REG_NET_GATEWAY_1:
    return g_net_gateway[1];
  case REG_NET_GATEWAY_2:
    return g_net_gateway[2];
  case REG_NET_GATEWAY_3:
    return g_net_gateway[3];

  case REG_BRIGE_1_FLUSH_DELAY:
    return (uint16_t)app_bridge_get_flush_delay_ms(APP_BRIDGE_CHANNEL_1);

  case REG_BRIGE_1_FLUSH_SIZE:
    return app_bridge_get_flush_size(APP_BRIDGE_CHANNEL_1);
  }

  return 0; // По умолчанию несуществующий регистр читается как 0
}

/*
  Запрос на запись от master
*/
static void reg_model_set(uint16_t reg, uint16_t value)
{
  switch (reg)
  {
  case REG_SAVE_EEPROM:
    if (value != 0)
      settings_save();
    break;

  case REG_REBOOT:
    if (value != 0)
      nvic_system_reset();
    break;

  case REG_NET_IP_0:
    g_net_ip[0] = (uint8_t)value;
    break;
  case REG_NET_IP_1:
    g_net_ip[1] = (uint8_t)value;
    break;
  case REG_NET_IP_2:
    g_net_ip[2] = (uint8_t)value;
    break;
  case REG_NET_IP_3:
    g_net_ip[3] = (uint8_t)value;
    break;
  ////////////////////////////////////
  case REG_NET_SUBNET_0:
    g_net_subnet[0] = (uint8_t)value;
    break;
  case REG_NET_SUBNET_1:
    g_net_subnet[1] = (uint8_t)value;
    break;
  case REG_NET_SUBNET_2:
    g_net_subnet[2] = (uint8_t)value;
    break;
  case REG_NET_SUBNET_3:
    g_net_subnet[3] = (uint8_t)value;
    break;
  ////////////////////////////////////
  case REG_NET_GATEWAY_0:
    g_net_gateway[0] = (uint8_t)value;
    break;
  case REG_NET_GATEWAY_1:
    g_net_gateway[1] = (uint8_t)value;
    break;
  case REG_NET_GATEWAY_2:
    g_net_gateway[2] = (uint8_t)value;
    break;
  case REG_NET_GATEWAY_3:
    g_net_gateway[3] = (uint8_t)value;
    break;

  case REG_BRIGE_1_FLUSH_DELAY:
    app_bridge_set_flush_delay_ms(APP_BRIDGE_CHANNEL_1, (uint32_t)value);
    break;

  case REG_BRIGE_1_FLUSH_SIZE:
    app_bridge_set_flush_size(APP_BRIDGE_CHANNEL_1, value);
    break;
  }
}

/********************************************************************/

void ModbusRegModelInit(void)
{
  // Устанавливаем callback-функции записи/чтения регистров.
  modbus_slave_set_read_holding_reg_callback(reg_model_get);
  modbus_slave_set_write_holding_reg_callback(reg_model_set);
}