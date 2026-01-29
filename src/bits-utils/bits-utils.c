#include "bits-utils.h"
int is_odd(int num) {
    return num & 1;  // 位运算&替代%2，效率更高
}
int is_even(int num) {
    return !(num & 1);
}
// 指定位操作：置1(|)/清零(&~)/翻转(^)，带指针判空、位范围检查
void bit_operate(uint32_t* num, uint8_t pos, BitOpType type) {
    if (num == NULL){
        return; 
    } 
    if (pos >= 32){
        return;  
    }// 32位整数，位范围0-31，避免越界
    uint32_t mask = 1 << pos;  // 生成位掩码（位运算左移）
    switch (type) {
        case BIT_SET:
            *num |= mask;
            break;
        case BIT_CLEAR:
            *num &= ~mask;
            break;
        case BIT_TOGGLE:
            *num ^= mask;
            break;
        default:
            break;
    }
}
// 统计二进制1的个数：经典x&(x-1)算法（消去最右侧的1）
int count_one(uint32_t num) {
    int count = 0;
    while (num) {
        count++;
        num &= num - 1;
    }
    return count;
}
