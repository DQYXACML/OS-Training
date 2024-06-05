#ifndef KLIB_H
#define KLIB_H
#include "comm/types.h"
#include <stdarg.h>

// 向上对齐到页边界
static inline uint32_t up2(uint32_t size, uint32_t bound)
{
    return (size + bound - 1) & ~(bound - 1);
}

// 向下对齐到界边界
// size = 0x1010 bound = 0x1000
// bound - 1 = 0x0FFF
// ~(bound-1) = 0xFFFF1000
// size & ~(bound-1) =  0x1010 & 0xFFFF1000 = 0x1000
static inline uint32_t down2(uint32_t size, uint32_t bound)
{
    return size & ~(bound - 1);
}

void kernel_strcpy(char *dest, const char *src);
void kernel_strncpy(char *dest, const char *src, int size);
int kernel_strncmp(const char *s1, const char *s2, int size);
int kernel_strlen(const char *str);

void kernel_memcpy(void *dest, void *src, int size);
void kernel_memset(void *dest, uint8_t v, int size);
int kernel_memcmp(void *d1, void *d2, int size);

void kernel_itoa(char *buf, int num, int base);
void kernel_vsprintf(char *buf, const char *fmt, va_list args);

#ifndef RELEASE
#define ASSERT(condition) \
    if (!(condition))     \
    panic(__FILE__, __LINE__, __func__, #condition)
void panic(const char *file, int line, const char *func, const char *cond);
#else
#define ASSERT(condition) ((void)0)
#endif

#endif