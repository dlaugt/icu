/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#ifndef _COLL
#include "unicode/coll.h"
#endif

#ifndef _TBLCOLL
#include "unicode/tblcoll.h"
#endif

#ifndef _UNISTR
#include "unicode/unistr.h"
#endif

#ifndef _SORTKEY
#include "unicode/sortkey.h"
#endif

#ifndef _TRCOLL
#include "trcoll.h"
#endif

#include "sfwdchit.h"

CollationTurkishTest::CollationTurkishTest()
: myCollation(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollation = Collator::createInstance(Locale("tr", "", ""),status);
}

CollationTurkishTest::~CollationTurkishTest()
{
    delete myCollation;
}

const UChar CollationTurkishTest::testSourceCases[][CollationTurkishTest::MAX_TOKEN_LEN] = {
    {0x73, 0x0327, 0},
    {0x76, 0x00E4, 0x74, 0},
    {0x6f, 0x6c, 0x64, 0},
    {0x00FC, 0x6f, 0x69, 0x64, 0},
    {0x68, 0x011E, 0x61, 0x6c, 0x74, 0},
    {0x73, 0x74, 0x72, 0x65, 0x73, 0x015E, 0},
    {0x76, 0x6f, 0x0131, 0x64, 0},
    {0x69, 0x64, 0x65, 0x61, 0},
    {0x00FC, 0x6f, 0x69, 0x64, 0},
    {0x76, 0x6f, 0x0131, 0x64, 0},
    {0x69, 0x64, 0x65, 0x61, 0}
};

const UChar CollationTurkishTest::testTargetCases[][CollationTurkishTest::MAX_TOKEN_LEN] = {
    {0x75, 0x0308, 0},
    {0x76, 0x62, 0x74, 0},
    {0x00D6, 0x61, 0x79, 0},
    {0x76, 0x6f, 0x69, 0x64, 0},
    {0x68, 0x61, 0x6c, 0x74, 0},
    {0x015E, 0x74, 0x72, 0x65, 0x015E, 0x73, 0},
    {0x76, 0x6f, 0x69, 0x64, 0},
    {0x49, 0x64, 0x65, 0x61, 0},
    {0x76, 0x6f, 0x69, 0x64, 0},
    {0x76, 0x6f, 0x69, 0x64, 0},
    {0x49, 0x64, 0x65, 0x61, 0}
};

const UCollationResult CollationTurkishTest::results[] = {
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    // test priamry > 8
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER
};

void CollationTurkishTest::TestTertiary(/* char* par */)
{
    int32_t i = 0;
    myCollation->setStrength(Collator::TERTIARY);
    for (i = 0; i < 8 ; i++) {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}
void CollationTurkishTest::TestPrimary(/* char* par */)
{
    int32_t i;
    myCollation->setStrength(Collator::PRIMARY);
    for (i = 8; i < 11; i++) {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}

void CollationTurkishTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CollationTurkishTest: ");

    if((!myCollation) && exec) {
      errln(__FILE__ " cannot test - failed to create collator.");
      name = "";
      return;
    }
    switch (index) {
        case 0: name = "TestPrimary";   if (exec)   TestPrimary(/* par */); break;
        case 1: name = "TestTertiary";  if (exec)   TestTertiary(/* par */); break;
        default: name = ""; break;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
