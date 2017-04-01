#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "global.h"

#ifdef RASPI2
#define RPI_SYSTIMER_BASE 0x3F003000
#else
#define RPI_SYSTIMER_BASE 0x20003000
#endif
typedef struct {
  volatile uint32_t control_status;
  volatile uint32_t counter_lo;
  volatile uint32_t counter_hi;
  volatile uint32_t compare0;
  volatile uint32_t compare1;
  volatile uint32_t compare2;
  volatile uint32_t compare3;
} rpi_sys_timer_t;


void Timer_WaitMicroSeconds(uint32_t time);
void Timer_WaitCycles(uint32_t count);

#endif
