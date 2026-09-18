#include <stdint.h>
#include "w5500_port.h"
#include "serial_ring_buffer_hal.h"

volatile uint32_t timebase_ticks;
extern SerialRingBuffer_t SerialBuffer_UART1;

void DMA1_Channel1_IRQHandler(void)
{
  w5500_port_DMA_IRQHandler();
}

void SysTick_Handler(void)
{
  timebase_ticks++;
}

void USART1_IRQHandler(void)
{
  SerialRingBufferHAL_IRQHandler(&SerialBuffer_UART1);
}