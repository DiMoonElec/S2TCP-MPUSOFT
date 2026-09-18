#include "w5500.h"
#include "w5500_exchange.h"
#include "w5500_socket.h"
#include "w5500_tcp_server.h"
#include "w5500_def.h"

/******************************************************************************/

enum
{
  STATE_TCPSERV_MAIN = 0,
  STATE_TCPSERV_CLOSE,
  STATE_TCPSERV_CLOSE_WAIT,
  STATE_TCPSERV_CLOSE_WAIT_1,

  STATE_TCPSERV_INIT,
  STATE_TCPSERV_MODE_INIT,
  STATE_TCPSERV_KPALVTR_INIT,

  STATE_TCPSERV_OPEN,

  STATE_TCPSERV_WAIT_OPEN,
  STATE_TCPSERV_WAIT_OPEN_1,

  STATE_TCPSERV_LISTEN_CMD,

  STATE_TCPSERV_LISTEN,
  STATE_TCPSERV_LISTEN_1,

  STATE_TCPSERV_ESTABLISHED,
  STATE_TCPSERV_ESTABLISHED_1,

  STATE_TCPSERV_READ_CHECK,

  STATE_TCPSERV_READ_RX_SIZE,
  STATE_TCPSERV_READ_RX_SIZE_1,
  STATE_TCPSERV_READ_RX_SIZE_2,
  STATE_TCPSERV_READ_RX_SIZE_3,

  STATE_TCPSERV_READ_RX_READ_POINTER,

  STATE_TCPSERV_READ_DATA,

  STATE_TCPSERV_WRITE_RX_READ_POINTER,

  STATE_TCPSERV_CMD_RECV,

  STATE_TCPSERV_CMD_RECV_COMPETED,
  STATE_TCPSERV_CMD_RECV_COMPETED_1,

  STATE_TCPSERV_SEND_DATA,

  STATE_TCPSERV_READ_TX_FREESIZE,
  STATE_TCPSERV_READ_TX_FREESIZE_1,
  STATE_TCPSERV_READ_TX_FREESIZE_2,
  STATE_TCPSERV_READ_TX_FREESIZE_3,

  STATE_TCPSERV_READ_TX_WRITE_POINTER,

  STATE_TCPSERV_WRITE_DATA,

  STATE_TCPSERV_WRITE_TX_WRITE_POINTER,

  STATE_TCPSERV_CMD_SEND,

  STATE_TCPSERV_CMD_SEND_COMPETED,
  STATE_TCPSERV_CMD_SEND_COMPETED_1,

  STATE_TCPSERV_IR_RESET,

  NUMSTATES_STATE_TCPSERV
};

#define BUFFER (instance->socket.exchange.buff + W5500_SPI_HEADER_SIZE)

int8_t w5500_tcp_serv_init(w5500_t *c, w5500_socket_tcp_serv_t *instance, uint8_t sockID, uint16_t port,
                           uint8_t *txbuffer, uint16_t txbufferSize,
                           uint8_t *rxbuffer, uint16_t rxbufferSize)
{
  w5500_exchange_context_init(c, &instance->socket.exchange, instance->work_buff, sizeof(instance->work_buff));

  instance->port = port;
  instance->tx_buff = txbuffer;
  instance->rx_buff = rxbuffer;

  instance->tx_buff_size = txbufferSize;
  instance->rx_buff_size = rxbufferSize;

  instance->rx_callback = 0;
  instance->tx_callback = 0;
  instance->established_callback = 0;
  instance->disconnected_callback = 0;

  instance->isConnected = 0;
  instance->flag_disconnect = 0;

  if (w5500_sock_fill_bsb(&instance->socket, sockID) != 0)
    return -1;

  instance->state = STATE_TCPSERV_MAIN;
  instance->_state = instance->state + 1;
  return 0;
}

void w5500_tcp_serv_set_data_received_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_data_received_callback_t callback)
{
  instance->rx_callback = callback;
}

void w5500_tcp_serv_set_data_transmitted_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_data_transmitted_callback_t callback)
{
  instance->tx_callback = callback;
}

void w5500_tcp_serv_set_established_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_established_callback_t callback)
{
  instance->established_callback = callback;
}

void w5500_tcp_serv_set_disconnected_callback(w5500_socket_tcp_serv_t *instance, w5500_tcp_serv_established_callback_t callback)
{
  instance->disconnected_callback = callback;
}

int8_t w5500_tcp_serv_tx_isready(w5500_socket_tcp_serv_t *instance)
{
  return instance->flag_txdata == 0;
}

int8_t w5500_tcp_serv_tx(w5500_socket_tcp_serv_t *instance, uint16_t size)
{
  if (instance->flag_txdata)
    return -1;
  instance->tx_size = size;
  instance->flag_txdata = 1;
  return 0;
}

uint8_t *w5500_tcp_serv_get_tx_buffer(w5500_socket_tcp_serv_t *instance, uint16_t *buff_size)
{
  if (instance->flag_txdata)
    return 0;

  (*buff_size) = (uint16_t)(instance->tx_buff_size - W5500_SPI_HEADER_SIZE);
  return instance->tx_buff + W5500_SPI_HEADER_SIZE;
}

uint8_t w5500_tcp_serv_isConnected(w5500_socket_tcp_serv_t *instance)
{
  return instance->isConnected;
}

int8_t w5500_tcp_serv_disconnect(w5500_socket_tcp_serv_t *instance)
{
  if (!instance->isConnected)
    return -1;

  instance->flag_disconnect = 1;
  return 0;
}

// Конечный автомат, который реализует работу TCP-сервера
void w5500_tcp_serv_process(w5500_socket_tcp_serv_t *instance)
{
  uint8_t entry = 0;

  if (instance->state != instance->_state)
  {
    instance->_state = instance->state;
    entry = 1;
  }

  switch (instance->state)
  {
  case STATE_TCPSERV_MAIN:
  {
    if (entry)
      w5500_sock_read_sr(&instance->socket);

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      if (BUFFER[0] != W5500_Sn_SR_SOCK_CLOSED)
        instance->state = STATE_TCPSERV_CLOSE;
      else
        instance->state = STATE_TCPSERV_INIT;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CLOSE:
  {
    if (entry)
      w5500_sock_write_cr(&instance->socket, W5500_Sn_CR_CLOSE);

    if (w5500_exchange_process(&instance->socket.exchange))
      instance->state = STATE_TCPSERV_CLOSE_WAIT;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CLOSE_WAIT:
  {
    w5500_sock_read_sr(&instance->socket);
    instance->state = STATE_TCPSERV_CLOSE_WAIT_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CLOSE_WAIT_1:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      if (BUFFER[0] != W5500_Sn_SR_SOCK_CLOSED)
        instance->state = STATE_TCPSERV_CLOSE_WAIT;
      else
        instance->state = STATE_TCPSERV_INIT;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_INIT:
  {
    // Устанавливаем порт
    if (entry)
      w5500_sock_write_port(&instance->socket, instance->port);

    if (w5500_exchange_process(&instance->socket.exchange))
      instance->state = STATE_TCPSERV_MODE_INIT;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_MODE_INIT:
  {
    if (entry)
      w5500_sock_write_mode(&instance->socket, W5500_Sn_MR_Protocol_TCP);

    if (w5500_exchange_process(&instance->socket.exchange))
      instance->state = STATE_TCPSERV_KPALVTR_INIT;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_KPALVTR_INIT:
  {
    if (entry)
      w5500_sock_write_kpalvtr(&instance->socket, 0x02);

    if (w5500_exchange_process(&instance->socket.exchange))
      instance->state = STATE_TCPSERV_OPEN;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_OPEN:
  {
    // Отправляем команду открытия сокета
    if (entry)
      w5500_sock_write_cr(&instance->socket, W5500_Sn_CR_OPEN);

    // Ждем завершения отправки
    if (w5500_exchange_process(&instance->socket.exchange))
      instance->state = STATE_TCPSERV_WAIT_OPEN;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_WAIT_OPEN:
  {
    w5500_sock_read_sr(&instance->socket);
    instance->state = STATE_TCPSERV_WAIT_OPEN_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_WAIT_OPEN_1:
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      if (BUFFER[0] != W5500_Sn_SR_SOCK_INIT)
        instance->state = STATE_TCPSERV_WAIT_OPEN;
      else
        instance->state = STATE_TCPSERV_LISTEN_CMD;
    }
    break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_LISTEN_CMD:
  {
    if (entry)
      w5500_sock_write_cr(&instance->socket, W5500_Sn_CR_LISTEN);

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->state = STATE_TCPSERV_LISTEN;
    }
  }
  break;
  ////////////////////////////////////////////
  ////////////////////////////////////////////
  case STATE_TCPSERV_LISTEN:
  {
    // Проверка состояния
    w5500_sock_read_sr(&instance->socket);
    instance->state = STATE_TCPSERV_LISTEN_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_LISTEN_1:
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->state = STATE_TCPSERV_LISTEN;

      uint8_t sr = BUFFER[0];

      switch (sr)
      {
      case W5500_Sn_SR_SOCK_ESTABLISHED:
      {
        instance->flag_txdata = 0;
        instance->isConnected = 1;
        if (instance->established_callback != 0)
          instance->established_callback(instance);

        instance->state = STATE_TCPSERV_ESTABLISHED;
      }
      break;

      case W5500_Sn_SR_SOCK_CLOSED:
      case W5500_Sn_SR_SOCK_INIT:
      case W5500_Sn_SR_SOCK_CLOSE_WAIT:
        instance->state = STATE_TCPSERV_MAIN;
        break;
      }
    }
    break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_ESTABLISHED:
  {
    // Проверка состояния
    w5500_sock_read_sr(&instance->socket);
    instance->state = STATE_TCPSERV_ESTABLISHED_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_ESTABLISHED_1:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      uint8_t sr = BUFFER[0];

      if (sr == W5500_Sn_SR_SOCK_CLOSED ||
          sr == W5500_Sn_SR_SOCK_INIT ||
          sr == W5500_Sn_SR_SOCK_CLOSE_WAIT)
      {
        // Партнер (клиент) уже закрыл или разорвал соединение
        instance->flag_disconnect = 0;
        instance->isConnected = 0;

        if (instance->disconnected_callback != 0)
          instance->disconnected_callback(instance);

        instance->state = STATE_TCPSERV_MAIN;
      }
      else if (instance->flag_disconnect)
      {
        /*
          Принудительный разрыв соединения по инициативе сервера
          (например, по таймауту неактивности клиента,
          отслеживаемому в пользовательском коде).
          Отменяем незавершенную отправку, если она была запрошена,
          и переходим в уже существующую ветку STATE_TCPSERV_CLOSE,
          которая закроет сокет и автоматически вернет его в LISTEN.
        */
        instance->flag_disconnect = 0;
        instance->flag_txdata = 0;
        instance->isConnected = 0;

        if (instance->disconnected_callback != 0)
          instance->disconnected_callback(instance);

        instance->state = STATE_TCPSERV_CLOSE;
      }
      else if (instance->flag_txdata)
      {
        // Если поступила команда отправить данные, то отправляем
        instance->state = STATE_TCPSERV_SEND_DATA;
      }
      else
      {
        instance->state = STATE_TCPSERV_READ_CHECK;
      }
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_CHECK:
  {
    // Проверяем, получено ли что-то
    instance->state = STATE_TCPSERV_READ_RX_SIZE;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_RX_SIZE:
  {
    w5500_sock_read_rx_rsr(&instance->socket);
    instance->state = STATE_TCPSERV_READ_RX_SIZE_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_RX_SIZE_1:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->rx_rsr = BUFFER[0] << 8;
      instance->rx_rsr |= BUFFER[1];

      if (instance->rx_rsr == 0)
        instance->state = STATE_TCPSERV_ESTABLISHED;
      else
        instance->state = STATE_TCPSERV_READ_RX_SIZE_2;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_RX_SIZE_2:
  {
    w5500_sock_read_rx_rsr(&instance->socket);
    instance->state = STATE_TCPSERV_READ_RX_SIZE_3;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_RX_SIZE_3:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->rx_rsr1 = BUFFER[0] << 8;
      instance->rx_rsr1 |= BUFFER[1];

      if (instance->rx_rsr != instance->rx_rsr1)
      {
        // Если 2 раза прочитали разные значения
        // то пробуем еще раз
        instance->state = STATE_TCPSERV_READ_RX_SIZE;
      }
      else
      {
        // Если прочитали 2 раза одинаковые значения
        // Если не вмещаемся, то читаем столько, сколько можем
        if (instance->rx_rsr > (instance->rx_buff_size - W5500_SPI_HEADER_SIZE))
          instance->rx_rsr = instance->rx_buff_size - W5500_SPI_HEADER_SIZE;

        // Идем далее
        instance->state = STATE_TCPSERV_READ_RX_READ_POINTER;
      }
    }
  }
  break;
  ////////////////////////////////////////////
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_RX_READ_POINTER:
  {
    if (entry)
      w5500_sock_read_rx_rd(&instance->socket);

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->rx_rd = BUFFER[0] << 8;
      instance->rx_rd |= BUFFER[1];
      /*
        Прочитали указатель на данные
        Выполняем подмену рабочего буфера на буфер приема
        и читаем в него данные
      */
      w5500_exchange_context_replace_buffer(&instance->socket.exchange, instance->rx_buff, instance->rx_buff_size);
      instance->state = STATE_TCPSERV_READ_DATA;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_DATA:
  {
    if (entry)
      w5500_sock_read_rxbuff(&instance->socket, instance->rx_rd, instance->rx_rsr);

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      // Данные приняты
      // Возвращаем обратно рабочий буфер
      w5500_exchange_context_replace_buffer(&instance->socket.exchange, instance->work_buff, sizeof(instance->work_buff));
      instance->state = STATE_TCPSERV_WRITE_RX_READ_POINTER;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_WRITE_RX_READ_POINTER:
  {
    if (entry)
      w5500_sock_write_rx_rd(&instance->socket, (uint16_t)(instance->rx_rd + instance->rx_rsr));

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      // Отправка указателя завершена
      instance->state = STATE_TCPSERV_CMD_RECV;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CMD_RECV:
  {
    /*
      Отправляем команду завершения приема
      После отправки команды читаем регистр команд до тех пор,
      пока он не станет равен 0x00 (команда выполнена)
    */
    if (entry)
      w5500_sock_write_cr(&instance->socket, W5500_Sn_CR_RECV);

    if (w5500_exchange_process(&instance->socket.exchange))
      instance->state = STATE_TCPSERV_CMD_RECV_COMPETED;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CMD_RECV_COMPETED:
  {
    w5500_sock_read_cr(&instance->socket);
    instance->state = STATE_TCPSERV_CMD_RECV_COMPETED_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CMD_RECV_COMPETED_1:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      // Если команда выполнена, то возвращаемся назад
      // в состояние ESTABLISHED
      // иначе, читаем еще раз
      if (BUFFER[0] == 0x00)
      {
        // Прием полностью завершен,
        // вызываем callback-функцию
        // которая информирует вышестоящее приложение
        // о получении данных
        if (instance->rx_callback != 0)
          instance->rx_callback(instance, instance->rx_buff + W5500_SPI_HEADER_SIZE, instance->rx_rsr);

        instance->state = STATE_TCPSERV_ESTABLISHED;
      }
      else
        instance->state = STATE_TCPSERV_CMD_RECV_COMPETED;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_SEND_DATA:
  {
    instance->state = STATE_TCPSERV_READ_TX_FREESIZE;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_TX_FREESIZE:
  {
    w5500_sock_read_tx_fsr(&instance->socket);
    instance->state = STATE_TCPSERV_READ_TX_FREESIZE_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_TX_FREESIZE_1:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->tx_fsr = BUFFER[0] << 8;
      instance->tx_fsr |= BUFFER[1];

      instance->state = STATE_TCPSERV_READ_TX_FREESIZE_2;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_TX_FREESIZE_2:
  {
    w5500_sock_read_tx_fsr(&instance->socket);
    instance->state = STATE_TCPSERV_READ_TX_FREESIZE_3;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_TX_FREESIZE_3:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->tx_fsr1 = BUFFER[0] << 8;
      instance->tx_fsr1 |= BUFFER[1];

      if (instance->tx_fsr != instance->tx_fsr1)
      {
        // Если оба раза прочитали разные значения
        // то пробудем еще раз
        instance->state = STATE_TCPSERV_READ_TX_FREESIZE;
      }
      else
      {
        // Оба раза прочитали одинаковые значения
        // Если свободного места в микросхеме меньше, чем мы хотим отправить
        // то попробуем повторить передачу на следующем цикле
        if (instance->tx_fsr < instance->tx_size)
          instance->state = STATE_TCPSERV_ESTABLISHED;
        else
          instance->state = STATE_TCPSERV_READ_TX_WRITE_POINTER;
      }
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_READ_TX_WRITE_POINTER:
  {
    if (entry)
      w5500_sock_read_tx_wr(&instance->socket);

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->tx_wr = BUFFER[0] << 8;
      instance->tx_wr |= BUFFER[1];
      /*
        Прочитали указатель на данные
        Выполняем подмену рабочего буфера на буфер передачи
        и отправляем данные из него
      */
      w5500_exchange_context_replace_buffer(&instance->socket.exchange, instance->tx_buff, instance->tx_buff_size);
      instance->state = STATE_TCPSERV_WRITE_DATA;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_WRITE_DATA:
  {
    if (entry)
      w5500_sock_write_txbuff(&instance->socket, instance->tx_wr, instance->tx_size);

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      // Данные записаны в буфер передатчика
      // Возвращаем обратно рабочий буфер
      w5500_exchange_context_replace_buffer(&instance->socket.exchange, instance->work_buff, sizeof(instance->work_buff));
      instance->state = STATE_TCPSERV_WRITE_TX_WRITE_POINTER;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_WRITE_TX_WRITE_POINTER:
  {
    if (entry)
      w5500_sock_write_tx_wr(&instance->socket, (uint16_t)(instance->tx_wr + instance->tx_size));

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      // Отправка указателя завершена
      instance->state = STATE_TCPSERV_CMD_SEND;
    }
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CMD_SEND:
  {
    /*
      Отправляем команду начала передачи
      отправленного буфера.
      После отправки команды читаем регистр команд до тех пор,
      пока он не станет равен 0x00 (команда выполнена)
    */
    if (entry)
      w5500_sock_write_cr(&instance->socket, W5500_Sn_CR_SEND);

    if (w5500_exchange_process(&instance->socket.exchange))
      instance->state = STATE_TCPSERV_CMD_SEND_COMPETED;
  }
  break;
  ////////////////////////////////////////////
  ////////////////////////////////////////////
  case STATE_TCPSERV_CMD_SEND_COMPETED:
  {
    w5500_sock_read_cr(&instance->socket);
    instance->state = STATE_TCPSERV_CMD_SEND_COMPETED_1;
  }
  break;
  ////////////////////////////////////////////
  case STATE_TCPSERV_CMD_SEND_COMPETED_1:
  {
    if (w5500_exchange_process(&instance->socket.exchange))
    {
      // Если команда выполнена, то возвращаемся назад
      // в состояние ESTABLISHED
      // иначе, читаем еще раз
      if (BUFFER[0] == 0x00)
      {
        instance->state = STATE_TCPSERV_IR_RESET;
      }
      else
        instance->state = STATE_TCPSERV_CMD_SEND_COMPETED;
    }
  }
  break;
  ////////////////////////////////////////////
  ////////////////////////////////////////////
  case STATE_TCPSERV_IR_RESET:
  {
    if (entry)
      w5500_sock_write_ir(&instance->socket, W5500_Sn_IR_SEND_OK);

    if (w5500_exchange_process(&instance->socket.exchange))
    {
      instance->flag_txdata = 0;

      // Прием полностью завершен,
      // вызываем callback-функцию
      // которая информирует вышестоящее приложение
      // о получении данных
      if (instance->tx_callback != 0)
        instance->tx_callback(instance);

      instance->state = STATE_TCPSERV_ESTABLISHED;
    }
  }
  break;
  ////////////////////////////////////////////
  default:
    instance->state = STATE_TCPSERV_MAIN;
    break;
  }
}