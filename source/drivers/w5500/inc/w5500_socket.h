#ifndef __W5500_SOCKET_H__
#define __W5500_SOCKET_H__

#include <stdint.h>
#include "w5500_exchange.h"

typedef struct
{
  uint8_t bsb_socket; // сегмент сокета
  uint8_t bsb_tx;     // сегмент буфера передачи
  uint8_t bsb_rx;     // сегмент буфера приема

  w5500_exchange_context_t exchange;
} w5500_socket_t;

int8_t w5500_sock_fill_bsb(w5500_socket_t *sock, uint8_t sockID);

// Чтение управляющих регистров соекта
void w5500_sock_read_sr(w5500_socket_t *sock);
void w5500_sock_read_cr(w5500_socket_t *instance);
void w5500_sock_read_rx_rsr(w5500_socket_t *instance);
void w5500_sock_read_tx_fsr(w5500_socket_t *instance);
void w5500_sock_read_rx_rd(w5500_socket_t *instance);
void w5500_sock_read_tx_wr(w5500_socket_t *instance);
void w5500_sock_read_rxbuff(w5500_socket_t *instance, uint16_t offset, uint16_t size);

// Запись управляющих регистров сокета
void w5500_sock_write_cr(w5500_socket_t *instance, uint8_t value);
void w5500_sock_write_ir(w5500_socket_t *instance, uint8_t value);
void w5500_sock_write_port(w5500_socket_t *instance, uint16_t port);
void w5500_sock_write_mode(w5500_socket_t *instance, uint8_t value);
void w5500_sock_write_kpalvtr(w5500_socket_t *instance, uint8_t value);
void w5500_sock_write_rx_rd(w5500_socket_t *instance, uint16_t value);
void w5500_sock_write_tx_wr(w5500_socket_t *instance, uint16_t value);
void w5500_sock_write_txbuff(w5500_socket_t *instance, uint16_t offset, uint16_t size);
void w5500_sock_write_txbuf_size(w5500_socket_t *instance, uint8_t value);

#endif
