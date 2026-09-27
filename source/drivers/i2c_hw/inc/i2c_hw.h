#ifndef __I2C_HW_H
#define __I2C_HW_H

#include <stdint.h>

typedef enum
{
  HW_I2C_OK = 0,
  HW_I2C_BUSY,           // транзакция ещё не завершена (используется как исходное значение)
  HW_I2C_ERR_NACK_ADDR,  // слейв не подтвердил адрес
  HW_I2C_ERR_NACK_DATA,  // слейв не подтвердил байт данных
  HW_I2C_ERR_BUS,        // BERR / ARLO / OVR
  HW_I2C_ERR_ABORTED     // операция прервана извне через hw_i2c_Abort()
} hw_i2c_status_t;

void hw_i2c_init(void);

// true, когда периферия свободна и можно запускать новую транзакцию
uint8_t hw_i2c_IsRdy(void);

/*
  Принудительно прерывает текущую операцию и приводит периферию I2C1
  в исходное состояние (программный сброс, см. RM0038 26.6.1 SWRST -
  "can be used to reinitialize the peripheral after an error or a
  locked state"). Предназначена для вызова внешним кодом по таймауту,
  когда hw_i2c_IsRdy() не становится true слишком долго.
*/
void hw_i2c_Abort(void);

// Результат последней завершённой транзакции (валиден после hw_i2c_IsRdy()==1)
hw_i2c_status_t hw_i2c_GetStatus(void);

// Запись len байт из buf по адресу addr7 (7-битный, БЕЗ сдвига,
// т.е. для EEPROM с адресным блоком 0xA0 сюда передаётся 0x50)
void hw_i2c_Write(uint8_t addr7, const uint8_t *buf, uint16_t len);

// Чтение len байт в buf по адресу addr7
void hw_i2c_Read(uint8_t addr7, uint8_t *buf, uint16_t len);

// Запись tx_buf[tx_len] с последующим Repeated Start и чтением
// rx_buf[rx_len] - типовой сценарий "указать регистр -> прочитать данные"
// для EEPROM и большинства датчиков (термометров и т.п.)
void hw_i2c_WriteRead(uint8_t addr7,
                       const uint8_t *tx_buf, uint16_t tx_len,
                       uint8_t *rx_buf, uint16_t rx_len);

#endif /* __I2C_HW_H */