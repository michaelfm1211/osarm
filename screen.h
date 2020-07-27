#ifndef SCREEN_H
#define SCREEN_H

unsigned int x;
unsigned int y;

int get_screen_mode();
void set_screen_mode(int new_mode);
void set_cursor(unsigned int new_x, unsigned int new_y);
void screen_del();
void screen_cls();
void print_hex(unsigned int d);
void print_c(const char s);
void print(const char *s);

#endif