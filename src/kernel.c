#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "serial.h"
#include "timer.h"
#include "interrupts.h"
#include "gpio.h"

#define GPIO_LED_PIN 47


void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	(void) r0;
	(void) r1;
	(void) atags;

    int* a = malloc(100*sizeof(int));
    a[10] = 4;

	GPIO_setOutputPin(GPIO_LED_PIN);
	while(1) {
		GPIO_setPinValue(GPIO_LED_PIN, true);
		Timer_WaitCycles(500000);
		GPIO_setPinValue(GPIO_LED_PIN, false);
		Timer_WaitCycles(500000);
    }
	enable_interrupts();
	Timer_Setup();
	Timer_SetLoad(200000);
	Timer_Enable();

    serial_init();

    serial_write("Entrez ce que vous voulez:\n");

    char message[256];
    message[255] = '\0';
	while(strcmp(message,"exit")) {
		serial_readline(message, 256);
		serial_write("==> ");
		serial_write(message);
        serial_newline();
	}

    serial_write("Programme fini");
    serial_newline();
    while(1) {
    }
}
