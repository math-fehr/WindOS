#ifndef TIMER_H
#define TIMER_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "kernel.h"

// TIP: RPI clock speed is 250MHz
// http://raspberrypi.stackexchange.com/questions/699/what-spi-frequencies-does-raspberry-pi-support

#define RPI_SYSTIMER_BASE (PERIPHERALS_BASE + 0x3000)


/** \st rpi_sys_timer_t
 *  \breif Structure of the system timer (see bcm2835 manual)
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
#define RPI_ARMTIMER_BASE (PERIPHERALS_BASE + 0xB400)


/** \st rpi_arm_timer_t
 *  \brief Structure of the ARM timer
 */
typedef struct {
    volatile uint32_t load;                 ///< Set the timer frequency (and reset the timer)
    const volatile uint32_t value;
    volatile uint32_t control;              ///< Control register
    volatile uint32_t irq_clear;            ///< Clear the IRQs
    const volatile uint32_t raw_irq;
    const volatile uint32_t masked_irq;
    volatile uint32_t reload;               ///< Set the timer frequency (continue its current tick)
    volatile uint32_t pre_divider;          ///< Used to set timer frequency
    volatile uint32_t freerunning_counter;  ///< Counter of microseconds
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
 * The function Timer_Setup must have been called before
 */
void Timer_Enable();


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
 * the functions called by the timer
 * params are id,param and context
 */
typedef void timerFunction (unsigned,void*,void*);

/** \st timerHandler
 *  \brief Represent a function handler for delayed function call
 */
typedef struct timerHandler {
    timerFunction* function;    ///< The address of the function
    void* param;                ///< Its second parameter
    void* context;              ///< Its third parameter
    uint32_t triggerTime;       ///< When to execute it (Timer_GetTime
    uint32_t overflowTime;      ///< How many overflow of Timer_GetTime before executing it
} timerHandler;

/**
 * Delay the call of a function
 */
int Timer_addHandler(uint32_t millis, timerFunction* function, void* param, void* context);

/**
 * Calculate the next time a handler needs to be called
 */
void Timer_updateNextHandler();

/**
 * Call a handling function and delete it
 */
void Timer_callHandlingFunction(unsigned int i);

/**
 * Call necessary handlers
 */
void Timer_callHandlers();

/**
 * Get a free timer handler
 */
int Timer_getFreeHandler();

/**
 * Delete a timer handler
 */
void Timer_deleteHandler(unsigned id);

/**
 * Return the current time
 */
uint32_t Timer_GetTime();


/**
 * Return the posix time
 */
 uint32_t timer_get_posix_time();

#endif //TIMER_H
