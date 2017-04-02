#include "interrupts.h"

void __attribute__ ((interrupt("UNDEF"))) undefined_instruction_vector(void) {

}

void __attribute__ ((interrupt("FIQ"))) fast_interrupt_vector(void) {

}

void __attribute__ ((interrupt("IRQ"))) interrupt_vector(void) {

}

void __attribute__ ((interrupt("SWI"))) software_interrupt_vector(void) {

}

void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void) {

}

void __attribute__ ((interrupt("ABORT"))) data_abort_vector(void) {

}

static rpi_irq_controller_t* rpiIRQController =
  (rpi_irq_controller_t*) RPI_INTERRUPT_CONTROLLER_BASE;

rpi_irq_controller_t* RPI_GetIRQController(void) {
  return rpiIRQController;
}

void enable_interrupts(void) {
  asm volatile
  ("mrs r0, cpsr\n"
    "bic r0, r0, #0x80\n"
    "msr cpsr, r0\n"
  );
}

void disable_interrupts(void) {
  asm volatile
  ("mrs r0, cpsr\n"
    "orr r0, r0, #0x80\n"
    "msr cpsr, r0\n"
  );
}
