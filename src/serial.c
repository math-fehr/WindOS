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

#define MAX_BUFFER 1024

int read_buffer_index;
char read_buffer[MAX_BUFFER];
int mode;


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

void serial_putc(unsigned char data) {
  while((auxiliary->MU_LSR & AUX_MULSR_TX_EMPTY) == 0) {}
  dmb();
  auxiliary->MU_IO = data;

  if (data == '\r') {
    serial_putc('\n');
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

void serial_setmode(int arg) {
	if (arg >= 0 && arg < 2) {
		mode = arg;
	}
}

void serial_irq() {
	//kernel_printf("fifo: %d\n", (auxiliary->MU_STAT & 0x000F0000) >> 16);
	while(auxiliary->MU_LSR & AUX_MULSR_DATA_READY) {
		char c = auxiliary->MU_IO;
		//kernel_printf("data ready: %c\n", c);
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

/*
 * this is not so safe (buffer overflow.)
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
