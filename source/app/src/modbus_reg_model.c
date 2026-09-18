#include "modbus_reg_model.h"
#include "ModbusSlave/public-api.h"
#include <stdint.h>
#include "at32f413.h"

/********************************************************************/

/* Битовые маски источников сброса (соответствуют флагам регистра CRM->CTRLSTS) */
#define RESET_FLAG_LPRSTF     (1U << 0)   /* Сброс из режима пониженного энергопотребления */
#define RESET_FLAG_WWDTRSTF   (1U << 1)   /* Сброс от оконного сторожевого таймера (WWDT) */
#define RESET_FLAG_WDTRSTF    (1U << 2)   /* Сброс от независимого сторожевого таймера (WDT) */
#define RESET_FLAG_SWRSTF     (1U << 3)   /* Программный сброс */
#define RESET_FLAG_PORRSTF    (1U << 4)   /* Сброс по включению питания (POR/PDR) */
#define RESET_FLAG_NRSTF      (1U << 5)   /* Аппаратный сброс по выводу NRST */

/*
 * Возвращает битовое поле активных флагов сброса из регистра CRM->CTRLSTS.
 * Биты соответствуют определениям RESET_FLAG_xxx выше.
 * Флаги регистра НЕ очищаются автоматически — вызывающий код
 * должен сам решить, когда вызвать очистку (см. reset_flags_clear()).
 */
uint16_t get_reset_source_raw(void)
{
    uint16_t flags = 0;

    if (CRM->ctrlsts_bit.lprstf)
        flags |= RESET_FLAG_LPRSTF;
    if (CRM->ctrlsts_bit.wwdtrstf)
        flags |= RESET_FLAG_WWDTRSTF;
    if (CRM->ctrlsts_bit.wdtrstf)
        flags |= RESET_FLAG_WDTRSTF;
    if (CRM->ctrlsts_bit.swrstf)
        flags |= RESET_FLAG_SWRSTF;
    if (CRM->ctrlsts_bit.porrstf)
        flags |= RESET_FLAG_PORRSTF;
    if (CRM->ctrlsts_bit.nrstf)
        flags |= RESET_FLAG_NRSTF;

    return flags;
}

/* Отдельно вынесенная очистка флагов сброса (запись 1 в RSTFC) */
void reset_flags_clear(void)
{
    CRM->ctrlsts_bit.rstfc = 1;
}

static uint16_t pack_cntr = 0;

/********************************************************************/

/*
  Запрос на чтение от master
*/
static uint16_t reg_model_get(uint16_t reg)
{
  switch (reg)
  {
  case 0:
    return get_reset_source_raw();
    
  case 1:
    pack_cntr++;
    return pack_cntr;
  }

  return 0; // По умолчанию несуществующий регистр читается как 0
}

/*
  Запрос на запись от master
*/
static void reg_model_set(uint16_t reg, uint16_t value)
{
  switch(reg)
  {
  case 0:
    if(value == 0)
       reset_flags_clear();
    break;
  }
}

/********************************************************************/

void ModbusRegModelInit(void)
{
  pack_cntr = 0;
  // Устанавливаем callback-функции записи/чтения регистров.
  modbus_slave_set_read_holding_reg_callback(reg_model_get);
  modbus_slave_set_write_holding_reg_callback(reg_model_set);
}