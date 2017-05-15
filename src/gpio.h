#ifndef GPIO_H
#define GPIO_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "debug.h"
#include "kernel.h"

/**
 * The base adress of the GPIO
 */
#define GPIO_BASE (PERIPHERALS_BASE + 0x200000)

/**
* Flags for pins pull
*/
#define NO_PULL 0b00
#define PULL_DOWN 0b01
#define PULL_UP 0b10

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


/** \struct rpi_gpio_controller_t
 * 	\brief The memory structure of GPIO control registers.
 *  Found in page 89 of BCM2835 ARM Peripherals.
 */
typedef struct {
  volatile uint32_t GPFSEL[6]; 	//!< Function Select
  volatile uint32_t GP_Reserved_0;
  volatile uint32_t GPSET[2];	//!< Pin output set
  volatile uint32_t GP_Reserved_1;
  volatile uint32_t GPCLR[2];	//!< Pin output clear
  volatile uint32_t GP_Reserved_2;
  volatile uint32_t GPLEV[2];	//!< Pin level
  volatile uint32_t GP_Reserved_3;
  volatile uint32_t GPEDS[2];	//!< Pin event detect status
  volatile uint32_t GP_Reserved_4;
  volatile uint32_t GPREN[2];	//!< Pin rising edge detect enable
  volatile uint32_t GP_Reserved_5;
  volatile uint32_t GPFEN[2];	//!< Pin falling edge detect enable
  volatile uint32_t GP_Reserved_6;
  volatile uint32_t GPHEN[2];	//!< Pin high detect enable
  volatile uint32_t GP_Reserved_7;
  volatile uint32_t GPLEN[2];	//!< Pin low detect enable
  volatile uint32_t GP_Reserved_8;
  volatile uint32_t GPAREN[2];	//!< Pin async rising edge detect
  volatile uint32_t GP_Reserved_9;
  volatile uint32_t GPAFEN[2]; 	//!< Pin async falling edge detect
  volatile uint32_t GP_Reserved_10;
  volatile uint32_t GPPUD;		//!< Pin pull-up/down enable
  volatile uint32_t GPPUDCLK0;	//!< Pin pull-up/down enable clock 0
  volatile uint32_t GPPUDCLK1;	//!< Pin pull-up/down enable clock 1
} rpi_gpio_controller_t;


volatile rpi_gpio_controller_t* getGPIOController(void);
void GPIO_setPinFunction 	(int pin, int function);
void GPIO_setOutputPin 		(int pin);
void GPIO_setInputPin 		(int pin);
void GPIO_setPinValue 		(int pin, bool value);
void GPIO_setPullUp 		(int pin);
void GPIO_setPullDown 		(int pin);
void GPIO_resetPull 		(int pin);
void GPIO_enableHighDetect 	(int pin);
void GPIO_disableHighDetect (int pin);
bool GPIO_getPinValue 		(int pin);
bool GPIO_hasDetectedEvent 	(int pin);


#endif
