/*
 * Copyright (c) 2016 Sean Parkinson (sparkinson@iprimus.com.au)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "share_meth.h"

#define NUM_ELEMS	3
#define NUM_BYTES	17
#define MOD_WORD	0x19

#define U128(w)		((__uint128_t)w)

/**
 * Copy the data of the number object into the result number object.
 *
 * @param [in] r  The result number object.
 * @param [in] a  The number object to copy.
 */
static void p128_copy(uint64_t *r, uint64_t *a)
{
    r[0] = a[0];
    r[1] = a[1];
    r[2] = a[2];
}

/**
 * Set the number object to be one word value - w.
 *
 * @param [in] a  The number object set.
 * @param [in] w  The word sized value to set.
 */
static void p128_set_word(uint64_t *a, uint64_t w)
{
    a[0] = w;
    a[1] = 0;
    a[2] = 0;
}
/**
 * Multiply by prime's (mod's) last word.
 *
 * @param [in] a  The number to multiply.
 * @return  The multiplicative result.
 */
#define MUL_MOD_WORD(a) \
    (((a) << 4) + ((a) << 3) + (a))

/**
 * Perform modulo operation on number, a, up to 16-bits longer than the prime
 * and put result in r.
 *
 * @param [in] r  The result of the reduction.
 * @param [in] a  The number to operate on.
 */
static void p128_mod_small(uint64_t *r, uint64_t *a)
{
    __int128_t t;

    t = (a[2] >> 1) * MOD_WORD; a[2] &= 0x1;
    t += a[0]; r[0] = t; t >>= 64;
    t += a[1]; r[1] = t; t >>= 64;
    t += a[2]; r[2] = t;
}

/**
 * Perform modulo operation on a product result in 128-bit elements.
 *
 * @param [in] r  The number reduce number.
 * @param [in] a  The product result in 128-bit elements.
 */
static void p128_mod_long(uint64_t *r, __uint128_t *a)
{
    __uint128_t t;

    t = (a[2] >> 1) + ((uint64_t)a[3] << 63); a[0] += MUL_MOD_WORD(t);
    t = (a[3] >> 1) + ((uint64_t)a[4] << 63); a[1] += MUL_MOD_WORD(t);

    r[0] = a[0]; a[1] += a[0] >> 64;
    r[1] = a[1];
    r[2] = (a[2] & 1) + (a[1] >> 64);

    p128_mod_small(r, r);
}

/**
 * Add two numbers, a and b, (modulo prime) and put the result in a third, r.
 *
 * @param [in] r  The result of the addition.
 * @param [in] a  The first operand.
 * @param [in] b  The second operand.
 */
static void p128_mod_add(uint64_t *r, uint64_t *a, uint64_t *b)
{
    __int128_t t;

    t  = a[0]; t += b[0]; r[0] = t; t >>= 64;
    t += a[1]; t += b[1]; r[1] = t; t >>= 64;
    t += a[2]; t += b[2]; r[2] = t;

    p128_mod_small(r, r);
}

/** Prime element 0. */
#define P128_0	0xffffffffffffffe7
/** Prime element 1. */
#define P128_1	0xffffffffffffffff
/** Prime element 2. */
#define P128_2	0x1

/**
 * Subtract b from a (modulo prime) and put the result r.
 *
 * @param [in] r  The result of the subtraction.
 * @param [in] a  The first operand.
 * @param [in] b  The second operand.
 */
static void p128_mod_sub(uint64_t *r, uint64_t *a, uint64_t *b)
{
    __uint128_t t = 0;
    t += P128_0; t += a[0]; t -= b[0]; r[0] = t; t >>= 64;
    t += P128_1; t += a[1]; t -= b[1]; r[1] = t; t >>= 64;
    t += P128_2; t += a[2]; t -= b[2]; r[2] = t;

    p128_mod_small(r, r);
}

/**
 * Square the number, a, modulo the prime and put in result in r.
 *
 * @param [in] r  The result of the squaring.
 * @param [in] a  The number object to square.
 */
static void p128_mod_sqr(uint64_t *r, uint64_t *a)
{
    uint64_t p64;
    __uint128_t p128;
    __uint128_t t[5];

    t[0] = 0; t[1] = 0; t[2] = 0; t[3] = 0; t[4] = 0;

    p128 = U128(a[0]) * a[0];
    t[0] += (uint64_t)p128;
    t[1] += p128 >> 64;
    p128 = U128(a[0]) * a[1];
    t[1] += (uint64_t)p128;
    t[2] += p128 >> 64;
    t[1] += (uint64_t)p128;
    t[2] += p128 >> 64;
    p64 = a[0] & (0 - a[2]);
    t[2] += p64;
    t[2] += p64;
    p128 = U128(a[1]) * a[1];
    t[2] += (uint64_t)p128;
    t[3] += p128 >> 64;
    p64 = a[1] & (0 - a[2]);
    t[3] += p64;
    t[3] += p64;
    p64 = a[2];
    t[4] += p64;

    p128_mod_long(r, t);
}

/**
 * Square the number, a, modulo the prime n times and put in result in r.
 *
 * @param [in] r  The result of the squaring.
 * @param [in] a  The number object to square.
 * @param [in] n  The number of times to square.
 */
static void p128_mod_sqr_n(uint64_t *r, uint64_t *a, uint16_t n)
{
    uint16_t i;

    p128_mod_sqr(r, a);
    for (i=1; i<n; i++)
        p128_mod_sqr(r, r);
}

/**
 * Multiply two numbers, a and b, modulo the prime amd put in result in r.
 *
 * @param [in] r  The result of the multiplication.
 * @param [in] a  The first operand number object.
 * @param [in] b  The first operand number object.
 */
static void p128_mod_mul(uint64_t *r, uint64_t *a, uint64_t *b)
{
    uint64_t p64;
    __uint128_t p128;
    __uint128_t t[5];

    t[0] = 0; t[1] = 0; t[2] = 0; t[3] = 0; t[4] = 0;

    p128 = U128(a[0]) * b[0];
    t[0] += (uint64_t)p128;
    t[1] += p128 >> 64;
    p128 = U128(a[0]) * b[1];
    t[1] += (uint64_t)p128;
    t[2] += p128 >> 64;
    p128 = U128(a[1]) * b[0];
    t[1] += (uint64_t)p128;
    t[2] += p128 >> 64;
    p64 = a[0] & (0 - b[2]);
    t[2] += p64;
    p128 = U128(a[1]) * b[1];
    t[2] += (uint64_t)p128;
    t[3] += p128 >> 64;
    p64 = b[0] & (0 - a[2]);
    t[2] += p64;
    p64 = a[1] & (0 - b[2]);
    t[3] += p64;
    p64 = b[1] & (0 - a[2]);
    t[3] += p64;
    p64 = a[2] & b[2];
    t[4] += p64;

    p128_mod_long(r, t);
}

/**
 * Reduce the number that is less than 2 times the prime modulo the prime.
 *
 * @param [in] r  The result of the reduction.
 * @param [in] a  The number to reduce.
 */
static void p128_mod(uint64_t *r,uint64_t *a)
{
    uint64_t c;
    __int128_t t;

    c = (a[2] == 0x1) & (a[1] == 0xffffffffffffffff) & (a[0] >= 0xffffffffffffffe7);
    t = c * MOD_WORD;
    t += a[0]; r[0] = t; t >>= 64;
    t += a[1]; r[1] = t; t >>= 64;
    t += a[2]; r[2] = t & 0x1;
}

/**
 * Calculate the inverse of a modulo the prime and store thre result in r.
 *
 * @param [in] r  The result of the inversion.
 * @param [in] a  The number to invert.
 */
static void p128_mod_inv(uint64_t *r, uint64_t *a)
{
    uint64_t t[NUM_ELEMS];
    uint64_t t2[NUM_ELEMS];
    uint64_t t3[NUM_ELEMS];
    uint64_t t5[NUM_ELEMS];

    p128_mod_sqr(t2, a);
    p128_mod_sqr(t, t2); p128_mod_mul(t5, a, t);
    				p128_mod_mul(t, t2, a);		/* 2 */
    p128_mod_sqr_n(t, t, 1);	p128_mod_mul(t, t, a);		/* 3 */
    p128_mod_sqr_n(t2, t, 3);	p128_mod_mul(t3, t2, t);	/* 6 */
    p128_mod_sqr_n(t2, t3, 6);	p128_mod_mul(t3, t2, t3);	/* 12 */
    p128_mod_sqr_n(t2, t3, 3);	p128_mod_mul(t, t2, t);		/* 15 */
    p128_mod_sqr_n(t2, t, 15);	p128_mod_mul(t, t2, t);		/* 30 */
    p128_mod_sqr(t2, t);	p128_mod_mul(t, t2, a);		/* 31 */
    p128_mod_sqr_n(t2, t, 31);	p128_mod_mul(t, t2, t);		/* 62 */
    p128_mod_sqr_n(t2, t, 62);	p128_mod_mul(t, t2, t);		/* 124 */
    p128_mod_sqr_n(t, t, 5);
    p128_mod_mul(r, t, t5);
}

/**
 * Create a new number object.
 *
 * @param [in]  len  The length of the secret in bytes.
 * @param [out] num  The new number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_p128_num_new(uint16_t len, void **num)
{
    SHARE_ERR err = NONE;

    len = len;

    *num = malloc(NUM_ELEMS*sizeof(uint64_t));
    if (*num == NULL)
        err = ALLOC;

    return err;
}

/**
 * Free the dynamic memory associated with the number object.
 *
 * @param [in] num  The number object.
 */
void share_p128_num_free(void *num)
{
    if (num != NULL) free(num);
}

/**
 * Encode the number object into data.
 * The data is assumed to be big-endian bytes.
 *
 * @param [in] num   The number object.
 * @param [in] data  The data to hold the encoding.
 * @param [in] len   The number of bytes that data can hold.
 * @return  PARAM_BAD_LEN when encoding is too long for data.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_p128_num_from_bin(const uint8_t *data, uint16_t len,
    void *num)
{
    SHARE_ERR err = NONE;
    int8_t i, j;
    uint64_t *n = num;

    if (len > NUM_BYTES)
    {
        err = PARAM_BAD_LEN;
        goto end;
    }

    for (i=0; i<NUM_ELEMS; i++)
        n[i] = 0;
    for (i=len-1,j=0; i>=0; i--,j++)
        n[j/8] |= ((uint64_t)data[i]) << ((j & 7) * 8);

end:
    return err;
}

/**
 * Encode the number object into data.
 * The data is assumed to be big-endian bytes.
 *
 * @param [in] num   The number object.
 * @param [in] data  The data to hold the encoding.
 * @param [in] len   The number of bytes that data can hold.
 * @return  PARAM_BAD_LEN when encoding is too long for data.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_p128_num_to_bin(void *num, uint8_t *data, uint16_t len)
{
    SHARE_ERR err = NONE;
    int8_t i, j;
    uint64_t *n = num;

    if (len < NUM_BYTES)
    {
        err = PARAM_BAD_LEN;
        goto end;
    }

    for (i=NUM_BYTES-1,j=0; i>=NUM_BYTES-len; i--,j++)
        data[i] = n[j/8] >> ((j & 7) * 8);
    for (; i>=0; i--)
        data[i] = 0;

end:
    return err;
}

/**
 * Calculate the y value of a split.
 * y = x^0.a[0] + x^1.a[1] + ... + x^(parts-1).a[parts-1]
 *
 * @param [in] prime  The prime as a number object. 
 * @param [in] parts  The number of parts that are required to recalcuate
 *                    secret. 
 * @param [in] a      The array of coefficients.
 * @param [in] x      The x value as a number object.
 * @param [in] y      The y value as a number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_p128_split(void *prime, uint8_t parts, void **a, void *x,
    void *y)
{
    SHARE_ERR err = NONE;
    uint8_t i;
    uint64_t t[NUM_ELEMS], m[NUM_ELEMS];
    uint64_t **ad = (uint64_t **)a;
    uint64_t *xd = x;
    uint64_t *yd = y;

    prime = prime;

    /* y = x^0.a[0] + x^1.a[1] - minimum of two parts. */
    p128_mod_mul(t, ad[1], xd);
    p128_mod_add(yd, ad[0], t);

    p128_copy(m, xd);
    for (i=2; i<parts; i++)
    {
        /* y += x^i.a[i] (m = x^i) */
        p128_mod_mul(m, m, xd);
        p128_mod_mul(t, ad[i], m);
        p128_mod_add(yd, yd, t);
    }
    p128_mod(yd, yd);

    return err;
}

/**
 * Calculate the secret from splits.
 * secret = sum of (i=0..parts-1) y[i] *
 *          product of (j=0..parts-1) x[j] / (x[j] - x[i]) where j != i
 *
 * @param [in] prime   The prime as a number object. 
 * @param [in] parts   The number of parts that are required to recalcuate
 *                     secret. 
 * @param [in] x       The array of x values as number objects.
 * @param [in] y       The array of y values as number objects.
 * @param [in] secret  The calculated secret as a number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_p128_join(void *prime, uint8_t parts, void **x, void **y,
    void *secret)
{
    SHARE_ERR err = NONE;
    uint8_t i, j;
    uint64_t np[NUM_ELEMS], t[NUM_ELEMS];
    uint64_t **xd = (uint64_t **)x;
    uint64_t **yd = (uint64_t **)y;
    uint64_t *sd = secret;
    uint64_t *nr, *n, *dr, *d;

    prime = prime;

    /* Arrays of numerators and denominators as number objects. */
    nr = malloc(NUM_ELEMS * parts * sizeof(uint64_t));
    dr = malloc(NUM_ELEMS * parts * sizeof(uint64_t));
    if ((nr == NULL) || (dr == NULL))
    {
        err = ALLOC;
        goto end;
    }

    /* np = x[0] * x[1] * .. * x[parts-1] */
    p128_copy(np, xd[0]);
    for (i=1; i<parts; i++)
        p128_mod_mul(np, np, x[i]);

    /* Calculate all the denominators. */
    for (i=0; i<parts; i++)
    {
        /* d[i] = x[i] * (product of all x[j] - x[i] where i != j). */
        n = &nr[i*NUM_ELEMS];
        d = &dr[i*NUM_ELEMS];
        p128_set_word(d, 1);
        for (j=0; j<parts; j++)
        {
            if (i == j)
                continue;

            p128_mod_sub(t, xd[j], xd[i]);
            p128_mod_mul(d, d, t);
        }
        p128_mod_mul(d, d, xd[i]);

        /* n[i] = y[i].np (as x[i] is multiplied into denominator) */
        p128_mod_mul(n, np, yd[i]);
    }

    /* Convert numerators to common denominator and sum. */
    for (i=0; i<parts; i++)
    {
        n = &nr[i*NUM_ELEMS];
        for (j=0; j<parts; j++)
        {
            if (i == j)
                continue;
            d = &dr[j*NUM_ELEMS];
            p128_mod_mul(n, n, d);
        }
        if (i > 0)
            p128_mod_add(nr, nr, n);
    }
    /* Common denominator is product of all denominators. */
    for (i=1; i<parts; i++)
        p128_mod_mul(dr, dr, &dr[i*NUM_ELEMS]);

    /* secret = inverse denominator * sum of numerators. */
    p128_mod_inv(t, dr);
    p128_mod_mul(sd, t, nr);
    p128_mod(sd, sd);

end:
    if (dr != NULL) free(dr);
    if (nr != NULL) free(nr);
    return err;
}

