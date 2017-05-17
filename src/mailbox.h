#ifndef MAILBOX_H
#define MAILBOX_H

#include "stdint.h"
#include "stdbool.h"

#define MBX_SUCCESS 0
#define MBX_FAILURE 1


#define MBX_CHANNEL_PM	0           // power management
#define MBX_CHANNEL_FB 	1			// frame buffer
#define MBX_CHANNEL_PROP	8       // property tags (ARM to VC)

#define PROPTAG_END                0x00000000


/**
 * Values used in propBuffer.code
 */
#define PROPBUFF_REQUEST    0x00000000
#define PROPBUFF_SUCCESS    0x80000000
#define PROPBUFF_FAILURE    0x80000001

/** \st propTagHeader
 *  \brief The header of the property tag
 */
typedef struct propTagHeader {
    uint32_t tagID;           ///< The tag ID
    uint32_t valueBuffSize;   ///< The size of the value buffer
    uint32_t valueLength;     ///< The value length
} propTagHeader;
#define VALUE_LENGTH_RESPONSE	(1 << 31)


/** \st propBuffer
 *  \brief The property buffer
 *  Contains different tags to send to the mailbox
 */
typedef struct propBuffer {
    uint32_t bufferSize;    ///< The size of the buffer
    uint32_t code;          ///< The code of the message (Response, or send)
    uint8_t tags[0];        ///< The start of the tags
} propBuffer;



/**
 * Wait for the mailbox to be empty
 */
void mbxFlush();

/**
 * Read in the mailbox at a specified channel
 */
uint32_t mbxRead(uint32_t channel);

/**
 * Write in the mailbox
 */
void mbxWrite(uint32_t channel, uint32_t data);

/**
 * Get the result of a request
 */
bool mbxGetTag (uint32_t channel, uint32_t tagID, void* tag, uint32_t tagSize, uint32_t requestParamSize);

/**
 * Write in the mailbox and wait for the answer
 */
uint32_t mbxWriteRead(uint32_t channel, uint32_t data);

/**
 * get the MAC address
 */
int mbxGetMACAddress(uint8_t buffer[6]);

/**
 * Set device power state on
 */
int mbxSetPowerStateOn(uint32_t deviceID);

/**
 * Set device power state off
 */
int mbxSetPowerStateOff(uint32_t deviceID);

#endif
