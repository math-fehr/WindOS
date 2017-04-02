#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "serial.h"
#include "timer.h"
#include "interrupts.h"

#define GPIO_LED_PIN 16


void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	(void) r0;
	(void) r1;
	(void) atags;

	serial_init();

	enable_interrupts();
	Timer_Setup();
	Timer_SetLoad(200000);
	Timer_Enable();
	Timer_Enable_Interrupts();

	while(1) {
		serial_write("Boucle principale!\n");
		Timer_WaitMicroSeconds(2000000);
	}
}
