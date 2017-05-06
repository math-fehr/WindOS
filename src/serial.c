#include "serial.h"
#include "string.h"
#include "debug.h"

#define SYS_FREQ 250000000

static aux_t* auxiliary = (aux_t*)AUX_BASE;

/**
 * Ensures that data comes in order when accessing the peripheral
 */
extern void dmb();

aux_t* serial_get_aux()
{
  return auxiliary;
}

void serial_init() {
  volatile int i;
  dmb();
  /* As this is a mini uart the configuration is complete! Now just
     enable the uart. Note from the documentation in section 2.1.1 of
     the ARM peripherals manual:

     If the enable bits are clear you will have no access to a
     peripheral. You can not even read or write the registers */
  auxiliary->ENABLES = AUX_ENA_MINIUART;

  /* Disable interrupts for now */
  /* auxillary->IRQ &= ~AUX_IRQ_MU; */

  auxiliary->MU_IER = 0;

  /* Disable flow control,enable transmitter and receiver! */
  auxiliary->MU_CNTL = 0;

  /* Decide between seven or eight-bit mode */
  auxiliary->MU_LCR = AUX_MULCR_8BIT_MODE;


  auxiliary->MU_MCR = 0;

  /* Disable all interrupts from MU and clear the fifos */
  auxiliary->MU_IER = 0;

  auxiliary->MU_IIR = 0xC6;

  /* Transposed calculation from Section 2.2.1 of the ARM peripherals
     manual */
  int baud = 115200;
  auxiliary->MU_BAUD = ( SYS_FREQ / ( 8 * baud ) ) - 1;

   /* Setup GPIO 14 and 15 as alternative function 5 which is
      UART 1 TXD/RXD. These need to be set before enabling the UART */
  GPIO_setPinFunction(14, PIN_FUN5 );
  GPIO_setPinFunction(15, PIN_FUN5 );

  getGPIOController()->GPPUD = 0;
  for( i=0; i<150; i++ ) { }
  getGPIOController()->GPPUDCLK0 = ( 1 << 14 );
  for( i=0; i<150; i++ ) { }
  getGPIOController()->GPPUDCLK0 = 0;
  dmb();
  /* Disable flow control,enable transmitter and receiver! */
  auxiliary->MU_CNTL = AUX_MUCNTL_TX_ENABLE | AUX_MUCNTL_RX_ENABLE;

  kdebug(D_SERIAL, 1, "Serial port is hopefully set up!\n");
}

void serial_putc(unsigned char data) {
  while((auxiliary->MU_LSR & AUX_MULSR_TX_EMPTY) == 0) {}
  dmb();
  auxiliary->MU_IO = data;

  if (data == '\n') {
    serial_putc('\r');
  }
}
unsigned char serial_readc() {
  while((auxiliary->MU_LSR & AUX_MULSR_DATA_READY) == 0) {}
  char data = auxiliary->MU_IO;
  dmb();
  return data;
}


void serial_write(char* str){
    while((*str) != '\0') {
        serial_putc(*(str++));
    }
}

void serial_newline() {
    serial_putc('\n');
}

int serial_readline(char* buffer, int buffer_size) {
	int i = 0;
	char c;

	while ((i < buffer_size) && ((c = serial_readc()) != '\r')){
		if (c == 0x7F) {
			serial_write("\033[D");
			serial_putc(' ');
			serial_write("\033[D");
			buffer[i--] = 0;	
		} else if (c == 0x1B) {
			serial_readc();
			serial_readc();
		} else {
			serial_putc(c);

			buffer[i] = c;
			i++;
		}

	}
	if (i < buffer_size) {
		buffer[i] = '\n';
		i++;
		serial_newline();
	}
  	return i;
}
