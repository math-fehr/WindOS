#ifndef GLOBAL_H
#define GLOBAL_H

//#define RASPI2

#ifdef RASPI2
  #define PERIPHERALS_BASE 0x3F000000
#else
  #define PERIPHERALS_BASE 0x20000000
#endif

#endif
