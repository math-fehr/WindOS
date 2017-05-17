#include "framebuffer.h"
#include "mailbox.h"
#include "arm.h"
#include "kernel.h"
#include "debug.h"
#include "malloc.h"


#define MBX_CHANNEL_FB 1




/** \fn bool fb_initialize(unsigned width, unsigned height, unsigned depth)
 *  \brief Initialize the current frameBuffer
 *  \param fb The address of the frameBuffer to initialize
 *  \param width The wanted width (is choosen optimally if =0 or height=0)
 *  \param height The wanted height (is choosen optimally if =0 or width=0)
 *  \param depth The wanted number of bits per pixels (=8 is set to 0)
 *  \warning Optimal chose is not yet implemented
 */
bool fb_initialize(frameBuffer* fb, unsigned width, unsigned height, unsigned depth) {
    if(height==0 || width == 0) {
        //TODO implement that
    }
    if(depth == 0) {
        depth = 8;
    }

    fb->width = width;
    fb->height = height;
    fb->virtualWidth = width;
    fb->virtualHeight = height;
    fb->pitch = 0;
    fb->depth = depth;
    fb->offsetX = 0;
    fb->offsetY = 0;
    fb->bufferPtr = 0;
    fb->bufferSize = 0;

    //for(int i = 0; i<256; i++) {
    //    fb->palette[i] = i *i;
    //}

    uint32_t fbBusAddress = ((uint32_t)(fb) & ~0xC0000000) | GPU_MEM_BASE;
    cleanDataCache();
    dsb();
    uint32_t result = mbxWriteRead(MBX_CHANNEL_FB,fbBusAddress);
    dmb();

    if(result != 0) {
        return FB_FAILURE;
    }
    if(fb->bufferPtr == 0) {
        return FB_FAILURE;
    }

    return FB_SUCCESS;
}

