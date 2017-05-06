#include "mailbox.h"
#include "kernel.h"

// This is the bit indicating that the mailbox is full
// and nothing can be written on it
#define MAIL_FULL (1UL << 31)
// This is the bit indicating that the mailbox is empty,
// and that there is nothing to read
#define MAIL_EMPTY (1UL << 30)

// The address of the peripherals to read,write, and get status
#define MB_READ   (PERIPHERALS_BASE + 0xB880)
#define MB_WRITE  (PERIPHERALS_BASE + 0xB8A0)
#define MB_STATUS (PERIPHERALS_BASE + 0xB898)

int mailbox_write(uint32_t channel, uint32_t value) {

    // The channel must be written on the first 4 bits,
    // And the value must be written on the last 28 bits
    if(((channel & 0xFFFFFFF0) || (value & 0xF)) != 0) {
        return -1;
    }

    //Wait if the mailbox is full
    while((*(volatile uint32_t*)(MB_STATUS) & MAIL_FULL) != 0) ;

    uint32_t command = channel | value;
    *(volatile uint32_t*)(MB_WRITE) = command;

    return 0;
}


int mailbox_read(uint32_t channel, uint32_t* value) {

    // The channel must be written on the first 4 bits,
    if((channel & 0xFFFFFFF0) != 0) {
        return -1;
    }

    uint32_t mail;

    do {
        //Wait until there is data to read
        while((*(volatile uint32_t*)(MB_STATUS) & MAIL_FULL) != 0) ;

        mail = *(volatile uint32_t*)(MB_READ);
    } while((mail & 0xF) != channel); //We read only from our channel

    *value = mail & (0xFFFFFFF0);
    return 0;
}

int mailbox_get_clock_rate(int id) {
	uint32_t addr;
	uint32_t mail;
	uint32_t pt[8192] __attribute__((aligned(16)));

	for(int i=0; i<8192; i++) {
		pt[i]=0;
	}

    int i = 0;

	pt[i] = 12; // placeholder for the total size
	pt[i] =  0; // Request code: process request
	pt[i] = 0x00030002; // tag identifeir = 0x00030002: get clock rate)
	pt[i] = 8; // value buffer size in bytes ((1 entry)*4)
	pt[i] = 0; // 1bit request/response (0/1), 31bit value length in bytes
	pt[i] = 1; // id = 2
	pt[i] = 0; // return value 2
	pt[i] = 0; // stop tag
	pt[0] = i*4;

	addr = ((uint32_t) pt) + 0x40000000;
	mailbox_write(addr, 8);
	mailbox_read(8, &mail);

	if (*((uint32_t *) (mail+4)) == 0x80000000) {
		return *((uint32_t *) (mail+6*4));
	}
	return -1;
}
