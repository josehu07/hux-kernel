/**
 * Common string utilities.
 */


#ifndef STRING_H
#define STRING_H


#include <stddef.h>


void *memset(void *dst, unsigned char c, size_t count);
void *memcpy(void *dst, const void *src, size_t count);

size_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);


#endif
