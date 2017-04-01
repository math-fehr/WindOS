#include "serial.h"

static rpi_uart_controller_t* UARTController =
  (rpi_uart_controller_t*) UART0_BASE;

static rpi_uart_controller_t* getUARTController() {
  return UARTController;
}

void serial_init() {
	// disable uart
  getUARTController()->CR = 0;
  getGPIOController()->GPPUD = 0;

  Timer_WaitCycles(150);

  // Disable pull up/down for pin 14,15 & delay for 150 cycles.
  getGPIOController()->GPPUDCLK0 = (1 << 14) | (1 << 15);
  Timer_WaitCycles(150);

	// Write 0 to GPPUDCLK0 to make it take effect.
  getGPIOController()->GPPUDCLK0 = 0;

	// clear interrupts
  getUARTController()->ICR = 0x7FF;

	// Baudrate = 115200; FUARTCLK = 3000000
	// BAUDDIV = int(FUARTCLK/(16*Baudrate))
	// BAUDFRAC = 64*(FUARTCLK/(16*Baudrate) - BAUDDIV)
  getUARTController()->IBRD = 1;
  getUARTController()->FBRD = 40;

	// FIFO
  getUARTController()->LCR_H = (1 << 4) | (1 << 5) | (1 << 6);

	// Mask interrupts
  // Étrange, seuls les 4 premiers bits sont utiles selon la doc.
	//ram_write(UART_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	//                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
  getUARTController()->IMSC = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);

	//ram_write(UART_CR, (1 << 0) | (1 << 8) | (1 << 9));
  getUARTController()->CR = CR_UARTEN | CR_TXE | CR_RXE;
}

void serial_write(unsigned char data) {
  // read flag register, wait for ready and write.
  while (getUARTController()->FR & 1 << 5);//FR_RXBUSY);
	getUARTController()->DR = data;
}

unsigned char serial_read() {
  while (getUARTController()->FR & 1 << 4);//FR_TXFE);
	return getUARTController()->DR;
}
