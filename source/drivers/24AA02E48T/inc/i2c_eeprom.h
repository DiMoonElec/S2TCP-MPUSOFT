#ifndef __I2C_EEPROM_H__
#define __I2C_EEPROM_H__

#include <stdint.h>

#define EEPROM_EUI48_WORD_ADDR 0xFA
#define EEPROM_EUI48_LEN 6

typedef enum
{
  I2C_EEPROM_OK = 0,
  I2C_EEPROM_ERROR
} i2c_eeprom_result_t;

/*
  Блокирующие функции. Каждая содержит внутренний контроль таймаута:
  при аппаратной неисправности (линия прижата к земле и т.п.)
  функция не зависает навсегда, а прерывает операцию через
  hw_i2c_Abort() и возвращает I2C_EEPROM_ERROR.
*/

// addr - адрес ячейки в основном блоке EEPROM (0..255)
i2c_eeprom_result_t i2c_eeprom_read(uint8_t addr, uint8_t *buf, uint16_t len);
i2c_eeprom_result_t i2c_eeprom_write(uint8_t addr, const uint8_t *buf, uint16_t len);

#endif