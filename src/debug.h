#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "serial.h"


#ifdef RPI2
#define GPIO_LED_PIN 47
#else
#define GPIO_LED_PIN 16
#endif

#define D_KERNEL 0
#define D_SERIAL 1
#define D_WESH   2
#define D_TIMER  3
#define D_IRQ    4
#define D_VFS    5
#define D_EXT2   6
#define D_GPIO   7

void kernel_printf(const char* fmt, ...);
void kdebug(int source, int level, const char* fmt, ...);

int min(int a, int b);
int max(int a, int b);

// computes a^b
int ipow(int a, int b);
#endif
