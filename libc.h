#ifndef LIBC_H
#define LIBC_H

unsigned long strlen(const char *a);
int strcmp(const char *a, const char *b);

char *strcpy(char *a, const char *b);
void *memcpy(void *a, const void *b, unsigned long len);
int pow(int n, int e);
int atoi(const char *s);
char *itoa(int n);

#endif