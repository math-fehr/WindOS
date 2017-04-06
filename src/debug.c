#include "debug.h"

void kernel_error(char* source, char* message) {
  serial_write("[ERROR] ");
  serial_write(source);
  serial_write(" : ");
  serial_write(message);
  serial_newline();
}

void kernel_info(char* source, char* message) {
  serial_write("[INFO] ");
  serial_write(source);
  serial_write(" : ");
  serial_write(message);
  serial_newline();
}


void print_hex(int number, int nbytes) {
  serial_write("  0x");
  for (int i=0;i<2*nbytes;i++) {
    char hx = (number >> (4*(2*nbytes-1-i))) & 15;
    if (hx < 10) {
      serial_putc(48+hx);
    } else {
      serial_putc(65+hx-10);
    }
  }
  serial_newline();
}
