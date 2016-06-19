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
Cycles/sec: 3400270316
new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  252883 0.993    13349  254720  P126 C
 join:  543174 1.001     6265  542740  P126 C

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  252339 1.002    13497  251927  P128 C
 join:  468098 1.002     7278  467198  P128 C

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  237035 0.996    14289  237964  P192 C
 join:  189504 1.001    17958  189345  P192 C

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  219400 0.993    15382  221055  P256 C
 join:   86839 0.990    38764   87717  P256 C

```

```
./share_test -speed -parts 5 -num 8

Parts: 5, Num: 8
Cycles/sec: 3400255272
new: 0, split init: 0, split: ........0 , join init: 0, update: .....0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  116128 1.000    29283  116117  P126 C
 join:  369954 0.944     8673  392050  P126 C

new: 0, split init: 0, split: ........0 , join init: 0, update: .....0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  109132 0.999    31135  109210  P128 C
 join:  317395 1.002    10738  316656  P128 C

new: 0, split init: 0, split: ........0 , join init: 0, update: .....0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:   98741 1.003    34555   98401  P192 C
 join:  146739 0.999    23147  146898  P192 C

new: 0, split init: 0, split: ........0 , join init: 0, update: .....0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:   85227 0.999    39865   85294  P256 C
 join:   72670 1.000    46801   72653  P256 C

```

```
./share_test -speed -gen

Parts: 2, Num: 3
Cycles/sec: 3400266144
new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  122766 1.000    27701  122748  OpenSSL Generic
 join:  103025 0.997    32910  103320  OpenSSL Generic

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  121751 1.000    27940  121698  OpenSSL Generic
 join:   92784 1.000    36629   92829  OpenSSL Generic

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  115560 1.001    29457  115431  OpenSSL Generic
 join:   62281 0.995    54339   62575  OpenSSL Generic

new: 0, split init: 0, split: ...0 , join init: 0, update: ..0, final: 0
   Op      ops  secs     c/op   ops/s  Impl
split:  109969 1.000    30926  109948  OpenSSL Generic
 join:   43604 1.000    77994   43596  OpenSSL Generic

```

