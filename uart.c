#include "mmio.h"
#include "mailbox.h"
#include "delay.h"
#include "uart.h"

void uart_init() {
  mmio_write(UART0_CR, 0x00000000);
  mmio_write(GPPUD, 0x00000000);
  wait_cycles(150);
  mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
  wait_cycles(150);
  mmio_write(GPPUDCLK0, 0x00000000);
  mmio_write(UART0_ICR, 0x7FF);

  // FOLLOWING IS NESSECARY FOR RPI 3 OR HIGHER
  mbox[0] = 9 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_SETCLKRATE; // set clock rate
  mbox[3] = 12;
  mbox[4] = 8;
  mbox[5] = 2;           // UART clock
  mbox[6] = 4000000;     // 4Mhz
  mbox[7] = 0;           // clear turbo
  mbox[8] = MBOX_TAG_LAST;
  mbox_call(MBOX_CH_PROP);
  // PREVIOUS IS NESSECARY FOR RPI 3 OR HIGHER

  mmio_write(UART0_IBRD, 1);
  mmio_write(UART0_FBRD, 40);

  mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));
  mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
  mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(unsigned char c) {
  while (mmio_read(UART0_FR) & (1 << 5)) {
  }
  mmio_write(UART0_DR, c);
}

unsigned char uart_getc() {
  while (mmio_read(UART0_FR) & (1 << 4)) {
  }
  return mmio_read(UART0_DR);
}

void uart_puts(const char *str) {
  int i;
  for (i = 0; str[i] != '\0'; i++) {
    uart_putc((unsigned char)str[i]);
  };
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_putc(n);
    }
}

void uart_dump(void *ptr) {
    unsigned long a, b, d;
    unsigned char c;
    for(a = (unsigned long)ptr; a < (unsigned long)ptr + 512; a += 16) {
        uart_hex(a);
        uart_puts(": ");
        for(b = 0; b < 16; b++) {
            c = mmio_read((a + b));

            d = (unsigned int)c;
            d >>= 4;
            d &= 0xF;
            d += d > 9 ? 0x37 : 0x30;
            uart_putc(d);

            d = (unsigned int)c;
            d &= 0xF;
            d += d > 9 ? 0x37 : 0x30;
            uart_putc(d);

            uart_putc(' ');
            if(b%4==3)
                uart_putc(' ');
        }
        for(b = 0; b < 16; b++) {
            c = mmio_read((a + b));
            uart_putc(c < 32 || c >= 127 ? '.' : c);
        }
        uart_putc('\r');
        uart_putc('\n');
    }
}