#include "gpio.h"


static rpi_gpio_controller_t* GPIOController =
  (rpi_gpio_controller_t*) GPIO_BASE;

rpi_gpio_controller_t* getGPIOController(void) {
  return GPIOController;
}

void GPIO_setPinFunction(int pin, int function) {
  int reg = pin/10;
  int ofs = pin%10;

  int mask = ~(0b111 << (ofs*3));
  function &= 0b111;

  getGPIOController()->GPFSEL[reg] &= mask;
  getGPIOController()->GPFSEL[reg] |= (function << ofs*3);
}

void GPIO_setOutputPin(int pin) {
  GPIO_setPinFunction(pin, PIN_OUTPUT);
}

void GPIO_setPinValue(int pin, bool value) {
  int set = pin >= 32;
  pin &= 31;

  if(value) {
    getGPIOController()->GPSET[set] |= 1 << pin;
  } else {
    getGPIOController()->GPCLR[set] |= 1 << pin;
  }
}
