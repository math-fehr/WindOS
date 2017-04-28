#include "interrupts.h"
#include "timer.h"
#include "serial.h"
#include "debug.h"

/**
 * function used to make sure data is sent in order to GPIO
 */
extern void dmb();

// In UNDEF mode
void __attribute__ ((interrupt("UNDEF"))) undefined_instruction_vector(void) {
  kdebug(D_IRQ, 5, "UNDEFINED INSTRUCTION.\n");
  while(1);
}

// In IRQ mode
void __attribute__ ((interrupt("FIQ"))) fast_interrupt_vector(void) {
  kdebug(D_IRQ, 0, "FIQ.\n");
  while(1);
}

static bool status;

// In IRQ mode
void __attribute__ ((interrupt("IRQ"))) interrupt_vector(void) {
  kdebug(D_IRQ, 0, "IRQ.\n");
  dmb();
  Timer_ClearInterrupt();
  GPIO_setPinValue(GPIO_LED_PIN, status);
  status = !status;
  dmb();
}

uint32_t interrupt_reg[17];


extern switchTo_end;

// In SVC mode
// returns were we should branch to next
uint32_t __attribute__ ((interrupt("SWI"))) software_interrupt_vector(void) {
    kdebug(D_IRQ, 1, "SWI.\n");
    uint32_t* r = interrupt_reg;
    switch(r[7]) {
        case 0x01:
            kdebug(D_IRQ, 10, "Program exited. (if this shows up something is fucked up)\n");
            return switchTo_end;    
        default:
        kdebug(D_IRQ, 10, "Undefined SWI.\n");
    }
    while(1);
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void) {
  uint32_t* r = interrupt_reg;

  kdebug(D_IRQ, 5, "PREFECTH_ABORT at instruction %#010x. Registers:\n", r[14]+8);
  kdebug(D_IRQ, 5, "r0 :%#010x r1:%#010x r2 :%#010x r3 :%#010x\n", r[0], r[1], r[2], r[3]);
  kdebug(D_IRQ, 5, "r4 :%#010x r5:%#010x r6 :%#010x r7 :%#010x\n", r[4], r[5], r[6], r[7]);
  kdebug(D_IRQ, 5, "r8 :%#010x r9:%#010x r10:%#010x r11:%#010x\n", r[8], r[9], r[10], r[11]);
  kdebug(D_IRQ, 5, "r12:%#010x sp:%#010x lr :%#010x pc :%#010x\n", r[12], r[13], r[14], r[15]);
  kdebug(D_IRQ, 5, "CPSR: %#010x\n", r[16]);
  while(1);
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) data_abort_vector(void) {
    uint32_t* r = interrupt_reg;

    kdebug(D_IRQ, 5, "DATA_ABORT at instruction %#010x. Registers:\n", r[14]+8);
    kdebug(D_IRQ, 5, "r0 :%#010x r1:%#010x r2 :%#010x r3 :%#010x\n", r[0], r[1], r[2], r[3]);
    kdebug(D_IRQ, 5, "r4 :%#010x r5:%#010x r6 :%#010x r7 :%#010x\n", r[4], r[5], r[6], r[7]);
    kdebug(D_IRQ, 5, "r8 :%#010x r9:%#010x r10:%#010x r11:%#010x\n", r[8], r[9], r[10], r[11]);
    kdebug(D_IRQ, 5, "r12:%#010x sp:%#010x lr :%#010x pc :%#010x\n", r[12], r[13], r[14], r[15]);
    kdebug(D_IRQ, 5, "CPSR: %#010x\n", r[16]);
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
