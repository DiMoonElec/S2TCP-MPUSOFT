#ifndef __W5500_RAW_SOCKET_H__
#define __W5500_RAW_SOCKET_H__

#include <stdint.h>
#include "w5500.h"
#include "w5500_socket.h"

typedef struct w5500_socket_raw_s w5500_socket_raw_t;

typedef void (*w5500_socket_raw_data_received_callback_t)(w5500_socket_raw_t *sock, uint8_t *data, uint16_t data_len);
typedef void (*w5500_socket_raw_data_transmitted_callback_t)(w5500_socket_raw_t *sock);

struct w5500_socket_raw_s
{
  uint8_t state;
  uint8_t _state;

  /*
    Этот флаг устанавливается в 1 при необходимости
    отправить данные
  */
  uint8_t flag_txdata;

  /*
   Временные переменная для регистра rx_rsr
   их 2, так как надо читать до тех пор,
   пока дважды не будет прочитано одинаковое значение
 */
  uint16_t rx_rsr;
  uint16_t rx_rsr1;

  /*
    Временная переменная для регистра tx_fsr
    аналогично rx_rsr
  */
  uint16_t tx_fsr;
  uint16_t tx_fsr1;

  /*
    Временный регистр rx_rd
  */
  uint16_t rx_rd;

  /*
    Временный регистр tx_wr
  */
  uint16_t tx_wr;

  uint16_t tx_buff_size;
  uint16_t rx_buff_size;

  // Кол-во байт для передачи
  uint16_t tx_size;

  w5500_socket_raw_data_received_callback_t rx_callback;
  w5500_socket_raw_data_transmitted_callback_t tx_callback;
  uint8_t *tx_buff;
  uint8_t *rx_buff;
  w5500_socket_t socket;
  uint8_t work_buff[W5500_SPI_HEADER_SIZE + 2];
};

void w5500_raw_sock_set_data_received_callback(w5500_socket_raw_t *sock, //
                                               w5500_socket_raw_data_received_callback_t callback);

void w5500_raw_sock_set_data_transmitted_callback(w5500_socket_raw_t *sock, //
                                                  w5500_socket_raw_data_transmitted_callback_t callback);

uint8_t *w5500_raw_sock_get_tx_buffer(w5500_socket_raw_t *sock, uint16_t *buff_size);

int8_t w5500_raw_sock_tx(w5500_socket_raw_t *sock, uint16_t size);

int8_t w5500_raw_sock_init(w5500_t *c, w5500_socket_raw_t *sock, uint8_t sockID,
                           uint8_t *txbuffer, uint16_t txbufferSize,
                           uint8_t *rxbuffer, uint16_t rxbufferSize);

void w5500_raw_sock_process(w5500_socket_raw_t *sock);

#endif