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

#ifndef SHARE_H
#define SHARE_H

#include <stdint.h>

/** Flag indicating the implementation is able to handle multiple primes. */
#define SHARE_METHS_FLAG_GENERIC	0x01

/** The maximum number of parts able to be required to reconstruct secret. */
#define SHARE_PARTS_MAX			16

/** Error codes. */
typedef enum share_err_en {
    /** No error. */
    NONE		= 0,
    /** Operation failed to produce a valid result. */
    FAILED		= 1,
    /** The data to work on is invalid for the operation. */
    INVALID_DATA	= 2,
    /** A parameter was a NULL pointer. */
    PARAM_NULL		= 10,
    /** A parameter was a bad value. */
    PARAM_BAD_VALUE	= 11,
    /** A parameter was a bad length. */
    PARAM_BAD_LEN	= 12,
    /** No result was found. */
    NOT_FOUND		= 20,
    /** Dynamic memory allocation error. */
    ALLOC		= 30,
    /** Random number generation failure. */
    RANDOM		= 40,
    /** Value has no modular inverse. */
    MOD_INV             = 41
} SHARE_ERR;

/** The structure for splitting and joining */
typedef struct share_st SHARE;

SHARE_ERR SHARE_new(uint16_t len, uint8_t parts, uint32_t flags, SHARE **share);
void SHARE_free(SHARE *share);

SHARE_ERR SHARE_get_len(SHARE *share, uint16_t *len);
SHARE_ERR SHARE_get_num(SHARE *share, uint16_t *num);
SHARE_ERR SHARE_get_impl_name(SHARE *share, char **name);

SHARE_ERR SHARE_split_init(SHARE *share, uint8_t *secret);
SHARE_ERR SHARE_split(SHARE *share, uint8_t *data);

SHARE_ERR SHARE_join_init(SHARE *share);
SHARE_ERR SHARE_join_update(SHARE *share, uint8_t *data);
SHARE_ERR SHARE_join_final(SHARE *share, uint8_t *secret);

#endif

