/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef CONTRACTIONTABLETEST_H
#define CONTRACTIONTABLETEST_H

#include "tscoll.h"
#include "ucol_cnt.h"
#include "utrie.h"

class ContractionTableTest: public IntlTestCollator {
public:
    ContractionTableTest();
    virtual ~ContractionTableTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // performs test with strength TERIARY
    void TestGrowTable(/* char* par */);
    void TestSetContraction();
    void TestAddATableElement();
    void TestClone();
    void TestChangeContraction();
    void TestChangeLastCE();
private:
    CntTable *testTable, *testClone;
    /*CompactEIntArray *testMapping;*/
    UNewTrie *testMapping;
    UErrorCode status;
};
#endif
