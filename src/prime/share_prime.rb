# Copyright (c) 2016 Sean Parkinson
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

class SharePrime

  def initialize(bits, word)
      @bits = bits
      @mod_bits = bits + 1
      @word = word
      @elems = (@mod_bits + 63) / 64
      @bytes = (@mod_bits + 7) / 8
      @last = @elems - 1
  end

  def write_header()
    File.readlines(File.dirname(__FILE__)+'/../../license/license.c').each { |l| puts l }

    puts <<EOF
#include <stdlib.h>
#include <string.h>
#include "share_meth.h"

#define NUM_ELEMS	#{@elems}
#define NUM_BYTES	#{@bytes}
#define MOD_WORD	0x#{@word.to_s(16)}

#define U128(w)		((__uint128_t)w)
EOF
  end

  def write_copy()
    puts <<EOF

/**
 * Copy the data of the number object into the result number object.
 *
 * @param [in] r  The result number object.
 * @param [in] a  The number object to copy.
 */
static void p#{@bits}_copy(uint64_t *r, uint64_t *a)
{
EOF
    0.upto(@elems-1) do |i|
      puts "    r[#{i}] = a[#{i}];"
    end
    puts <<EOF
}
EOF
  end

  def write_set_word()
    puts <<EOF

/**
 * Set the number object to be one word value - w.
 *
 * @param [in] a  The number object set.
 * @param [in] w  The word sized value to set.
 */
static void p#{@bits}_set_word(uint64_t *a, uint64_t w)
{
    a[0] = w;
EOF
    1.upto(@elems-1) do |i|
      puts "    a[#{i}] = 0;"
    end
    puts <<EOF
}
EOF
  end

  def write_mod_small()
    puts <<EOF

/**
 * Multiply by prime's (mod's) last word.
 *
 * @param [in] a  The number to multiply.
 * @return  The multiplicative result.
 */
#define MUL_MOD_WORD(a) \\
EOF
    if @bits <= 128
      w = @word
      h = w.to_s(2).length
      print "    ("
      h.downto(1) do |i|
        print "((a) << #{i}) + " if w & (1 << i) != 0
      end
      puts "(a))"
    else
      puts "    ((a) * MOD_WORD)"
    end
    puts <<EOF

/**
 * Perform modulo operation on number, a, up to 16-bits longer than the prime
 * and put result in r.
 *
 * @param [in] r  The result of the reduction.
 * @param [in] a  The number to operate on.
 */
static void p#{@bits}_mod_small(uint64_t *r, uint64_t *a)
{
    static const __int128_t m0 = ((__int128_t)1 << 64) + MOD_WORD;
    __int128_t t = 0;

    t -= MUL_MOD_WORD(a[#{@last}]);
EOF
    m = "          m0"
    0.upto(@last-1) do |i|
      puts "    t += #{m}; t += a[#{i}]; r[#{i}] = t; t >>= 64;"
      m = "(uint64_t)-1"
    end
    puts <<EOF
                                  r[#{@last}] = t;
}
EOF
  end

  def write_mod_long()
    ls = 4
puts <<EOF

/**
 * Perform modulo operation on a product result in 128-bit elements.
 *
 * @param [in] r  The number reduce number.
 * @param [in] a  The product result in 128-bit elements.
 */
static void p#{@bits}_mod_long(uint64_t *r, __uint128_t *a)
{
    static const __int128_t m0 = (((__int128_t)MOD_WORD) << #{64+ls}) +
        ((1 << #{ls}) * MOD_WORD * MOD_WORD);
    static const __int128_t m1 = ((__int128_t)MOD_WORD) << #{64+ls};

EOF
    print "   "
    print " a[0] += m0;"
    1.upto(@last-1) { |i| print " a[#{i}] += m1;" }
    (@last+1).upto(@last*2-1) { |i| print " a[#{i}] += 1 << #{ls};" }; puts
    puts <<EOF

    a[#{@last-1}] += (a[#{@last}] & 1) << 64; a[#{@last}] ^= a[#{@last}] & 1;

    a[0] -= MUL_MOD_WORD(a[#{@last}]); a[0] += (0 - a[#{@last*2}]) & (MOD_WORD * MOD_WORD);
EOF
    1.upto(@last-1) do |i|
      puts "    a[#{i}] -= MUL_MOD_WORD(a[#{i+@last}]); a[#{i}] += a[#{i-1}] >> 64;"
    end
    puts <<EOF
                                a[#{@last}]  = a[#{@last-1}] >> 64;

EOF
    0.upto(@last) { |i| puts "    r[#{i}] = a[#{i}];" }
    puts <<EOF

    p#{@bits}_mod_small(r, r);
}
EOF
  end

  def write_mod_add()
    puts <<EOF

/**
 * Add two numbers, a and b, (modulo prime) and put the result in a third, r.
 *
 * @param [in] r  The result of the addition.
 * @param [in] a  The first operand.
 * @param [in] b  The second operand.
 */
static void p#{@bits}_mod_add(uint64_t *r, uint64_t *a, uint64_t *b)
{
    __int128_t t;

EOF
    0.upto(@last) do |i|
      op = (i != 0) ? "+" : " "
      shift_t = (i != @last) ? " t >>= 64;" : ""
      puts "    t #{op}= a[#{i}]; t += b[#{i}]; r[#{i}] = t;#{shift_t}"
    end
    puts <<EOF

    p#{@bits}_mod_small(r, r);
}
EOF
  end

  def write_mod_sub()
    puts <<EOF

/**
 * Subtract b from a (modulo prime) and put the result r.
 *
 * @param [in] r  The result of the subtraction.
 * @param [in] a  The first operand.
 * @param [in] b  The second operand.
 */
static void p#{@bits}_mod_sub(uint64_t *r, uint64_t *a, uint64_t *b)
{
    static const __int128_t m0 = ((__int128_t)1 << 64) + MOD_WORD;
    __int128_t t = 0;

EOF
    m = "          m0"
    0.upto(@last) do |i|
      add_mod = (i != @last) ? "t += #{m};" : "                  "
      shift_t = (i != @last) ? " t >>= 64;" : ""
      puts "    #{add_mod} t += a[#{i}]; t -= b[#{i}]; r[#{i}] = t;#{shift_t}"
      m = "(uint64_t)-1"
    end
    puts <<EOF

    p#{@bits}_mod_small(r, r);
}
EOF
  end

  def write_mod_sqr()
    puts <<EOF

/**
 * Square the number, a, modulo the prime amd put in result in r.
 *
 * @param [in] r  The result of the squaring.
 * @param [in[ a  The number object to square.
 */
static void p#{@bits}_mod_sqr(uint64_t *r, uint64_t *a)
{
    __uint128_t p128;
    uint64_t p64;
    __uint128_t t[NUM_ELEMS*2-1];

EOF
    print "   "
    0.upto(@last*2) { |i| print " t[#{i}] = 0;" }; puts
    puts
    0.upto(@last*2) do |i|
      0.upto(@last) do |j|
        k = i - j
        next if k < 0 || k > @last || j > k
        v = (k != @last and j != @last) ? "p128" : "p64"
        a = (k != @last and j != @last) ? "U128(a[#{j}])" : "a[#{j}]"
        if j == @last and k == @last
          puts "    #{v} = #{a};"
        elsif j == @last and @bits <= 128
          puts "    #{v} = a[#{k}] & (0 - #{a});"
        elsif k == @last and @bits <= 128
          puts "    #{v} = #{a} & (0 - a[#{k}]);"
        else
          puts "    #{v} = #{a} * a[#{k}];"
        end
        0.upto(j == k ? 0 : 1) do
          if k != @last and j != @last
            puts "    t[#{i}] += (uint64_t)p128;"
            puts "    t[#{i+1}] += p128 >> 64;"
          else
            puts "    t[#{i}] += p64;"
          end
        end
      end
    end
    puts <<EOF

    p#{@bits}_mod_long(r, t);
}
EOF
  end

  def write_mod_mul()
    puts <<EOF

/**
 * Multiply two numbers, a and b, modulo the prime amd put in result in r.
 *
 * @param [in] r  The result of the multiplication.
 * @param [in[ a  The first operand number object.
 * @param [in[ b  The first operand number object.
 */
static void p#{@bits}_mod_mul(uint64_t *r, uint64_t *a, uint64_t *b)
{
    __uint128_t p128;
    uint64_t p64;
    __uint128_t t[NUM_ELEMS*2-1];

EOF
    print "   "
    0.upto(@last*2) { |i| print " t[#{i}] = 0;" }; puts
    puts
    0.upto(@last*2) do |i|
      0.upto(@last) do |j|
        k = i - j
        next if k < 0 || k > @last
        v = (k != @last and j != @last) ? "p128" : "p64"
        a = (k != @last and j != @last) ? "U128(a[#{j}])" : "a[#{j}]"
        if j == @last and k == @last
          puts "    #{v} = #{a} & b[#{k}];"
        elsif j == @last and @bits <= 128
          puts "    #{v} = b[#{k}] & (0 - #{a});"
        elsif k == @last and @bits <= 128
          puts "    #{v} = #{a} & (0 - b[#{k}]);"
        else
          puts "    #{v} = #{a} * b[#{k}];"
        end
        if k != @last and j != @last
          puts "    t[#{i}] += (uint64_t)p128;"
          puts "    t[#{i+1}] += p128 >> 64;"
        else
          puts "    t[#{i}] += p64;"
        end
      end
    end
    puts <<EOF

    p#{@bits}_mod_long(r, t);
}
EOF
  end

  def write_mod()
    puts <<EOF

/**
 * Reduce the number that is less than 2 times the prime modulo the prime.
 *
 * @param [in] r  The result of the reduction.
 * @param [in] a  The number to reduce.
 */
static void p#{@bits}_mod(uint64_t *r,uint64_t *a)
{
    int c;
    __int128_t t;

EOF
    print "    c = (a[#{@last}] == 0x1) & "
    cb = ""
    (@last-1).downto(1) do |i|
      print "((a[#{i}] > 0x0) | "
      cb += ")"
    end
    puts "(a[0] >= MOD_WORD)#{cb};"
    puts <<EOF
    t = -((0-c) & MOD_WORD);
EOF
    0.upto(@last-1) do |i|
      puts "    t = a[#{i}] + t; r[#{i}] = t; t >>= 64;"
    end
    puts <<EOF
    r[#{@last}] = a[#{@last}] - c;
}
EOF
  end

  def write_mod_inv()
    w = @word - 2
    ws = w.to_s(16)
    h = w.to_s(2).length
    puts <<EOF

/**
 * Calculate the inverse of a modulo the prime and store thre result in r.
 *
 * @param [in] r  The result of the inversion.
 * @param [in] a  The number to invert.
 */
static void p#{@bits}_mod_inv(uint64_t *r, uint64_t *a)
{
    int i;
    uint64_t t#{ws}[NUM_ELEMS];
    uint64_t t[NUM_ELEMS];

EOF
    a = "a"
    v = "a"
    1.upto(h-1) do |i|
      print "    p#{@bits}_mod_sqr(t, #{a});"
      a = "t"
      if w & (1 << i) != 0
        print " p#{@bits}_mod_mul(t#{ws}, #{v}, t);"
        v = "t#{ws}"
      end
      puts
    end
    puts <<EOF
    for (i=#{h-1}; i<#{@bits}; i++)
        p#{@bits}_mod_sqr(t, t);
    p#{@bits}_mod_mul(r, t, t#{ws});
}
EOF
  end

  def write_num_new()
    puts <<EOF

/**
 * Create a new number object.
 *
 * @param [in]  len  The length of the secret in bytes.
 * @param [out] num  The new number object.
 * @return  ALLOC when dynamic memory allocation fails.<br>
 *          NONE otherwise.
 */
SHARE_ERR share_p#{@bits}_num_new(uint16_t len, void **num)
{
    SHARE_ERR err = NONE;

    len = len;

    *num = malloc(NUM_ELEMS*sizeof(uint64_t));
    if (*num == NULL)
        err = ALLOC;

    return err;
}
EOF
  end

  def write_num_free()
    puts <<EOF

/**
 * Free the dynamic memory associated with the number object.
 *
 * @param [in] num  The number object.
 */
void share_p#{@bits}_num_free(void *num)
{
    if (num != NULL) free(num);
}
EOF
  end

  def write_num_from_bin()
    puts <<EOF

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
SHARE_ERR share_p#{@bits}_num_from_bin(const uint8_t *data, uint16_t len,
    void *num)
{
    SHARE_ERR err = NONE;
    int i, j;
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
EOF
  end

  def write_num_to_bin()
    puts <<EOF

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
SHARE_ERR share_p#{@bits}_num_to_bin(void *num, uint8_t *data, uint16_t len)
{
    SHARE_ERR err = NONE;
    int i, j;
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
EOF
  end

  def write_split()
    puts <<EOF

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
SHARE_ERR share_p#{@bits}_split(void *prime, uint8_t parts, void **a, void *x,
    void *y)
{
    SHARE_ERR err = NONE;
    int i;
    uint64_t t[NUM_ELEMS], m[NUM_ELEMS];
    uint64_t **ad = (uint64_t **)a;
    uint64_t *xd = x;
    uint64_t *yd = y;

    prime = prime;

    /* y = x^0.a[0] + x^1.a[1] - minimum of two parts. */
    p#{@bits}_mod_mul(t, ad[1], xd);
    p#{@bits}_mod_add(yd, ad[0], t);

    p#{@bits}_copy(m, xd);
    for (i=2; i<parts; i++)
    {
        /* y += x^i.a[i] (m = x^i) */
        p#{@bits}_mod_mul(m, m, xd);
        p#{@bits}_mod_mul(t, ad[i], m);
        p#{@bits}_mod_add(yd, yd, t);
    }
    p#{@bits}_mod(yd, yd);

    return err;
}
EOF
  end

  def write_join()
    puts <<EOF

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
SHARE_ERR share_p#{@bits}_join(void *prime, uint8_t parts, void **x, void **y,
    void *secret)
{
    SHARE_ERR err = NONE;
    int i, j;
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
    p#{@bits}_copy(np, xd[0]);
    for (i=1; i<parts; i++)
        p#{@bits}_mod_mul(np, np, x[i]);

    /* Calculate all the denominators. */
    for (i=0; i<parts; i++)
    {
        /* d[i] = x[i] * (product of all x[j] - x[i] where i != j). */
        n = &nr[i*NUM_ELEMS];
        d = &dr[i*NUM_ELEMS];
        p#{@bits}_set_word(d, 1);
        for (j=0; j<parts; j++)
        {
            if (i == j)
                continue;

            p#{@bits}_mod_sub(t, xd[j], xd[i]);
            p#{@bits}_mod_mul(d, d, t);
        }
        p#{@bits}_mod_mul(d, d, xd[i]);

        /* n[i] = y[i].np (as x[i] is multiplied into denominator) */
        p#{@bits}_mod_mul(n, np, yd[i]);
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
            p#{@bits}_mod_mul(n, n, d);
        }
        if (i > 0)
            p#{@bits}_mod_add(nr, nr, n);
    }
    /* Common denominator is product of all denominators. */
    for (i=1; i<parts; i++)
        p#{@bits}_mod_mul(dr, dr, &dr[i*NUM_ELEMS]);

    /* secret = inverse denominator * sum of numerators. */
    p#{@bits}_mod_inv(t, dr);
    p#{@bits}_mod_mul(sd, t, nr);
    p#{@bits}_mod(sd, sd);

end:
    if (dr != NULL) free(dr);
    if (nr != NULL) free(nr);
    return err;
}
EOF
  end

  def write()
    write_header()
    write_copy()
    write_set_word()
    write_mod_small()
    write_mod_long()
    write_mod_add()
    write_mod_sub()
    write_mod_sqr()
    write_mod_mul()
    write_mod()
    write_mod_inv()
    write_num_new()
    write_num_free()
    write_num_from_bin()
    write_num_to_bin()
    write_split()
    write_join()
    puts
  end
end

prime = SharePrime.new(ARGV[0].to_i, ARGV[1].to_i(16))
prime.write

