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


all: share_test

SHARE_IMPL=share_openssl.o share_p126.o share_p128.o share_p192.o share_p256.o

src/prime/share_p126.c: src/prime/share_prime.rb
	ruby ./src/prime/share_prime.rb 126 1 > src/prime/share_p126.c
src/prime/share_p128.c: src/prime/share_prime.rb
	ruby ./src/prime/share_prime.rb 128 19 > src/prime/share_p128.c
src/prime/share_p192.c: src/prime/share_prime.rb
	ruby ./src/prime/share_prime.rb 192 1f > src/prime/share_p192.c
src/prime/share_p256.c: src/prime/share_prime.rb
	ruby ./src/prime/share_prime.rb 256 5d > src/prime/share_p256.c

share_p126.o: src/prime/share_p126.c src/*.h include/*.h
	$(CC) -c $(CFLAGS) -Isrc -o $@ $<
share_p129.o: src/prime/share_p129.c src/*.h include/*.h
	$(CC) -c $(CFLAGS) -Isrc -o $@ $<
share_p128.o: src/prime/share_p128.c src/*.h include/*.h
	$(CC) -c $(CFLAGS) -Isrc -o $@ $<
share_p192.o: src/prime/share_p192.c src/*.h include/*.h
	$(CC) -c $(CFLAGS) -Isrc -o $@ $<
share_p256.o: src/prime/share_p256.c src/*.h include/*.h
	$(CC) -c $(CFLAGS) -Isrc -o $@ $<

SHARE_OBJ=share.o $(SHARE_IMPL) share_meth.o random.o share_sha3.o

%.o: src/%.c src/*.h include/*.h
	$(CC) -c $(CFLAGS) -o $@ $<

share_test.o: test/share_test.c
	$(CC) -c $(CFLAGS) -Isrc -o $@ $<
share_test: share_test.o $(SHARE_OBJ)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o
	rm -f share_test

