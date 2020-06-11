#ifndef __SLMEM_TYPES_H
#define __SLMEM_TYPES_H

#ifdef __cplusplus
  extern "C" {
#endif


#include <stddef.h>
#include <stdint.h>

#define ssize_t ptrdiff_t
#define Int ptrdiff_t
#define UInt size_t
#define Char char
#define UChar unsigned char
#define Short short
#define UShort unsigned short

#define charBit __CHAR_BIT__
#define sizeof_int __SIZEOF_INT__
#define sizeof_long __SIZEOF_LONG__
#define sizeof_long_long __SIZEOF_LONG_LONG__

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define LittleEndian
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define BigEndian
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
  #define PdpEndian
  #error PDP endian not supported
#endif

#define BSwap16(x) (UShort)(((UShort)(x) << 8) + ((UShort)(x) >> 8))

#ifdef LittleEndian
  #define LittleEndian16(x) (x)
  #define BigEndian16(x) BSwap16(x)
  #define LittleEndian32(x) (x)
  #define BigEndian32(x) __builtin_bswap32(x)
  #define LittleEndian64(x) (x)
  #define BigEndian64(x) __builtin_bswap64(x)
#endif

#ifdef BigEndian
  #define LittleEndian16(x) BSwap16(x)
  #define BigEndian16(x) (x)
  #define LittleEndian32(x) __builtin_bswap32(x)
  #define BigEndian32(x) (x)
  #define LittleEndian64(x) __builtin_bswap64(x)
  #define BigEndian64(x) (x)
#endif

#if sizeof_int == 8
  #define LittleEndian_int LittleEndian64
  #define BigEndian_int BigEndian64
#elif sizeof_int == 4
  #define LittleEndian_int LittleEndian32
  #define BigEndian_int BigEndian32
#elif sizeof_int == 2
  #define LittleEndian_int LittleEndian16
  #define BigEndian_int BigEndian16
#endif

#if sizeof_long == 8
  #define LittleEndian_long LittleEndian64
  #define BigEndian_long BigEndian64
#elif sizeof_long == 4
  #define LittleEndian_long LittleEndian32
  #define BigEndian_long BigEndian32
#elif sizeof_long == 2
  #define LittleEndian_long LittleEndian16
  #define BigEndian_long BigEndian16
#endif

#define sizeofInt __SIZEOF_SIZE_T__

#if sizeofInt == 8
  #define LittleEndianInt LittleEndian64
  #define BigEndianInt BigEndian64
  #define log2sizeofInt 3
#elif sizeofInt == 4
  #define LittleEndianInt LittleEndian32
  #define BigEndianInt BigEndian32
  #define log2sizeofInt 2
#endif

#if sizeofInt == sizeof_int
  #define clrSb __builtin_clrsb
#elif sizeofInt == sizeof_long
  #define clrSb __builtin_clrsbl
#elif sizeofInt == sizeof_long_long
  #define clrSb __builtin_clrsbll
#endif

#define sizeofIntBits  (sizeofInt * charBit)

#define log2(X) ((Char) (charBit * sizeof(Int) - clrSb((Int)(X)) - 2))
#define log2u(X) ((Int)(X) < 0 ? charBit * sizeof(Int) - 1 : log2(X))

#define granul(items,by)  (( (items) + (by) - 1 ) / (by))
#define bitsChars(bits)  granul(bits, charBit)

#define bitOffset(ptr,offset)  ((Char *)(ptr))[(offset) / charBit]
#define bitValue(offset)  ( (Char)1 << (charBit - 1 - ((offset) % charBit)) )

#define Min(a,b)  ((a) < (b) ? (a) : (b))
#define Max(a,b)  ((a) > (b) ? (a) : (b))
#define Mins(a,b)  if (a > (b)) a = (b);
#define Maxs(a,b)  if (a < (b)) a = (b);

#define ARG(...) __VA_ARGS__

#ifdef __cplusplus
  }
#endif
#endif
