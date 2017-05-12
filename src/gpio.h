#ifndef GPIO_H
#define GPIO_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "debug.h"
#include "kernel.h"

/**
 * The base adress of the GPIO

#ifdef RPI2
#define GPIO_BASE (0x3F200000UL + 0x80000000UL)
#else
#define GPIO_BASE (0x20200000UL + 0x80000000UL)
#endif
*/
#define GPIO_BASE (PERIPHERALS_BASE + 0x200000)

/**
 * The flags identifying the functions of the pins
 */
#define PIN_OUTPUT  0b001
#define PIN_INPUT   0b000
#define PIN_FUN0    0b100
#define PIN_FUN1    0b101
#define PIN_FUN2    0b110
#define PIN_FUN3    0b111
#define PIN_FUN4    0b011
#define PIN_FUN5    0b010


/**
 * The structure of the GPIO in memory
 */
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


/**
 * Return a pointer to the GPIO
 */
volatile rpi_gpio_controller_t* getGPIOController(void);


/**
 * Set a function to a pin
 */
void GPIO_setPinFunction(int pin, int function) ;


/**
 * Set a pin in output mode
 */
void GPIO_setOutputPin(int pin);

/**
 * Set a pin in input mode
 */
void GPIO_setInputPin(int pin);

/**
 * Set a value to a pin
 * true for activee and false for inactive
 */
void GPIO_setPinValue(int pin, bool value);


/**
 * Pull Up the resistor on pin
 */
void GPIO_setPullUp(int pin);


/**
 * Pull Down the resistor on pin
 */
void GPIO_setPullDown(int pin);


/**
 * Reset the pull of the resistor on pin
 */
void GPIO_resetPull(int pin);


/**
 * A high level on the pin set a bit in GPED
 */
void GPIO_enableHighDetect(int pin);


/**
 * A high level on the pin do not set a bit in GPED anymore
 */
void GPIO_disableHighDetect(int pin);


/**
 * Return the actual value of the pin
 * true is for high, false is for low
 */
bool GPIO_getPinValue(int pin);


/**
 * Return true if an edge has been detected in the pin
 * Note that the value has to be reset
 */
bool GPIO_hasDetectedEvent(int pin);


#endif
