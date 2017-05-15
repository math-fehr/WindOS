#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>

#include "kernel.h"
#include "mmu.h"
#include "debug.h"
#include "scheduler.h"
#include "syscalls.h"
#include "memalloc.h"
#include "arm.h"

/**
 *	List of syscall indices.
 */
#define     SVC_EXIT        0x01
#define     SVC_FORK        0x02
#define     SVC_READ        0x03
#define     SVC_WRITE       0x04
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
#define 	SVC_GETDENTS 	0x4e
#define 	SVC_OPENAT 		0x127
#define 	SVC_MKNODAT 	0x129
#define 	SVC_UNLINKAT	0x12d

/** \def RPI_INTERRUPT_CONTROLLER_BASE
 * 	The base adress of the interrupt controller.
 */
#define RPI_INTERRUPT_CONTROLLER_BASE (PERIPHERALS_BASE + 0xB200UL)

/**	\struct rpi_irq_controller_t
 *	\brief Memory representation of ARM IRQ controller.
 */
typedef struct {
  volatile uint32_t IRQ_basic_pending; ///< A bit is set there when a basic IRQ is pending.
  volatile uint32_t IRQ_pending_1; ///< A bit is set when a general IRQ between 0 and 31 is pending.
  volatile uint32_t IRQ_pending_2; ///< A bit is set when a general IRQ between 32 and 63 is pending.
  volatile uint32_t FIQ_control; ///< Enable Fast Interrupt Mode for an interrupt.
  volatile uint32_t Enable_IRQs_1; ///< General IRQ enable bit 0 to 31.
  volatile uint32_t Enable_IRQs_2; ///< General IRQ enable bit 32 to 63.
  volatile uint32_t Enable_Basic_IRQs; ///< Basic IRQ enable bit.
  volatile uint32_t Disable_IRQs_1; ///< General IRQ disable bit 0 to 31.
  volatile uint32_t Disable_IRQs_2; ///< General IRQ disable bit 32 to 63.
  volatile uint32_t Disable_Basic_IRQs; ///< Basic IRQ disable bit.
} rpi_irq_controller_t;


/**
 * Types used for IRQ handling
 */

/** \var void interruptFunction (void*)
 *	\brief Type representing a function.
 */
typedef void interruptFunction (void*);

/** \def NUMBER_IRQ_INTERRUPTS 64
 * 	\brief Prints maximum index of a general interrupt.
 */
#define NUMBER_IRQ_INTERRUPTS  64

/** \struct interruptHandler
 * 	\brief Represents an interrupt handler.
 */
typedef struct interruptHandler {
    interruptFunction* function; ///< The function to call.
    void* param; ///< A pointer to the function parameter.
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


rpi_irq_controller_t* RPI_GetIRQController(void);
void init_irq_interruptHandlers(void);
void connectIRQInterrupt(unsigned int irqID, interruptFunction* function, void* param);
void callInterruptHandlers();
void enable_interrupts(void);
void disable_interrupts(void);

#endif //INTERRUPTS_H
