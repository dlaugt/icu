/*
*******************************************************************************
*
*   Copyright (C) 2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  swapimpl.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2005may05
*   created by: Markus W. Scherer
*
*   Data file swapping functions moved here from the common library
*   because some data is hardcoded in ICU4C and needs not be swapped any more.
*   Moving the functions here simplifies testing (for code coverage) because
*   we need not jump through hoops (like adding snapshots of these files
*   to testdata).
*
*   The declarations for these functions remain in the internal header files
*   in icu/source/common/
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/udata.h"
#include "cmemory.h"
#include "cstring.h"
#include "uinvchar.h"
#include "uassert.h"
#include "uarrsort.h"
#include "ucmndata.h"
#include "udataswp.h"

#include "uprops.h"
#include "ucase.h"
#include "ubidi_props.h"
#include "unormimp.h"

/* Unicode properties data swapping ----------------------------------------- */

U_CAPI int32_t U_EXPORT2
uprops_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize, i;

    int32_t dataIndexes[UPROPS_INDEX_COUNT];
    const int32_t *inData32;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x55 &&   /* dataFormat="UPro" */
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6f &&
        (pInfo->formatVersion[0]==3 || pInfo->formatVersion[0]==4) &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    )) {
        udata_printError(ds, "uprops_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not a Unicode properties file\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    /* the properties file must contain at least the indexes array */
    if(length>=0 && (length-headerSize)<(int32_t)sizeof(dataIndexes)) {
        udata_printError(ds, "uprops_swap(): too few bytes (%d after header) for a Unicode properties file\n",
                         length-headerSize);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    /* read the indexes */
    inData32=(const int32_t *)((const char *)inData+headerSize);
    for(i=0; i<UPROPS_INDEX_COUNT; ++i) {
        dataIndexes[i]=udata_readInt32(ds, inData32[i]);
    }

    /*
     * comments are copied from the data format description in genprops/store.c
     * indexes[] constants are in uprops.h
     */
    if(length>=0) {
        int32_t *outData32;

        if((length-headerSize)<(4*dataIndexes[UPROPS_RESERVED_INDEX])) {
            udata_printError(ds, "uprops_swap(): too few bytes (%d after header) for a Unicode properties file\n",
                             length-headerSize);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outData32=(int32_t *)((char *)outData+headerSize);

        /* copy everything for inaccessible data (padding) */
        if(inData32!=outData32) {
            uprv_memcpy(outData32, inData32, 4*dataIndexes[UPROPS_RESERVED_INDEX]);
        }

        /* swap the indexes[16] */
        ds->swapArray32(ds, inData32, 4*UPROPS_INDEX_COUNT, outData32, pErrorCode);

        /*
         * swap the main properties UTrie
         * PT serialized properties trie, see utrie.h (byte size: 4*(i0-16))
         */
        utrie_swap(ds,
            inData32+UPROPS_INDEX_COUNT,
            4*(dataIndexes[UPROPS_PROPS32_INDEX]-UPROPS_INDEX_COUNT),
            outData32+UPROPS_INDEX_COUNT,
            pErrorCode);

        /*
         * swap the properties and exceptions words
         * P  const uint32_t props32[i1-i0];
         * E  const uint32_t exceptions[i2-i1];
         */
        ds->swapArray32(ds,
            inData32+dataIndexes[UPROPS_PROPS32_INDEX],
            4*(dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX]-dataIndexes[UPROPS_PROPS32_INDEX]),
            outData32+dataIndexes[UPROPS_PROPS32_INDEX],
            pErrorCode);

        /*
         * swap the UChars
         * U  const UChar uchars[2*(i3-i2)];
         */
        ds->swapArray16(ds,
            inData32+dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX],
            4*(dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX]-dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX]),
            outData32+dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX],
            pErrorCode);

        /*
         * swap the additional UTrie
         * i3 additionalTrieIndex; -- 32-bit unit index to the additional trie for more properties
         */
        utrie_swap(ds,
            inData32+dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX],
            4*(dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX]-dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX]),
            outData32+dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX],
            pErrorCode);

        /*
         * swap the properties vectors
         * PV const uint32_t propsVectors[(i6-i4)/i5][i5]==uint32_t propsVectors[i6-i4];
         */
        ds->swapArray32(ds,
            inData32+dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX],
            4*(dataIndexes[UPROPS_RESERVED_INDEX]-dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX]),
            outData32+dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX],
            pErrorCode);
    }

    /* i6 reservedItemIndex; -- 32-bit unit index to the top of the properties vectors table */
    return headerSize+4*dataIndexes[UPROPS_RESERVED_INDEX];
}

/* Unicode case mapping data swapping --------------------------------------- */

U_CAPI int32_t U_EXPORT2
ucase_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[16];

    int32_t i, offset, count, size;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==UCASE_FMT_0 &&    /* dataFormat="cAsE" */
        pInfo->dataFormat[1]==UCASE_FMT_1 &&
        pInfo->dataFormat[2]==UCASE_FMT_2 &&
        pInfo->dataFormat[3]==UCASE_FMT_3 &&
        pInfo->formatVersion[0]==1 &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    )) {
        udata_printError(ds, "ucase_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as case mapping data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<16*4) {
            udata_printError(ds, "ucase_swap(): too few bytes (%d after header) for case mapping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    /* read the first 16 indexes (ICU 3.2/format version 1: UCASE_IX_TOP==16, might grow) */
    for(i=0; i<16; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    /* get the total length of the data */
    size=indexes[UCASE_IX_LENGTH];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "ucase_swap(): too few bytes (%d after header) for all of case mapping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        /* copy the data for inaccessible bytes */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        /* swap the int32_t indexes[] */
        count=indexes[UCASE_IX_INDEX_TOP]*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        /* swap the UTrie */
        count=indexes[UCASE_IX_TRIE_SIZE];
        utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the uint16_t exceptions[] and unfold[] */
        count=(indexes[UCASE_IX_EXC_LENGTH]+indexes[UCASE_IX_UNFOLD_LENGTH])*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        U_ASSERT(offset==size);
    }

    return headerSize+size;
}

/* Unicode bidi/shaping data swapping --------------------------------------- */

U_CAPI int32_t U_EXPORT2
ubidi_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[16];

    int32_t i, offset, count, size;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==UBIDI_FMT_0 &&    /* dataFormat="BiDi" */
        pInfo->dataFormat[1]==UBIDI_FMT_1 &&
        pInfo->dataFormat[2]==UBIDI_FMT_2 &&
        pInfo->dataFormat[3]==UBIDI_FMT_3 &&
        pInfo->formatVersion[0]==1 &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    )) {
        udata_printError(ds, "ubidi_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as bidi/shaping data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<16*4) {
            udata_printError(ds, "ubidi_swap(): too few bytes (%d after header) for bidi/shaping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    /* read the first 16 indexes (ICU 3.4/format version 1: UBIDI_IX_TOP==16, might grow) */
    for(i=0; i<16; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    /* get the total length of the data */
    size=indexes[UBIDI_IX_LENGTH];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "ubidi_swap(): too few bytes (%d after header) for all of bidi/shaping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        /* copy the data for inaccessible bytes */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        /* swap the int32_t indexes[] */
        count=indexes[UBIDI_IX_INDEX_TOP]*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        /* swap the UTrie */
        count=indexes[UBIDI_IX_TRIE_SIZE];
        utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the uint32_t mirrors[] */
        count=indexes[UBIDI_IX_MIRROR_LENGTH]*4;
        ds->swapArray32(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* just skip the uint8_t jgArray[] */
        count=indexes[UBIDI_IX_JG_LIMIT]-indexes[UBIDI_IX_JG_START];
        offset+=count;

        U_ASSERT(offset==size);
    }

    return headerSize+size;
}

/* Unicode normalization data swapping -------------------------------------- */

#if !UCONFIG_NO_NORMALIZATION

U_CAPI int32_t U_EXPORT2
unorm_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[32];

    int32_t i, offset, count, size;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x4e &&   /* dataFormat="Norm" */
        pInfo->dataFormat[1]==0x6f &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6d &&
        pInfo->formatVersion[0]==2
    )) {
        udata_printError(ds, "unorm_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as unorm.icu\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<32*4) {
            udata_printError(ds, "unorm_swap(): too few bytes (%d after header) for unorm.icu\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    /* read the first 32 indexes (ICU 2.8/format version 2.2: _NORM_INDEX_TOP==32, might grow) */
    for(i=0; i<32; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    /* calculate the total length of the data */
    size=
        32*4+ /* size of indexes[] */
        indexes[_NORM_INDEX_TRIE_SIZE]+
        indexes[_NORM_INDEX_UCHAR_COUNT]*2+
        indexes[_NORM_INDEX_COMBINE_DATA_COUNT]*2+
        indexes[_NORM_INDEX_FCD_TRIE_SIZE]+
        indexes[_NORM_INDEX_AUX_TRIE_SIZE]+
        indexes[_NORM_INDEX_CANON_SET_COUNT]*2;

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "unorm_swap(): too few bytes (%d after header) for all of unorm.icu\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        /* copy the data for inaccessible bytes */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        /* swap the indexes[] */
        count=32*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        /* swap the main UTrie */
        count=indexes[_NORM_INDEX_TRIE_SIZE];
        utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the uint16_t extraData[] and the uint16_t combiningTable[] */
        count=(indexes[_NORM_INDEX_UCHAR_COUNT]+indexes[_NORM_INDEX_COMBINE_DATA_COUNT])*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the FCD UTrie */
        count=indexes[_NORM_INDEX_FCD_TRIE_SIZE];
        if(count!=0) {
            utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
            offset+=count;
        }

        /* swap the aux UTrie */
        count=indexes[_NORM_INDEX_AUX_TRIE_SIZE];
        if(count!=0) {
            utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
            offset+=count;
        }

        /* swap the uint16_t combiningTable[] */
        count=indexes[_NORM_INDEX_CANON_SET_COUNT]*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;
    }

    return headerSize+size;
}

#endif
