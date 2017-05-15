/** \file interrupts.c
 *	\brief Interrupt handling features.
 *
 *	This code is called by interrupts_asm.S to handle ARM interupts.
 */

#include "interrupts.h"


/** \var volatile rpi_irq_controller_t* rpiIRQController
 *  \brief This structure controls the hardware registers ot the interrupt
 * 	controller.
 */
volatile rpi_irq_controller_t* rpiIRQController =
  (rpi_irq_controller_t*) RPI_INTERRUPT_CONTROLLER_BASE;

/** \var static interruptHandler IRQInterruptHandlers[NUMBER_IRQ_INTERRUPTS]
 * 	\brief Contains the handler for ARM interrupts.
 */
static interruptHandler IRQInterruptHandlers[NUMBER_IRQ_INTERRUPTS];

/** \var rpi_irq_controller_t* RPI_GetIRQController(void)
 *	\brief A getter function for the IRQ controller.
 */
rpi_irq_controller_t* RPI_GetIRQController(void) {
  return rpiIRQController;
}

/** \fn void __attribute__ ((interrupt("UNDEF"))) undefined_instruction_vector (void)
 * 	\brief Handler called when the instruction parsing failed.
 */
void __attribute__ ((interrupt("UNDEF"))) undefined_instruction_vector(void) {
  kdebug(D_IRQ, 5, "UNDEFINED INSTRUCTION.\n");
  while(1);
}

/** \fn void __attribute__ ((interrupt("FIQ"))) fast_interrupt_vector (void)
 * 	\brief Handler called on fast interrupt. Not used in our context.
 */
void __attribute__ ((interrupt("FIQ"))) fast_interrupt_vector(void) {
  kdebug(D_IRQ, 0, "FIQ.\n");
  while(1);
}


/** \var print_context(int id, int level, user_context_t* ctx)
 * 	\brief Prints the context of usermode before the interrupt.
 *	\param id Debug source id.
 * 	\param level Debug severity.
 * 	\param user_context_t* Pointer to the context to print.
 */
void print_context(int id, int level, user_context_t* ctx) {
    kdebug(id, level, "r0 :%#010x r1:%#010x r2 :%#010x r3 :%#010x\n", ctx->r[0], ctx->r[1], ctx->r[2], ctx->r[3]);
    kdebug(id, level, "r4 :%#010x r5:%#010x r6 :%#010x r7 :%#010x\n", ctx->r[4], ctx->r[5], ctx->r[6], ctx->r[7]);
    kdebug(id, level, "r8 :%#010x r9:%#010x r10:%#010x r11:%#010x\n", ctx->r[8], ctx->r[9], ctx->r[10], ctx->r[11]);
    kdebug(id, level, "fp :%#010x sp:%#010x lr :%#010x pc :%#010x\n", ctx->r[12], ctx->r[13], ctx->r[14], ctx->pc);
    kdebug(id, level, "CPSR: %#010x\n", ctx->cpsr);
}

uint32_t software_interrupt_vector(void* user_context);

/** \var static bool status
 * 	\brief LED status.
 *
 * 	The ACT LED should blink every 100 interrupts.
 */
static bool status;

/** \var static int count
 * 	\brief LED status count.
 */
int count;


/**	\fn void interrupt_vector(void* user_context)
 *	\brief Hardware interrupt handler.
 * 	\param void* Pointer to the usermode context.
 *
 * 	The usermode context is gathered by interrupts_asm.S
 * 	It should be saved on the current process data.
 *
 * 	If the interruption is triggered by the Timer, it performs a process switch.
 * 	The scheduler choses a new process and updates user_context to this new
 *	process' context. When this function returns, the context is restored.
 */
void interrupt_vector(void* user_context) {
    callInterruptHandlers();

	user_context_t* uc = user_context;
    kdebug(D_IRQ,5,"ENTREEIRQ\n");
	kdebug(D_IRQ,5,"fp: %p\n", uc->r[12]);
	process* p = get_current_process();
    if(p != NULL) {
        p->ctx.pc -= 4;
        p->ctx = *(user_context_t*)user_context; // Save current program context.
        print_context(D_IRQ,5, user_context);
    }

    dmb();

	uint32_t irq = RPI_GetIRQController()->IRQ_basic_pending;

	if (irq & (1 << 8)) {
		if (RPI_GetIRQController()->IRQ_pending_1 & (1 << 29)) {
			kernel_printf("aux\n");
			serial_irq();
		} else {
			kdebug(D_IRQ, 10, "[ERROR] Unhandled IRQ (%X)!\n",irq);
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


		p = get_next_process();
		if (p == NULL) {
			kdebug(D_IRQ, 10,
		"Every one is dead. Only the void remains. In the distance, sirens.\n");
            return; //We are probably in the kernel
		}

	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
        *(user_context_t*)user_context = p->ctx;
		if (p->status == status_blocked_svc) {
			software_interrupt_vector(user_context);
		}
	}

    kdebug(D_IRQ, 7, "<= %d.\n", get_current_process_id());
	print_context(D_IRQ, 2, user_context);
}



/**	\var uint32_t software_interrupt_vector(void* user_context)
 *	\brief Software interrupt handler.
 * 	\param void* Pointer to the usermode context.
 * 	The usermode context is gathered by interrupts_asm.S
 * 	It should be saved on the current process data.
 *
 * 	Here the process called for a kernel feature.
 *	This function decodes the call, and branches to the functions of syscalls.c.
 *	Some system calls can cause a process switch, these are handler the same way
 *	as in interrupt_vector().
 */
uint32_t software_interrupt_vector(void* user_context) {
    kdebug(D_IRQ, 5, "ENTREESWI. %p \n", user_context);
	user_context_t* ctx = (user_context_t*) user_context;
	//if (0xfeff726b == ctx->r[12]) {
	//	while(1) {}
	//}
	if (ctx->r[7] != SVC_READ) {
	    kdebug(D_SYSCALL, 5, "S=> %d.\n", get_current_process_id());
		print_context(D_SYSCALL, 5, ctx);
	}
	process* p;
swi_beg:
	p = get_current_process();
	p->ctx = *ctx;
	uint32_t res;

	int r_bef = ctx->r[7];

    switch(ctx->r[7]) {
		case SVC_IOCTL:
            kdebug(D_SYSCALL,5,"ENTREEIOCTL\n");
			res = svc_ioctl(ctx->r[0],ctx->r[1],ctx->r[2]);
			break;
        case SVC_EXIT:
            kdebug(D_SYSCALL,5,"ENTREEEXIT\n");
			res = svc_exit();
			break;
        case SVC_SBRK:
            kdebug(D_SYSCALL,5,"ENTREESBRK\n");
			res = svc_sbrk(ctx->r[0]);
			break;
		case SVC_FORK:
            kdebug(D_SYSCALL,5,"ENTREEFORK\n");
			res = svc_fork();
			break;
        case SVC_WRITE:
            kdebug(D_SYSCALL,5,"ENTREEWRITE\n");
            res = svc_write(ctx->r[0],(char*)ctx->r[1],ctx->r[2]);
            kdebug(D_IRQ,5,"SORTIEWRITE\n");
			break;
        case SVC_CLOSE:
            kdebug(D_SYSCALL,5,"ENTREECLOSE\n");
            res = svc_close(ctx->r[0]);
			break;
		case SVC_WAITPID:
            kdebug(D_SYSCALL,5,"ENTREEWAITPID\n");
			res = svc_waitpid(ctx->r[0],(int*) ctx->r[1], ctx->r[2]);
			break;
        case SVC_FSTAT:
            kdebug(D_SYSCALL,5,"ENTREEFSTAT\n");
            res = svc_fstat(ctx->r[0],(struct stat*)ctx->r[1]);
			break;
        case SVC_LSEEK:
            kdebug(D_SYSCALL,5,"ENTREELSEEK\n");
			res = svc_lseek(ctx->r[0],ctx->r[1],ctx->r[2]);
            break;
        case SVC_READ:
            //kdebug(D_IRQ,5,"ENTREEREAD\n");
            res = svc_read(ctx->r[0],(char*)ctx->r[1],ctx->r[2]);
			if (p->status == status_blocked_svc) {
				get_next_process();
				/*int n = get_number_active_processes();
				process** p_list = get_process_list();
				kernel_printf("f => %d %d\n", current_process_id, n);
				for (int i=0;i<n;i++) {
				 	kernel_printf(">%d: %d\n", i, p_list[get_active_processes()[i]]->dummy);
				}*/
			}
			break;
		case SVC_TIME:
            kdebug(D_SYSCALL,5,"ENTREETIME\n");
			res = svc_time((time_t*)ctx->r[0]);
			break;
		case SVC_EXECVE:
            kdebug(D_SYSCALL,5,"ENTREEEXECVE\n");
			res = svc_execve((char*)ctx->r[0],(const char**)ctx->r[1],(const char**)ctx->r[2]);
            kdebug(D_IRQ,5,"SORTIEEXECVE\n");
			break;
		case SVC_GETCWD:
            kdebug(D_SYSCALL,5,"ENTREEGETCWD\n");
			res = (uint32_t)svc_getcwd((char*)ctx->r[0],ctx->r[1]);
			break;
		case SVC_CHDIR:
            kdebug(D_SYSCALL,5,"ENTREECHDIR\n");
			res = svc_chdir((char*)ctx->r[0]);
			break;
		case SVC_GETDENTS:
            kdebug(D_SYSCALL,5,"ENTREEGETENTS\n");
			res = svc_getdents(ctx->r[0], (struct dirent *)ctx->r[1]);
			break;
		case SVC_OPENAT:
            kdebug(D_SYSCALL,5,"ENTREEOPENAT\n");
			res = svc_openat(ctx->r[0], (char*) ctx->r[1], ctx->r[2]);
			break;
		case SVC_MKNODAT:
			kdebug(D_SYSCALL,5,"ENTREEMKNODAT\n");
			res = svc_mknodat(ctx->r[0], (char*) ctx->r[1], ctx->r[2], ctx->r[3]);
			break;
		case SVC_UNLINKAT:
			res = svc_unlinkat(ctx->r[0], (char*) ctx->r[1], ctx->r[2]);
			break;
        default:
        kdebug(D_IRQ, 10, "Undefined SWI. %#02x\n", ctx->r[7]);
		while(1) {}
    }

	if ((ctx->r[7] == SVC_EXIT)
	|| 	(ctx->r[7] == SVC_EXECVE)
    ||	(ctx->r[7] == SVC_WAITPID && res == (uint32_t)-1)
	||  (p->status == status_blocked_svc)) {
		p = get_current_process();
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		*ctx = p->ctx; // Copy next process ctx
		if (p->status == status_blocked_svc) {
			*(user_context_t*)user_context = p->ctx;
			goto swi_beg;
		}
	} else {
		ctx->r[0] = res; // let's return the result in r0
	}

	if (r_bef != SVC_READ) {
		print_context(D_SYSCALL, 5, ctx);
	    kdebug(D_SYSCALL, 5, "Sortie SWI : <= %d.\n", get_current_process_id());
	}
	return 0;
}

/** \var static char* messages[]
 * 	\brief Messages triggered during a data abort
 */
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

/** \fn void kern_debug()
 *	\brief Memory inspection function, when everything crashed.
 * 	This is an infinite loop that allows us to inspect memory when an abort
 *	occured.
 */
void kern_debug() {
	char buf[1024];
	serial_setmode(1);
	paging_print_status();
	while(1) {
		while(serial_readline(buf, 1024) == 0){}
		uintptr_t val;
		sscanf(buf, "%x", (unsigned*)(&val));
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

/** \fn void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void* data)
 *	\brief Prefetch abort interrupt handler.
 */
void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void* data) {
	user_context_t* ctx = (user_context_t*) data;
	kdebug(D_IRQ, 10, "PREFETCH ABORT at instruction %#010x.\n", ctx->pc-8);
	print_context(D_IRQ,10, ctx);

	uint32_t reg = mrc(p15, 0, c5, c0, 1);
	uint32_t status = reg & 0xF;

	kdebug(D_IRQ, 10, "\"%s\" occured.\n", messages[status]);
	kern_debug();

    while(1);
}

/** \fn void data_abort_vector(void* data)
 *	\brief Data abort interrupt handler.
 *
 * 	When the MMU signals a data abort, check if it caused by the kernel or a
 * 	process. In case of a process, kill it. If this is the kernel, branch into
 * 	the last resort debug tool.
 */
void data_abort_vector(void* data) {
	user_context_t* ctx = (user_context_t*) data;
	if ((ctx->cpsr & 0x1F) == 0x10) {
		kdebug(D_IRQ, 10, "USER DATA ABORT of process %d at instruction %#010x.\n", get_current_process_id(), ctx->pc-8);
		print_context(D_IRQ,10, ctx);
		uint32_t reg = mrc(p15, 0, c5, c0, 0);
		uint32_t status = reg & 0xF;
		uint32_t domain = (reg & 0xF0) >> 4;
		bool wnr = reg & (1 << 11);

		kdebug(D_IRQ, 10, "\"%s\" occured on domain %d. (w=%d)\n", messages[status], domain, wnr);
		kern_debug();
		kill_process(get_current_process_id(), -1); //we are sure a running process exist
		process* p = get_next_process();
		if (p == NULL) {
			kdebug(D_IRQ, 10, "No process left.\n");
			while(1) {}
		}
		kdebug(D_IRQ, 10, "Switching to %d.\n", get_current_process_id());
		p = get_current_process();
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		*ctx = p->ctx; // Copy next process ctx
	} else {
		kdebug(D_IRQ, 10, "KERNEL DATA ABORT at instruction %#010x.\n", ctx->pc-8);
		print_context(D_IRQ,10, ctx);
		uint32_t reg = mrc(p15, 0, c5, c0, 0);
		uint32_t status = reg & 0xF;
		uint32_t domain = (reg & 0xF0) >> 4;
		bool wnr = reg & (1 << 11);

		kdebug(D_IRQ, 10, "\"%s\" occured on domain %d. (w=%d)\n", messages[status], domain, wnr);
		kern_debug();
	    while(1);
	}
}

/*	\fn void init_irq_interruptHandlers(void)
 *	\brief Initialize general interrupt handlers.
 */
void init_irq_interruptHandlers(void) {
    for(int i = 0; i<NUMBER_IRQ_INTERRUPTS; i++) {
        IRQInterruptHandlers[i].function = NULL;
        IRQInterruptHandlers[i].param = NULL;
    }
}


/**	\fn void connectIRQInterrupt(unsigned int irqID, interruptFunction* function, void* param)
 *	\brief Connects a general interrupt to a particular handler.
 */
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

/**	\fn void callInterruptHandlers()
 *	\brief When an interruption is set, choose which handler to execute.
 */
void callInterruptHandlers() {
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
}

/** \fn void enable_interrupts(void)
 *	\brief Enable interrupts
 */
void enable_interrupts(void) {
    /*kdebug(D_IRQ, 1, "Enabling interrupts.\n");
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

    asm volatile("cpsie i");*/
    RPI_GetIRQController()->Enable_Basic_IRQs = 1;
    asm volatile("cpsie i");
}

/** \fn void disable_interrupts(void)
 *	\brief Disable interrupts
 */
void disable_interrupts(void) {
    kdebug(D_IRQ, 1, "Disabling interrupts.\n");
    asm volatile("cpsid i");
}
