/*
*******************************************************************************
* Copyright (C) 1997-1999, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File TXTBDRY.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/18/97    aliu        Converted from OpenClass.  Added DONE.
*****************************************************************************************
*/

// *****************************************************************************
// This file was generated from the java source file BreakIterator.java
// *****************************************************************************

#include "dbbi.h"
#include "unicode/brkiter.h"
#include "unicode/udata.h"
#include "resbund.h"

#include <string.h>

// *****************************************************************************
// class BreakIterator
// This class implements methods for finding the location of boundaries in text. 
// Instances of BreakIterator maintain a current position and scan over text
// returning the index of characters where boundaries occur.
// *****************************************************************************

const UTextOffset BreakIterator::DONE = (int32_t)-1;

// -------------------------------------

// Creates a simple text boundary for word breaks.
BreakIterator*
BreakIterator::createWordInstance(const Locale& key)
{
    // WARNING: This routine is currently written specifically to handle only the
    // default rules files and the alternate rules files for Thai.  This function
    // will have to be made fully general at some time in the future!
    BreakIterator* result = NULL;
    const char* filename = "word";

    UnicodeString temp;
    if (key.getLanguage(temp) == UnicodeString("th", (char*)0)) {
        filename = "word_th";
    }

    UErrorCode err = U_ZERO_ERROR;
    UDataMemory* file = udata_open(NULL, "brk", filename, &err);

    if (!U_FAILURE(err)) {
        const void* image = udata_getMemory(file);

        if (image != NULL) {
            if (key.getLanguage(temp) == UnicodeString("th", (char*)0)) {
                const char* dataDir = u_getDataDirectory();
                filename = "thaidict.brk";
                char* fullPath = new char[strlen(dataDir) + strlen(filename) + 1];
                strcpy(fullPath, dataDir);
                strcpy(fullPath, filename);
                
                result = new DictionaryBasedBreakIterator(image, fullPath);
                delete [] fullPath;
            }
            else {
                result = new RuleBasedBreakIterator(image);
            }
        }
    }
    
    return result;
}

// -------------------------------------

// Creates a simple text boundary for line breaks.
BreakIterator*
BreakIterator::createLineInstance(const Locale& key)
{
    // WARNING: This routine is currently written specifically to handle only the
    // default rules files and the alternate rules files for Thai.  This function
    // will have to be made fully general at some time in the future!
    BreakIterator* result = NULL;
    const char* filename = "line";

    UnicodeString temp;
    if (key.getLanguage(temp) == UnicodeString("th", (char*)0)) {
        filename = "line_th";
    }

    UErrorCode err = U_ZERO_ERROR;
    UDataMemory* file = udata_open(NULL, "brk", filename, &err);

    if (!U_FAILURE(err)) {
        const void* image = udata_getMemory(file);

        if (image != NULL) {
            if (key.getLanguage(temp) == UnicodeString("th", (char*)0)) {
                const char* dataDir = u_getDataDirectory();
                filename = "thaidict.brk";
                char* fullPath = new char[strlen(dataDir) + strlen(filename) + 1];
                strcpy(fullPath, dataDir);
                strcat(fullPath, filename);
                
                result = new DictionaryBasedBreakIterator(image, fullPath);
                delete [] fullPath;
            }
            else {
                result = new RuleBasedBreakIterator(image);
            }
        }
    }
    
    return result;
}

// -------------------------------------

// Creates a simple text boundary for character breaks.
BreakIterator*
BreakIterator::createCharacterInstance(const Locale& key)
{
    // WARNING: This routine is currently written specifically to handle only the
    // default rules files and the alternate rules files for Thai.  This function
    // will have to be made fully general at some time in the future!
    BreakIterator* result = NULL;
    const char* filename = "char";

    UErrorCode err = U_ZERO_ERROR;
    UDataMemory* file = udata_open(NULL, "brk", filename, &err);

    if (!U_FAILURE(err)) {
        const void* image = udata_getMemory(file);

        if (image != NULL) {
            result = new RuleBasedBreakIterator(image);
        }
    }
    
    return result;
}

// -------------------------------------

// Creates a simple text boundary for sentence breaks.
BreakIterator*
BreakIterator::createSentenceInstance(const Locale& key)
{
    // WARNING: This routine is currently written specifically to handle only the
    // default rules files and the alternate rules files for Thai.  This function
    // will have to be made fully general at some time in the future!
    BreakIterator* result = NULL;
    const char* filename = "sent";

    UErrorCode err = U_ZERO_ERROR;
    UDataMemory* file = udata_open(NULL, "brk", filename, &err);

    if (!U_FAILURE(err)) {
        const void* image = udata_getMemory(file);

        if (image != NULL) {
            result = new RuleBasedBreakIterator(image);
        }
    }
    
    return result;
}

// -------------------------------------

// Gets all the available locales that has localized text boundary data.
const Locale*
BreakIterator::getAvailableLocales(int32_t& count)
{
    return Locale::getAvailableLocales(count);
}

// -------------------------------------
// Gets the objectLocale display name in the default locale language.
UnicodeString&
BreakIterator::getDisplayName(const Locale& objectLocale,
                             UnicodeString& name)
{
    return objectLocale.getDisplayName(name);
}

// -------------------------------------
// Gets the objectLocale display name in the displayLocale language.
UnicodeString&
BreakIterator::getDisplayName(const Locale& objectLocale,
                             const Locale& displayLocale,
                             UnicodeString& name)
{
    return objectLocale.getDisplayName(displayLocale, name);
}

// -------------------------------------

// Needed because we declare the copy constructor (in order to prevent synthesizing one) and
// so the default constructor is no longer synthesized.

BreakIterator::BreakIterator()
{
}

BreakIterator::~BreakIterator()
{
}

//eof
