// Slice Memory (SlMem) Data Manager
// (c) 2014-2020. Prywit Research Labs <http://pwnz.pro/prywit>, Taras Mykhailovych <tarquas@gmail.com>
//
// Usage:
//   #define SlMem <<my_prefix>>  ("SlMem" by default)
//   #include <<this>>
//   ...
//   SlMemMmData_H(<<my_other_prefix>>)
//
// Definitions:
//   - "Offset" -- char offset in Slice;
//   - "IntVR" -- Integer Variable-size Reversed -- byte-reversed IntV data type [1];
//   - Data Slice Relocation -- continuation of Data in Next Data Slice;
//   - "Relocation Index" -- IntVR value in tail of Slice.
//
//  [1] Taras Mykhailovych. The Power of Binary Walk. // Prywit Research Labs
//        - 2013 - http://my.spdu.org/qpbtk/spec/bw.txt
//        - 2020 - http://pwnz.pro/usefuls/qpbtk/bw.txt
//
// Features:
//   - Theoretically unlimited in scalability of Data size.
//   - Platform-independent manager with O(log2 N) complexity of Data Slice Relocation.
//   - "Relocation Index" of newly created Data Slices is optimized to be minimal
//       in its absolute value and fast in performance at the same time.

#ifndef __SLMEM_MM_DATA_H
#define __SLMEM_MM_DATA_H
#ifdef __cplusplus
  extern "C" {
#endif

#include "mm.h"

#define SlMemMmData_IntVR_ntail(ptr) (clrSb(LittleEndianInt(*(Int *)((Char *)(ptr) - sizeofInt + 1))))
#define SlMemMmData_IntVR_length(ptr) (SlMemMmData_IntVR_ntail(ptr) + 1)
#define SlMemMmData_sliceRelocLength(ptr) (SlMemMmData_IntVR_length((Char *)(ptr) + SlMemMm_nDataChars - 1))
#define SlMemMmData_sliceAvailChars(ptr) (SlMemMm_nDataChars - SlMemMmData_sliceRelocLength(ptr))

typedef struct SlMemPtr {
  void *_page;
  unsigned char *_slicePtr;
  char *ptr;
  unsigned _slice;
} SlMemPtr;

// ============ CODE BUILDER
#define SlMemMmData_H_(SlMem)  \
SlMemMm_H(SlMem)  \
\
ptrdiff_t SlMem##MmData_IntVR_read(char **intV);  \
\
void SlMem##MmData_IntVR_write(char **intV, ptrdiff_t value, char *ntailp);  \
\
ptrdiff_t SlMem##MmData_alloc(SlMemPtr *p, SlMemPtr *after);  \
\
/* Read `count` chars to `dst` and return number of chars read. */\
/* Use `dst=NULL` to perform seek. */\
\
size_t SlMem##MmData_read(SlMemPtr *p,  \
  void *dst, size_t count  \
  /*void *src, UInt cmpCount, //TODO: compare*/\
);  \
\
/* Modify data: move `nRemove` chars to `dst` (or remove if `dst=NULL`),*/\
/* insert `nInsert` chars.*/\
/* Returns number of chars discarded (moved or removed). */\
\
size_t SlMem##MmData_splice(SlMemPtr *p,  \
  void *dst, size_t nRemove,  \
  void *src, size_t nInsert  \
);  \
\
ptrdiff_t SlMem##MmData_sliceNext(SlMemPtr *p, ptrdiff_t shift);  \
\
void *SlMem##MmData_MemPtr_init(SlMemPtr *p, char *start);  \
void *SlMem##MmData_MemPtr_reset(SlMemPtr *p);  \
\
char SlMem##MmData_common(SlMemPtr *result, SlMemPtr *p1, SlMemPtr *p2);

#define SlMemMmData_H(name) SlMemMmData_H_(name)

SlMemMmData_H(SlMem)

#ifdef __cplusplus
  }
#endif
#endif
