#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "serial.h"
#include "timer.h"
#include "interrupts.h"
#include "gpio.h"

#define GPIO_LED_PIN 16


void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	(void) r0;
	(void) r1;
	(void) atags;

	GPIO_setOutputPin(GPIO_LED_PIN);

	while(1) {
		GPIO_setPinValue(GPIO_LED_PIN, true);
		Timer_WaitMicroSeconds(500000);
		GPIO_setPinValue(GPIO_LED_PIN, false);
		Timer_WaitMicroSeconds(500000);
	}

	serial_init();

	enable_interrupts();
	Timer_Setup();
	Timer_SetLoad(200000);
	Timer_Enable();

	serial_write("Entrez ce que vous voulez:\n");

	char message[256];
	while(1) {
		serial_readline(message, 256);
		serial_write("==> ");
		serial_write(message);
		serial_putc('\n');
	}
}
