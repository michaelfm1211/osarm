#include <stdint.h>
#include "mmio.h"

void mmio_write(long long reg, int data) {
  *(volatile int *)reg = data;
}

uint32_t mmio_read(long long reg) {
	return *(volatile int *)reg;
}
