#include "serial2.h"
#include "string.h"
#include "process.h"
#include "scheduler.h"

static volatile rpi_uart_controller_t* UARTController =
  (rpi_uart_controller_t*) UART0_BASE;

volatile rpi_uart_controller_t* getUARTController() {
  return UARTController;
}

extern void dmb();

int read_buffer_index_2;
char read_buffer_2[MAX_BUFFER];
int mode_2;

void serial2_init() {
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
  getUARTController()->IMSC = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);

	//ram_write(UART_CR, (1 << 0) | (1 << 8) | (1 << 9));
  getUARTController()->CR = CR_UARTEN | CR_TXE | CR_RXE;
}

void serial2_putc(unsigned char data) {
  // read flag register, wait for ready and write.


  if (data == '\r') {
    serial2_putc('\n');
  }
  if (data & 0x80) { // ASCII Symbol > 127. Encoding to UTF-8.
	  char left  = ((data & 0xc0) >> 6) | 0b11000000;
	  char right = (data & 0x3F) | 0b10000000;
	  while (getUARTController()->FR & FR_TXFF);
	  dmb();
	  getUARTController()->DR = left;

	  while (getUARTController()->FR & FR_TXFF);
	  dmb();
	  getUARTController()->DR = right;
  } else {
  	while (getUARTController()->FR & FR_TXFF);
	  dmb();
	  getUARTController()->DR = data;
  }
}

void serial2_write(char* str){
    while((*str) != '\0') {
        serial2_putc(*(str++));
    }
}

unsigned char serial2_readc() {
	while (getUARTController()->FR & FR_RXFE);
	return getUARTController()->DR;
}

void serial2_setmode(int arg) {
   if (arg >= 0 && arg < 2) {
	   mode_2 = arg;
   }
}


void serial2_irq() {
   //kernel_printf("fifo: %d\n", (auxiliary->MU_STAT & 0x000F0000) >> 16);
   while(!(getUARTController()->FR & FR_RXFE)) {
	//s   kernel_printf("serial2 IRQ\n");
	   char c = getUARTController()->DR;
	   //kernel_printf("data ready: %c\n", c);
	   if (c & 0x80) { // UTF-8 Symbol. Here we do a partial decoding, translating to ASCII.
		   char d = serial2_readc();
		   c = (c << 6) | (d & 0x3F);
	   }

	   if (c == 1) { // Ctrl-A
		   kernel_printf("\nCurrent process: %d\n", get_current_process_id());
	   }

	   if (c == 2) { // Ctrl-B
		   kernel_printf("\n");
		   paging_print_status();
	   }

	   if (c == 4) { // Ctrl-D
		   process** list = get_process_list();
		   kernel_printf("\n");
		   kernel_printf("%d %d %d\n", get_number_active_processes(), get_number_free_processes(), get_number_zombie_processes());
		   for (int i = 0;i<MAX_PROCESSES;i++) {
			   if (list[i] != NULL) {
				   char* sstr;
				   switch (list[i]->status) {
					   case status_active:
						   sstr = "R";
						   break;
					   case status_zombie:
						   sstr = "Z";
						   break;
					   case status_blocked_svc:
						   sstr = "BS";
						   break;
					   case status_wait:
						   sstr = "S";
						   break;
				   }
				   kernel_printf("[%d] %2s %d %s | %x\n", i, sstr, list[i]->parent_id, list[i]->name, list[i]->ctx.pc);
			   }
		   }
	   }

	   if (mode_2 == 1) { // canon mode
		   if (c == 0x7F || c == 0x08) { // DEL
			   if (read_buffer_index_2 > 0) {
				   serial2_write("\033[D");
				   serial2_putc(' ');
				   serial2_write("\033[D");
				   read_buffer_2[read_buffer_index_2--] = 0;
			   }
		   } else if (c == 0x1B) { // ANSI Escape sequence
			   serial2_readc();
			   serial2_readc();
		   } else if (c <= 31 && c != '\r') { // Ctrl-Something
		   /*	if (c == 4) { // Ctrl-D: send sigterm.
				   siginfo_t sgn;
				   sgn.si_pid = -1;
				   sgn.si_signo = SIGTERM;
				   if (process_signal(get_current_process(), sgn)) {
					   kill_process(get_current_process_id(), (SIGTERM << 8) | 1);
				   }
				   return;
			   }*/
		   } else {
			   serial2_putc(c);
			   read_buffer_2[read_buffer_index_2] = c;
			   read_buffer_index_2++;
		   }
	   } else {
		   read_buffer_2[read_buffer_index_2] = c;
		   read_buffer_index_2++;
	   }

	   dmb();
   }
   //kernel_printf("fifo>: %d\n", (auxiliary->MU_STAT & 0x000F0000) >> 16);
}

int serial2_readline(char* buffer, int buffer_size) {
	serial2_irq();
	if (read_buffer_index_2 == 0) {
	   return 0;
	}

	if (mode_2 == 0) {
		int sz = min(buffer_size, read_buffer_index_2);
		memcpy(buffer, read_buffer_2, sz);
		read_buffer_index_2 -= sz;
		return sz;
	}

   int i = 0;

   if (buffer_size > MAX_BUFFER)
	   buffer_size = MAX_BUFFER;

   for (;i<buffer_size && i<read_buffer_index_2 && read_buffer_2[i] != '\r';i++) {}

   if (i == read_buffer_index_2) {
	   return 0;
   }
   memcpy(buffer, read_buffer_2, i+1);
   buffer[i+1] = 0;
   buffer[i] = '\n';

   read_buffer_index_2 -= i+1;

   for (int j=0;j<buffer_size;j++) {
	   read_buffer_2[j] = read_buffer_2[j+i];
   }
   return i+1;
}
