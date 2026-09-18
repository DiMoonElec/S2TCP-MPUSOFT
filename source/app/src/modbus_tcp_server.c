#include "modbus_tcp_server.h"
#include "w5500_tcp_server.h"
#include "ModbusSlave/public-api.h"

#define MODBUS_TCP_SERV_BUFFER_SIZE 1024
#define TCP_PORT 502

static w5500_socket_tcp_serv_t server;
static uint8_t rxbuffer[MODBUS_TCP_SERV_BUFFER_SIZE];
static uint8_t txbuffer[MODBUS_TCP_SERV_BUFFER_SIZE];


static void rx_data_callback(w5500_socket_tcp_serv_t* sock, uint8_t* data, uint16_t data_len)
{
  uint16_t buff_size;
  //Получаем указатель на буфер передачи
  uint8_t* buff = w5500_tcp_serv_get_tx_buffer(sock, &buff_size);
  
  //Вызываем парсер ModBus Ethernet
  uint16_t tx_size = modbus_slave_ethernet_parse(data, data_len, buff, buff_size);
  
  if(tx_size != 0)
  {
    w5500_tcp_serv_tx(sock, tx_size);
  }
}

void modbus_tcp_serv_init(w5500_t* c)
{
  w5500_tcp_serv_init(c, &server, 0, TCP_PORT,
    txbuffer, MODBUS_TCP_SERV_BUFFER_SIZE,
    rxbuffer, MODBUS_TCP_SERV_BUFFER_SIZE);
  
  w5500_tcp_serv_set_data_received_callback(&server, rx_data_callback);
}

void modbus_tcp_serv_process(void)
{
  w5500_tcp_serv_process(&server);
}

