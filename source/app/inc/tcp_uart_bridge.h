#ifndef __TCP_UART_BRIDGE_H__
#define __TCP_UART_BRIDGE_H__

#include <stdint.h>
#include "w5500_tcp_server.h"

/*
ВНИМАНИЕ: путь до заголовка SerialRingBuffer поправьте под структуру
вашего проекта (по аналогии с "ModbusSlave/public-api.h" в исходниках).
*/
#include "SerialRingBuffer/public-api.h"

/* Размер программной кольцевой очереди UART -> TCP.
В отличие от предыдущей версии, этот размер НЕ обязан быть <= размера
tx-буфера сокета: отправка идет частями по мере готовности сокета,
а то, что не влезло в один w5500_tcp_serv_tx(), просто ждет своей очереди
в кольце (данные больше не теряются). */
#ifndef TCP_UART_BRIDGE_BUFFER_SIZE
#define TCP_UART_BRIDGE_BUFFER_SIZE 2048
#endif

/* Размер программной очереди байт, ожидающих записи в кольцевой UART-буфер
(TCP -> UART). Нужна на случай, если UART не успевает передавать данные
так же быстро, как они приходят по TCP. */
#ifndef TCP_UART_BRIDGE_QUEUE_SIZE
#define TCP_UART_BRIDGE_QUEUE_SIZE 2048
#endif

/* Размеры буферов TCP сокета (W5500).
Максимальный размер буфера на сокет в W5500 обычно 2 КБ (2048 байт). */
#ifndef TCP_UART_BRIDGE_TCP_TX_BUFFER_SIZE
#define TCP_UART_BRIDGE_TCP_TX_BUFFER_SIZE 1024
#endif

#ifndef TCP_UART_BRIDGE_TCP_RX_BUFFER_SIZE
#define TCP_UART_BRIDGE_TCP_RX_BUFFER_SIZE 1024
#endif

/* Пауза в приеме UART (мс), после которой накопленные байты считаются
завершенной "пачкой" и отправляются в сокет одним TCP-сегментом.
В отличие от прежних 100 мс, значение должно быть лишь немного больше
межбайтового интервала на вашей скорости UART (при 115200 бод это
~87 мкс/байт) - и на порядки меньше пауз между разными сообщениями
протокола, иначе вернется прежнее зависание. Единицы измерения должны
совпадать с единицами SYSTICK_GET_VALUE(). Подбирайте под свой захват
трафика: должно с запасом перекрывать паузы ВНУТРИ пачки байт и быть
намного меньше пауз МЕЖДУ пачками. */
#ifndef TCP_UART_BRIDGE_FLUSH_DELAY_MS
#define TCP_UART_BRIDGE_FLUSH_DELAY_MS 2
#endif

/* Если данных накопилось уже столько - отправляем не дожидаясь паузы.
Защита от непрерывного потока без пауз (иначе задержка росла бы
неограниченно, пока UART не замолчит). */
#ifndef TCP_UART_BRIDGE_FLUSH_SIZE
#define TCP_UART_BRIDGE_FLUSH_SIZE 512
#endif

typedef struct
{
    SerialRingBuffer_t *srb;

    /* Сокет встроен по значению (не по указателю) - это дает возможность
    в колбэках w5500 вычислить указатель на весь TcpUartBridge_t через
    offsetof (см. tcp_uart_bridge.c, макрос BRIDGE_FROM_SOCK). */
    w5500_socket_tcp_serv_t sock;
    volatile uint8_t connected;

    /* ==== Буферы TCP сокета ==== */
    uint8_t tcp_txbuffer[TCP_UART_BRIDGE_TCP_TX_BUFFER_SIZE];
    uint8_t tcp_rxbuffer[TCP_UART_BRIDGE_TCP_RX_BUFFER_SIZE];

    /* ==== UART -> TCP (кольцевая программная очередь) ====
    Байты копятся здесь и отправляются одним TCP-сегментом либо после
    короткой паузы в приеме UART (TCP_UART_BRIDGE_FLUSH_DELAY_MS - признак
    конца "пачки"/кадра), либо раньше, если накопилось уже много
    (TCP_UART_BRIDGE_FLUSH_SIZE). Это дает настоящий батчинг на стороне
    TCP - W5500 сам ничего не накапливает и шлет сегмент на каждый вызов
    w5500_tcp_serv_tx(), это подтвердил захват трафика. */
    uint8_t uart2tcp_buff[TCP_UART_BRIDGE_BUFFER_SIZE];
    volatile uint16_t uart2tcp_head;
    volatile uint16_t uart2tcp_tail;
    volatile uint16_t uart2tcp_count;
    uint32_t uart2tcp_flush_timer; /* момент (SYSTICK) последнего принятого байта UART */
    uint32_t uart2tcp_dropped; /* счетчик потерянных байт при переполнении очереди (диагностика) */

    /* ==== TCP -> UART (кольцевая программная очередь) ==== */
    uint8_t tcp2uart_buff[TCP_UART_BRIDGE_QUEUE_SIZE];
    volatile uint16_t tcp2uart_head;
    volatile uint16_t tcp2uart_tail;
    volatile uint16_t tcp2uart_count;
    uint32_t tcp2uart_dropped; /* счетчик потерянных байт при переполнении очереди (диагностика) */
} TcpUartBridge_t;

/* Инициализация моста. Буферы TCP берутся из самой структуры instance */
int8_t tcp_uart_bridge_init(TcpUartBridge_t *instance, w5500_t *c, SerialRingBuffer_t *srb,
                            uint8_t sockID, uint16_t port);

void tcp_uart_bridge_process(TcpUartBridge_t *instance);
uint8_t tcp_uart_bridge_isConnected(TcpUartBridge_t *instance);

#endif /* __TCP_UART_BRIDGE_H__ */