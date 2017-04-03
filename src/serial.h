#ifndef SERIAL_H
#define SERIAL_H

#include "libc/stdbool.h"
#include "libc/stddef.h"
#include "libc/stdint.h"

#include "global.h"
#include "gpio.h"
#include "timer.h"
#include "libc/string.h"

void serial_init();
void serial_putc(unsigned char data);
void serial_write(char* str);
unsigned char serial_readc();
int serial_readline(char* buffer, int buffer_size);



#ifdef RASPI2
#define UART0_BASE 0x3F201000
#else
#define UART0_BASE 0x20201000
#endif

typedef struct {
  uint32_t DR; // 0x00
  uint32_t RSR_ECR; // 0x04
  uint8_t reserved1[0x10]; //0x08
  uint32_t FR; //0x18
  uint8_t reserved2[0x4]; // 0x1c
  uint32_t LPR; // 0x20
  uint32_t IBRD; // 0x24
  uint32_t FBRD; // 0x28
  uint32_t LCR_H; // 0x2c
  uint32_t CR; // 0x30
  uint32_t IFLS; // 0x34
  uint32_t IMSC; // 0x38
  uint32_t RIS; // 0x3c
  uint32_t MIS; // 0x40
  uint32_t ICR; // 0x44
  uint32_t DMACR; // 0x48
} rpi_uart_controller_t;

#define DR_OVERRUN_ERROR  (1 << 11)
#define DR_BREAK_ERROR    (1 << 10)
#define DR_PARITY_ERROR   (1 << 9)
#define DR_FRAMING_ERROR  (1 << 8)

#define LCRH_SPS 1 << 7
#define LCRH_WLEN_8_BITS 1 << 6 | 1 << 5
#define LCRH_WLEN_7_BITS 1 << 6
#define LCRH_WLEN_6_BITS          1 << 5
#define LCRH_WLEN_5_BITS 0
#define LCRH_FIFO 1 << 4

#define IMSC_OEIM   (1 << 10)
#define IMSC_BEIM   (1 << 9)
#define IMSC_PEIM   (1 << 8)
#define IMSC_FEIM   (1 << 7)
#define IMSC_RTIM   (1 << 6)
#define IMSC_TXIM   (1 << 5)
#define IMSC_RXIM   (1 << 4)
#define IMSC_CTSMIM (1 << 1)

#define CR_UARTEN 1
#define CR_TXE    1 << 8
#define CR_RXE    1 << 9

#define FR_TXFE   (1 << 7)
#define FR_RXFF   (1 << 6)
#define FR_TXFF   (1 << 5)
#define FR_RXFE   (1 << 4)
#define FR_BUSY   (1 << 3)
#endif
