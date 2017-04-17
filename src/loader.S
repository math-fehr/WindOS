.section .init
.globl _start

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
_undefined_instruction_vector:
	mov sp, #0x1000
	b undefined_instruction_vector

_software_interrupt_vector:
	mov sp, #0x1000
	b software_interrupt_vector

_prefetch_abort_vector:
	mov sp, #0x1000
	b prefetch_abort_vector

_data_abort_vector:
	mov sp, #0x1000
	b data_abort_vector

_interrupt_vector:
	mov sp, #0x1000
	b interrupt_vector

_fast_interrupt_vector:
	mov sp, #0x1000
	b fast_interrupt_vector


_reset_:
	//Setting up interrupt table at adress 0x0000
	ldr     r0, =_start
	mov     r1, #0x0000
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9} // copy the IVT
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}

	ldr sp, =(64*1024*1024) // 64MB of stack
	ldr r4, =__bss_start //	this value is set by gcc
	ldr r9, =__bss_end   //	this value is set by gcc
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	b zero_loop_end


zero_loop_beg: // We set the bss section to zero
	stmia r4!, {r5-r8}


zero_loop_end:
	cmp r4, r9
	blo zero_loop_beg

	ldr r3, =kernel_main //We call kernel_main, the entry of the C kernel
	blx r3


end: //If kernel_main returns, the code here traps the program counter
	wfe
	b end



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
