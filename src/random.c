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
#include <stdlib.h>
#include "random.h"

#ifdef OPT_NTRU_OPENSSL_RAND
#include "openssl/rand.h"
#else
#include "share_sha3.h"
#endif

/**
 * Fill the buffer with random bytes.
 *
 * @param [in] r  The buffer to fill.
 * @param [in] l  The length of the buffer in bytes.
 * @return  0 on success.<Br>
 *          1 when random data is not available.
 */
int pseudo_random(unsigned char *r, int l)
{
#ifdef OPT_NTRU_OPENSSL_RAND
    return RAND_bytes(r, l) != 1;
#else
#ifdef OPT_NTRU_RDRAND
    int i;
    uint64_t rd[4];

    for (i=0; i<4; i++)
        asm volatile ("rdrand %0" : "=r" (rd[i]));

    return share_shake256(r, l, (unsigned char *)rd, sizeof(rd)) == 0;
#else
    int i;
    static uint64_t rd[4] = { 0, 0, 0, 0 };

    for (i=0; i<4 && ++rd[i] == 0; i++) ;

    return share_shake256(r, l, (unsigned char *)rd, sizeof(rd)) == 0;
#endif
#endif
}

