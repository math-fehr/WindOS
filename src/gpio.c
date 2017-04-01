#include "gpio.h"


static rpi_gpio_controller_t* GPIOController =
  (rpi_gpio_controller_t*) GPIO_BASE;

rpi_gpio_controller_t* getGPIOController(void) {
  return GPIOController;
}

void setPinFunction(uint8_t pin, uint8_t function) {
  uint8_t reg = pin/10;
  uint8_t ofs = pin%10;

  uint32_t mask = ~(0b111 << (ofs*3));
  function &= 0b111;

  getGPIOController()->GPFSEL[reg] &= mask;
  getGPIOController()->GPFSEL[reg] |= (function << ofs*3);
}

void setOutputPin(uint8_t pin) {
  setPinFunction(pin, PIN_OUTPUT);
}

void setPinValue(uint8_t pin, bool value) {
  uint8_t set = pin >= 32;
  pin &= 32;

  if(value) {
    getGPIOController()->GPSET[set] |= 1 << pin;
  } else {
    getGPIOController()->GPCLR[set] |= 1 << pin;
  }
}
