// Slice Memory (SlMem) Manager
// (c) 2014-2020. Prywit Research Labs <http://pwnz.pro/prywit>, Taras Mykhailovych <tarquas@gmail.com>

// Usage:
//   #define SlMemMm_nDataCharsBits  4   // override some defaults if needed
//   #define SlMem <<my_prefix>>  ("SlMem" by default)
//   #include <<this>>
//   ...
//   #define SlMemMm_nDataCharsBits  6   // override some defaults if needed
//   SlMemMm_C(<<my_other_prefix>>)
//

// *** IMPLEMENTATION

#include "types.h"
#include "mm.h"
#include "macro.c"
#include "mm-config.c"

#define SlMemMm_TypePageHeader(SlMem, type)  PStruct( SlMem##Mm_##type##PageHeader, ( \
  SlMem##Mm_LeafPage  *pageUp; \
  Int  \
    sliceUp,  \
    slicesFree, \
    branchesFree;  \
  Char shadow [bitsChars(SlMemMm_n##type##Slices)];  \
  Char slicesSpace[0];  \
) );

#define SlMemMm_TypePageHeaderSlices(type)  \
  ( granul(sizeof(SlMem##Mm_##type##PageHeader), sizeof(SlMem##Mm_##type##Slice)) )

#define SlMemMm_TypePageSlicesFreeAll(type)  \
  ( SlMemMm_n##type##Slices - SlMemMm_TypePageHeaderSlices(type) )

#define SlMemMm_TypePageAlloc(type)  \
SlMem##Mm_##type##Page* SlMem##Mm_##type##Page_alloc (void) {  \
  if (!SlMem##Mm_malloc) return 0;  \
  \
  SlMem##Mm_##type##PageHeader *page = (SlMemMm_##type##PageHeader *)  \
    SlMem##Mm_malloc(sizeof(SlMemMm_##type##Page));  \
  if (!page) return 0;  \
  page->pageUp = 0;  \
  \
  REM( "Set shadow for page header" )  \
  bitSet( \
    &page->shadow,  \
    0,  \
    SlMemMm_TypePageHeaderSlices(type),  \
    1, 1 \
  );  \
  \
  REM( "Reset shadow of all available slices" )  \
  bitSet(&page->shadow,  \
    SlMemMm_TypePageHeaderSlices(type),  \
    SlMemMm_n##type##Slices,  \
    0,1);  \
  \
  REM( "Set counter of free slices" )  \
  page->slicesFree =  \
  page->branchesFree = SlMemMm_TypePageSlicesFreeAll(type);  \
  \
  REM( "For leafages, reset all branches" )  \
  \
  if (SlMemMm_Page_of ## type == SlMemMm_Page_ofLeaf) {  \
    bitSet( \
      &(*(SlMemMm_##type##Page *) page)[SlMemMm_TypePageHeaderSlices(type)],  \
      0,  \
      SlMemMm_TypePageSlicesFreeAll(type) * SlMemMm_n##type##Chars * charBit,  \
      0, 1 \
    );  \
  }  \
  \
  return (SlMemMm_##type##Page *)page;  \
}

// Slice_occupy

#define SlMemMm_Slice_occupy(SlMem, _page,_slice)  {  \
  SlMem##Mm_LeafPageHeader *page = (SlMem##Mm_LeafPageHeader *)(_page);  \
  Char  \
    *sha = &bitOffset(&page->shadow,(_slice)),  \
    shv = bitValue(_slice);  \
  if (!(*sha & shv)) {  \
    *sha |= shv;  \
    --page->slicesFree;  \
  }  \
}

// Slice_release

#define SlMemMm_Slice_release(SlMem, _page,_slice) {  \
  SlMem##Mm_LeafPageHeader *page = (SlMem##Mm_LeafPageHeader *)(_page);  \
  Char  \
    *sha = &bitOffset(&page->shadow,(_slice)),  \
    shv = bitValue(_slice);  \
  if (*sha & shv) {  \
    *sha &= -1 ^ shv;  \
    ++page->slicesFree;  \
  }  \
}

// Slice_freeIfEmpty

#define SlMemMm_TypeSlice_freeIfEmpty(SlMem, _page, type) if (SlMem##Mm_free) {  \
  SlMem##Mm_##type##PageHeader  *page = (SlMem##Mm_##type##PageHeader *)(_page);  \
  if (page && (  \
    (  SlMemMm_Page_of##type == SlMemMm_Page_ofLeaf  \
      ?  page->branchesFree  \
      :  page->slicesFree  \
    ) == SlMemMm_TypePage_slicesFreeAll(SlMem, type)  \
  )) {  \
    if (page->pageUp) {  \
      (*page->pageUp)[page->sliceUp] = 0;  \
      ++((SlMem##Mm_LeafPageHeader *)page->pageUp)->branchesFree;  \
    }  \
    SlMem##Mm_free((void*)page);  \
  }  \
}

// Slice_alloc

#define SlMemMm_TypeSlice_alloc(SlMem, type)  void  SlMem##Mm_##type##Slice_alloc(  \
  SlMem##Mm_##type##Page **_page,  \
  unsigned  *_slice,  \
  Int *_shift  \
) {  \
  SlMem##Mm_##type##PageHeader  *page;  \
  unsigned  slice, slicep;  \
  Int  shift, shiftl, shiftr;  \
  \
  if (!*_page) {  \
    *_page = SlMem##Mm_##type##Page_alloc();  \
    *_slice = SlMemMm_n##type##Slices / 2;  \
    *_shift = 0;  \
    return;  \
  }  \
  \
  page = (SlMem##Mm_##type##PageHeader *)(*_page);  \
  slice = *_slice;  \
  if (!page->slicesFree) {  \
    SlMem##Mm_LeafPage  *pageUp = page->pageUp; \
    unsigned  sliceUp;  \
    if (pageUp) {  \
      sliceUp = page->sliceUp;  \
    } else {  \
      SlMem##Mm_LeafSlice_alloc(&pageUp, &sliceUp, &shift);  \
      (*pageUp)[sliceUp] = page;  \
    }  \
    SlMemMm_Slice_occupy(SlMem, pageUp, sliceUp);  \
    --((SlMem##Mm_LeafPageHeader *)pageUp)->branchesFree;  \
    REM( "This finds a 'page' with at least 1 free slice of a free slot for a new 'page'" )  \
    SlMem##Mm_LeafSlice_alloc(&pageUp, &sliceUp, &shift);  \
    shift *= SlMemMm_nLeafSlices;  \
    page = (SlMem##Mm_##type##PageHeader *)((*pageUp)[sliceUp]);  \
    slicep = slice;  \
    \
    if (!page) {  \
      page = (SlMem##Mm_##type##PageHeader *) SlMem##Mm_##type##Page_alloc();  \
      REM( "Prefer the least shift - align to left or right" )  \
      slice = shift < 0 ? SlMemMm_n##type##Slices - 1 : 0;  \
      page->pageUp = pageUp;  \
      page->sliceUp = sliceUp;  \
      (*pageUp)[sliceUp] = page;  \
      --((SlMem##Mm_LeafPageHeader *)pageUp)->branchesFree;  \
    }  \
    REM( "Here assumed, that 'page' has got at least 1 free slice" )  \
    SlMem##Mm_##type##Slice_alloc((SlMem##Mm_##type##Page **)&page, &slice, &shiftl);  \
    shift += (Int) slice - (Int) slicep;  \
    *_page = (SlMem##Mm_##type##Page *) page;  \
    *_slice = slice;  \
    *_shift = shift;  \
    return;  \
  }  \
  \
  REM(" Locate closest free ")  \
  Char  \
    *res = 0,  \
    mask = 0,  \
    *left = (Char *) &bitOffset( &page->shadow, slice ),  \
    *right = left,  \
    *pages = (Char *)&page->shadow,  \
    *pagee = (Char *)&page->slicesSpace;  \
  UChar  \
    left_mask,  \
    right_mask;  \
  \
  if (*left != -1) {  \
    shiftl = shiftr = 0;  \
    left_mask =  \
    right_mask = (UChar) bitValue(slice);  \
  } else {  \
    left_mask = (UChar) bitValue(charBit - 1);  \
    shiftl = charBit - ((Int) slice % charBit) - 1;  \
    right_mask = (UChar) bitValue(0);  \
    shiftr = -((Int) slice % charBit);  \
  }  \
  while (  \
    left >= pages && right < pagee &&  \
    *left == -1 && *right == -1  \
  ) {  \
    -- left; shiftl -= charBit;  \
    ++ right; shiftr += charBit;  \
  }  \
  if (left < pages || right >= pagee) {  \
    if (right < pagee) {  \
      while (right < pagee && *right == -1) {  \
        ++ right; shiftr += charBit;  \
      }  \
    }  \
    if (right >= pagee) {  \
      while (left >= pages && *left == -1) {  \
        -- left; shiftl -= charBit;  \
      }  \
    }  \
  }  \
  if (left >= pages && right < pagee && *left != -1 && *right != -1) {  \
    while (  \
      (*right & right_mask) &&  \
      (*left & left_mask)  \
    ) {  \
      left_mask <<= 1; -- shiftl;  \
      right_mask >>= 1; ++ shiftr;  \
    }  \
    if (right_mask && !(*right & right_mask)) {  \
      *_slice += shiftr;  \
      *_shift = shiftr;  \
      return;  \
    }  \
    if (left_mask && !(*left & left_mask)) {  \
      *_slice += shiftl;  \
      *_shift = shiftl;  \
      return;  \
    }  \
  }  \
  if (right < pagee && *right != -1) {  \
    while (*right & right_mask) {  \
      right_mask >>= 1; ++ shiftr;  \
    }  \
    if (right_mask && !(*right & right_mask)) {  \
      *_slice += shiftr;  \
      *_shift = shiftr;  \
      return;  \
    }  \
  }  \
  if (left >= pages && *left != -1) {  \
    while (*left & left_mask) {  \
      left_mask <<= 1; -- shiftl;  \
    }  \
    if (left_mask && !(*left & left_mask)) {  \
      *_slice += shiftl;  \
      *_shift = shiftl;  \
      return;  \
    }  \
  }  \
  *_page = 0;  \
  *_shift = 0;  \
}

#define SlMemMm_TypeSlice_locate(SlMem, type)  void*  SlMem##Mm_##type##Slice_locate(  \
  void **_page,  \
  unsigned  *_slice,  \
  Int  shift  \
) {  \
  SlMem##Mm_##type##PageHeader  *page = (SlMem##Mm_##type##PageHeader *)(*_page);  \
  unsigned  slice = *_slice;  \
  Int  upShift;  \
  \
  upShift = shift + slice;  \
  if (upShift < 0 || upShift >= SlMemMm_n##type##Slices) {  \
    SlMem##Mm_LeafPage  *pageUp = page->pageUp;  \
    unsigned sliceUp = page->sliceUp;  \
    REM(" If fallen out of bounds ")\
    if (!pageUp) { *_page = 0; return 0; }  \
    upShift /= SlMemMm_n##type##Slices;  \
    if (upShift <= 0) --upShift;  \
    SlMem##Mm_LeafSlice_locate((void **) &pageUp, &sliceUp, upShift);  \
    REM(" If fallen out of bounds ")\
    if (!pageUp) { *_page = 0; return 0; }  \
    *_page = (*pageUp)[sliceUp];  \
    upShift = (shift + slice) % SlMemMm_n##type##Slices;  \
    if (upShift < 0) upShift += SlMemMm_n##type##Slices;  \
  }  \
  if (shift) *_slice = upShift;  \
  return &(**(SlMem##Mm_##type##Page **) _page)[upShift];  \
}

#define SlMemMm_TypePage_headerSlices(SlMem, type)  \
  ( granul(sizeof(SlMem##Mm_##type##PageHeader), sizeof(SlMem##Mm_##type##Slice)) )

#define SlMemMm_TypePage_slicesFreeAll(SlMem, type)  \
  ( SlMemMm_n##type##Slices - SlMemMm_TypePage_headerSlices(SlMem, type) )

#define SlMemMm_TypePage_alloc(SlMem, type)  \
SlMem##Mm_##type##Page* SlMem##Mm_##type##Page_alloc(void) {  \
  if (!SlMem##Mm_malloc) return 0;  \
  \
  SlMem##Mm_##type##PageHeader *page = (SlMem##Mm_##type##PageHeader *) SlMem##Mm_malloc(sizeof(SlMem##Mm_##type##Page));  \
  if (!page) return 0;  \
  page->pageUp = 0;  \
  \
  REM( "Set shadow for page header" )  \
  bitSet( \
    &page->shadow,  \
    0,  \
    SlMemMm_TypePage_headerSlices(SlMem, type),  \
    1, 1 \
  );  \
  \
  REM( "Reset shadow of all available slices" )  \
  bitSet(&page->shadow,  \
    SlMemMm_TypePage_headerSlices(SlMem, type),  \
    SlMemMm_n##type##Slices,  \
    0,1);  \
  \
  REM( "Set counter of free slices" )  \
  page->slicesFree =  \
  page->branchesFree = SlMemMm_TypePage_slicesFreeAll(SlMem, type);  \
  \
  REM( "For leafages, reset all branches" )  \
  \
  if (SlMemMm_Page_of##type == SlMemMm_Page_ofLeaf) {  \
    bitSet( \
      &(*(SlMem##Mm_##type## Page *) page)[SlMemMm_TypePage_headerSlices(SlMem, type)],  \
      0,  \
      SlMemMm_TypePage_slicesFreeAll(SlMem, type) * SlMemMm_n##type##Chars * charBit,  \
      0, 1 \
    );  \
  }  \
  \
  return (SlMem##Mm_##type##Page *)page;  \
}

// ============ CODE BUILDER
#define SlMemMm_C_(SlMem)  \
SlMemMm_H(SlMem)  \
\
/* *** DEFINITIONS*/\
\
/* Upstream */\
static void *(*SlMem##Mm_malloc)(UInt) = SlMemMm_mAlloc;  \
static void (*SlMem##Mm_free)(void*) = SlMemMm_mFree;  \
\
void SlMem##Mm_setUpstreamAlloc(void *(*malloc)(UInt), void (*free)(void*) ) {  \
  SlMem##Mm_malloc = malloc;  \
  SlMem##Mm_free = free;  \
}  \
\
/* Slice */\
\
typedef Char SlMem##Mm_DataSlice [SlMemMm_nDataChars];  \
typedef void *SlMem##Mm_LeafSlice;  \
\
/* Page */\
\
typedef SlMem##Mm_DataSlice SlMem##Mm_DataPage[SlMemMm_nDataSlices];  \
typedef SlMem##Mm_LeafSlice SlMem##Mm_LeafPage[SlMemMm_nLeafSlices];  \
\
/* PageHeader */\
\
SlMemMm_TypePageHeader(SlMem, Leaf);  \
SlMemMm_TypePageHeader(SlMem, Data);  \
\
/* Page_alloc */\
\
SlMemMm_TypePage_alloc(SlMem, Leaf);  \
SlMemMm_TypePage_alloc(SlMem, Data);  \
\
SlMemMm_TypeSlice_alloc(SlMem, Leaf);  \
SlMemMm_TypeSlice_alloc(SlMem, Data);  \
\
__attribute__((__visibility__("default")))  \
void SlMem##Mm_DataSlice_allocOccupy(void **_page, unsigned  *_slice, Int *_shift) {  \
  SlMem##Mm_DataSlice_alloc((SlMem##Mm_DataPage **)_page, _slice, _shift);  \
  SlMemMm_Slice_occupy(SlMem, *_page, *_slice);  \
}  \
\
__attribute__((__visibility__("default")))  \
void SlMem##Mm_DataSlice_allocRelease(void *_page, unsigned _slice) {  \
  SlMem##Mm_LeafPageHeader *page = (SlMem##Mm_LeafPageHeader *) _page;  \
  SlMem##Mm_LeafPage *pageUp = page->pageUp;  \
  unsigned sliceUp = page->sliceUp, slice;  \
  \
  SlMemMm_Slice_release(SlMem, _page, _slice);  \
  SlMemMm_TypeSlice_freeIfEmpty(SlMem, _page, Data);  \
  \
  while (pageUp) {  \
    page = (SlMem##Mm_LeafPageHeader *) pageUp;  \
    if (SlMem##Mm_free) {  \
      SlMemMm_Slice_release(SlMem, pageUp, sliceUp);  \
    } else {  \
      Char  \
        *sha = &bitOffset(&page->shadow,sliceUp),  \
        shv = bitValue(sliceUp);  \
      if (*sha & shv) {  \
        *sha &= -1 ^ shv;  \
        ++page->slicesFree;  \
      } else break;  \
    }  \
    pageUp = page->pageUp;  \
    sliceUp = page->sliceUp;  \
    SlMemMm_TypeSlice_freeIfEmpty(SlMem, pageUp, Leaf);  \
  }  \
}  \
\
SlMemMm_TypeSlice_locate(SlMem, Leaf);  \
SlMemMm_TypeSlice_locate(SlMem, Data);  \
\
/* Slice_length */\
\
__attribute__((__visibility__("default")))  \
unsigned SlMem##Mm_DataSlice_lengthBits() {  \
  return SlMemMm_nDataCharsBits;  \
}  \
__attribute__((__visibility__("default")))  \
unsigned SlMem##Mm_DataSlice_length() {  \
  return SlMemMm_nDataChars;  \
}

#define SlMemMm_C(name)  SlMemMm_C_(name)

SlMemMm_C(SlMem)
