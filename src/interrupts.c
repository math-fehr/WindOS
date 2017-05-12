#include "interrupts.h"

#include "arm.h"

/**
 * function used to make sure data is sent in order to GPIO
 */

static rpi_irq_controller_t* rpiIRQController =
  (rpi_irq_controller_t*) RPI_INTERRUPT_CONTROLLER_BASE;

static interruptHandler IRQInterruptHandlers[NUMBER_IRQ_INTERRUPTS];

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

extern uint32_t current_process_id;



void print_context(int level, user_context_t* ctx) {
    kdebug(D_IRQ, level, "r0 :%#010x r1:%#010x r2 :%#010x r3 :%#010x\n", ctx->r[0], ctx->r[1], ctx->r[2], ctx->r[3]);
    kdebug(D_IRQ, level, "r4 :%#010x r5:%#010x r6 :%#010x r7 :%#010x\n", ctx->r[4], ctx->r[5], ctx->r[6], ctx->r[7]);
    kdebug(D_IRQ, level, "r8 :%#010x r9:%#010x r10:%#010x r11:%#010x\n", ctx->r[8], ctx->r[9], ctx->r[10], ctx->r[11]);
    kdebug(D_IRQ, level, "fp :%#010x sp:%#010x lr :%#010x pc :%#010x\n", ctx->r[12], ctx->r[13], ctx->r[14], ctx->pc);
    kdebug(D_IRQ, level, "CPSR: %#010x\n", ctx->cpsr);
}

uint32_t software_interrupt_vector(void* user_context);

int count;


// In IRQ mode - r0 => SP
void interrupt_vector(void* user_context) {
	user_context_t* uc = user_context;
	kernel_printf("fp: %p\n", uc->r[12]);
	process* p = get_process_list()[current_process_id];
	p->ctx = *(user_context_t*)user_context; // Save current program context.

    kdebug(D_IRQ, 5, "T=> %d.\n", current_process_id);
	print_context(5, user_context);

    dmb();

	uint32_t irq = RPI_GetIRQController()->IRQ_basic_pending;

	if (irq & (1 << 8)) {
		if (RPI_GetIRQController()->IRQ_pending_1 & (1 << 29)) {
			kernel_printf("aux\n");
			serial_irq();
		} else {
			kernel_printf("ghost\n");
			return;
		}
	} else if (irq & RPI_BASIC_ARM_TIMER_IRQ) {
		count++;
		//kernel_printf("timer\n");
		dmb();
		Timer_ClearInterrupt();

		if (count == 100) {
			count = 0;
			GPIO_setPinValue(GPIO_LED_PIN, status);
			status = !status;
		}
		dmb();

		current_process_id = get_next_process();
		if (current_process_id == -1) {
			kdebug(D_IRQ, 10,
		"Every one is dead. Only the void remains. In the distance, sirens.\n");
			while(1) {} // TODO: Reboot?
		}
		p = get_process_list()[current_process_id];
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		*(user_context_t*)user_context = p->ctx;
		if (p->status == status_blocked_svc) {
			software_interrupt_vector(user_context);
		}
	} else {
		kernel_printf("ghost2\n");
		// Ghost interrupt.
		return;
	}

    dmb();
    for(uint32_t i = 0; i<NUMBER_IRQ_INTERRUPTS; i++) {
        if(i<32) {
            if(RPI_GetIRQController()->IRQ_pending_1 & (1UL << i)) {
                RPI_GetIRQController()->IRQ_pending_1 = (1UL << i);
                if(IRQInterruptHandlers[i].function != NULL) {
                    IRQInterruptHandlers[i].function(IRQInterruptHandlers[i].param);
                    dmb();
                }
                else {
                    RPI_GetIRQController()->Disable_IRQs_1 = (1 << i);
                }
            }
        }
        else {
            if(RPI_GetIRQController()->IRQ_pending_2 & (1UL << (i&31))) {
                RPI_GetIRQController()->IRQ_pending_2 = (1UL << (i&31));
                if(IRQInterruptHandlers[i].function != NULL) {
                    IRQInterruptHandlers[i].function(IRQInterruptHandlers[i].param);
                    dmb();
                }
                else {
                    RPI_GetIRQController()->Disable_IRQs_2 = (1UL << (i&31));
                }
            }
        }
    }
    dmb();

    kdebug(D_IRQ, 2, "<= %d.\n", current_process_id);
	print_context(2, user_context);
}


// In SVC mode
uint32_t software_interrupt_vector(void* user_context) {
	uint32_t irq;
    kdebug(D_IRQ,10, "ENTREESWI. %p \n", user_context);
	user_context_t* ctx = (user_context_t*) user_context;
	if (0xfeff726b == ctx->r[12]) {
		while(1) {}
	}
	kernel_printf("fp: %p\n", ctx->r[12]);
    kdebug(D_IRQ, 5, "S=> %d.\n", current_process_id);

	print_context(5, ctx);
	process* p;
swi_beg:
	p = get_process_list()[current_process_id];
	p->ctx = *ctx;
	uint32_t res;

    switch(ctx->r[7]) {
		case SVC_IOCTL:
			res = svc_ioctl(ctx->r[0],ctx->r[1],ctx->r[2]);
			break;
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
			res = svc_lseek(ctx->r[0],ctx->r[1],ctx->r[2]);
            break;
        case SVC_READ:
            res = svc_read(ctx->r[0],(char*)ctx->r[1],ctx->r[2]);
			if (p->status == status_blocked_svc) {
				current_process_id = get_next_process();
				int n = get_number_active_processes();
				process** p_list = get_process_list();
				/*kernel_printf("f => %d %d\n", current_process_id, n);
				for (int i=0;i<n;i++) {
				 	kernel_printf(">%d: %d\n", i, p_list[get_active_processes()[i]]->dummy);
				}*/
			}
			break;
		case SVC_TIME:
			res = svc_time((time_t*)ctx->r[0]);
			break;
		case SVC_EXECVE:
			res = svc_execve((char*)ctx->r[0],(const char**)ctx->r[1],(const char**)ctx->r[2]);
			break;
		case SVC_GETCWD:
			res = (uint32_t)svc_getcwd((char*)ctx->r[0],ctx->r[1]);
			break;
		case SVC_CHDIR:
			res = svc_chdir((char*)ctx->r[0]);
			break;
		case SVC_GETDENTS:
			res = svc_getdents(ctx->r[0], (struct dirent *)ctx->r[1]);
			break;
		case SVC_OPEN:
			res = svc_open((char*) ctx->r[0], ctx->r[1]);
			break;
        default:
        kdebug(D_IRQ, 10, "Undefined SWI. %#02x\n", ctx->r[7]);
		while(1) {}
    }

	if ((ctx->r[7] == SVC_EXIT)
	|| 	(ctx->r[7] == SVC_EXECVE)
    ||	(ctx->r[7] == SVC_WAITPID && res == (uint32_t)-1)
	||  (p->status == status_blocked_svc)) {
		p = get_process_list()[current_process_id];
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		*ctx = p->ctx; // Copy next process ctx
		if (p->status == status_blocked_svc) {
			*(user_context_t*)user_context = p->ctx;
			goto swi_beg;
		}
	} else {
		ctx->r[0] = res; // let's return the result in r0
	}

	print_context(5, ctx);
    kdebug(D_IRQ, 5, "S<= %d.\n", current_process_id);
	return 0;
}


static char* messages[] =
{
	"",
	"Alignment fault",
	"Debug event",
	"Access flag fault, section",
	"Instruction cache maintenance fault",
	"Translation fault, section",
	"Access flag fault, page",
	"Translation fault, page",
	"Synchronous external abort, non-translation",
	"Domain fault, section",
	"",
	"Domain fault, page",
	"Synchronous external abort on ttw 1",
	"Permission fault, section",
	"Synchronous external abort on ttw 2",
	"Permission fault, page"
};

extern uint32_t __ram_size;

void kern_debug() {
	char buf[1024];
	serial_setmode(1);
	paging_print_status();
	while(1) {
		while(serial_readline(buf, 1024) == 0){}
		uintptr_t val;
		sscanf(buf, "%x", &val);
		uintptr_t phy = mmu_vir2phy(val);
		if (phy <= __ram_size) {
			kernel_printf("phi: %010p\n", phy);
			phy += 0x80000000;
			kernel_printf("%010p: %010p.\n", val, *((int*)phy));
		} else {
			kernel_printf("%010p: denied.\n", val);
		}
	}
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void* data) {
	user_context_t* ctx = (user_context_t*) data;
	kdebug(D_IRQ, 10, "PREFETCH ABORT at instruction %#010x.\n", ctx->pc-8);
	print_context(10, ctx);

	uint32_t reg = mrc(p15, 0, c5, c0, 1);
	uint32_t status = reg & 0xF;

	kdebug(D_IRQ, 10, "\"%s\" occured.\n", messages[status]);
	kern_debug();

    while(1);
}

// In ABORT mode
void data_abort_vector(void* data) {
	user_context_t* ctx = (user_context_t*) data;
	if ((ctx->cpsr & 0x1F) == 0x10) {
		kdebug(D_IRQ, 10, "USER DATA ABORT of process %d at instruction %#010x.\n", current_process_id, ctx->pc-8);
		print_context(10, ctx);
		uint32_t reg = mrc(p15, 0, c5, c0, 0);
		uint32_t status = reg & 0xF;
		uint32_t domain = (reg & 0xF0) >> 4;
		bool wnr = reg & (1 << 11);

		kdebug(D_IRQ, 10, "\"%s\" occured on domain %d. (w=%d)\n", messages[status], domain, wnr);
		kern_debug();
		kill_process(current_process_id, -1);
		current_process_id = get_next_process();
		if (current_process_id == -1) {
			kdebug(D_IRQ, 10, "No process left.\n");
			while(1) {}
		}
		kdebug(D_IRQ, 10, "Switching to %d.\n", current_process_id);
		process* p = get_process_list()[current_process_id];
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		*ctx = p->ctx; // Copy next process ctx
	} else {
		kdebug(D_IRQ, 10, "KERNEL DATA ABORT at instruction %#010x.\n", ctx->pc-8);
		print_context(10, ctx);
		uint32_t reg = mrc(p15, 0, c5, c0, 0);
		uint32_t status = reg & 0xF;
		uint32_t domain = (reg & 0xF0) >> 4;
		bool wnr = reg & (1 << 11);

		kdebug(D_IRQ, 10, "\"%s\" occured on domain %d. (w=%d)\n", messages[status], domain, wnr);
		kern_debug();

	    while(1);
	}
}



void init_irq_interruptHandlers(void) {
    for(int i = 0; i<NUMBER_IRQ_INTERRUPTS; i++) {
        IRQInterruptHandlers[i].function = NULL;
        IRQInterruptHandlers[i].param = NULL;
    }
}


void connectIRQInterrupt(unsigned int irqID, interruptFunction* function, void* param) {
    IRQInterruptHandlers[irqID].function = function;
    IRQInterruptHandlers[irqID].param = param;
    if(irqID < 32) {
        RPI_GetIRQController()->Enable_IRQs_1 = (uint32_t)1 << (uint32_t)irqID;
    }
    else {
        irqID &= 32;
        RPI_GetIRQController()->Enable_IRQs_2 = (uint32_t)1 << (uint32_t)irqID;
    }
}


void enable_interrupts(void) {
    kdebug(D_IRQ, 1, "Enabling interrupts.\n");
    cleanDataCache();
    dsb();

    invalidateInstructionCache();
    flush_branch_prediction();
    dsb();

    isb();
    dmb();

    rpi_irq_controller_t* controller =  RPI_GetIRQController();
    controller->FIQ_control = 0;
    controller->Disable_IRQs_1 = (uint32_t) -1;
    controller->Disable_IRQs_2 = (uint32_t) -1;
    controller->Disable_Basic_IRQs = (uint32_t) -1;
    controller->IRQ_basic_pending = controller->IRQ_basic_pending;
    controller->IRQ_pending_1 = controller->IRQ_pending_1;
    controller->IRQ_pending_2 = controller->IRQ_pending_2;

    dmb();

    asm volatile("cpsie i");
}

void disable_interrupts(void) {
    kdebug(D_IRQ, 1, "Disabling interrupts.\n");
    asm volatile("cpsid i");
}
