/*
******************************************************************************
*
*   Copyright (C) 1999-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
*
*  ucnv_io.c:
*  initializes global variables and defines functions pertaining to file
*  access, and name resolution aspect of the library.
*
*   new implementation:
*
*   created on: 1999nov22
*   created by: Markus W. Scherer
*
*   Use the binary cnvalias.icu (created from convrtrs.txt) to work
*   with aliases for converter names.
*
*   Date        Name        Description
*   11/22/1999  markus      Created
*   06/28/2002  grhoten     Major overhaul of the converter alias design.
*                           Now an alias can map to different converters
*                           depending on the specified standard.
*******************************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ucnv.h"           /* This file implements ucnv_xXXX() APIs */
#include "unicode/udata.h"

#include "umutex.h"
#include "cstring.h"
#include "cmemory.h"
#include "ucnv_io.h"
#include "uenumimp.h"
#include "ucln_cmn.h"

/* Format of cnvalias.icu -----------------------------------------------------
 *
 * cnvalias.icu is a binary, memory-mappable form of convrtrs.txt.
 * This binary form contains several tables. All indexes are to uint16_t
 * units, and not to the bytes (uint8_t units). Addressing everything on
 * 16-bit boundaries allows us to store more information with small index
 * numbers, which are also 16-bit in size. The majority of the table (except
 * the string table) are 16-bit numbers.
 *
 * First there is the size of the Table of Contents (TOC). The TOC
 * entries contain the size of each section. In order to find the offset
 * you just need to sum up the previous offsets.
 *
 * 1) This section contains a list of converters. This list contains indexes
 * into the string table for the converter name. The index of this list is
 * also used by other sections, which are mentioned later on.
 *
 * 2) This section contains a list of tags. This list contains indexes
 * into the string table for the tag name. The index of this list is
 * also used by other sections, which are mentioned later on.
 *
 * 3) This section contains a list of sorted unique aliases. This
 * list contains indexes into the string table for the alias name. The
 * index of this list is also used by other sections, like the 4th section.
 * The index for the 3rd and 4th section is used to get the
 * alias -> converter name mapping. Section 3 and 4 form a two column table.
 *
 * 4) This section contains a list of mapped converter names. Consider this
 * as a table that maps the 3rd section to the 1st section. This list contains
 * indexes into the 1st section. The index of this list is the same index in
 * the 3rd section. There is also some extra information in the high bits of
 * each converter index in this table. Currently it's only used to say that
 * an alias mapped to this converter is ambiguous. See UCNV_CONVERTER_INDEX_MASK
 * and UCNV_AMBIGUOUS_ALIAS_MAP_BIT for more information. This section is
 * the predigested form of the 5th section so that an alias lookup can be fast.
 * 
 * 5) This section contains a 2D array with indexes to the 6th section. This
 * section is the full form of all alias mappings. The column index is the
 * index into the converter list (column header). The row index is the index
 * to tag list (row header). This 2D array is the top part a 3D array. The
 * third dimension is in the 6th section.
 *
 * 6) This is blob of variable length arrays. Each array starts with a size,
 * and is followed by indexes to alias names in the string table. This is
 * the third dimension to the section 5. No other section should be referencing
 * this section.
 *
 * 7) Reserved at this time (There is no information). This _usually_ has a
 * size of 0. Future versions may add more information here.
 *
 * 8) This is the string table. All strings are indexed on an even address.
 * There are two reasons for this. First many chip architectures locate strings
 * faster on even address boundaries. Second, since all indexes are 16-bit
 * numbers, this string table can be 128KB in size instead of 64KB when we
 * only have strings starting on an even address.
 *
 *
 * Here is the concept of section 5 and 6. It's a 3D cube. Each tag
 * has a unique alias among all converters. That same alias can
 * be mentioned in other standards on different converters,
 * but only one alias per tag can be unique.
 *
 *
 *              Converter Names (Usually in TR22 form)
 *           -------------------------------------------.
 *     T    /                                          /|
 *     a   /                                          / |
 *     g  /                                          /  |
 *     s /                                          /   |
 *      /                                          /    |
 *      ------------------------------------------/     |
 *    A |                                         |     |
 *    l |                                         |     |
 *    i |                                         |    /
 *    a |                                         |   /
 *    s |                                         |  /
 *    e |                                         | /
 *    s |                                         |/
 *      -------------------------------------------
 *
 *
 *
 * Here is what it really looks like. It's like swiss cheese.
 * There are holes. Some converters aren't recognized by
 * a standard, or they are really old converters that the
 * standard doesn't recognize anymore.
 *
 *              Converter Names (Usually in TR22 form)
 *           -------------------------------------------.
 *     T    /##########################################/|
 *     a   /     #            #                       /#
 *     g  /  #      ##     ##     ### # ### ### ### #/  
 *     s / #             #####  ####        ##  ## #/#  
 *      / ### # # ##  #  #   #          ### # #   #/##  
 *      ------------------------------------------/# #
 *    A |### # # ##  #  #   #          ### # #   #|# #
 *    l |# # #    #     #               ## #     #|# #
 *    i |# # #    #     #                #       #|#
 *    a |#                                       #|#
 *    s |                                        #|#
 *    e 
 *    s 
 *      
 */

/**
 * Used by the UEnumeration API
 */
typedef struct UAliasContext {
    uint32_t listOffset;
    uint32_t listIdx;
} UAliasContext;

static const char DATA_NAME[] = "cnvalias";
static const char DATA_TYPE[] = "icu";

static UDataMemory *gAliasData=NULL;

static const uint16_t *gConverterList = NULL;
static const uint16_t *gTagList = NULL;
static const uint16_t *gAliasList = NULL;
static const uint16_t *gUntaggedConvArray = NULL;
static const uint16_t *gTaggedAliasArray = NULL;
static const uint16_t *gTaggedAliasLists = NULL;
static const uint16_t *gStringTable = NULL;

static uint32_t gConverterListSize;
static uint32_t gTagListSize;
static uint32_t gAliasListSize;
static uint32_t gUntaggedConvArraySize;
static uint32_t gTaggedAliasArraySize;
static uint32_t gTaggedAliasListsSize;
static uint32_t gStringTableSize;

static const char **gAvailableConverters = NULL;
static uint16_t gAvailableConverterCount = 0;

static char gDefaultConverterNameBuffer[UCNV_MAX_CONVERTER_NAME_LENGTH + 1]; /* +1 for NULL */
static const char *gDefaultConverterName = NULL;

#define GET_STRING(idx) (const char *)(gStringTable + (idx))

static UBool U_CALLCONV
isAcceptable(void *context,
             const char *type, const char *name,
             const UDataInfo *pInfo) {
    return (UBool)(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x43 &&   /* dataFormat="CvAl" */
        pInfo->dataFormat[1]==0x76 &&
        pInfo->dataFormat[2]==0x41 &&
        pInfo->dataFormat[3]==0x6c &&
        pInfo->formatVersion[0]==3);
}

static UBool
haveAliasData(UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return FALSE;
    }

    /* load converter alias data from file if necessary */
    if(gAliasData==NULL) {
        UDataMemory *data = NULL;
        const uint16_t *table = NULL;
        uint32_t tableStart;
        uint32_t currOffset;
        uint32_t reservedSize1;

        data = udata_openChoice(NULL, DATA_TYPE, DATA_NAME, isAcceptable, NULL, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return FALSE;
        }

        table = (const uint16_t *)udata_getMemory(data);

        tableStart      = ((const uint32_t *)(table))[0];
        if (tableStart < 8) {
            *pErrorCode = U_INVALID_FORMAT_ERROR;
            udata_close(data);
            return FALSE;
        }

        umtx_lock(NULL);
        if(gAliasData==NULL) {
            gAliasData = data;
            data=NULL;

            gConverterListSize      = ((const uint32_t *)(table))[1];
            gTagListSize            = ((const uint32_t *)(table))[2];
            gAliasListSize          = ((const uint32_t *)(table))[3];
            gUntaggedConvArraySize  = ((const uint32_t *)(table))[4];
            gTaggedAliasArraySize   = ((const uint32_t *)(table))[5];
            gTaggedAliasListsSize   = ((const uint32_t *)(table))[6];
            reservedSize1           = ((const uint32_t *)(table))[7];   /* reserved */
            gStringTableSize        = ((const uint32_t *)(table))[8];

            currOffset = tableStart * (sizeof(uint32_t)/sizeof(uint16_t)) + (sizeof(uint32_t)/sizeof(uint16_t));
            gConverterList = table + currOffset;

            currOffset += gConverterListSize;
            gTagList = table + currOffset;

            currOffset += gTagListSize;
            gAliasList = table + currOffset;

            currOffset += gAliasListSize;
            gUntaggedConvArray = table + currOffset;

            currOffset += gUntaggedConvArraySize;
            gTaggedAliasArray = table + currOffset;

            /* aliasLists is a 1's based array, but it has a padding character */
            currOffset += gTaggedAliasArraySize;
            gTaggedAliasLists = table + currOffset;

            currOffset += gTaggedAliasListsSize;
            /* reserved */

            currOffset += reservedSize1;
            gStringTable = table + currOffset;

        }
        umtx_unlock(NULL);

        /* if a different thread set it first, then close the extra data */
        if(data!=NULL) {
            udata_close(data); /* NULL if it was set correctly */
        }
    }

    return TRUE;
}

U_INLINE static UBool
isAlias(const char *alias, UErrorCode *pErrorCode) {
    if(alias==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    } else if(*alias==0) {
        return FALSE;
    } else {
        return TRUE;
    }
}

UBool 
ucnv_io_cleanup()
{
    if (gAliasData) {
        udata_close(gAliasData);
        gAliasData = NULL;
    }

    ucnv_io_flushAvailableConverterCache();

    gConverterListSize       = 0;
    gTagListSize             = 0;
    gAliasListSize           = 0;
    gUntaggedConvArraySize   = 0;
    gTaggedAliasArraySize    = 0;
    gTaggedAliasListsSize    = 0;
    gStringTableSize         = 0;

    gConverterList = NULL;
    gTagList = NULL;
    gAliasList = NULL;
    gUntaggedConvArray = NULL;
    gTaggedAliasArray = NULL;
    gTaggedAliasLists = NULL;
    gStringTable = NULL;

    gDefaultConverterName = NULL;
    gDefaultConverterNameBuffer[0] = 0;

    return TRUE;                   /* Everything was cleaned up */
}


static uint32_t getTagNumber(const char *tagname) {
    if (gTagList) {
        uint32_t tagNum;
        for (tagNum = 0; tagNum < gTagListSize; tagNum++) {
            if (!uprv_stricmp(GET_STRING(gTagList[tagNum]), tagname)) {
                return tagNum;
            }
        }
    }

    return UINT32_MAX;
}

/* @see ucnv_compareNames */
U_CFUNC char * U_EXPORT2
ucnv_io_stripForCompare(char *dst, const char *name) {
    char c1 = *name;
    char *dstItr = dst;

    while (c1) {
        /* Ignore delimiters '-', '_', and ' ' */
        while ((c1 = *name) == '-' || c1 == '_' || c1 == ' ') {
            ++name;
        }

        /* lowercase for case-insensitive comparison */
        *(dstItr++) = uprv_tolower(c1);
        ++name;
    }
    return dst;
}

/**
 * Do a fuzzy compare of a two converter/alias names.  The comparison
 * is case-insensitive.  It also ignores the characters '-', '_', and
 * ' ' (dash, underscore, and space).  Thus the strings "UTF-8",
 * "utf_8", and "Utf 8" are exactly equivalent.
 * 
 * This is a symmetrical (commutative) operation; order of arguments
 * is insignificant.  This is an important property for sorting the
 * list (when the list is preprocessed into binary form) and for
 * performing binary searches on it at run time.
 * 
 * @param name1 a converter name or alias, zero-terminated
 * @param name2 a converter name or alias, zero-terminated
 * @return 0 if the names match, or a negative value if the name1
 * lexically precedes name2, or a positive value if the name1
 * lexically follows name2.
 *
 * @see ucnv_io_stripForCompare
 */
U_CAPI int U_EXPORT2
ucnv_compareNames(const char *name1, const char *name2) {
    int rc;
    char c1, c2;

    for (;;) {
        /* Ignore delimiters '-', '_', and ' ' */
        while ((c1 = *name1) == '-' || c1 == '_' || c1 == ' ') {
            ++name1;
        }
        while ((c2 = *name2) == '-' || c2 == '_' || c2 == ' ') {
            ++name2;
        }

        /* If we reach the ends of both strings then they match */
        if ((c1|c2)==0) {
            return 0;
        }
        
        /* Case-insensitive comparison */
        rc = (int)(unsigned char)uprv_tolower(c1) -
             (int)(unsigned char)uprv_tolower(c2);
        if (rc != 0) {
            return rc;
        }
        ++name1;
        ++name2;
    }
}

/*
 * search for an alias
 * return the converter number index for gConverterList
 */
static uint32_t
findConverter(const char *alias, UErrorCode *pErrorCode) {
    uint32_t mid, start, limit;
	uint32_t lastMid;
    int result;

    /* do a binary search for the alias */
    start = 0;
    limit = gUntaggedConvArraySize;
    mid = limit;
	lastMid = UINT32_MAX;

    for (;;) {
        mid = (uint32_t)((start + limit) / 2);
		if (lastMid == mid) {	/* Have we moved? */
			break;	/* We haven't moved, and it wasn't found. */
		}
		lastMid = mid;
        result = ucnv_compareNames(alias, GET_STRING(gAliasList[mid]));

        if (result < 0) {
            limit = mid;
        } else if (result > 0) {
            start = mid;
        } else {
            /* Since the gencnval tool folds duplicates into one entry,
             * this alias in gAliasList is unique, but different standards
             * may map an alias to different converters.
             */
            if (gUntaggedConvArray[mid] & UCNV_AMBIGUOUS_ALIAS_MAP_BIT) {
                *pErrorCode = U_AMBIGUOUS_ALIAS_WARNING;
            }
            return gUntaggedConvArray[mid] & UCNV_CONVERTER_INDEX_MASK;
        }
    }

    return UINT32_MAX;
}

/*
 * Search for an standard name of an alias (what is the default name
 * that this standard uses?)
 * return the listOffset for gTaggedAliasLists. If it's 0,
 * the it couldn't be found, but the parameters are valid.
 */
static uint32_t
findTaggedAliasListsOffset(const char *alias, const char *standard, UErrorCode *pErrorCode) {
    uint32_t idx;
    uint32_t listOffset;
    uint32_t convNum;
    UErrorCode myErr = U_ZERO_ERROR;
    uint32_t tagNum = getTagNumber(standard);

    /* Make a quick guess. Hopefully they used a TR22 canonical alias. */
    convNum = findConverter(alias, &myErr);
    if (myErr != U_ZERO_ERROR) {
        *pErrorCode = myErr;
    }

    if (tagNum < (gTagListSize - UCNV_NUM_RESERVED_TAGS) && convNum < gConverterListSize) {
        listOffset = gTaggedAliasArray[tagNum*gConverterListSize + convNum];
        if (listOffset && gTaggedAliasLists[listOffset + 1]) {
            return listOffset;
        }
        if (myErr == U_AMBIGUOUS_ALIAS_WARNING) {
            /* Uh Oh! They used an ambiguous alias.
               We have to search the whole swiss cheese starting
               at the highest standard affinity.
               This may take a while.
            */
            for (idx = 0; idx < gTaggedAliasArraySize; idx++) {
                listOffset = gTaggedAliasArray[idx];
                if (listOffset) {
                    uint32_t currAlias;
                    uint32_t listCount = gTaggedAliasLists[listOffset];
                    /* +1 to skip listCount */
                    const uint16_t *currList = gTaggedAliasLists + listOffset + 1;
                    for (currAlias = 0; currAlias < listCount; currAlias++) {
                        if (currList[currAlias]
                            && ucnv_compareNames(alias, GET_STRING(currList[currAlias]))==0)
                        {
                            /*return listOffset;*/
                            uint32_t currTagNum = idx/gConverterListSize;
                            uint32_t currConvNum = (idx - currTagNum*gConverterListSize);
                            uint32_t tempListOffset = gTaggedAliasArray[tagNum*gConverterListSize + currConvNum];
                            if (tempListOffset && gTaggedAliasLists[tempListOffset + 1]) {
                                return tempListOffset;
                            }
                            /* else keep on looking */
                            /* We could speed this up by starting on the next row
                               because an alias is unique per row, right now.
                               This would change if alias versioning appears. */
                        }
                    }
                }
            }
            /* The standard doesn't know about the alias */
        }
        /* else no default name */
        return 0;
    }
    /* else converter or tag not found */

    return UINT32_MAX;
}

U_CFUNC const char *
ucnv_io_getConverterName(const char *alias, UErrorCode *pErrorCode) {
    if(haveAliasData(pErrorCode) && isAlias(alias, pErrorCode)) {
        uint32_t convNum = findConverter(alias, pErrorCode);
        if (convNum < gConverterListSize) {
            return GET_STRING(gConverterList[convNum]);
        }
        /* else converter not found */
    }
    return NULL;
}

static int32_t U_CALLCONV
ucnv_io_countStandardAliases(UEnumeration *enumerator, UErrorCode *pErrorCode) {
    int32_t value = 0;
    UAliasContext *myContext = (UAliasContext *)(enumerator->context);
    uint32_t listOffset = myContext->listOffset;

    if (listOffset) {
        value = gTaggedAliasLists[listOffset];
    }
    return value;
}

static const char* U_CALLCONV
ucnv_io_nextStandardAliases(UEnumeration *enumerator,
                            int32_t* resultLength,
                            UErrorCode *pErrorCode)
{
    UAliasContext *myContext = (UAliasContext *)(enumerator->context);
    uint32_t listOffset = myContext->listOffset;

    if (listOffset) {
        uint32_t listCount = gTaggedAliasLists[listOffset];
        const uint16_t *currList = gTaggedAliasLists + listOffset + 1;

        if (myContext->listIdx < listCount) {
            const char *myStr = GET_STRING(currList[myContext->listIdx++]);
            if (resultLength) {
                *resultLength = uprv_strlen(myStr);
            }
            return myStr;
        }
    }
    /* Either we accessed a zero length list, or we enumerated too far. */
    *pErrorCode = U_INDEX_OUTOFBOUNDS_ERROR;
    return NULL;
}

static void U_CALLCONV
ucnv_io_resetStandardAliases(UEnumeration *enumerator, UErrorCode *pErrorCode) {
    ((UAliasContext *)(enumerator->context))->listIdx = 0;
}

static void U_CALLCONV
ucnv_io_closeUEnumeration(UEnumeration *enumerator) {
    uprv_free(enumerator->context);
    uprv_free(enumerator);
}

/* Enumerate the aliases for the specified converter and standard tag */
static const UEnumeration gEnumAliases = {
    NULL,
    NULL,
    ucnv_io_closeUEnumeration,
    ucnv_io_countStandardAliases,
    uenum_unextDefault,
    ucnv_io_nextStandardAliases,
    ucnv_io_resetStandardAliases
};

U_CAPI UEnumeration *
ucnv_openStandardNames(const char *convName,
                            const char *standard,
                            UErrorCode *pErrorCode)
{
    UEnumeration *myEnum = NULL;
    if (haveAliasData(pErrorCode) && isAlias(convName, pErrorCode)) {
        uint32_t listOffset = findTaggedAliasListsOffset(convName, standard, pErrorCode);

        /* When listOffset == 0, we want to acknowledge that the
           converter name and standard are okay, but there
           is nothing to enumerate. */
        if (listOffset < gTaggedAliasListsSize) {
            UAliasContext *myContext;

            myEnum = uprv_malloc(sizeof(UEnumeration));
            if (myEnum == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                return NULL;
            }
            uprv_memcpy(myEnum, &gEnumAliases, sizeof(UEnumeration));
            myContext = uprv_malloc(sizeof(UAliasContext));
            if (myContext == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                uprv_free(myEnum);
                return NULL;
            }
            myEnum->context = myContext;
            myContext->listOffset = listOffset;
            myContext->listIdx = 0;
        }
        /* else converter or tag not found */
    }
    return myEnum;
}

U_CFUNC uint16_t
ucnv_io_countAliases(const char *alias, UErrorCode *pErrorCode) {
    if(haveAliasData(pErrorCode) && isAlias(alias, pErrorCode)) {
        uint32_t convNum = findConverter(alias, pErrorCode);
        if (convNum < gConverterListSize) {
            /* tagListNum - 1 is the ALL tag */
            int32_t listOffset = gTaggedAliasArray[(gTagListSize - 1)*gConverterListSize + convNum];

            if (listOffset) {
                return gTaggedAliasLists[listOffset];
            }
            /* else this shouldn't happen. internal program error */
        }
        /* else converter not found */
    }
    return 0;
}

U_CFUNC uint16_t
ucnv_io_getAliases(const char *alias, uint16_t start, const char **aliases, UErrorCode *pErrorCode) {
    if(haveAliasData(pErrorCode) && isAlias(alias, pErrorCode)) {
        uint32_t currAlias;
        uint32_t convNum = findConverter(alias, pErrorCode);
        if (convNum < gConverterListSize) {
            /* tagListNum - 1 is the ALL tag */
            int32_t listOffset = gTaggedAliasArray[(gTagListSize - 1)*gConverterListSize + convNum];

            if (listOffset) {
                uint32_t listCount = gTaggedAliasLists[listOffset];
                /* +1 to skip listCount */
                const uint16_t *currList = gTaggedAliasLists + listOffset + 1;

                for (currAlias = start; currAlias < listCount; currAlias++) {
                    aliases[currAlias] = GET_STRING(currList[currAlias]);
                }
            }
            /* else this shouldn't happen. internal program error */
        }
        /* else converter not found */
    }
    return 0;
}

U_CFUNC const char *
ucnv_io_getAlias(const char *alias, uint16_t n, UErrorCode *pErrorCode) {
    if(haveAliasData(pErrorCode) && isAlias(alias, pErrorCode)) {
        uint32_t convNum = findConverter(alias, pErrorCode);
        if (convNum < gConverterListSize) {
            /* tagListNum - 1 is the ALL tag */
            int32_t listOffset = gTaggedAliasArray[(gTagListSize - 1)*gConverterListSize + convNum];

            if (listOffset) {
                uint32_t listCount = gTaggedAliasLists[listOffset];
                /* +1 to skip listCount */
                const uint16_t *currList = gTaggedAliasLists + listOffset + 1;

                if (n < listCount)  {
                    return GET_STRING(currList[n]);
                }
                *pErrorCode = U_INDEX_OUTOFBOUNDS_ERROR;
            }
            /* else this shouldn't happen. internal program error */
        }
        /* else converter not found */
    }
    return NULL;
}

U_CFUNC uint16_t
ucnv_io_countStandards(UErrorCode *pErrorCode) {
    if (haveAliasData(pErrorCode)) {
        /* Don't include the empty list */
        return (uint16_t)(gTagListSize - UCNV_NUM_RESERVED_TAGS);
    }

    return 0;
}

U_CAPI const char * U_EXPORT2
ucnv_getStandard(uint16_t n, UErrorCode *pErrorCode) {
    if (haveAliasData(pErrorCode)) {
        if (n < gTagListSize - UCNV_NUM_RESERVED_TAGS) {
            return GET_STRING(gTagList[n]);
        }
        *pErrorCode = U_INDEX_OUTOFBOUNDS_ERROR;
    }

    return NULL;
}

U_CAPI const char * U_EXPORT2
ucnv_getStandardName(const char *alias, const char *standard, UErrorCode *pErrorCode) {
    if (haveAliasData(pErrorCode) && isAlias(alias, pErrorCode)) {
        uint32_t listOffset = findTaggedAliasListsOffset(alias, standard, pErrorCode);

        if (0 < listOffset && listOffset < gTaggedAliasListsSize) {
            const uint16_t *currList = gTaggedAliasLists + listOffset + 1;

            /* Get the preferred name from this list */
            if (currList[0]) {
                return GET_STRING(currList[0]);
            }
            /* else someone screwed up the alias table. */
            /* *pErrorCode = U_INVALID_FORMAT_ERROR */
        }
    }

    return NULL;
}

void
ucnv_io_flushAvailableConverterCache() {
    if (gAvailableConverters) {
        umtx_lock(NULL);
        gAvailableConverterCount = 0;
        uprv_free((char **)gAvailableConverters);
        gAvailableConverters = NULL;
        umtx_unlock(NULL);
    }
}

static UBool haveAvailableConverterList(UErrorCode *pErrorCode) {
    if (gAvailableConverters == NULL) {
        uint16_t idx;
        uint16_t localConverterCount;
        UErrorCode status;
        const char *converterName;
        const char **localConverterList;

        if (!haveAliasData(pErrorCode)) {
            return FALSE;
        }

        /* We can't have more than "*converterTable" converters to open */
        localConverterList = (const char **) uprv_malloc(gConverterListSize * sizeof(char*));
        if (!localConverterList) {
            *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }

        localConverterCount = 0;

        for (idx = 0; idx < gConverterListSize; idx++) {
            status = U_ZERO_ERROR;
            converterName = GET_STRING(gConverterList[idx]);
            ucnv_close(ucnv_open(converterName, &status));
            if (U_SUCCESS(status)) {
                localConverterList[localConverterCount++] = converterName;
            }
        }

        umtx_lock(NULL);
        if (gAvailableConverters == NULL) {
            gAvailableConverters = localConverterList;
            gAvailableConverterCount = localConverterCount;
        }
        else {
            uprv_free((char **)localConverterList);
        }
        umtx_unlock(NULL);
    }
    return TRUE;
}

U_CFUNC uint16_t
ucnv_io_countAvailableConverters(UErrorCode *pErrorCode) {
    if (haveAvailableConverterList(pErrorCode)) {
        return gAvailableConverterCount;
    }
    return 0;
}

U_CFUNC const char *
ucnv_io_getAvailableConverter(uint16_t n, UErrorCode *pErrorCode) {
    if (haveAvailableConverterList(pErrorCode)) {
        if (n < gAvailableConverterCount) {
            return gAvailableConverters[n];
        }
        *pErrorCode = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    return NULL;
}

U_CFUNC void
ucnv_io_fillAvailableConverters(const char **aliases, UErrorCode *pErrorCode) {
    if (haveAvailableConverterList(pErrorCode)) {
        uint16_t count = 0;
        while (count < gAvailableConverterCount) {
            *aliases++=gAvailableConverters[count++];
        }
    }
}

U_CFUNC uint16_t
ucnv_io_countAvailableAliases(UErrorCode *pErrorCode) {
    if (haveAliasData(pErrorCode)) {
        return (uint16_t)gAliasListSize;
    }
    return 0;
}

/* default converter name --------------------------------------------------- */

/*
 * In order to be really thread-safe, the get function would have to take
 * a buffer parameter and copy the current string inside a mutex block.
 * This implementation only tries to be really thread-safe while
 * setting the name.
 * It assumes that setting a pointer is atomic.
 */

U_CFUNC const char *
ucnv_io_getDefaultConverterName() {
    /* local variable to be thread-safe */
    const char *name=gDefaultConverterName;
    if(name==NULL) {
        const char *codepage = uprv_getDefaultCodepage();
        if(codepage!=NULL) {
            UErrorCode errorCode=U_ZERO_ERROR;
            name=ucnv_io_getConverterName(codepage, &errorCode);
            if(U_FAILURE(errorCode) || name==NULL) {
                name=codepage;
            }
        }

        /* if the name is there, test it out */
        if(name != NULL) {
            UErrorCode errorCode = U_ZERO_ERROR;
            UConverter *cnv = ucnv_open(name, &errorCode);
            if(U_FAILURE(errorCode) || (cnv == NULL)) {
                /* Panic time, let's use a fallback. */
#if (U_CHARSET_FAMILY == U_ASCII_FAMILY) 
                name = "US-ASCII";
                /* there is no 'algorithmic' converter for EBCDIC */
#elif defined(OS390)
                name = "ibm-1047" UCNV_SWAP_LFNL_OPTION_STRING;
#else
                name = "ibm-37";
#endif
            }
            ucnv_close(cnv);
        }

        if(name != NULL) {
            umtx_lock(NULL);
            /* Did find a name. And it works.*/
            gDefaultConverterName=name;
            umtx_unlock(NULL);
        }
    }

    return name;
}

U_CFUNC void
ucnv_io_setDefaultConverterName(const char *converterName) {
    if(converterName==NULL) {
        /* reset to the default codepage */
        gDefaultConverterName=NULL;
    } else {
        UErrorCode errorCode=U_ZERO_ERROR;
        const char *name=ucnv_io_getConverterName(converterName, &errorCode);
        if(U_SUCCESS(errorCode) && name!=NULL) {
            gDefaultConverterName=name;
        } else {
            /* do not set the name if the alias lookup failed and it is too long */
            int32_t length=(int32_t)(uprv_strlen(converterName));
            if(length<sizeof(gDefaultConverterNameBuffer)) {
                /* it was not found as an alias, so copy it - accept an empty name */
                UBool didLock;
                if(gDefaultConverterName==gDefaultConverterNameBuffer) {
                    umtx_lock(NULL);
                    didLock=TRUE;
                } else {
                    didLock=FALSE;
                }
                uprv_memcpy(gDefaultConverterNameBuffer, converterName, length);
                gDefaultConverterNameBuffer[length]=0;
                gDefaultConverterName=gDefaultConverterNameBuffer;
                if(didLock) {
                    umtx_unlock(NULL);
                }
            }
        }
    }
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */

