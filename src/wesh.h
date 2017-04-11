#ifndef WESH_H
#define WESH_H

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
#include "debug.h"
#include "ext2.h"
#include "vfs.h" 
#include "storage_driver.h"
#include "process.h"
#include "scheduler.h"


void wesh();

#endif
