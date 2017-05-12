#include "timer.h"
#include "interrupts.h"

static rpi_sys_timer_t* rpiSystemTimer =
  (rpi_sys_timer_t*)RPI_SYSTIMER_BASE;

static rpi_arm_timer_t* rpiArmTimer =
  (rpi_arm_timer_t*)RPI_ARMTIMER_BASE;

/**
 * Ensure the data comes in order to the peripheral
 */
extern void dmb();

void Timer_Setup() {
  dmb();
  rpiArmTimer->control = 0x003E0000;
  rpiArmTimer->pre_divider = 0xF9; // Gives a 1MHz timer.
  dmb();
}

void Timer_Enable() {
  dmb();
  rpiArmTimer->control |= TIMER_CTRL_FREERUNNING_COUNTER;
  rpiArmTimer->control |= TIMER_CTRL_ENABLE_BIT;
  rpiArmTimer->control |= TIMER_CTRL_COUNTER_23BITS;
  dmb();
}

void Timer_Enable_Interrupts() {
  dmb();
  rpiArmTimer->control |= TIMER_CTRL_INTERRUPT_BIT;
  dmb();
}

void Timer_Disable_Interrupts() {
  dmb();
  rpiArmTimer->control &= ~TIMER_CTRL_INTERRUPT_BIT;
  dmb();
}

void Timer_Disable() {
  dmb();
  rpiArmTimer->control &= ~TIMER_CTRL_ENABLE_BIT;
  rpiArmTimer->control &= ~TIMER_CTRL_FREERUNNING_COUNTER;
  dmb();
}

void Timer_SetLoad(uint32_t value) {
  dmb();
  rpiArmTimer->load = value;
}

void Timer_SetReload(uint32_t value) {
  dmb();
  rpiArmTimer->reload = value;
}

void Timer_ClearInterrupt() {
  dmb();
  rpiArmTimer->irq_clear = 1;
}

uint32_t Timer_GetTime() {
  uint32_t res = rpiArmTimer->freerunning_counter;
  dmb();
  return res;
}

void Timer_WaitMicroSeconds(uint32_t time) {
  volatile uint32_t tstart = rpiSystemTimer->counter_lo;
  while((rpiSystemTimer->counter_lo - tstart) < time) {
    // Wait
  }
  dmb();
}

void Timer_WaitCycles(uint32_t count) {
  volatile uint32_t val = count;
  for (;val>0;--val) {
      asm("");
  }
}

//TODO implement the function
uint32_t timer_get_posix_time() {
	42;
}
