#include "uspibind.h"

#include <uspi.h>
#include "timer.h"
#include "debug.h"
#include "stdint.h"
#include "mailbox.h"
#include "string.h"
#include "interrupts.h"
#include "stdarg.h"


/** \file uspibind.c
 *  \brief This file contains the needed USPi function
 *  Uspi needs the implementation of some basic functions to work.
 */


/** \fn void MsDelay(unsigned nMilliSeconds)
 *  \brief Wait for n milliseconds
 *  \param nMilliSeconds the number of milliseconds to wait
 */
void MsDelay(unsigned nMilliSeconds) {
    Timer_WaitMicroSeconds(((uint32_t)1000)*(uint32_t)(nMilliSeconds));
}


/** \fn void usDelay(unsigned nMicroSeconds)
 *  \brief Wait for n microseconds
 *  \param nMicroSeconds The number of microseconds to wait
 */
void usDelay(unsigned nMicroSeconds) {
    Timer_WaitMicroSeconds(nMicroSeconds);
}


/** \fn unsigned StartKernelTimer (unsigned	hzDelay, TKernelTimerHandler *handler, void *param, void *context)
 *  \brief Delay The call of a function
 *  \param hzDelay The delay of the function in 1/100 seconds
 *  \param handler The address of the function handler
 *  \param param The address of the first function parameter
 *  \param context The address of the second function parameter
 *  \return The index of the handler (if the function fails, then return 0xFFFFFFFF)
 */
unsigned StartKernelTimer (unsigned	hzDelay, TKernelTimerHandler *handler, void *param, void *context) {
    timerFunction* function = (timerFunction*)handler;
    uint32_t millis = (uint32_t)((uint64_t)(hzDelay * 1000) / (uint64_t)HZ);
    int id = Timer_addHandler(millis,function,param,context);
    if(id == -1) {
        return -1; //Will result in a bug, but uspi didn't expect this
                   //function to fail
    }
    return id;
}


/** \fn void CancelKernelTimer (unsigned id)
 *  \brief Remove a delayed function
 *  \param id The id of the function handler
 */
void CancelKernelTimer (unsigned id) {
    Timer_deleteHandler(id);
}


/** \fn void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam)
 *  \brief Call a function whenever a given IRQ was triggered
 *  \param nIRQ The IRQ number
 *  \param pHandler The function handler
 *  \param pParam The function parameter
 */
void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam) {
    connectIRQInterrupt(nIRQ,(interruptFunction*)pHandler,pParam);
}


/** \fn int SetPowerStateOn (unsigned deviceId)
 *  \brief Set power on on a device
 *  \param deviceID The id of the device
 *  \return 1 on success, and 0 on failure
 */
int SetPowerStateOn (unsigned deviceId) {
    uint32_t device32 = (uint32_t)deviceId;
    int success = mbxSetPowerStateOn(device32);
    if(success == MBX_SUCCESS) {
        return 1;
    }
    return 0;
}

/** \fn int GetMACAddress(unsigned char buffer[6])
 *  \brief Retrieve the MAC address
 *  \param buffer The buffer to store the mac address
 *  \return 1 on success, and 0 on failure
 */
int GetMACAddress(unsigned char buffer[6]) {
    int success = mbxGetMACAddress(buffer);
    if(success == MBX_SUCCESS) {
        return 1;
    }
    return 0;
}


/** \fn void LogWrite (const char *pSource, unsigned Severity, const char *pMessage, ...)
 *  \brief Write log on the serial
 *  \param pSource The source of the log
 *  \param Severity The severity of the log
 *  \param pMessage The message (in printf format)
 */
void LogWrite (const char *pSource, unsigned Severity, const char *pMessage, ...) {
    kernel_printf("[USPI][%s][%d] ",pSource,Severity);

    va_list args;
    va_start(args,pMessage);
    vkernel_printf(pMessage,args);
    va_end(args);
    kernel_printf("\n");
}


/** \fn void uspi_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine)
 *  \brief The action to do when USPi assert failed
 *  \param pExpr The expression which failed
 *  \param pFile The file where the assertion failed
 *  \param nLine The line where the assertion failed
 */
void uspi_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine) {
    kdebug(12,10,"Assertion '%s' on file '%s',line %d\n",pExpr,pFile,nLine);
}


/** \fn void DebugHexdump (const void *pBuffer, unsigned nBufLen, const char *pSource)
 *  \brief Debug hexadecimals values
 *  \param pBuffer The buffer containing the values
 *  \param nBufLen The size of the buffer
 *  \pSource The source of the values
 *  \warning Not implemented
 */
void DebugHexdump (const void *pBuffer, unsigned nBufLen, const char *pSource) {
    (void)pBuffer;
    (void)nBufLen;
    (void)pSource;
    //TODO do that
    kdebug(12,0,"Needs to implement DebugHexDump function");
}

