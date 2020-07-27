#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

#define MMIO_BASE 0x3F000000
#define GPIO_BASE (MMIO_BASE + 0x200000)

#define GPFSEL0         (MMIO_BASE+0x00200000)
#define GPFSEL1         (MMIO_BASE+0x00200004)
#define GPFSEL2         (MMIO_BASE+0x00200008)
#define GPFSEL3         (MMIO_BASE+0x0020000C)
#define GPFSEL4         (MMIO_BASE+0x00200010)
#define GPFSEL5         (MMIO_BASE+0x00200014)
#define GPSET0          (MMIO_BASE+0x0020001C)
#define GPSET1          (MMIO_BASE+0x00200020)
#define GPCLR0          (MMIO_BASE+0x00200028)
#define GPLEV0          (MMIO_BASE+0x00200034)
#define GPLEV1          (MMIO_BASE+0x00200038)
#define GPEDS0          (MMIO_BASE+0x00200040)
#define GPEDS1          (MMIO_BASE+0x00200044)
#define GPHEN0          (MMIO_BASE+0x00200064)
#define GPHEN1          (MMIO_BASE+0x00200068)
#define GPPUD           (MMIO_BASE+0x00200094)
#define GPPUDCLK0       (MMIO_BASE+0x00200098)
#define GPPUDCLK1       (MMIO_BASE+0x0020009C)

void mmio_write(long long reg, int data);
uint32_t mmio_read(long long reg);

#endif
