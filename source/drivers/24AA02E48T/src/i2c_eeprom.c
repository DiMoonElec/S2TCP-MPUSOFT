#include <string.h>

#include "i2c_eeprom.h"
#include "i2c_hw.h"
#include "systick.h"

/******************************************************************************/

// 24AA02E48T-I/OT: 2 Kbit (256 байт) I2C EEPROM с преднастроенным EUI-48.
// В отличие от AT24MAC402, у этой микросхемы НЕТ отдельного блока Extended
// Memory со своим адресом устройства - весь массив висит на ОДНОМ 7-битном
// адресе. Выводов A0/A1/A2 у 24AA02E48/24AA02E64 нет (в отличие от
// 24AA025x), соответствующие биты control byte - "don't care", поэтому
// адрес устройства фиксирован: control byte 1010000 -> 0xA0 (write) / 0xA1
// (read при использовании как 8-битный адрес с битом R/W).
#define EEPROM_I2C_ADDR 0xA0

#define EEPROM_SIZE_BYTES 256
#define EEPROM_PAGE_SIZE 8 // 24AA02E48/24AA02E64: страница 8 байт (см. datasheet)

// Верхняя половина массива (0x80-0xFF) аппаратно write-protected навсегда,
// в ней же (0xFA-0xFF) зашит преднастроенный EUI-48 (OUI + Extension ID).
// Модуль работает с массивом единообразно - какой адрес относится к EUI-48,
// а какой к пользовательским данным, решает приложение (см. datasheet
// AT24AA02E48/E64, п.6.3 "Write Protection" и п.9 "Pre-Programmed
// EUI-48/EUI-64 Node Address").
#define EEPROM_WRITE_PROTECTED_START 0x80
#define EEPROM_EUI48_WORD_ADDR 0xFA
#define EEPROM_EUI48_LEN 6

// Таймаут одной I2C-транзакции: базовый запас + ~1 мс на байт
// (реальное время байта на 100 kHz ~90 мкс, запас более чем 10-кратный)
#define EEPROM_OP_TIMEOUT_BASE_MS 10

// Таймаут ожидания завершения внутреннего цикла записи страницы EEPROM.
// По datasheet (TWC, Table 1-2 AC CHARACTERISTICS): typ. 3 мс, max 5 мс.
// Здесь взято с запасом.
#define EEPROM_WRITE_CYCLE_TIMEOUT_MS 20

/******************************************************************************/

static uint32_t calc_op_timeout_ms(uint16_t len)
{
  return EEPROM_OP_TIMEOUT_BASE_MS + (uint32_t)len;
}

/*
  Ждёт завершения уже запущенной hw_i2c_* операции с контролем таймаута.
  При превышении таймаута принудительно прерывает операцию через
  hw_i2c_Abort().
*/
static i2c_eeprom_result_t wait_ready(uint32_t timeout_ms)
{
  uint32_t t0 = SYSTICK_GET_VALUE();

  while (!hw_i2c_IsRdy())
  {
    if ((SYSTICK_GET_VALUE() - t0) >= timeout_ms)
    {
      hw_i2c_Abort();
      return I2C_EEPROM_ERROR;
    }
  }

  return (hw_i2c_GetStatus() == HW_I2C_OK) ? I2C_EEPROM_OK : I2C_EEPROM_ERROR;
}

/*
  ACK-polling: после записи страницы EEPROM некоторое время не отвечает
  на свой адрес, пока не завершит внутренний цикл записи (см. п.7.0
  "Acknowledge Polling" datasheet). Пробный запрос - однобайтовая запись
  word address (безопасна, ничего не портит - EEPROM трактует первый байт
  после адреса устройства как указатель, а не данные).
  Ограничена общим таймаутом на весь цикл поллинга, а не на одну попытку.
*/
static i2c_eeprom_result_t wait_write_cycle_complete(uint32_t timeout_ms)
{
  uint32_t t0 = SYSTICK_GET_VALUE();
  uint8_t probe = 0;

  for (;;)
  {
    hw_i2c_Write(EEPROM_I2C_ADDR, &probe, 1);

    while (!hw_i2c_IsRdy())
    {
      if ((SYSTICK_GET_VALUE() - t0) >= timeout_ms)
      {
        hw_i2c_Abort();
        return I2C_EEPROM_ERROR;
      }
    }

    hw_i2c_status_t st = hw_i2c_GetStatus();

    if (st == HW_I2C_OK)
      return I2C_EEPROM_OK; // микросхема подтвердила адрес - запись завершена

    if (st != HW_I2C_ERR_NACK_ADDR)
      return I2C_EEPROM_ERROR; // любая другая ошибка - не признак "занято записью"

    if ((SYSTICK_GET_VALUE() - t0) >= timeout_ms)
      return I2C_EEPROM_ERROR;

    // NACK на адрес - микросхема ещё занята внутренней записью, повтор
  }
}

/******************************************************************************/

i2c_eeprom_result_t i2c_eeprom_read(uint8_t addr, uint8_t *buf, uint16_t len)
{
  if ((uint32_t)addr + len > EEPROM_SIZE_BYTES)
    return I2C_EEPROM_ERROR;

  if (len == 0)
    return I2C_EEPROM_OK;

  uint8_t word_addr = addr;

  hw_i2c_WriteRead(EEPROM_I2C_ADDR, &word_addr, 1, buf, len);

  return wait_ready(calc_op_timeout_ms(len));
}

/*
  Запись работает со всей областью памяти одинаково - в т.ч. с
  write-protected диапазоном 0x80-0xFF, где зашит EUI-48 (0xFA-0xFF).
  Попытка записи в этот диапазон аппаратно не изменит содержимое чипа
  (устройство просто не подтвердит запись данных, как описано в п.6.3
  datasheet); модуль намеренно не проверяет это на своём уровне - какие
  адреса трогать, решает приложение.
*/
i2c_eeprom_result_t i2c_eeprom_write(uint8_t addr, const uint8_t *buf, uint16_t len)
{
  if ((uint32_t)addr + len > EEPROM_SIZE_BYTES)
    return I2C_EEPROM_ERROR;

  while (len > 0)
  {
    uint8_t chunk[1 + EEPROM_PAGE_SIZE];

    uint16_t page_remaining = EEPROM_PAGE_SIZE - (addr % EEPROM_PAGE_SIZE);
    uint16_t chunk_len = (len < page_remaining) ? len : page_remaining;

    chunk[0] = addr;
    memcpy(&chunk[1], buf, chunk_len);

    hw_i2c_Write(EEPROM_I2C_ADDR, chunk, (uint16_t)(1 + chunk_len));

    if (wait_ready(calc_op_timeout_ms(chunk_len)) != I2C_EEPROM_OK)
      return I2C_EEPROM_ERROR;

    if (wait_write_cycle_complete(EEPROM_WRITE_CYCLE_TIMEOUT_MS) != I2C_EEPROM_OK)
      return I2C_EEPROM_ERROR;

    addr += chunk_len;
    buf += chunk_len;
    len -= chunk_len;
  }

  return I2C_EEPROM_OK;
}