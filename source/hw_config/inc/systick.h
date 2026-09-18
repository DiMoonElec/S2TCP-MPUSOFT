#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include <stdint.h>

extern volatile uint32_t timebase_ticks;

#define SYSTICK_GET_VALUE()     (timebase_ticks)

void delay_ms(uint32_t delay);

#endif
