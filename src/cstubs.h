#ifndef CSTUBS_H
#define CSTUBS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include "memalloc.h"
#include "mmu.h"

int min(int const a, int const b);
int max(int const a, int const b);
int ipow(int a, int b);

#endif  //CSTUBS_H
