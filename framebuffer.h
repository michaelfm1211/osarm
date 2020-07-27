#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

void fb_init();
void fb_char(int x, int y, const char s);
void fb_cpy_char(int xorig, int yorig, int x, int y);
void fb_scroll();

#endif