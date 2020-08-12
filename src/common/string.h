/**
 * Common string utilities.
 */


#ifndef STRING_H
#define STRING_H


#include <stddef.h>


/** These four MUST be present, as required by GCC. */
void *memset(void *dst, unsigned char c, size_t count);
void *memcpy(void *dst, const void *src, size_t count);
void *memmove(void *dst, const void *src, size_t count);
int memcmp(const void *ptr1, const void *ptr2, size_t count);

size_t strlen(const char *str);

size_t strnlen(const char *str, size_t count);
int strncmp(const char *str1, const char *str2, size_t count);
char *strncpy(char *dst, const char *src, size_t count);
char *strncat(char *dst, const char *src, size_t count);


#endif
