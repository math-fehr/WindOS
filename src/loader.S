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
    //TEST

	mov 		r0, #(CPSR_MODE_SVR | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
	msr 		cpsr_c, r0

	//Setting up interrupt table at physical address 0
	ldr     r0, =_start
	mov     r1, #0x0000
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}

	// change base address for interrupts
	mov 		r0, #0xf0000000
	mcr 		p15, 0, r0, c12, c0, 0



	ldr sp, =_start // a temporary stack
	ldr r3, =init_get_ram
	blx r3
	mov r3, r0
	sub sp, r3, #0x8000 //put sp at the end of the ram

	mov 		r0, #(CPSR_MODE_IRQ | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
	msr 		cpsr_c, r0
	sub 		sp, r3, #0x1000
	add 		sp, sp, #0x80000000

	mov 		r0, #(CPSR_MODE_ABORT | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
 	msr 		cpsr_c, r0
	mov 		sp, r3
	add 		sp, sp, #0x80000000

	mov 		r0, #(CPSR_MODE_SVR | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
	msr 		cpsr_c, r0

	// builds a correct ttb at address 0x4000
	ldr r3, =init_setup_ttb
	blx r3


	mov r0, #0x4000
	mov r1, #1
	// starts the mmu.
	mov r2,#0

    #ifdef RPI2
  	  mcr p15,0,r2,c8,c7,0  // Invalidate tlb
	  mcr p15,0,r2,c7,c5,6  // BPIALL - Invalidate entire branch predictor array
    #else
      mcr p15,0,r2,c7,c7,0  // invalidate caches
      mcr p15,0,r2,c8,c7,0  // invalidate tlb
	  mcr p15,0,r2,c7,c10,4 // DSB ??
    #endif

	mvn r2,#0
	bic r2,#0xC

    mcr p15,0,r2,c3,c0,0  // domain

	mcr p15,0,r0,c2,c0,0  // tlb base
	mcr p15,0,r0,c2,c0,1  // tlb base

	mrc p15,0,r2,c1,c0,0
	orr r2,r2,r1
	mcr p15,0,r2,c1,c0,0

	// hopefully the mmu is enabled.
	add sp, sp, #0x80000000 // new sp pointing to virtual address

	ldr r3, =kernel_main //We call kernel_main, the entry of the C kernel
	blx r3


test:
  	ldr r0,=0x3f200000         // Load the GPIO address into r0.
	mov r1,#1
	lsl r1,#21
	str r1,[r0,#16]
	mov r1,#1
	lsl r1,#15
	str r1,[r0,#32]
	loop:
	b loop

end: //If kernel_main returns, the code here traps the program counter
	wfe
	b end
