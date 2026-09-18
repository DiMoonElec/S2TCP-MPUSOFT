/*
  Драйвер W5500
  Autor: DiMoon Electronics
  Date: 2024/06/04
*/

#ifndef __W5500_H__
#define __W5500_H__

#include <stdint.h>

#define W5500_SPI_HEADER_SIZE 3

/******************************************************************************/

typedef struct
{
  uint8_t busID;
} w5500_t;


/*
typedef struct
{
  uint8_t state;
  uint8_t flag_ready;
  uint16_t size;
  uint16_t bufferSize;
  uint16_t offset;
  uint8_t *buff;
  w5500_t *c;

} w5500_exchange_context_t;
*/
/******************************************************************************/

//typedef void (*w5500_exchange_completed_callback_t)(w5500_exchange_context_t *c);

/******************************************************************************/

int8_t w5500_init(w5500_t *c, uint8_t busID);

int8_t w5500_set_mac(w5500_t *c, uint8_t *mac);
int8_t w5500_get_mac(w5500_t *c, uint8_t *mac);

int8_t w5500_set_subnet(w5500_t *c, uint8_t *subnet);
int8_t w5500_set_ip(w5500_t *c, uint8_t *ip);
int8_t w5500_set_gateway(w5500_t *c, uint8_t *gateway);

int8_t w5500_set_phycfgr(w5500_t *c, uint8_t phycfgr);
int8_t w5500_get_phycfgr(w5500_t *c, uint8_t *phycfgr);

int8_t w5500_set_txbuf_sizes(w5500_t *c, uint8_t *txbuf_sizes);
int8_t w5500_get_txbuf_sizes(w5500_t *c, uint8_t *txbuf_sizes);

#endif