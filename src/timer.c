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


/** \fn void Timer_Enable()
 *  \brief Enable the Timer and its interruptions
 *  \warning Timer_Setup and Timer_SetLoad Must have been called before
 */
void Timer_Enable() {
  dmb();
  rpiArmTimer->control |= TIMER_CTRL_FREERUNNING_COUNTER;
  rpiArmTimer->control |= TIMER_CTRL_ENABLE_BIT;
  rpiArmTimer->control |= TIMER_CTRL_INTERRUPT_BIT;
  rpiArmTimer->irq_clear = 0;
  dmb();

}


/** \fn void Timer_Disable()
 *  \brief Disable the Timer and its interruptions
 */
void Timer_Disable() {
  dmb();
  rpiArmTimer->control &= ~TIMER_CTRL_ENABLE_BIT;
  rpiArmTimer->control &= ~TIMER_CTRL_FREERUNNING_COUNTER;
  rpiArmTimer->control &= ~TIMER_CTRL_INTERRUPT_BIT;
  dmb();
}


/** \fn void Timer_SetLoad(uint32_t value)
 *  \brief Set the timer period
 *  \param value The approximate period in microseconds (with our setup)
 */
void Timer_SetLoad(uint32_t value) {
  dmb();
  rpiArmTimer->load = value;
}


/** \fn void Timer_SetReload(uint32_t value)
 *  \brief Reload the timer on the next tick
 *  \param value The approximate period in microseconds (with our setup)
 */
void Timer_SetReload(uint32_t value) {
  dmb();
  rpiArmTimer->reload = value;
}


/** \fn void Timer_ClearInterrupt()
 *  \brief Acknoledge the interrupts (clear the interrupt signal)
 */
void Timer_ClearInterrupt() {
  dmb();
  rpiArmTimer->irq_clear = 1;
}


#define TIMER_MAX_HANDLERS 128
/** \var timerHandler timerHandlers[TIMER_AUX_HANDLERS]
 *  \brief Contains the delayed function call
 */
static timerHandler timerHandlers[TIMER_MAX_HANDLERS];


/** \var uint32_t nextHandlerTrigger
 *  \brief The time when the next handler will be called
 */
static uint32_t nextHandlerTrigger;


/** \var uint32_t nextHandlerOverflow
 *  \brief The number of overflow needed before the next handler will be called (an overflow of the timer freerunning counter = one hour approximately)
 */
static uint32_t nextHandlerOverflow;


/** \var uint32_t lastTimeCheckedHandlers
 *  \brief Last time we checked for handlers (used to know when there is an overflow of the freerunning counter)
 */
static uint32_t lastTimeCheckedHandlers;


/** \fn int Timer_addHandler(uint32_t millis, timerFunction* function, void* param, void* context)
 *  \brief Delay the call of a function
 *  \param millis The delay in milliseconds
 *  \param function The function pointer
 *  \param param The second parameter of the function
 *  \param context The third parameter of the function
 *  \return The index of the function handler (-1 if there is no left)
 *  \warning This function expect the timer to run at 1MHz
 */
int Timer_addHandler(uint32_t millis, timerFunction* function, void* param, void* context) {
    uint64_t micro = (uint64_t)millis * (uint64_t)1000 + (uint64_t)Timer_GetTime();

    int i = Timer_getFreeHandler();
    if(i == -1) {
        kdebug(4,9,"No free handlers left");
        return -1; //No free handlers (this will probably result in a bug)
    }
    timerHandlers[i].function = function;
    timerHandlers[i].param = param;
    timerHandlers[i].context = context;
    timerHandlers[i].overflowTime = (uint32_t)(micro >> 32);
    timerHandlers[i].triggerTime = (uint32_t)(micro & 0xFFFFFFFF);

    if(timerHandlers[i].overflowTime < nextHandlerOverflow ||
       (timerHandlers[i].overflowTime == nextHandlerOverflow &&
        timerHandlers[i].triggerTime < nextHandlerTrigger)) {
        nextHandlerOverflow = timerHandlers[i].overflowTime;
        nextHandlerTrigger = timerHandlers[i].triggerTime;
    }
    return i;
}


/** \fn void Timer_callHandlingFunction(unsigned int id)
 *  \brief Call a function that has been delayed
 *  \param id The id of the function handler
 */
void Timer_callHandlingFunction(unsigned int id) {
    if(timerHandlers[id].function != NULL) {
        timerHandlers[id].function(id,timerHandlers[id].param,
                                   timerHandlers[id].context);
        timerHandlers[id].function = NULL;
    }
    else {
        kdebug(4,8,"Trying to call a undefined handler");
    }
}


/** \fn void Timer_updateNextHandler()
 *  \brief Compute the values of nextHandlerTrigger and nextHandlerOverflow
 */
void Timer_updateNextHandler() {
    nextHandlerOverflow = 0xFFFFFFFF;
    nextHandlerTrigger = 0xFFFFFFFF;
    for(int i = 0; i<TIMER_MAX_HANDLERS; i++) {
        if(timerHandlers[i].function != NULL) {
            if (timerHandlers[i].overflowTime < nextHandlerOverflow
                || (timerHandlers[i].overflowTime == nextHandlerOverflow
                    && timerHandlers[i].triggerTime < nextHandlerTrigger)) {
                nextHandlerOverflow = timerHandlers[i].overflowTime;
                nextHandlerTrigger = timerHandlers[i].triggerTime;
            }
        }
    }
}


/** \fn void Timer_callHandlers()
 *  \brief call needed function that has been delayed
 */
void Timer_callHandlers() {
    bool needCheck = false;
    //If the freerunning counter has overflowed
    if(lastTimeCheckedHandlers > Timer_GetTime()) {
        for(int i = 0; i<TIMER_MAX_HANDLERS; i++) {
            if(timerHandlers[i].overflowTime > 0) {
                timerHandlers[i].overflowTime--;
            }
            else if(timerHandlers[i].overflowTime == 0 &&
                    timerHandlers[i].function != NULL) {
                Timer_callHandlingFunction(i);
                needCheck = true;
            }
        }
        if(nextHandlerTrigger != 0) {
            nextHandlerTrigger--;
        }
    }

    lastTimeCheckedHandlers = Timer_GetTime();
    //We check the function only if necessary
    if((nextHandlerOverflow == 0 && nextHandlerTrigger <= lastTimeCheckedHandlers) || needCheck) {
        for(int i = 0; i<TIMER_MAX_HANDLERS; i++) {
            if(timerHandlers[i].overflowTime == 0 &&
               timerHandlers[i].triggerTime < lastTimeCheckedHandlers &&
               timerHandlers[i].function != NULL) {
                Timer_callHandlingFunction(i);
            }
        }
        Timer_updateNextHandler();
    }
}


/** \fn int Timer_getFreeHandler()
 *  \brief get the index of a free handler (to delay functions)
 *  \return The indef of a free handler (or -1 if there is no free handlers)
 */
int Timer_getFreeHandler() {
    for(int i = 0; i<TIMER_MAX_HANDLERS; i++) {
        if(timerHandlers[i].function == NULL) {
            return i;
        }
    }
    return -1;
}


/** \fn void Timer_deleteHandler(unsigned id)
 *  \brief Delete the call a delayed function
 *  \param id The id of the delayed function
 */
void Timer_deleteHandler(unsigned id) {
    timerHandlers[id].function = NULL;
}


/** \fn uint32_t Timer_GetTime()
 *  \brief Get the freerunning counter value
 *  \return The freerunning counter value in microseconds
 */
uint32_t Timer_GetTime() {
  uint32_t res = rpiArmTimer->freerunning_counter;
  dmb();
  return res;
}


/** \fn void Timer_WaitMicroSeconds(uint32_t time)
 *  \brief Wait for a certain amount of time (in microseconds)
 *  \param time The time to wait in microseconds
 */
void Timer_WaitMicroSeconds(uint32_t time) {
  volatile uint32_t tstart = rpiSystemTimer->counter_lo;
  while((rpiSystemTimer->counter_lo - tstart) < time) {
    // Wait
  }
  dmb();
}


/** \fn void Timer_WaitCycles(uint32_t count)
 *  \brief Wait for a certain amount of cycles
 *  \param count The number of cycles to wait
 *  \warning This is an order of magnitude
 */
void Timer_WaitCycles(uint32_t count) {
  volatile uint32_t val = count;
  for (;val>0;--val) {
      asm("");
  }
}


/** \fn void Timer_Setup() {
 *  \brief Setup the timer (will not start it, nor start the interruptions)
 *  \warning This function needs to be called before all others timer fu
 */
void Timer_Setup() {
    dmb();
    rpiArmTimer->control = (62 << 16)
                           | TIMER_CTRL_COUNTER_23BITS
                           | TIMER_CTRL_PRESCALE_1;
    rpiArmTimer->pre_divider = 249; // Gives a 1MHz timer.
    rpiArmTimer->irq_clear = 0;
    dmb();
    RPI_GetIRQController()->Enable_Basic_IRQs |= RPI_BASIC_ARM_TIMER_IRQ;
    dmb();

    nextHandlerTrigger = 0xFFFFFFFF;
    nextHandlerOverflow = 0xFFFFFFFF;
    lastTimeCheckedHandlers = Timer_GetTime();
}


/** \fn uint32_t timer_get_posix_time()
 *  \brief Return the posix time
 *  \warning Not implemented yet, returns 42
 */
uint32_t timer_get_posix_time() {
    return 42;
}
