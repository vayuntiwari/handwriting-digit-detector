// fix15.h
#ifndef FIX15_H
#define FIX15_H

#include <stdint.h>
#include <stdlib.h> // for abs()

typedef int32_t fix15;

#define multfix15(a,b) ((fix15)((((int64_t)(a)) * ((int64_t)(b))) >> 15))
#define float2fix15(a) ((fix15)((a) * 32768.0f)) // 2^15
#define fix2float15(a) ((float)(a) / 32768.0f)
#define absfix15(a) abs(a)
#define int2fix15(a) ((fix15)((a) << 15))
#define fix2int15(a) ((int)((a) >> 15))
#define char2fix15(a) ((fix15)((int8_t)(a) << 15))
#define divfix(a,b) ((fix15)(((int64_t)(a) << 15) / (int64_t)(b)))

#endif
