// See ARM section A2.2 (Processor Modes)

.equ    CPSR_MODE_USER,         0x10
.equ    CPSR_MODE_FIQ,          0x11
.equ    CPSR_MODE_IRQ,          0x12
.equ    CPSR_MODE_SVR,          0x13
.equ    CPSR_MODE_ABORT,        0x17
.equ    CPSR_MODE_UNDEFINED,    0x1B
.equ    CPSR_MODE_SYSTEM,       0x1F

// See ARM section A2.5 (Program status registers)
.equ    CPSR_IRQ_INHIBIT,       0x80
.equ    CPSR_FIQ_INHIBIT,       0x40


.globl _undefined_instruction_vector // TODO: put some stack for this handler
_undefined_instruction_vector:
    b _undefined_instruction_vector
	ldr pc, =undefined_instruction_vector

.globl _software_interrupt_vector
_software_interrupt_vector:
	stmfd	sp, {r0-r14}^ // Save caller registers.
	sub 	sp, sp, #15*4
	mrs 	r1, spsr  // Get caller CPSR
	stmfd 	sp!, {r1, lr} // Save CPSR and LR.

	mov 	r0, sp
	ldr 	r1, =software_interrupt_vector
	blx		r1

	ldmfd 	sp!, {r1, lr}
	msr 	spsr, r1
	ldmfd  	sp, {r0-r14}^ // Restore registers
	add 	sp, sp, #15*4
	movs 	pc, lr // Context switch

.globl _prefetch_abort_vector
_prefetch_abort_vector:
	stmfd	sp, {r0-r14}^
	sub 	sp, sp, #15*4
	mrs 	r1, spsr
	stmfd 	sp!, {r1, lr}
	mov 	r0, sp
	ldr     pc, =prefetch_abort_vector

.globl _data_abort_vector
_data_abort_vector:
	stmfd	sp, {r0-r14}^
	sub 	sp, sp, #15*4
	mrs 	r1, spsr
	stmfd 	sp!, {r1, lr}
	mov 	r0, sp
	ldr     pc, =data_abort_vector

.globl _interrupt_vector
_interrupt_vector:
	stmfd	sp, {r0-r14}^ // Save caller registers.
	sub 	sp, sp, #15*4
	mrs 	r1, spsr_all  // Get caller CPSR
	subs 	lr, lr, #4
	stmfd 	sp!, {r1, lr} // Save CPSR and LR.
	mov 	r0, sp

	ldr 	r1, =interrupt_vector

	blx		r1
	ldmfd 	sp!, {r1, lr}
	msr 	spsr_all, r1
	ldmfd  	sp, {r0-r14}^ // Restore registers
	add 	sp, sp, #15*4
	movs 	pc, lr // Context switch

.globl _fast_interrupt_vector
_fast_interrupt_vector:
	ldr pc, =fast_interrupt_vector


//  From https://github.com/xinu-os/xinu/blob/master/system/arch/arm/memory_barrier.S
// When accessing different peripherals, data can arrive out of order.
// data-memory barrier ensure all data has been transferred before moving on
/* From BCM 2835 manual page 7:
You should place:
•	A memory write barrier before the first write to a peripheral.
•	A memory read barrier after the last read of a peripheral
*/
/*.globl dmb
dmb:
	mov	r12, #0
	mcr	p15, 0, r12, c7, c10, 5
	mov 	pc, lr*/