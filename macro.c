// *** HELPERS

#include "types.h"

#define bitSetOne(ptr,offset,bitval) {  \
  if (bitval)  \
    bitOffset(ptr,offset) |= bitValue(offset);  \
  else  \
    bitOffset(ptr,offset) &= ((Char) -1) ^ bitValue(offset);  \
}

#define bitSetChar(ptr,offset,end,bitval,opaque)  if ((end) > (offset)) {  \
  if (!( ((end) ^ (offset)) / charBit )) {  \
    Char  \
      *chead = &( bitOffset(ptr,offset) );  \
    \
    unsigned  \
      latchStart = (offset) % charBit,  \
      latchEnd = (end) % charBit;  \
    \
    if (bitval) {  \
      if (opaque)  \
        *(UChar *)chead =  \
          (((UChar) 1 << (charBit - latchStart)) - 1) &  \
          (UChar)((Char) -1 << (charBit - latchEnd));  \
      else  \
        *(UChar *)chead |=  \
          (((UChar) 1 << (charBit - latchStart)) - 1) &  \
          (UChar)((Char) -1 << (charBit - latchEnd));  \
    } else {  \
      if (opaque)  \
        *(UChar *)chead =  \
          (UChar)((Char) -1 << (charBit - latchStart)) |  \
          (((UChar)1 << (charBit - latchEnd)) - 1);  \
      else  \
        *(UChar *)chead &=  \
          (UChar)((Char) -1 << (charBit - latchStart)) |  \
          (((UChar) 1 << (charBit - latchEnd)) - 1);  \
    }  \
  } else {  \
    Char  \
      *i,  \
      *chead = (Char *)(ptr) + ((offset) / charBit),  \
      *ctail,  \
      cbitval = (bitval) ? (Char) -1 : (Char) 0;  \
    \
    unsigned  \
      bits,  \
      latchBits;  \
    \
    if (( latchBits = (offset) % charBit )) {  \
      if (bitval) {  \
        if (opaque)  \
          *(UChar *) chead = ((UChar) 1 << (charBit - latchBits)) - 1;  \
        else  \
          *(UChar *) chead |= ((UChar) 1 << (charBit - latchBits)) - 1;  \
      } else {  \
        if (opaque)  \
          *(Char *) chead = ((Char) -1 << (charBit - latchBits));  \
        else  \
          *(Char *) chead &= ((Char) -1 << (charBit - latchBits));  \
      }  \
      ++chead;  \
    }  \
    \
    bits = (end) - (chead - (Char *)(ptr)) * charBit;  \
    \
    for (  \
      i = (  \
        ctail = chead + (bits / charBit)  \
      ) - 1;  \
      i >= chead;  \
      --i  \
    ) *i = (cbitval);  \
    \
    if (( latchBits = bits % charBit )) {  \
      if (bitval) {  \
        if (opaque)  \
          *(Char *)ctail = ((Char) -1 << (charBit - latchBits));  \
        else  \
          *(Char *)ctail |= ((Char) -1 << (charBit - latchBits));  \
      } else {  \
        if (opaque)  \
          *(UChar *)ctail = ((UChar)1 << (charBit - latchBits)) - 1;  \
        else  \
          *(UChar *)ctail &= ((UChar)1 << (charBit - latchBits)) - 1;  \
      }  \
    }  \
  }  \
}

#define bitSet(ptr,offset,end,bitval,opaque)  if ((end) > (offset)) {  \
  if (!( ((end) ^ (offset)) / (sizeof(Int) * charBit) )) {  \
    bitSetChar(ptr, offset, end, bitval, opaque);  \
  } else {  \
    Char  \
      *i,  \
      *lhead = (Char*)(ptr) + ((offset) / (charBit * sizeof(Int))) * sizeof(Int),  \
      *chead = (Char*)(ptr) + ((offset) / charBit),  \
      *ltail,  \
      *ctail,  \
      cbitval = (bitval) ? (Char) -1 : (Char) 0;  \
    \
    Int  \
      lbitval = (bitval) ? (Int) -1 : (Int) 0;  \
    \
    unsigned  \
      latchBits,  \
      latchBytes,  \
      bits;  \
    \
    if (( latchBits = (offset) % charBit )) {  \
      if (bitval) {  \
        if (opaque)  \
          *(UChar *) chead = ((UChar) 1 << (charBit - latchBits)) - 1;  \
        else  \
          *(UChar *) chead |= ((UChar) 1 << (charBit - latchBits)) - 1;  \
      } else {  \
        if (opaque)  \
          *(Char *) chead = ((Char) -1 << (charBit - latchBits));  \
        else  \
          *(Char *) chead &= ((Char) -1 << (charBit - latchBits));  \
      }  \
      \
      ++chead;  \
      lhead += sizeof(Int);  \
    }  \
    \
    if (( latchBytes = bitsChars(offset) % sizeof(Int) )) {  \
      for ( i = lhead + latchBytes - sizeof(Int) ; i < lhead ; ++i )  \
        *i = cbitval;  \
    }  \
    \
    bits = (end) - (lhead - (Char *)(ptr)) * charBit;  \
    \
    for (  \
      i = (  \
        ltail = lhead + ( bits / (charBit * sizeof(Int)) ) * sizeof(Int)  \
      ) - sizeof(Int);  \
      \
      i >= lhead;  \
      i -= sizeof(Int)  \
    ) *(Int *) i = lbitval;  \
    \
    for (  \
      i = (  \
        ctail = ltail + ( bits % (charBit * sizeof(Int)) ) / charBit  \
      ) - 1;  \
      \
      i >= ltail;  \
      --i  \
    ) *i = cbitval;  \
    \
    if (( latchBits = bits % charBit )) {  \
      if (bitval) {  \
        if (opaque)  \
          *(Char *) ctail = ((Char) -1 << (charBit - latchBits));  \
        else  \
          *(Char *) ctail |= ((Char) -1 << (charBit - latchBits));  \
      } else {  \
        if (opaque)  \
          *(UChar *) ctail = ((UChar) 1 << (charBit - latchBits)) - 1;  \
        else  \
          *(UChar *) ctail &= ((UChar) 1 << (charBit - latchBits)) - 1;  \
      }  \
    }  \
  }  \
}

#define REM(...)

#define PStruct(name, def)  typedef struct __attribute__((__packed__)) name { ARG def } name
#define Struct(name, def)  typedef struct name { ARG def } name

#define QtyV_nTailOfLog(l) (((l<<2) + (l<<5) + (l<<8) + l) >> 11)
#define IntV_nTailOfLog(l) (((l<<2) + (l<<5) + (l<<8) + l + 293) >> 11)

#define QtyV_nTailOfUInt(l, n) { l = log2u(n); l = QtyV_nTailOfLog(l); }
#define IntV_nTailOfInt(l, n) { l = log2(n); l = IntV_nTailOfLog(l); }

#define IntV_nTailByHdrChar(hdr) clrSb((Int) (hdr) << ((sizeof(Int) - sizeof(hdr)) * charBit))
#define IntV_lenByHdrChar(hdr) (IntV_nTailByHdrChar(hdr) + 1)

#define IntV_hdrAndMaskByLen(len) (Int)(((Int) 1 << (sizeof(Int) * charBit - (len))) - 1)
#define IntV_hdrOrMaskByLen(len) (Int)(((Int) -1) << (sizeof(Int) * charBit - (len)))
