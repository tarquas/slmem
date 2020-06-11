// Slice Memory (SlMem) Manager
// (c) 2014-2020. Prywit Research Labs <http://pwnz.pro/prywit>, Taras Mykhailovych <tarquas@gmail.com>

// *** IMPLEMENTATION


#include "types.h"
#include "mm-data.h"
#include "macro.c"
#include "mm-config.c"

#include <string.h>

#define SlMemMmData_MemPtr_getSlice(SlMem, memPtr) (*((SlMem##Mm_DataPage *)(memPtr)->page)[(memPtr)->slice])
#define SlMemMmData_Slice_isSticky(slice) (*(UChar *)(slice) & (1 << (charBit - 1)))
#define SlMemMmData_Slice_getCount(SlMem, slice) (*(UChar *)(slice) & (SlMem##Mm_nDataSlices - 1))
#define SlMemMmData_Slice_getTailPtr(SlMem, slice) ((Char *)(slice) + SlMem##Mm_nDataSlices - 1)

#define SlMemMmData_DataSlice_maxRelocHdrLen  Min(SlMemMm_nDataChars, sizeof(Int) + 2)

// ============ CODE BUILDER
#define SlMemMmData_C_(SlMem)  \
SlMemMmData_H(SlMem)  \
\
Int SlMem##MmData_alloc(SlMemPtr *p, SlMemPtr *after) {  \
  Int shift, afterReloc, nTrans;  \
  Char ntailShift, i, *data, *afterData, *afterTail, *q;  \
  \
  if (after) {  \
    if (p != after) {  \
      p->_page = after->_page;  \
      p->_slice = after->_slice;  \
    }  \
    \
    afterData = (Char *) SlMem##Mm_DataSlice_locate(&p->_page, &p->_slice, 0);  \
    afterTail = afterData + SlMemMm_nDataChars - 1;  \
    afterReloc = SlMem##MmData_IntVR_read(&afterTail);  \
  } else {  \
    p->_page = 0;  \
    afterReloc = 0;  \
  }  \
  \
  SlMem##Mm_DataSlice_allocOccupy(&p->_page, &p->_slice, &shift);  \
  data = SlMem##Mm_DataSlice_locate(&p->_page, &p->_slice, 0);  \
  IntV_nTailOfInt(ntailShift, shift);  \
  \
  if (after) {  \
    /* TODO: nTrans -- number of Data chars to be transitioned */\
    /* to Next Data Slice */\
    nTrans = (  \
      (Int) *afterData -  \
      (SlMemMm_nDataChars - 2) +  \
      (Int) ntailShift  \
    );  \
    \
    if (nTrans > 0) {  \
      afterData[0] -= (UChar) nTrans;  \
      /* OLD: q = data + nTrans; i = nTrans; while (i--) *(q--) = *(afterTail --); */\
      memcpy(data + 1, afterTail - nTrans + 1, nTrans);  \
    } else {  \
      nTrans = 0;  \
    }  \
    \
    afterTail = afterData + SlMemMm_nDataChars - 1;  \
    SlMem##MmData_IntVR_write(&afterTail, shift, &ntailShift);  \
  } else {  \
    nTrans = 0;  \
  }  \
  \
  q = data + SlMemMm_nDataChars - 1;  \
  SlMem##MmData_IntVR_write(&q, afterReloc ? afterReloc - shift : 0, 0);  \
  \
  *data = (UChar) nTrans; /* taken */\
  p->ptr = data + nTrans + 1;  \
  p->_slicePtr = data;  \
  return shift;  \
}  \
\
Int SlMem##MmData_IntVR_read(Char **intVR) {  \
  Int value;  \
  Char length, bias;  \
  \
  value = LittleEndianInt(*(Int *)(*intVR - sizeof(Int) + 1));  \
  length = IntV_lenByHdrChar(value);  \
  \
  if (length > SlMemMmData_DataSlice_maxRelocHdrLen) {  \
    *intVR = 0;  \
    return 0; /* not-supported */\
  }  \
  \
  *intVR -= length;  \
  \
  if (value >= 0) value |= IntV_hdrOrMaskByLen(length);  \
  else value &= IntV_hdrAndMaskByLen(length);  \
  \
  bias = length - sizeof(Int);  \
  \
  if (bias < 0) value >>= -bias * charBit;  \
  else if (bias == 1) value = (value << charBit) + (*(UChar *)((*intVR) + 1));  \
  else if (bias == 2) value = (value << (charBit * 2)) + LittleEndian16(*(UShort *)((*intVR) + 1));  \
  \
  return value;  \
}  \
\
/*
Int SlMem##MmData_IntVR_read(Char **intV) {
  Char tail, length;
  Int value;

  tail = *((*intV)--);

  if ((UChar) tail == 0x80) return 0;

  length = IntV_lenByHdrChar(tail);

  while (!(length % charBit) && (tail ^ (tail = *((*intV)--))) >= 0) {
    length += IntV_nTailByHdrChar(tail);
  }

  if (length - 1 <= sizeof(Int)) {
    if (tail >= 0) value = (Int) (Char) (tail | IntV_hdrOrMaskByLen(length));
    else value = (Int) (Char) (tail & IntV_hdrAndMaskByLen(length));

    while (-- length) value = (value << charBit) + ((UChar) *((*intV)--)); //TODO: OPT
    return value;
  }

  // value out of platform bounds
  *intV = 0;
  return 0;
}
*/\
\
void SlMem##MmData_IntVR_write(Char **intVR, Int value, Char *ntailp) {  \
  Int tmp, mask;  \
  Char ntail, bias, shift, length;  \
  Int *ptr;  \
  Char latchChar;  \
  Short latchShort;  \
  \
  if (!ntailp) {  \
    IntV_nTailOfInt(ntail, value);  \
  } else if ((ntail = *ntailp) < 0) {  \
    IntV_nTailOfInt(ntail, value);  \
    *ntailp = ntail;  \
  }  \
  \
  ptr = (Int *) (*intVR - sizeof(Int) + 1);  \
  length = ntail + 1;  \
  bias = length - sizeof(Int);  \
  \
  if (bias < 0) {  \
    shift = -bias * charBit;  \
    mask = ((Int) 1 << shift) - 1;  \
    value <<= shift;  \
  } else if (bias == 1) {  \
    latchChar = (Char) value;  \
    value >>= charBit;  \
  } else if (bias == 2) {  \
    latchShort = (Short) value;  \
    value >>= charBit * 2;  \
  }  \
  \
  if (value < 0) value &= IntV_hdrAndMaskByLen(length);  \
  else value |= IntV_hdrOrMaskByLen(length);  \
  \
  if (bias < 0) {  \
    tmp = *ptr;  \
    tmp &= LittleEndianInt(mask);  \
    tmp |= LittleEndianInt(value);  \
    *ptr = tmp;  \
  } else {  \
    *ptr = LittleEndianInt(value);  \
    if (bias == 1) *((Char *)ptr - 1) = latchChar;  \
    else if (bias == 2) *(Short *)((Char *)ptr - 2) = LittleEndian16(latchShort);  \
  }  \
  \
  *intVR -= length;  \
}  \
\
void *SlMem##MmData_MemPtr_init(SlMemPtr *p, Char *start) {  \
  if (!start) start = SlMem##Mm_DataSlice_locate(&p->_page, &p->_slice, 0);  \
  p->_slicePtr = start;  \
  p->ptr = start + 1;  \
  return start;  \
}  \
\
void *SlMem##MmData_MemPtr_reset(SlMemPtr *p) {  \
  p->ptr = p->_slicePtr + 1;  \
}  \
\
Int SlMem##MmData_sliceNext(SlMemPtr *p, Int shift) {  \
  Char *start, *end, tail, digits;  \
  \
  if (!shift) {  \
    start = p->_slicePtr;  \
    end = start + SlMemMm_nDataChars - 1;  \
    shift = SlMem##MmData_IntVR_read(&end);  \
    \
    if (!end) {  \
      /* TODO: special case: big data */\
      return 0; /*not implemented */\
    }  \
  }  \
  \
  if (!shift) return 0; /* EOF */\
  void *data = SlMem##Mm_DataSlice_locate(&p->_page, &p->_slice, shift);  \
  if (data) SlMem##MmData_MemPtr_init(p, (Char *) data);  \
  return data ? shift : 0;  \
}  \
\
UInt SlMem##MmData_read(SlMemPtr *p, void *dst, UInt count) {  \
  Int offset, taken, avail, shift;  \
  UInt read = 0;  \
  \
  if (count) do {  \
    offset = (UChar *) p->ptr - p->_slicePtr - 1;  \
    taken = *p->_slicePtr;  \
    avail = taken - offset;  \
    \
    if (avail <= 0) {  \
      shift = SlMem##MmData_sliceNext(p, 0);  \
      if (!shift) break;  \
      p->ptr -= avail;  \
      continue;  \
    }  \
    \
    Mins(avail, count);  \
    \
    if (dst) {  \
      memcpy(dst, p->ptr, avail);  \
      dst += avail;  \
    }  \
    \
    read += avail;  \
    count -= avail;  \
    p->ptr += avail;  \
    \
    if (!count) break;  \
    shift = SlMem##MmData_sliceNext(p, 0);  \
  } while (shift);  \
  \
  return read;  \
}  \
\
UInt SlMem##_MmData_replace(SlMemPtr *p,  \
  void **dst, UInt *_nRemove,  \
  void **src, UInt *_nInsert,  \
  UInt *_nReplace  \
) {  \
  UInt nRemove, nInsert, nReplace;  \
  Int offset, taken, replace, latch, insert, hdr;  \
  Char *end;  \
  UInt discarded = 0;  \
  \
  nRemove = *_nRemove;  \
  nInsert = *_nInsert;  \
  nReplace = Min(nRemove, nInsert);  \
  \
  if (nReplace) {  \
    do {  \
      /* end = p->_slicePtr + SlMem##Mm_DataSlice_nChars - 1; */\
      /* shift = SlMem##MmData_IntVR_read(&end); */\
      end = p->_slicePtr + SlMemMm_nDataChars - 1;  \
      hdr = LittleEndianInt(*(Int *)(end - sizeof(Int) + 1));  \
      end -= IntV_nTailByHdrChar(hdr);  \
      \
      if (!end) {  \
        /* TODO: big data */\
        return discarded; /* not implemented */\
      }  \
      \
      offset = (UChar *) p->ptr - p->_slicePtr - 1;  \
      taken = *p->_slicePtr;  \
      replace = taken - offset;  \
      Mins(replace, nReplace);  \
      \
      if (replace > 0) {  \
        if (*dst) { memcpy(*dst, p->ptr, replace); *dst += replace; }  \
        discarded += replace;  \
        nRemove -= replace;  \
        nReplace -= replace;  \
      }  \
      \
      latch = (UChar *) end - p->_slicePtr - taken - 1;  \
      Mins(latch, nReplace);  \
      insert = latch + replace;  \
      \
      if (insert > 0) {  \
        if (*src) { memcpy(p->ptr, *src, insert); *src += insert; }  \
        else memset(p->ptr, 0, insert);  \
        *p->_slicePtr += latch;  \
        p->ptr += insert;  \
        nInsert -= insert;  \
        nReplace -= latch;  \
      }  \
    } while (nReplace && SlMem##MmData_sliceNext(p, 0));  \
    \
    *_nRemove = nRemove;  \
    *_nInsert = nInsert;  \
  }  \
  \
  *_nReplace = nReplace;  \
  return discarded;  \
}  \
\
Int  SlMem##_MmData_nextSqueeze(SlMemPtr *p, SlMemPtr *prev, Int shift, Int ptrToNext) {  \
  Int taken, remove, rest, nextShift;  \
  Char ntail, len, *end;  \
  void *page;  \
  unsigned slice;  \
  \
  end = p->_slicePtr + SlMemMm_nDataChars - 1;  \
  nextShift = SlMem##MmData_IntVR_read(&end);  \
  \
  if (prev && prev->_slicePtr) {  \
    taken = *prev->_slicePtr;  \
    rest = SlMemMm_nDataChars - taken - 1;  \
    remove = *p->_slicePtr;  \
    \
    if (nextShift) {  \
      if (!shift) {  \
        end = prev->_slicePtr + SlMemMm_nDataChars - 1;  \
        shift = SlMem##MmData_IntVR_read(&end) + nextShift;  \
      } else {  \
        shift += nextShift;  \
      }  \
    } else {  \
      shift = 0;  \
    }  \
    \
    IntV_nTailOfInt(ntail, shift);  \
    len = ntail + 1;  \
    \
    if (remove + len <= rest) {  \
      page = p->_page;  \
      slice = p->_slice;  \
      end = p->_slicePtr;  \
      if (nextShift) SlMem##MmData_sliceNext(p, nextShift);  \
      \
      if (remove) {  \
        memcpy(prev->_slicePtr + taken + 1, end + 1, remove);  \
        *prev->_slicePtr += remove;  \
      }  \
      \
      end = prev->_slicePtr + SlMemMm_nDataChars - 1;  \
      SlMem##MmData_IntVR_write(&end, shift, &ntail);  \
      SlMem##Mm_DataSlice_allocRelease(page, slice);  \
      \
      /*if (!ptrToNext) {  // ???
        *p = *prev;
        p->ptr = p->_slicePtr + *p->_slicePtr + 1;
      }*/\
      \
      return shift;  \
    }  \
  }  \
  \
  if (!nextShift) return 0;  \
  *prev = *p;  \
  if (ptrToNext) SlMem##MmData_sliceNext(p, nextShift);  \
  return nextShift;  \
}  \
\
UInt SlMem##_MmData_remove(SlMemPtr *p,  \
  void **dst, UInt *_nRemove  \
) {  \
  UInt nRemove;  \
  Char buf[SlMemMm_nDataChars];  \
  Int offset, taken, remove, rest, shift;  \
  SlMemPtr prev;  \
  UInt discarded = 0;  \
  \
  prev._slicePtr = NULL;  \
  nRemove = *_nRemove;  \
  \
  if (nRemove) {  \
    do {  \
      offset = (UChar *) p->ptr - p->_slicePtr - 1;  \
      taken = *p->_slicePtr;  \
      rest = taken - offset;  \
      remove = Min(rest, nRemove);  \
      rest -= remove;  \
      \
      if (remove > 0) {  \
        if (*dst) { memcpy(*dst, p->ptr, remove); *dst += remove; }  \
        discarded += remove;  \
        nRemove -= remove;  \
        \
        if (rest > 0) {  \
          memcpy(buf, p->ptr + remove, rest);  \
          memcpy(p->ptr, buf, rest);  \
        }  \
        \
        *p->_slicePtr -= remove;  \
      }  \
      \
      if (!nRemove) break;  \
      shift = SlMem##_MmData_nextSqueeze(p, &prev, shift, nRemove);  \
    } while (nRemove && shift);  \
    \
    /* if (shift) SlMem##_MmData_nextSqueeze(p, &prev, shift, nRemove); */\
    *_nRemove = nRemove;  \
  }  \
  \
  return discarded;  \
}  \
\
UInt SlMem##_MmData_append(SlMemPtr *p,  \
  void **src, UInt *_nInsert  \
) {  \
  UInt nInsert;  \
  Int hdr, avail, latch, insert, shift;  \
  Char *end;  \
  UInt inserted = 0;  \
  \
  nInsert = *_nInsert;  \
  \
  if (nInsert) {  \
    do {  \
      end = p->_slicePtr + SlMemMm_nDataChars - 1;  \
      hdr = LittleEndianInt(*(Int *)(end - sizeof(Int) + 1));  \
      end -= IntV_nTailByHdrChar(hdr);  \
      \
      avail = end - p->ptr;  \
      insert = Min(avail, nInsert);  \
      \
      if (insert > 0) {  \
        if (*src) { memcpy(p->ptr, *src, insert); *src += insert; }  \
        else memset(p->ptr, 0, insert);  \
        *p->_slicePtr += insert;  \
        p->ptr += insert;  \
        nInsert -= insert;  \
      }  \
    } while (nInsert && SlMem##MmData_alloc(p, p));  \
    \
    *_nInsert = nInsert;  \
  }  \
  \
  return inserted;  \
}  \
\
UInt SlMem##_MmData_insert(SlMemPtr *p,  \
  void **src, UInt *_nInsert  \
) {  \
  UInt nInsert;  \
  Char buf[SlMemMm_nDataChars];  \
  Int offset, taken, rest, shift;  \
  Char *pbuf = buf;  \
  UInt inserted = 0;  \
  SlMemPtr tmp;  \
  \
  nInsert = *_nInsert;  \
  \
  if (nInsert) {  \
    offset = (UChar *) p->ptr - p->_slicePtr - 1;  \
    taken = *p->_slicePtr;  \
    rest = taken - offset;  \
    \
    if (rest > 0) {  \
      memcpy(buf, p->ptr, rest);  \
      *p->_slicePtr -= rest;  \
    }  \
    \
    inserted += SlMem##_MmData_append(p, src, &nInsert);  \
    tmp = *p;  \
    SlMem##_MmData_append(&tmp, (void **) &pbuf, &rest);  \
    \
    *_nInsert = nInsert;  \
  }  \
  \
  return inserted;  \
}  \
\
UInt SlMem##MmData_splice(SlMemPtr *p, void *dst, UInt nRemove, void *src, UInt nInsert) {  \
  UInt nReplace;  \
  UInt discarded = 0;  \
  \
  discarded += SlMem##_MmData_replace(p, &dst, &nRemove, &src, &nInsert, &nReplace);  \
  if (!nReplace) discarded += SlMem##_MmData_remove(p, &dst, &nRemove);  \
  SlMem##_MmData_insert(p, &src, &nInsert);  \
  \
  return discarded;  \
}

/*
char SlMem##_MMData_common(SlMem##_MMData_MemPtr *p1, SlMem##_MMData_MemPtr *p2, long length) {
  char c = p1->count;
  char c2 = p2->count;
  if (c2 < c) c = c2;
  if (length < (long)c) return (char)length;
  return c;
}

char SlMem##_MMData_skip(SlMem##_MMData_MemPtr *p, long length) {

  return c;
}
*/


//TODO: SlMem##_MMData_copy
//TODO: makeSticky

#define SlMemMmData_C(SlMem)  SlMemMmData_C_(SlMem)

SlMemMmData_C(SlMem)
