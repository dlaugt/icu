/*
********************************************************************************
*   Copyright (C) 1996-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File UCHAR.C
*
* Modification History:
*
*   Date        Name        Description
*   04/02/97    aliu        Creation.
*   4/15/99     Madhu       Updated all the function definitions for C Implementation
*   5/20/99     Madhu       Added the function u_getVersion()
*   8/19/1999   srl         Upgraded scripts to Unicode3.0 
*   11/11/1999  weiv        added u_isalnum(), cleaned comments
*   01/11/2000  helena      Renamed u_getVersion to u_getUnicodeVersion.
*   06/20/2000  helena      OS/400 port changes; mostly typecast.
******************************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/udata.h"
#include "unicode/uloc.h"
#include "unicode/uiter.h"
#include "umutex.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "utrie.h"
#include "ustr_imp.h"
#include "uprops.h"

/* dynamically loaded Unicode character properties -------------------------- */

/*
 * loaded uprops.dat -
 * for a description of the file format, see icu/source/tools/genprops/store.c
 */
static const char DATA_NAME[] = "uprops";
static const char DATA_TYPE[] = "icu";

static UDataMemory *propsData=NULL;

static uint8_t formatVersion[4]={ 0, 0, 0, 0 };
static UVersionInfo dataVersion={ 3, 0, 0, 0 };

static UTrie propsTrie={ 0 }, propsVectorsTrie={ 0 };
static const uint32_t *pData32=NULL, *props32Table=NULL, *exceptionsTable=NULL, *propsVectors=NULL;
static const UChar *ucharsTable=NULL;
static int32_t countPropsVectors=0, propsVectorsColumns=0;

static int8_t havePropsData=0;

/* index values loaded from uprops.dat */
static int32_t indexes[UPROPS_INDEX_COUNT];

/* if bit 15 is set, then the folding offset is in bits 14..0 of the 16-bit trie result */
static int32_t U_CALLCONV
getFoldingPropsOffset(uint32_t data) {
    if(data&0x8000) {
        return (int32_t)(data&0x7fff);
    } else {
        return 0;
    }
}

static UBool U_CALLCONV
isAcceptable(void *context,
             const char *type, const char *name,
             const UDataInfo *pInfo) {
    if(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x55 &&   /* dataFormat="UPro" */
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6f &&
        pInfo->formatVersion[0]==3 &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    ) {
        uprv_memcpy(formatVersion, pInfo->formatVersion, 4);
        uprv_memcpy(dataVersion, pInfo->dataVersion, 4);
        return TRUE;
    } else {
        return FALSE;
    }
}

UBool
uchar_cleanup()
{
    if (propsData) {
        udata_close(propsData);
        propsData=NULL;
    }
    pData32=NULL;
    props32Table=NULL;
    exceptionsTable=NULL;
    ucharsTable=NULL;
    propsVectors=NULL;
    countPropsVectors=0;
    havePropsData=FALSE;
    return TRUE;
}

static int8_t
loadPropsData(void) {
    /* load Unicode character properties data from file if necessary */
    if(havePropsData==0) {
        UTrie trie={ 0 }, trie2={ 0 };
        UErrorCode errorCode=U_ZERO_ERROR;
        UDataMemory *data;
        const uint32_t *p=NULL;
        int32_t length;

        /* open the data outside the mutex block */
        data=udata_openChoice(NULL, DATA_TYPE, DATA_NAME, isAcceptable, NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            return havePropsData=-1;
        }

        p=(const uint32_t *)udata_getMemory(data);

        /* unserialize the trie; it is directly after the int32_t indexes[UPROPS_INDEX_COUNT] */
        length=(int32_t)p[UPROPS_PROPS32_INDEX]*4;
        length=utrie_unserialize(&trie, (const uint8_t *)(p+UPROPS_INDEX_COUNT), length-64, &errorCode);
        if(U_FAILURE(errorCode)) {
            udata_close(data);
            return havePropsData=-1;
        }
        trie.getFoldingOffset=getFoldingPropsOffset;

        /* unserialize the properties vectors trie, if any */
        if( p[UPROPS_ADDITIONAL_TRIE_INDEX]!=0 &&
            p[UPROPS_ADDITIONAL_VECTORS_INDEX]!=0
        ) {
            length=(int32_t)(p[UPROPS_ADDITIONAL_VECTORS_INDEX]-p[UPROPS_ADDITIONAL_TRIE_INDEX])*4;
            length=utrie_unserialize(&trie2, (const uint8_t *)(p+p[UPROPS_ADDITIONAL_TRIE_INDEX]), length, &errorCode);
            if(U_FAILURE(errorCode)) {
                uprv_memset(&trie2, 0, sizeof(trie2));
            } else {
                trie2.getFoldingOffset=getFoldingPropsOffset;
            }
        }

        /* in the mutex block, set the data for this process */
        umtx_lock(NULL);
        if(propsData==NULL) {
            propsData=data;
            data=NULL;
            pData32=p;
            p=NULL;
            uprv_memcpy(&propsTrie, &trie, sizeof(trie));
            uprv_memcpy(&propsVectorsTrie, &trie2, sizeof(trie2));
        }
        umtx_unlock(NULL);

        /* initialize some variables */
        uprv_memcpy(indexes, pData32, sizeof(indexes));
        props32Table=pData32+indexes[UPROPS_PROPS32_INDEX];
        exceptionsTable=pData32+indexes[UPROPS_EXCEPTIONS_INDEX];
        ucharsTable=(const UChar *)(pData32+indexes[UPROPS_EXCEPTIONS_TOP_INDEX]);

        /* additional properties */
        if(indexes[UPROPS_ADDITIONAL_VECTORS_INDEX]!=0) {
            propsVectors=pData32+indexes[UPROPS_ADDITIONAL_VECTORS_INDEX];
            countPropsVectors=indexes[UPROPS_RESERVED_INDEX]-indexes[UPROPS_ADDITIONAL_VECTORS_INDEX];
            propsVectorsColumns=indexes[UPROPS_ADDITIONAL_VECTORS_COLUMNS_INDEX];
        }

        havePropsData=1;

        /* if a different thread set it first, then close the extra data */
        if(data!=NULL) {
            udata_close(data); /* NULL if it was set correctly */
        }
    }

    return havePropsData;
}

/* constants and macros for access to the data */

/* getting a uint32_t properties word from the data */
#define HAVE_DATA (havePropsData>0 || (havePropsData==0 && loadPropsData()>0))
#define VALIDATE(c) (((uint32_t)(c))<=0x10ffff && HAVE_DATA)
#define GET_PROPS_UNSAFE(c, result) \
    UTRIE_GET16(&propsTrie, c, result); \
    (result)=props32Table[(result)]
#define GET_PROPS(c, result) \
    if(HAVE_DATA) { \
        GET_PROPS_UNSAFE(c, result); \
    } else { \
        (result)=0; \
    }

/* finding an exception value */
#define HAVE_EXCEPTION_VALUE(flags, index) ((flags)&(1UL<<(index)))

/* number of bits in an 8-bit integer value */
#define EXC_GROUP 8
static const uint8_t flagsOffset[256]={
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

#define ADD_EXCEPTION_OFFSET(flags, index, offset) { \
    if((index)>=EXC_GROUP) { \
        (offset)+=flagsOffset[(flags)&((1<<EXC_GROUP)-1)]; \
        (flags)>>=EXC_GROUP; \
        (index)-=EXC_GROUP; \
    } \
    (offset)+=flagsOffset[(flags)&((1<<(index))-1)]; \
}

U_CFUNC UBool
uprv_haveProperties() {
    return (UBool)HAVE_DATA;
}

/* API functions ------------------------------------------------------------ */

/* Gets the Unicode character's general category.*/
U_CAPI int8_t U_EXPORT2
u_charType(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (int8_t)GET_CATEGORY(props);
}

/* Enumerate all code points with their general categories. */
struct _EnumTypeCallback {
    UCharEnumTypeRange *enumRange;
    const void *context;
};

static uint32_t U_CALLCONV
_enumTypeValue(const void *context, uint32_t value) {
    /* access the general category from the 32-bit properties, and those from the 16-bit trie value */
    return GET_CATEGORY(props32Table[value]);
}

static UBool U_CALLCONV
_enumTypeRange(const void *context, UChar32 start, UChar32 limit, uint32_t value) {
    /* just cast the value to UCharCategory */
    return ((struct _EnumTypeCallback *)context)->
        enumRange(((struct _EnumTypeCallback *)context)->context,
                  start, limit, (UCharCategory)value);
}

U_CAPI void U_EXPORT2
u_enumCharTypes(UCharEnumTypeRange *enumRange, const void *context) {
    struct _EnumTypeCallback callback;

    if(enumRange==NULL || !HAVE_DATA) {
        return;
    }

    callback.enumRange=enumRange;
    callback.context=context;
    utrie_enum(&propsTrie, _enumTypeValue, _enumTypeRange, &callback);
}

/* Checks if ch is a lower case letter.*/
U_CAPI UBool U_EXPORT2
u_islower(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_LOWERCASE_LETTER);
}

/* Checks if ch is an upper case letter.*/
U_CAPI UBool U_EXPORT2
u_isupper(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_UPPERCASE_LETTER);
}

/* Checks if ch is a title case letter; usually upper case letters.*/
U_CAPI UBool U_EXPORT2
u_istitle(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_TITLECASE_LETTER);
}

/* Checks if ch is a decimal digit. */
U_CAPI UBool U_EXPORT2
u_isdigit(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(((1UL<<GET_CATEGORY(props))&
            (1UL<<U_DECIMAL_DIGIT_NUMBER|1UL<<U_OTHER_NUMBER|1UL<<U_LETTER_NUMBER)
           )!=0);
    /* ### TODO: should this not check only U_DECIMAL_DIGIT_NUMBER?! */
}

/* Checks if the Unicode character is a letter.*/
U_CAPI UBool U_EXPORT2
u_isalpha(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(((1UL<<GET_CATEGORY(props))&
            (1UL<<U_UPPERCASE_LETTER|1UL<<U_LOWERCASE_LETTER|1UL<<U_TITLECASE_LETTER|1UL<<U_MODIFIER_LETTER|1UL<<U_OTHER_LETTER)
           )!=0);
}

/* Checks if ch is a letter or a decimal digit */
U_CAPI UBool U_EXPORT2
u_isalnum(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(((1UL<<GET_CATEGORY(props))&
            (1UL<<U_DECIMAL_DIGIT_NUMBER|1UL<<U_OTHER_NUMBER|1UL<<U_LETTER_NUMBER|
             1UL<<U_UPPERCASE_LETTER|1UL<<U_LOWERCASE_LETTER|1UL<<U_TITLECASE_LETTER|1UL<<U_MODIFIER_LETTER|1UL<<U_OTHER_LETTER)
           )!=0);
}

/* Checks if ch is a unicode character with assigned character type.*/
U_CAPI UBool U_EXPORT2
u_isdefined(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)!=0);
}

/* Checks if the Unicode character is a base form character that can take a diacritic.*/
U_CAPI UBool U_EXPORT2
u_isbase(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(((1UL<<GET_CATEGORY(props))&
            (1UL<<U_DECIMAL_DIGIT_NUMBER|1UL<<U_OTHER_NUMBER|1UL<<U_LETTER_NUMBER|
             1UL<<U_UPPERCASE_LETTER|1UL<<U_LOWERCASE_LETTER|1UL<<U_TITLECASE_LETTER|1UL<<U_MODIFIER_LETTER|1UL<<U_OTHER_LETTER|
             1UL<<U_NON_SPACING_MARK|1UL<<U_ENCLOSING_MARK|1UL<<U_COMBINING_SPACING_MARK)
           )!=0);
}

/* Checks if the Unicode character is a control character.*/
U_CAPI UBool U_EXPORT2
u_iscntrl(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           ((1UL<<GET_CATEGORY(props))&
            (1UL<<U_CONTROL_CHAR|1UL<<U_FORMAT_CHAR|1UL<<U_LINE_SEPARATOR|1UL<<U_PARAGRAPH_SEPARATOR)
           )!=0);
}

/* Some control characters that are used as space. */
#define IS_THAT_CONTROL_SPACE(c) \
    ((c>=0x09 && c <= 0x0d) || (c>=0x1c && c <=0x1f) || c==0x85)

/* Checks if the Unicode character is a space character.*/
U_CAPI UBool U_EXPORT2
u_isspace(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((((1UL<<GET_CATEGORY(props))&
            (1UL<<U_SPACE_SEPARATOR|1UL<<U_LINE_SEPARATOR|1UL<<U_PARAGRAPH_SEPARATOR)
           )!=0) || IS_THAT_CONTROL_SPACE(c));
}

/* Checks if the Unicode character is a whitespace character.*/
U_CAPI UBool U_EXPORT2
u_isWhitespace(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((((1UL<<GET_CATEGORY(props))&
            (1UL<<U_SPACE_SEPARATOR|1UL<<U_LINE_SEPARATOR|1UL<<U_PARAGRAPH_SEPARATOR)
           )!=0 &&
           c!=0xa0 && c!=0x202f && c!=0xfeff) || /* exclude no-break spaces */
           IS_THAT_CONTROL_SPACE(c));
}

/* Checks if the Unicode character is printable.*/
U_CAPI UBool U_EXPORT2
u_isprint(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
            ((1UL<<GET_CATEGORY(props))&
            ~(1UL<<U_UNASSIGNED|
              1UL<<U_CONTROL_CHAR|1UL<<U_FORMAT_CHAR|1UL<<U_PRIVATE_USE_CHAR|1UL<<U_SURROGATE|
              1UL<<U_GENERAL_OTHER_TYPES|1UL<<31)
           )!=0);
}

/* Checks if the Unicode character can start a Unicode identifier.*/
U_CAPI UBool U_EXPORT2
u_isIDStart(UChar32 c) {
    /* same as u_isalpha() */
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(((1UL<<GET_CATEGORY(props))&
            (1UL<<U_UPPERCASE_LETTER|1UL<<U_LOWERCASE_LETTER|1UL<<U_TITLECASE_LETTER|1UL<<U_MODIFIER_LETTER|1UL<<U_OTHER_LETTER)
           )!=0);
}

/* Checks if the Unicode character can be a Unicode identifier part other than starting the
 identifier.*/
U_CAPI UBool U_EXPORT2
u_isIDPart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           ((1UL<<GET_CATEGORY(props))&
            (1UL<<U_DECIMAL_DIGIT_NUMBER|1UL<<U_LETTER_NUMBER|
             1UL<<U_UPPERCASE_LETTER|1UL<<U_LOWERCASE_LETTER|1UL<<U_TITLECASE_LETTER|1UL<<U_MODIFIER_LETTER|1UL<<U_OTHER_LETTER|
             1UL<<U_CONNECTOR_PUNCTUATION|1UL<<U_COMBINING_SPACING_MARK|1UL<<U_NON_SPACING_MARK)
           )!=0 ||
           u_isIDIgnorable(c));
}

/*Checks if the Unicode character can be ignorable in a Java or Unicode identifier.*/
U_CAPI UBool U_EXPORT2
u_isIDIgnorable(UChar32 c) {
    return (UBool)((uint32_t)c<=8 ||
           (uint32_t)(c-0xe)<=(0x1b-0xe) ||
           (uint32_t)(c-0x7f)<=(0x9f-0x7f) ||
           (uint32_t)(c-0x200a)<=(0x200f-0x200a) ||
           (uint32_t)(c-0x206a)<=(0x206f-0x206a) ||
           c==0xfeff);
}

/*Checks if the Unicode character can start a Java identifier.*/
U_CAPI UBool U_EXPORT2
u_isJavaIDStart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           ((1UL<<GET_CATEGORY(props))&
            (1UL<<U_UPPERCASE_LETTER|1UL<<U_LOWERCASE_LETTER|1UL<<U_TITLECASE_LETTER|1UL<<U_MODIFIER_LETTER|1UL<<U_OTHER_LETTER|
             1UL<<U_CURRENCY_SYMBOL|1UL<<U_CONNECTOR_PUNCTUATION)
           )!=0);
}

/*Checks if the Unicode character can be a Java identifier part other than starting the
 * identifier.
 */
U_CAPI UBool U_EXPORT2
u_isJavaIDPart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           ((1UL<<GET_CATEGORY(props))&
            (1UL<<U_DECIMAL_DIGIT_NUMBER|1UL<<U_LETTER_NUMBER|
             1UL<<U_UPPERCASE_LETTER|1UL<<U_LOWERCASE_LETTER|1UL<<U_TITLECASE_LETTER|1UL<<U_MODIFIER_LETTER|1UL<<U_OTHER_LETTER|
             1UL<<U_CURRENCY_SYMBOL|1UL<<U_CONNECTOR_PUNCTUATION|
             1UL<<U_COMBINING_SPACING_MARK|1UL<<U_NON_SPACING_MARK)
           )!=0 ||
           u_isIDIgnorable(c));
}

/* Transforms the Unicode character to its lower case equivalent.*/
U_CAPI UChar32 U_EXPORT2
u_tolower(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        if((1UL<<GET_CATEGORY(props))&(1UL<<U_UPPERCASE_LETTER|1UL<<U_TITLECASE_LETTER)) {
            return c+GET_SIGNED_VALUE(props);
        }
    } else {
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe;
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_LOWERCASE)) {
            int i=EXC_LOWERCASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            return (UChar32)*pe;
        }
    }
    return c; /* no mapping - return c itself */
}
    
/* Transforms the Unicode character to its upper case equivalent.*/
U_CAPI UChar32 U_EXPORT2
u_toupper(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        if(GET_CATEGORY(props)==U_LOWERCASE_LETTER) {
            return c-GET_SIGNED_VALUE(props);
        }
    } else {
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe;
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_UPPERCASE)) {
            int i=EXC_UPPERCASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            return (UChar32)*pe;
        }
    }
    return c; /* no mapping - return c itself */
}

/* Transforms the Unicode character to its title case equivalent.*/
U_CAPI UChar32 U_EXPORT2
u_totitle(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        if(GET_CATEGORY(props)==U_LOWERCASE_LETTER) {
            /* here, titlecase is same as uppercase */
            return c-GET_SIGNED_VALUE(props);
        }
    } else {
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe;
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_TITLECASE)) {
            int i=EXC_TITLECASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            return (UChar32)*pe;
        } else if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_UPPERCASE)) {
            /* here, titlecase is same as uppercase */
            int i=EXC_UPPERCASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            return (UChar32)*pe;
        }
    }
    return c; /* no mapping - return c itself */
}

U_CAPI int32_t U_EXPORT2
u_charDigitValue(UChar32 c) {
    uint32_t props, numericType;
    GET_PROPS(c, props);
    numericType=GET_NUMERIC_TYPE(props);
#if 0
    /* ### TODO: new numericType==4 for Han numbers?! */
    if(numericType==0) {
        return -1;
    }
#endif

    if(numericType==1) {
        if(!PROPS_VALUE_IS_EXCEPTION(props)) {
            return GET_SIGNED_VALUE(props);
        } else {
            const uint32_t *pe=GET_EXCEPTIONS(props);
            uint32_t firstExceptionValue=*pe;
            if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_NUMERIC_VALUE)) {
                int i=EXC_NUMERIC_VALUE;
                ++pe;
                ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
                return (int32_t)*pe;
            }
        }
    }

    /* ### TODO: new numericType==4 for Han numbers?! */

    /* if there is no value in the properties table, then check for some special characters */
    /* see Unihan.txt kPrimaryNumeric values 0..9 ### TODO get from (new) UCD file */
    switch(c) {
    case 0x3007:    return 0; /* Han Zero*/
    case 0x4e00:    return 1; /* Han One*/
    case 0x4e8c:    return 2; /* Han Two*/
    case 0x4e09:    return 3; /* Han Three*/
    case 0x56db:    return 4; /* Han Four*/
    case 0x4e94:    return 5; /* Han Five*/
    case 0x516d:    return 6; /* Han Six*/
    case 0x4e03:    return 7; /* Han Seven*/
    case 0x516b:    return 8; /* Han Eight*/
    case 0x4e5d:    return 9; /* Han Nine*/
    default:        return -1; /* no value */
    }
}

U_CAPI double U_EXPORT2
u_getNumericValue(UChar32 c) {
    uint32_t props, numericType;
    GET_PROPS(c, props);
    numericType=GET_NUMERIC_TYPE(props);

    if(numericType==0 || numericType>=(int32_t)U_NT_COUNT) {
        return U_NO_NUMERIC_VALUE;
    } else {
        if(!PROPS_VALUE_IS_EXCEPTION(props)) {
            return GET_SIGNED_VALUE(props);
        } else {
            const uint32_t *pe;
            uint32_t firstExceptionValue;

            double numValue;
            uint32_t denominator;

            pe=GET_EXCEPTIONS(props);
            firstExceptionValue=*pe++;

            if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_NUMERIC_VALUE)) {
                uint32_t flags=firstExceptionValue;
                int i=EXC_NUMERIC_VALUE;
                const uint32_t *p=pe;
                int32_t numerator;

                ADD_EXCEPTION_OFFSET(flags, i, p);
                numerator=(int32_t)*p;

                /*
                 * There are special values for huge numbers that are powers of ten.
                 * genprops/store.c documents:
                 *   if numericValue=0x7fffff00+x then numericValue=10^x
                 */
                if(numerator<0x7fffff00) {
                    numValue=(double)numerator;
                } else {
                    numerator&=0xff;

                    /* 10^x without math.h */
                    numValue=1.;
                    while(numerator>=4) {
                        numValue*=10000.;
                        numerator-=4;
                    }
                    switch(numerator) {
                    case 3:
                        numValue*=1000.;
                        break;
                    case 2:
                        numValue*=100.;
                        break;
                    case 1:
                        numValue*=10.;
                        break;
                    case 0:
                    default:
                        break;
                    }
                }
            } else {
                numValue=0.;
            }
            if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_DENOMINATOR_VALUE)) {
                uint32_t flags=firstExceptionValue;
                int i=EXC_DENOMINATOR_VALUE;
                const uint32_t *p=pe;
                ADD_EXCEPTION_OFFSET(flags, i, p);
                denominator=*p;
            } else {
                denominator=0;
            }

            switch(firstExceptionValue&((1UL<<EXC_NUMERIC_VALUE)|(1UL<<EXC_DENOMINATOR_VALUE))) {
            case 1UL<<EXC_NUMERIC_VALUE:
                return numValue;
            case 1UL<<EXC_DENOMINATOR_VALUE:
                return (double)1./(double)denominator;
            case (1UL<<EXC_NUMERIC_VALUE)|(1UL<<EXC_DENOMINATOR_VALUE):
                return numValue/(double)denominator;
            case 0: /* none (should not occur with numericType>0) */
            default:
                return U_NO_NUMERIC_VALUE;
            }
        }
    }
}

/* Gets the character's linguistic directionality.*/
U_CAPI UCharDirection U_EXPORT2
u_charDirection(UChar32 c) {   
    uint32_t props;
    GET_PROPS(c, props);
    return (UCharDirection)((props>>UPROPS_BIDI_SHIFT)&0x1f);
}

U_CAPI UBool U_EXPORT2
u_isMirrored(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(props&(1UL<<UPROPS_MIRROR_SHIFT) ? TRUE : FALSE);
}

U_CAPI UChar32 U_EXPORT2
u_charMirror(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    if((props&(1UL<<UPROPS_MIRROR_SHIFT))==0) {
        /* not mirrored - the value is not a mirror offset */
        return c;
    } else if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        return c+GET_SIGNED_VALUE(props);
    } else {
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe;
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_MIRROR_MAPPING)) {
            int i=EXC_MIRROR_MAPPING;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            return (UChar32)*pe;
        } else {
            return c;
        }
    }
}

/* ICU 2.1: u_getCombiningClass() moved to unorm.cpp */

U_CAPI int32_t U_EXPORT2
u_digit(UChar32 ch, int8_t radix) {
    int8_t value;
    if((uint8_t)(radix-2)<=(36-2)) {
        value=(int8_t)u_charDigitValue(ch);
        if(value<0) {
            /* ch is not a decimal digit, try latin letters */
            if(ch>=0x61 && ch<=0x7A) {
                value=(int8_t)(ch-0x57);  /* ch - 'a' + 10 */
            } else if(ch>=0x41 && ch<=0x5A) {
                value=(int8_t)(ch-0x37);  /* ch - 'A' + 10 */
            }
        }
    } else {
        value=-1;   /* invalid radix */
    }
    return (int8_t)((value<radix) ? value : -1);
}

U_CAPI UChar32 U_EXPORT2
u_forDigit(int32_t digit, int8_t radix) {
    if((uint8_t)(radix-2)>(36-2) || (uint32_t)digit>=(uint32_t)radix) {
        return 0;
    } else if(digit<10) {
        return (UChar32)(0x30+digit);
    } else {
        return (UChar32)((0x61-10)+digit);
    }
}

/* static data tables ------------------------------------------------------- */

static const UChar cellWidthRanges[] =
{
    0x0000, /* general scripts area*/
    0x1100, /* combining Hangul choseong*/
    0x1160, /* combining Hangul jungseong and jongseong*/
    0x1e00, /* Latin Extended Additional, Greek Extended*/
    0x2000, /* symbols and punctuation*/
    0x3000, /* CJK phonetics & symbols, CJK ideographs, Hangul syllables*/
    0xd800, /* surrogates, private use*/
    0xf900, /* CJK compatibility ideographs*/
    0xfb00, /* alphabetic presentation forms, Arabic presentations forms A, combining half marks*/
    0xfe30, /* CJK compatibility forms, small form variants*/
    0xfe70, /* Arabic presentation forms B*/
    0xff00, /* fullwidth ASCII*/
    0xff60, /* halfwidth, CJK punctuation, Katakana, Hangul Jamo*/
    0xffe0, /* fullwidth punctuation and currency signs*/
    0xffe8, /* halfwidth forms, arrows, and shapes*/
    0xfff0  /* specials*/
};

static const UChar cellWidthValues[] =
{
    U_HALF_WIDTH,    /* general scripts area*/
    U_FULL_WIDTH,    /* combining Hangul choseong*/
    U_ZERO_WIDTH,    /* combining Hangul jungseong and jongseong*/
    U_HALF_WIDTH,    /* Latin extended aAdditional, Greek extended*/
    U_NEUTRAL_WIDTH, /* symbols and punctuation*/
    U_FULL_WIDTH,    /* CJK phonetics & symbols, CJK ideographs, Hangul syllables*/
    U_NEUTRAL_WIDTH, /* surrogates, private use*/
    U_FULL_WIDTH,    /* CJK compatibility ideographs*/
    U_HALF_WIDTH,    /* alphabetic presentation forms, Arabic presentations forms A, combining half marks*/
    U_FULL_WIDTH,    /* CJK compatibility forms, small form variants*/
    U_HALF_WIDTH,    /* Arabic presentation forms B*/
    U_FULL_WIDTH,    /* fullwidth ASCII*/
    U_HALF_WIDTH,    /* halfwidth CJK punctuation, Katakana, Hangul Jamo*/
    U_FULL_WIDTH,    /* fullwidth punctuation and currency signs*/
    U_HALF_WIDTH,    /* halfwidth forms, arrows, and shapes*/
    U_ZERO_WIDTH     /* specials*/
};

#define NUM_CELL_WIDTH_VALUES (sizeof(cellWidthValues)/sizeof(cellWidthValues[0]))
/* Gets table cell width of the Unicode character.*/
U_CAPI uint16_t U_EXPORT2
u_charCellWidth(UChar32 ch)
{
    int16_t i;
    int32_t type = u_charType(ch);

    /* surrogate support is still incomplete */
    if((uint32_t)ch>0xffff) {
        return U_ZERO_WIDTH;
    }

    /* these Unicode character types are scattered throughout the Unicode range, so
     special-case for them*/
    switch (type) {
        case U_UNASSIGNED:
        case U_NON_SPACING_MARK:
        case U_ENCLOSING_MARK:
        case U_LINE_SEPARATOR:
        case U_PARAGRAPH_SEPARATOR:
        case U_CONTROL_CHAR:
        case U_FORMAT_CHAR:
            return U_ZERO_WIDTH;

        default:
            /* for all remaining characters, find out which Unicode range they belong to using
               the table above, and then look up the appropriate return value in that table*/
            for (i = 0; i < (int16_t)NUM_CELL_WIDTH_VALUES; ++i) {
                if (ch < cellWidthRanges[i]) {
                    break;
                }
            }
            --i;
            return cellWidthValues[i];
    }
}

U_CAPI void U_EXPORT2
u_getUnicodeVersion(UVersionInfo versionArray) {
    if(versionArray!=NULL) {
        if(HAVE_DATA) {
            uprv_memcpy(versionArray, dataVersion, U_MAX_VERSION_LENGTH);
        } else {
            uprv_memset(versionArray, 0, U_MAX_VERSION_LENGTH);
        }
    }
}

U_CFUNC uint32_t
u_getUnicodeProperties(UChar32 c, int32_t column) {
    uint16_t vecIndex;

    if(column==-1) {
        uint32_t props;
        GET_PROPS(c, props);
        return props;
    } else if( !HAVE_DATA || countPropsVectors==0 ||
               (uint32_t)c>0x10ffff ||
               column<0 || column>=propsVectorsColumns
    ) {
        return 0;
    } else {
        UTRIE_GET16(&propsVectorsTrie, c, vecIndex);
        return propsVectors[vecIndex+column];
    }
}

U_CFUNC int32_t
uprv_getMaxValues() {
    if(HAVE_DATA) {
        return indexes[UPROPS_MAX_VALUES_INDEX];
    } else {
        return 0;
    }
}

/* string casing ------------------------------------------------------------ */

/*
 * These internal string case mapping functions are here instead of ustring.c
 * because we need efficient access to the character properties.
 *
 * This section contains helper functions that check for conditions
 * in the input text surrounding the current code point
 * according to SpecialCasing.txt.
 *
 * Starting with ICU 2.1, the "surrounding text" is passed in as an instance of
 * UCharIterator to allow the core case mapping functions to be used
 * inside transliterators (using Replaceable instead of UnicodeString/UChar *)
 * etc.
 *
 * Each helper function gets the index
 * - after the current code point if it looks at following text
 * - before the current code point if it looks at preceding text
 *
 * Unicode 3.2 UAX 21 "Case Mappings" defines the conditions as follows:
 *
 * Final_Sigma
 *   C is preceded by a sequence consisting of
 *     a cased letter and a case-ignorable sequence,
 *   and C is not followed by a sequence consisting of
 *     an ignorable sequence and then a cased letter.
 *
 * More_Above
 *   C is followed by one or more characters of combining class 230 (ABOVE)
 *   in the combining character sequence.
 *
 * After_Soft_Dotted
 *   The last preceding character with combining class of zero before C
 *   was Soft_Dotted,
 *   and there is no intervening combining character class 230 (ABOVE).
 *
 * Before_Dot
 *   C is followed by combining dot above (U+0307).
 *   Any sequence of characters with a combining class that is neither 0 nor 230
 *   may intervene between the current character and the combining dot above.
 *
 * Helper definitions in Unicode 3.2 UAX 21:
 *
 * D1. A character C is defined to be cased
 *     if it meets any of the following criteria:
 *
 *   - The general category of C is Titlecase Letter (Lt)
 *   - In [CoreProps], C has one of the properties Uppercase, or Lowercase
 *   - Given D = NFD(C), then it is not the case that:
 *     D = UCD_lower(D) = UCD_upper(D) = UCD_title(D)
 *     (This third criterium does not add any characters to the list
 *      for Unicode 3.2. Ignored.)
 *
 * D2. A character C is defined to be case-ignorable
 *     if it meets either of the following criteria:
 *
 *   - The general category of C is
 *     Nonspacing Mark (Mn), or Enclosing Mark (Me), or Format Control (Cf), or
 *     Letter Modifier (Lm), or Symbol Modifier (Sk)
 *   - C is one of the following characters 
 *     U+0027 APOSTROPHE
 *     U+00AD SOFT HYPHEN (SHY)
 *     U+2019 RIGHT SINGLE QUOTATION MARK
 *            (the preferred character for apostrophe)
 *
 * D3. A case-ignorable sequence is a sequence of
 *     zero or more case-ignorable characters.
 */

enum {
    LOC_ROOT,
    LOC_TURKISH,
    LOC_LITHUANIAN
};

static int32_t
getCaseLocale(const char *locale) {
    char lang[32];
    UErrorCode errorCode;
    int32_t length;

    errorCode=U_ZERO_ERROR;
    length=uloc_getLanguage(locale, lang, sizeof(lang), &errorCode);
    if(U_FAILURE(errorCode) || length!=2) {
        return LOC_ROOT;
    }

    if( (lang[0]=='t' && lang[1]=='r') ||
        (lang[0]=='a' && lang[1]=='z')
    ) {
        return LOC_TURKISH;
    } else if(lang[0]=='l' && lang[1]=='t') {
        return LOC_LITHUANIAN;
    } else {
        return LOC_ROOT;
    }
}

/* Is case-ignorable? */
static U_INLINE UBool
isCaseIgnorable(UChar32 c, uint32_t category) {
    return (FLAG(category)&(_Mn|_Me|_Cf|_Lm|_Sk))!=0 ||
            c==0x27 || c==0xad || c==0x2019;
}

/* Is this a "cased" character? */
static U_INLINE UBool
isCased(UChar32 c, uint32_t category) {
    /* Lt+Uppercase+Lowercase = Lt+Lu+Ll+Other_Uppercase+Other_Lowercase */
    return (FLAG(category)&(_Lt|_Lu|_Ll))!=0 ||
            (u_getUnicodeProperties(c, 1)&(FLAG(UPROPS_OTHER_UPPERCASE)|FLAG(UPROPS_OTHER_LOWERCASE)))!=0;
}

/* Is Soft_Dotted? */
static U_INLINE UBool
isSoftDotted(UChar32 c) {
    return (u_getUnicodeProperties(c, 1)&FLAG(UPROPS_SOFT_DOTTED))!=0;
}

/* Is followed by {case-ignorable}* cased  ? */
static UBool
isFollowedByCasedLetter(UCharIterator *iter, int32_t index) {
    /* This is volatile because AIX 5.1 Visual Age 5.0 in 32-bit mode can't
        optimize this correctly. It couldn't optimize (1UL<<category)&0xE
    */
    volatile uint32_t category;
    uint32_t props;
    int32_t c;

    if(iter==NULL) {
        return FALSE;
    }

    iter->move(iter, index, UITER_ZERO);
    for(;;) {
        c=uiter_next32(iter);
        if(c<0) {
            break;
        }
        GET_PROPS_UNSAFE(c, props);
        category=GET_CATEGORY(props);
        if(isCased(c, category)) {
            return TRUE; /* followed by cased letter */
        }
        if(!isCaseIgnorable(c, category)) {
            return FALSE; /* not ignorable */
        }
    }

    return FALSE; /* not followed by cased letter */
}

/* Is preceded by cased {case-ignorable}*  ? */
static UBool
isPrecededByCasedLetter(UCharIterator *iter, int32_t index) {
    /* This is volatile because AIX 5.1 Visual Age 5.0 in 32-bit mode can't
        optimize this correctly. It couldn't optimize (1UL<<category)&0xE
    */
    volatile uint32_t category;
    uint32_t props;
    int32_t c;

    if(iter==NULL) {
        return FALSE;
    }

    iter->move(iter, index, UITER_ZERO);
    for(;;) {
        c=uiter_previous32(iter);
        if(c<0) {
            break;
        }
        GET_PROPS_UNSAFE(c, props);
        category=GET_CATEGORY(props);
        if(isCased(c, category)) {
            return TRUE; /* preceded by cased letter */
        }
        if(!isCaseIgnorable(c, category)) {
            return FALSE; /* not ignorable */
        }
    }

    return FALSE; /* not followed by cased letter */
}

/* Is preceded by Soft_Dotted character with no intervening cc=230 ? */
static UBool
isPrecededBySoftDotted(UCharIterator *iter, int32_t index) {
    int32_t c;
    uint8_t cc;

    if(iter==NULL) {
        return FALSE;
    }

    iter->move(iter, index, UITER_ZERO);
    for(;;) {
        c=uiter_previous32(iter);
        if(c<0) {
            break;
        }
        if(isSoftDotted(c)) {
            return TRUE; /* preceded by TYPE_i */
        }

        cc=u_getCombiningClass(c);
        if(cc==0 || cc==230) {
            return FALSE; /* preceded by different base character (not TYPE_i), or intervening cc==230 */
        }
    }

    return FALSE; /* not preceded by TYPE_i */
}

#if 0
/*
 * ### TODO write a bug doc for the UTC and re-enable this with a newer version
 * of Unicode.
 *
 * ICU 2.0/2.1 used to check for After_I for the Turkic-conditional removal
 * of U+0307 instead of checking for After_i (now After_Soft_Dotted).
 *
 * I believe that After_Soft_Dotted is a mistake because it results in different
 * lowercase mappings for the canonically equivalent I-dot and I+dot
 * (should both map to i).
 * The comment in SpecialCasing.txt appears to agree.

# When lowercasing, remove dot_above in the sequence I + dot_above, which will turn into i.
# This matches the behavior of the canonically equivalent I-dot_above

0307; ; 0307; 0307; tr After_Soft_Dotted; # COMBINING DOT ABOVE
0307; ; 0307; 0307; az After_Soft_Dotted; # COMBINING DOT ABOVE

 * For ICU 2.2 I am withdrawing this "fix" to make ICU conform to Unicode 3.2.
 *
 * Markus W. Scherer 2002-jun-07
 */

/* Is preceded by base character 'I' with no intervening cc=230 ? */
static UBool
isAfter_I(UCharIterator *iter, int32_t index) {
    int32_t c;
    uint8_t cc;

    if(iter==NULL) {
        return FALSE;
    }

    iter->move(iter, index, UITER_ZERO);
    for(;;) {
        c=uiter_previous32(iter);
        if(c<0) {
            break;
        }
        if(c==0x49) {
            return TRUE; /* preceded by I */
        }

        cc=u_getCombiningClass(c);
        if(cc==0 || cc==230) {
            return FALSE; /* preceded by different base character (not I), or intervening cc==230 */
        }
    }

    return FALSE; /* not preceded by I */
}
#endif

/* Is followed by one or more cc==230 ? */
static UBool
isFollowedByMoreAbove(UCharIterator *iter, int32_t index) {
    int32_t c;
    uint8_t cc;

    if(iter==NULL) {
        return FALSE;
    }

    iter->move(iter, index, UITER_ZERO);
    for(;;) {
        c=uiter_next32(iter);
        if(c<0) {
            break;
        }
        cc=u_getCombiningClass(c);
        if(cc==230) {
            return TRUE; /* at least one cc==230 following */
        }
        if(cc==0) {
            return FALSE; /* next base character, no more cc==230 following */
        }
    }

    return FALSE; /* no more cc==230 following */
}

/* Is followed by a dot above (without cc==230 in between) ? */
static UBool
isFollowedByDotAbove(UCharIterator *iter, int32_t index) {
    int32_t c;
    uint8_t cc;

    if(iter==NULL) {
        return FALSE;
    }

    iter->move(iter, index, UITER_ZERO);
    for(;;) {
        c=uiter_next32(iter);
        if(c<0) {
            break;
        }
        if(c==0x307) {
            return TRUE;
        }
        cc=u_getCombiningClass(c);
        if(cc==0 || cc==230) {
            return FALSE; /* next base character or cc==230 in between */
        }
    }

    return FALSE; /* no dot above following */
}

/* lowercasing -------------------------------------------------------------- */

/* internal, see ustr_imp.h */
U_CAPI int32_t U_EXPORT2
u_internalToLower(UChar32 c, UCharIterator *iter,
                  UChar *dest, int32_t destCapacity,
                  const char *locale) {
    UChar buffer[8];
    uint32_t props;
    UChar32 result;
    int32_t i, length;

    result=c;
    GET_PROPS(c, props);
    if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        if((1UL<<GET_CATEGORY(props))&(1UL<<U_UPPERCASE_LETTER|1UL<<U_TITLECASE_LETTER)) {
            result=c+GET_SIGNED_VALUE(props);
        }
    } else {
        const UChar *u;
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe, specialCasing;
        int32_t minLength;

        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_SPECIAL_CASING)) {
            i=EXC_SPECIAL_CASING;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            specialCasing=*pe;
            /* fill u and length with the case mapping result string */
            if(specialCasing&0x80000000) {
                /* use hardcoded conditions and mappings */
                int32_t loc=getCaseLocale(locale),
                        srcIndex= iter!=NULL ? iter->getIndex(iter, UITER_CURRENT) : 0;

                /*
                 * Test for conditional mappings first
                 *   (otherwise the unconditional default mappings are always taken),
                 * then test for characters that have unconditional mappings in SpecialCasing.txt,
                 * then get the UnicodeData.txt mappings.
                 */
                if( loc==LOC_LITHUANIAN &&
                        /* base characters, find accents above */
                        (((c==0x49 || c==0x4a || c==0x12e) &&
                            isFollowedByMoreAbove(iter, srcIndex)) ||
                        /* precomposed with accent above, no need to find one */
                        (c==0xcc || c==0xcd || c==0x128))
                ) {
                    /*
                        # Lithuanian

                        # Lithuanian retains the dot in a lowercase i when followed by accents.

                        # Introduce an explicit dot above when lowercasing capital I's and J's
                        # whenever there are more accents above.
                        # (of the accents used in Lithuanian: grave, acute, tilde above, and ogonek)

                        0049; 0069 0307; 0049; 0049; lt More_Above; # LATIN CAPITAL LETTER I
                        004A; 006A 0307; 004A; 004A; lt More_Above; # LATIN CAPITAL LETTER J
                        012E; 012F 0307; 012E; 012E; lt More_Above; # LATIN CAPITAL LETTER I WITH OGONEK
                        00CC; 0069 0307 0300; 00CC; 00CC; lt; # LATIN CAPITAL LETTER I WITH GRAVE
                        00CD; 0069 0307 0301; 00CD; 00CD; lt; # LATIN CAPITAL LETTER I WITH ACUTE
                        0128; 0069 0307 0303; 0128; 0128; lt; # LATIN CAPITAL LETTER I WITH TILDE
                     */
                    u=buffer;
                    buffer[1]=0x307;
                    switch(c) {
                    case 0x49:  /* LATIN CAPITAL LETTER I */
                        buffer[0]=0x69;
                        length=2;
                        break;
                    case 0x4a:  /* LATIN CAPITAL LETTER J */
                        buffer[0]=0x6a;
                        length=2;
                        break;
                    case 0x12e: /* LATIN CAPITAL LETTER I WITH OGONEK */
                        buffer[0]=0x12f;
                        length=2;
                        break;
                    case 0xcc:  /* LATIN CAPITAL LETTER I WITH GRAVE */
                        buffer[0]=0x69;
                        buffer[2]=0x300;
                        length=3;
                        break;
                    case 0xcd:  /* LATIN CAPITAL LETTER I WITH ACUTE */
                        buffer[0]=0x69;
                        buffer[2]=0x301;
                        length=3;
                        break;
                    case 0x128: /* LATIN CAPITAL LETTER I WITH TILDE */
                        buffer[0]=0x69;
                        buffer[2]=0x303;
                        length=3;
                        break;
                    default:
                        return 0; /* will not occur */
                    }
                /* # Turkish and Azeri */
                } else if(loc==LOC_TURKISH && c==0x130) {
                    /*
                        # I and i-dotless; I-dot and i are case pairs in Turkish and Azeri
                        # The following rules handle those cases.

                        0130; 0069; 0130; 0130; tr # LATIN CAPITAL LETTER I WITH DOT ABOVE
                        0130; 0069; 0130; 0130; az # LATIN CAPITAL LETTER I WITH DOT ABOVE
                     */
                    result=0x69;
                    goto single;
                } else if(loc==LOC_TURKISH && c==0x307 && isPrecededBySoftDotted(iter, srcIndex-1)) {
                    /* ### TODO see comment above about isAfter_I() */
                    /*
                        # When lowercasing, remove dot_above in the sequence I + dot_above, which will turn into i.
                        # This matches the behavior of the canonically equivalent I-dot_above

                        0307; ; 0307; 0307; tr After_Soft_Dotted; # COMBINING DOT ABOVE
                        0307; ; 0307; 0307; az After_Soft_Dotted; # COMBINING DOT ABOVE
                     */
                    return 0; /* remove the dot (continue without output) */
                } else if(loc==LOC_TURKISH && c==0x49 && !isFollowedByDotAbove(iter, srcIndex)) {
                    /*
                        # When lowercasing, unless an I is before a dot_above, it turns into a dotless i.

                        0049; 0131; 0049; 0049; tr Not_Before_Dot; # LATIN CAPITAL LETTER I
                        0049; 0131; 0049; 0049; az Not_Before_Dot; # LATIN CAPITAL LETTER I
                     */
                    result=0x131;
                    goto single;
                } else if(c==0x130) {
                    /*
                        # Preserve canonical equivalence for I with dot. Turkic is handled below.

                        0130; 0069 0307; 0130; 0130; # LATIN CAPITAL LETTER I WITH DOT ABOVE
                     */
                    static const UChar iWithDot[2]={ 0x69, 0x307 };
                    u=iWithDot;
                    length=2;
                } else if(  c==0x3a3 &&
                            !isFollowedByCasedLetter(iter, srcIndex) &&
                            isPrecededByCasedLetter(iter, srcIndex-1)
                ) {
                    /* greek capital sigma maps depending on surrounding cased letters (see SpecialCasing.txt) */
                    /*
                        # Special case for final form of sigma

                        03A3; 03C2; 03A3; 03A3; Final_Sigma; # GREEK CAPITAL LETTER SIGMA
                     */
                    result=0x3c2; /* greek small final sigma */
                    goto single;
                } else {
                    /* no known conditional special case mapping, use a normal mapping */
                    pe=GET_EXCEPTIONS(props); /* restore the initial exception pointer */
                    firstExceptionValue=*pe;
                    goto notSpecial;
                }
            } else {
                /* get the special case mapping string from the data file */
                u=ucharsTable+(specialCasing&0xffff);
                length=(int32_t)((*u++)&0x1f);
            }

            /* copy the result string */
            minLength = (length < destCapacity) ? length : destCapacity;
            i=0;
            while(i<minLength) {
                dest[i++]=*u++;
            }
            return length;
        }

notSpecial:
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_LOWERCASE)) {
            i=EXC_LOWERCASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            result=(UChar32)*pe;
        }
    }

single:
    length=UTF_CHAR_LENGTH(result);
    if(length<=destCapacity) {
        /* write result to dest */
        i=0;
        UTF_APPEND_CHAR_UNSAFE(dest, i, result);
    }
    return (result==c) ? -length : length;
}

/*
 * Lowercases [srcStart..srcLimit[ but takes
 * context [0..srcLength[ into account.
 */
U_CFUNC int32_t
u_internalStrToLower(UChar *dest, int32_t destCapacity,
                     const UChar *src, int32_t srcLength,
                     int32_t srcStart, int32_t srcLimit,
                     const char *locale,
                     UErrorCode *pErrorCode) {
    UCharIterator iter;
    uint32_t props;
    int32_t srcIndex, destIndex;
    UChar32 c;

    /* test early, once, if there is a data file */
    if(!HAVE_DATA) {
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return 0;
    }

    /* set up local variables */
    uiter_setString(&iter, src, srcLength);

    /* case mapping loop */
    srcIndex=srcStart;
    destIndex=0;
    while(srcIndex<srcLimit) {
        UTF_NEXT_CHAR(src, srcIndex, srcLimit, c);
        GET_PROPS_UNSAFE(c, props);
        if(!PROPS_VALUE_IS_EXCEPTION(props)) {
            if((1UL<<GET_CATEGORY(props))&(1UL<<U_UPPERCASE_LETTER|1UL<<U_TITLECASE_LETTER)) {
                c+=GET_SIGNED_VALUE(props);
            }

            /* handle 1:1 code point mappings from UnicodeData.txt */
            if(c<=0xffff) {
                if(destIndex<destCapacity) {
                    dest[destIndex++]=(UChar)c;
                } else {
                    /* buffer overflow */
                    /* keep incrementing the destIndex for preflighting */
                    ++destIndex;
                }
            } else {
                if((destIndex+2)<=destCapacity) {
                    dest[destIndex++]=UTF16_LEAD(c);
                    dest[destIndex++]=UTF16_TRAIL(c);
                } else {
                    /* buffer overflow */
                    /* write the first surrogate if possible */
                    if(destIndex<destCapacity) {
                        dest[destIndex]=UTF16_LEAD(c);
                    }
                    /* keep incrementing the destIndex for preflighting */
                    destIndex+=2;
                }
            }
        } else {
            /* handle all exceptions in u_internalToLower() */
            int32_t length;

            iter.move(&iter, srcIndex, UITER_ZERO);
            if(destIndex<destCapacity) {
                length=u_internalToLower(c, &iter, dest+destIndex, destCapacity-destIndex, locale);
            } else {
                length=u_internalToLower(c, &iter, NULL, 0, locale);
            }
            if(length<0) {
                length=-length;
            }
            destIndex+=length;
        }
    }

    if(destIndex>destCapacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return destIndex;
}

/* uppercasing -------------------------------------------------------------- */

/* internal */
static int32_t
u_internalToUpperOrTitle(UChar32 c, UCharIterator *iter,
                         UChar *dest, int32_t destCapacity,
                         const char *locale,
                         UBool upperNotTitle) {
    uint32_t props;
    UChar32 result;
    int32_t i, length;

    result=c;
    GET_PROPS(c, props);
    if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        if(GET_CATEGORY(props)==U_LOWERCASE_LETTER) {
            result=c-GET_SIGNED_VALUE(props);
        }
    } else {
        const UChar *u;
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe, specialCasing;
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_SPECIAL_CASING)) {
            i=EXC_SPECIAL_CASING;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            specialCasing=*pe;
            /* fill u and length with the case mapping result string */
            if(specialCasing&0x80000000) {
                /* use hardcoded conditions and mappings */
                int32_t loc=getCaseLocale(locale),
                        srcIndex= iter!=NULL ? iter->getIndex(iter, UITER_CURRENT) : 0;

                if(loc==LOC_TURKISH && c==0x69) {
                    /*
                        # Turkish and Azeri

                        # I and i-dotless; I-dot and i are case pairs in Turkish and Azeri
                        # The following rules handle those cases.

                        # When uppercasing, i turns into a dotted capital I

                        0069; 0069; 0130; 0130; tr; # LATIN SMALL LETTER I
                        0069; 0069; 0130; 0130; az; # LATIN SMALL LETTER I
                    */
                    result=0x130;
                    goto single;
                } else if(loc==LOC_LITHUANIAN && c==0x307 && isPrecededBySoftDotted(iter, srcIndex-1)) {
                    /*
                        # Lithuanian

                        # Lithuanian retains the dot in a lowercase i when followed by accents.

                        # Remove DOT ABOVE after "i" with upper or titlecase

                        0307; 0307; ; ; lt After_Soft_Dotted; # COMBINING DOT ABOVE
                     */
                    return 0; /* remove the dot (continue without output) */
                } else {
                    /* no known conditional special case mapping, use a normal mapping */
                    pe=GET_EXCEPTIONS(props); /* restore the initial exception pointer */
                    firstExceptionValue=*pe;
                    goto notSpecial;
                }
            } else {
                /* get the special case mapping string from the data file */
                u=ucharsTable+(specialCasing&0xffff);
                length=(int32_t)*u++;

                /* skip the lowercase result string */
                u+=length&0x1f;
                if(upperNotTitle) {
                    length=(length>>5)&0x1f;
                } else {
                    /* skip the uppercase result strings too */
                    u+=(length>>5)&0x1f;
                    length=(length>>10)&0x1f;
                }
            }

            /* copy the result string */
            i=0;
            while(i<length && i<destCapacity) {
                dest[i++]=*u++;
            }
            return length;
        }

notSpecial:
        if(!upperNotTitle && HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_TITLECASE)) {
            i=EXC_TITLECASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            result=(UChar32)*pe;
        } else if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_UPPERCASE)) {
            /* here, titlecase is same as uppercase */
            i=EXC_UPPERCASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            result=(UChar32)*pe;
        }
    }

single:
    length=UTF_CHAR_LENGTH(result);
    if(length<=destCapacity) {
        /* write result to dest */
        i=0;
        UTF_APPEND_CHAR_UNSAFE(dest, i, result);
    }
    return (result==c) ? -length : length;
}

/* internal, see ustr_imp.h */
U_CAPI int32_t U_EXPORT2
u_internalToUpper(UChar32 c, UCharIterator *iter,
                  UChar *dest, int32_t destCapacity,
                  const char *locale) {
    return u_internalToUpperOrTitle(c, iter, dest, destCapacity, locale, TRUE);
}

U_CFUNC int32_t
u_internalStrToUpper(UChar *dest, int32_t destCapacity,
                     const UChar *src, int32_t srcLength,
                     const char *locale,
                     UErrorCode *pErrorCode) {
    UCharIterator iter;
    uint32_t props;
    int32_t srcIndex, destIndex;
    UChar32 c;

    /* test early, once, if there is a data file */
    if(!HAVE_DATA) {
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return 0;
    }

    /* set up local variables */
    uiter_setString(&iter, src, srcLength);

    /* case mapping loop */
    srcIndex=destIndex=0;
    while(srcIndex<srcLength) {
        UTF_NEXT_CHAR(src, srcIndex, srcLength, c);
        GET_PROPS_UNSAFE(c, props);
        if(!PROPS_VALUE_IS_EXCEPTION(props)) {
            if(GET_CATEGORY(props)==U_LOWERCASE_LETTER) {
                c-=GET_SIGNED_VALUE(props);
            }

            /* handle 1:1 code point mappings from UnicodeData.txt */
            if(c<=0xffff) {
                if(destIndex<destCapacity) {
                    dest[destIndex++]=(UChar)c;
                } else {
                    /* buffer overflow */
                    /* keep incrementing the destIndex for preflighting */
                    ++destIndex;
                }
            } else {
                if((destIndex+2)<=destCapacity) {
                    dest[destIndex++]=UTF16_LEAD(c);
                    dest[destIndex++]=UTF16_TRAIL(c);
                } else {
                    /* buffer overflow */
                    /* write the first surrogate if possible */
                    if(destIndex<destCapacity) {
                        dest[destIndex]=UTF16_LEAD(c);
                    }
                    /* keep incrementing the destIndex for preflighting */
                    destIndex+=2;
                }
            }
        } else {
            /* handle all exceptions in u_internalToUpper() */
            int32_t length;

            iter.move(&iter, srcIndex, UITER_ZERO);
            if(destIndex<destCapacity) {
                length=u_internalToUpperOrTitle(c, &iter, dest+destIndex, destCapacity-destIndex, locale, TRUE);
            } else {
                length=u_internalToUpperOrTitle(c, &iter, NULL, 0, locale, TRUE);
            }
            if(length<0) {
                length=-length;
            }
            destIndex+=length;
        }
    }

    if(destIndex>destCapacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return destIndex;
}

/* titlecasing -------------------------------------------------------------- */

/* internal, see ustr_imp.h */
U_CAPI int32_t U_EXPORT2
u_internalToTitle(UChar32 c, UCharIterator *iter,
                  UChar *dest, int32_t destCapacity,
                  const char *locale) {
    return u_internalToUpperOrTitle(c, iter, dest, destCapacity, locale, FALSE);
}

/* case folding ------------------------------------------------------------- */

/*
 * Case folding is similar to lowercasing.
 * The result may be a simple mapping, i.e., a single code point, or
 * a full mapping, i.e., a string.
 * If the case folding for a code point is the same as its simple (1:1) lowercase mapping,
 * then only the lowercase mapping is stored.
 *
 * Some special cases are hardcoded because their conditions cannot be
 * parsed and processed from CaseFolding.txt.
 *
 * Unicode 3.2 CaseFolding.txt specifies for its status field:

# C: common case folding, common mappings shared by both simple and full mappings.
# F: full case folding, mappings that cause strings to grow in length. Multiple characters are separated by spaces.
# S: simple case folding, mappings to single characters where different from F.
# T: special case for uppercase I and dotted uppercase I
#    - For non-Turkic languages, this mapping is normally not used.
#    - For Turkic languages (tr, az), this mapping can be used instead of the normal mapping for these characters.
#
# Usage:
#  A. To do a simple case folding, use the mappings with status C + S.
#  B. To do a full case folding, use the mappings with status C + F.
#
#    The mappings with status T can be used or omitted depending on the desired case-folding
#    behavior. (The default option is to exclude them.)

 * Unicode 3.2 has 'T' mappings as follows:

0049; T; 0131; # LATIN CAPITAL LETTER I
0130; T; 0069; # LATIN CAPITAL LETTER I WITH DOT ABOVE

 * while the default mappings for these code points are:

0049; C; 0069; # LATIN CAPITAL LETTER I
0130; F; 0069 0307; # LATIN CAPITAL LETTER I WITH DOT ABOVE

 * U+0130 is otherwise lowercased to U+0069 (UnicodeData.txt).
 *
 * In case this code is used with CaseFolding.txt from an older version of Unicode
 * where CaseFolding.txt contains mappings with a status of 'I' that
 * have the opposite polarity ('I' mappings are included by default but excluded for Turkic),
 * we must also hardcode the Unicode 3.2 mappings for the code points 
 * with 'I' mappings.
 * Unicode 3.1.1 has 'I' mappings for U+0130 and U+0131.
 * Unicode 3.2 has a 'T' mapping for U+0130, and lowercases U+0131 to itself (see UnicodeData.txt).
 */

/* return the simple case folding mapping for c */
U_CAPI UChar32 U_EXPORT2
u_foldCase(UChar32 c, uint32_t options) {
    uint32_t props;
    GET_PROPS(c, props);
    if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        if((1UL<<GET_CATEGORY(props))&(1UL<<U_UPPERCASE_LETTER|1UL<<U_TITLECASE_LETTER)) {
            return c+GET_SIGNED_VALUE(props);
        }
    } else {
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe;
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_CASE_FOLDING)) {
            const uint32_t *oldPE=pe;
            int i=EXC_CASE_FOLDING;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            props=*pe;
            if(props!=0) {
                /* return the simple mapping, if there is one */
                const UChar *uchars=ucharsTable+(props&0xffff);
                UChar32 simple;
                i=0;
                UTF_NEXT_CHAR_UNSAFE(uchars, i, simple);
                if(simple!=0) {
                    return simple;
                }
                /* fall through to use the lowercase exception value if there is no simple mapping */
                pe=oldPE;
            } else {
                /* special case folding mappings, hardcoded */
                if((options&_FOLD_CASE_OPTIONS_MASK)==U_FOLD_CASE_DEFAULT) {
                    /* default mappings */
                    if(c==0x49) {
                        /* 0049; C; 0069; # LATIN CAPITAL LETTER I */
                        return 0x69;
                    } else if(c==0x130) {
                        /* no simple default mapping for U+0130, use UnicodeData.txt */
                        return 0x69;
                    }
                } else {
                    /* Turkic mappings */
                    if(c==0x49) {
                        /* 0049; T; 0131; # LATIN CAPITAL LETTER I */
                        return 0x131;
                    } else if(c==0x130) {
                        /* 0130; T; 0069; # LATIN CAPITAL LETTER I WITH DOT ABOVE */
                        return 0x69;
                    }
                }
                /* return c itself because there is no special mapping for it */
                return c;
            }
        }
        /* not else! - allow to fall through from above */
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_LOWERCASE)) {
            int i=EXC_LOWERCASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            return (UChar32)*pe;
        }
    }
    return c; /* no mapping - return c itself */
}

/* internal, see ustr_imp.h */
U_CAPI int32_t U_EXPORT2
u_internalFoldCase(UChar32 c,
                   UChar *dest, int32_t destCapacity,
                   uint32_t options) {
    uint32_t props;
    UChar32 result;
    int32_t i, length;

    result=c;
    GET_PROPS_UNSAFE(c, props);
    if(!PROPS_VALUE_IS_EXCEPTION(props)) {
        if((1UL<<GET_CATEGORY(props))&(1UL<<U_UPPERCASE_LETTER|1UL<<U_TITLECASE_LETTER)) {
            /* same as lowercase */
            result=c+GET_SIGNED_VALUE(props);
        }
    } else {
        const uint32_t *pe=GET_EXCEPTIONS(props);
        uint32_t firstExceptionValue=*pe;
        if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_CASE_FOLDING)) {
            i=EXC_CASE_FOLDING;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            props=*pe;
            if(props!=0) {
                /* return the full mapping */
                const UChar *uchars=ucharsTable+(props&0xffff)+2;
                int32_t minLength;

                length=props>>24;
                minLength = (length < destCapacity) ? length : destCapacity;

                /* copy the result string */
                i=0;
                while(i<minLength) {
                    dest[i++]=*(uchars++);
                }
                return length;
            } else {
                /* special case folding mappings, hardcoded */
                if((options&_FOLD_CASE_OPTIONS_MASK)==U_FOLD_CASE_DEFAULT) {
                    /* default mappings */
                    if(c==0x49) {
                        /* 0049; C; 0069; # LATIN CAPITAL LETTER I */
                        result=0x69;
                    } else if(c==0x130) {
                        /* 0130; F; 0069 0307; # LATIN CAPITAL LETTER I WITH DOT ABOVE */
                        if(0<destCapacity) {
                            dest[0]=0x69;
                        }
                        if(1<destCapacity) {
                            dest[1]=0x307;
                        }
                        return 2;
                    }
                } else {
                    /* Turkic mappings */
                    if(c==0x49) {
                        /* 0049; T; 0131; # LATIN CAPITAL LETTER I */
                        result=0x131;
                    } else if(c==0x130) {
                        /* 0130; T; 0069; # LATIN CAPITAL LETTER I WITH DOT ABOVE */
                        result=0x69;
                    }
                }
                /* return c itself because there is no special mapping for it */
                /* goto single; */
            }
        } else if(HAVE_EXCEPTION_VALUE(firstExceptionValue, EXC_LOWERCASE)) {
            i=EXC_LOWERCASE;
            ++pe;
            ADD_EXCEPTION_OFFSET(firstExceptionValue, i, pe);
            result=(UChar32)*pe;
        }
    }

/* single: */
    length=UTF_CHAR_LENGTH(result);
    if(length<=destCapacity) {
        /* write result to dest */
        i=0;
        UTF_APPEND_CHAR_UNSAFE(dest, i, result);
    }
    return (result==c) ? -length : length;
}

/* case-fold the source string using the full mappings */
U_CFUNC int32_t
u_internalStrFoldCase(UChar *dest, int32_t destCapacity,
                      const UChar *src, int32_t srcLength,
                      uint32_t options,
                      UErrorCode *pErrorCode) {
    uint32_t props;
    int32_t srcIndex, destIndex;
    UChar32 c;

    /* test early, once, if there is a data file */
    if(!HAVE_DATA) {
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return 0;
    }

    /* case mapping loop */
    srcIndex=destIndex=0;
    while(srcIndex<srcLength) {
        UTF_NEXT_CHAR(src, srcIndex, srcLength, c);
        GET_PROPS_UNSAFE(c, props);
        if(!PROPS_VALUE_IS_EXCEPTION(props)) {
            if((1UL<<GET_CATEGORY(props))&(1UL<<U_UPPERCASE_LETTER|1UL<<U_TITLECASE_LETTER)) {
                c+=GET_SIGNED_VALUE(props);
            }

            /* handle 1:1 code point mappings from UnicodeData.txt */
            if(c<=0xffff) {
                if(destIndex<destCapacity) {
                    dest[destIndex++]=(UChar)c;
                } else {
                    /* buffer overflow */
                    /* keep incrementing the destIndex for preflighting */
                    ++destIndex;
                }
            } else {
                if((destIndex+2)<=destCapacity) {
                    dest[destIndex++]=UTF16_LEAD(c);
                    dest[destIndex++]=UTF16_TRAIL(c);
                } else {
                    /* buffer overflow */
                    /* write the first surrogate if possible */
                    if(destIndex<destCapacity) {
                        dest[destIndex]=UTF16_LEAD(c);
                    }
                    /* keep incrementing the destIndex for preflighting */
                    destIndex+=2;
                }
            }
        } else {
            /* handle all exceptions in u_internalFoldCase() */
            int32_t length;

            if(destIndex<destCapacity) {
                length=u_internalFoldCase(c, dest+destIndex, destCapacity-destIndex, options);
            } else {
                length=u_internalFoldCase(c, NULL, 0, options);
            }
            if(length<0) {
                length=-length;
            }
            destIndex+=length;
        }
    }

    if(destIndex>destCapacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return destIndex;
}
