#include "mailbox.h"
#include "kernel.h"
#include "arm.h"

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


// Tag values
const uint32_t TAG_MACADDR = 0x00010003;

int mailbox_write(uint32_t channel, uint32_t value) {

    // The channel must be written on the first 4 bits,
    // And the value must be written on the last 28 bits
    if(((channel & 0xFFFFFFF0) || (value & 0xF)) != 0) {
        return -1;
    }

    //Wait if the mailbox is full
    while((*(volatile uint32_t*)(MB_STATUS) & MAIL_FULL) != 0) ;

    uint32_t command = channel | value;

    dmb();
    *(volatile uint32_t*)(MB_WRITE) = command;
    dmb();

    return 0;
}


int mailbox_read(uint32_t channel, uint32_t value) {

    // The channel must be written on the first 4 bits,
    if((channel & 0xFFFFFFF0) != 0 || (value & 0xF) != 0) {
        return -1;
    }

    uint32_t mail;

    do {
        //Wait until there is data to read
        while((*(volatile uint32_t*)(MB_STATUS) & MAIL_EMPTY) != 0) ;

        dmb();
        mail = *(volatile uint32_t*)(MB_READ);
        dmb();
    } while((mail != (channel | value))); //We read only from our channel

    return 0;
}


uint32_t* mailbox_createBuffer(size_t size) {
    if((size & 0xF) != 0) {
        size += 0x10 - (size & 0xF);
    }

    //the data need to be aligned
    uint32_t* buffer = (uint32_t*)memalign(16,size);
    return buffer;
}

uint32_t* mailbox_initBuffer(uint32_t* buffer) {
    buffer[0] = 0;
    buffer[1] = 0;
    buffer += 2;
    return buffer;
}


void mailbox_closeBuffer(uint32_t volatile* buffer, uint32_t volatile* pos) {
    *(pos++) = 0; //End tag
    buffer[0] = (uint32_t) (pos - buffer);
}


uint32_t* mailbox_writeBuffer(uint32_t volatile* buffer, uint32_t tag,
                         size_t buffer_length, size_t value_length) {
    buffer[0] = tag;
    buffer[1] = buffer_length;
    buffer[2] = value_length & 0x7FFFFFFF;

    for(size_t i=3; i<buffer_length+3; i++) {
        buffer[i] = 0;
    }

    return (uint32_t*) (buffer + 3 + buffer_length/4);

}


uint64_t mailbox_getMacAddress() {
    static uint64_t mac_address = 0; //The address wont change

    if(mac_address != 0) {
        return mac_address;
    }

    //We create the buffer
    uint32_t *buffer = mailbox_createBuffer(64);
    uint32_t *write_addr = mailbox_initBuffer(buffer);
    uint32_t* end_addr = mailbox_writeBuffer(write_addr,TAG_MACADDR,8,0);
    mailbox_closeBuffer(buffer,end_addr);

    //And then we write the request
    mailbox_write(8,(uint32_t)buffer);
    mailbox_read(8,(uint32_t)buffer);
    for(int i = 0; i<15; i++) {
        kernel_printf("%d\n", buffer[i]);
    }
    mac_address = buffer[5] | (((uint64_t)buffer[6]) << 32);

    free(buffer);

    return mac_address;
}
