#include "gpio.h"

extern void dmb();

static rpi_gpio_controller_t* GPIOController =
  (rpi_gpio_controller_t*) GPIO_BASE;
 
rpi_gpio_controller_t* getGPIOController(void) {
    return GPIOController;
}

void GPIO_setPinFunction(int pin, int function) {
  kdebug(D_GPIO, 1, "Set pin %d to function %d\n", pin, function);
  int reg = pin/10;
  int ofs = pin%10;

  int mask = ~(0b111 << (ofs*3));
  function &= 0b111;
  dmb();
  getGPIOController()->GPFSEL[reg] &= mask;
  getGPIOController()->GPFSEL[reg] |= (function << ofs*3);
  dmb();
}

void GPIO_setOutputPin(int pin) {
  GPIO_setPinFunction(pin, PIN_OUTPUT);
}

void GPIO_setPinValue(int pin, bool value) {
  kdebug(D_GPIO, 1, "Set pin %d to value %d\n", pin, value);
  int set = 0;
  if (pin >= 32) {
    set = 1;
  }
  pin &= 31;
  dmb();
  if(value) {
      getGPIOController()->GPSET[set] |= (1 << pin);
  } else {
      getGPIOController()->GPCLR[set] |= (1 << pin);
  }
  dmb();
}
