#include "w5500.h"
#include "w5500_socket.h"
#include "w5500_exchange.h"
#include "w5500_def.h"

int8_t w5500_sock_fill_bsb(w5500_socket_t *sock, uint8_t sockID)
{
  switch (sockID)
  {
  case 0:
    sock->bsb_socket = W5500_BSB_SOCKET0;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET0;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET0;
    return 0;

  case 1:
    sock->bsb_socket = W5500_BSB_SOCKET1;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET1;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET1;
    return 0;

  case 2:
    sock->bsb_socket = W5500_BSB_SOCKET2;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET2;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET2;
    return 0;

  case 3:
    sock->bsb_socket = W5500_BSB_SOCKET3;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET3;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET3;
    return 0;

  case 4:
    sock->bsb_socket = W5500_BSB_SOCKET4;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET4;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET4;
    return 0;

  case 5:
    sock->bsb_socket = W5500_BSB_SOCKET5;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET5;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET5;
    return 0;

  case 6:
    sock->bsb_socket = W5500_BSB_SOCKET6;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET6;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET6;
    return 0;

  case 7:
    sock->bsb_socket = W5500_BSB_SOCKET7;
    sock->bsb_tx = W5500_BSB_TX_BUFF_SOCKET7;
    sock->bsb_rx = W5500_BSB_RX_BUFF_SOCKET7;
    return 0;
  }

  return -1;
}

void w5500_sock_read_sr(w5500_socket_t *instance)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_SR, 0, 1);
}

void w5500_sock_read_cr(w5500_socket_t *instance)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_CR, 0, 1);
}

void w5500_sock_read_rx_rsr(w5500_socket_t *instance)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_RX_RSR, 0, 2);
}

void w5500_sock_read_tx_fsr(w5500_socket_t *instance)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_TX_FSR, 0, 2);
}

void w5500_sock_read_rx_rd(w5500_socket_t *instance)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_RX_RD, 0, 2);
}

void w5500_sock_read_tx_wr(w5500_socket_t *instance)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_TX_WR, 0, 2);
}

void w5500_sock_read_rxbuff(w5500_socket_t *instance, uint16_t offset, uint16_t size)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_rx, offset, 0, size);
}

void w5500_sock_write_cr(w5500_socket_t *instance, uint8_t value)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = value;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_CR, 1, 1);
}

void w5500_sock_write_ir(w5500_socket_t *instance, uint8_t value)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = value;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_IR, 1, 1);
}

void w5500_sock_write_port(w5500_socket_t *instance, uint16_t port)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = port >> 8;
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 1] = port;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_PORT, 1, 2);
}

void w5500_sock_write_mode(w5500_socket_t *instance, uint8_t value)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = value;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_MR, 1, 1);
}

void w5500_sock_write_kpalvtr(w5500_socket_t *instance, uint8_t value)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = value;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_KPALVTR, 1, 1);
}

void w5500_sock_write_rx_rd(w5500_socket_t *instance, uint16_t value)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = value >> 8;
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 1] = value;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_RX_RD, 1, 2);
}

void w5500_sock_write_tx_wr(w5500_socket_t *instance, uint16_t value)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = value >> 8;
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 1] = value;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_TX_WR, 1, 2);
}

void w5500_sock_write_txbuff(w5500_socket_t *instance, uint16_t offset, uint16_t size)
{
  w5500_exchange_begin(&instance->exchange, instance->bsb_tx, offset, 1, size);
}

void w5500_sock_write_txbuf_size(w5500_socket_t *instance, uint8_t value)
{
  instance->exchange.buff[W5500_SPI_HEADER_SIZE + 0] = value;
  w5500_exchange_begin(&instance->exchange, instance->bsb_socket, W5500_Sn_TXBUF_SIZE, 1, 1);
}
