#include "uart.h"
#include "framebuffer.h"
#include "screen.h"

int mode = 3;
// 0 for no IO, 1 for UART only, 2 for video only, 3 for both

int get_screen_mode() {
	return mode;
}

void set_screen_mode(int new_mode) {
	mode = new_mode;
}

unsigned int x = 0;
unsigned int y = 0;

void set_cursor(unsigned int new_x, unsigned int new_y) {
    x = new_x;
    y = new_y;
}

void screen_del() {
	if(mode == 1 || mode == 3) {
		uart_puts("\b \b");
	}

	if(mode == 2 || mode == 3) {
		fb_char(x - 1, y, ' ');
		set_cursor(x - 1, y);
	}
}

void screen_cls() {
	if(mode == 2 || mode == 3) {
		unsigned int i;
	    unsigned int j;
	    for(i = 0; i < 24; i++) {
	        for(j = 0; j < 80; j++) {
	            fb_char(j, i, ' ');
	        }
	    }
	}
}

void print_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        print_c(n);
    }
}

void print_c(const char s){
	if(!mode) {
		return;
	}

	if(mode == 1 || mode == 3) {
		uart_putc(s);
	}

	if(mode == 2 || mode == 3) {
		if(s == '\r') {
			x = 0;
		} else if(s == '\n') {
			x = 0;
			if(y == 23) {	// we're zero based,
							// so if we're about
							// get go over, do 
							// new line
				fb_scroll();
			} else {
				y++;
			}
		} else {
			fb_char(x, y, s);
			x++;
		}
	}
}

// Print to UART and Screen
void print(const char *s){
	while(*s) {
		print_c(*s);
		s++;
	}
  // unsigned long i;
  // for(i = 0; i < strlen(s); i++) {
  //   print_c(s[i]);
  // }
}