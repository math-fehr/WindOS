/** \file serial.c
 * 	\brief Serial communication interface.
 */

#include "serial.h"
#include "string.h"
#include "debug.h"

static aux_t* auxiliary = (aux_t*)AUX_BASE;

extern void dmb();

/**	\fn aux_t* serial_get_aux()
 * 	\brief Return a pointer to the peripheral
 */
aux_t* serial_get_aux()
{
  return auxiliary;
}

/** \var int read_buffer_index
 * 	\brief Position in the read buffer.
 */
int read_buffer_index;

/** \var char read_buffer[MAX_BUFFER]
 *	\brief Serial port input buffer.
 */
char read_buffer[MAX_BUFFER];

/** \var int mode
 * 	\brief Serial read mode.
 *
 *	See serial_setmode for more informations.
 */
int mode;

/** \fn void serial_init()
 *	\brief Initialize the serial peripheral.
 */
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
	auxiliary->IRQ = 0;

	auxiliary->MU_IER = 0;

	/* Disable flow control,enable transmitter and receiver! */
	auxiliary->MU_CNTL = 0;

	/* Decide between seven or eight-bit mode */
	auxiliary->MU_LCR = AUX_MULCR_8BIT_MODE;


	auxiliary->MU_MCR = 0;

	/* Enable interrupts */
	auxiliary->MU_IER = 0x00;
	/* Clear the fifos */
	//auxiliary->MU_IIR = 0x06;
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
	auxiliary->MU_CNTL = AUX_MUCNTL_TX_ENABLE | AUX_MUCNTL_RX_ENABLE ;

	kdebug(D_SERIAL, 1, "Serial port is hopefully set up!\n");
	mode = 1;
}

/** \fn void serial_putc(unsigned char data)
 * 	\brief Put a single character on serial port.
 *	\param data The character to write.
 *
 *	\warning If the character is \r, a \n is added to ensure correct output.
 */
void serial_putc(unsigned char data) {
	if (data & 0x80) { // ASCII Symbol > 127. Encoding to UTF-8.
		char left  = ((data & 0xc0) >> 6) | 0b11000000;
		char right = (data & 0x3F) | 0b10000000;
		while((auxiliary->MU_LSR & AUX_MULSR_TX_EMPTY) == 0) {}
		dmb();
		auxiliary->MU_IO = left ;

		while((auxiliary->MU_LSR & AUX_MULSR_TX_EMPTY) == 0) {}
		dmb();
		auxiliary->MU_IO = right;
	} else {
		while((auxiliary->MU_LSR & AUX_MULSR_TX_EMPTY) == 0) {}
		dmb();
		auxiliary->MU_IO = data;

		if (data == '\r') {
			serial_putc('\n');
		}
	}
}

/**	\fn unsigned char serial_readc()
 *	\brief Read a single character on serial port.
 *	\return The read character.
 * 	\warning This call is blocking.
 */
unsigned char serial_readc() {
  while((auxiliary->MU_LSR & AUX_MULSR_DATA_READY) == 0) {}
  char data = auxiliary->MU_IO;
  dmb();
  return data;
}

/** \fn void serial_write(char* str)
 * 	\brief Print a null-terminated string to serial port.
 */
void serial_write(char* str){
    while((*str) != '\0') {
        serial_putc(*(str++));
    }
}

/** \fn void serial_setmode(int arg)
 * 	\param arg Chosen mode.
 *	\brief Sets the read mode for serial port.
 *
 *	Two modes are available:
 *	- 1: canonical mode, bypassing control characters and pre-handling buffer.
 *	- 2: raw mode, returns every read character.
 */
void serial_setmode(int arg) {
	if (arg >= 0 && arg < 2) {
		mode = arg;
	}
}

/** \fn void serial_irq()
 *	\brief Update serial read buffer.
 *
 *	The serial read_buffer is updated according to current mode and the content
 *	of the RX FIFO.
 */
void serial_irq() {
	//kernel_printf("fifo: %d\n", (auxiliary->MU_STAT & 0x000F0000) >> 16);
	while(auxiliary->MU_LSR & AUX_MULSR_DATA_READY) {
		char c = auxiliary->MU_IO;
		//kernel_printf("data ready: %c\n", c);
		if (c & 0x80) { // UTF-8 Symbol. Here we do a partial decoding, translating to ASCII.
			char d = serial_readc();
			c = (c << 6) | (d & 0x3F);
		}

		if (mode == 1) { // canon mode
			if (c == 0x7F) { // DEL
				if (read_buffer_index > 0) {
					serial_write("\033[D");
					serial_putc(' ');
					serial_write("\033[D");
					read_buffer[read_buffer_index--] = 0;
				}
			} else if (c == 0x1B) { // ANSI Escape sequence
				serial_readc();
				serial_readc();
			} else {
				serial_putc(c);
				read_buffer[read_buffer_index] = c;
				read_buffer_index++;
			}
		} else {
			read_buffer[read_buffer_index] = c;
			read_buffer_index++;
		}

		dmb();
	}
	//kernel_printf("fifo>: %d\n", (auxiliary->MU_STAT & 0x000F0000) >> 16);
}

/** \fn int serial_readline(char* buffer, int buffer_size)
 *	\brief Read serial buffer.
 *	\param buffer The output buffer
 *	\param buffer_size The output buffer size.
 *	\return The number of read characters.
 *
 * 	The behavior depends on the mode:
 *	- In canonical mode, returns zero until a newline symbol is found.
 *	- In raw mode, returns every read character.
 *
 *	\bug Buffer overflow in canonical mode.
 */
int serial_readline(char* buffer, int buffer_size) {
	serial_irq();
	if (read_buffer_index == 0) {
		return 0;
	}

	if (mode == 0) {
		int sz = min(buffer_size, read_buffer_index);
		memcpy(buffer, read_buffer, sz);
		read_buffer_index -= sz;
		return sz;
	}

	int i = 0;

	if (buffer_size > MAX_BUFFER)
		buffer_size = MAX_BUFFER;

	for (;i<buffer_size && i<read_buffer_index && read_buffer[i] != '\r';i++) {}

	if (i == read_buffer_index) {
		return 0;
	}
	memcpy(buffer, read_buffer, i+1);
	buffer[i+1] = 0;
	buffer[i] = '\n';

	read_buffer_index -= i+1;

	for (int j=0;j<buffer_size;j++) {
		read_buffer[j] = read_buffer[j+i];
	}
  	return i+1;
}
