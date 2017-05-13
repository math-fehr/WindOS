#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "kernel.h"
#include "errno.h"
#include "mmu.h"
#include "debug.h"
#include "scheduler.h"
#include "syscalls.h"

#include "arm.h"

/**
 * Theses functions handle interrupts made by the processor
 */


#define ALIGNMENT_FAULT 		0b0001
#define DEBUG_EVENT 			0b0010
#define ACCESS_FAULT_SEC 		0b0011
#define INSTR_CACHE_FAULT 		0b0100
#define TRANSLATION_FAULT_SEC 	0b0101
#define ACCESS_FAULT_PAGE  		0b0110
#define TRANSLATION_FAULT_PAGE	0b0111
#define SYNC_EXT_ABORT 			0b1000
#define DOMAIN_FAULT_SEC 		0b1001
#define DOMAIN_FAULT_PAGE 		0b1011
#define SYNC_EXT_ABORT_TBL_1 	0b1100
#define PERMISSION_FAULT_SEC	0b1101
#define SYNC_EXT_ABORT_TBL_2 	0b1110
#define PERMISSION_FAULT_PAGE 	0b1111


/**
 * The base adress of the interrupt controller
 */

#define     SVC_EXIT        0x01
#define     SVC_FORK        0x02
#define     SVC_READ        0x03
#define     SVC_WRITE       0x04
#define 	SVC_OPEN 		0x05
#define     SVC_CLOSE       0x06
#define 	SVC_WAITPID		0x07
#define 	SVC_EXECVE 		0x0b
#define 	SVC_CHDIR 		0x0c
#define 	SVC_TIME 		0x0d
#define     SVC_LSEEK       0x13
#define     SVC_FSTAT       0x1c
#define     SVC_SBRK        0x2d
#define 	SVC_IOCTL 		0x36
#define 	SVC_GETCWD 		0xb7
#define 	SVC_GETDENTS 	78

#define RPI_INTERRUPT_CONTROLLER_BASE (PERIPHERALS_BASE + 0xB200UL)

/**
 * The structure of the interrupt controller
 */
typedef struct {
  volatile uint32_t IRQ_basic_pending;
  volatile uint32_t IRQ_pending_1;
  volatile uint32_t IRQ_pending_2;
  volatile uint32_t FIQ_control;
  volatile uint32_t Enable_IRQs_1;
  volatile uint32_t Enable_IRQs_2;
  volatile uint32_t Enable_Basic_IRQs;
  volatile uint32_t Disable_IRQs_1;
  volatile uint32_t Disable_IRQs_2;
  volatile uint32_t Disable_Basic_IRQs;
} rpi_irq_controller_t;


/**
 * Types used for IRQ handling
 */
typedef void interruptFunction (void*);
#define NUMBER_IRQ_INTERRUPTS  64
typedef struct interruptHandler {
    interruptFunction* function;
    void* param;
} interruptHandler;


/**
 * Bits in the Enable_Basic_IRQs register to enable various interrupts.
 * See the BCM2835 ARM Peripherals manual, section 7.5
 */
#define RPI_BASIC_ARM_TIMER_IRQ         (1 << 0)
#define RPI_BASIC_ARM_MAILBOX_IRQ       (1 << 1)
#define RPI_BASIC_ARM_DOORBELL_0_IRQ    (1 << 2)
#define RPI_BASIC_ARM_DOORBELL_1_IRQ    (1 << 3)
#define RPI_BASIC_GPU_0_HALTED_IRQ      (1 << 4)
#define RPI_BASIC_GPU_1_HALTED_IRQ      (1 << 5)
#define RPI_BASIC_ACCESS_ERROR_1_IRQ    (1 << 6)
#define RPI_BASIC_ACCESS_ERROR_0_IRQ    (1 << 7)

/**
 * Return a pointer to the interrupt controller
 */
rpi_irq_controller_t* RPI_GetIRQController(void);


/**
 * Initialize the IRQ interrupt handlers table
 */
void init_irq_interruptHandlers(void);

/**
 * Connect a function to an IRQ interrupt
 */
void connectIRQInterrupt(unsigned int irqID, interruptFunction* function, void* param);

/**
 * Call all handled function that needs to be called
 */
void callInterruptHandlers();

/**
 * enable interrupts
 */
void enable_interrupts(void);

/**
 * disable interrupts
 */
void disable_interrupts(void);

#endif //INTERRUPTS_H
