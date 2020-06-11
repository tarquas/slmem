// *** DEFAULTS

#ifndef SlMemMm_nDataCharsBits
#define SlMemMm_nDataCharsBits  5
#endif

// log2 of data slices per page (must be >= 3)
#ifndef SlMemMm_nDataSlicesBits
#define SlMemMm_nDataSlicesBits  (16 - SlMemMm_nDataCharsBits)
#endif

#ifndef SlMemMm_nLeafSlicesBits
#define SlMemMm_nLeafSlicesBits  13
#endif

#ifndef SlMemMm_mAlloc
#include <stdlib.h>
#define SlMemMm_mAlloc  malloc
#define SlMemMm_mFree   free
#endif

#ifndef SlMemMm_mFree
#define SlMemMm_mFree   NULL
#endif

// *** EXPRESSIONS

// Page types

#define SlMemMm_Page_ofLeaf  0
#define SlMemMm_Page_ofData  1

// Slice

#define SlMemMm_nDataChars  (1 << (SlMemMm_nDataCharsBits))
#define SlMemMm_nLeafChars  sizeofInt

// Page

#define SlMemMm_nDataSlices (1 << (SlMemMm_nDataSlicesBits))
#define SlMemMm_nLeafSlices (1 << (SlMemMm_nLeafSlicesBits))

#define SlMemMm_DataPageAlignBits  (SlMemMm_nDataCharsBits + SlMemMm_nDataSlicesBits)
#define SlMemMm_LeafPageAlignBits  (log2sizeofInt + SlMemMm_nLeafSlicesBits)

#define SlMemMm_DataPageAlign  (1 << (SlMemMm_DataPageAlignBits))
#define SlMemMm_LeafPageAlign  (1 << (SlMemMm_LeafPageAlignBits))
