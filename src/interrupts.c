#include "interrupts.h"
#include "timer.h"
#include "serial.h"
#include "debug.h"

/**
 * function used to make sure data is sent in order to GPIO
 */
extern void dmb();

void __attribute__ ((interrupt("UNDEF"))) undefined_instruction_vector(void) {
  kdebug(D_IRQ, 5, "UNDEFINED INSTRUCTION.\n");
  while(1);
}

void __attribute__ ((interrupt("FIQ"))) fast_interrupt_vector(void) {
  kdebug(D_IRQ, 0, "FIQ.\n");
  while(1);
}

static bool status;


void __attribute__ ((interrupt("IRQ"))) interrupt_vector(void) {
  kdebug(D_IRQ, 0, "IRQ.\n");
  dmb();
  Timer_ClearInterrupt();
  GPIO_setPinValue(GPIO_LED_PIN, status);
  status = !status;
  dmb();
}


void __attribute__ ((interrupt("SWI"))) software_interrupt_vector(void) {
  kdebug(D_IRQ, 5, "SWI.\n");
  while(1);
}

void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void) {
  kdebug(D_IRQ, 5, "PREFETCH ABORT.\n");
  while(1);
}

void __attribute__ ((interrupt("ABORT"))) data_abort_vector(uint32_t addr) {
  kdebug(D_IRQ, 5, "DATA_ABORT at instruction %#010x.\n", addr-8);
  while(1);
}

static rpi_irq_controller_t* rpiIRQController =
  (rpi_irq_controller_t*) RPI_INTERRUPT_CONTROLLER_BASE;

rpi_irq_controller_t* RPI_GetIRQController(void) {
  return rpiIRQController;
}

void enable_interrupts(void) {
    kdebug(D_IRQ, 1, "Enabling interrupts.\n");
    asm volatile("cpsie i");
}

void disable_interrupts(void) {
    kdebug(D_IRQ, 1, "Disabling interrupts.\n");
    asm volatile("cpsid i");
}
