#!/usr/bin/ruby
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


bits=ARGV[0].to_i
num = 1<<bits
puts "#bits = #{bits}"
1.step(1001,2) do |i|
  n = num - i
  p = `openssl prime #{n}`
  if p =~ /is prime/
    s = n.to_s(16)
    puts "Prime: 0x#{s}"
    puts "Word: 0x#{i.to_s(16)}"
    s = "0"*(s.length & 1) + s
    puts "Hex bytes:"
    d = s.length & 15
    print "    "
    0.step(s.length-1,2) do |j|
        print "0x#{s[j..j+1]}, "
        print "\n    " if (j-d) & 15 == 14
    end
    puts
    puts "Num bytes: #{(s.length + 1) / 2}"
    break
  end
end

