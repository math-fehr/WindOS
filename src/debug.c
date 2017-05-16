/** \file debug.c
  * \brief Debug features.
  * Methods to debug information on the serial console.
  * Fine tuning by associating values to each debug and global thresholds.
  */

#include "debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "serial.h"
/** \def N_SOURCES
 * 	\brief Defines the number of debug input.
 */
#define N_SOURCES 14

/** \var const char sources[N_SOURCES][9]
 * 	\brief Printed names of the different debug sources.
 */
const char sources[N_SOURCES][9]= { "KERNEL", "SERIAL", "WESH",
                                    "TIMER" , "IRQ"   , "VFS" ,
                                    "EXT2"  , "GPIO"  , "PROCESS",
                                    "MEMORY", "SDCARD", "SYSCALL",
                                    "USPI"  , "MBX"};

/**	\var const int enable_source[N_SOUCES]
 * 	\brief Fixed thresholds for debug output.
 * 	On a scale from zero to ten (zero showing all errors):
 * 	- 0-1: high throughput info, low-level debugs
 * 	- 2: Feature call description.
 * 	- 3-5: Feature call debug.
 * 	- 6-7: Feature call warning (user error).
 * 	- 8-9: Feature call error.
 * 	- 10: Critical error, probably leading to a kernel crash.
 */
const int enable_source[N_SOURCES] = { 8,8,8,
                                       8,8,8,
									   8,8,8,
									   8,8,8,
                                       8,8};

/** \fn kernel_printf(const char* fmt, ...)
 *	\brief Same behaviour as printf, on the serial port.
 * 	\param const char* fmt Format string
 * 	\param ... Optional parameters feeding the string.
 */
void kernel_printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int size = vsnprintf(NULL, 0, fmt, args);
  char* buffer = (char*) malloc(size+1);
  vsnprintf(buffer, size+1, fmt, args);
  va_end(args);
  buffer[size] = 0;
  serial_write(buffer);
  free(buffer);
}


/** \fn vkernel_printf(const char* fmt, va_list args)
 *	\brief Acts the same as kernel_printf, but when the va_list is already set up.
 * 	\param fmt Format string
 * 	\param args Optional parameters feeding the string.
 */
void vkernel_printf(const char* fmt, va_list args) {
  int size = vsnprintf(NULL, 0, fmt, args);
  char* buffer = (char*) malloc(size+1);
  vsnprintf(buffer, size+1, fmt, args);
  buffer[size] = 0;
  serial_write(buffer);
  free(buffer);
}

/** \fn kdebug(int from, int level, const char* fmt, ...)
 *	\brief Print data on the serial port if the level is higher than the fixed
 * 	threshold.
 *	\param from Debug source
 *	\param level Debug severity
 * 	\param const char* fmt Format string
 * 	\param ... Optional parameters feeding the string.
 */
void kdebug(int from, int level, const char* fmt, ...) {
  if (from < 0 || from >= N_SOURCES) return;
  if (enable_source[from] > level) return;
  kernel_printf("[%s][%d] ", sources[from], level);

  va_list args;
  va_start(args, fmt);
  vkernel_printf(fmt, args);
  va_end(args);
}
