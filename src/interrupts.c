#include "interrupts.h"
#include "timer.h"
#include "serial.h"
#include "debug.h"
#include "scheduler.h"
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

// In IRQ mode
void __attribute__ ((interrupt("IRQ"))) interrupt_vector(void) {
	if (RPI_GetIRQController()->IRQ_basic_pending == RPI_BASIC_ARM_TIMER_IRQ) {
		kdebug(D_IRQ, 0, "Timer\n");
		dmb();
		Timer_ClearInterrupt();
		GPIO_setPinValue(GPIO_LED_PIN, status);
		status = !status;
		dmb();
	} else {
		kdebug(D_IRQ, 10, "[ERROR] Unhandled IRQ!\n");
		while(1) {}
	}
}

uint32_t interrupt_reg[17];

extern uint32_t current_process_id;

void print_registers(int level) {
    uint32_t* r = interrupt_reg;

    kdebug(D_IRQ, level, "r0 :%#010x r1:%#010x r2 :%#010x r3 :%#010x\n", r[0], r[1], r[2], r[3]);
    kdebug(D_IRQ, level, "r4 :%#010x r5:%#010x r6 :%#010x r7 :%#010x\n", r[4], r[5], r[6], r[7]);
    kdebug(D_IRQ, level, "r8 :%#010x r9:%#010x r10:%#010x r11:%#010x\n", r[8], r[9], r[10], r[11]);
    kdebug(D_IRQ, level, "r12:%#010x sp:%#010x lr :%#010x pc :%#010x\n", r[12], r[13], r[14], r[15]);
    //kdebug(D_IRQ, level, "CPSR: %#010x\n", r[16]);
}

// In SVC mode
// returns were we should branch to next
uint32_t software_interrupt_vector(void) {
    kdebug(D_IRQ, 1, "SWI.\n");
    print_registers(1);
    uint32_t* r = interrupt_reg;
    switch(r[7]) {
        case SVC_EXIT:
            kdebug(D_IRQ, 10, "Program wants to quit (switch him to zombie state)\n");
            while(1) {}
        case SVC_SBRK:
            kdebug(D_IRQ, 2, "SBRK %d\n", r[0]);
            process* p = get_process_list()[current_process_id];
            // TODO: check that the program doesn't fuck it up
            int old_brk         = p->brk;
            int current_brk     = old_brk+r[0];
            int pages_needed    = (current_brk - 1) / (PAGE_SECTION); // As the base brk is PAGE_SECTION (should be moved though)

            if (pages_needed > p->brk_page) {
                page_list_t* pages = paging_allocate(pages_needed - p->brk_page);
                while (pages != NULL) {
                    while (pages->size > 0) {
                        mmu_add_section(p->ttb_address,(p->brk_page+1)*PAGE_SECTION,pages->address*PAGE_SECTION,0); // TODO: setup flags
                        pages->size--;
                        pages->address++;
                        p->brk_page++;
                    }
                    page_list_t* bef;
                    bef = pages;
                    pages = pages->next;
                    free(bef);
                }
                if (pages_needed != p->brk_page) {
                    kdebug(D_IRQ, 10, "SBRK: ENOMEM %d %d\n", pages_needed, p->brk_page);
                    errno = ENOMEM;
                    return -1;
                }
            } else if (pages_needed < p->brk_page) {
                // should free pages.
            }
            p->brk = p->brk + r[0];
            kdebug(D_IRQ, 2, "SBRK => %#010x\n", old_brk);
            return old_brk;
		case SVC_FORK:
			///process* p = get_process_list()[current_process_id];

			return 0;
        case SVC_WRITE:
            kdebug(D_IRQ, 2, "WRITE %d %#010x %#010x\n", r[0], r[1], r[2]);
            //int fd = r[0];
            char* buf = (char*)(intptr_t)r[1];
            size_t cnt = r[2];

            if (r[0] >= MAX_OPEN_FILES) {
                return -1;
            }

            fd_t* fd = &get_process_list()[current_process_id]->fd[r[0]];
            if (fd->position >= 0) {
                int n = vfs_fwrite(fd->inode, buf, cnt, 0);
                kdebug(D_IRQ, 2, "WRITE => %d\n",n);
				//fd->position += n;
                return n;
            } else {
                return -1;
            }

        case SVC_CLOSE:
            kdebug(D_IRQ, 10, "CLOSE %d\n", r[0]);
            return 0;
        case SVC_FSTAT:
            kdebug(D_IRQ, 2, "FSTAT %d %#010x\n", r[0], r[1]);
            if (r[0] >= MAX_OPEN_FILES) {
                return -1;
            }
            fd_t req_fd = get_process_list()[current_process_id]->fd[r[0]];
            if (req_fd.position >= 0) {
                memcpy((struct stat*)(uintptr_t)r[1],&req_fd.inode->st,sizeof(struct stat));
                kdebug(D_IRQ, 2, "FSTAT OK\n");
                return 0;
            } else {
                kdebug(D_IRQ, 5, "ERR\n");
                return -1;
            }
        case SVC_LSEEK:
            kdebug(D_IRQ, 10, "LSEEK %d\n", r[0]);
            while(1) {}
        case SVC_READ:
            kdebug(D_IRQ, 2, "READ %d %#010x %d\n", r[0], r[1], r[2]);
            char* w_buf = (char*)(intptr_t)r[1];
            size_t w_cnt = r[2];

			if (r[0] >= MAX_OPEN_FILES) {
                return -1;
            }

			fd_t* w_fd = &get_process_list()[current_process_id]->fd[r[0]];
			if (w_fd->position >= 0) {
				int n = vfs_fread(w_fd->inode, w_buf, w_cnt, w_fd->position);
				//w_fd->position += n;
				kdebug(D_IRQ, 2, "READ => %d\n",n);
				return n;
			} else {
				return -1;
			}
        default:
        kdebug(D_IRQ, 10, "Undefined SWI.\n");
    }
    while(1);
}

// In ABORT mode
void __attribute__ ((interrupt("ABORT"))) prefetch_abort_vector(void) {
    uint32_t* r = interrupt_reg;
    kdebug(D_IRQ, 5, "PREFECTH_ABORT at instruction %#010x. Registers:\n", r[14]-8);
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
