#include "gpio.h"
#include "timer.h"


/**
 * Flags for pins pull
 */
#define NO_PULL 0b00
#define PULL_DOWN 0b01
#define PULL_UP 0b10


extern void dmb();

static volatile rpi_gpio_controller_t* GPIOController =
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
  dmb(); //this ensure that the gpio will recieve data in order
  getGPIOController()->GPFSEL[reg] &= mask;
  getGPIOController()->GPFSEL[reg] |= (function << ofs*3);
  dmb(); //this too
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
  dmb(); //this ensure that the gpio will recieve data in order
  if(value) {
      getGPIOController()->GPSET[set] |= (1 << pin);
  } else {
      getGPIOController()->GPCLR[set] |= (1 << pin);
  }
  dmb(); //this too
}

static void GPIO_setPull(int pin, int pull) {
    dmb();
    getGPIOController()->GPPUD = pull;
    Timer_WaitCycles(150);
    if(pin < 32) {
        getGPIOController()->GPPUDCLK0 |= ((uint32_t)1 << (uint32_t)pin);
        dmb();
        Timer_WaitCycles(150);
        getGPIOController()->GPPUD = 0b11;
        dmb();
        getGPIOController()->GPPUDCLK0 &= ~((uint32_t)1 << (uint32_t)pin);
        dmb();
    }
    else {
        getGPIOController()->GPPUDCLK1 |= ((uint32_t)1 << (uint32_t)(pin-32));
        dmb();
        Timer_WaitCycles(150);
        getGPIOController()->GPPUD = 0b11;
        dmb();
        getGPIOController()->GPPUDCLK1 &= ~((uint32_t)1 << (uint32_t)(pin-32));
        dmb();
    }
}

void GPIO_setPullUp(int pin) {
    GPIO_setPull(pin,PULL_UP);
}

void GPIO_setPullDown(int pin) {
    GPIO_setPull(pin,PULL_DOWN);
}

void GPIO_resetPull(int pin) {
    GPIO_setPull(pin,NO_PULL);
}

void GPIO_enableHighDetect(int pin) {
    dmb();
    int n = pin < 32 ? 0 : 1;
    pin = pin < 32 ? pin : (32-pin);
    getGPIOController()->GPHEN[n] |= (uint32_t)(1) << (uint32_t)(pin);
    dmb();
}

void GPIO_disableHighDetect(int pin) {
    dmb();
    int n = pin < 32 ? 0 : 1;
    pin = pin < 32 ? pin : (32-pin);
    getGPIOController()->GPHEN[n] &= ~((uint32_t)(1) << (uint32_t)(pin));
    dmb();
}
