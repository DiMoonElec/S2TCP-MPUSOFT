#ifndef __W5500_EXCHANGE_H__
#define __W5500_EXCHANGE_H__

#include <stdint.h>
#include "w5500.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  uint8_t state;
  uint8_t flag_ready;
  uint16_t size;
  uint16_t bufferSize;
  uint16_t offset;
  uint8_t *buff;
  w5500_t *c;

} w5500_exchange_context_t;

typedef void (*w5500_exchange_completed_callback_t)(w5500_exchange_context_t *c);

////////////////////////////////////////////////////////////////////////////////

int8_t w5500_exchange_init(w5500_t *c);

int8_t w5500_exchange_begin(w5500_exchange_context_t *context,
                            uint8_t segment, uint16_t offset, uint8_t isWrite,
                            uint16_t size);

int8_t w5500_exchange_process(w5500_exchange_context_t *instance);

void w5500_exchange_context_init(w5500_t *c, w5500_exchange_context_t *context,
                                 uint8_t *buffer, uint16_t bufferSize);

void w5500_exchange_context_replace_buffer(w5500_exchange_context_t *context,
                                           uint8_t *new_buffer, uint16_t new_bufferSize);

#endif