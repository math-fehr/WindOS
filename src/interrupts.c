#include "interrupts.h"
#include "timer.h"
#include "serial.h"

void __attribute__ ((interrupt("UNDEF"))) undefined_instruction_vector(void) {
  while(1);
}

void __attribute__ ((interrupt("FIQ"))) fast_interrupt_vector(void) {
  while(1);
}

void __attribute__ ((interrupt("IRQ"))) interrupt_vector(void) {
    disable_interrupts();
    static bool on = false;
    if(on) {
        GPIO_setPinValue(GPIO_LED_PIN,true);
    }
    else {
        GPIO_setPinValue(GPIO_LED_PIN,false);
    }
    on = !on;
    GPIO_setPinValue(47,false);
    Timer_ClearInterrupt();
}

void __attribute__ ((interrupt("SWI"))) software_interrupt_vector(void) {
  while(1);
}

void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void) {
  while(1);
}

void __attribute__ ((interrupt("ABORT"))) data_abort_vector(void) {
  while(1);
}

static rpi_irq_controller_t* rpiIRQController =
  (rpi_irq_controller_t*) RPI_INTERRUPT_CONTROLLER_BASE;

rpi_irq_controller_t* RPI_GetIRQController(void) {
  return rpiIRQController;
}

void enable_interrupts(void) {
    asm volatile("cpsie i");
}

void disable_interrupts(void) {
    asm volatile("cpsid i");
}
