#ifndef MAILBOX_H
#define MAILBOX_H

#include "stdint.h"

#define MBX_SUCCESS 0
#define MBX_FAILURE 1

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
