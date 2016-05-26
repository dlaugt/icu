/*
 *
 * Copyright (C) 2016 and later: Unicode, Inc. and others. License & terms of use: http://www.unicode.org/copyright.html
 *
 */

#ifndef __SUBSTITUTIONLOOKUPS_H
#define __SUBSTITUTIONLOOKUPS_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "GlyphIterator.h"
#include "LookupProcessor.h"

U_NAMESPACE_BEGIN

struct SubstitutionLookupRecord
{
    le_uint16  sequenceIndex;
    le_uint16  lookupListIndex;
};

struct SubstitutionLookup
{
    static void applySubstitutionLookups(
        LookupProcessor *lookupProcessor, 
        SubstitutionLookupRecord *substLookupRecordArray,
        le_uint16 substCount,
        GlyphIterator *glyphIterator,
        const LEFontInstance *fontInstance,
        le_int32 position,
		LEErrorCode& success);
};

U_NAMESPACE_END
#endif

