/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-1999, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/***********************************************************************
* Modification history
* Date        Name        Description
* 02/14/2001  synwee      Added attributes in TestTertiary and 
*                         TestSecondary
***********************************************************************/

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

#ifndef _FRCOLL
#include "frcoll.h"
#endif

#include "sfwdchit.h"

CollationFrenchTest::CollationFrenchTest()
: myCollation(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollation = Collator::createInstance(Locale::FRANCE, status);
    if(!myCollation || U_FAILURE(status)) {
      errln(__FILE__ "failed to create! err " + UnicodeString(u_errorName(status)));
	/* if it wasn't already: */
	delete myCollation;
	myCollation = NULL;
    }

}

CollationFrenchTest::~CollationFrenchTest()
{
    delete myCollation;
}

const UChar CollationFrenchTest::testSourceCases[][CollationFrenchTest::MAX_TOKEN_LEN] =
{
    {0x61, 0x62, 0x63, 0},
    {0x43, 0x4f, 0x54, 0x45, 0},
    {0x63, 0x6f, 0x2d, 0x6f, 0x70, 0},
    {0x70, 0x00EA, 0x63, 0x68, 0x65, 0},
    {0x70, 0x00EA, 0x63, 0x68, 0x65, 0x72, 0},
    {0x70, 0x00E9, 0x63, 0x68, 0x65, 0x72, 0},
    {0x70, 0x00E9, 0x63, 0x68, 0x65, 0x72, 0},
    {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0},
    {0x01f1, 0},
    {0xfb00, 0},
    {0x01fa, 0},
    {0x0101, 0}
};

const UChar CollationFrenchTest::testTargetCases[][CollationFrenchTest::MAX_TOKEN_LEN] =
{
    {0x41, 0x42, 0x43, 0},
    {0x63, 0x00f4, 0x74, 0x65, 0},
    {0x43, 0x4f, 0x4f, 0x50, 0},
    {0x70, 0x00E9, 0x63, 0x68, 0x00E9, 0},
    {0x70, 0x00E9, 0x63, 0x68, 0x00E9, 0},
    {0x70, 0x00EA, 0x63, 0x68, 0x65, 0},
    {0x70, 0x00EA, 0x63, 0x68, 0x65, 0x72, 0},
    {0x68, 0x65, 0x6c, 0x6c, 0x4f, 0},
    {0x01ee, 0},
    {0x25ca, 0},
    {0x00e0, 0},
    {0x01df, 0}
};

const Collator::EComparisonResult CollationFrenchTest::results[] =
{
    Collator::LESS,
    Collator::LESS,
    Collator::GREATER,
    Collator::LESS,
    Collator::GREATER,
    Collator::GREATER,
    Collator::LESS,
    Collator::GREATER,
    Collator::GREATER,
    Collator::GREATER,
    Collator::LESS,
    Collator::LESS
};

// 0x0300 is grave, 0x0301 is acute
// the order of elements in this array must be different than the order in CollationEnglishTest
const UChar CollationFrenchTest::testAcute[][CollationFrenchTest::MAX_TOKEN_LEN] =
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
#if 0
/*00*/    {0x65, 0x65, 0},
/*01*/    {0x65, 0x0301, 0x65, 0},
/*02*/    {0x65, 0x0301, 0x0300, 0x65, 0},
/*03*/    {0x65, 0x0300, 0x65, 0},
/*04*/    {0x65, 0x0300, 0x0301, 0x65, 0},
/*05*/    {0x65, 0x65, 0x0301, 0},
/*06*/    {0x65, 0x0301, 0x65, 0x0301, 0},
/*07*/    {0x65, 0x0301, 0x0300, 0x65, 0x0301, 0},
/*08*/    {0x65, 0x0300, 0x65, 0x0301, 0},
/*09*/    {0x65, 0x0300, 0x0301, 0x65, 0x0301, 0},
/*0a*/    {0x65, 0x65, 0x0301, 0x0300, 0},
/*0b*/    {0x65, 0x0301, 0x65, 0x0301, 0x0300, 0},
/*0c*/    {0x65, 0x0301, 0x0300, 0x65, 0x0301, 0x0300, 0},
/*0d*/    {0x65, 0x0300, 0x65, 0x0301, 0x0300, 0},
/*0e*/    {0x65, 0x0300, 0x0301, 0x65, 0x0301, 0x0300, 0},
/*0f*/    {0x65, 0x65, 0x0300, 0},
/*10*/    {0x65, 0x0301, 0x65, 0x0300, 0},
/*11*/    {0x65, 0x0301, 0x0300, 0x65, 0x0300, 0},
/*12*/    {0x65, 0x0300, 0x65, 0x0300, 0},
/*13*/    {0x65, 0x0300, 0x0301, 0x65, 0x0300, 0},
/*14*/    {0x65, 0x65, 0x0300, 0x0301, 0},
/*15*/    {0x65, 0x0301, 0x65, 0x0300, 0x0301, 0},
/*16*/    {0x65, 0x0301, 0x0300, 0x65, 0x0300, 0x0301, 0},
/*17*/    {0x65, 0x0300, 0x65, 0x0300, 0x0301, 0},
/*18*/    {0x65, 0x0300, 0x0301, 0x65, 0x0300, 0x0301, 0}
#endif

};

const UChar CollationFrenchTest::testBugs[][CollationFrenchTest::MAX_TOKEN_LEN] =
{
    {0x61, 0},
    {0x41, 0},
    {0x65, 0},
    {0x45, 0},
    {0x00e9, 0},
    {0x00e8, 0},
    {0x00ea, 0},
    {0x00eb, 0},
    {0x65, 0x61, 0},
    {0x78, 0}
};

void CollationFrenchTest::doTest( UnicodeString source, UnicodeString target, Collator::EComparisonResult result)
{
    Collator::EComparisonResult compareResult = myCollation->compare(source, target);
    SimpleFwdCharIterator src(source);
    SimpleFwdCharIterator trg(target);
    Collator::EComparisonResult incResult = myCollation->compare(src, trg);
    CollationKey sortKey1, sortKey2;
    UErrorCode key1status = U_ZERO_ERROR, key2status = U_ZERO_ERROR; //nos
    myCollation->getCollationKey(source, /*nos*/ sortKey1, key1status );
    myCollation->getCollationKey(target, /*nos*/ sortKey2, key2status );
    if (U_FAILURE(key1status) || U_FAILURE(key2status))
    {
        errln("SortKey generation Failed.\n");
        return;
    }

    Collator::EComparisonResult keyResult = sortKey1.compareTo(sortKey2);
    reportCResult( source, target, sortKey1, sortKey2, compareResult, keyResult, incResult, result );
}

void CollationFrenchTest::TestTertiary(/* char* par */)
{
    int32_t i = 0;
    UErrorCode status = U_ZERO_ERROR;
    myCollation->setStrength(Collator::TERTIARY);
    myCollation->setAttribute(UCOL_FRENCH_COLLATION, UCOL_ON, status);
    myCollation->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
    if (U_FAILURE(status))
      errln("Error setting attribute in French collator");
    else
      for (i = 0; i < 12 ; i++)
      {
          doTest(testSourceCases[i], testTargetCases[i], results[i]);
      }
}

void CollationFrenchTest::TestSecondary(/* char* par */)
{
    //test acute and grave ordering
    int32_t i = 0;
    int32_t j;
    Collator::EComparisonResult expected;
    UErrorCode status = U_ZERO_ERROR;
    myCollation->setAttribute(UCOL_FRENCH_COLLATION, UCOL_ON, status);
    if (U_FAILURE(status))
      errln("Error setting attribute in French collator");
    else
    {
      const int32_t testAcuteSize = sizeof(testAcute) / sizeof(testAcute[0]);
      for (i = 0; i < testAcuteSize; i++)
      {
          for (j = 0; j < testAcuteSize; j++)
          {
              if (i <  j)
                  expected = Collator::LESS;
              else if (i == j)
                  expected = Collator::EQUAL;
              else // (i >  j)
                  expected = Collator::GREATER;
              doTest(testAcute[i], testAcute[j], expected );
          }
      }
    }
}

void CollationFrenchTest::TestExtra(/* char* par */)
{
    int32_t i, j;
    myCollation->setStrength(Collator::TERTIARY);
    for (i = 0; i < 9 ; i++)
    {
        for (j = i + 1; j < 10; j += 1)
        {
            doTest(testBugs[i], testBugs[j], Collator::LESS);
        }
    }
}

void CollationFrenchTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CollationFrenchTest: ");

    if((!myCollation) && exec) {
      errln(__FILE__ " cannot test - failed to create collator.");
      name = "";
      return;
    }

    switch (index) {
        case 0: name = "TestSecondary"; if (exec)   TestSecondary(/* par */); break;
        case 1: name = "TestTertiary";  if (exec)   TestTertiary(/* par */); break;
        case 2: name = "TestExtra";     if (exec)   TestExtra(/* par */); break;
        default: name = ""; break;
    }
}


