#include "libc.h"

unsigned long strlen(const char *a) {
	int x = 0;
	int i;
	for(i = 0; a[i] != '\0'; i++) {
		x++;
	}
	return x;
}

int strcmp(const char *a, const char *b) {
	if(strlen(a) != strlen(b)) {
		return 1;
	}
	unsigned long i;
	for(i = 0; i < strlen(a); i++) {
		if(a[i] != b[i]) {
			return 1;
		}
	}
	return 0;
}

// inspired from https://code.woboq.org/userspace/glibc/string/strcpy.c.html
char *strcpy(char *a, const char *b) {
	char *res = memcpy(a, b, strlen(b) + 1);
	// *(a + strlen(b)) = 0;
	return res;
}

// inspired from https://code.woboq.org/gcc/libgcc/memcpy.c.html
void *memcpy(void *a, const void *b, unsigned long len) {
	char *aa = a;
	const char *bb = b;
	unsigned long i;
	for(i = 0; i < len; i++) {
		*aa++ = *bb++;
	}
	return a;
}

int pow(int n, int e) {
	int r = 1;
	int i;
	for(i = 0; i < e; i++) {
		r = r * n;
	}
	return r;
}

int atoi(const char *s) {
	unsigned long digits = strlen(s);
	int r = 0;
	unsigned int i;
	for(i = 0; (unsigned long)i < digits; i++) {
		r += (s[i] - '0') * (unsigned int)pow(10, digits - i-1);
	}
	return r;
}

#include "uart.h"

char *itoa(int n) {
	char *r = "";
	float nn = (float)n;
	int i;
	for(i = 10; (int)nn > 0; i *= 10) {	
		int t = (int)nn;
		uart_hex((char)t + '0');
		r[strlen(r)] = (char)t + '0';
		nn /= (float)i;
		uart_putc('\n');
	}
	uart_hex(*r);
	uart_putc('\n');
	uart_hex(*"7");
	uart_putc(*(const char *)r);
	return r;
}