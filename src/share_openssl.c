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
 *
 */

#include <string.h>
#include "share_meth.h"
#include "openssl/bn.h"

/**
 * Create a new number object.
 *
 * @param [in]  len  The length of the secret in bytes.
 * @param [out] num  The new number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_openssl_num_new(uint16_t len, void **num)
{
    SHARE_ERR err = NONE;

    len = len;

    *num = BN_new();
    if (*num == NULL)
        err = ALLOC;

    return err;
}

/**
 * Free the dynamic memory associated with the number object.
 *
 * @param [in] num  The number object.
 */
void share_openssl_num_free(void *num)
{
    BN_free(num);
}

/**
 * Decode the data into the number object.
 * The data is assumed to be big-endian bytes.
 *
 * @param [in] data  The data to be decoded.
 * @param [in] len   The length of the data to be decoded.
 * @param [in] num   The number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_openssl_num_from_bin(const uint8_t *data, uint16_t len,
    void *num)
{
    SHARE_ERR err = NONE;

    if (BN_bin2bn(data, len, num) == NULL)
        err = ALLOC;

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
SHARE_ERR share_openssl_num_to_bin(void *num, uint8_t *data, uint16_t len)
{
    SHARE_ERR err = NONE;

    if (BN_bn2binpad(num, data, len) == -1)
        err = PARAM_BAD_LEN;

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
SHARE_ERR share_openssl_split(void *prime, uint8_t parts, void **a, void *x,
    void *y)
{
    SHARE_ERR err = ALLOC;
    int ret = 1;
    int i;
    BN_CTX *ctx;
    BIGNUM *t, *m;

    ctx = BN_CTX_new();
    t = BN_new();
    m = BN_new();
    if ((ctx == NULL) || (t == NULL) || (m == NULL))
        goto end;

    /* y = x^0.a[0] + x^1.a[1] - minimum of two parts. */
    ret &= BN_mod_mul(t, a[1], x, prime, ctx);
    ret &= BN_add(y, a[0], t);
    if (BN_cmp(y, prime) >= 0)
        ret &= BN_sub(y, y, prime);

    ret &= (BN_copy(m, x) != NULL);
    for (i=2; i<parts; i++)
    {
        /* y += x^i.a[i] (m = x^i) */
        ret &= BN_mod_mul(m, m, x, prime, ctx);
        ret &= BN_mod_mul(t, a[i], m, prime, ctx);
        ret &= BN_add(y, y, t);
        if (BN_cmp(y, prime) >= 0)
            ret &= BN_sub(y, y, prime);
    }

    /* No error if all operations succeeded. */
    if (ret == 1)
        err = NONE;
end:
    BN_free(m);
    BN_free(t);
    BN_CTX_free(ctx);
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
SHARE_ERR share_openssl_join(void *prime, uint8_t parts, void **x, void **y,
    void *secret)
{
    SHARE_ERR err = ALLOC;
    int ret = 1;
    int i, j;
    BN_CTX *ctx;
    BIGNUM *np, *t;
    BIGNUM **n = NULL, **d = NULL;

    ctx = BN_CTX_new();
    np = BN_new();
    t = BN_new();
    if ((ctx == NULL) || (np == NULL) || (t == NULL))
        goto end;

    /* Arrays of numerators and denominators as number objects. */
    n = malloc(parts * sizeof(*n));
    d = malloc(parts * sizeof(*d));
    if ((n == NULL) || (d == NULL))
        goto end;
    for (i=0; i<parts; i++)
    {
        n[i] = BN_new();
        d[i] = BN_new();
        if ((n[i] == NULL) || (d[i] == NULL))
            goto end;
    }

    /* np = x[0] * x[1] * .. * x[parts-1] */
    ret &= (BN_copy(np, x[0]) != NULL);
    for (i=1; i<parts; i++)
        ret &= BN_mod_mul(np, np, x[i], prime, ctx);

    /* Calculate all the denominators. */
    for (i=0; i<parts; i++)
    {
        /* d[i] = x[i] * (product of all x[j] - x[i] where i != j). */
        ret &= BN_set_word(d[i], 1);
        for (j=0; j<parts; j++)
        {
            if (i == j)
                continue;

            ret &= BN_sub(t, x[j], x[i]);
            ret &= BN_mod_mul(d[i], d[i], t, prime, ctx);
        }
        /* Ensure positive for inversion. */
        if (BN_is_negative(d[i]))
            ret &= BN_add(d[i], d[i], prime);
        ret &= BN_mod_mul(d[i], d[i], x[i], prime, ctx);

        /* n[i] = y[i].np (as x[i] is multiplied into denominator) */
        ret &= BN_mod_mul(n[i], np, y[i], prime, ctx);
    }

    /* Convert numerators to common denominator and sum. */
    for (i=0; i<parts; i++)
    {
        for (j=0; j<parts; j++)
        {
            if (i == j)
                continue;
            ret &= BN_mod_mul(n[i], n[i], d[j], prime, ctx);
        }
        if (i > 0)
        {
            ret &= BN_add(n[0], n[0], n[i]);
            if (BN_cmp(n[0], prime) >= 0)
                ret &= BN_sub(n[0], n[0], prime);
        }
    }
    /* Common denominator is product of all denominators. */
    for (i=1; i<parts; i++)
        ret &= BN_mod_mul(d[0], d[0], d[i], prime, ctx);

    /* secret = inverse denominator * sum of numerators. */
    ret &= (BN_mod_inverse(t, d[0], prime, ctx) != NULL);
    ret &= BN_mod_mul(secret, t, n[0], prime, ctx);

    /* No error if all operations succeeded. */
    if (ret == 1)
        err = NONE;
end:
    if (d != NULL)
    {
        for (i=parts-1; i>=0; i--)
            BN_free(d[i]);
        free(d);
    }
    if (n != NULL)
    {
        for (i=parts-1; i>=0; i--)
            BN_free(n[i]);
        free(n);
    }
    BN_free(t);
    BN_free(np);
    BN_CTX_free(ctx);
    return err;
}

