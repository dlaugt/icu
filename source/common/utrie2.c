/*
******************************************************************************
*
*   Copyright (C) 2001-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  utrie2.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2008aug16 (starting from a copy of utrie.c)
*   created by: Markus W. Scherer
*
*   This is a common implementation of a Unicode trie.
*   It is a kind of compressed, serializable table of 16- or 32-bit values associated with
*   Unicode code points (0..0x10ffff).
*   This is the second common version of a Unicode trie (hence the name UTrie2).
*   See utrie2.h for a comparison.
*/
#ifdef UTRIE2_DEBUG
#   include <stdio.h>
#endif

#include "unicode/utypes.h"
#include "cmemory.h"
#include "utrie2.h"

#include "utrie.h" /* TODO: move with utrie2_fromUTrie() (and getVersion()?) */

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/* Implementation notes ----------------------------------------------------- */

/*
 * The UTRIE2_SHIFT_1, UTRIE2_SHIFT_2, UTRIE2_INDEX_SHIFT and other values
 * have been chosen to minimize trie sizes overall.
 * Most of the code is flexible enough to work with a range of values,
 * within certain limits.
 *
 * Requires UTRIE2_SHIFT_2<=6. Otherwise 0xc0 which is the top of the ASCII-linear data
 * including the bad-UTF-8-data block is not a multiple of UTRIE2_DATA_BLOCK_LENGTH
 * and map[block>>UTRIE2_SHIFT_2] (used in reference counting and compaction
 * remapping) stops working.
 *
 * Requires UTRIE2_SHIFT_1>=10 because unewtrie2_enumForLeadSurrogate()
 * assumes that a single index-2 block is used for 0x400 code points
 * corresponding to one lead surrogate.
 *
 * Requires UTRIE2_SHIFT_1<=16. Otherwise one single index-2 block contains
 * more than one Unicode plane, and the split of the index-2 table into a BMP
 * part and a supplementary part, with a gap in between, would not work.
 *
 * Requires UTRIE2_INDEX_SHIFT>=1 not because of the code but because
 * there is data with more than 64k distinct values,
 * for example for Unihan collation with a separate collation weight per
 * Han character.
 */

/* Public UTrie2 API implementation ----------------------------------------- */

/*
 * UTrie and UTrie2 signature values,
 * in platform endianness and opposite endianness.
 */
#define UTRIE_SIG       0x54726965
#define UTRIE_OE_SIG    0x65697254

#define UTRIE2_SIG      0x54726932
#define UTRIE2_OE_SIG   0x32697254

/**
 * Trie data structure in serialized form:
 *
 * UTrie2Header header;
 * uint16_t index[header.index2Length];
 * uint16_t data[header.shiftedDataLength<<2];  -- or uint32_t data[...]
 * @internal
 */
typedef struct UTrie2Header {
    /** "Tri2" in big-endian US-ASCII (0x54726932) */
    uint32_t signature;

    /**
     * options bit field:
     * 15.. 4   reserved (0)
     *  3.. 0   UTrie2ValueBits valueBits
     */
    uint16_t options;

    /** UTRIE2_INDEX_1_OFFSET..UTRIE2_MAX_INDEX_LENGTH */
    uint16_t indexLength;

    /** (UTRIE2_DATA_START_OFFSET..UTRIE2_MAX_DATA_LENGTH)>>UTRIE2_INDEX_SHIFT */
    uint16_t shiftedDataLength;

    /** Null index and data blocks, not shifted. */
    uint16_t index2NullOffset, dataNullOffset;

    /**
     * First code point of the single-value range ending with U+10ffff,
     * rounded up and then shifted right by UTRIE2_SHIFT_1.
     */
    uint16_t shiftedHighStart;
} UTrie2Header;

/**
 * Constants for use with UTrie2Header.options.
 * @internal
 */
enum {
    /** Mask to get the UTrie2ValueBits valueBits from options. */
    UTRIE2_OPTIONS_VALUE_BITS_MASK=0xf
};

static U_INLINE int32_t
u8Index(const UTrie2 *trie, UChar32 c, int32_t i) {
    int32_t index;
    if(c>=0) {
        index=_UTRIE2_INDEX_FROM_CP(trie, c);
    } else {
        index=UTRIE2_BAD_UTF8_DATA_OFFSET;
        if(trie->data32==NULL) {
            index+=trie->indexLength;  /* 16-bit trie */
        }
    }
    return (index<<3)|i;
}

U_CAPI int32_t U_EXPORT2
utrie2_internalU8NextIndex(const UTrie2 *trie, UChar32 c,
                           const uint8_t *src, const uint8_t *limit) {
    int32_t i, length;
    i=0;
    /* support 64-bit pointers by avoiding cast of arbitrary difference */
    if((limit-src)<=7) {
        length=(int32_t)(limit-src);
    } else {
        length=7;
    }
    c=utf8_nextCharSafeBody(src, &i, length, c, -1);
    return u8Index(trie, c, i);
}

U_CAPI int32_t U_EXPORT2
utrie2_internalU8PrevIndex(const UTrie2 *trie, UChar32 c,
                           const uint8_t *start, const uint8_t *src) {
    int32_t i, length;
    /* support 64-bit pointers by avoiding cast of arbitrary difference */
    if((src-start)<=7) {
        i=length=(int32_t)(src-start);
    } else {
        i=length=7;
        start=src-7;
    }
    c=utf8_prevCharSafeBody(start, 0, &i, c, -1);
    i=length-i;  /* number of bytes read backward from src */
    return u8Index(trie, c, i);
}

#ifdef UTRIE2_DEBUG
U_CAPI void U_EXPORT2
utrie_printLengths(const UTrie *trie);

U_CAPI void U_EXPORT2
utrie2_printLengths(const UTrie2 *trie, const char *which) {
    long indexLength=trie->indexLength;
    long dataLength=(long)trie->dataLength;
    long totalLength=(long)sizeof(UTrie2Header)+indexLength*2+dataLength*(trie->data32!=NULL ? 4 : 2);
    printf("**UTrie2Lengths(%s)** index:%6ld  data:%6ld  serialized:%6ld\n",
           which, indexLength, dataLength, totalLength);
}
#endif

U_CAPI int32_t U_EXPORT2
utrie2_unserialize(UTrie2 *trie, UTrie2ValueBits valueBits,
                   const void *data, int32_t length, UErrorCode *pErrorCode) {
    const UTrie2Header *header;
    const uint16_t *p16;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( trie==NULL ||
        length<=0 || (((int32_t)data&3)!=0) ||
        valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* enough data for a trie header? */
    if(length<sizeof(UTrie2Header)) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    /* check the signature */
    header=(const UTrie2Header *)data;
    if(header->signature!=UTRIE2_SIG) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    /* get the options */
    if(valueBits!=(UTrie2ValueBits)(header->options&UTRIE2_OPTIONS_VALUE_BITS_MASK)) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    /* get the length values and offsets */
    trie->indexLength=header->indexLength;
    trie->dataLength=header->shiftedDataLength<<UTRIE2_INDEX_SHIFT;
    trie->index2NullOffset=header->index2NullOffset;
    trie->dataNullOffset=header->dataNullOffset;

    trie->highStart=header->shiftedHighStart<<UTRIE2_SHIFT_1;
    trie->highValueIndex=trie->dataLength-UTRIE2_DATA_GRANULARITY;
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        trie->highValueIndex+=trie->indexLength;
    }

    length-=(int32_t)sizeof(UTrie2Header);

    /* enough data for the index? */
    if(length<2*trie->indexLength) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }
    p16=(const uint16_t *)(header+1);
    trie->index=p16;
    p16+=trie->indexLength;
    length-=2*trie->indexLength;

    /* get the data */
    switch(valueBits) {
    case UTRIE2_16_VALUE_BITS:
        if(length<2*trie->dataLength) {
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return 0;
        }

        trie->data16=p16;
        trie->data32=NULL;
        trie->initialValue=trie->index[trie->dataNullOffset];
        trie->errorValue=trie->data16[UTRIE2_BAD_UTF8_DATA_OFFSET];
        length=(int32_t)sizeof(UTrie2Header)+2*trie->indexLength+2*trie->dataLength;
        break;
    case UTRIE2_32_VALUE_BITS:
        if(length<4*trie->dataLength) {
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return 0;
        }
        trie->data16=NULL;
        trie->data32=(const uint32_t *)p16;
        trie->initialValue=trie->data32[trie->dataNullOffset];
        trie->errorValue=trie->data32[UTRIE2_BAD_UTF8_DATA_OFFSET];
        length=(int32_t)sizeof(UTrie2Header)+2*trie->indexLength+4*trie->dataLength;
        break;
    default:
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    return length;
}

U_CAPI int32_t U_EXPORT2
utrie2_unserializeDummy(UTrie2 *trie,
                        UTrie2ValueBits valueBits,
                        uint32_t initialValue, uint32_t errorValue,
                        void *data, int32_t capacity,
                        UErrorCode *pErrorCode) {
    uint32_t *p;
    uint16_t *dest16;
    int32_t indexLength, dataLength, length, i;
    int32_t dataMove;  /* >0 if the data is moved to the end of the index array */

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( trie==NULL ||
        capacity<0 || (capacity>0 && (data==NULL || (((int32_t)data&3)!=0))) ||
        valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* calculate the total length of the dummy trie data */
    indexLength=UTRIE2_INDEX_1_OFFSET;
    dataLength=UTRIE2_DATA_START_OFFSET+UTRIE2_DATA_GRANULARITY;
    length=indexLength*2;
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        length+=dataLength*2;
    } else {
        length+=dataLength*4;
    }

    if(length>capacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return length;
    }

    /* set the header fields */
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        dataMove=indexLength;
    } else {
        dataMove=0;
    }

    trie->indexLength=indexLength;
    trie->dataLength=dataLength;
    trie->index2NullOffset=UTRIE2_INDEX_2_OFFSET;
    trie->dataNullOffset=(uint16_t)dataMove;
    trie->initialValue=initialValue;
    trie->errorValue=errorValue;
    trie->highStart=0;
    trie->highValueIndex=dataMove+UTRIE2_DATA_START_OFFSET;

    /* fill the index and data arrays */
    dest16=(uint16_t *)data;
    trie->index=dest16;

    /* write the index-2 array values shifted right by UTRIE2_INDEX_SHIFT */
    for(i=0; i<UTRIE2_INDEX_2_BMP_LENGTH; ++i) {
        *dest16++=(uint16_t)(dataMove>>UTRIE2_INDEX_SHIFT);  /* null data block */
    }

    /* write UTF-8 2-byte index-2 values, not right-shifted */
    for(i=0; i<(0xc2-0xc0); ++i) {                                  /* C0..C1 */
        *dest16++=(uint16_t)(dataMove+UTRIE2_BAD_UTF8_DATA_OFFSET);
    }
    for(; i<(0xe0-0xc0); ++i) {                                     /* C2..DF */
        *dest16++=(uint16_t)dataMove;
    }

    /* write the 16/32-bit data array */
    switch(valueBits) {
    case UTRIE2_16_VALUE_BITS:
        /* write 16-bit data values */
        trie->data16=dest16;
        trie->data32=NULL;
        for(i=0; i<0x80; ++i) {
            *dest16++=(uint16_t)initialValue;
        }
        for(; i<0xc0; ++i) {
            *dest16++=(uint16_t)errorValue;
        }
        /* highValue and reserved values */
        for(i=0; i<UTRIE2_DATA_GRANULARITY; ++i) {
            *dest16++=(uint16_t)initialValue;
        }
        break;
    case UTRIE2_32_VALUE_BITS:
        /* write 32-bit data values */
        p=(uint32_t *)dest16;
        trie->data16=NULL;
        trie->data32=p;
        for(i=0; i<0x80; ++i) {
            *p++=initialValue;
        }
        for(; i<0xc0; ++i) {
            *p++=errorValue;
        }
        /* highValue and reserved values */
        for(i=0; i<UTRIE2_DATA_GRANULARITY; ++i) {
            *p++=initialValue;
        }
        break;
    default:
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    return length;
}

/*
 * utrie2_enum() is at the bottom of the file because it shares
 * its implementation with unewtrie2_enum().
 */

U_CAPI int32_t U_EXPORT2
utrie2_getVersion(const void *data, int32_t length, UBool anyEndianOk) {
    uint32_t signature;
    if(length<16 || data==NULL || (((int32_t)data&3)!=0)) {
        return 0;
    }
    signature=*(const uint32_t *)data;
    if(signature==UTRIE2_SIG) {
        return 2;
    }
    if(anyEndianOk && signature==UTRIE2_OE_SIG) {
        return 2;
    }
    if(signature==UTRIE_SIG) {
        return 1;
    }
    if(anyEndianOk && signature==UTRIE_OE_SIG) {
        return 1;
    }
    return 0;
}

U_CAPI void U_EXPORT2
utrie_enumGeneral(const UTrie *trie, UBool enumLeadCUNotCP,
                  UTrieEnumValue *enumValue, UTrieEnumRange *enumRange, const void *context);

typedef struct NewTrieAndStatus {
    UNewTrie2 *newTrie;
    uint32_t initialValue;
#ifdef UTRIE2_DEBUG
    int32_t countValues;
#endif
    UBool ok;
} NewTrieAndStatus;

static UBool U_CALLCONV
copyEnumRange(const void *context, UChar32 start, UChar32 limit, uint32_t value) {
    NewTrieAndStatus *nt=(NewTrieAndStatus *)context;
    if(value!=nt->initialValue) {
#ifdef UTRIE2_DEBUG
        nt->countValues+=limit-start;
#endif
        if(start==(limit-1)) {
            return nt->ok=unewtrie2_set32(nt->newTrie, start, value);
        } else {
            return nt->ok=unewtrie2_setRange32(nt->newTrie, start, limit, value, TRUE);
        }
    } else {
        return TRUE;
    }
}

/**
 * TODO: Move to a separate .c file for modularization.
 */
U_CAPI void * U_EXPORT2
utrie2_fromUTrie(UTrie2 *trie2, const UTrie *trie1,
                 uint32_t errorValue, UBool copyLeadCUNotCP,
                 UErrorCode *pErrorCode) {
    NewTrieAndStatus context;
    void *memory;

    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    if(trie2==NULL || trie1==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
    context.newTrie=unewtrie2_open(trie1->initialValue, errorValue, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    context.initialValue=trie1->initialValue;
#ifdef UTRIE2_DEBUG
    context.countValues=0;
#endif
    context.ok=TRUE;
    utrie_enumGeneral(trie1, copyLeadCUNotCP, NULL, copyEnumRange, &context);
    memory=NULL;
    if(context.ok) {
        memory=unewtrie2_build(context.newTrie,
                               trie1->data32!=NULL ? UTRIE2_32_VALUE_BITS : UTRIE2_16_VALUE_BITS,
                               trie2, pErrorCode);
    } else {
        /* most likely reason for unewtrie2_setRange32() to fail */
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
    }
    unewtrie2_close(context.newTrie);
#ifdef UTRIE2_DEBUG
    if(U_SUCCESS(*pErrorCode)) {
        utrie_printLengths(trie1);
        utrie2_printLengths(trie2, "fromUTrie");
    }
#endif
    return memory;
}

#ifdef UNEWTRIE2_COMPARE_WITH_UTRIE
U_CAPI void U_EXPORT2
utrie_enumNewTrie(const UNewTrie *trie,
                  UTrieEnumValue *enumValue, UTrieEnumRange *enumRange, const void *context);

U_CAPI void U_EXPORT2
unewtrie2_compareWithUTrie(const UNewTrie *trie1, UBool reduceTo16Bits, UBool copyLeadCUNotCP) {
    UTrie2 trie2;
    NewTrieAndStatus context;
    void *memory;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    context.newTrie=unewtrie2_open(trie1->data[0], trie1->data[0], &errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }
    context.initialValue=trie1->data[0];
#ifdef UTRIE2_DEBUG
    context.countValues=0;
#endif
    context.ok=TRUE;
    utrie_enumNewTrie(trie1, NULL, copyEnumRange, &context);
    memory=NULL;
    if(context.ok) {
#ifdef UTRIE2_DEBUG
        printf("-*- unewtrie2_compareWithUTrie() countValues=%ld\n", (long)context.countValues);
#endif
        memory=unewtrie2_build(context.newTrie,
                               reduceTo16Bits ? UTRIE2_16_VALUE_BITS : UTRIE2_32_VALUE_BITS,
                               &trie2, &errorCode);
    } else {
        /* most likely reason for unewtrie2_setRange32() to fail */
        errorCode=U_MEMORY_ALLOCATION_ERROR;
    }
    unewtrie2_close(context.newTrie);
    if(U_SUCCESS(errorCode)) {
        utrie2_printLengths(&trie2, "compareWithUTrie");
    }
    uprv_free(memory);
}
#endif

U_CAPI int32_t U_EXPORT2
utrie2_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    const UTrie2Header *inTrie;
    UTrie2Header trie;
    int32_t dataLength, size;
    UTrie2ValueBits valueBits;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || (length>=0 && outData==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* setup and swapping */
    if(length>=0 && length<sizeof(UTrie2Header)) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    inTrie=(const UTrie2Header *)inData;
    trie.signature=ds->readUInt32(inTrie->signature);
    trie.options=ds->readUInt16(inTrie->options);
    trie.indexLength=ds->readUInt16(inTrie->indexLength);
    trie.shiftedDataLength=ds->readUInt16(inTrie->shiftedDataLength);

    valueBits=trie.options&UTRIE2_OPTIONS_VALUE_BITS_MASK;
    dataLength=(int32_t)trie.shiftedDataLength<<UTRIE2_INDEX_SHIFT;

    if( trie.signature!=UTRIE2_SIG ||
        valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits ||
        trie.indexLength<UTRIE2_INDEX_1_OFFSET ||
        dataLength<UTRIE2_DATA_START_OFFSET
    ) {
        *pErrorCode=U_INVALID_FORMAT_ERROR; /* not a UTrie */
        return 0;
    }

    size=sizeof(UTrie2Header)+trie.indexLength*2;
    switch(valueBits) {
    case UTRIE2_16_VALUE_BITS:
        size+=dataLength*2;
        break;
    case UTRIE2_32_VALUE_BITS:
        size+=dataLength*4;
        break;
    default:
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    if(length>=0) {
        UTrie2Header *outTrie;

        if(length<size) {
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outTrie=(UTrie2Header *)outData;

        /* swap the header */
        ds->swapArray32(ds, &inTrie->signature, 4, &outTrie->signature, pErrorCode);
        ds->swapArray16(ds, &inTrie->options, 12, &outTrie->options, pErrorCode);

        /* swap the index and the data */
        switch(valueBits) {
        case UTRIE2_16_VALUE_BITS:
            ds->swapArray16(ds, inTrie+1, (trie.indexLength+dataLength)*2, outTrie+1, pErrorCode);
            break;
        case UTRIE2_32_VALUE_BITS:
            ds->swapArray16(ds, inTrie+1, trie.indexLength*2, outTrie+1, pErrorCode);
            ds->swapArray32(ds, (const uint16_t *)(inTrie+1)+trie.indexLength, dataLength*4,
                                     (uint16_t *)(outTrie+1)+trie.indexLength, pErrorCode);
            break;
        default:
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return 0;
        }
    }

    return size;
}

/* Building a trie ---------------------------------------------------------- */

enum {
    /**
     * At build time, leave a gap in the index-2 table,
     * at least as long as the maximum lengths of the 2-byte UTF-8 index-2 table
     * and the supplementary index-1 table.
     * Round up to UTRIE2_INDEX_2_BLOCK_LENGTH for proper compacting.
     */
    UNEWTRIE2_INDEX_GAP_OFFSET=UTRIE2_INDEX_2_BMP_LENGTH,
    UNEWTRIE2_INDEX_GAP_LENGTH=
        ((UTRIE2_UTF8_2B_INDEX_2_LENGTH+UTRIE2_MAX_INDEX_1_LENGTH)+UTRIE2_INDEX_2_MASK)&
        ~UTRIE2_INDEX_2_MASK,

    /** The null index-2 block. */
    UNEWTRIE2_INDEX_2_NULL_OFFSET=UNEWTRIE2_INDEX_GAP_OFFSET+UNEWTRIE2_INDEX_GAP_LENGTH,

    /** The start of allocated index-2 blocks. */
    UNEWTRIE2_INDEX_2_START_OFFSET=UNEWTRIE2_INDEX_2_NULL_OFFSET+UTRIE2_INDEX_2_BLOCK_LENGTH,

    /**
     * Maximum length of the build-time index-2 array.
     * Maximum number of Unicode code points (0x110000) shifted right by UTRIE2_SHIFT_2,
     * plus the build-time index gap,
     * plus the null index-2 block.
     */
    UNEWTRIE2_MAX_INDEX_2_LENGTH=(0x110000>>UTRIE2_SHIFT_2)+UNEWTRIE2_INDEX_GAP_LENGTH+UTRIE2_INDEX_2_BLOCK_LENGTH,

    UNEWTRIE2_INDEX_1_LENGTH=0x110000>>UTRIE2_SHIFT_1,

    /**
     * The null data block.
     * Length 64=0x40 even if UTRIE2_DATA_BLOCK_LENGTH is smaller,
     * to work with 6-bit trail bytes from 2-byte UTF-8.
     */
    UNEWTRIE2_DATA_NULL_OFFSET=UTRIE2_DATA_START_OFFSET,

    /** The start of allocated data blocks. */
    UNEWTRIE2_DATA_START_OFFSET=UNEWTRIE2_DATA_NULL_OFFSET+0x40,

    /**
     * The start of data blocks for U+0800 and above.
     * Below, compaction uses a block length of 64 for 2-byte UTF-8.
     * From here on, compaction uses UTRIE2_DATA_BLOCK_LENGTH.
     * Data values for 0x780 code points beyond ASCII.
     */
    UNEWTRIE2_DATA_0800_OFFSET=UNEWTRIE2_DATA_START_OFFSET+0x780
};

/* Start with allocation of 16k data entries. */
#define UNEWTRIE2_INITIAL_DATA_LENGTH ((int32_t)1<<14)

/* Grow about 8x each time. */
#define UNEWTRIE2_MEDIUM_DATA_LENGTH ((int32_t)1<<17)

/**
 * Maximum length of the build-time data array.
 * One entry per 0x110000 code points, plus the illegal-UTF-8 block and the null block.
 */
#define UNEWTRIE2_MAX_DATA_LENGTH (0x110000+0x40+0x40)

/*
 * Build-time trie structure.
 *
 * Just using a boolean flag for "repeat use" could lead to data array overflow
 * because we would not be able to detect when a data block becomes unused.
 * It also leads to orphan data blocks that are kept through serialization.
 *
 * Need to use reference counting for data blocks,
 * and allocDataBlock() needs to look for a free block before increasing dataLength.
 *
 * This scheme seems like overkill for index-2 blocks since the whole index array is
 * preallocated anyway (unlike the growable data array).
 * Just allocating multiple index-2 blocks as needed.
 */
struct UNewTrie2 {
    int32_t index1[UNEWTRIE2_INDEX_1_LENGTH];
    int32_t index2[UNEWTRIE2_MAX_INDEX_2_LENGTH];
    uint32_t *data;

    uint32_t initialValue, errorValue;
    int32_t index2Length, dataCapacity, dataLength;
    int32_t firstFreeBlock;
    int32_t index2NullOffset, dataNullOffset;
    UChar32 highStart;
    UBool isCompacted;

    /**
     * Multi-purpose per-data-block table.
     *
     * Before compacting:
     *
     * Per-data-block reference counters/free-block list.
     *  0: unused
     * >0: reference counter (number of index-2 entries pointing here)
     * <0: next free data block in free-block list
     *
     * While compacting:
     *
     * Map of adjusted indexes, used in compactData() and compactIndex2().
     * Maps from original indexes to new ones.
     */
    int32_t map[UNEWTRIE2_MAX_DATA_LENGTH>>UTRIE2_SHIFT_2];
};

/* Building a trie ----------------------------------------------------------*/

static int32_t
allocIndex2Block(UNewTrie2 *trie);

U_CAPI UNewTrie2 * U_EXPORT2
unewtrie2_open(uint32_t initialValue, uint32_t errorValue, UErrorCode *pErrorCode) {
    UNewTrie2 *trie;
    int32_t i, j;

    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }

    trie=(UNewTrie2 *)uprv_malloc(sizeof(UNewTrie2));
    if(trie==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    trie->data=(uint32_t *)uprv_malloc(UNEWTRIE2_INITIAL_DATA_LENGTH*4);
    if(trie->data==NULL) {
        uprv_free(trie);
        return NULL;
    }
    trie->dataCapacity=UNEWTRIE2_INITIAL_DATA_LENGTH;

    trie->initialValue=initialValue;
    trie->errorValue=errorValue;
    trie->highStart=0x110000;
    trie->firstFreeBlock=0;  /* no free block in the list */
    trie->isCompacted=FALSE;

    /*
     * preallocate and reset
     * - ASCII
     * - the bad-UTF-8-data block
     * - the null data block
     */
    for(i=0; i<0x80; ++i) {
        trie->data[i]=initialValue;
    }
    for(; i<0xc0; ++i) {
        trie->data[i]=errorValue;
    }
    for(i=UNEWTRIE2_DATA_NULL_OFFSET; i<UNEWTRIE2_DATA_START_OFFSET; ++i) {
        trie->data[i]=initialValue;
    }
    trie->dataNullOffset=UNEWTRIE2_DATA_NULL_OFFSET;
    trie->dataLength=UNEWTRIE2_DATA_START_OFFSET;

    /* set the index-2 indexes for the 2=0x80>>UTRIE2_SHIFT_2 ASCII data blocks */
    for(i=0, j=0; j<0x80; ++i, j+=UTRIE2_DATA_BLOCK_LENGTH) {
        trie->index2[i]=j;
        trie->map[i]=1;
    }
    /* reference counts for the bad-UTF-8-data block */
    for(; j<0xc0; ++i, j+=UTRIE2_DATA_BLOCK_LENGTH) {
        trie->map[i]=0;
    }
    /*
     * Reference counts for the null data block: all blocks but ASCII.
     * Plus 1 so that we don't drop this block during compaction.
     */
    /* i==trie->dataNullOffset */
    trie->map[i++]=(0x110000>>UTRIE2_SHIFT_2)-(0x80>>UTRIE2_SHIFT_2)+1;
    j+=UTRIE2_DATA_BLOCK_LENGTH;
    for(; j<UNEWTRIE2_DATA_START_OFFSET; ++i, j+=UTRIE2_DATA_BLOCK_LENGTH) {
        trie->map[i]=0;
    }

    /*
     * set the remaining indexes in the BMP index-2 block
     * to the null data block
     */
    for(i=0x80>>UTRIE2_SHIFT_2; i<UTRIE2_INDEX_2_BMP_LENGTH; ++i) {
        trie->index2[i]=UNEWTRIE2_DATA_NULL_OFFSET;
    }

    /*
     * Fill the index gap with impossible values so that compaction
     * does not overlap other index-2 blocks with the gap.
     */
    for(i=0; i<UNEWTRIE2_INDEX_GAP_LENGTH; ++i) {
        trie->index2[UNEWTRIE2_INDEX_GAP_OFFSET+i]=-1;
    }

    /* set the indexes in the null index-2 block */
    for(i=0; i<UTRIE2_INDEX_2_BLOCK_LENGTH; ++i) {
        trie->index2[UNEWTRIE2_INDEX_2_NULL_OFFSET+i]=UNEWTRIE2_DATA_NULL_OFFSET;
    }
    trie->index2NullOffset=UNEWTRIE2_INDEX_2_NULL_OFFSET;
    trie->index2Length=UNEWTRIE2_INDEX_2_START_OFFSET;

    /* set the index-1 indexes for the linear index-2 block */
    for(i=0, j=0;
        i<(UTRIE2_INDEX_2_BMP_LENGTH>>UTRIE2_SHIFT_1_2);
        ++i, j+=UTRIE2_INDEX_2_BLOCK_LENGTH
    ) {
        trie->index1[i]=j;
    }

    /* set the remaining index-1 indexes to the null index-2 block */
    for(; i<UNEWTRIE2_INDEX_1_LENGTH; ++i) {
        trie->index1[i]=UNEWTRIE2_INDEX_2_NULL_OFFSET;
    }

    /*
     * Preallocate and reset data for U+0080..U+07ff,
     * for 2-byte UTF-8 which will be compacted in 64-blocks
     * even if UTRIE2_DATA_BLOCK_LENGTH is smaller.
     */
    for(i=0x80; i<0x800; i+=UTRIE2_DATA_BLOCK_LENGTH) {
        unewtrie2_set32(trie, i, initialValue);
    }

    return trie;
}

U_CAPI UNewTrie2 * U_EXPORT2
unewtrie2_clone(const UNewTrie2 *other) {
    UNewTrie2 *trie;

    /* do not clone if other is not valid or already compacted */
    if(other==NULL || other->data==NULL) {
        return NULL;
    }

    trie=(UNewTrie2 *)uprv_malloc(sizeof(UNewTrie2));
    if(trie==NULL) {
        return NULL;
    }

    trie->data=(uint32_t *)uprv_malloc(other->dataCapacity*4);
    if(trie->data==NULL) {
        uprv_free(trie);
        return NULL;
    }
    trie->dataCapacity=other->dataCapacity;

    /* clone data */
    uprv_memcpy(trie->index1, other->index1, sizeof(trie->index1));
    uprv_memcpy(trie->index2, other->index2, other->index2Length*4);
    trie->index2NullOffset=other->index2NullOffset;
    trie->index2Length=other->index2Length;

    uprv_memcpy(trie->data, other->data, other->dataLength*4);
    trie->dataNullOffset=other->dataNullOffset;
    trie->dataLength=other->dataLength;

    /* reference counters */
    if(other->isCompacted) {
        trie->firstFreeBlock=0;
    } else {
        uprv_memcpy(trie->map, other->map, (other->dataLength>>UTRIE2_SHIFT_2)*4);
        trie->firstFreeBlock=other->firstFreeBlock;
    }

    trie->initialValue=other->initialValue;
    trie->errorValue=other->errorValue;
    trie->highStart=other->highStart;
    trie->isCompacted=other->isCompacted;

    return trie;
}

U_CAPI void U_EXPORT2
unewtrie2_close(UNewTrie2 *trie) {
    if(trie!=NULL) {
        uprv_free(trie->data);
        uprv_free(trie);
    }
}

U_CAPI uint32_t * U_EXPORT2
unewtrie2_getData(const UNewTrie2 *trie, int32_t *pLength) {
    if(trie==NULL || pLength==NULL) {
        return NULL;
    }

    *pLength=trie->dataLength;
    return trie->data;
}

static U_INLINE UBool
isInNullBlock(UNewTrie2 *trie, UChar32 c) {
    int32_t block=trie->index2[
                trie->index1[c>>UTRIE2_SHIFT_1]+
                ((c>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK)];
    return (UBool)(block==trie->dataNullOffset);
}

U_CAPI uint32_t U_EXPORT2
unewtrie2_get32(const UNewTrie2 *trie, UChar32 c) {
    int32_t block;

    /* valid trie and valid c? */
    if(trie==NULL) {
        return 0;
    }
    if((uint32_t)c>0x10ffff) {
        return trie->errorValue;
    }
    if(c>=trie->highStart) {
        return trie->data[trie->dataLength-UTRIE2_DATA_GRANULARITY];
    }

    block=trie->index2[
            trie->index1[c>>UTRIE2_SHIFT_1]+
            ((c>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK)];
    return trie->data[block+(c&UTRIE2_DATA_MASK)];
}

static int32_t
allocIndex2Block(UNewTrie2 *trie) {
    int32_t newBlock, newTop;

    newBlock=trie->index2Length;
    newTop=newBlock+UTRIE2_INDEX_2_BLOCK_LENGTH;
    if(newTop>LENGTHOF(trie->index2)) {
        /*
         * Should never occur.
         * Either UTRIE2_MAX_BUILD_TIME_INDEX_LENGTH is incorrect,
         * or the code writes more values than should be possible.
         */
        return -1;
    }
    trie->index2Length=newTop;
    uprv_memcpy(trie->index2+newBlock, trie->index2+trie->index2NullOffset, UTRIE2_INDEX_2_BLOCK_LENGTH*4);
    return newBlock;
}

static int32_t
getIndex2Block(UNewTrie2 *trie, UChar32 c) {
    int32_t i1, i2;

    i1=c>>UTRIE2_SHIFT_1;
    i2=trie->index1[i1];
    if(i2==trie->index2NullOffset) {
        i2=allocIndex2Block(trie);
        if(i2<0) {
            return -1;  /* program error */
        }
        trie->index1[i1]=i2;
    }
    return i2;
}

static int32_t
allocDataBlock(UNewTrie2 *trie, int32_t copyBlock) {
    int32_t newBlock, newTop;

    if(trie->firstFreeBlock!=0) {
        /* get the first free block */
        newBlock=trie->firstFreeBlock;
        trie->firstFreeBlock=-trie->map[newBlock>>UTRIE2_SHIFT_2];
    } else {
        /* get a new block from the high end */
        newBlock=trie->dataLength;
        newTop=newBlock+UTRIE2_DATA_BLOCK_LENGTH;
        if(newTop>trie->dataCapacity) {
            /* out of memory in the data array */
            int32_t capacity;
            uint32_t *data;

            if(trie->dataCapacity<UNEWTRIE2_MEDIUM_DATA_LENGTH) {
                capacity=UNEWTRIE2_MEDIUM_DATA_LENGTH;
            } else if(trie->dataCapacity<UNEWTRIE2_MAX_DATA_LENGTH) {
                capacity=UNEWTRIE2_MAX_DATA_LENGTH;
            } else {
                /*
                 * Should never occur.
                 * Either UNEWTRIE2_MAX_DATA_LENGTH is incorrect,
                 * or the code writes more values than should be possible.
                 */
                return -1;
            }
            data=(uint32_t *)uprv_malloc(capacity*4);
            if(data==NULL) {
                return -1;
            }
            uprv_memcpy(data, trie->data, trie->dataLength*4);
            uprv_free(trie->data);
            trie->data=data;
            trie->dataCapacity=capacity;
        }
        trie->dataLength=newTop;
    }
    uprv_memcpy(trie->data+newBlock, trie->data+copyBlock, UTRIE2_DATA_BLOCK_LENGTH*4);
    trie->map[newBlock>>UTRIE2_SHIFT_2]=0;
    return newBlock;
}

/* call when the block's reference counter reaches 0 */
static void
releaseDataBlock(UNewTrie2 *trie, int32_t block) {
    /* put this block at the front of the free-block chain */
    trie->map[block>>UTRIE2_SHIFT_2]=-trie->firstFreeBlock;
    trie->firstFreeBlock=block;
}

static U_INLINE UBool
isWritableBlock(UNewTrie2 *trie, int32_t block) {
    return (UBool)(block!=trie->dataNullOffset && 1==trie->map[block>>UTRIE2_SHIFT_2]);
}

static U_INLINE void
setIndex2Entry(UNewTrie2 *trie, int32_t i2, int32_t block) {
    int32_t oldBlock;
    ++trie->map[block>>UTRIE2_SHIFT_2];  /* increment first, in case block==oldBlock! */
    oldBlock=trie->index2[i2];
    if(0 == --trie->map[oldBlock>>UTRIE2_SHIFT_2]) {
        releaseDataBlock(trie, oldBlock);
    }
    trie->index2[i2]=block;
}

/**
 * No error checking for illegal arguments.
 *
 * @return -1 if no new data block available (out of memory in data array)
 * @internal
 */
static int32_t
getDataBlock(UNewTrie2 *trie, UChar32 c) {
    int32_t i2, oldBlock, newBlock;

    i2=getIndex2Block(trie, c);
    if(i2<0) {
        return -1;  /* program error */
    }

    i2+=(c>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK;
    oldBlock=trie->index2[i2];
    if(isWritableBlock(trie, oldBlock)) {
        return oldBlock;
    }

    /* allocate a new data block */
    newBlock=allocDataBlock(trie, oldBlock);
    if(newBlock<0) {
        /* out of memory in the data array */
        return -1;
    }
    setIndex2Entry(trie, i2, newBlock);
    return newBlock;
}

/**
 * @return TRUE if the value was successfully set
 */
U_CAPI UBool U_EXPORT2
unewtrie2_set32(UNewTrie2 *trie, UChar32 c, uint32_t value) {
    int32_t block;

    /* valid, uncompacted trie and valid c? */
    if(trie==NULL || trie->isCompacted || (uint32_t)c>0x10ffff) {
        return FALSE;
    }

    block=getDataBlock(trie, c);
    if(block<0) {
        return FALSE;
    }

    trie->data[block+(c&UTRIE2_DATA_MASK)]=value;
    return TRUE;
}

static void
writeBlock(uint32_t *block, uint32_t value) {
    uint32_t *limit=block+UTRIE2_DATA_BLOCK_LENGTH;
    while(block<limit) {
        *block++=value;
    }
}

/**
 * initialValue is ignored if overwrite=TRUE
 * @internal
 */
static void
fillBlock(uint32_t *block, UChar32 start, UChar32 limit,
          uint32_t value, uint32_t initialValue, UBool overwrite) {
    uint32_t *pLimit;

    pLimit=block+limit;
    block+=start;
    if(overwrite) {
        while(block<pLimit) {
            *block++=value;
        }
    } else {
        while(block<pLimit) {
            if(*block==initialValue) {
                *block=value;
            }
            ++block;
        }
    }
}

U_CAPI UBool U_EXPORT2
unewtrie2_setRange32(UNewTrie2 *trie,
                     UChar32 start, UChar32 limit,
                     uint32_t value, UBool overwrite) {
    /*
     * repeat value in [start..limit[
     * mark index values for repeat-data blocks by setting bit 31 of the index values
     * fill around existing values if any, if(overwrite)
     */
    int32_t block, rest, repeatBlock;

    /* valid, uncompacted trie and valid indexes? */
    if( trie==NULL || trie->isCompacted ||
        (uint32_t)start>0x10ffff || (uint32_t)limit>0x110000 || start>limit
    ) {
        return FALSE;
    }
    if(start==limit || (!overwrite && value==trie->initialValue)) {
        return TRUE; /* nothing to do */
    }

    if(start&UTRIE2_DATA_MASK) {
        UChar32 nextStart;

        /* set partial block at [start..following block boundary[ */
        block=getDataBlock(trie, start);
        if(block<0) {
            return FALSE;
        }

        nextStart=(start+UTRIE2_DATA_BLOCK_LENGTH)&~UTRIE2_DATA_MASK;
        if(nextStart<=limit) {
            fillBlock(trie->data+block, start&UTRIE2_DATA_MASK, UTRIE2_DATA_BLOCK_LENGTH,
                      value, trie->initialValue, overwrite);
            start=nextStart;
        } else {
            fillBlock(trie->data+block, start&UTRIE2_DATA_MASK, limit&UTRIE2_DATA_MASK,
                      value, trie->initialValue, overwrite);
            return TRUE;
        }
    }

    /* number of positions in the last, partial block */
    rest=limit&UTRIE2_DATA_MASK;

    /* round down limit to a block boundary */
    limit&=~UTRIE2_DATA_MASK;

    /* iterate over all-value blocks */
    if(value==trie->initialValue) {
        repeatBlock=trie->dataNullOffset;
    } else {
        repeatBlock=-1;
    }

    while(start<limit) {
        int32_t i2;
        UBool setRepeatBlock=FALSE;

        if(value==trie->initialValue && isInNullBlock(trie, start)) {
            start+=UTRIE2_DATA_BLOCK_LENGTH; /* nothing to do */
            continue;
        }

        /* get index value */
        i2=getIndex2Block(trie, start);
        if(i2<0) {
            return FALSE;  /* program error */
        }
        i2+=(start>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK;
        block=trie->index2[i2];
        if(isWritableBlock(trie, block)) {
            /* already allocated */
            if(overwrite && block>=UNEWTRIE2_DATA_0800_OFFSET) {
                /*
                 * We overwrite all values, and it's not a
                 * protected (ASCII-linear or 2-byte UTF-8) block:
                 * replace with the repeatBlock.
                 */
                setRepeatBlock=TRUE;
            } else {
                /* !overwrite, or protected block: just write the values into this block */
                fillBlock(trie->data+block,
                          0, UTRIE2_DATA_BLOCK_LENGTH,
                          value, trie->initialValue, overwrite);
            }
        } else if(trie->data[block]!=value && (overwrite || block==trie->dataNullOffset)) {
            /*
             * Set the repeatBlock instead of the null block or previous repeat block:
             *
             * If !isWritableBlock() then all entries in the block have the same value
             * because it's the null block or a range block (the repeatBlock from a previous
             * call to unewtrie2_setRange32()).
             * No other blocks are used multiple times before compacting.
             *
             * The null block is the only non-writable block with the initialValue because
             * of the repeatBlock initialization above. (If value==initialValue, then
             * the repeatBlock will be the null data block.)
             *
             * We set our repeatBlock if the desired value differs from the block's value,
             * and if we overwrite any data or if the data is all initial values
             * (which is the same as the block being the null block, see above).
             */
            setRepeatBlock=TRUE;
        }
        if(setRepeatBlock) {
            if(repeatBlock>=0) {
                setIndex2Entry(trie, i2, repeatBlock);
            } else {
                /* create and set and fill the repeatBlock */
                repeatBlock=getDataBlock(trie, start);
                if(repeatBlock<0) {
                    return FALSE;
                }
                writeBlock(trie->data+repeatBlock, value);
            }
        }

        start+=UTRIE2_DATA_BLOCK_LENGTH;
    }

    if(rest>0) {
        /* set partial block at [last block boundary..limit[ */
        block=getDataBlock(trie, start);
        if(block<0) {
            return FALSE;
        }

        fillBlock(trie->data+block, 0, rest, value, trie->initialValue, overwrite);
    }

    return TRUE;
}

U_CAPI void * U_EXPORT2
unewtrie2_build(UNewTrie2 *newTrie, UTrie2ValueBits valueBits,
                UTrie2 *trie, UErrorCode *pErrorCode) {
    void *memory;
    int32_t length;

    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    if(newTrie==NULL || trie==NULL ||
        valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    /* preflight the length */
    length=unewtrie2_serialize(newTrie, valueBits, NULL, 0, pErrorCode);
    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* expected for preflighting */
        *pErrorCode=U_ZERO_ERROR;
        memory=uprv_malloc(length);
        if(memory==NULL) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        } else {
            length=unewtrie2_serialize(newTrie, valueBits, memory, length, pErrorCode);
            utrie2_unserialize(trie, valueBits, memory, length, pErrorCode);
        }
    } else if(U_SUCCESS(*pErrorCode)) {
        *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
    }
    if(U_FAILURE(*pErrorCode)) {
        uprv_free(memory);
        memory=NULL;
    }
    return memory;
}

/* compaction --------------------------------------------------------------- */

static U_INLINE UBool
equal_int32(const int32_t *s, const int32_t *t, int32_t length) {
    while(length>0 && *s==*t) {
        ++s;
        ++t;
        --length;
    }
    return (UBool)(length==0);
}

static U_INLINE UBool
equal_uint32(const uint32_t *s, const uint32_t *t, int32_t length) {
    while(length>0 && *s==*t) {
        ++s;
        ++t;
        --length;
    }
    return (UBool)(length==0);
}

static int32_t
findSameIndex2Block(const int32_t *index, int32_t index2Length, int32_t otherBlock) {
    int32_t block;

    /* ensure that we do not even partially get past index2Length */
    index2Length-=UTRIE2_INDEX_2_BLOCK_LENGTH;

    for(block=0; block<=index2Length; ++block) {
        if(equal_int32(index+block, index+otherBlock, UTRIE2_INDEX_2_BLOCK_LENGTH)) {
            return block;
        }
    }
    return -1;
}

static int32_t
findSameDataBlock(const uint32_t *data, int32_t dataLength, int32_t otherBlock, int32_t blockLength) {
    int32_t block;

    /* ensure that we do not even partially get past dataLength */
    dataLength-=blockLength;

    for(block=0; block<=dataLength; block+=UTRIE2_DATA_GRANULARITY) {
        if(equal_uint32(data+block, data+otherBlock, blockLength)) {
            return block;
        }
    }
    return -1;
}

/*
 * Find the start of the last range in the trie by enumerating backward.
 * Indexes for supplementary code points higher than this will be omitted.
 */
static UChar32
findHighStart(UNewTrie2 *trie) {
    const uint32_t *data32;

    uint32_t value, initialValue, highValue;
    UChar32 c, prev;
    int32_t i1, i2, j, i2Block, prevI2Block, index2NullOffset, block, prevBlock, nullBlock;

    data32=trie->data;
    initialValue=trie->initialValue;
    highValue=unewtrie2_get32(trie, 0x10ffff);

    index2NullOffset=trie->index2NullOffset;
    nullBlock=trie->dataNullOffset;

    /* set variables for previous range */
    if(highValue==initialValue) {
        prevI2Block=index2NullOffset;
        prevBlock=nullBlock;
    } else {
        prevI2Block=-1;
        prevBlock=-1;
    }
    prev=0x110000;

    /* enumerate index-2 blocks */
    i1=UNEWTRIE2_INDEX_1_LENGTH;
    c=prev;
    while(c>0) {
        i2Block=trie->index1[--i1];
        if(i2Block==prevI2Block) {
            /* the index-2 block is the same as the previous one, and filled with highValue */
            c-=UTRIE2_CP_PER_INDEX_1_ENTRY;
            continue;
        }
        prevI2Block=i2Block;
        if(i2Block==index2NullOffset) {
            /* this is the null index-2 block */
            if(highValue!=initialValue) {
                return c;
            }
            c-=UTRIE2_CP_PER_INDEX_1_ENTRY;
        } else {
            /* enumerate data blocks for one index-2 block */
            for(i2=UTRIE2_INDEX_2_BLOCK_LENGTH; i2>0;) {
                block=trie->index2[i2Block+ --i2];
                if(block==prevBlock) {
                    /* the block is the same as the previous one, and filled with highValue */
                    c-=UTRIE2_DATA_BLOCK_LENGTH;
                    continue;
                }
                prevBlock=block;
                if(block==nullBlock) {
                    /* this is the null data block */
                    if(highValue!=initialValue) {
                        return c;
                    }
                    c-=UTRIE2_DATA_BLOCK_LENGTH;
                } else {
                    for(j=UTRIE2_DATA_BLOCK_LENGTH; j>0;) {
                        value=data32[block+ --j];
                        if(value!=highValue) {
                            return c;
                        }
                        --c;
                    }
                }
            }
        }
    }

    /* deliver last range */
    return 0;
}

/*
 * Compact a build-time trie.
 *
 * The compaction
 * - removes blocks that are identical with earlier ones
 * - overlaps adjacent blocks as much as possible (if overlap==TRUE)
 * - moves blocks in steps of the data granularity
 * - moves and overlaps blocks that overlap with multiple values in the overlap region
 *
 * It does not
 * - try to move and overlap blocks that are not already adjacent
 */
static void
compactData(UNewTrie2 *trie) {
    int32_t start, newStart, movedStart;
    int32_t blockLength, overlap;
    int32_t i, mapIndex, blockCount;

    /* do not compact linear-ASCII data */
    newStart=UTRIE2_DATA_START_OFFSET;
    for(start=0, i=0; start<newStart; start+=UTRIE2_DATA_BLOCK_LENGTH, ++i) {
        trie->map[i]=start;
    }

    /*
     * Start with a block length of 64 for 2-byte UTF-8,
     * then switch to UTRIE2_DATA_BLOCK_LENGTH.
     */
    blockLength=64;
    blockCount=blockLength>>UTRIE2_SHIFT_2;
    for(start=newStart; start<trie->dataLength;) {
        /*
         * start: index of first entry of current block
         * newStart: index where the current block is to be moved
         *           (right after current end of already-compacted data)
         */
        if(start==UNEWTRIE2_DATA_0800_OFFSET) {
            blockLength=UTRIE2_DATA_BLOCK_LENGTH;
            blockCount=1;
        }

        /* skip blocks that are not used */
        if(trie->map[start>>UTRIE2_SHIFT_2]<=0) {
            /* advance start to the next block */
            start+=blockLength;

            /* leave newStart with the previous block! */
            continue;
        }

        /* search for an identical block */
        if( (movedStart=findSameDataBlock(trie->data, newStart, start, blockLength))
             >=0
        ) {
            /* found an identical block, set the other block's index value for the current block */
            for(i=blockCount, mapIndex=start>>UTRIE2_SHIFT_2; i>0; --i) {
                trie->map[mapIndex++]=movedStart;
                movedStart+=UTRIE2_DATA_BLOCK_LENGTH;
            }

            /* advance start to the next block */
            start+=blockLength;

            /* leave newStart with the previous block! */
            continue;
        }

        /* see if the beginning of this block can be overlapped with the end of the previous block */
        /* look for maximum overlap (modulo granularity) with the previous, adjacent block */
        for(overlap=blockLength-UTRIE2_DATA_GRANULARITY;
            overlap>0 && !equal_uint32(trie->data+(newStart-overlap), trie->data+start, overlap);
            overlap-=UTRIE2_DATA_GRANULARITY) {}

        if(overlap>0 || newStart<start) {
            /* some overlap, or just move the whole block */
            movedStart=newStart-overlap;
            for(i=blockCount, mapIndex=start>>UTRIE2_SHIFT_2; i>0; --i) {
                trie->map[mapIndex++]=movedStart;
                movedStart+=UTRIE2_DATA_BLOCK_LENGTH;
            }

            /* move the non-overlapping indexes to their new positions */
            start+=overlap;
            for(i=blockLength-overlap; i>0; --i) {
                trie->data[newStart++]=trie->data[start++];
            }
        } else /* no overlap && newStart==start */ {
            for(i=blockCount, mapIndex=start>>UTRIE2_SHIFT_2; i>0; --i) {
                trie->map[mapIndex++]=start;
                start+=UTRIE2_DATA_BLOCK_LENGTH;
            }
            newStart=start;
        }
    }

    /* now adjust the index-2 table */
    for(i=0; i<trie->index2Length; ++i) {
        trie->index2[i]=trie->map[trie->index2[i]>>UTRIE2_SHIFT_2];
    }
    trie->dataNullOffset=trie->map[trie->dataNullOffset>>UTRIE2_SHIFT_2];

    /* ensure dataLength alignment */
    while((newStart&(UTRIE2_DATA_GRANULARITY-1))!=0) {
        /* Set initialValue in case this is used with unewtrie2_getData(). */
        trie->data[newStart++]=trie->initialValue;
    }

#ifdef UTRIE2_DEBUG
    /* we saved some space */
    printf("compacting UTrie2: count of 32-bit data words %lu->%lu\n",
            (long)trie->dataLength, (long)newStart);
#endif

    trie->dataLength=newStart;
}

static void
compactIndex2(UNewTrie2 *trie) {
    int32_t i, start, newStart, movedStart, overlap;

    /* do not compact linear-ASCII index-2 blocks */
    newStart=UTRIE2_INDEX_2_BMP_LENGTH;
    for(start=0, i=0; start<newStart; start+=UTRIE2_INDEX_2_BLOCK_LENGTH, ++i) {
        trie->map[i]=start;
    }

    /* Reduce the index table gap to what will be needed at runtime. */
    newStart+=UTRIE2_UTF8_2B_INDEX_2_LENGTH+((trie->highStart-0x10000)>>UTRIE2_SHIFT_1);

    for(start=UNEWTRIE2_INDEX_2_NULL_OFFSET; start<trie->index2Length;) {
        /*
         * start: index of first entry of current block
         * newStart: index where the current block is to be moved
         *           (right after current end of already-compacted data)
         */

        /* search for an identical block */
        if( (movedStart=findSameIndex2Block(trie->index2, newStart, start))
             >=0
        ) {
            /* found an identical block, set the other block's index value for the current block */
            trie->map[start>>UTRIE2_SHIFT_1_2]=movedStart;

            /* advance start to the next block */
            start+=UTRIE2_INDEX_2_BLOCK_LENGTH;

            /* leave newStart with the previous block! */
            continue;
        }

        /* see if the beginning of this block can be overlapped with the end of the previous block */
        /* look for maximum overlap with the previous, adjacent block */
        for(overlap=UTRIE2_INDEX_2_BLOCK_LENGTH-1;
            overlap>0 && !equal_int32(trie->index2+(newStart-overlap), trie->index2+start, overlap);
            --overlap) {}

        if(overlap>0 || newStart<start) {
            /* some overlap, or just move the whole block */
            trie->map[start>>UTRIE2_SHIFT_1_2]=newStart-overlap;

            /* move the non-overlapping indexes to their new positions */
            start+=overlap;
            for(i=UTRIE2_INDEX_2_BLOCK_LENGTH-overlap; i>0; --i) {
                trie->index2[newStart++]=trie->index2[start++];
            }
        } else /* no overlap && newStart==start */ {
            trie->map[start>>UTRIE2_SHIFT_1_2]=start;
            start+=UTRIE2_INDEX_2_BLOCK_LENGTH;
            newStart=start;
        }
    }

    /* now adjust the index-1 table */
    for(i=0; i<UNEWTRIE2_INDEX_1_LENGTH; ++i) {
        trie->index1[i]=trie->map[trie->index1[i]>>UTRIE2_SHIFT_1_2];
    }
    trie->index2NullOffset=trie->map[trie->index2NullOffset>>UTRIE2_SHIFT_1_2];

    /*
     * Ensure data table alignment:
     * Needs to be granularity-aligned for 16-bit trie
     * (so that dataMove will be down-shiftable),
     * and 2-aligned for uint32_t data.
     */
    while((newStart&((UTRIE2_DATA_GRANULARITY-1)|1))!=0) {
        /* Arbitrary value: 0x3fffc not possible for real data. */
        trie->index2[newStart++]=(int32_t)0xffff<<UTRIE2_INDEX_SHIFT;
    }

#ifdef UTRIE2_DEBUG
    /* we saved some space */
    printf("compacting UTrie2: count of 16-bit index-2 words %lu->%lu\n",
            (long)trie->index2Length, (long)newStart);
#endif

    trie->index2Length=newStart;
}

/* serialization ------------------------------------------------------------ */

/**
 * Maximum length of the runtime index array.
 * Limited by its own 16-bit index values, and by uint16_t UTrie2Header.indexLength.
 * (The actual maximum length is lower,
 * (0x110000>>UTRIE2_SHIFT_2)+UTRIE2_UTF8_2B_INDEX_2_LENGTH+UTRIE2_MAX_INDEX_1_LENGTH.)
 */
#define UTRIE2_MAX_INDEX_LENGTH 0xffff

/**
 * Maximum length of the runtime data array.
 * Limited by 16-bit index values that are left-shifted by UTRIE2_INDEX_SHIFT,
 * and by uint16_t UTrie2Header.shiftedDataLength.
 */
#define UTRIE2_MAX_DATA_LENGTH (0xffff<<UTRIE2_INDEX_SHIFT)

U_CAPI int32_t U_EXPORT2
unewtrie2_serialize(UNewTrie2 *trie, UTrie2ValueBits valueBits,
                    void *data, int32_t capacity,
                    UErrorCode *pErrorCode) {
    UTrie2Header *header;
    uint32_t *p;
    uint16_t *dest16;
    int32_t i, length;
    int32_t allIndexesLength;
    int32_t dataMove;  /* >0 if the data is moved to the end of the index array */
    UChar32 highStart;

    /* argument check */
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( trie==NULL ||
        capacity<0 || (capacity>0 && (data==NULL || (((int32_t)data&3)!=0))) ||
        valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* compact if necessary */
    if(!trie->isCompacted) {
        UChar32 suppHighStart;
        uint32_t highValue;

        /* find highStart and round it up */
        highStart=findHighStart(trie);
        highStart=(highStart+(UTRIE2_CP_PER_INDEX_1_ENTRY-1))&~(UTRIE2_CP_PER_INDEX_1_ENTRY-1);
        highValue=highStart<0x110000 ? unewtrie2_get32(trie, highStart) : trie->errorValue;

        /*
         * Set trie->highStart only after unewtrie2_get32(trie, highStart).
         * Otherwise unewtrie2_get32(trie, highStart) would try to read the highValue.
         */
        trie->highStart=highStart;

#ifdef UTRIE2_DEBUG
        printf("UTrie2: highStart U+%04lx  highValue 0x%lx  initialValue 0x%lx\n",
                (long)highStart, (long)highValue, (long)trie->initialValue);
#endif

        /* Blank out [highStart..110000[ to release associated data blocks. */
        suppHighStart= highStart<=0x10000 ? 0x10000 : highStart;
        unewtrie2_setRange32(trie, suppHighStart, 0x110000, trie->initialValue, TRUE);

        compactData(trie);
        if(highStart>0x10000) {
            compactIndex2(trie);
#ifdef UTRIE2_DEBUG
        } else {
            printf("UTrie2: highStart U+%04lx  count of 16-bit index-2 words %lu->%lu\n",
                    (long)highStart, (long)trie->index2Length, (long)UTRIE2_INDEX_1_OFFSET);
#endif
        }

        /*
         * Store the highValue in the data array and round up the dataLength.
         * Must be done after compactData() because that assumes that dataLength
         * is a multiple of UTRIE2_DATA_BLOCK_LENGTH.
         */
        trie->data[trie->dataLength++]=highValue;
        while((trie->dataLength&(UTRIE2_DATA_GRANULARITY-1))!=0) {
            /* Set initialValue in case this is used with unewtrie2_getData(). */
            trie->data[trie->dataLength++]=trie->initialValue;
        }

        trie->isCompacted=TRUE;
    } else {
        highStart=trie->highStart;
    }

    if(highStart<=0x10000) {
        allIndexesLength=UTRIE2_INDEX_1_OFFSET;
    } else {
        allIndexesLength=trie->index2Length;
    }
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        dataMove=allIndexesLength;
    } else {
        dataMove=0;
    }

    /* are indexLength and dataLength within limits? */
    if( /* for unshifted indexLength */
        allIndexesLength>UTRIE2_MAX_INDEX_LENGTH ||
        /* for unshifted dataNullOffset */
        (dataMove+trie->dataNullOffset)>0xffff ||
        /* for unshifted 2-byte UTF-8 index-2 values */
        (dataMove+UNEWTRIE2_DATA_0800_OFFSET)>0xffff ||
        /* for shiftedDataLength */
        (dataMove+trie->dataLength)>UTRIE2_MAX_DATA_LENGTH
    ) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    /* calculate the total serialized length */
    length=sizeof(UTrie2Header)+allIndexesLength*2;
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        length+=trie->dataLength*2;
    } else {
        length+=trie->dataLength*4;
    }

    if(length>capacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return length;
    }

    /* set the header fields */
    header=(UTrie2Header *)data;
    dest16=(uint16_t *)(header+1);

    header->signature=UTRIE2_SIG; /* "Tri2" */
    header->options=(uint16_t)valueBits;

    header->indexLength=(uint16_t)allIndexesLength;
    header->shiftedDataLength=(uint16_t)(trie->dataLength>>UTRIE2_INDEX_SHIFT);
    header->index2NullOffset=(uint16_t)(UTRIE2_INDEX_2_OFFSET+trie->index2NullOffset);
    header->dataNullOffset=(uint16_t)(dataMove+trie->dataNullOffset);
    header->shiftedHighStart=(uint16_t)(highStart>>UTRIE2_SHIFT_1);

    /* write the index-2 array values shifted right by UTRIE2_INDEX_SHIFT, after adding dataMove */
    p=(uint32_t *)trie->index2;
    for(i=UTRIE2_INDEX_2_BMP_LENGTH; i>0; --i) {
        *dest16++=(uint16_t)((dataMove + *p++)>>UTRIE2_INDEX_SHIFT);
    }

    /* write UTF-8 2-byte index-2 values, not right-shifted */
    for(i=0; i<(0xc2-0xc0); ++i) {                                  /* C0..C1 */
        *dest16++=(uint16_t)(dataMove+UTRIE2_BAD_UTF8_DATA_OFFSET);
    }
    for(; i<(0xe0-0xc0); ++i) {                                     /* C2..DF */
        *dest16++=(uint16_t)(dataMove+trie->index2[i<<(6-UTRIE2_SHIFT_2)]);
    }

    if(highStart>0x10000) {
        int32_t index1Length=(highStart-0x10000)>>UTRIE2_SHIFT_1;
        int32_t index2Offset=UTRIE2_INDEX_2_BMP_LENGTH+UTRIE2_UTF8_2B_INDEX_2_LENGTH+index1Length;

        /* write 16-bit index-1 values for supplementary code points */
        p=(uint32_t *)trie->index1+UTRIE2_OMITTED_BMP_INDEX_1_LENGTH;
        for(i=index1Length; i>0; --i) {
            *dest16++=(uint16_t)(UTRIE2_INDEX_2_OFFSET + *p++);
        }

        /*
         * write the index-2 array values for supplementary code points,
         * shifted right by UTRIE2_INDEX_SHIFT, after adding dataMove
         */
        p=(uint32_t *)trie->index2+index2Offset;
        for(i=trie->index2Length-index2Offset; i>0; --i) {
            *dest16++=(uint16_t)((dataMove + *p++)>>UTRIE2_INDEX_SHIFT);
        }
    }

    /* write the 16/32-bit data array */
    switch(valueBits) {
    case UTRIE2_16_VALUE_BITS:
        /* write 16-bit data values */
        p=trie->data;
        for(i=trie->dataLength; i>0; --i) {
            *dest16++=(uint16_t)*p++;
        }
        break;
    case UTRIE2_32_VALUE_BITS:
        /* write 32-bit data values */
        uprv_memcpy(dest16, trie->data, trie->dataLength*4);
        break;
    default:
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    return length;
}

/* enumeration -------------------------------------------------------------- */

/* default UTrie2EnumValue() returns the input value itself */
static uint32_t U_CALLCONV
enumSameValue(const void *context, uint32_t value) {
    return value;
}

/**
 * Enumerate all ranges of code points with the same relevant values.
 * The values are transformed from the raw trie entries by the enumValue function.
 *
 * Optimizations:
 * - Skip a whole block if we know that it is filled with a single value,
 *   and it is the same as we visited just before.
 * - Handle the null block specially because we know a priori that it is filled
 *   with a single value.
 */
static void
enumEitherTrie(const UTrie2 *runTimeTrie, const UNewTrie2 *newTrie,
               UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange, const void *context) {
    const uint32_t *data32;
    const uint16_t *index;

    uint32_t value, prevValue, initialValue;
    UChar32 c, prev, highStart;
    int32_t i1, i2, j, i2Block, prevI2Block, index2NullOffset, block, prevBlock, nullBlock;

    if(enumValue==NULL) {
        enumValue=enumSameValue;
    }

    if(runTimeTrie!=NULL) {
        index=runTimeTrie->index;
        data32=runTimeTrie->data32;

        highStart=runTimeTrie->highStart;

        /* get the enumeration value that corresponds to an initial-value trie data entry */
        initialValue=enumValue(context, runTimeTrie->initialValue);

        index2NullOffset=runTimeTrie->index2NullOffset;
        nullBlock=runTimeTrie->dataNullOffset;
    } else {
        index=NULL;
        data32=newTrie->data;

        highStart=newTrie->highStart;

        /* get the enumeration value that corresponds to an initial-value trie data entry */
        initialValue=enumValue(context, newTrie->initialValue);

        index2NullOffset=newTrie->index2NullOffset;
        nullBlock=newTrie->dataNullOffset;
    }

    /* set variables for previous range */
    prevI2Block=-1;
    prevBlock=-1;
    prev=0;
    prevValue=0;

    /* enumerate index-2 blocks */
    i1=0;
    i2Block=0;
    for(c=0; c<highStart;) {
        if(runTimeTrie!=NULL) {
            if(c<=0xffff) {
                i2Block=c>>UTRIE2_SHIFT_2;
            } else {
                i2Block=index[UTRIE2_INDEX_1_OFFSET+i1++];
            }
        } else {
            i2Block=newTrie->index1[i1++];
        }
        if(i2Block==prevI2Block && (c-prev)>=UTRIE2_CP_PER_INDEX_1_ENTRY) {
            /* the index-2 block is the same as the previous one, and filled with prevValue */
            c+=UTRIE2_CP_PER_INDEX_1_ENTRY;
            continue;
        }
        prevI2Block=i2Block;
        if(i2Block==index2NullOffset) {
            /* this is the null index-2 block */
            if(prevValue!=initialValue) {
                if(prev<c && !enumRange(context, prev, c, prevValue)) {
                    return;
                }
                prevBlock=nullBlock;
                prev=c;
                prevValue=initialValue;
            }
            c+=UTRIE2_CP_PER_INDEX_1_ENTRY;
        } else {
            /* enumerate data blocks for one index-2 block */
            for(i2=0; i2<UTRIE2_INDEX_2_BLOCK_LENGTH; ++i2) {
                if(runTimeTrie!=NULL) {
                    block=(int32_t)index[i2Block+i2]<<UTRIE2_INDEX_SHIFT;
                } else {
                    block=newTrie->index2[i2Block+i2];
                }
                if(block==prevBlock && (c-prev)>=UTRIE2_DATA_BLOCK_LENGTH) {
                    /* the block is the same as the previous one, and filled with prevValue */
                    c+=UTRIE2_DATA_BLOCK_LENGTH;
                    continue;
                }
                prevBlock=block;
                if(block==nullBlock) {
                    /* this is the null data block */
                    if(prevValue!=initialValue) {
                        if(prev<c && !enumRange(context, prev, c, prevValue)) {
                            return;
                        }
                        prev=c;
                        prevValue=initialValue;
                    }
                    c+=UTRIE2_DATA_BLOCK_LENGTH;
                } else {
                    for(j=0; j<UTRIE2_DATA_BLOCK_LENGTH; ++j) {
                        value=enumValue(context, data32!=NULL ? data32[block+j] : index[block+j]);
                        if(value!=prevValue) {
                            if(prev<c && !enumRange(context, prev, c, prevValue)) {
                                return;
                            }
                            prev=c;
                            prevValue=value;
                        }
                        ++c;
                    }
                }
            }
        }
    }

    /* c==highStart */
    if(c<0x110000) {
        uint32_t highValue;
        if(runTimeTrie!=NULL) {
            highValue=
                data32!=NULL ?
                    data32[runTimeTrie->highValueIndex] :
                    index[runTimeTrie->highValueIndex];
        } else {
            highValue=newTrie->data[newTrie->dataLength-UTRIE2_DATA_GRANULARITY];
        }
        value=enumValue(context, highValue);
        if(value!=prevValue) {
            if(prev<c && !enumRange(context, prev, c, prevValue)) {
                return;
            }
            prev=c;
            prevValue=value;
        }
    }

    /* deliver last range */
    enumRange(context, prev, 0x110000, prevValue);
}

U_CAPI void U_EXPORT2
utrie2_enum(const UTrie2 *trie,
            UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange, const void *context) {
    if(trie==NULL || trie->index==NULL || enumRange==NULL) {
        return;
    }
    enumEitherTrie(trie, NULL, enumValue, enumRange, context);
}

U_CAPI void U_EXPORT2
unewtrie2_enum(const UNewTrie2 *trie,
               UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange, const void *context) {
    if(trie==NULL || enumRange==NULL) {
        return;
    }
    enumEitherTrie(NULL, trie, enumValue, enumRange, context);
}

U_CAPI void U_EXPORT2
unewtrie2_enumForLeadSurrogate(const UNewTrie2 *newTrie, UChar32 lead,
                               UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange,
                               const void *context) {
    uint32_t prevValue;
    UChar32 c, prev;
    int32_t i1, i2;

    if(newTrie==NULL || enumRange==NULL || !U16_IS_LEAD(lead)) {
        return;
    }

    if(enumValue==NULL) {
        enumValue=enumSameValue;
    }

    lead-=0xd7c0;                   /* start code point shifted right by 10 (lead=c>>10) */
    prev=c=lead<<10;                /* start code point */
    if(c>=newTrie->highStart) {
        enumRange(context,
                  c, c+0x400,
                  enumValue(context, newTrie->data[newTrie->dataLength-UTRIE2_DATA_GRANULARITY]));
        return;
    }

    /* get the enumeration value that corresponds to an initial-value trie data entry */
    prevValue=enumValue(context, newTrie->initialValue);

    i1=lead>>(UTRIE2_SHIFT_1-10);   /* >>2 */
    i2=newTrie->index1[i1];         /* same index-2 block for all of these 1024 code points */
    if(i2==newTrie->index2NullOffset) {
        /* this is the null index-2 block */
        c+=0x400;
    } else {
        const uint32_t *data32;
        uint32_t value, initialValue;
        int32_t i2Limit, j, block, prevBlock, nullBlock;

        initialValue=prevValue;
        data32=newTrie->data;
        prevBlock=nullBlock=newTrie->dataNullOffset;

        /* enumerate data blocks for half of this index-2 block */
        /* i2+=((c>>5)&0x3f)=((lead<<5)&0x3f)=((lead&1)<<5) where 5==UTRIE2_SHIFT_2 */
        i2+=(c>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK;
        i2Limit=i2+(0x400>>UTRIE2_SHIFT_2); /* +0x10 */
        for(; i2<i2Limit; ++i2) {
            block=newTrie->index2[i2];
            if(block==prevBlock && (c-prev)>=UTRIE2_DATA_BLOCK_LENGTH) {
                /* the block is the same as the previous one, and filled with prevValue */
                c+=UTRIE2_DATA_BLOCK_LENGTH;
                continue;
            }
            prevBlock=block;
            if(block==nullBlock) {
                /* this is the null data block */
                if(prevValue!=initialValue) {
                    if(prev<c && !enumRange(context, prev, c, prevValue)) {
                        return;
                    }
                    prev=c;
                    prevValue=initialValue;
                }
                c+=UTRIE2_DATA_BLOCK_LENGTH;
            } else {
                for(j=0; j<UTRIE2_DATA_BLOCK_LENGTH; ++j) {
                    value=enumValue(context, data32[block+j]);
                    if(value!=prevValue) {
                        if(prev<c && !enumRange(context, prev, c, prevValue)) {
                            return;
                        }
                        prev=c;
                        prevValue=value;
                    }
                    ++c;
                }
            }
        }
    }

    /* deliver last range */
    enumRange(context, prev, c, prevValue);
}
