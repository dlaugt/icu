/*
 * @(#)StateTableProcessor.cpp	1.6 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "StateTables.h"
#include "MorphStateTables.h"
#include "SubtableProcessor.h"
#include "StateTableProcessor.h"
#include "LESwaps.h"

StateTableProcessor::StateTableProcessor()
{
}

StateTableProcessor::StateTableProcessor(MorphSubtableHeader *morphSubtableHeader)
  : SubtableProcessor(morphSubtableHeader)
{
    stateTableHeader = (MorphStateTableHeader *) morphSubtableHeader;

    stateSize = SWAPW(stateTableHeader->stHeader.stateSize);
    classTableOffset = SWAPW(stateTableHeader->stHeader.classTableOffset);
    stateArrayOffset = SWAPW(stateTableHeader->stHeader.stateArrayOffset);
    entryTableOffset = SWAPW(stateTableHeader->stHeader.entryTableOffset);

    classTable = (ClassTable *) ((char *) &stateTableHeader->stHeader + classTableOffset);
    firstGlyph = SWAPW(classTable->firstGlyph);
    lastGlyph = firstGlyph + SWAPW(classTable->nGlyphs);
}

StateTableProcessor::~StateTableProcessor()
{
}

void StateTableProcessor::process(LEGlyphID *glyphs, le_int32 *charIndices, le_int32 glyphCount)
{
    // Start at state 0
    // XXX: How do we know when to start at state 1?
    ByteOffset currentState = stateArrayOffset;

    // XXX: reverse? 
    le_int32 currGlyph = 0;

    beginStateTable();

    while (currGlyph <= glyphCount)
    {
        ClassCode classCode = classCodeOOB;
        if (currGlyph == glyphCount)
        {
            // XXX: How do we handle EOT vs. EOL?
            classCode = classCodeEOT;
        }
        else
        {
            LEGlyphID glyphCode = glyphs[currGlyph];

            if (glyphCode == 0xFFFF)
            {
                classCode = classCodeDEL;
            }
            else if ((glyphCode >= firstGlyph) && (glyphCode < lastGlyph))
            {
                classCode = classTable->classArray[glyphCode - firstGlyph];
            }
        }

        EntryTableIndex *stateArray = (EntryTableIndex *) ((char *) &stateTableHeader->stHeader + currentState);
        EntryTableIndex entryTableIndex = stateArray[(le_uint8)classCode];

        currentState = processStateEntry(glyphs, charIndices, currGlyph, glyphCount, entryTableIndex);
    }

    endStateTable();
}

