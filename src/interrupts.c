#include "interrupts.h"
/**
 * function used to make sure data is sent in order to GPIO
 */
extern void dmb();

static rpi_irq_controller_t* rpiIRQController =
  (rpi_irq_controller_t*) RPI_INTERRUPT_CONTROLLER_BASE;

rpi_irq_controller_t* RPI_GetIRQController(void) {
  return rpiIRQController;
}

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
uint32_t interrupt_reg[17];
extern uint32_t current_process_id;

int counter;
// In IRQ mode - r0 => SP
void interrupt_vector(uint32_t sp) {
	process* p = get_process_list()[current_process_id];

	uint32_t* phy_stack = (uint32_t*)(sp);
	for (int i=0;i<17;i++) {
		kdebug(D_IRQ,3,"<=%d|%d: %p\n",current_process_id,i,phy_stack[i]);
	}



	if (RPI_GetIRQController()->IRQ_basic_pending == RPI_BASIC_ARM_TIMER_IRQ) {
		dmb();
		Timer_ClearInterrupt();
		GPIO_setPinValue(GPIO_LED_PIN, status);
		status = !status;
		dmb();

		p->sp = sp;

		kdebug(D_IRQ,3,"%dSP at %#010x\n",current_process_id,sp);

		current_process_id = get_next_process();
		kdebug(D_IRQ, 2	, "Switching to process %d\n", current_process_id);

		p = get_process_list()[current_process_id];
		kdebug(D_IRQ,3,"%dSP at %#010x\n",current_process_id,p->sp);


		process_switchTo(p);

		while(1) {}
	} else {
		kdebug(D_IRQ, 10, "[ERROR] Unhandled IRQ!\n");
		while(1) {}
	}
}


void print_registers(int level) {
    uint32_t* r = interrupt_reg;

    kdebug(D_IRQ, level, "r0 :%#010x r1:%#010x r2 :%#010x r3 :%#010x\n", r[0], r[1], r[2], r[3]);
    kdebug(D_IRQ, level, "r4 :%#010x r5:%#010x r6 :%#010x r7 :%#010x\n", r[4], r[5], r[6], r[7]);
    kdebug(D_IRQ, level, "r8 :%#010x r9:%#010x r10:%#010x r11:%#010x\n", r[8], r[9], r[10], r[11]);
    kdebug(D_IRQ, level, "r12:%#010x sp:%#010x lr :%#010x pc :%#010x\n", r[12], r[13], r[14], r[15]);
    kdebug(D_IRQ, level, "CPSR: %#010x\n", r[16]);
}

// In SVC mode
// returns were we should branch to next
uint32_t software_interrupt_vector(void) {
    kdebug(D_IRQ, 1, "SWI.\n");
	print_registers(1);
    uint32_t* r = interrupt_reg;
    switch(r[7]) {
        case SVC_EXIT:
			svc_exit();
        case SVC_SBRK:
			return svc_sbrk(r[0]);
		case SVC_FORK:
			return svc_fork();
        case SVC_WRITE:
            return svc_write(r[0],(char*)r[1],r[2]);
        case SVC_CLOSE:
            return svc_close(r[0]);
        case SVC_FSTAT:
            return svc_fstat(r[0],(struct stat*)r[1]);
        case SVC_LSEEK:
            kdebug(D_IRQ, 10, "LSEEK %d\n", r[0]);
            while(1) {}
        case SVC_READ:
            return svc_read(r[0],(char*)r[1],r[2]);
		case SVC_TIME:
			return svc_time((time_t*)r[0]);
		case SVC_EXECVE:
			return svc_execve((char*)r[0],(char**)r[1],(char**)r[2]);
        default:
        kdebug(D_IRQ, 10, "Undefined SWI. %#02x\n", r[7]);
    }
    while(1);
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void) {
    uint32_t* r = interrupt_reg;
    kdebug(D_IRQ, 10, "PREFECTH_ABORT at instruction %#010x. Registers:\n", r[14]-8);
    print_registers(10);
    while(1);
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) data_abort_vector(void) {
    uint32_t* r = interrupt_reg;
    kdebug(D_IRQ, 10, "DATA_ABORT at instruction %#010x. Registers:\n", r[14]-8);
    print_registers(10);
    while(1);
}


void enable_interrupts(void) {
    kdebug(D_IRQ, 1, "Enabling interrupts.\n");
    asm volatile("cpsie i");
}

void disable_interrupts(void) {
    kdebug(D_IRQ, 1, "Disabling interrupts.\n");
    asm volatile("cpsid i");
}
