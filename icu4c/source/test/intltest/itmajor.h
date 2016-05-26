/********************************************************************
 * COPYRIGHT: 
 * Copyright (C) 2016 and later: Unicode, Inc. and others.
 * License & terms of use: http://www.unicode.org/copyright.html
 ********************************************************************/

/**
 * MajorTestLevel is the top level test class for everything in the directory "IntlWork".
 */

#ifndef _INTLTESTMAJOR
#define _INTLTESTMAJOR


#include "intltest.h"


class MajorTestLevel: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
};

class IntlTestNormalize: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
};

#endif
