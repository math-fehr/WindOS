#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>
#include <stdlib.h>

/**
 * The functions to read and to write from the mailbox.
 * Here, we send requests one by one, without using the power of the FIFO
 */

#define CHAN_POWER          0
#define CHAN_FRAMEBUFFER    1
#define CHAN_VIRTUAL_UART   2
#define CHAN_VCHIQ          3
#define CHAN_LEDS           4
#define CHAN_BUTTONS        5
#define CHAN_TOUCH_SCREEN   6
#define CHAN_TAGS_ARM_VC    8
#define CHAN_TAGS_VC_ARM    9

/**
 * Write something to the mailbox
 * The channel must be written on the first 4 bits,
 * And the value must be written on the last 28 bits
 */
int mailbox_write(uint32_t channel, uint32_t value);
int mailbox_read(uint32_t channel, uint32_t value);
int mailbox_get_memory_size();
int mailbox_get_clock_rate(int id);
int mailbox_get_emmc_clock(int id);
uint64_t mailbox_getMac();

uint32_t* mailbox_initBuffer(uint32_t* buffer);
void mailbox_closeBuffer(uint32_t volatile* buffer, uint32_t volatile* pos);
uint32_t* mailbox_writeBuffer(uint32_t volatile* buffer, uint32_t tag,
                              size_t buffer_length, size_t value_length);


uint64_t mailbox_getMacAddress();
#endif //MAILBOX_H
