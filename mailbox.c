#include "mmio.h"
#include "mailbox.h"

volatile unsigned int __attribute__((aligned(16)))
mbox[36] = {9 * 4, 0, 0x38002, 12, 8, 2, 3000000, 0, 0};

#define MBOX_BASE (MMIO_BASE + 0xB880)
#define MBOX_READ (MBOX_BASE + 0x0)
#define MBOX_STATUS (MBOX_BASE + 0x18)
#define MBOX_WRITE (MBOX_BASE + 0x20)
#define MBOX_POLL       (MBOX_BASE + 0x10)
#define MBOX_SENDER     (MBOX_BASE + 0x14)
#define MBOX_CONFIG     (MBOX_BASE + 0x1C)

#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

int mbox_call(unsigned char channel)
{
	unsigned int r = (((unsigned long long)(&mbox) & ~0xF) | (channel & 0xF));
	while (mmio_read(MBOX_STATUS) & MBOX_FULL) {
	}
	mmio_write(MBOX_WRITE, r);
	while ((mmio_read(MBOX_STATUS) & MBOX_EMPTY)) {
	}
	if(mmio_read(MBOX_READ) == r) {
		return (mbox[1] == MBOX_RESPONSE);
	}
	return 0;
}
