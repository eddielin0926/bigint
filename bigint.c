#include "bigint.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define __binary_digit(x) (sizeof(x) * 8 - __builtin_clz(x))

struct bigint init_bigint(long long num)
{
    struct bigint nr = {.bytes = {0}};
    if (num >> 63)
        nr = not_bigint(nr);
    nr.bytes[LSB] = num;
    nr.bytes[LSB + 1] = num >> 32;
    return nr;
}

int bitwidth_bigint(struct bigint num)
{
    int cnt = 0;
    for (int i = BIG_INT_SIZE - 1; i >= 0; i++)
    {
        if (num.bytes[i])
            return 32 * i + __binary_digit(num.bytes[i]);
    }
    return 0;
}

int str_bigint(char *buf, int size, struct bigint num)
{
    // Double Dabble Algorithm
    if (!buf || size < 2)
        return -1;

    int n = bitwidth_bigint(num);
    int bcd_sz = (n + 4 * ((n + 2) / 3)) / 8;
    int idx = 0;
    uint8_t bcd[bcd_sz];
    printf("%d\n", bcd_sz);

    if (num.bytes[MSB] >> 31)
    {
        if (bcd_sz + 1 > size)
            return -1;
        num = neg_bigint(num);
        buf[idx++] = '-';
    }
    else
    {
        if (bcd_sz > size)
            return -1;
    }

    for (int i = 0; i < bcd_sz; i++)
    {
        bcd[i] = 0;
    }

    for (int i = 0; i < BIG_INT_SIZE * 32; i++)
    {
        for (int j = 0; j < bcd_sz; j++)
        {
            if ((bcd[j] & 0xF) >= 5)
            {
                bcd[j] += 3;
            }
            if ((bcd[j] >> 4) >= 5)
            {
                bcd[j] += 0x30;
            }
        }

        for (int j = bcd_sz - 1; j > 0; j--)
        {
            bcd[j] <<= 1;
            bcd[j] |= ((bcd[j - 1] >> 7) & 1);
        }
        bcd[0] <<= 1;
        bcd[0] |= ((num.bytes[MSB] >> 31) & 1);

        for (int j = BIG_INT_SIZE - 1; j > 0; j--)
        {
            num.bytes[j] <<= 1;
            num.bytes[j] |= ((num.bytes[j - 1] >> 31) & 1);
        }
        num.bytes[0] <<= 1;
    }

    bool lead = true;
    for (int i = bcd_sz - 1; i >= 0; i--)
    {
        if (!lead || (bcd[i] >> 4))
        {
            lead = false;
            buf[idx++] = (bcd[i] >> 4) + '0';
        }
        if (!lead || (bcd[i] & 0xF))
        {
            lead = false;
            buf[idx++] = (bcd[i] & 0xF) + '0';
        }
    }
    if (lead)
    {
        buf[idx++] = '0';
    }
    buf[idx++] = '\0';
    return idx;
}

struct bigint add_bigint(struct bigint a, struct bigint b)
{
    struct bigint result = {.bytes = {0}};
    for (int i = 0; i < BIG_INT_SIZE; i++)
    {
        if (__builtin_add_overflow(result.bytes[i], a.bytes[i], &result.bytes[i]) && (i < (BIG_INT_SIZE - 1)))
        {
            result.bytes[i + 1] += 1;
        }
        if (__builtin_add_overflow(result.bytes[i], b.bytes[i], &result.bytes[i]) && (i < (BIG_INT_SIZE - 1)))
        {
            result.bytes[i + 1] += 1;
        }
    }
    return result;
}

struct bigint sub_bigint(struct bigint a, struct bigint b)
{
    return add_bigint(a, neg_bigint(b));
}

struct bigint mul_bigint(struct bigint a, struct bigint b)
{
    struct bigint result = {.bytes = {0}};
    struct bigint intmd[BIG_INT_SIZE] = {{.bytes = {0}}};

    for (int i = 0; i < BIG_INT_SIZE; i++)
    {
        for (int j = 0; j < BIG_INT_SIZE; j++)
        {
            uint64_t res = (uint64_t)a.bytes[j] * b.bytes[i] + intmd[i].bytes[j];
            intmd[i].bytes[j] = (uint32_t)res;
            if (j < BIG_INT_SIZE - 1)
            {
                intmd[i].bytes[j + 1] = res >> 32;
            }
        }
    }
    for (int i = 0; i < BIG_INT_SIZE; i++)
    {
        result = add_bigint(result, shl_bigint(intmd[i], i * 32));
    }
    return result;
}

struct bigint neg_bigint(struct bigint a)
{
    struct bigint one = {.bytes = {1}};
    return add_bigint(not_bigint(a), one);
}

struct bigint not_bigint(struct bigint a)
{
    for (int i = 0; i < BIG_INT_SIZE; i++)
    {
        a.bytes[i] = ~a.bytes[i];
    }
    return a;
}

struct bigint shr_bigint(struct bigint a, int shift)
{
    if (shift == 0)
    {
        return a;
    }
    if (shift < 0)
    {
        return shl_bigint(a, -shift);
    }

    int shift32 = shift / 32;
    for (int i = BIG_INT_SIZE - 1; i >= shift32; i--)
    {
        a.bytes[i - shift32] = a.bytes[i];
    }
    for (int i = BIG_INT_SIZE - 1; i >= BIG_INT_SIZE - 1 - shift32; i--)
    {
        a.bytes[i] = 0;
    }

    int shiftbit = shift % 32;
    if (!shiftbit)
        return a;
    for (int i = 0; i < BIG_INT_SIZE; i++)
    {
        a.bytes[i] >>= shiftbit;
        if (i < BIG_INT_SIZE - 1)
        {
            a.bytes[i] |= a.bytes[i + 1] << (32 - shiftbit);
        }
    }
    return a;
}

struct bigint shl_bigint(struct bigint a, int shift)
{
    if (shift == 0)
    {
        return a;
    }
    if (shift < 0)
    {
        return shr_bigint(a, -shift);
    }

    int shift32 = shift / 32;
    for (int i = BIG_INT_SIZE - 1; i >= shift32; i--)
    {
        a.bytes[i] = a.bytes[i - shift32];
    }
    for (int i = 0; i < shift32; i++)
    {
        a.bytes[i] = 0;
    }

    int shiftbit = shift % 32;
    if (!shiftbit)
        return a;
    for (int i = BIG_INT_SIZE - 1; i >= 0; i--)
    {
        a.bytes[i] <<= shiftbit;
        if (i > 0)
        {
            a.bytes[i] |= (a.bytes[i - 1] >> (32 - shiftbit));
        }
    }
    return a;
}

struct bigint fib_rc(long long k)
{
    struct bigint f[3] = {init_bigint(0), init_bigint(1), init_bigint(1)};

    if (k < 2)
        return f[k];

    for (int i = 2; i <= k; i++)
    {
        f[2] = add_bigint(f[1], f[0]);
        f[0] = f[1];
        f[1] = f[2];
    }
    return f[2];
}

struct bigint fib_fd(long long k)
{
    struct bigint a = init_bigint(0);
    struct bigint b = init_bigint(1);

    for (int i = __binary_digit(k); i > 0; i--)
    {
        struct bigint t1 = mul_bigint(a, sub_bigint(mul_bigint(init_bigint(2), b), a));
        struct bigint t2 = add_bigint(mul_bigint(b, b), mul_bigint(a, a));
        a = t1;
        b = t2;
        if (k & ((1LL << i) >> 1))
        {
            t1 = add_bigint(a, b);
            a = b;
            b = t1;
        }
    }
    return a;
}

int main(int argc, char const *argv[])
{
    char buf[10] = {'\0'};
    str_bigint(buf, 10, fib_rc(300));
    printf("fib(%d) = %s\n", 300, buf);
    return 0;
}
