.section .init
.globl _start

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
  /**
   * redirect to functions that handle interrupts
   */
_start:
	ldr pc, _reset_h
	ldr pc, _undefined_instruction_vector_h
	ldr pc, _software_interrupt_vector_h
	ldr pc, _prefetch_abort_vector_h
	ldr pc, _data_abort_vector_h
	ldr pc, _unused_handler_h
	ldr pc, _interrupt_vector_h
	ldr pc, _fast_interrupt_vector_h

  /**
   * Adress of functions that handle interrupts
   */
_reset_h:                           .word   _reset_
_undefined_instruction_vector_h:    .word   _undefined_instruction_vector
_software_interrupt_vector_h:       .word   _software_interrupt_vector
_prefetch_abort_vector_h:           .word   _prefetch_abort_vector
_data_abort_vector_h:               .word   _data_abort_vector
_unused_handler_h:                  .word   _reset_
_interrupt_vector_h:                .word   _interrupt_vector
_fast_interrupt_vector_h:           .word   _fast_interrupt_vector


  /**
   * Theses functions call the C functions
   */

_reset_:
    #ifdef RPI2
    mrs     r0, cpsr
    // The first thing is to make sure we boot in the right mode,
    // Rpi2 boots in hypervisor mode so we have to go back to svr mode.
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     spsr_cxsf,r0
    add     r0,pc,#4
    msr     ELR_hyp,r0
    eret
    #endif
	//Setting up interrupt table at physical address 0
	ldr     r0, =_start
	mov     r1, #0x0000
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}


	ldr sp, =_start // a temporary stack
	bl init_get_ram

	mov r7, r0
	mov r3, r0
	sub sp, r3, #0x8000 //put sp at the end of the ram


	mov 	r0, #(CPSR_MODE_IRQ | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
	msr 	cpsr_c, r0
	sub 	sp, r3, #0x1000
	add 	sp, sp, #0x80000000

	mov 	r0, #(CPSR_MODE_ABORT | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
 	msr 	cpsr_c, r0
	mov 	sp, r3
	add 	sp, sp, #0x80000000

	mov 	r0, #(CPSR_MODE_SVR | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
	msr 	cpsr_c, r0

    bl sys_init

	mov r0, r7
	add sp, sp, #0x80000000 // new sp pointing to virtual address

    //We call kernel_main, the entry of the C kernel
	bl kernel_main

end: //If kernel_main returns, the code here traps the program counter
	wfe
	b end
