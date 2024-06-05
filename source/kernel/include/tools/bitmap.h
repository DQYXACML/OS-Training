#ifndef BITMAP_H
#define BITMAP_H

#include "comm/types.h"

/**
 * @brief 位图数据结构
 */
typedef struct _bitmap_t
{
    int bit_count; // 位的数据
    uint8_t *bits; // 位空间
} bitmap_t;

void bitmap_init(bitmap_t *bitmap, uint8_t *bits, int count, int init_bit);
int bitmap_byte_count(int bit_count);
int bitmap_get_bit(bitmap_t *bitmap, int index);                      // 获取某一页的状态
void bitmap_set_bit(bitmap_t *bitmap, int index, int count, int bit); // 设置某一页的状态
int bitmap_is_set(bitmap_t *bitmap, int index);                       // 判断某一页是0还是1
int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count);         // 分配多个页，找位图中连续多个位取反

#endif // BITMAP_H