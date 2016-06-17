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

#include "share.h"

/**
 * The prototype of a function that creates a new number object.
 *
 * @param [in]  len  The length of the secret in bytes.
 * @param [out] num  The new number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
typedef SHARE_ERR (SHARE_NUM_NEW_FUNC)(uint16_t len, void **num);
/**
 * The prototype of a function that frees a number object.
 *
 * @param [in] num  The number object.
 */
typedef void (SHARE_NUM_FREE_FUNC)(void *num);
/**
 * The prototype of a function that decodes data into a number object.
 * The data is assumed to be big-endian bytes.
 *
 * @param [in] data  The data to be decoded.
 * @param [in] len   The length of the data to be decoded.
 * @param [in] num   The number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
typedef SHARE_ERR (SHARE_NUM_FROM_BIN_FUNC)(const uint8_t *data, uint16_t len,
    void *num);
/**
 * The prototype of a function that encodes a number object into data.
 * The data is assumed to be big-endian bytes.
 *
 * @param [in] num   The number object.
 * @param [in] data  The data to hold the encoding.
 * @param [in] len   The number of bytes that data can hold.
 * @return  PARAM_BAD_LEN when encoding is too long for data.<br>
 *          NONE otherwise.
 */
typedef SHARE_ERR (SHARE_NUM_TO_BIN_FUNC)(void *num, uint8_t *data,
    uint16_t len);
/**
 * The prototype of a function that calculates the y value of a split.
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
typedef SHARE_ERR (SHARE_SPLIT_FUNC)(void *prime, uint8_t parts, void **a,
    void *x, void *y);
/**
 * The prototype of a function that calculates the secret from splits.
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
typedef SHARE_ERR (SHARE_JOIN_FUNC)(void *prime, uint8_t parts, void **x,
    void **y, void *secret);

/** The data structure of an implementation method. */
typedef struct share_meth_st
{
    /** The name of the implementation method. */
    char *name;
    /** The maximim length of the secret. No maximum: 0. */
    uint16_t len;
    /** The number of parts that the implementation supports. Any: 0. */
    uint8_t parts;
    /** Flags indicating features of implementation. */
    uint32_t flags;
    /** Creates a new number object. */
    SHARE_NUM_NEW_FUNC *num_new;
    /** Frees a number object. */
    SHARE_NUM_FREE_FUNC *num_free;
    /** Decodes data into a number object. */
    SHARE_NUM_FROM_BIN_FUNC *num_from_bin;
    /** Encodes a number object into data. */
    SHARE_NUM_TO_BIN_FUNC *num_to_bin;
    /** Calculates the y value of a split. */
    SHARE_SPLIT_FUNC *split;
    /** Calculates the secret from splits. */
    SHARE_JOIN_FUNC *join;
} SHARE_METH;

SHARE_ERR share_meths_get(uint16_t len, uint8_t parts, uint32_t flags,
    SHARE_METH **meth);

/* The 128-bit secret prime optimized implementation. */
SHARE_ERR share_p128_num_new(uint16_t len, void **num);
void share_p128_num_free(void *num);
SHARE_ERR share_p128_num_from_bin(const uint8_t *data, uint16_t len, void *num);
SHARE_ERR share_p128_num_to_bin(void *num, uint8_t *data, uint16_t len);
SHARE_ERR share_p128_split(void *prime, uint8_t parts, void **a, void *x,
    void *y);
SHARE_ERR share_p128_join(void *prime, uint8_t parts, void **x, void **y,
    void *secret);

/* The 192-bit secret prime optimized implementation. */
SHARE_ERR share_p192_num_new(uint16_t len, void **num);
void share_p192_num_free(void *num);
SHARE_ERR share_p192_num_from_bin(const uint8_t *data, uint16_t len, void *num);
SHARE_ERR share_p192_num_to_bin(void *num, uint8_t *data, uint16_t len);
SHARE_ERR share_p192_split(void *prime, uint8_t parts, void **a, void *x,
    void *y);
SHARE_ERR share_p192_join(void *prime, uint8_t parts, void **x, void **y,
    void *secret);

/* The 256-bit secret prime optimized implementation. */
SHARE_ERR share_p256_num_new(uint16_t len, void **num);
void share_p256_num_free(void *num);
SHARE_ERR share_p256_num_from_bin(const uint8_t *data, uint16_t len, void *num);
SHARE_ERR share_p256_num_to_bin(void *num, uint8_t *data, uint16_t len);
SHARE_ERR share_p256_split(void *prime, uint8_t parts, void **a, void *x,
    void *y);
SHARE_ERR share_p256_join(void *prime, uint8_t parts, void **x, void **y,
    void *secret);

#ifdef SHARE_USE_OPENSSL
/* The generic implementation that uses OpenSSL. */
SHARE_ERR share_openssl_num_new(uint16_t len, void **num);
void share_openssl_num_free(void *num);
SHARE_ERR share_openssl_num_from_bin(const uint8_t *data, uint16_t len,
    void *num);
SHARE_ERR share_openssl_num_to_bin(void *num, uint8_t *data, uint16_t len);
SHARE_ERR share_openssl_split(void *prime, uint8_t parts, void **a, void *x,
    void *y);
SHARE_ERR share_openssl_join(void *prime, uint8_t parts, void **x, void **y,
    void *secret);
#endif

