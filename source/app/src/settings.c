#include "settings.h"
#include "crc8.h"
#include "i2c_eeprom.h"
#include <string.h>
#include <assert.h>
#include "main.h"
#include "app_bridge.h"

/* Адрес в EEPROM, с которого начинаются настройки */
#define SETTINGS_EEPROM_ADDR 0x00

/* Упаковка структуры без выравнивания по байтам */
#pragma pack(push, 1)
typedef struct
{
  /* 1. Полезная нагрузка (поля настроек) */
  uint8_t ip[4];
  uint8_t subnet[4];
  uint8_t gateway[4];

  uint32_t flush_delay_bridge1;
  uint16_t flush_size_bridge1;

  uint8_t rezerved1;
  uint8_t rezerved2;
  uint8_t rezerved3;
  uint8_t rezerved4;
  uint8_t rezerved5;
  uint8_t rezerved6;
  uint8_t rezerved7;
  uint8_t rezerved8;
  uint8_t rezerved9;
  uint8_t rezerved10;
  uint8_t rezerved11;
  uint8_t rezerved12;
  uint8_t rezerved13;
  uint8_t rezerved14;
  uint8_t rezerved15;

  /* 2. Контрольная сумма ДОЛЖНА БЫТЬ ПОСЛЕДНИМ ПОЛЕМ */
  uint8_t crc;
} settings_storage_t;
#pragma pack(pop)

/* Проверка размера структуры settings_storage_t во время компиляции */
static_assert(sizeof(settings_storage_t) <= 128, "settings_storage_t exceeds EEPROM user area size (128 bytes)!");

/* -------------------------------------------------------------------------
 * Приватные функции-помощники (Helper functions)
 * ------------------------------------------------------------------------- */

/* Подсчет CRC8 для всей структуры настроек за исключением самого поля crc */
static uint8_t settings_calculate_crc(const settings_storage_t *storage)
{
  size_t payload_size = sizeof(settings_storage_t) - sizeof(storage->crc);
  return Crc8((unsigned char *)storage, (unsigned char)payload_size);
}

/* Заполняет структуру базовыми (дефолтными) значениями */
static void settings_set_defaults(settings_storage_t *storage)
{
  memset(storage, 0, sizeof(settings_storage_t));

  /* Задаем значения по умолчанию */
  storage->ip[0] = 192;
  storage->ip[1] = 168;
  storage->ip[2] = 1;
  storage->ip[3] = 55;

  storage->subnet[0] = 255;
  storage->subnet[1] = 255;
  storage->subnet[2] = 255;
  storage->subnet[3] = 0;

  storage->gateway[0] = 192;
  storage->gateway[1] = 168;
  storage->gateway[2] = 1;
  storage->gateway[3] = 1;

  storage->flush_delay_bridge1 = app_bridge_get_flush_delay_ms(APP_BRIDGE_CHANNEL_1);
  storage->flush_size_bridge1 = app_bridge_get_flush_size(APP_BRIDGE_CHANNEL_1);
}

/* Собирает данные из глобальных переменных/модулей системы в структуру */
static void settings_pack_from_system(settings_storage_t *storage)
{
  memset(storage, 0, sizeof(settings_storage_t));

  storage->ip[0] = g_net_ip[0];
  storage->ip[1] = g_net_ip[1];
  storage->ip[2] = g_net_ip[2];
  storage->ip[3] = g_net_ip[3];

  storage->subnet[0] = g_net_subnet[0];
  storage->subnet[1] = g_net_subnet[1];
  storage->subnet[2] = g_net_subnet[2];
  storage->subnet[3] = g_net_subnet[3];

  storage->gateway[0] = g_net_gateway[0];
  storage->gateway[1] = g_net_gateway[1];
  storage->gateway[2] = g_net_gateway[2];
  storage->gateway[3] = g_net_gateway[3];

  storage->flush_delay_bridge1 = app_bridge_get_flush_delay_ms(APP_BRIDGE_CHANNEL_1);
  storage->flush_size_bridge1 = app_bridge_get_flush_size(APP_BRIDGE_CHANNEL_1);
}

/* Применяет данные из структуры к рабочим переменным системы*/
static void settings_apply_to_system(const settings_storage_t *storage)
{
  g_net_ip[0] = storage->ip[0];
  g_net_ip[1] = storage->ip[1];
  g_net_ip[2] = storage->ip[2];
  g_net_ip[3] = storage->ip[3];

  g_net_subnet[0] = storage->subnet[0];
  g_net_subnet[1] = storage->subnet[1];
  g_net_subnet[2] = storage->subnet[2];
  g_net_subnet[3] = storage->subnet[3];

  g_net_gateway[0] = storage->gateway[0];
  g_net_gateway[1] = storage->gateway[1];
  g_net_gateway[2] = storage->gateway[2];
  g_net_gateway[3] = storage->gateway[3];

  app_bridge_set_flush_delay_ms(APP_BRIDGE_CHANNEL_1, storage->flush_delay_bridge1);
  app_bridge_set_flush_size(APP_BRIDGE_CHANNEL_1, storage->flush_size_bridge1);
}

/* -------------------------------------------------------------------------
 * Реализация публичного API
 * ------------------------------------------------------------------------- */

/* Загрузка настроек из EEPROM с валидацией и откатом на дефолты */
bool settings_load(void)
{
  settings_storage_t storage;
  bool is_read_ok = false;

  /* Чтение MAC */
  if (I2C_EEPROM_OK != i2c_eeprom_read(EEPROM_EUI48_WORD_ADDR, g_net_mac, sizeof(g_net_mac)))
  {
    g_net_mac[0] = 0x02;
    g_net_mac[1] = 0x00;
    g_net_mac[2] = 0xAA;
    g_net_mac[3] = 0xBB;
    g_net_mac[4] = 0xCC;
    g_net_mac[5] = 0xDD;
  }

  /* 1. Пытаемся прочитать данные из EEPROM */
  if (I2C_EEPROM_OK == i2c_eeprom_read(SETTINGS_EEPROM_ADDR,
                                       (uint8_t *)&storage,
                                       sizeof(settings_storage_t)))
  {
    /* 2. Проверяем контрольную сумму */
    uint8_t calculated_crc = settings_calculate_crc(&storage);

    if (calculated_crc == storage.crc)
    {
      is_read_ok = true;
    }
  }

  /* 3. Обработка ошибки: если чтение фейлилось или CRC битый */
  if (!is_read_ok)
  {
    settings_set_defaults(&storage);

    /* Дополнительно: можно сразу перезаписать EEPROM правильными дефолтами,
       чтобы восстановить её при первом старте/после сбоя */
    // i2c_eeprom_write(SETTINGS_EEPROM_ADDR, (uint8_t *)&storage, sizeof(settings_storage_t));
  }

  /* 4. Единая точка применения настроек в систему */
  settings_apply_to_system(&storage);

  return is_read_ok;
}

/* Сбор и сохранение текущих настроек системы в EEPROM */
bool settings_save(void)
{
  settings_storage_t storage;

  /* 1. Собираем свежие данные из системы и считаем CRC */
  settings_pack_from_system(&storage);

  storage.crc = settings_calculate_crc(&storage);

  /* 2. Пишем готовую структуру одним блоком в EEPROM */
  if (I2C_EEPROM_OK == i2c_eeprom_write(SETTINGS_EEPROM_ADDR,
                                        (const uint8_t *)&storage,
                                        sizeof(settings_storage_t)))
  {
    return true;
  }

  return false;
}