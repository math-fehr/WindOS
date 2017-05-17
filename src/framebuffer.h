#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "framebuffer.h"
#include "stdbool.h"
#include "stdint.h"

#define FB_SUCCESS 0
#define FB_FAILURE 1


/** \st frameBuffer
 *  \brief The frameBuffer shared between the CPU and the GPU
 *  \warning This will be 16 byte aligned in memory
 */
typedef struct frameBuffer {
    uint32_t width;          ///<Physical width
    uint32_t height;         ///<Physical height
    uint32_t virtualWidth;   ///<Virtual width (equal to physical)
    uint32_t virtualHeight;  ///<Virtual height (equal to physical)
    uint32_t pitch;          ///<Should be 0
    uint32_t depth;          ///<Bits per pixel
    uint32_t offsetX;        ///<Should be 0
    uint32_t offsetY;        ///<Should be 0
    uint32_t bufferPtr;      ///<Frame buffer address
    uint32_t bufferSize;     ///<Size of frame buffer
    uint16_t palette[256];   ///<Used only when depth = 8 (256 colors)
} frameBuffer;


/**
 * Initialize the frame buffer
 */
bool fb_initialize(frameBuffer* fb, unsigned width, unsigned height, unsigned depth, unsigned vir_height);

void fb_set_offset_y(unsigned offset);

#endif // FRAMEBUFFER_H
