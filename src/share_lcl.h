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
#include "share_meth.h"

/** The structure holding primes to use. */
typedef struct share_prime_st
{
    /** The maximum number of bits supported by prime. */
    uint16_t max;
    /** The prime encoded in big-endian bytes. */
    const uint8_t *data;
    /** The length of the prime encoding in bytes. */
    uint16_t len;
} SHARE_PRIME;

/** The data structure for the share operations object. */
struct share_st
{
    /** The methods of an implementation of share operations. */
    SHARE_METH *meth;
    /** The length of the secret in bytes. */
    uint16_t len;
    /** The mask for the top word. */
    uint8_t mask;
    /** The number of parts required to calculate the secret. */
    uint8_t parts;
    /** The length of the prime in bytes. */
    uint16_t prime_len;
    /** The prime as a number object. */
    void *prime;
    /** An array of number objects. */
    void **num;
    /** An array of number objects. */
    void **y;
    /** Storage for encoded and decoded numbers. */
    uint8_t *random;
    /** Result number object. */
    void *res;
    /** Count of splits generated when splitting or added when joining. */
    int cnt;
};

