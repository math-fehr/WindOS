#include "uspibind.h"

#include <uspi.h>
#include "timer.h"
#include "debug.h"
#include "stdint.h"
#include "mailbox.h"
#include "string.h"
#include "interrupts.h"
#include "stdarg.h"

void MsDelay(unsigned nMilliSeconds) {
    Timer_WaitMicroSeconds(((uint32_t)1000)*(uint32_t)(nMilliSeconds));
}


void usDelay(unsigned nMicroSeconds) {
    Timer_WaitMicroSeconds(nMicroSeconds);
}


unsigned StartKernelTimer (unsigned	hzDelay, TKernelTimerHandler *handler, void *param, void *context) {
    timerFunction* function = (timerFunction*)handler;
    uint32_t millis = (uint32_t)((uint64_t)(hzDelay * 1000) / (uint64_t)HZ);
    int id = Timer_addHandler(millis,function,param,context);
    if(id == -1) {
        return 0; //Will result in a bug, but uspi didn't expect this
                  //function to fail
    }
    return id;
}


void CancelKernelTimer (unsigned id) {
    Timer_deleteHandler(id);
}


void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam) {
    connectIRQInterrupt(nIRQ,(interruptFunction*)pHandler,pParam);
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
    kernel_printf("[USPI][%s][%d] ",pSource,Severity);

    va_list args;
    va_start(args,pMessage);
    vkernel_printf(pMessage,args);
    va_end(args);
    kernel_printf("\n");
}


void uspi_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine) {
    kdebug(12,10,"Assertion '%s' on file '%s',line %d\n",pExpr,pFile,nLine);
}

void DebugHexdump (const void *pBuffer, unsigned nBufLen, const char *pSource) {
    (void)pBuffer;
    (void)nBufLen;
    (void)pSource;
    //TODO do that
    kdebug(12,0,"Needs to implement DebugHexDump function");
}

