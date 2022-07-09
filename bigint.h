#ifndef BIGINT_H
#define BIGINT_H

#include <stdint.h>

#define BIG_INT_SIZE 12
#define LSB 0
#define MSB (BIG_INT_SIZE - 1)

struct bigint {
    uint32_t bytes[BIG_INT_SIZE];
};

struct bigint init_bigint(long long num);

int bitwidth_bigint(struct bigint num);

int str_bigint(char *buf, int size, struct bigint num);

struct bigint add_bigint(struct bigint a, struct bigint b);

struct bigint sub_bigint(struct bigint a, struct bigint b);

struct bigint mul_bigint(struct bigint a, struct bigint b); /* TODO */

struct bigint neg_bigint(struct bigint a);

struct bigint not_bigint(struct bigint a);

struct bigint shr_bigint(struct bigint a, int shift);

struct bigint shl_bigint(struct bigint a, int shift);

#endif