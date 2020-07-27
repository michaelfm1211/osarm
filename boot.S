// AArch64

.section .text.boot

.globl _start

_start:
	// Put stack before code
	ldr x5, =_start
	mov sp, x5

	// clear bss
	ldr x5, =__bss_start
	ldr w6, =__bss_size	// w6 is upper 32 bits of x5
3:	cbz w6, 4f	// cbz is compare and branch on zero
	str xzr, [x5], #8	// xzr is synonymous to 0
	sub w6, w6, #1	// w6--
	cbnz w6, 3b	// cbnz is compare and branch on non zero
4:	bl kernel_main
