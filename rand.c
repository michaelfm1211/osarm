#include "mmio.h"
#include "rand.h"

#define RNG_CTRL        (MMIO_BASE+0x00104000)
// #define RNG_STATUS      ((volatile unsigned int*)(MMIO_BASE+0x00104004))
#define RNG_STATUS        (MMIO_BASE+0x00104004)
#define RNG_DATA        (MMIO_BASE+0x00104008)
#define RNG_INT_MASK    (MMIO_BASE+0x00104010)

void rand_init() {
	mmio_write(RNG_STATUS, 0x40000);
	// Mask interrupt
	mmio_write(RNG_INT_MASK, mmio_read(RNG_INT_MASK) | 1);
	// enable
	mmio_write(RNG_CTRL, mmio_read(RNG_CTRL) | 1);
	// wait to gain entropy
	while(!((*(volatile unsigned int*)RNG_STATUS)>>24)) {
		asm volatile("nop");
	}
}

unsigned int rand(unsigned int min, unsigned int max) {
	return mmio_read(RNG_DATA) % (max - min) + min;
}