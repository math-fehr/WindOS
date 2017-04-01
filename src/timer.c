#include "timer.h"

static volatile rpi_sys_timer_t* rpiSystemTimer =
  (rpi_sys_timer_t*)RPI_SYSTIMER_BASE;

void Timer_WaitMicroSeconds(uint32_t time) {
  volatile uint32_t tstart = rpiSystemTimer->counter_lo;
  while((rpiSystemTimer->counter_lo - tstart) < time) {
    // Wait
  }
}

void Timer_WaitCycles(uint32_t count) {
  volatile uint32_t val = count;
  for (;val>0;--val);
}
