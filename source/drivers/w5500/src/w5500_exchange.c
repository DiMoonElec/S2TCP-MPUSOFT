#include "w5500_exchange.h"
#include "w5500.h"
#include "w5500_def.h"
#include "w5500_port.h"

////////////////////////////////////////////////////////////////////////////////

enum
{
  READSTATE_BEGIN = 0,
  READSTATE_WAIT,
  READSTATE_END,

  NUM_READSTATES
};

////////////////////////////////////////////////////////////////////////////////

static void callback(w5500_exchange_context_t *instance)
{
  instance->flag_ready = 1;
}

static void __set_header(uint8_t *buffer, uint16_t adr, uint8_t control)
{
  buffer[0] = (uint8_t)(adr >> 8);
  buffer[1] = (uint8_t)(adr);
  buffer[2] = control;
}

////////////////////////////////////////////////////////////////////////////////

int8_t w5500_exchange_init(w5500_t *c)
{
  w5500_port_init(c);
  return 0;
}

int8_t w5500_exchange_begin(w5500_exchange_context_t *instance,
                           uint8_t segment, uint16_t offset, uint8_t isWrite,
                           uint16_t size)
{
  instance->size = size + W5500_SPI_HEADER_SIZE;
  instance->state = READSTATE_BEGIN;
  instance->flag_ready = 0;

  if (instance->bufferSize < instance->size)
    return -1;

  __set_header(instance->buff, offset, (isWrite ? W5500_RWB_ACCESS_WRITE : W5500_RWB_ACCESS_READ) | segment);
  return 0;
}

int8_t w5500_exchange_process(w5500_exchange_context_t *instance)
{
  switch (instance->state)
  {
  case READSTATE_BEGIN:
    if (w5500_port_isrdy(instance->c))
    {
      w5500_port_exchange(instance, callback);
      instance->state = READSTATE_WAIT;
    }
    break;
  /////////////////////////////////////////////////////////
  case READSTATE_WAIT:
    if (instance->flag_ready)
    {
      instance->state = READSTATE_END;
      return 1;
    }
    break;
  /////////////////////////////////////////////////////////
  case READSTATE_END:
    return 1;
  }

  return 0;
}

void w5500_exchange_context_init(w5500_t *c, w5500_exchange_context_t *context,
                                 uint8_t *buffer, uint16_t bufferSize)
{
  context->c = c;
  context->buff = buffer;
  context->bufferSize = bufferSize;
}

void w5500_exchange_context_replace_buffer(w5500_exchange_context_t *context,
                                           uint8_t *new_buffer, uint16_t new_bufferSize)
{
  context->buff = new_buffer;
  context->bufferSize = new_bufferSize;
}
