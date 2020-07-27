#include "mmio.h"
#include "mailbox.h"
#include "delay.h"
#include "screen.h"
#include "power.h"

#define POWER_RSTC 					(MMIO_BASE+0x0010001c)
#define POWER_RSTS 					(MMIO_BASE+0x00100020)
#define POWER_WATCHDOG 				(MMIO_BASE+0x00100024)
#define POWER_WATCHDOG_MAGIC 		0x5a000000
#define POWER_RSTC_FULLREBOOT 		0x0000002

/**
 * Shutdown the board
 */
void power_off()
{
    unsigned long r;

    // power off devices one by one
    for(r = 0; r < 16; r++) {
        mbox[0] = 8 * 4;
        mbox[1] = MBOX_REQUEST;
        mbox[2] = MBOX_TAG_SETPOWER; // set power state
        mbox[3] = 8;
        mbox[4] = 8;
        mbox[5] = (unsigned int)r;   // device id
        mbox[6] = 0;                 // bit 0: off, bit 1: no wait
        mbox[7] = MBOX_TAG_LAST;
        mbox_call(MBOX_CH_PROP);
    }

    // power off gpio pins (but not VCC pins)
    mmio_write(GPFSEL0, 0);
    mmio_write(GPFSEL1, 0);
    mmio_write(GPFSEL2, 0);
    mmio_write(GPFSEL3, 0);
    mmio_write(GPFSEL4, 0);
    mmio_write(GPFSEL5, 0);
    mmio_write(GPPUD, 0);
    wait_cycles(150);
    mmio_write(GPPUDCLK0, 0xffffffff);
    mmio_write(GPPUDCLK1, 0xffffffff);
    wait_cycles(150);
    mmio_write(GPPUDCLK0, 0);
    mmio_write(GPPUDCLK1, 0);	// flush GPIO setup

    // power off the SoC (GPU + CPU)
    r = mmio_read(POWER_RSTS);
    r &= ~0xfffffaaa;
    r |= 0x555;    // partition 63 used to indicate halt
    mmio_write(POWER_RSTS, POWER_WATCHDOG_MAGIC | r);
    mmio_write(POWER_WATCHDOG, POWER_WATCHDOG_MAGIC | 10);
    mmio_write(POWER_RSTC, POWER_WATCHDOG_MAGIC | POWER_RSTC_FULLREBOOT);
}

void reboot()
{
    int i = 3;
    while(i) {
        print("\rReboot in ");
        print_c((char) i + '0');
        i--;
        wait_msec(1000000);
    }
    unsigned int r;
    // trigger a restart by instructing the GPU to boot from partition 0
    r = mmio_read(POWER_RSTS);
    r &= ~0xfffffaaa;
    mmio_write(POWER_RSTS, POWER_WATCHDOG_MAGIC | r);	// boot from partition 0
    mmio_write(POWER_WATCHDOG, POWER_WATCHDOG_MAGIC | 10);
    mmio_write(POWER_RSTC, POWER_WATCHDOG_MAGIC | POWER_RSTC_FULLREBOOT);
}
