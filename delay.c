#include "mmio.h"
#include "delay.h"

#define SYSTMR_LO        (MMIO_BASE+0x00003004)
#define SYSTMR_HI        (MMIO_BASE+0x00003008)

void wait_cycles(unsigned int n)
{
	if(!n) {
		return;
	}
    while(n--) {
		asm volatile("nop");
	}
}

void wait_msec(unsigned int n)
{
    register unsigned long f, t, r = 0;
    // get the current counter frequency
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    // read the current counter
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    // calculate expire value for counter
    t += ( (f / 1000) * n ) / 1000;
    while (r < t) {
    	asm volatile ("mrs %0, cntpct_el0" : "=r"(r));
    }
}

unsigned long get_system_timer()
{
    unsigned int h=-1, l;
    // we must read MMIO area as two separate 32 bit reads
    h = mmio_read(SYSTMR_HI);
    l = mmio_read(SYSTMR_LO);
    // we have to repeat it if high word changed during read
    if(h != mmio_read(SYSTMR_HI)) {
        h = mmio_read(SYSTMR_HI);
        l = mmio_read(SYSTMR_LO);
    }
    // compose long int value
    return ((unsigned long) h << 32) | l;
}

void wait_msec_st(unsigned int n)
{
    unsigned long t = get_system_timer();
    // we must check if it's non-zero, because qemu does not emulate
    // system timer, and returning constant zero would mean infinite loop
    if(t) {
    	while(get_system_timer() < t + n) {
		}
    }
}