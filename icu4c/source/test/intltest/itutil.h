/********************************************************************
 * COPYRIGHT: 
 * Copyright (C) 2016 and later: Unicode, Inc. and others.
 * License & terms of use: http://www.unicode.org/copyright.html
 ********************************************************************/

/**
 * MajorTestLevel is the top level test class for everything in the directory "IntlWork".
 */

#ifndef _INTLTESTUTILITIES
#define _INTLTESTUTILITIES

#include "intltest.h"

class IntlTestUtilities: public IntlTest {
public:
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
};

class ErrorCodeTest: public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL);
    void TestErrorCode();
    void TestSubclass();
};

#endif
