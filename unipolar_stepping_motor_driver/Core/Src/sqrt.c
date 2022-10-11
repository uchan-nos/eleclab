/*
 * sqrt.c
 *
 *  Created on: Oct 5, 2022

このファイルに含まれる整数根号計算プログラムは http://www.azillionmonkeys.com/qed/sqroot.html から持ってきました。
したがって、このファイルに含まれるソースコードのライセンスは全体のライセンスとは異なり、以下の通りです。

Copyright (c) 2010, Paul Hsieh
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice, this list of conditions
  and the following disclaimer in the documentation and/or other materials provided with the distribution.

- Neither my name, Paul Hsieh, nor the names of any other contributors to the code use may not be used
  to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

/* by Mark Crowne */
unsigned int mcrowne_isqrt(unsigned long val) {
  unsigned int temp, g=0;

  if (val >= 0x40000000) {
    g = 0x8000;
    val -= 0x40000000;
  }

#define INNER_ISQRT(s)                        \
  temp = (g << (s)) + (1 << ((s) * 2 - 2));   \
  if (val >= temp) {                          \
    g += 1 << ((s)-1);                        \
    val -= temp;                              \
  }

  INNER_ISQRT (15)
  INNER_ISQRT (14)
  INNER_ISQRT (13)
  INNER_ISQRT (12)
  INNER_ISQRT (11)
  INNER_ISQRT (10)
  INNER_ISQRT ( 9)
  INNER_ISQRT ( 8)
  INNER_ISQRT ( 7)
  INNER_ISQRT ( 6)
  INNER_ISQRT ( 5)
  INNER_ISQRT ( 4)
  INNER_ISQRT ( 3)
  INNER_ISQRT ( 2)

#undef INNER_ISQRT

  temp = g+g+1;
  if (val >= temp) g++;
  return g;
}

unsigned julery_isqrt(unsigned long val) {
    unsigned long temp, g=0, b = 0x8000, bshft = 15;
    do {
        if (val >= (temp = (((g << 1) + b)<<bshft--))) {
           g += b;
           val -= temp;
        }
    } while (b >>= 1);
    return g;
}

#include <stdint.h>
uint32_t julery_isqrt64(uint64_t val) {
  uint64_t temp, g=0, b = 0x80000000, bshft = 31;
  do {
      if (val >= (temp = (((g << 1) + b)<<bshft--))) {
         g += b;
         val -= temp;
      }
  } while (b >>= 1);
  return g;
}

