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
extern uint32_t current_process;

// In IRQ mode - r0 => SP
void interrupt_vector(void* user_context) {
	process* p = get_process_list()[current_process];
	p->ctx = *(user_context_t*)user_context; // Save current program context.

    kdebug(D_IRQ, 2, "=> %d.\n", current_process);
	print_context(2, user_context);

	if (RPI_GetIRQController()->IRQ_basic_pending == RPI_BASIC_ARM_TIMER_IRQ) {
		dmb();
		Timer_ClearInterrupt();
		GPIO_setPinValue(GPIO_LED_PIN, status);
		status = !status;
		dmb();

		current_process = get_next_process();
		p = get_process_list()[current_process];
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		*(user_context_t*)user_context = p->ctx;
	} else {
		kdebug(D_IRQ, 10, "[ERROR] Unhandled IRQ!\n");
		while(1) {}
	}
    kdebug(D_IRQ, 2, "<= %d.\n", current_process);
	print_context(2, user_context);
}


void print_context(int level, user_context_t* ctx) {
    kdebug(D_IRQ, level, "r0 :%#010x r1:%#010x r2 :%#010x r3 :%#010x\n", ctx->r[0], ctx->r[1], ctx->r[2], ctx->r[3]);
    kdebug(D_IRQ, level, "r4 :%#010x r5:%#010x r6 :%#010x r7 :%#010x\n", ctx->r[4], ctx->r[5], ctx->r[6], ctx->r[7]);
    kdebug(D_IRQ, level, "r8 :%#010x r9:%#010x r10:%#010x r11:%#010x\n", ctx->r[8], ctx->r[9], ctx->r[10], ctx->r[11]);
    kdebug(D_IRQ, level, "fp :%#010x sp:%#010x lr :%#010x pc :%#010x\n", ctx->r[12], ctx->r[13], ctx->r[14], ctx->pc);
    kdebug(D_IRQ, level, "CPSR: %#010x\n", ctx->cpsr);
}

// In SVC mode
uint32_t software_interrupt_vector(void* user_context) {
    kdebug(D_IRQ, 1, "ENTREESWI.\n");
	user_context_t* ctx = (user_context_t*) user_context;
	print_context(1, ctx);
	process* p = get_process_list()[current_process];
	p->ctx = *ctx;
	int res;

    switch(ctx->r[7]) {
        case SVC_EXIT:
			res = svc_exit();
			break;
        case SVC_SBRK:
			res = svc_sbrk(ctx->r[0]);
			break;
		case SVC_FORK:
			res = svc_fork();
			break;
        case SVC_WRITE:
            res = svc_write(ctx->r[0],(char*)ctx->r[1],ctx->r[2]);
			break;
        case SVC_CLOSE:
            res = svc_close(ctx->r[0]);
			break;
		case SVC_WAITPID:
			res = svc_waitpid(ctx->r[0],(int*) ctx->r[1], ctx->r[2]);
			break;
        case SVC_FSTAT:
            res = svc_fstat(ctx->r[0],(struct stat*)ctx->r[1]);
			break;
        case SVC_LSEEK:
            kdebug(D_IRQ, 2, "LSEEK %d %d %d \n", ctx->r[0], ctx->r[1], ctx->r[2]);
			res = 0;
            break;
        case SVC_READ:
            res = svc_read(ctx->r[0],(char*)ctx->r[1],ctx->r[2]);
			break;
		case SVC_TIME:
			res = svc_time((time_t*)ctx->r[0]);
			break;
		case SVC_EXECVE:
			res = svc_execve((char*)ctx->r[0],(const char**)ctx->r[1],(const char**)ctx->r[2]);
			break;
		case SVC_GETCWD:
			res = svc_getcwd((char*)ctx->r[0],ctx->r[1]);
			break;
		case SVC_CHDIR:
			res = svc_chdir((char*)ctx->r[0]);
        default:
        kdebug(D_IRQ, 10, "Undefined SWI. %#02x\n", ctx->r[7]);
		while(1) {}
    }

	if ((ctx->r[7] == SVC_EXIT)
	|| 	(ctx->r[7] == SVC_EXECVE)
	||	(ctx->r[7] == SVC_WAITPID && res == -1)) {
		p = get_process_list()[current_process];
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		*ctx = p->ctx; // Copy next process ctx
	} else {
		ctx->r[0] = res; // let's return the result in r0
	}

	print_context(1, ctx);
	return 0;
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void* data) {
	user_context_t* ctx = (user_context_t*) data;
	kdebug(D_IRQ, 10, "PREFETCH ABORT at instruction %#010x.\n", ctx->pc-8);
	print_context(10, ctx);
    while(1);
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) data_abort_vector(void* data) {
	user_context_t* ctx = (user_context_t*) data;
	kdebug(D_IRQ, 10, "DATA ABORT at instruction %#010x.\n", ctx->pc-8);
	print_context(10, ctx);
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
