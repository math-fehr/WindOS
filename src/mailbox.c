#include "mailbox.h"
#include "arm.h"
#include "timer.h"
#include "string.h"
#include "kernel.h"
#include "stdint.h"
#include "debug.h"


/**
 * Wait for the mailbox to be empty
 */
static void mbxFlush();

/**
 * Read in the mailbox at a specified channel
 */
static uint32_t mbxRead(uint32_t channel);

/**
 * Write in the mailbox
 */
static void mbxWrite(uint32_t channel, uint32_t data);

/**
 * Get the result of a request
 */
static bool mbxGetTag (uint32_t channel, uint32_t tagID, void* tag, uint32_t tagSize, uint32_t requestParamSize);

/**
 * Write in the mailbox and wait for the answer
 */
static uint32_t mbxWriteRead(uint32_t channel, uint32_t data);



#define MBX_BASE		(PERIPHERALS_BASE + 0xB880)
#define MBX0_STATUS 	(MBX_BASE + 0x18)
#define MBX0_READ  		(MBX_BASE + 0x00)
#define MBX1_WRITE		(MBX_BASE + 0x20)
#define MBX1_STATUS 	(MBX_BASE + 0x38)

#define MBX_STATUS_EMPTY	0x40000000
#define MBX_STATUS_FULL	    0x80000000

#define MBX_CHANNEL_PM	0           // power management
#define MBX_CHANNEL_FB 	1			// frame buffer
#define MBX_CHANNEL_PROP	8       // property tags (ARM to VC)

#define PROPTAG_END                0x00000000

/**
 * The header of the property tag
 */
typedef struct propTagHeader {
    uint32_t tagID;
    uint32_t valueBuffSize;
    uint32_t valueLength;
} propTagHeader;
#define VALUE_LENGTH_RESPONSE	(1 << 31)


/**
 * The property buffer
 * Contains different tags to send to the mailbox
 */
typedef struct propBuffer {
    uint32_t bufferSize;
    uint32_t code;
    uint8_t tags[0]; //This indicates the start of the tags
} propBuffer;

/**
 * Values used in propBuffer.code
 */
#define PROPBUFF_REQUEST    0x00000000
#define PROPBUFF_SUCCESS    0x80000000
#define PROPBUFF_FAILURE    0x80000001




static uint32_t mbxWriteRead(uint32_t channel, uint32_t data) {
    dmb();
    mbxFlush(channel);
    mbxWrite(channel,data);
    uint32_t result = mbxRead(channel);
    dmb();
    return result;
}


static void mbxFlush() {
    while(!(*(uint32_t*)(MBX0_STATUS) & MBX_STATUS_EMPTY)) {
        //TODO check if the data is actually read
        *(volatile uint32_t*)(MBX0_READ);  // Update the data
        Timer_WaitCycles(100);
    }
}


static uint32_t mbxRead(uint32_t channel) {
    uint32_t result;
    do {
        while(*(uint32_t*)(MBX0_STATUS) & MBX_STATUS_EMPTY) {
            Timer_WaitCycles(1000);
        }
        result = *(uint32_t*)(MBX0_READ);
    } while((result & 0xF) != channel);
    return result & 0xFFFFFFF0;
}


static void mbxWrite(uint32_t channel, uint32_t data) {
    // We wait for the mailbox to have space
    while(*(uint32_t*)(MBX1_STATUS) & MBX_STATUS_FULL) {}

    *(uint32_t*)(MBX1_WRITE) = channel | data;
}


static bool mbxGetTag (uint32_t channel, uint32_t tagID, void* tag, uint32_t tagSize, uint32_t requestParamSize) {
    uint32_t bufferSize = sizeof(propBuffer) + tagSize + sizeof(uint32_t);
    uint8_t buffer[bufferSize + 15];
    propBuffer* propBuff = (propBuffer*) (((uint32_t) buffer + 15) & ~15);

    propBuff->bufferSize = bufferSize;
    propBuff->code = PROPBUFF_REQUEST;
    memcpy(propBuff->tags, tag, tagSize);

    propTagHeader* header = (propTagHeader*) propBuff->tags;
    header->tagID = tagID;
    header->valueBuffSize = tagSize - sizeof(propTagHeader);
    header->valueLength = requestParamSize & ~VALUE_LENGTH_RESPONSE;

    uint32_t* endTag = (uint32_t*) (propBuff->tags + tagSize);
    *endTag = PROPTAG_END;

    cleanDataCache();
    dsb();

    uint32_t bufferAddress = ((uint32_t)propBuff & ~0xC0000000) | GPU_MEM_BASE;
    if (mbxWriteRead (channel, bufferAddress) != bufferAddress)
    {
        kdebug(13,5,"Error while writing/reading in mailbox\n");
        return false;
    }

    dmb();

    if(propBuff->code != PROPBUFF_SUCCESS) {
        kdebug(13,5,"Failure from VC\n");
        return false;
    }

    if(!(header->valueLength & VALUE_LENGTH_RESPONSE)) {
        kdebug(13,5,"Length response was different than expected\n");
        return false;
    }
    header->valueLength &= ~VALUE_LENGTH_RESPONSE;

    if(header->valueLength == 0) {
        kdebug(13,5,"Error in valueLength\n");
        return false;
    }

    memcpy(tag, propBuff->tags, tagSize);

    return true;
}


/**
 * The property tag for getting the mac address
 */
typedef struct propTagMACAddress {
    propTagHeader header;
    uint8_t address[6];
    uint8_t padding[2];
} propTagMACAddress;

#define PROPTAG_GET_MAC_ADDRESS    0x00010003

int mbxGetMACAddress(uint8_t buffer[6]) {
    static uint8_t cachedAddress[6];    //We keep in memory the mac address
    static bool isCached = false;

    if(isCached) {
        memcpy(buffer,cachedAddress,6);
        return MBX_SUCCESS;
    }

    uint32_t channel = MBX_CHANNEL_PROP;
    propTagMACAddress macAddress;
    if(!mbxGetTag(channel,PROPTAG_GET_MAC_ADDRESS, &macAddress, sizeof macAddress, 0)) {
        return MBX_FAILURE;
    }
    memcpy(buffer,macAddress.address, 6);
    memcpy(cachedAddress,macAddress.address, 6);
    isCached = true;
    return MBX_SUCCESS;
}


// Id of important devices
#define DEVICE_ID_SD_CARD	0
#define DEVICE_ID_USB_HCD	3

// values of actions
#define POWER_STATE_OFF		(0 << 0)
#define POWER_STATE_ON		(1 << 0)
#define POWER_STATE_WAIT	(1 << 1)

// Response of VC
#define POWER_STATE_NO_DEVICE	(1 << 1)


/**
 * The property tag for setting power state for devices
 */
typedef struct propTagPowerState
{
	propTagHeader header;
	uint32_t		deviceID;
	uint32_t		state;
} propTagPowerState;

#define PROPTAG_SET_POWER_STATE		0x00028001

int mbxSetPowerStateOn(uint32_t deviceID) {
    propTagPowerState tag;
    uint32_t channel = MBX_CHANNEL_PROP;
    tag.deviceID = deviceID;
    tag.state = POWER_STATE_ON | POWER_STATE_WAIT;
    if(!mbxGetTag(channel,PROPTAG_SET_POWER_STATE,&tag,sizeof(tag),8)) {
        kdebug(13,5,"Error while getting tag for power state");
        return MBX_FAILURE;
    }
    else if(tag.state & POWER_STATE_NO_DEVICE) {
        kdebug(13,5,"Device %d not found while setting power on",deviceID);
        return MBX_FAILURE;
    }
    else if(!(tag.state & POWER_STATE_ON)) {
        kdebug(13,5,"Device %d was expected to be powered on, but failed",deviceID);
        return MBX_FAILURE;
    }

    return MBX_SUCCESS;
}


int mbxSetPowerStateOff(uint32_t deviceID) {
    propTagPowerState tag;
    uint32_t channel = MBX_CHANNEL_PROP;
    tag.deviceID = deviceID;
    tag.state = POWER_STATE_OFF | POWER_STATE_WAIT;
    if(!mbxGetTag(channel,PROPTAG_SET_POWER_STATE,&tag,sizeof(tag),8)) {
        return MBX_FAILURE;
    }
    else if(tag.state & POWER_STATE_NO_DEVICE) {
        kdebug(13,5,"Device %d not found while setting power off",deviceID);
        return MBX_FAILURE;
    }
    else if(!(tag.state & POWER_STATE_OFF)) {
        kdebug(13,5,"Device %d was expected to be powered off, but failed",deviceID);
        return MBX_FAILURE;
    }

    return MBX_SUCCESS;
}
