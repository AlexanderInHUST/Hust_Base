#pragma once

#include "math.h"
#include "stdio.h"

#define EPS 0.00001
#define DEBUG
#define ALL_LOGS

int count_bit_set(unsigned char v);

int most_significant_bit_pos(unsigned char v);

int least_significant_bit_pos(unsigned char v);

bool floatEqual(float a, float b);

bool floatLess(float a, float b);

#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) /** empty**/
#endif

#ifdef ALL_LOGS
#define ALL_LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define ALL_LOG(fmt, ...) /** empty**/
#endif