#ifndef STORAGE_DRIVER_H
#define STORAGE_DRIVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  // (address, buffer, size)
  int (*read)   (uint32_t, void *, uint32_t);
  int (*write)  (uint32_t, void *, uint32_t);
} storage_driver;


#endif //STORAGE_DRIVER_H
