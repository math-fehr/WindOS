#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "global.h"

#ifdef RASPI2
#define GPIO_BASE 0x3F200000
#else
#define GPIO_BASE 0x20200000
#endif

#define PIN_OUTPUT  0b001
#define PIN_INPUT   0b000
#define PIN_FUN0    0b100
#define PIN_FUN1    0b101
#define PIN_FUN2    0b110
#define PIN_FUN3    0b111
#define PIN_FUN4    0b011
#define PIN_FUN5    0b010


typedef struct {
  volatile uint32_t GPFSEL[6];
  volatile uint32_t GP_Reserved_0;
  volatile uint32_t GPSET[2];
  volatile uint32_t GP_Reserved_1;
  volatile uint32_t GPCLR[2];
  volatile uint32_t GP_Reserved_2;
  volatile uint32_t GPLEV[2];
  volatile uint32_t GP_Reserved_3;
  volatile uint32_t GPEDS[2];
  volatile uint32_t GP_Reserved_4;
  volatile uint32_t GPREN[2];
  volatile uint32_t GP_Reserved_5;
  volatile uint32_t GPFEN[2];
  volatile uint32_t GP_Reserved_6;
  volatile uint32_t GPHEN[2];
  volatile uint32_t GP_Reserved_7;
  volatile uint32_t GPLEN[2];
  volatile uint32_t GP_Reserved_8;
  volatile uint32_t GPAREN[2];
  volatile uint32_t GP_Reserved_9;
  volatile uint32_t GPAFEN[2];
  volatile uint32_t GP_Reserved_10;
  volatile uint32_t GPPUD;
  volatile uint32_t GPPUDCLK0;
  volatile uint32_t GPPUDCLK1;
} rpi_gpio_controller_t;


rpi_gpio_controller_t* getGPIOController(void);
void setPinFunction(uint8_t pin, uint8_t function) ;
void setOutputPin(uint8_t pin);
void setPinValue(uint8_t pin, bool value);

#endif
