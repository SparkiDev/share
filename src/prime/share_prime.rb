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
      @last = @elems-1
      @shift = @mod_bits & 63
      @mask = "0x#{((1 << @shift) - 1).to_s(16)}"
      @shift_l = 64 - @shift
      @prime = (1 << @mod_bits) - @word
      @hi_bits = @mod_bits & 63
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
    0.upto(@last) do |i|
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
    1.upto(@last) do |i|
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
    __int128_t t;

    t = (a[#{@last}] >> #{@shift}) * MOD_WORD; a[#{@last}] &= #{@mask};
EOF
    0.upto(@last) do |i|
        print "    t += a[#{i}]; r[#{i}] = t;"
        print " t >>= 64;" if i != @last
        puts
    end
    puts <<EOF
}
EOF
  end

  def write_mod_long()
puts <<EOF

/**
 * Perform modulo operation on a product result in 128-bit elements.
 *
 * @param [in] r  The number reduce number.
 * @param [in] a  The product result in 128-bit elements.
 */
static void p#{@bits}_mod_long(uint64_t *r, __uint128_t *a)
{
EOF
    print "    a[0] += MUL_MOD_WORD(a[#{@last}] >> #{@shift});"
    puts " a[#{@last}] &= #{@mask};"
    0.upto(@last) do |i|
      puts "    a[#{i}] += MUL_MOD_WORD(a[#{@last+1+i}] << #{@shift_l});"
    end
    0.upto(@last-1) do |i|
        puts "    a[#{i+1}] += a[#{i}] >> 64; a[#{i}] = (uint64_t)a[#{i}];"
    end
    print "    a[0] += MUL_MOD_WORD(a[#{@last}] >> #{@shift});"
    puts " a[#{@last}] &= #{@mask};"
    0.upto(@last-1) do |i|
        puts "    a[#{i+1}] += a[#{i}] >> 64;"
    end
    0.upto(@last) do |i|
        puts "    r[#{i}] = a[#{i}];"
    end
    puts <<EOF
}
EOF
  end

  def write_mod_long_1()
puts <<EOF

/**
 * Perform modulo operation on a product result in 128-bit elements.
 *
 * @param [in] r  The number reduce number.
 * @param [in] a  The product result in 128-bit elements.
 */
static void p#{@bits}_mod_long(uint64_t *r, __uint128_t *a)
{
    __uint128_t t;

EOF
    0.upto(@last-1) do |i|
      puts "    t = (a[#{@last+i}] >> 1) + ((uint64_t)a[#{@last+i+1}] << 63); a[#{i}] += MUL_MOD_WORD(t);"
    end
    puts
    0.upto(@last-2) do |i|
        puts "    r[#{i}] = a[#{i}]; a[#{i+1}] += a[#{i}] >> 64;"
    end
    puts "    r[#{@last-1}] = a[#{@last-1}];"
    puts "    r[#{@last}] = (a[#{@last}] & 1) + (a[#{@last-1}] >> 64);"
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
    puts
    0.upto(@last) do |i|
        puts "/** Prime element #{i}. */"
        v = (@prime >> (i*64)) & 0xffffffffffffffff
        puts "#define P#{@bits}_#{i}\t0x#{v.to_s(16)}"
    end

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
    __uint128_t t = 0;
EOF
    0.upto(@last) do |i|
      shift_t = (i != @last) ? " t >>= 64;" : ""
      puts "    t += P#{@bits}_#{i}; t += a[#{i}]; t -= b[#{i}]; r[#{i}] = t;#{shift_t}"
    end
    puts <<EOF

    p#{@bits}_mod_small(r, r);
}
EOF
  end

  def write_mod_sqr()
    if @hi_bits == 1
      p64 = "\n    uint64_t p64;"
      t_elems = @elems * 2 - 1;
    else
      p64 = ""
      t_elems = @elems * 2;
    end
    puts <<EOF

/**
 * Square the number, a, modulo the prime and put in result in r.
 *
 * @param [in] r  The result of the squaring.
 * @param [in] a  The number object to square.
 */
static void p#{@bits}_mod_sqr(uint64_t *r, uint64_t *a)
{#{p64}
    __uint128_t p128;
    __uint128_t t[#{t_elems}];

EOF
    print "   "
    0.upto(t_elems-1) { |i| print " t[#{i}] = 0;" }; puts
    puts
    0.upto(@last*2) do |i|
      0.upto(@last) do |j|
        k = i - j
        next if k < 0 || k > @last || j > k
        if @hi_bits == 1
          if j == @last and k == @last
            puts "    p64 = a[#{j}];"
          elsif (j == @last or k == @last) and @mod_bits >= 256
            puts "    p64 = a[#{j}] * a[#{k}];"
          elsif j == @last
            puts "    p64 = a[#{k}] & (0 - a[#{j}]);"
          elsif k == @last
            puts "    p64 = a[#{j}] & (0 - a[#{k}]);"
          else
            puts "    p128 = U128(a[#{j}]) * a[#{k}];"
          end
        else
          puts "    p128 = U128(a[#{j}]) * a[#{k}];"
        end
        0.upto(j == k ? 0 : 1) do
          if @hi_bits != 1 or (j != @last and k != @last)
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

/**
 * Square the number, a, modulo the prime n times and put in result in r.
 *
 * @param [in] r  The result of the squaring.
 * @param [in] a  The number object to square.
 * @param [in] n  The number of times to square.
 */
static void p#{@bits}_mod_sqr_n(uint64_t *r, uint64_t *a, uint16_t n)
{
    uint16_t i;

    p#{@bits}_mod_sqr(r, a);
    for (i=1; i<n; i++)
        p#{@bits}_mod_sqr(r, r);
}
EOF
  end

  def write_mod_mul()
    if @hi_bits == 1
      p64 = "\n    uint64_t p64;"
      t_elems = @elems * 2 - 1;
    else
      p64 = ""
      t_elems = @elems * 2;
    end
    puts <<EOF

/**
 * Multiply two numbers, a and b, modulo the prime amd put in result in r.
 *
 * @param [in] r  The result of the multiplication.
 * @param [in] a  The first operand number object.
 * @param [in] b  The first operand number object.
 */
static void p#{@bits}_mod_mul(uint64_t *r, uint64_t *a, uint64_t *b)
{#{p64}
    __uint128_t p128;
    __uint128_t t[#{t_elems}];

EOF
    print "   "
    0.upto(t_elems-1) { |i| print " t[#{i}] = 0;" }; puts
    puts
    0.upto(@last*2) do |i|
      0.upto(@last) do |j|
        k = i - j
        next if k < 0 || k > @last
        if @hi_bits == 1
          if j == @last and k == @last
            puts "    p64 = a[#{j}] & b[#{k}];"
          elsif (j == @last or k == @last) and @mod_bits >= 256
            puts "    p64 = a[#{j}] * b[#{k}];"
          elsif j == @last
            puts "    p64 = b[#{k}] & (0 - a[#{j}]);"
          elsif k == @last
            puts "    p64 = a[#{j}] & (0 - b[#{k}]);"
          else
            puts "    p128 = U128(a[#{j}]) * b[#{k}];"
          end
        else
          puts "    p128 = U128(a[#{j}]) * b[#{k}];"
        end
        if @hi_bits != 1 or (j != @last and k != @last)
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
    uint64_t c;
    __int128_t t;

EOF
    top = ((1 << (@mod_bits & 63)) - 1)
    mid = (1 << 64) - 1
    bot = (1 << 64) - @word
    print "    c = (a[#{@last}] == 0x#{top.to_s(16)}) & "
    (@last-1).downto(1) do |i|
      print "(a[#{i}] == 0x#{mid.to_s(16)}) & "
    end
    puts "(a[0] >= 0x#{bot.to_s(16)});"
    puts "    t = c * MOD_WORD;"
    0.upto(@last) do |i|
      print "    t += a[#{i}]; r[#{i}] = t"
      print "; t >>= 64;" if i != @last
      print " & 0x#{top.to_s(16)};" if i == @last
      puts
    end
    puts <<EOF
}
EOF
  end

  def write_mod_inv()
    ls = (@word + 1).to_s(2).length
    w = (1 << ls) - (@word + 2)
    ws = w.to_s(16)
    t3 = ""

    a = []
    h = @mod_bits - ls
    while h != 1
      if h % 2 == 0
        a << 2
        h /= 2
      elsif h % 5 == 0
        a << 5
        h /= 5
        t3 = "\n    uint64_t t3[NUM_ELEMS];"
      elsif h % 3 == 0
        a << 3
        h /= 3
      else
        a << 1
        h -= 1;
      end
    end

    puts <<EOF

/**
 * Calculate the inverse of a modulo the prime and store thre result in r.
 *
 * @param [in] r  The result of the inversion.
 * @param [in] a  The number to invert.
 */
static void p#{@bits}_mod_inv(uint64_t *r, uint64_t *a)
{
    uint64_t t[NUM_ELEMS];
    uint64_t t2[NUM_ELEMS];#{t3}
EOF
    puts "    uint64_t t#{ws}[NUM_ELEMS];" if w > 1
    puts

    wt = w >> 1
    n = "a"
    wn = "a"
    ts = "t2"
    1.upto(ls) do |i|
      break if wt == 0
      print "    p#{@bits}_mod_sqr(#{ts}, #{n});"
      print " p#{@bits}_mod_mul(t#{ws}, #{wn}, #{ts});" if (wt & 1) != 0
      wn = "t#{ws}" if (wt & 1) != 0
      puts
      n = ts
      ts = "t"
      wt >>= 1
    end

    h = 1
    n = "a"
    a.reverse.each do |o|
      case o
      when 1
        if h == 1
            print "    \t\t\t"
        else
            print "    p#{@bits}_mod_sqr(t2, #{n});"
        end
        print "\tp#{@bits}_mod_mul(t, t2, a);"
        puts "\t\t/* #{h+1} */"
        h += 1
      when 2
        if h == 1
           print "    \t\t\t"
        else
           print "    p#{@bits}_mod_sqr_n(t2, #{n}, #{h});"
        end
        print "\tp#{@bits}_mod_mul(t, t2, t);"
        puts "\t\t/* #{h*2} */"
        h *= 2
      when 3
        puts "    p#{@bits}_copy(t2, #{n});" if n != "a"
        if h == 1
           print "    \t\t\t"
           print "\tp#{@bits}_mod_mul(t, t2, a);"
        else
            print "    p#{@bits}_mod_sqr_n(t, #{n}, #{h});"
            print "\tp#{@bits}_mod_mul(t, t, t2);"
        end
        puts "\t\t/* #{h*2} */"
        print "    p#{@bits}_mod_sqr_n(t, t, #{h});"
        if h != 1
          print "\tp#{@bits}_mod_mul(t, t, t2);"
        else
          print "\tp#{@bits}_mod_mul(t, t, a);"
        end
        puts "\t\t/* #{h*3} */"
        h *= 3
      when 5
        print "    p#{@bits}_mod_sqr_n(t2, #{n}, #{h});"
        print "\tp#{@bits}_mod_mul(t3, t2, #{n});"
        puts "\t/* #{h*2} */"
        print "    p#{@bits}_mod_sqr_n(t2, t3, #{2*h});"
        print "\tp#{@bits}_mod_mul(t3, t2, t3);"
        puts "\t/* #{h*2*2} */"
        print "    p#{@bits}_mod_sqr_n(t2, t3, #{h});"
        print "\tp#{@bits}_mod_mul(t, t2, #{n});"
        puts "\t\t/* #{h*2*2+h} */"
        h *= 5
      end
      n = "t"
    end
    puts <<EOF
    p#{@bits}_mod_sqr_n(t, t, #{ls});
    p#{@bits}_mod_mul(r, t, #{wn});
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
    uint8_t i;
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
    if @mod_bits & 63 == 1
        write_mod_long_1()
    else
        write_mod_long()
    end
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

