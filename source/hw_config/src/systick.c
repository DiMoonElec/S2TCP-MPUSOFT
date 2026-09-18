#include "systick.h"

void delay_ms(uint32_t delay)
{
  uint32_t start_tick = SYSTICK_GET_VALUE();

  if(delay < 0xFFFFFFFFU)
  {
    delay += 1;
  }

  while((SYSTICK_GET_VALUE() - start_tick) < delay)
  {
  }
}

