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

#include <stdlib.h>
#include "share_meth.h"

/** The implementation methods for share operations. */
SHARE_METH share_meths[] =
{
    /* The 126-bit prime optimized implementation. */
    { "P126 C",
      126, 0, 0,
      share_p126_num_new, share_p126_num_free,
      share_p126_num_from_bin, share_p126_num_to_bin,
      share_p126_split, share_p126_join },
    /* The 128-bit prime optimized implementation. */
    { "P128 C",
      128, 0, 0,
      share_p128_num_new, share_p128_num_free,
      share_p128_num_from_bin, share_p128_num_to_bin,
      share_p128_split, share_p128_join },
    /* The 192-bit prime optimized implementation. */
    { "P192 C",
      192, 0, 0,
      share_p192_num_new, share_p192_num_free,
      share_p192_num_from_bin, share_p192_num_to_bin,
      share_p192_split, share_p192_join },
    /* The 256-bit prime optimized implementation. */
    { "P256 C",
      256, 0, 0,
      share_p256_num_new, share_p256_num_free,
      share_p256_num_from_bin, share_p256_num_to_bin,
      share_p256_split, share_p256_join },
#ifdef SHARE_USE_OPENSSL
    /* The generic implementation that uses OpenSSL. */
    { "OpenSSL Generic",
      0, 0, SHARE_METHS_FLAG_GENERIC,
      share_openssl_num_new, share_openssl_num_free,
      share_openssl_num_from_bin, share_openssl_num_to_bin,
      share_openssl_split, share_openssl_join },
#endif
};

/** The number of implementation methods. */
#define SHARE_METHS_NUM ((int8_t)(sizeof(share_meths)/(sizeof(*share_meths))))

/**
 * Retrieves an implementation method that matches the requirements.
 *
 * @param [in]  len    The length of the secret in bits required to be
 *                     supported.
 * @param [in]  parts  The number of parts required to be supported.
 * @param [in]  flags  Flags required of the implementation.
 * @param [out] meth   The method that matches the requirements.
 * @return  NOT_FOUND when no available method meets the requirements.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_meths_get(uint16_t len, uint8_t parts, uint32_t flags,
    SHARE_METH **meth)
{
    SHARE_ERR err = NOT_FOUND;
    int8_t i;
    SHARE_METH *m = NULL;

    /* Find the first implementation that matches. */
    for (i=0; i<SHARE_METHS_NUM; i++)
    {
        /* Length of zero indicates no restriction. Otherwise it must match.
         * Parts of zero indicates no restriction. Otherwise it must match.
         * Must have at least the flags requested.
         */
        if (((share_meths[i].len == 0) || (share_meths[i].len == len)) &&
            ((share_meths[i].parts == 0) || (share_meths[i].parts == parts)) &&
            ((share_meths[i].flags & flags) == flags))
        {
            m = &share_meths[i];
            err = NONE;
            break;
        }
    }

    *meth = m;

    return err;
}

