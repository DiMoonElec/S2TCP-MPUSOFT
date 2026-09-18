/*
  Аппаратно-зависимые функции драйвера W5500 для микроконтроллера AT32F407VG
  Autor: DiMoon Electronics
  Date: 2024/06/04
*/

#ifndef __W5500_PORT_H__
#define __W5500_PORT_H__

#include <stdint.h>
#include "w5500_exchange.h"

void w5500_port_init(w5500_t *c);
int8_t w5500_port_isrdy(w5500_t *c);
int8_t w5500_port_exchange(w5500_exchange_context_t *context,
                           w5500_exchange_completed_callback_t callback);

void w5500_port_DMA_IRQHandler(void);
#endif