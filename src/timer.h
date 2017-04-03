#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "global.h"

// TIP: RPI clock speed is 250MHz
// http://raspberrypi.stackexchange.com/questions/699/what-spi-frequencies-does-raspberry-pi-support

// The system timer isn't emulated in QEMU
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

// Found in page 196 of the BCM2835 manual.
#ifdef RASPI2
#define RPI_ARMTIMER_BASE 0x3F00B400
#else
#define RPI_ARMTIMER_BASE 0x2000B400
#endif

typedef struct {
  volatile uint32_t load;
  const volatile uint32_t value;
  volatile uint32_t control;
  volatile uint32_t irq_clear;
  const volatile uint32_t raw_irq;
  const volatile uint32_t masked_irq;
  volatile uint32_t reload;
  volatile uint32_t pre_divider;
  volatile uint32_t freerunning_counter;
} rpi_arm_timer_t;

#define TIMER_CTRL_COUNTER_23BITS (1 << 1)
#define TIMER_CTRL_PRESCALE_1 0
#define TIMER_CTRL_PRESCALE_16 (1 << 2)
#define TIMER_CTRL_PRESCALE_256 (1 << 3)
#define TIMER_CTRL_INTERRUPT_BIT (1 << 5)
#define TIMER_CTRL_ENABLE_BIT (1 << 7)
#define TIMER_CTRL_HALT_STOP (1 << 8) // Timer halted if ARM is in debug halt mode
#define TIMER_CTRL_FREERUNNING_COUNTER (1 << 9)

void Timer_Setup();
void Timer_Enable();
void Timer_Enable_Interrupts();
void Timer_Disable_Interrupts();
void Timer_Disable();
void Timer_SetLoad(uint32_t value);
void Timer_SetReload(uint32_t value);
void Timer_ClearInterrupt();
uint32_t Timer_GetTime();

#endif
