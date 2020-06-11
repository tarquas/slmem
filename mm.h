// Slice Memory (SlMem) Manager
// (c) 2014-2020. Prywit Research Labs <http://pwnz.pro/prywit>, Taras Mykhailovych <tarquas@gmail.com>
//
// Usage:
//   #define SlMem <<my_prefix>>  ("SlMem" by default)
//   #include <<this>>
//   ...
//   SlMemMm_H(<<my_other_prefix>>)
//
// Definitions:
//   - "We" -- the current component in its current implementation.
//   - "You" -- a software, which refers to Us and uses Our
//       publically available functions.
//   - "Platform" -- Your execution context and its specifications.
//   - "Slice" -- a memory block of a constant size: a "Slice Length",
//       which is defined by Us.
//   - "Page" -- a group of contiguous Slices, number of which
//       is constant and defined by Us.
//   - "Space" -- dynamically extensible (theoretically: up to infinity)
//       group of Pages, in such a way, that every Slice inside a Space
//       is identified by an undefined (see below) integer "Global Index".
//   - the difference of Global Indexes of any two given Slices is defined and
//       is called "Relative Index of second slice to first slice".
//   - Relative Index is designed to be used as a "pointer to Slice" data type
//       as a "binary-transitive[1] integer typed field" inside a Slice.
//       It addresses the another Slice from current one inside a common Space.
//   - "Page Address" -- a Platform address of a Page, containing a Slice.
//   - "Slice Index" -- unsigned integer zero-based Slice index inside a Page
//   - Page Address and Slice Index are the sufficient parameters
//       to identify a Slice in the platform and are called
//       "Slice Platform Location".
//
//  [1] Taras Mykhailovych. The Power of Binary Walk. // Prywit Research Labs
//        - 2013 - http://my.spdu.org/qpbtk/spec/bw.txt
//        - 2020 - http://pwnz.pro/usefuls/qpbtk/bw.txt
//
// Features:
//   - Theoretically unlimited in scalability of Slices count within a Space.
//   - Platform-independent manager with O(log2 N) complexity of Slice locator.
//   - "Relative Index" of newly created Slices is optimized to be minimal
//       in its absolute value and fast in performance at the same time.
//
// Language: C
// Dependencies: none
// Limitations: only Platform limitations
//

#ifndef __SLMEM_MM_H
#define __SLMEM_MM_H
#ifdef __cplusplus
  extern "C" {
#endif

#include <stddef.h>

// ============ CODE BUILDER
#define SlMemMm_H_(SlMem)  \
\
/* Before working with Us, You should define the upstream memory manager */\
void SlMem##Mm_setUpstreamAlloc(  \
  void*  (*malloc)  (size_t),  /* will be called to allocate given size of memory */\
  void   (*free)    (void*)  /* (optional)  if not null, called to free the empty Pages*/\
);  \
/*NOTE: if we are the top-level memory manager and a part of the kernel,
  you can define "malloc" callback as pointer incremental allocator
  and set the "free" callback to null. */\
\
/* Returns Our Slice Length (is a power-of-2) */\
unsigned SlMem##Mm_DataSlice_length();  \
\
/* Returns log2 of Our Slice Length */\
unsigned SlMem##Mm_DataSlice_lengthBits();  \
\
/* Allocate a Slice, close enough to given Slice */\
void SlMem##Mm_DataSlice_allocOccupy(  \
  void **_page,  \
    /* [in,out] pointer to a variable, containing current Page Address,
       and gets returned a Page Address of the newly created Slice.
       NOTE: pass the null value as an address to create a new Space and
      get a Slice in it.*/\
  unsigned  *_slice,  \
    /* [in,out] pointer to a variable, containing current Slice Index,
       and gets returned a Slice Index of the newly created Slice.
       NOTE: if a null is given as a Page Address, input value of this
       parameter is ignored, but returns the first Slice Index in newly
       created Space.*/\
  ptrdiff_t *_shift  \
    /* [out] returns a Relative Index of newly
       allocated Slice to the current Slice. */\
);  \
\
/* Release a Slice */\
void SlMem##Mm_DataSlice_allocRelease(  \
  void *_page,  /* Page Address of the Slice to be released. */\
  unsigned _slice  /* Slice Index of the Slice to be released. */\
);  \
\
/* Get the target Slice Platform Location
   by a given Slice Platform Location and its Relative Index. */\
void* SlMem##Mm_DataSlice_locate(  \
  void **_page,  \
    /* [in,out] pointer to a variable, containing Page Address of
       base Slice and gets returned a Page Address of target Slice. */\
  unsigned  *_slice,  \
    /* [in,out] pointer to a variable, containing base Slice Index
       and gets returned a target Slice Index. */\
  ptrdiff_t shift  /* Relative Index of target Slice to base Slice. */\
);

#define SlMemMm_H(name) SlMemMm_H_(name)

SlMemMm_H(SlMem)

#ifdef __cplusplus
  }
#endif
#endif
