/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-1999, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef _UTYPES
#include "unicode/utypes.h"
#endif

#ifndef _COLL
#include "unicode/coll.h"
#endif

#ifndef _INTLTEST
#include "intltest.h"
#endif

class LotusCollationKoreanTest: public IntlTest {
public:
    // static constants
    enum EToken_Len { MAX_TOKEN_LEN = 128 };

    LotusCollationKoreanTest();
    virtual ~LotusCollationKoreanTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // main test routine, tests rules specific to "Kana" locale
    void doTest( UnicodeString source, UnicodeString target, Collator::EComparisonResult result);

    // performs test with strength TERIARY
    void TestTertiary(/* char* par */);

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];

    Collator *myCollation;
};
