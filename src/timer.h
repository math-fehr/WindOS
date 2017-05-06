#ifndef TIMER_H
#define TIMER_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "kernel.h"

// TIP: RPI clock speed is 250MHz
// http://raspberrypi.stackexchange.com/questions/699/what-spi-frequencies-does-raspberry-pi-support

#define RPI_SYSTIMER_BASE (PERIPHERALS_BASE + 0x3000)


/**
 * Structure of the system timer
 */
typedef struct {
  volatile uint32_t control_status;
  volatile uint32_t counter_lo;
  volatile uint32_t counter_hi;
  volatile uint32_t compare0;
  volatile uint32_t compare1;
  volatile uint32_t compare2;
  volatile uint32_t compare3;
} rpi_sys_timer_t;


/**
 * Wait in the function for time microseconds
 */
void Timer_WaitMicroSeconds(uint32_t time);

/**
 * Wait in the function for approximately count cycles (not precise)
 */
void Timer_WaitCycles(uint32_t count);

// Base adress of the arm timer
/*
#ifdef RPI2
#define RPI_ARMTIMER_BASE 0x3F00B400
#else
#define RPI_ARMTIMER_BASE 0x2000B400
#endif*/
#define RPI_ARMTIMER_BASE (PERIPHERALS_BASE + 0xB400)

/**
 * Scructure of the ARM timer
 */
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

/**
 * Defines used by the functions
 */
#define TIMER_CTRL_COUNTER_23BITS (1 << 1)
#define TIMER_CTRL_PRESCALE_1 0
#define TIMER_CTRL_PRESCALE_16 (1 << 2)
#define TIMER_CTRL_PRESCALE_256 (1 << 3)
#define TIMER_CTRL_INTERRUPT_BIT (1 << 5)
#define TIMER_CTRL_ENABLE_BIT (1 << 7)
#define TIMER_CTRL_HALT_STOP (1 << 8) // Timer halted if ARM is in debug halt mode
#define TIMER_CTRL_FREERUNNING_COUNTER (1 << 9)

/**
 * Setup the timer
 */
void Timer_Setup();

/**
 * Enable the timer
 * The function Timer_SetLoad or SetReload must have been called
 * to change the frequency of the clock
 */
void Timer_Enable();

/**
 * Enable the interrupts
 */
void Timer_Enable_Interrupts();

/**
 * Disable the interrupts
 */
void Timer_Disable_Interrupts();

/**
 * Disable the timer
 */
void Timer_Disable();

/**
 * Change the frequency of the timer (in microseconds)
 * Calling this function reset the timer
 */
void Timer_SetLoad(uint32_t value);

/**
 * Change the frequency of the timer (in microseonds)
 * The frequency is reset on the next tick
 */
void Timer_SetReload(uint32_t value);

/**
 * Inform the timer that the interruption has been treated
 */
void Timer_ClearInterrupt();

/**
 * Return the current time
 */
uint32_t Timer_GetTime();

/**
 * Return posix time (not implemented)
 * TODO implement that function
 */
uint32_t timer_get_posix_time();

#endif //TIMER_H
