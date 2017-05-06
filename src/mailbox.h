#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>

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
int mailbox_read(uint32_t channel, uint32_t* value);
int mailbox_get_memory_size();
int mailbox_get_clock_rate(int id);
int mailbox_get_emmc_clock(int id);

#endif //MAILBOX_H
