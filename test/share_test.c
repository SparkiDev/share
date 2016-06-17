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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "share.h"
#include "random.h"

/* The printf format of a 64-bit number */
#ifdef CC_CLANG
#define PRIu64 "llu"
#else
#define PRIu64 "lu"
#endif


/* Valid length values for test. */
static uint16_t valid[] = {128, 192, 256};
/* The number of valid values for test. */
#define VALID_NUM    (int)(sizeof(valid)/sizeof(*valid))

/* Number of cycles/sec. */
uint64_t cps = 0;

/*
 * Get the current cycle count from the CPU.
 *
 * @return  Cycle counter from CPU.
 */
uint64_t get_cycles()
{
    uint32_t hi, lo;

    asm volatile ("rdtsc\n\t" : "=a" (lo), "=d"(hi));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

/*
 * Calculate the number of cycles/second.
 */
void calc_cps()
{
    uint64_t end, start = get_cycles();
    sleep(1);
    end = get_cycles();
    cps = end-start;
    printf("Cycles/sec: %"PRIu64"\n", cps);
}

/*
 * Calcuate the number of cycles and operations per second of splitting.
 *
 * @param [in] share   The share object to split with.
 * @param [in] parts   The number of parts required to recreate secret.
 * @param [in] num     The number of splits to create.
 * @param [in] secret  The secret to split.
 * @param [in] split   Array of split values as byte arrays.
 */
void speed_split(SHARE *share, uint8_t parts, uint8_t num, uint8_t *secret,
    uint8_t **split)
{
    uint32_t i, j;
    uint32_t num_ops;
    uint64_t start, end, diff;
    char *name = "";

    SHARE_get_impl_name(share, &name);

    /* Prime the caches, etc */
    for (i=0; i<10000; i++)
    {
        SHARE_split_init(share, secret);
        for (j=0; j<num; j++)
            SHARE_split(share, split[j % parts]);
        SHARE_split(share, split[0]);
    }

    /* Approximate number of ops in a second. */
    start = get_cycles();
    for (i=0; i<1000; i++)
    {
        SHARE_split_init(share, secret);
        for (j=0; j<num; j++)
            SHARE_split(share, split[j % parts]);
        SHARE_split(share, split[0]);
    }
    end = get_cycles();
    num_ops = cps/((end-start)/1000);

    /* Perform about 1 seconds worth of operations. */
    start = get_cycles();
    for (i=0; i<num_ops; i++)
    {
        SHARE_split_init(share, secret);
        for (j=0; j<num; j++)
            SHARE_split(share, split[j % parts]);
        SHARE_split(share, split[0]);
    }
    end = get_cycles();

    diff = end - start;

    printf("split: %7d %2.3f  %7"PRIu64" %7"PRIu64"  %s\n", num_ops,
        diff/(cps*1.0), diff/num_ops, cps/(diff/num_ops), name);
}

/*
 * Calcuate the number of cycles and operations per second of joining.
 *
 * @param [in] share   The share object to split with.
 * @param [in] parts   The number of parts required to recreate secret.
 * @param [in] split   Array of split values as byte arrays.
 * @param [in] secret  Array to hold secret.
 */
void speed_join(SHARE *share, uint8_t parts, uint8_t **split, uint8_t *secret)
{
    uint32_t i, j;
    uint32_t num_ops;
    uint64_t start, end, diff;
    char *name = "";

    SHARE_get_impl_name(share, &name);

    /* Prime the caches, etc */
    for (i=0; i<10000; i++)
    {
        SHARE_join_init(share);
        for (j=0; j<parts; j++)
            SHARE_join_update(share, split[j]);
        SHARE_join_final(share, secret);
    }

    /* Approximate number of ops in a second. */
    start = get_cycles();
    for (i=0; i<1000; i++)
    {
        SHARE_join_init(share);
        for (j=0; j<parts; j++)
            SHARE_join_update(share, split[j]);
        SHARE_join_final(share, secret);
    }
    end = get_cycles();
    num_ops = cps/((end-start)/1000);

    /* Perform about 1 seconds worth of operations. */
    start = get_cycles();
    for (i=0; i<num_ops; i++)
    {
        SHARE_join_init(share);
        for (j=0; j<parts; j++)
            SHARE_join_update(share, split[j]);
        SHARE_join_final(share, secret);
    }
    end = get_cycles();

    diff = end - start;

    printf(" join: %7d %2.3f  %7"PRIu64" %7"PRIu64"  %s\n", num_ops,
        diff/(cps*1.0), diff/num_ops, cps/(diff/num_ops), name);
}

/*
 * Test an implementation of secret splitting.
 *
 * @param [in] length  The length of the secret.
 * @param [in] parts   The number of parts required to recreate secret.
 * @param [in] flags   The extra requirements on the methods to choose.
 * @param [in] num     The number of splits to create.
 * @param [in] speed   Indicates whether to calculate speed of operations.
 * @return  0 on successful testing.<br>
 *          1 otherwise.
 */
int test_share(uint16_t length, uint8_t parts, uint32_t flags, uint8_t num,
    uint8_t speed)
{
    int ret = 1;
    SHARE_ERR err;
    SHARE *share = NULL;
    uint8_t *secret = NULL;
    uint8_t *sec = NULL;
    uint8_t **split = NULL;
    uint32_t i, j;
    uint16_t len;

    /* Create share object for joing and splitting. */
    err = SHARE_new(length/8, parts, flags, &share);
    fprintf(stderr, "new: %d", err);
    if (err != NONE) goto end;

    /* Get the length of the encoded data. */
    err = SHARE_get_len(share, &len);
    if (err != NONE) goto end;

    sec = malloc(length/8);
    if (sec == NULL) goto end;
    secret = malloc(length/8);
    if (secret == NULL) goto end;
    /* Generate random secret. */
    pseudo_random(secret, length/8);

    split = malloc(parts * sizeof(*split));
    if (split == NULL) goto end;
    memset(split, 0, parts * sizeof(*split));
    for (i=0; i<parts; i++)
    {
        split[i] = malloc(len);
        if (split[i] == NULL) goto end;
    }

    /* Split */
    err = SHARE_split_init(share, secret);
    fprintf(stderr, ", split init: %d", err);
    if (err != NONE) goto end;
    fprintf(stderr, ", split: ");
    for (i=0; i<num; i++)
    {
        err = SHARE_split(share, split[i % parts]);
        if (err != NONE)
        {
            fprintf(stderr, "%d", err);
            goto end;
        }
        else
            fprintf(stderr, ".");
    }
    err = SHARE_split(share, split[0]);
    fprintf(stderr, "%d ", err);
    if (err != NONE) goto end;

    /* Join */
    err = SHARE_join_init(share);
    fprintf(stderr, ", join init: %d", err);
    if (err != NONE) goto end;
    fprintf(stderr, ", update: ");
    for (i=0; i<parts; i++)
    {
        err = SHARE_join_update(share, split[i]);
        if (err != NONE)
        {
            fprintf(stderr, "%d", err);
            goto end;
        }
        else
            fprintf(stderr, ".");
    }
    fprintf(stderr, "%d", err);
    err = SHARE_join_final(share, sec);
    fprintf(stderr, ", final: %d", err);
    if (err != NONE) goto end;
    for (i=0; i<length/8; i++)
    {
        if (sec[i] != secret[i])
        {
            fprintf(stderr, " %02x/%02x (%d)", sec[i], secret[i], i);
            goto end;
        }
    }
    fprintf(stderr, "\n");

    if (speed)
    {
        printf("%5s  %7s %5s  %7s %7s  %s\n", "Op", "ops", "secs", "c/op",
            "ops/s", "Impl");
        speed_split(share, parts, num, secret, split);
        speed_join(share, parts, split, secret);
    }
    else
    {
        fprintf(stderr, " s=");
        for (i=0; i<length/8; i++)
            fprintf(stderr, "%02x", secret[i]);
        for (j=0; j<parts; j++)
        {
            fprintf(stderr, "\n%2d=", j);
            for (i=0; i<len/2; i++)
                fprintf(stderr, "%02x", split[j][i]);
            fprintf(stderr, "\n   ");
            for (; i<len; i++)
                fprintf(stderr, "%02x", split[j][i]);
        }
    }

    ret = 0;
end:
    fprintf(stderr, "\n");
    if (split != NULL)
    {
        for (i=0; i<parts; i++)
            if (split[i] != NULL) free(split[i]);
        free(split);
    }
    if (secret != NULL) free(secret);
    if (sec != NULL) free(sec);
    SHARE_free(share);
    return ret;
}

/*
 * The main entry point of program.
 *
 * @param [in] argc  The count of command line arguments.
 * @param [in] argv  The command line arguments.
 * @return  1 on test failure.<br>
 *          0 otherwise.
 */
int main(int argc, char *argv[])
{
    int ret = 0;
    uint8_t i;
    uint16_t s;
    uint8_t parts = 2;
    uint8_t num = 3;
    uint16_t which = 0;
    uint8_t speed = 0;
    uint32_t flags = 0;

    while (--argc)
    {
        argv++;

        if (strcmp(*argv, "-speed") == 0)
            speed = 1;
        else if (strcmp(*argv, "-parts") == 0)
        {
            if (--argc == 0)
            {
                fprintf(stderr, "Number of parts missing from command line\n");
                ret = 1;
                goto end;
            }
            parts = atoi(*(++argv));
            if ((parts < 2) || (parts > SHARE_PARTS_MAX))
            {
                fprintf(stderr, "Parts invalid: 2 <= %d <= %d\n", parts,
                    SHARE_PARTS_MAX);
                ret = 1;
                goto end;
            }
            num = parts + 1;
        }
        else if (strcmp(*argv, "-num") == 0)
        {
            if (--argc == 0)
            {
                fprintf(stderr, "Number of splits missing from command line\n");
                ret = 1;
                goto end;
            }
            num = atoi(*(++argv));
            if (num <= parts)
            {
                fprintf(stderr, "Num invalid: %d < %d <= %d\n", parts, num,
                    255);
                ret = 1;
                goto end;
            }
        }
        else if (strcmp(*argv, "-gen") == 0)
            flags |= SHARE_METHS_FLAG_GENERIC;
        else
        {
            s = atoi(*argv);
            /* If strength value is valid then set corresponding bit. */
            for (i=0; i<VALID_NUM; i++)
            {
                if (valid[i] == s)
                    which |= 1<<i;
            }
        }
    }

    printf("Parts: %d, Num: %d\n", parts, num);

    if (speed)
        calc_cps();

    /* Test all prime lengths requested. */
    for (i=0; i<VALID_NUM; i++)
    {
        if ((which == 0) || ((which & (1<<i)) != 0))
            ret |= test_share(valid[i], parts, flags, num, speed);
    }

end:
    return ret;
}

