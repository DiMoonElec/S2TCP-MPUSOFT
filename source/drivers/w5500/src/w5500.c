#include "w5500.h"
#include "w5500_def.h"
#include "w5500_port.h"
#include "w5500_exchange.h"

/******************************************************************************/

#define W5500_BUFF_DATA 3
#define W5500_BUFF_HEADER 0

/******************************************************************************/

int8_t w5500_init(w5500_t *c, uint8_t busID)
{
  c->busID = busID;

  if (w5500_exchange_init(c) != 0)
    return -1;

  return 0;
}

/******************************************************************************/

int8_t w5500_set_mac(w5500_t *c, uint8_t *mac)
{
  uint8_t buffer[W5500_SPI_HEADER_SIZE + 6];
  buffer[W5500_BUFF_DATA + 0] = mac[0];
  buffer[W5500_BUFF_DATA + 1] = mac[1];
  buffer[W5500_BUFF_DATA + 2] = mac[2];
  buffer[W5500_BUFF_DATA + 3] = mac[3];
  buffer[W5500_BUFF_DATA + 4] = mac[4];
  buffer[W5500_BUFF_DATA + 5] = mac[5];

  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));
  if (w5500_exchange_begin(&context, W5500_BSB_COMMON, W5500_SR_MAC, 1, 6) != 0)
    return -1;

  while (!w5500_exchange_process(&context))
    ;

  return 0;
}

int8_t w5500_get_mac(w5500_t *c, uint8_t *mac)
{
  uint8_t buffer[W5500_SPI_HEADER_SIZE + 6];
  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));

  if (w5500_exchange_begin(&context, W5500_BSB_COMMON, W5500_SR_MAC, 0, 6) != 0)
    return -1;

  while (!w5500_exchange_process(&context))
    ;

  mac[0] = buffer[W5500_BUFF_DATA + 0];
  mac[1] = buffer[W5500_BUFF_DATA + 1];
  mac[2] = buffer[W5500_BUFF_DATA + 2];
  mac[3] = buffer[W5500_BUFF_DATA + 3];
  mac[4] = buffer[W5500_BUFF_DATA + 4];
  mac[5] = buffer[W5500_BUFF_DATA + 5];

  return 0;
}

int8_t w5500_set_subnet(w5500_t *c, uint8_t *subnet)
{
  uint8_t buffer[W5500_SPI_HEADER_SIZE + 4];
  buffer[W5500_BUFF_DATA + 0] = subnet[0];
  buffer[W5500_BUFF_DATA + 1] = subnet[1];
  buffer[W5500_BUFF_DATA + 2] = subnet[2];
  buffer[W5500_BUFF_DATA + 3] = subnet[3];

  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));
  if (w5500_exchange_begin(&context, W5500_BSB_COMMON, W5500_SR_SUBNET, 1, 4) != 0)
    return -1;

  while (!w5500_exchange_process(&context))
    ;

  return 0;
}

int8_t w5500_set_ip(w5500_t *c, uint8_t *ip)
{
  uint8_t buffer[W5500_SPI_HEADER_SIZE + 4];
  buffer[W5500_BUFF_DATA + 0] = ip[0];
  buffer[W5500_BUFF_DATA + 1] = ip[1];
  buffer[W5500_BUFF_DATA + 2] = ip[2];
  buffer[W5500_BUFF_DATA + 3] = ip[3];

  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));
  if (w5500_exchange_begin(&context, W5500_BSB_COMMON, W5500_SR_IP, 1, 4) != 0)
    return -1;

  while (!w5500_exchange_process(&context))
    ;

  return 0;
}

int8_t w5500_set_gateway(w5500_t *c, uint8_t *gateway)
{
  uint8_t buffer[W5500_SPI_HEADER_SIZE + 4];
  buffer[W5500_BUFF_DATA + 0] = gateway[0];
  buffer[W5500_BUFF_DATA + 1] = gateway[1];
  buffer[W5500_BUFF_DATA + 2] = gateway[2];
  buffer[W5500_BUFF_DATA + 3] = gateway[3];

  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));
  if (w5500_exchange_begin(&context, W5500_BSB_COMMON, W5500_SR_GATEWAY, 1, 4) != 0)
    return -1;

  while (!w5500_exchange_process(&context))
    ;

  return 0;
}

int8_t w5500_set_phycfgr(w5500_t *c, uint8_t phycfgr)
{
  uint8_t buffer[W5500_SPI_HEADER_SIZE + 1];
  buffer[W5500_BUFF_DATA + 0] = phycfgr;

  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));
  if (w5500_exchange_begin(&context, W5500_BSB_COMMON, W5500_SR_PHYCFGR, 1, 1) != 0)
    return -1;

  while (!w5500_exchange_process(&context))
    ;

  return 0;
}

int8_t w5500_get_phycfgr(w5500_t *c, uint8_t *phycfgr)
{
  uint8_t buffer[W5500_SPI_HEADER_SIZE + 1];
  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));

  if (w5500_exchange_begin(&context, W5500_BSB_COMMON, W5500_SR_PHYCFGR, 0, 1) != 0)
    return -1;

  while (!w5500_exchange_process(&context))
    ;

  *phycfgr = buffer[W5500_BUFF_DATA + 0];

  return 0;
}

int8_t w5500_set_txbuf_sizes(w5500_t *c, uint8_t *txbuf_sizes)
{
  const uint8_t bsb_sockets[] = {
      W5500_BSB_SOCKET0, W5500_BSB_SOCKET1, W5500_BSB_SOCKET2, W5500_BSB_SOCKET3,
      W5500_BSB_SOCKET4, W5500_BSB_SOCKET5, W5500_BSB_SOCKET6, W5500_BSB_SOCKET7};

  uint8_t buffer[W5500_SPI_HEADER_SIZE + 1];
  w5500_exchange_context_t context;

  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));

  for (int i = 0; i < 8; i++)
  {
    buffer[W5500_BUFF_DATA + 0] = txbuf_sizes[i];

    if (w5500_exchange_begin(&context, bsb_sockets[i], W5500_Sn_TXBUF_SIZE, 1, 1) != 0)
      return -1;

    while (!w5500_exchange_process(&context))
      ;
  }

  return 0;
}

int8_t w5500_get_txbuf_sizes(w5500_t *c, uint8_t *txbuf_sizes)
{
  const uint8_t bsb_sockets[] = {
      W5500_BSB_SOCKET0, W5500_BSB_SOCKET1, W5500_BSB_SOCKET2, W5500_BSB_SOCKET3,
      W5500_BSB_SOCKET4, W5500_BSB_SOCKET5, W5500_BSB_SOCKET6, W5500_BSB_SOCKET7};

  uint8_t buffer[W5500_SPI_HEADER_SIZE + 1];
  w5500_exchange_context_t context;

  // Инициализация контекста для обмена данными
  w5500_exchange_context_init(c, &context, buffer, sizeof(buffer));

  // Получение размеров буферов для каждого сокета
  for (int i = 0; i < 8; i++)
  {
    if (w5500_exchange_begin(&context, bsb_sockets[i], W5500_Sn_TXBUF_SIZE, 0, 1) != 0)
      return -1;

    while (!w5500_exchange_process(&context))
      ;

    // Сохранение прочитанного значения в массив txbuf_sizes
    txbuf_sizes[i] = buffer[W5500_BUFF_DATA + 0];
  }

  return 0;
}
