#pragma once
#include <stddef.h>
#include <stdint.h>
typedef enum {
    BIT_SET,    // 置1指定位
    BIT_CLEAR,  // 清零指定位
    BIT_TOGGLE  // 翻转指定位
} BitOpType;
int is_odd(int num);                                           // 判断奇数
int is_even(int num);                                          // 判断偶数
void bit_operate(uint32_t* num, uint8_t pos, BitOpType type);  // 指定位操作
int count_one(uint32_t num);  // 统计二进制1的个数
