#include "tcp_uart_bridge.h"
#include <string.h>
#include <stddef.h>

/* Так как w5500_socket_tcp_serv_t встроен в TcpUartBridge_t по значению,
из указателя на сокет, который приходит в колбэки w5500, можно получить
указатель на весь контекст моста. Стандартный прием "container_of". */
#define BRIDGE_FROM_SOCK(p) ((TcpUartBridge_t *)((uint8_t *)(p) - offsetof(TcpUartBridge_t, sock)))

static void established_callback(w5500_socket_tcp_serv_t *sock);
static void disconnected_callback(w5500_socket_tcp_serv_t *sock);
static void rx_data_callback(w5500_socket_tcp_serv_t *sock, uint8_t *data, uint16_t data_len);
static void uart2tcp_process(TcpUartBridge_t *br);
static void tcp2uart_process(TcpUartBridge_t *br);

/////////////////////////////////////////////////////////////////////////
// Инициализация
/////////////////////////////////////////////////////////////////////////
int8_t tcp_uart_bridge_init(TcpUartBridge_t *instance, w5500_t *c, SerialRingBuffer_t *srb,
                            uint8_t sockID, uint16_t port)
{
    if (instance == 0 || srb == 0)
        return -1;

    memset(instance, 0, sizeof(TcpUartBridge_t));
    instance->srb = srb;

    /* Передаем указатели на внутренние буферы структуры и их размеры из #define */
    int8_t res = w5500_tcp_serv_init(c, &instance->sock, sockID, port,
                                     instance->tcp_txbuffer, TCP_UART_BRIDGE_TCP_TX_BUFFER_SIZE,
                                     instance->tcp_rxbuffer, TCP_UART_BRIDGE_TCP_RX_BUFFER_SIZE);
    if (res != 0)
        return res;

    w5500_tcp_serv_set_established_callback(&instance->sock, established_callback);
    w5500_tcp_serv_set_disconnected_callback(&instance->sock, disconnected_callback);
    w5500_tcp_serv_set_data_received_callback(&instance->sock, rx_data_callback);

    return 0;
}

uint8_t tcp_uart_bridge_isConnected(TcpUartBridge_t *instance)
{
    return instance->connected;
}

static void reset_queues(TcpUartBridge_t *br)
{
    br->uart2tcp_head = 0;
    br->uart2tcp_tail = 0;
    br->uart2tcp_count = 0;

    br->tcp2uart_head = 0;
    br->tcp2uart_tail = 0;
    br->tcp2uart_count = 0;
}

/////////////////////////////////////////////////////////////////////////
// Колбэки установления / разрыва соединения и приема данных из сокета
/////////////////////////////////////////////////////////////////////////
static void established_callback(w5500_socket_tcp_serv_t *sock)
{
    TcpUartBridge_t *br = BRIDGE_FROM_SOCK(sock);

    // Сбрасываем состояние моста под новое соединение - на случай,
    // если от предыдущей сессии остались хвосты в очередях.
    reset_queues(br);
    br->connected = 1;
}

static void disconnected_callback(w5500_socket_tcp_serv_t *sock)
{
    TcpUartBridge_t *br = BRIDGE_FROM_SOCK(sock);
    br->connected = 0;

    // Недоотправленные/недопринятые данные завершившейся сессии теряют
    // смысл - сбрасываем ОБЕ очереди. Раньше здесь чистилась только
    // очередь UART->TCP, а очередь TCP->UART продолжала "доливаться"
    // в UART уже после разрыва соединения (и могла утечь в следующую
    // сессию, если бы established_callback не подчищал за нее).
    reset_queues(br);
}

static void rx_data_callback(w5500_socket_tcp_serv_t *sock, uint8_t *data, uint16_t data_len)
{
    TcpUartBridge_t *br = BRIDGE_FROM_SOCK(sock);

    // Копируем принятые по TCP данные в программную очередь на отправку в UART.
    // Если очередь переполнена (UART не успевает), лишние байты отбрасываются,
    // счетчик tcp2uart_dropped можно проверять для диагностики.
    for (uint16_t i = 0; i < data_len; i++)
    {
        if (br->tcp2uart_count >= TCP_UART_BRIDGE_QUEUE_SIZE)
        {
            br->tcp2uart_dropped += (uint32_t)(data_len - i);
            break;
        }
        br->tcp2uart_buff[br->tcp2uart_head] = data[i];
        br->tcp2uart_head = (uint16_t)((br->tcp2uart_head + 1) % TCP_UART_BRIDGE_QUEUE_SIZE);
        br->tcp2uart_count++;
    }
}

/////////////////////////////////////////////////////////////////////////
// UART -> TCP
//
// Никакого ожидания паузы в приеме и никакого накопления "пакета" до
// определенного размера - это и было причиной зависаний. Забираем из
// UART все, что накопилось к текущему вызову process(), и как только
// сокет готов - сразу отправляем то, что есть. Именно так работает
// эталонный tcp_serial_redirect.py (sendall() на каждый data_received).
/////////////////////////////////////////////////////////////////////////
static void uart2tcp_pull(TcpUartBridge_t *br)
{
    int16_t c;
    while ((c = SerialRingBuffer_Getc(br->srb)) != -1)
    {
        if (br->uart2tcp_count >= TCP_UART_BRIDGE_BUFFER_SIZE)
        {
            // Софтверная очередь переполнена - байт теряется, но мы все
            // равно продолжаем вычитывать UART, чтобы не переполнить
            // еще и аппаратный/кольцевой буфер SerialRingBuffer.
            br->uart2tcp_dropped++;
            continue;
        }
        br->uart2tcp_buff[br->uart2tcp_head] = (uint8_t)c;
        br->uart2tcp_head = (uint16_t)((br->uart2tcp_head + 1) % TCP_UART_BRIDGE_BUFFER_SIZE);
        br->uart2tcp_count++;
    }
}

static void uart2tcp_push(TcpUartBridge_t *br)
{
    if (br->uart2tcp_count == 0)
        return;

    if (!br->connected || !w5500_tcp_serv_tx_isready(&br->sock))
        return; // подождем следующего вызова process(), байты остаются в очереди

    uint16_t buff_size;
    uint8_t *buff = w5500_tcp_serv_get_tx_buffer(&br->sock, &buff_size);

    uint16_t size = br->uart2tcp_count;
    if (size > buff_size)
        size = buff_size; // отправим сколько влезет, остаток уйдет следующим вызовом

    // Копируем с учетом возможного переноса через конец кольцевого буфера
    // (в отличие от старой версии, "лишний" хвост здесь НЕ теряется).
    uint16_t first_chunk = (uint16_t)(TCP_UART_BRIDGE_BUFFER_SIZE - br->uart2tcp_tail);
    if (first_chunk > size)
        first_chunk = size;

    memcpy(buff, &br->uart2tcp_buff[br->uart2tcp_tail], first_chunk);
    if (size > first_chunk)
        memcpy(buff + first_chunk, &br->uart2tcp_buff[0], (size_t)(size - first_chunk));

    w5500_tcp_serv_tx(&br->sock, size);

    br->uart2tcp_tail = (uint16_t)((br->uart2tcp_tail + size) % TCP_UART_BRIDGE_BUFFER_SIZE);
    br->uart2tcp_count = (uint16_t)(br->uart2tcp_count - size);
}

static void uart2tcp_process(TcpUartBridge_t *br)
{
    uart2tcp_pull(br);
    uart2tcp_push(br);
}

/////////////////////////////////////////////////////////////////////////
// TCP -> UART
/////////////////////////////////////////////////////////////////////////
static void tcp2uart_process(TcpUartBridge_t *br)
{
    while (br->tcp2uart_count > 0)
    {
        uint8_t c = br->tcp2uart_buff[br->tcp2uart_tail];
        if (SerialRingBuffer_Putc(br->srb, c) != 0)
            break; // кольцевой буфер UART полон - продолжим в следующем вызове process()

        br->tcp2uart_tail = (uint16_t)((br->tcp2uart_tail + 1) % TCP_UART_BRIDGE_QUEUE_SIZE);
        br->tcp2uart_count--;
    }
}

/////////////////////////////////////////////////////////////////////////
// Основной цикл
/////////////////////////////////////////////////////////////////////////
void tcp_uart_bridge_process(TcpUartBridge_t *instance)
{
    if (instance == 0)
        return;

    w5500_tcp_serv_process(&instance->sock);
    uart2tcp_process(instance);
    tcp2uart_process(instance);
}