.section .init
.globl _start

_start:
	ldr pc, _reset_h
	ldr pc, _undefined_instruction_vector_h
	ldr pc, _software_interrupt_vector_h
	ldr pc, _prefetch_abort_vector_h
	ldr pc, _data_abort_vector_h
	ldr pc, _unused_handler_h
	ldr pc, _interrupt_vector_h
	ldr pc, _fast_interrupt_vector_h

_reset_h:                           .word   _reset_
_undefined_instruction_vector_h:    .word   undefined_instruction_vector
_software_interrupt_vector_h:       .word   software_interrupt_vector
_prefetch_abort_vector_h:           .word   prefetch_abort_vector
_data_abort_vector_h:               .word   data_abort_vector
_unused_handler_h:                  .word   _reset_
_interrupt_vector_h:                .word   interrupt_vector
_fast_interrupt_vector_h:           .word   fast_interrupt_vector

_reset_:
	//Setting up interrupt table at adress 0x0000
	ldr     r0, =_start
	mov     r1, #0x0000
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9} // on copie l'IVT
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
	ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9} // on copie les données suivantes
	stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}

	mov sp, #0x8000
	ldr r4, =__bss_start
	ldr r9, =__bss_end
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	b zero_loop_end

zero_loop_beg: // On met à zéro le segment bss
	stmia r4!, {r5-r8}

zero_loop_end:
	cmp r4, r9
	blo zero_loop_beg

	ldr r3, =kernel_main
	blx r3

fin:
	wfe
	b fin
