#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "cstubs.h"
#include "serial.h"

/** \def GPIO_LED_PIN
 * 	The number of the pin holding the raspberry pi ACT LED.
 */
#ifdef RPI2
#define GPIO_LED_PIN 47
#else
#define GPIO_LED_PIN 16
#endif

/**
 * 	The sources of debug log
 */
#define D_KERNEL  0
#define D_SERIAL  1
#define D_WESH    2
#define D_TIMER   3
#define D_IRQ     4
#define D_VFS     5
#define D_EXT2    6
#define D_GPIO    7
#define D_PROCESS 8
#define D_MEMORY  9
#define D_SDCARD  10
#define D_SYSCALL 11
#define D_USPI    12
#define D_MBX     13


void vkernel_printf(const char* fmt, va_list args);
void kernel_printf(const char* fmt, ...);
void kdebug(int source, int level, const char* fmt, ...);

#endif //DEBUG_H
