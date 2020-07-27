#include <stdint.h>
#include "mmio.h"
#include "uart.h"
#include "mailbox.h"
#include "framebuffer.h"

/* PC Screen Font as used by Linux Console */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int headersize;
    unsigned int flags;
    unsigned int numglyph;
    unsigned int bytesperglyph;
    unsigned int height;
    unsigned int width;
    unsigned char glyphs;
} __attribute__((packed)) psf_t;
extern volatile unsigned char _binary_font_psf_start;

unsigned int width, height, pitch, isrgb;   /* dimensions and channel order */
uint8_t *fb;                         /* raw frame buffer address */
int bgcolor = 0x0;
int textcolor = 0xFFFFFF;
psf_t *font;

void fb_init() {
    font = (psf_t*)&_binary_font_psf_start;

    mbox[0] = 35*4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48003;  //set physical width/height
    mbox[3] = 8;
    mbox[4] = 8;
    // use font size to set up standered 80x24 char terminal
    mbox[5] = (font->width * 80);         //FrameBufferInfo.width
    mbox[6] = (font->height * 24);        //FrameBufferInfo.height

    mbox[7] = 0x48004;  //set virtual width/height
    mbox[8] = 8;
    mbox[9] = 8;
    // use font size to set up standered 80x24 char terminal
    mbox[10] = (font->width * 80);         //FrameBufferInfo.width
    mbox[11] = (font->height * 24);        //FrameBufferInfo.height

    mbox[12] = 0x48009; //set virtual offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;           //FrameBufferInfo.x_offset
    mbox[16] = 0;           //FrameBufferInfo.y.offset

    mbox[17] = 0x48005; //set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32;          //FrameBufferInfo.depth

    mbox[21] = 0x48006; //set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1;           //RGB, not BGR preferably

    mbox[25] = 0x40001; //get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096;        //FrameBufferInfo.pointer
    mbox[29] = 0;           //FrameBufferInfo.size

    mbox[30] = 0x40008; //get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;           //FrameBufferInfo.pitch

    mbox[34] = MBOX_TAG_LAST;

    //this might not return exactly what we asked for, could be
    //the closest supported resolution instead
    if(mbox_call(MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF;   //convert GPU address to ARM address
        width = mbox[5];          //get actual physical width
        height = mbox[6];         //get actual physical height
        pitch = mbox[33];         //get number of bytes per line
        isrgb = mbox[24];         //get the actual channel order
        fb = (void*)((unsigned long)mbox[28]);
    } else {
        uart_puts("Unable to set screen resolution to 80x24 char with 32 depth\n");
    }
}

void fb_scroll() {
    int k;  // NOTE: k has no meaning, it's just random and it's a better letter than i
    for(k = 0; k < 80; k++) {   // clear line 0
        // write space char for first row to get all 0s
        fb_char(k, 0, ' ');
    }

    int i;
    for(i = 1; i <= 24; i++) {   // yes, copy 24 lines, so #23 will be overwritten by nothing & we're counting from 0
        for(k = 0; k < 80; k++) {   // we'll just reuse k
            fb_cpy_char(k, i, k, i - 1);
        }
    }
}

void fb_char(int x, int y, const char s) {
    // get the offset of the glyph. Need to adjust this to support unicode table
    unsigned char *glyph = (unsigned char*)&_binary_font_psf_start + font->headersize + (s * font->bytesperglyph);
    // calculate the offset on screen
    unsigned int offs = ((y * font->width) * 2 * pitch) + ((x * font->height) * 2); // don't know how it works, but it does
    // variables
    unsigned int i, j, line, mask, bytesperline = (font->width + 7) / 8;
    // display a character
    for(j = 0; j < font->height; j++){
        // display one row
        line = offs;
        mask = 1 << (font->width - 1);
        for(i = 0; i < font->width; i++){
            // if bit set, we use white color, otherwise black
            // mmio_write((long long)(fb + line), (int)*glyph & mask ? 0xFFFFFF : 0x0);
            mmio_write((long long)(fb + line), (int)*glyph & mask ? textcolor : bgcolor);
            mask >>= 1;
            line += 4;
        }
        // adjust to next line
        glyph += bytesperline;
        offs += pitch;
    }
}

void fb_cpy_char(int xorig, int yorig, int x, int y) { // main purpose is for fb_scroll
    // calculate the offset on screen
    unsigned int orig_offs = ((yorig * font->width) * 2 * pitch) + ((xorig * font->height) * 2); // don't know how it works, but it does
    unsigned int offs = ((y * font->width) * 2 * pitch) + ((x * font->height) * 2); // don't know how it works, but it does
    // variables
    unsigned int i, j, orig_line, line;
    // display a character
    for(j = 0; j < font->height; j++){
        // display one row
        orig_line = orig_offs;
        line = offs;
        for(i = 0; i < font->width; i++){
            // if bit set, we use white color, otherwise black
            mmio_write((long long)(fb + line), mmio_read((long long)(fb + orig_line)));
            // mmio_write((long long)(fb + line), (int)*glyph & mask ? 0xFFFFFF : 0x0);
            orig_line += 4;
            line += 4;
        }
        // adjust to next line
        orig_offs += pitch;
        offs += pitch;
    }
}