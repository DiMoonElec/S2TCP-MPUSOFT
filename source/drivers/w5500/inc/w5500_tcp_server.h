#ifndef __W5500_TCP_SERVER_H__
#define __W5500_TCP_SERVER_H__

#include <stdint.h>
#include "w5500.h"
#include "w5500_socket.h"

typedef struct w5500_socket_tcp_serv_s w5500_socket_tcp_serv_t;

typedef void (*w5500_tcp_serv_data_received_callback_t)(w5500_socket_tcp_serv_t *instance, uint8_t *data, uint16_t data_len);
typedef void (*w5500_tcp_serv_data_transmitted_callback_t)(w5500_socket_tcp_serv_t *instance);
typedef void (*w5500_tcp_serv_established_callback_t)(w5500_socket_tcp_serv_t *instance);

struct w5500_socket_tcp_serv_s
{
  uint8_t state;
  uint8_t _state;

  uint8_t isConnected;

  /*
    Этот флаг устанавливается в 1 при необходимости
    отправить данные
  */
  uint8_t flag_txdata;

  /*
    Этот флаг устанавливается в 1 при необходимости
    принудительно разорвать соединение со стороны сервера
    (например, по таймауту неактивности клиента).
    Устанавливается функцией w5500_tcp_serv_disconnect().
  */
  uint8_t flag_disconnect;
  uint16_t port;

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
  w5500_tcp_serv_data_received_callback_t rx_callback;
  w5500_tcp_serv_data_transmitted_callback_t tx_callback;
  w5500_tcp_serv_established_callback_t established_callback;
  w5500_tcp_serv_established_callback_t disconnected_callback;
  uint8_t *tx_buff;
  uint8_t *rx_buff;
  w5500_socket_t socket;
  uint8_t work_buff[W5500_SPI_HEADER_SIZE + 2];
};

int8_t w5500_tcp_serv_init(w5500_t *c, w5500_socket_tcp_serv_t *instance, uint8_t sockID, uint16_t port,
                           uint8_t *txbuffer, uint16_t txbufferSize,
                           uint8_t *rxbuffer, uint16_t rxbufferSize);

void w5500_tcp_serv_set_data_received_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_data_received_callback_t callback);
void w5500_tcp_serv_set_data_transmitted_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_data_transmitted_callback_t callback);
void w5500_tcp_serv_set_established_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_established_callback_t callback);
void w5500_tcp_serv_set_disconnected_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_established_callback_t callback);

int8_t w5500_tcp_serv_tx_isready(w5500_socket_tcp_serv_t *instance);
int8_t w5500_tcp_serv_tx(w5500_socket_tcp_serv_t *instance, uint16_t size);
uint8_t *w5500_tcp_serv_get_tx_buffer(w5500_socket_tcp_serv_t *instance, uint16_t *buff_size);

uint8_t w5500_tcp_serv_isConnected(w5500_socket_tcp_serv_t *instance);

/*
  Запрашивает принудительный разрыв текущего TCP-соединения со стороны сервера.
  Реальный разрыв произойдет асинхронно внутри w5500_tcp_serv_process().
  После разрыва сокет автоматически переходит обратно в режим LISTEN
  и готов принять новое соединение.

  Возвращает 0, если запрос принят, -1 если сокет и так не был подключен.
*/
int8_t w5500_tcp_serv_disconnect(w5500_socket_tcp_serv_t *instance);

void w5500_tcp_serv_process(w5500_socket_tcp_serv_t *instance);

#endif