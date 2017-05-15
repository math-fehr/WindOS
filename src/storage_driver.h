#ifndef STORAGE_DRIVER_H
#define STORAGE_DRIVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** \struct storage_driver
 *	\brief Functions implemented by any mass-storage driver.
 */
typedef struct {
  int (*read)   (uint32_t, void *, uint32_t); 	///< Read from the device (address, buffer, size), returns the number of byte read.
  int (*write)  (uint32_t, void *, uint32_t);	///< Write to the device (address, buffer, size), returns the number of byte written.
} storage_driver;


#endif //STORAGE_DRIVER_H
