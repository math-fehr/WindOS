#ifndef MAILBOX_H
#define MAILBOX_H

#include "stdint.h"
#include "stdbool.h"

#define MBX_SUCCESS 0
#define MBX_FAILURE 1

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
