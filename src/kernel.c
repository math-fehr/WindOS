#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "serial.h"
#include "timer.h"

#define GPIO_LED_PIN 16


void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	(void) r0;
	(void) r1;
	(void) atags;

	serial_init();

	volatile uint32_t count = 0;
	while(1) {
		Timer_WaitMicroSeconds(500); // Ça marche pas jsp pourquoi
		serial_write('a');
		Timer_WaitMicroSeconds(500);
		count++;
	}
}
