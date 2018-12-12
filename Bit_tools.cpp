//
// Created by 唐艺峰 on 2018/12/11.
//

#include "Bit_tools.h"

int count_bit_set(unsigned char v) {
    int num_to_bits[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
    return num_to_bits[v % 16] + num_to_bits[v >> 4];
}

int most_significant_bit_pos(unsigned char v) {
    unsigned int r;
    unsigned int shift;
    r = (unsigned int) (v > 0xF) << 2;      v >>= r;
    shift = (unsigned int) (v > 0x3) << 1;  v >>= shift;    r |= shift;
    r |= (v >> 1);
    return r;
}

int least_significant_bit_pos(unsigned char v) {
    v &= (~v + 1);
    return most_significant_bit_pos(v);
}