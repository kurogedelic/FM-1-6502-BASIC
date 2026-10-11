/* Freestanding shim: the FM-1 firmware build has no libc headers on its include path.
 * The functions are provided by felucca-bsp's chip/libc.c and firmware/fm1_basic.c. */
#ifndef FM1_FW_STRING_H
#define FM1_FW_STRING_H
#include <stddef.h>
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
int memcmp(const void *a, const void *b, size_t n);
#endif
