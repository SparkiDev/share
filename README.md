Shamir's Secret Sharing
=======================

Open Source Shamir's Secret Sharing Implementation

Written by Sean Parkinson (sparkinson@iprimus.com.au)

For information on copyright see: COPYRIGHT.md

Contents of Repository
----------------------

This code implements Shamir's Secret Sharing with a prime modulus.
(https://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing)

There are splitting and joining APIs.

Computationally nice prime numbers are chosen and hard-coded.
A secret up to 256-bits in length can be split and joined.

The code is small and fast.
Generic code using OpenSSL and custom prime specific code are avaialble at
runtime.

The arithmetic operations of custom prime specific code are constant time.

Building
--------

make

Testing
-------

Run all tests: share_test

Run tests with different parts and number of secrets: share_test -parts 5 -num 8

Run tests with genric implementation: share_test -gen

Run all tests and calculate speed: share_test -speed

Performance
-----------

Examples of ntru_test output on a 3.4 GHz Intel Ivy Bridge CPU:

```
./share_test -speed

Parts: 2, Num: 3
Cycles/sec: 3400282726
new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  257655 0.999    13177  258046  P128 C
 join:  405423 0.979     8208  414264  P128 C

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  242478 0.995    13946  243817  P192 C
 join:  155313 1.001    21905  155228  P192 C

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  230793 0.999    14715  231075  P256 C
 join:   77161 1.000    44083   77133  P256 C
```

```
./share_test -speed -parts 5 -num 8

Parts: 5, Num: 8
Cycles/sec: 3400280364
new: 0, split init: 0, split: ........0 , join init: 0, update: .....0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  110044 1.001    30930  109934  P128 C
 join:  284518 1.000    11952  284494  P128 C

new: 0, split init: 0, split: ........0 , join init: 0, update: .....0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:   96126 0.998    35298   96330  P192 C
 join:  118737 1.000    28645  118704  P192 C

new: 0, split init: 0, split: ........0 , join init: 0, update: .....0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:   84005 1.001    40515   83926  P256 C
 join:   62566 1.002    54438   62461  P256 C
```

```
./share_test -speed -gen

Parts: 2, Num: 3
Cycles/sec: 3400349682
new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  122942 1.001    27672  122880  OpenSSL Generic
 join:   94780 0.999    35823   94920  OpenSSL Generic

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  116120 1.001    29303  116041  OpenSSL Generic
 join:   65964 0.998    51436   66108  OpenSSL Generic

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  110591 1.000    30746  110594  OpenSSL Generic
 join:   47522 0.997    71341   47663  OpenSSL Generic
```

