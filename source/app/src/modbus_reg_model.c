#include "modbus_reg_model.h"
#include "ModbusSlave/public-api.h"
#include <stdint.h>
#include "app_bridge.h"

/********************************************************************/

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