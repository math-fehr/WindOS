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
	mov 	sp, #0x85000000
	stmfd	sp, {r0-r14}^
	sub 	sp, sp, #15*4
	mrs 	r1, spsr
	stmfd 	sp!, {r1, lr}
	mov 	r0, sp
	and 	r4, sp, #4
	sub 	sp, sp, r4
	dmb
	ldr pc, =undefined_instruction_vector

.globl _software_interrupt_vector
_software_interrupt_vector:
    stmfd	sp, {r0-r14}^
	sub 	sp, sp, #15*4
	mrs 	r1, spsr
	stmfd 	sp!, {r1, lr}
	mov 	r0, sp
	and 	r4, sp, #4
	sub 	sp, sp, r4
	dmb
	ldr 	r1, =software_interrupt_vector
	blx 	r1
	dmb
	add 	sp, sp, r4
	ldmfd 	sp!, {r1, lr}
	msr 	spsr, r1
	ldmfd  	sp, {r0-r14}^ // Restore registers
	add 	sp, sp, #15*4
	movs 	pc, lr

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

    and     r4, sp, #4   //sp must be 8-aligned before a c call
	sub     sp, sp, r4

	ldr 	r1, =data_abort_vector
	blx 	r1

    add     sp, sp, r4

	ldmfd 	sp!, {r1, lr}
	msr 	spsr, r1
	ldmfd  	sp, {r0-r14}^ // Restore registers
	add 	sp, sp, #15*4
	movs 	pc, lr // Context switch

.globl _interrupt_vector
_interrupt_vector:
	dmb
	isb
	dsb
	sub 	lr, lr, #4
  	stmfd	sp, {r0-r14}^
	sub 	sp, sp, #15*4
	mrs 	r1, spsr
	stmfd 	sp!, {r1, lr}
	mov 	r0, sp

  	and     r4, sp, #4   //sp must be 8-aligned before a c call
	sub     sp, sp, r4

	ldr 	r1, =interrupt_vector
	blx 	r1

    add     sp, sp, r4

	ldmfd 	sp!, {r1, lr}
	msr 	spsr, r1
	ldmfd  	sp, {r0-r14}^ // Restore registers
	add 	sp, sp, #15*4
	movs 	pc, lr

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
