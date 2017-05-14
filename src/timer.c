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

void Timer_Enable() {
  dmb();
  rpiArmTimer->control |= TIMER_CTRL_FREERUNNING_COUNTER;
  rpiArmTimer->control |= TIMER_CTRL_ENABLE_BIT;
  rpiArmTimer->control |= TIMER_CTRL_INTERRUPT_BIT;
  rpiArmTimer->irq_clear = 0;
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

#define TIMER_MAX_HANDLERS 128
static timerHandler timerHandlers[TIMER_MAX_HANDLERS];
static uint32_t nextHandlerTrigger;
static uint32_t nextHandlerOverflow;
static uint32_t lastTimeCheckedHandlers;

// This code expect the timer to work at 1kHz
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


void Timer_callHandlers() {
    bool needCheck = false;
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


int Timer_getFreeHandler() {
    for(int i = 0; i<TIMER_MAX_HANDLERS; i++) {
        if(timerHandlers[i].function == NULL) {
            return i;
        }
    }
    return -1;
}

void Timer_deleteHandler(unsigned id) {
    timerHandlers[id].function = NULL;
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
	return 42;
}

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
