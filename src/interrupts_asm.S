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
    stmfd   sp!, {r2-r12, lr} // save user registers
	stmfd	sp!, {ip}
	mrs     ip, spsr

	stmfd 	sp!, {ip}
    ldr     ip, =interrupt_reg

	msr 	cpsr_c, #0x9F
	stmia 	ip, {r0-r15}
	msr 	cpsr_c, #0x93

	ldmfd  	sp!, {r0}
	ldmfd 	sp!, {r1}
	str 	r0, [ip, #16*4]
	str 	lr, [ip, #15*4]
	str 	r1, [ip, #12*4]
    bl      software_interrupt_vector // returns in r0
	mov 	r1, r0 // a non documented feature places cpsr in r0 while going back to user mode, whatever let's use r1
	ldmfd 	sp!, {r2-r12, pc}^


.globl _prefetch_abort_vector
_prefetch_abort_vector:
    ldr     ip, =interrupt_reg
    stmia   ip, {r0-r15}
    mrs     r0, spsr
    str     r0, [ip, #16*4]
	ldr     pc, =prefetch_abort_vector

.globl _data_abort_vector
_data_abort_vector:
    ldr     ip, =interrupt_reg
    stmia   ip, {r0-r15}
    mrs     r0, spsr
    str     r0, [ip, #16*4]
	ldr     pc, =data_abort_vector

.globl _interrupt_vector
_interrupt_vector:
	sub 	lr, lr, #4
	msr 	cpsr_c, #0x9F // Switching to SYS mode to save SP and LR
	stmfd 	sp,	{r0-r15}
	mov 	r0, sp
	msr 	cpsr_c, #0x92 // Back to IRQ mode
	mrs 	r1, spsr // Get program cpsr
	str 	lr, [r0, #-4] // Save program PC
	sub 	r0, r0, #4*16
	stmfd 	r0!, {r1} // Save program cpsr
	ldr pc, =interrupt_vector

.globl _fast_interrupt_vector
_fast_interrupt_vector:
    ldr     ip, =interrupt_reg
    stmia   ip!, {r0-r15}
    mrs     r0, spsr
    str     r0, [ip, #16*4]
	ldr pc, =fast_interrupt_vector


//  From https://github.com/xinu-os/xinu/blob/master/system/arch/arm/memory_barrier.S
// When accessing different peripherals, data can arrive out of order.
// data-memory barrier ensure all data has been transferred before moving on
/* From BCM 2835 manual page 7:
You should place:
•	A memory write barrier before the first write to a peripheral.
•	A memory read barrier after the last read of a peripheral
*/
.globl dmb
dmb:
	mov	r12, #0
	mcr	p15, 0, r12, c7, c10, 5
	mov 	pc, lr
