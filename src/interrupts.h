#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef RASPI2
#define RPI_INTERRUPT_CONTROLLER_BASE 0x3F00B200
#else
#define RPI_INTERRUPT_CONTROLLER_BASE 0x2000B200
#endif

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

/** @brief Bits in the Enable_Basic_IRQs register to enable various interrupts.
        See the BCM2835 ARM Peripherals manual, section 7.5 */
#define RPI_BASIC_ARM_TIMER_IRQ         (1 << 0)
#define RPI_BASIC_ARM_MAILBOX_IRQ       (1 << 1)
#define RPI_BASIC_ARM_DOORBELL_0_IRQ    (1 << 2)
#define RPI_BASIC_ARM_DOORBELL_1_IRQ    (1 << 3)
#define RPI_BASIC_GPU_0_HALTED_IRQ      (1 << 4)
#define RPI_BASIC_GPU_1_HALTED_IRQ      (1 << 5)
#define RPI_BASIC_ACCESS_ERROR_1_IRQ    (1 << 6)
#define RPI_BASIC_ACCESS_ERROR_0_IRQ    (1 << 7)


#endif
