/*
******************************************************************************
*
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  utrie.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001oct20
*   created by: Markus W. Scherer
*
*   This is a common implementation of a "folded" trie.
*   It is a kind of compressed, serializable table of 16- or 32-bit values associated with
*   Unicode code points (0..0x10ffff).
*/

#ifdef UTRIE_DEBUG
#   include <stdio.h>
#endif

#include "unicode/utypes.h"
#include "cmemory.h"
#include "utrie.h"

#undef ABS
#define ABS(x) ((x)>=0 ? (x) : -(x))

/* Building a trie ----------------------------------------------------------*/

U_CAPI UNewTrie * U_EXPORT2
utrie_open(UNewTrie *fillIn,
           uint32_t *aliasData, int32_t maxDataLength,
           uint32_t initialValue, UBool latin1Linear) {
    UNewTrie *trie;
    int32_t i, j;

    if( maxDataLength<UTRIE_DATA_BLOCK_LENGTH ||
        (latin1Linear && maxDataLength<1024)
    ) {
        return NULL;
    }

    if(fillIn!=NULL) {
        trie=fillIn;
    } else {
        trie=(UNewTrie *)uprv_malloc(sizeof(UNewTrie));
        if(trie==NULL) {
            return NULL;
        }
    }
    uprv_memset(trie, 0, sizeof(UNewTrie));
    trie->isAllocated= (UBool)(fillIn==NULL);

    if(aliasData!=NULL) {
        trie->data=aliasData;
        trie->isDataAllocated=FALSE;
    } else {
        trie->data=(uint32_t *)uprv_malloc(maxDataLength*4);
        if(trie->data==NULL) {
            uprv_free(trie);
            return NULL;
        }
        trie->isDataAllocated=TRUE;
    }

    /* preallocate and reset the first data block (block index 0) */
    j=UTRIE_DATA_BLOCK_LENGTH;

    if(latin1Linear) {
        /* preallocate and reset the first block (number 0) and Latin-1 (U+0000..U+00ff) after that */
        /* made sure above that maxDataLength>=1024 */

        /* set indexes to point to consecutive data blocks */
        i=0;
        do {
            /* do this at least for trie->index[0] even if that block is only partly used for Latin-1 */
            trie->index[i++]=j;
            j+=UTRIE_DATA_BLOCK_LENGTH;
        } while(i<(256>>UTRIE_SHIFT));
    }

    /* reset the initially allocated blocks to the initial value */
    trie->dataLength=j;
    while(j>0) {
        trie->data[--j]=initialValue;
    }

    trie->indexLength=UTRIE_MAX_INDEX_LENGTH;
    trie->dataCapacity=maxDataLength;
    trie->isLatin1Linear=latin1Linear;
    trie->isCompacted=FALSE;
    return trie;
}

U_CAPI UNewTrie * U_EXPORT2
utrie_clone(UNewTrie *fillIn, const UNewTrie *other, uint32_t *aliasData, int32_t aliasDataCapacity) {
    UNewTrie *trie;
    UBool isDataAllocated;

    /* do not clone if other is not valid or already compacted */
    if(other==NULL || other->data==NULL || other->isCompacted) {
        return NULL;
    }

    /* clone data */
    if(aliasData!=NULL && aliasDataCapacity>=other->dataCapacity) {
        isDataAllocated=FALSE;
    } else {
        aliasDataCapacity=other->dataCapacity;
        aliasData=(uint32_t *)uprv_malloc(other->dataCapacity*4);
        if(aliasData==NULL) {
            return NULL;
        }
        isDataAllocated=TRUE;
    }

    trie=utrie_open(fillIn, aliasData, aliasDataCapacity, other->data[0], other->isLatin1Linear);
    if(trie==NULL) {
        uprv_free(aliasData);
    } else {
        uprv_memcpy(trie->index, other->index, sizeof(trie->index));
        uprv_memcpy(trie->data, other->data, other->dataLength*4);
        trie->dataLength=other->dataLength;
        trie->isDataAllocated=isDataAllocated;
    }

    return trie;
}

U_CAPI void U_EXPORT2
utrie_close(UNewTrie *trie) {
    if(trie!=NULL) {
        if(trie->isDataAllocated) {
            uprv_free(trie->data);
            trie->data=NULL;
        }
        if(trie->isAllocated) {
            uprv_free(trie);
        }
    }
}

U_CAPI uint32_t * U_EXPORT2
utrie_getData(UNewTrie *trie, int32_t *pLength) {
    if(trie==NULL || pLength==NULL) {
        return NULL;
    }

    *pLength=trie->dataLength;
    return trie->data;
}

/**
 * No error checking for illegal arguments.
 *
 * @return -1 if no new data block available (out of memory in data array)
 * @internal
 */
static int32_t
utrie_getDataBlock(UNewTrie *trie, UChar32 c) {
    int32_t indexValue, newBlock, newTop;

    c>>=UTRIE_SHIFT;
    indexValue=trie->index[c];
    if(indexValue>0) {
        return indexValue;
    }

    /* allocate a new data block */
    newBlock=trie->dataLength;
    newTop=newBlock+UTRIE_DATA_BLOCK_LENGTH;
    if(newTop>trie->dataCapacity) {
        /* out of memory in the data array */
        return -1;
    }
    trie->dataLength=newTop;
    trie->index[c]=newBlock;

    /* copy-on-write for a block from a setRange() */
    uprv_memcpy(trie->data+newBlock, trie->data-indexValue, 4*UTRIE_DATA_BLOCK_LENGTH);
    return newBlock;
}

/**
 * @return TRUE if the value was successfully set
 */
U_CAPI UBool U_EXPORT2
utrie_set32(UNewTrie *trie, UChar32 c, uint32_t value) {
    int32_t block;

    /* valid, uncompacted trie and valid c? */
    if(trie==NULL || trie->isCompacted || (uint32_t)c>0x10ffff) {
        return FALSE;
    }

    block=utrie_getDataBlock(trie, c);
    if(block<0) {
        return FALSE;
    }

    trie->data[block+(c&UTRIE_MASK)]=value;
    return TRUE;
}

U_CAPI uint32_t U_EXPORT2
utrie_get32(UNewTrie *trie, UChar32 c, UBool *pInBlockZero) {
    int32_t block;

    /* valid, uncompacted trie and valid c? */
    if(trie==NULL || trie->isCompacted || (uint32_t)c>0x10ffff) {
        if(pInBlockZero!=NULL) {
            *pInBlockZero=TRUE;
        }
        return 0;
    }

    block=trie->index[c>>UTRIE_SHIFT];
    if(pInBlockZero!=NULL) {
        *pInBlockZero= (UBool)(block==0);
    }

    return trie->data[ABS(block)+(c&UTRIE_MASK)];
}

/**
 * @internal
 */
static void
utrie_fillBlock(uint32_t *block, UChar32 start, UChar32 limit,
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
utrie_setRange32(UNewTrie *trie, UChar32 start, UChar32 limit, uint32_t value, UBool overwrite) {
    /*
     * repeat value in [start..limit[
     * mark index values for repeat-data blocks by setting bit 31 of the index values
     * fill around existing values if any, if(overwrite)
     */
    uint32_t initialValue;
    int32_t block, rest, repeatBlock;

    /* valid, uncompacted trie and valid indexes? */
    if( trie==NULL || trie->isCompacted ||
        (uint32_t)start>0x10ffff || (uint32_t)limit>0x110000 || start>limit
    ) {
        return FALSE;
    }
    if(start==limit) {
        return TRUE; /* nothing to do */
    }

    initialValue=trie->data[0];
    if(start&UTRIE_MASK) {
        UChar32 nextStart;

        /* set partial block at [start..following block boundary[ */
        block=utrie_getDataBlock(trie, start);
        if(block<0) {
            return FALSE;
        }

        nextStart=(start+UTRIE_DATA_BLOCK_LENGTH)&~UTRIE_MASK;
        if(nextStart<=limit) {
            utrie_fillBlock(trie->data+block, start&UTRIE_MASK, UTRIE_DATA_BLOCK_LENGTH,
                            value, initialValue, overwrite);
            start=nextStart;
        } else {
            utrie_fillBlock(trie->data+block, start&UTRIE_MASK, limit&UTRIE_MASK,
                            value, initialValue, overwrite);
            return TRUE;
        }
    }

    /* number of positions in the last, partial block */
    rest=limit&UTRIE_MASK;

    /* round down limit to a block boundary */
    limit&=~UTRIE_MASK;

    /* iterate over all-value blocks */
    if(value==initialValue) {
        repeatBlock=0;
    } else {
        repeatBlock=-1;
    }
    while(start<limit) {
        /* get index value */
        block=trie->index[start>>UTRIE_SHIFT];
        if(block>0) {
            /* already allocated, fill in value */
            utrie_fillBlock(trie->data+block, 0, UTRIE_DATA_BLOCK_LENGTH, value, initialValue, overwrite);
        } else if(trie->data[-block]!=value && (block==0 || overwrite)) {
            /* set the repeatBlock instead of the current block 0 or range block */
            if(repeatBlock>=0) {
                trie->index[start>>UTRIE_SHIFT]=-repeatBlock;
            } else {
                /* create and set and fill the repeatBlock */
                repeatBlock=utrie_getDataBlock(trie, start);
                if(repeatBlock<0) {
                    return FALSE;
                }

                /* set the negative block number to indicate that it is a repeat block */
                trie->index[start>>UTRIE_SHIFT]=-repeatBlock;
                utrie_fillBlock(trie->data+repeatBlock, 0, UTRIE_DATA_BLOCK_LENGTH, value, initialValue, TRUE);
            }
        }

        start+=UTRIE_DATA_BLOCK_LENGTH;
    }

    if(rest>0) {
        /* set partial block at [last block boundary..limit[ */
        block=utrie_getDataBlock(trie, start);
        if(block<0) {
            return FALSE;
        }

        utrie_fillBlock(trie->data+block, 0, rest, value, initialValue, overwrite);
    }

    return TRUE;
}

static int32_t
_findSameIndexBlock(const int32_t *index, int32_t indexLength,
                    int32_t otherBlock) {
    int32_t block, i;

    for(block=UTRIE_BMP_INDEX_LENGTH; block<indexLength; block+=UTRIE_SURROGATE_BLOCK_COUNT) {
        for(i=0; i<UTRIE_SURROGATE_BLOCK_COUNT; ++i) {
            if(index[block+i]!=index[otherBlock+i]) {
                break;
            }
        }
        if(i==UTRIE_SURROGATE_BLOCK_COUNT) {
            return block;
        }
    }
    return indexLength;
}

/*
 * Fold the normalization data for supplementary code points into
 * a compact area on top of the BMP-part of the trie index,
 * with the lead surrogates indexing this compact area.
 *
 * Duplicate the index values for lead surrogates:
 * From inside the BMP area, where some may be overridden with folded values,
 * to just after the BMP area, where they can be retrieved for
 * code point lookups.
 */
static void
utrie_fold(UNewTrie *trie, UNewTrieGetFoldedValue *getFoldedValue, UErrorCode *pErrorCode) {
    int32_t leadIndexes[UTRIE_SURROGATE_BLOCK_COUNT];
    int32_t *index;
    uint32_t value;
    UChar32 c;
    int32_t indexLength, block;

    index=trie->index;

    /* copy the lead surrogate indexes into a temporary array */
    uprv_memcpy(leadIndexes, index+(0xd800>>UTRIE_SHIFT), 4*UTRIE_SURROGATE_BLOCK_COUNT);

    /*
     * to protect the copied lead surrogate values,
     * mark all their indexes as repeat blocks
     * (causes copy-on-write)
     */
    for(c=0xd800; c<=0xdbff; ++c) {
        block=index[c>>UTRIE_SHIFT];
        if(block>0) {
            index[c>>UTRIE_SHIFT]=-block;
        }
    }

    /*
     * Fold significant index values into the area just after the BMP indexes.
     * In case the first lead surrogate has significant data,
     * its index block must be used first (in which case the folding is a no-op).
     * Later all folded index blocks are moved up one to insert the copied
     * lead surrogate indexes.
     */
    indexLength=UTRIE_BMP_INDEX_LENGTH;

    /* search for any index (stage 1) entries for supplementary code points */
    for(c=0x10000; c<0x110000;) {
        if(index[c>>UTRIE_SHIFT]!=0) {
            /* there is data, treat the full block for a lead surrogate */
            c&=~0x3ff;

#ifdef UTRIE_DEBUG
            printf("supplementary data for lead surrogate U+%04lx\n", (long)(0xd7c0+(c>>10)));
#endif

            /* is there an identical index block? */
            block=_findSameIndexBlock(index, indexLength, c>>UTRIE_SHIFT);

            /* get a folded value for [c..c+0x400[ and, if 0, set it for the lead surrogate */
            value=getFoldedValue(trie, c, block+UTRIE_SURROGATE_BLOCK_COUNT);
            if(value!=0) {
                if(!utrie_set32(trie, 0xd7c0+(c>>10), value)) {
                    /* data table overflow */
                    *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                    return;
                }

                /* if we did not find an identical index block... */
                if(block==indexLength) {
                    /* move the actual index (stage 1) entries from the supplementary position to the new one */
                    uprv_memmove(index+indexLength,
                                 index+(c>>UTRIE_SHIFT),
                                 4*UTRIE_SURROGATE_BLOCK_COUNT);
                    indexLength+=UTRIE_SURROGATE_BLOCK_COUNT;
                }
            }
            c+=0x400;
        } else {
            c+=UTRIE_DATA_BLOCK_LENGTH;
        }
    }

    /*
     * index array overflow?
     * This is to guarantee that a folding offset is of the form
     * UTRIE_BMP_INDEX_LENGTH+n*UTRIE_SURROGATE_BLOCK_COUNT with n=0..1023.
     * If the index is too large, then n>=1024 and more than 10 bits are necessary.
     *
     * In fact, it can only ever become n==1024 with completely unfoldable data and
     * the additional block of duplicated values for lead surrogates.
     */
    if(indexLength>=UTRIE_MAX_INDEX_LENGTH) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }

    /*
     * make space for the lead surrogate index block and
     * insert it between the BMP indexes and the folded ones
     */
    uprv_memmove(index+UTRIE_BMP_INDEX_LENGTH+UTRIE_SURROGATE_BLOCK_COUNT,
                 index+UTRIE_BMP_INDEX_LENGTH,
                 4*(indexLength-UTRIE_BMP_INDEX_LENGTH));
    uprv_memcpy(index+UTRIE_BMP_INDEX_LENGTH,
                leadIndexes,
                4*UTRIE_SURROGATE_BLOCK_COUNT);
    indexLength+=UTRIE_SURROGATE_BLOCK_COUNT;

#ifdef UTRIE_DEBUG
    printf("trie index count: BMP %ld  all Unicode %ld  folded %ld\n",
           UTRIE_BMP_INDEX_LENGTH, (long)UTRIE_MAX_INDEX_LENGTH, indexLength);
#endif

    trie->indexLength=indexLength;
    return;
}

/*
 * Compact a folded build-time trie.
 *
 * The compaction
 * - removes all-initial-value blocks
 * - maps all blocks that are completely filled with the same values to only of them
 * - overlaps adjacent blocks as much as possible
 *
 * It does not
 * - find blocks that are identical but not completely filled with the same value
 * - try to move and overlap blocks that are not already adjacent
 */
static void
utrie_compact(UNewTrie *trie, UErrorCode *pErrorCode) {
    /*
     * Map of whole blocks that are filled with all the same value.
     * The first such block per value is stored in this lookup table,
     * and following blocks will be replaced with the previous block's index.
     */
    uint32_t wholeBlockValues[64];
    int32_t wholeBlockIndexes[64];

    uint32_t x;
    int32_t i, start, prevEnd, newStart, overlapStart, countWholeBlocks;
    UBool addWholeBlock;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    /* valid, uncompacted trie? */
    if(trie==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(trie->isCompacted) {
        return; /* nothing left to do */
    }

    /* compaction */

    /* never move the all-initial-value block 0 */
    trie->map[0]=0;

    /* prime the whole blocks lookup table with the all-initial-value block 0 */
    wholeBlockValues[0]=trie->data[0];
    wholeBlockIndexes[0]=0;
    countWholeBlocks=1;

    /* if Latin-1 is preallocated and linear, then do not compact Latin-1 data */
    if(trie->isLatin1Linear && UTRIE_SHIFT<=8) {
        overlapStart=UTRIE_DATA_BLOCK_LENGTH+256;
    } else {
        overlapStart=UTRIE_DATA_BLOCK_LENGTH;
    }

    newStart=UTRIE_DATA_BLOCK_LENGTH;
    prevEnd=newStart-1;
    for(start=newStart; start<trie->dataLength;) {
        /*
         * start: index of first entry of current block
         * prevEnd: index to last entry of previous block
         * newStart: index where the current block is to be moved
         */

        /* x: first value in the current block */
        x=trie->data[start];
        addWholeBlock=FALSE;

        /* see if the current block is filled with this value x */
        for(i=1; i<UTRIE_DATA_BLOCK_LENGTH && x==trie->data[start+i]; ++i) {}
        if(i==UTRIE_DATA_BLOCK_LENGTH) {
            /*
             * yes, the block is filled with x
             * if this is the first such block, then add it to the whole block lookup table,
             * but defer that until after overlap checking
             */
            if(countWholeBlocks<sizeof(wholeBlockValues)/4) {
                addWholeBlock=TRUE;
            }

            /* did we already see another block that is also filled with x? */
            for(i=0; i<countWholeBlocks; ++i) {
                if(x==wholeBlockValues[i]) {
                    if(start>=overlapStart) {
                        /* yes, set the other block's index value for the current block */
                        trie->map[start>>UTRIE_SHIFT]=wholeBlockIndexes[i];

                        /* advance start to the next block */
                        start+=UTRIE_DATA_BLOCK_LENGTH;

                        /* leave prevEnd and newStart with the previous block! */
                        break; /* Java: continue outerLoop; */
                    } else {
                        /*
                         * Latin-1 is linear and this is a Latin-1 block
                         * do not replace its index value (to keep it linear)
                         * do not add it into the whole blocks lookup table
                         *   (because an equivalent block is in there already)
                         * finish the rest of the outer loop
                         */
                        addWholeBlock=FALSE;
                    }
                }
            }
            if(i<countWholeBlocks) {
                /*
                 * this is from the break in the previous loop
                 * because C does not allow a multi-level "continue outer loop";
                 * we have replaced this block's index value by a repeat block
                 */
                continue;
            }
        }

        /* see if the beginning of this block can be overlapped with the end of the previous block */
        if(x==trie->data[prevEnd] && start>=overlapStart) {
            /* overlap by at least one */
            for(i=1; i<UTRIE_DATA_BLOCK_LENGTH && x==trie->data[start+i] && x==trie->data[prevEnd-i]; ++i) {}

            /* overlap by i, rounded down for the data block granularity */
            i&=~(UTRIE_DATA_GRANULARITY-1);
        } else {
            i=0;
        }

        if(addWholeBlock) {
            /* add this block to the lookup table */
            wholeBlockValues[countWholeBlocks]=x;
            wholeBlockIndexes[countWholeBlocks]=newStart-i;
            ++countWholeBlocks;
        }

        if(i>0) {
            /* some overlap */
            trie->map[start>>UTRIE_SHIFT]=newStart-i;

            /* move the non-overlapping indexes to their new positions */
            start+=i;
            for(i=UTRIE_DATA_BLOCK_LENGTH-i; i>0; --i) {
                trie->data[newStart++]=trie->data[start++];
            }
        } else if(newStart<start) {
            /* no overlap, just move the indexes to their new positions */
            trie->map[start>>UTRIE_SHIFT]=newStart;
            for(i=UTRIE_DATA_BLOCK_LENGTH; i>0; --i) {
                trie->data[newStart++]=trie->data[start++];
            }
        } else /* no overlap && newStart==start */ {
            trie->map[start>>UTRIE_SHIFT]=start;
            newStart+=UTRIE_DATA_BLOCK_LENGTH;
            start=newStart;
        }

        prevEnd=newStart-1;
    }

    /* now adjust the index (stage 1) table */
    for(i=0; i<trie->indexLength; ++i) {
        trie->index[i]=trie->map[ABS(trie->index[i])>>UTRIE_SHIFT];
    }

#ifdef UTRIE_DEBUG
    /* we saved some space */
    printf("compacting trie: count of 32-bit words %lu->%lu\n",
            (long)trie->dataLength, (long)newStart);
#endif

    trie->dataLength=newStart;
    return;
}

/* serialization ------------------------------------------------------------ */

/**
 * Trie data structure in serialized form:
 *
 * UTrieHeader header;
 * uint16_t index[header.indexLength];
 * uint16_t data[header.dataLength];
 */
struct UTrieHeader {
    /** "Trie" in big-endian US-ASCII (0x54726965) */
    uint32_t signature;

    /**
     * options bit field:
     *     9    1=Latin-1 data is stored linearly at data+UTRIE_DATA_BLOCK_LENGTH
     *     8    0=16-bit data, 1=32-bit data
     *  7..4    UTRIE_INDEX_SHIFT   // 0..UTRIE_SHIFT
     *  3..0    UTRIE_SHIFT         // 1..9
     */
    uint32_t options;

    /** indexLength is a multiple of 1024>>UTRIE_SHIFT */
    int32_t indexLength;

    /** dataLength>=UTRIE_DATA_BLOCK_LENGTH */
    int32_t dataLength;
};

typedef struct UTrieHeader UTrieHeader;

/**
 * Constants for use with UTrieHeader.options.
 */
enum {
    /** Mask to get the UTRIE_SHIFT value from options. */
    UTRIE_OPTIONS_SHIFT_MASK=0xf,

    /** Shift options right this much to get the UTRIE_INDEX_SHIFT value. */
    UTRIE_OPTIONS_INDEX_SHIFT=4,

    /** If set, then the data (stage 2) array is 32 bits wide. */
    UTRIE_OPTIONS_DATA_IS_32_BIT=0x100,

    /**
     * If set, then Latin-1 data (for U+0000..U+00ff) is stored in the data (stage 2) array
     * as a simple, linear array at data+UTRIE_DATA_BLOCK_LENGTH.
     */
    UTRIE_OPTIONS_LATIN1_IS_LINEAR=0x200
};

U_CAPI int32_t U_EXPORT2
utrie_serialize(UNewTrie *trie, uint8_t *data, int32_t capacity,
                UNewTrieGetFoldedValue *getFoldedValue,
                UBool reduceTo16Bits,
                UErrorCode *pErrorCode) {
    UTrieHeader *header;
    uint32_t *p;
    uint16_t *dest16;
    int32_t i, length;

    /* argument check */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(trie==NULL || capacity<0 || (capacity>0 && data==NULL) || getFoldedValue==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* fold and compact if necessary, also checks that indexLength is within limits */
    if(!trie->isCompacted) {
        utrie_fold(trie, getFoldedValue, pErrorCode);
        utrie_compact(trie, pErrorCode);
        trie->isCompacted=TRUE;
        if(U_FAILURE(*pErrorCode)) {
            return 0;
        }
    }

    /* is dataLength within limits? */
    if( (reduceTo16Bits ? (trie->dataLength+trie->indexLength) : trie->dataLength) >= UTRIE_MAX_DATA_LENGTH) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    }

    length=sizeof(UTrieHeader)+2*trie->indexLength;
    if(reduceTo16Bits) {
        length+=2*trie->dataLength;
    } else {
        length+=4*trie->dataLength;
    }

    if(length>capacity) {
        return length; /* preflighting */
    }

    /* set the header fields */
    header=(UTrieHeader *)data;
    data+=sizeof(UTrieHeader);

    header->signature=0x54726965; /* "Trie" */
    header->options=UTRIE_SHIFT | (UTRIE_INDEX_SHIFT<<UTRIE_OPTIONS_INDEX_SHIFT);

    if(!reduceTo16Bits) {
        header->options|=UTRIE_OPTIONS_DATA_IS_32_BIT;
    }
    if(trie->isLatin1Linear) {
        header->options|=UTRIE_OPTIONS_LATIN1_IS_LINEAR;
    }

    header->indexLength=trie->indexLength;
    header->dataLength=trie->dataLength;

    /* write the index (stage 1) array and the 16/32-bit data (stage 2) array */
    if(reduceTo16Bits) {
        /* write 16-bit index values shifted right by UTRIE_INDEX_SHIFT, after adding indexLength */
        p=(uint32_t *)trie->index;
        dest16=(uint16_t *)data;
        for(i=trie->indexLength; i>0; --i) {
            *dest16++=(uint16_t)((*p++ + trie->indexLength)>>UTRIE_INDEX_SHIFT);
        }

        /* write 16-bit data values */
        p=trie->data;
        for(i=trie->dataLength; i>0; --i) {
            *dest16++=(uint16_t)*p++;
        }
    } else {
        /* write 16-bit index values shifted right by UTRIE_INDEX_SHIFT */
        p=(uint32_t *)trie->index;
        dest16=(uint16_t *)data;
        for(i=trie->indexLength; i>0; --i) {
            *dest16++=(uint16_t)(*p++ >> UTRIE_INDEX_SHIFT);
        }

        /* write 32-bit data values */
        uprv_memcpy(dest16, trie->data, 4*trie->dataLength);
    }

    return length;
}

U_CAPI int32_t U_EXPORT2
utrie_unserialize(UTrie *trie, const uint8_t *data, int32_t length, UErrorCode *pErrorCode) {
    UTrieHeader *header;
    uint16_t *p16;
    uint32_t options;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return -1;
    }

    /* enough data for a trie header? */
    if(length<sizeof(UTrieHeader)) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }

    /* check the signature */
    header=(UTrieHeader *)data;
    if(header->signature!=0x54726965) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }

    /* get the options and check the shift values */
    options=header->options;
    if( (options&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_SHIFT ||
        ((options>>UTRIE_OPTIONS_INDEX_SHIFT)&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_INDEX_SHIFT
    ) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }
    trie->isLatin1Linear= (UBool)((options&UTRIE_OPTIONS_LATIN1_IS_LINEAR)!=0);

    /* get the length values */
    trie->indexLength=header->indexLength;
    trie->dataLength=header->dataLength;

    length-=sizeof(UTrieHeader);

    /* enough data for the index? */
    if(length<2*trie->indexLength) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }
    p16=(uint16_t *)(header+1);
    trie->index=p16;
    p16+=trie->indexLength;
    length-=2*trie->indexLength;

    /* get the data */
    if(options&UTRIE_OPTIONS_DATA_IS_32_BIT) {
        if(length<4*trie->dataLength) {
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return -1;
        }
        trie->data32=(const uint32_t *)p16;
        trie->initialValue=trie->data32[0];
        return sizeof(UTrieHeader)+2*trie->indexLength+4*trie->dataLength;
    } else {
        if(length<2*trie->dataLength) {
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return -1;
        }

        /* the "data16" data is used via the index pointer */
        trie->data32=NULL;
        trie->initialValue=trie->index[trie->indexLength];
        return sizeof(UTrieHeader)+2*trie->indexLength+2*trie->dataLength;
    }
}

/* enumeration -------------------------------------------------------------- */

/* default UTrieEnumValue() returns the input value itself */
static uint32_t U_CALLCONV
enumSameValue(const void *context, uint32_t value) {
    return value;
}

/**
 * Enumerate all ranges of code points with the same relevant values.
 * The values are transformed from the raw trie entries by the enumValue function.
 */
U_CAPI void U_EXPORT2
utrie_enum(UTrie *trie,
           UTrieEnumValue *enumValue, UTrieEnumRange *enumRange, const void *context) {
    const uint32_t *data32;
    const uint16_t *index;

    uint32_t value, prevValue, initialValue;
    UChar32 c, prev;
    int32_t l, i, j, block, prevBlock, offset;

    /* check arguments */
    if(trie==NULL || trie->index==NULL || enumRange==NULL) {
        return;
    }
    if(enumValue==NULL) {
        enumValue=enumSameValue;
    }

    index=trie->index;
    data32=trie->data32;

    /* get the enumeration value that corresponds to an initial-value trie data entry */
    initialValue=enumValue(context, trie->initialValue);

    /* set variables for previous range */
    prevBlock=0;
    prev=0;
    prevValue=initialValue;

    /* enumerate BMP - the main loop enumerates data blocks */
    for(i=0, c=0; c<=0xffff; ++i) {
        if(c==0xd800) {
            /* skip lead surrogate code _units_, go to lead surr. code _points_ */
            i=UTRIE_BMP_INDEX_LENGTH;
        } else if(c==0xdc00) {
            /* go back to regular BMP code points */
            i=c>>UTRIE_SHIFT;
        }

        block=index[i]<<UTRIE_INDEX_SHIFT;
        if(block==prevBlock) {
            /* the block is the same as the previous one, and filled with value */
            c+=UTRIE_DATA_BLOCK_LENGTH;
        } else if(block==0) {
            /* this is the all-initial-value block */
            if(prevValue!=initialValue) {
                if(prev<c) {
                    if(!enumRange(context, prev, c, prevValue)) {
                        return;
                    }
                }
                prevBlock=0;
                prev=c;
                prevValue=initialValue;
            }
            c+=UTRIE_DATA_BLOCK_LENGTH;
        } else {
            prevBlock=block;
            for(j=0; j<UTRIE_DATA_BLOCK_LENGTH; ++j) {
                value=enumValue(context, data32!=NULL ? data32[block+j] : index[block+j]);
                if(value!=prevValue) {
                    if(prev<c) {
                        if(!enumRange(context, prev, c, prevValue)) {
                            return;
                        }
                    }
                    if(j>0) {
                        prevBlock=-1;
                    }
                    prev=c;
                    prevValue=value;
                }
                ++c;
            }
        }
    }

    /* enumerate supplementary code points */
    for(l=0xd800; l<0xdc00;) {
        /* lead surrogate access */
        offset=index[l>>UTRIE_SHIFT]<<UTRIE_INDEX_SHIFT;
        if(data32!=NULL) {
            if(offset==0) {
                /* no entries for a whole block of lead surrogates */
                l+=UTRIE_DATA_BLOCK_LENGTH;
                c+=UTRIE_DATA_BLOCK_LENGTH<<10;
                continue;
            }
            value=data32[offset+(l&UTRIE_MASK)];
        } else {
            if(offset==trie->indexLength) {
                /* no entries for a whole block of lead surrogates */
                l+=UTRIE_DATA_BLOCK_LENGTH;
                c+=UTRIE_DATA_BLOCK_LENGTH<<10;
                continue;
            }
            value=index[offset+(l&UTRIE_MASK)];
        }

        /* enumerate trail surrogates for this lead surrogate */
        offset=trie->getFoldingOffset(value);
        if(offset<=0) {
            /* no data for this lead surrogate */
            if(prevValue!=initialValue) {
                if(prev<c) {
                    if(!enumRange(context, prev, c, prevValue)) {
                        return;
                    }
                }
                prevBlock=0;
                prev=c;
                prevValue=initialValue;
            }

            /* nothing else to do for the supplementary code points for this lead surrogate */
            c+=0x400;
        } else {
            /* enumerate code points for this lead surrogate */
            i=offset;
            offset+=UTRIE_SURROGATE_BLOCK_COUNT;
            do {
                /* copy of most of the body of the BMP loop */
                block=index[i]<<UTRIE_INDEX_SHIFT;
                if(block==prevBlock) {
                    /* the block is the same as the previous one, and filled with value */
                    c+=UTRIE_DATA_BLOCK_LENGTH;
                } else if(block==0) {
                    /* this is the all-initial-value block */
                    if(prevValue!=initialValue) {
                        if(prev<c) {
                            if(!enumRange(context, prev, c, prevValue)) {
                                return;
                            }
                        }
                        prevBlock=0;
                        prev=c;
                        prevValue=initialValue;
                    }
                    c+=UTRIE_DATA_BLOCK_LENGTH;
                } else {
                    prevBlock=block;
                    for(j=0; j<UTRIE_DATA_BLOCK_LENGTH; ++j) {
                        value=enumValue(context, data32!=NULL ? data32[block+j] : index[block+j]);
                        if(value!=prevValue) {
                            if(prev<c) {
                                if(!enumRange(context, prev, c, prevValue)) {
                                    return;
                                }
                            }
                            if(j>0) {
                                prevBlock=-1;
                            }
                            prev=c;
                            prevValue=value;
                        }
                        ++c;
                    }
                }
            } while(++i<offset);
        }

        ++l;
    }

    /* deliver last range */
    enumRange(context, prev, c, prevValue);
}
