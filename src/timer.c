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
  rpiArmTimer->control = 0;
  rpiArmTimer->control |= (249 << 16);
  rpiArmTimer->control |= TIMER_CTRL_COUNTER_23BITS;
  rpiArmTimer->control |= TIMER_CTRL_PRESCALE_1;
  rpiArmTimer->control |= TIMER_CTRL_HALT_STOP;
  rpiArmTimer->pre_divider = 249; // Gives a 1MHz timer.
  dmb();
  RPI_GetIRQController()->Enable_Basic_IRQs |= RPI_BASIC_ARM_TIMER_IRQ;
  dmb();
}

void Timer_Enable() {
  dmb();
  rpiArmTimer->control |= TIMER_CTRL_FREERUNNING_COUNTER;
  rpiArmTimer->control |= TIMER_CTRL_ENABLE_BIT;
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
  for (;val>0;--val);
}

//TODO implement the function
uint32_t timer_get_posix_time() {
	42;
}
