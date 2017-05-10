#ifndef MAILBOX_H
#define MAILBOX_H

#define MBX_SUCCESS 0
#define MBX_FAILURE 1

/**
 * get the MAC address
 */
int mbxGetMACAddress(unsigned char buffer[6]);

#endif
