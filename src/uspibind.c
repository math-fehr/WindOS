#include "uspibind.h"

#include <uspi.h>
#include "timer.h"
#include "debug.h"
#include "stdint.h"
#include "mailbox.h"
#include "string.h"

void MsDelay(unsigned nMilliSeconds) {
    Timer_WaitMicroSeconds(((uint32_t)1000)*(uint32_t)(nMilliSeconds));
}


void usDelay(unsigned nMicroSeconds) {
    Timer_WaitMicroSeconds(nMicroSeconds);
}


unsigned StartKernelTimer (unsigned	nHzDelay, TKernelTimerHandler *pHandler, void *pParam, void *pContext) {
    return 0;
}


void CancelKernelTimer (unsigned hTimer) {
}


void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam) {
    kernel_printf("Connect interrupt");
}

int SetPowerStateOn (unsigned deviceId) {
    uint32_t device32 = (uint32_t)deviceId;
    int success = mbxSetPowerStateOn(device32);
    if(success == MBX_SUCCESS) {
        return 1;
    }
    return 0;
}


int GetMACAddress(unsigned char buffer[6]) {
    int success = mbxGetMACAddress(buffer);
    if(success == MBX_SUCCESS) {
        return 1;
    }
    return 0;
}

void LogWrite (const char *pSource, unsigned Severity, const char *pMessage, ...) {
    //TODO
    kdebug(12,0,"[%s] %s\n",pSource,pMessage);
}


void uspi_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine) {
    kdebug(12,0,"Assertion '%s' on file '%s',line %d\n",pExpr,pFile,nLine);
}

void DebugHexdump (const void *pBuffer, unsigned nBufLen, const char *pSource) {
    //TODO do that
    kdebug(12,0,"Needs to implement DebugHexDump function");
}

