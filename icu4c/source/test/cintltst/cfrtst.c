/********************************************************************
 * COPYRIGHT: 
 * Copyright (C) 2016 and later: Unicode, Inc. and others.
 * License & terms of use: http://www.unicode.org/copyright.html
 ********************************************************************/
/********************************************************************************
*
* File CFRTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Ported for C API
*********************************************************************************/
/**
 * CollationFrenchTest is a third level test class.  This tests the locale
 * specific primary, secondary and tertiary rules.  For example, the ignorable
 * character '-' in string "black-bird".  The en_US locale uses the default
 * collation rules as its sorting sequence.
 */

#include <stdlib.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "cfrtst.h"
#include "cmemory.h"
#include "unicode/ustring.h"
#include "string.h"

static  UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] =
{
    {0x0061/*'a'*/, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},
    {0x0043/*'C'*/, 0x004f/*'O'*/, 0x0054/*'T'*/, 0x0045/*'E'*/, 0x0000},
    {0x0063/*'c'*/, 0x006f/*'o'*/, 0x002d/*'-'*/, 0x006f/*'o'*/, 0x0070/*'p'*/, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0070/*'p'*/, 0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0070/*'p'*/, 0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0048/*'H'*/, 0x0065/*'e'*/, 0x006c/*'l'*/, 0x006c/*'l'*/, 0x006f/*'o'*/, 0x0000},
    {0x01f1, 0x0000},
    {0xfb00, 0x0000},
    {0x01fa, 0x0000},
    {0x0101, 0x0000}
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] =
{
    {0x0041/*'A'*/, 0x0042/*'B'*/, 0x0043/*'C'*/, 0x0000},
    {0x0063/*'c'*/, 0x00f4, 0x0074/*'t'*/, 0x0065/*'e'*/, 0x0000},
    {0x0043/*'C'*/, 0x004f/*'O'*/, 0x004f/*'O'*/, 0x0050/*'P'*/, 0x0000},
    {0x0070/*'p'*/, 0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x00E9, 0x0000},
    {0x0070/*'p'*/,  0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x00E9, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0068/*'h'*/, 0x0065/*'e'*/, 0x006c/*'l'*/, 0x006c/*'l'*/, 0x004f/*'O'*/, 0x0000},
    {0x01ee, 0x0000},
    {0x25ca, 0x0000},
    {0x00e0, 0x0000},
    {0x01df, 0x0000}
};

const static UCollationResult results[] =
{
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS, /*UCOL_GREATER,*/
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_LESS, /*UCOL_GREATER,*/
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_LESS
};

/* 0x0300 is grave, 0x0301 is acute*/
/* the order of elements in this array must be different than the order in CollationEnglishTest*/
const static UChar testAcute[][MAX_TOKEN_LEN] =
{
/*00*/    {0x0065/*'e'*/, 0x0065/*'e'*/,  0x0000},
/*01*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/,  0x0000},
/*02*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/,  0x0000},
/*03*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/,  0x0000},
/*04*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/,  0x0000},
/*05*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0301, 0x0000}, 
/*06*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0000},
/*07*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0000},
/*08*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0000},
/*09*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0000},
/*0a*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0b*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0c*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0d*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0e*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0f*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0300, 0x0000},
/*10*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0000},
/*11*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0000},
/*12*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0000},
/*13*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0000},
/*14*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*15*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*16*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*17*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*18*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000}
};

const static UChar testBugs[][MAX_TOKEN_LEN] =
{
    {0x0061/*'a'*/, 0x000},
    {0x0041/*'A'*/, 0x000},
    {0x0065/*'e'*/, 0x000},
    {0x0045/*'E'*/, 0x000},
    {0x00e9, 0x000},
    {0x00e8, 0x000},
    {0x00ea, 0x000},
    {0x00eb, 0x000},
    {0x0065/*'e'*/, 0x0061/*'a'*/, 0x000},
    {0x0078/*'x'*/, 0x000}
};


static void TestGetSortKey(void);


void addFrenchCollTest(TestNode** root)
{
    addTest(root, &TestSecondary, "tscoll/cfrtst/TestSecondary");
    addTest(root, &TestTertiary, "tscoll/cfrtst/TestTertiary");
    addTest(root, &TestExtra, "tscoll/cfrtst/TestExtra");
    addTest(root, &TestGetSortKey, "tscoll/cfrtst/TestGetSortKey");
}


static void TestTertiary( )
{

    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("fr_CA", &status);
    if(U_FAILURE(status) || !myCollation){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }

    ucol_setAttribute(myCollation, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing fr_CA Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_QUATERNARY);
    for (i = 0; i < 12 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestSecondary()
{
    int32_t i,j, testAcuteSize;
    UCollationResult expected=UCOL_EQUAL;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("fr_CA", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    ucol_setAttribute(myCollation, UCOL_STRENGTH, UCOL_SECONDARY, &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing fr_CA Collation with Secondary strength\n");
    /*test acute and grave ordering (compare to french collation)*/
    testAcuteSize = UPRV_LENGTHOF(testAcute);
    for (i = 0; i < testAcuteSize; i++)
    {
        for (j = 0; j < testAcuteSize; j++)
        {
            if (i <  j) expected = UCOL_LESS;
            if (i == j) expected = UCOL_EQUAL;
            if (i >  j) expected = UCOL_GREATER;
            doTest(myCollation, testAcute[i], testAcute[j], expected );
        }
    }
    ucol_close(myCollation);
}

static void TestExtra()
{
    int32_t i, j;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("fr_CA", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing fr_CA Collation extra with secondary strength\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 9 ; i++)
    {
        for (j = i + 1; j < 10; j += 1)
        {
            doTest(myCollation, testBugs[i], testBugs[j], UCOL_LESS);
        }
    }
    ucol_close(myCollation);
}

static void TestGetSortKey() {
    /* This is meant to test a buffer reallocation crash while using
    French secondary sorting with a large buffer.
    The fact that Japanese characters are used is irrelevant. */
    static const UChar pucUTF16[] = {
        0x3049,0x30b9,0x3088,0xfffd,0xfffd,0x308f,0xfffd,0x3042,
        0xfffd,0xfffd,0x305e,0xfffd,0x30b6,0x30bb,0x305b,0x30b1,
        0x3050,0x30af,0x304e,0x30bd,0xfffd,0x30c6,0xfffd,0xfffd,
        0x30e1,0xfffd,0xfffd,0x30d9,0xfffd,0x3092,0x3075,0x304a,
        0x3074,0x3070,0x30f5,0x30c4,0x306e,0x30df,0x3053,0xfffd,
        0x30a6,0x30b6,0x30e0,0xfffd,0x30bc,0x30ef,0x3087,0x30cc,
        0x305f,0x30de,0xfffd,0x3090,0x3063,0x30dc,0x30b6,0x30b9,
        0x30d2,0x3072,0x3061,0xfffd,0xfffd,0xfffd,0x307b,0x3092,
        0x30a5,0x30a9,0x30b1,0x30e7,0xfffd,0xfffd,0xfffd,0xfffd,
        0xfffd,0x305e,0xfffd,0x30c7,0x30ae,0x305b,0x308b,0x30c0,
        0x30f5,0xfffd,0xfffd,0xfffd,0x307d,0x304e,0xfffd,0xfffd,
        0x30c0,0x30c8,0x306f,0x307a,0x30dd,0x30e4,0x3084,0xfffd,
        0x308c,0x30f1,0xfffd,0x30c6,0xfffd,0x307a,0xfffd,0x3052,
        0x3056,0x305d,0x30b7,0xfffd,0x305b,0x30b0,0x30b9,0xfffd,
        0x30b2,0x306d,0x3044,0xfffd,0x3073,0xfffd,0x30be,0x30cf,
        0x3080,0xfffd,0x30a8,0x30f5,0x30a5,0x30c7,0x307c,0xfffd,
        0x30d1,0x305f,0x30b2,0xfffd,0x3053,0x30ca,0xfffd,0x30dd,
        0x3058,0x30c0,0x305d,0x30e1,0xfffd,0x30bb,0x305f,0x30d1,
        0x30f2,0x3058,0x3086,0x30ce,0x30db,0x30cb,0x30e9,0xfffd,
        0x308c,0xfffd,0xfffd,0x30af,0x30c4,0x3076,0x304c,0x30f5,
        0x30e8,0x308c,0xfffd,0x30e2,0x3073,0x30a3,0x304e,0x30ea,
        0xfffd,0x304f,0xfffd,0x306c,0x3044,0xfffd,0xfffd,0x30c9,
        0xfffd,0x30f5,0xfffd,0xfffd,0xfffd,0x30eb,0x30a8,0xfffd,
        0x306d,0x307d,0x30d8,0x3069,0xfffd,0xfffd,0x3086,0x30a9,
        0xfffd,0x3076,0x30e9,0x30cc,0x3074,0x30e0,0xfffd,0xfffd,
        0xfffd,0x30f0,0x3086,0x30ac,0x3076,0x3068,0x30c7,0xfffd,
        0x30b7,0x30d2,0x3048,0x308e,0x30e8,0x30d9,0x30ce,0x30d0,
        0x308b,0x30ee,0x30e6,0x3079,0x30f3,0x30af,0xfffd,0x3079,
        0xfffd,0xfffd,0x30ca,0x30bf,0xfffd,0x30b5,0xfffd,0xfffd,
        0x3093,0xfffd,0x30ba,0xfffd,0x3076,0x3047,0x304a,0xfffd,
        0xfffd,0x3086,0xfffd,0x3081,0xfffd,0x30f6,0x3066,0xfffd,
        0xfffd,0x30b6,0x30ef,0x30e2,0x30bf,0xfffd,0x3053,0x304a,
        0xfffd,0xfffd,0x304a,0x30e8,0xfffd,0x30e2,0xfffd,0xfffd,
        0x305c,0x3081,0x30c6,0xfffd,0x3091,0x3046,0x306a,0x3059,
        0xfffd,0xfffd,0x30dd,0x30d1,0x308a,0x30ee,0xfffd,0xfffd,
        0x308a,0x3042,0x30da,0xfffd,0x3064,0x30ef,0x305c,0x306b,
        0xfffd,0x30ca,0x3085,0x3067,0x30ea,0x30c2,0x30c8,0xfffd,
        0x30f5,0xfffd,0xfffd,0xfffd,0x30ca,0xfffd,0x3050,0x30f1,
        0x3050,0x3053,0x3072,0xfffd,0xfffd,0xfffd,0x3074,0xfffd,
        0x304b,0x30dd,0x306d,0xfffd,0x3049,0x30a1,0x30cc,0x30de,
        0x30ae,0x307b,0x308a,0xfffd,0x3065,0xfffd,0xfffd,0x30c0,
        0xfffd,0x3048,0x30dc,0x304f,0x3085,0x3059,0x304b,0x30d3,
        0x30eb,0x30a4,0x3073,0xfffd,0x30ba,0x308f,0x30a7,0x30c3,
        0x3074,0x30cf,0x306c,0x3053,0x30c0,0xfffd,0x3066,0xfffd,
        0x308f,0xfffd,0x30b5,0xfffd,0x3092,0x30c4,0xfffd,0x30d6,
        0x3056,0x30ad,0x30d2,0x30ba,0xfffd,0x30e6,0x304c,0x3088,
        0x30b6,0x3048,0x3077,0x30d1,0xfffd,0x3050,0xfffd,0x3042,
        0xfffd,0xfffd,0x308f,0xfffd,0x30c1,0xfffd,0x3074,0x3061,
        0x3056,0x30e5,0xfffd,0xfffd,0x3057,0xfffd,0xfffd,0xfffd,
        0xfffd,0x30bd,0x30b3,0x30ee,0xfffd,0x30f2,0x3084,0x3050,
        0xfffd,0x30e7,0xfffd,0xfffd,0x3060,0x3049,0x30f2,0x30ad,
        0x30bf,0x30f1,0x30a2,0xfffd,0x30af,0xfffd,0x3060,0x30a1,
        0x30e9,0x30c3,0xfffd,0x3072,0x3093,0x3070,0xfffd,0x308f,
        0x3060,0xfffd,0x3067,0x306f,0x3082,0x308b,0x3051,0xfffd,
        0x3058,0xfffd,0xfffd,0x30a8,0x3051,0x3054,0x30ad,0x30f0,
        0x3053,0xfffd,0x30e1,0x30d7,0x308d,0x307f,0x30be,0x30b0,
        0xfffd,0x30db,0xfffd,0x30d1,0xfffd,0x3054,0x30a5,0xfffd,
        0x306a,0xfffd,0x305c,0xfffd,0x3052,0x3088,0xfffd,0x306e,
        0xfffd,0x30a9,0x30a1,0x30b4,0x3083,0x30bd,0xfffd,0xfffd,
        0x306a,0x3070,0x30cd,0xfffd,0x3072,0x30ed,0x30c6,0x30be,
        0x30c4,0x305e,0x30b3,0x30e1,0x308a,0xfffd,0x305b,0xfffd,
        0x3042,0x3088,0xfffd,0x304c,0xfffd,0x3089,0x3071,0xfffd,
        0xfffd,0x30c6,0x3062,0x3079,0xfffd,0x304b,0x304a,0xfffd,
        0x30ad,0x3045,0x3045,0x3087,0xfffd,0x306a,0x308b,0x0000,
        0x30bd,0x3065,0x30b8,0x3086,0x30d3,0x3076,0xfffd,0xfffd,
        0x308f,0x3053,0x307c,0x3053,0x3084,0x30ae,0x30c4,0x3045,
        0x30a8,0x30d0,0x30e1,0x308c,0x30e6,0x30b7,0xfffd,0xfffd,
        0xfffd,0x3046,0x305f,0xfffd,0x3086,0x30ab,0xfffd,0xfffd,
        0x30c8,0xfffd,0x30a1,0x3052,0x3059,0xfffd,0x30a4,0xfffd,
        0xfffd,0x308c,0x3085,0x30ab,0x30b5,0x3091,0x30bf,0x30e3,
        0xfffd,0xfffd,0x3087,0xfffd,0x30f6,0x3051,0x30bd,0x3092,
        0x3063,0xfffd,0x30a9,0x3063,0x306e,0xfffd,0xfffd,0xfffd,
        0x306c,0xfffd,0x307e,0x30ad,0x3077,0x30c2,0x30e9,0x30d5,
        0xfffd,0xfffd,0x30c6,0x305c,0xfffd,0xfffd,0x3089,0xfffd,
        0x3048,0x30cb,0x308c,0xfffd,0xfffd,0x3044,0xfffd,0x3080,
        0x3063,0x3079,0xfffd,0x308a,0x30cb,0x3042,0x3057,0xfffd,
        0x307c,0x30c1,0x30a8,0x30cf,0xfffd,0x3083,0xfffd,0xfffd,
        0x306c,0xfffd,0x305e,0x3092,0xfffd,0x30dc,0x30b0,0x3081,
        0x30e3,0x30f0,0x304e,0x30cc,0x308e,0x30c4,0x30ad
    };

    UErrorCode status = U_ZERO_ERROR;
    UCollator *pCollator;
    int32_t lenActualSortKey;
    uint8_t pucSortKey[4096];
    static const int32_t LENSORTKEY = (int32_t)sizeof(pucSortKey);

    ucol_prepareShortStringOpen("LFR_AN_CX_EX_FO_HX_NX_S3", 0, NULL, &status);

    pCollator = ucol_openFromShortString("LFR_AN_CX_EX_FO_HX_NX_S3", 0, NULL, &status);

    if (U_FAILURE(status)) {
        log_data_err("error opening collator -> %s. (Are you missing data?)\n", u_errorName(status));
        return;
    }

    lenActualSortKey = ucol_getSortKey(pCollator,
        (const UChar *)pucUTF16,
        UPRV_LENGTHOF(pucUTF16),
        pucSortKey,
        LENSORTKEY);

    if (lenActualSortKey > LENSORTKEY) {
        log_err("sort key too big for original buffer. Got: %d Expected: %d\n", lenActualSortKey, LENSORTKEY);
        return;
    }
    /* If the test didn't crash, then the test succeeded. */
    ucol_close(pCollator);
}

#endif /* #if !UCONFIG_NO_COLLATION */
