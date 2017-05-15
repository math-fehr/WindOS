#include "gpio.h"
#include "timer.h"

/** \file gpio.c
 * 	\brief Driver to communicate with the GPIO hardware.
 * 	The raspberry pi is shipped with a GPIO (General Purpose Input/Output).
 * 	These functions allows us to exploit some of these feature.
 */

// The data memory barrier instruction, useful to ensure synchronisation.
extern void dmb();

/** \var volatile rpi_gpio_controller_t* GPIOController
  *	Structure of the GPIO controller registers.
  */
volatile rpi_gpio_controller_t* GPIOController =
  (rpi_gpio_controller_t*) GPIO_BASE;

/** \fn volatile rpi_gpio_controller_t*t getGPIOController (void)
 * 	\brief A getter function for the GPIO controller.
 * 	\return A pointer to the previously defined structure.
 */
volatile rpi_gpio_controller_t* getGPIOController (void) {
    return GPIOController;
}

/** \fn void GPIO_setPinFunction (int pin, int function)
 *	\brief Sets the function of a specified pin.
 *	\param pin The pin to configure.
 * 	\param function The function to set (as defined in gpio.h).
 */
void GPIO_setPinFunction (int pin, int function) {
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

/** \fn void GPIO_setOutputPin (int pin)
 *	\brief Sets a pin to the output mode.
 * 	\param pin The updated pin.
 */
void GPIO_setOutputPin (int pin) {
  GPIO_setPinFunction(pin, PIN_OUTPUT);
}

/** \fn void GPIO_setInputPin (int pin)
 *	\brief Sets a pin to the input mode.
 * 	\param pin The updated pin.
 */
void GPIO_setInputPin (int pin) {
    GPIO_setPinFunction(pin, PIN_INPUT);
}

/** \fn void GPIO_setPinValue (int pin, bool value)
 *	\brief Sets the value of a pin.
 * 	\param pin The updated pin.
 * 	\param value The pin value.
 * 	\warning The pin should be in output mode.
 * 	\warning Between the raspberry pi 1 and 2 the value yields opposite result for the LED.
 */
void GPIO_setPinValue (int pin, bool value) {
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

/** \fn void GPIO_setPull (int pin, bool pull)
 *	\brief Sets the resistor pull mode of a pin.
 * 	\param pin The updated pin.
 * 	\param pull The pull mode as defined in gpio.h.
 */
static void GPIO_setPull (int pin, int pull) {
    dmb();
    getGPIOController()->GPPUD = pull;
    Timer_WaitCycles(300);
    if(pin < 32) {
        getGPIOController()->GPPUDCLK0 |= ((uint32_t)1 << (uint32_t)pin);
        dmb();
        Timer_WaitCycles(300);
        getGPIOController()->GPPUD = 0b11;
        dmb();
        getGPIOController()->GPPUDCLK0 &= ~((uint32_t)1 << (uint32_t)pin);
        dmb();
    }
    else {
        getGPIOController()->GPPUDCLK1 |= ((uint32_t)1 << (uint32_t)(pin-32));
        dmb();
        Timer_WaitCycles(300);
        getGPIOController()->GPPUD = 0b11;
        dmb();
        getGPIOController()->GPPUDCLK1 &= ~((uint32_t)1 << (uint32_t)(pin-32));
        dmb();
    }
}

/** \fn void GPIO_setPullUp(int pin)
 *	\brief Resistor pull up on specified pin
 * 	\param pin The updated pin.
 */
void GPIO_setPullUp(int pin) {
    GPIO_setPull(pin,PULL_UP);
}

/** \fn void GPIO_setPullDown(int pin)
 *	\brief Resistor pull down on specified pin
 * 	\param pin The updated pin.
 */
void GPIO_setPullDown(int pin) {
    GPIO_setPull(pin,PULL_DOWN);
}

/** \fn void GPIO_resetPull(int pin)
 *	\brief Resistor pull reset on specified pin
 * 	\param pin The updated pin.
 */
void GPIO_resetPull(int pin) {
    GPIO_setPull(pin,NO_PULL);
}

/** \fn void GPIO_enableHighDetect(int pin)
 *	\brief Enable the high detection feature on this pin.
 * 	\param pin The updated pin.
 * 	High detect set a bit in GPED when a high level on pin is detected.
 */
void GPIO_enableHighDetect(int pin) {
    dmb();
    int n = pin < 32 ? 0 : 1;
    pin = pin < 32 ? pin : (32-pin);
    getGPIOController()->GPHEN[n] |= (uint32_t)(1) << (uint32_t)(pin);
    dmb();
}

/** \fn void GPIO_disableHighDetect(int pin)
 *	\brief Disable the high detection feature on this pin.
 * 	\param pin The updated pin.
 */
void GPIO_disableHighDetect(int pin) {
    dmb();
    int n = pin < 32 ? 0 : 1;
    pin = pin < 32 ? pin : (32-pin);
    getGPIOController()->GPHEN[n] &= ~((uint32_t)(1) << (uint32_t)(pin));
    dmb();
}

/** \fn void GPIO_getPinValue(int pin)
 *	\brief Reads the value of the pin.
 * 	\param pin The updated pin.
 * 	\return true if the pin is high, false if it's low.
 *	\warning The pin must be in input mode.
 */
bool GPIO_getPinValue(int pin) {
    dmb();
    int n = pin < 32 ? 0 : 1;
    pin = pin < 32 ? pin : (32-pin);
    uint32_t value = getGPIOController()->GPLEV[n] & (uint32_t)(1) << (uint32_t)(pin);
    return value != (uint32_t)0;
}

/** \fn void GPIO_hasDetectedEvent(int pin)
 *	\brief Check if an edge has been detected on this pin.
 * 	\param pin The check pin.
 * 	\return true if an edge has been detected.
 *	\warning The value is reset as soon a this call is done.
 */
bool GPIO_hasDetectedEvent(int pin) {
    dmb();
    int n = pin < 32 ? 0 : 1;
    pin = pin < 32 ? pin : (32-pin);
    uint32_t value = getGPIOController()->GPEDS[n] & (uint32_t)(1) << (uint32_t)(pin);
    getGPIOController()->GPEDS[n] |= (uint32_t)(1) << (uint32_t)(pin);
    dmb();
    return value != (uint32_t)0;
}
