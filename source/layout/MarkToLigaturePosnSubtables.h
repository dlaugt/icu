/*
 * @(#)MarkToLigaturePosnSubtables.h	1.5 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000, 2001 - All Rights Reserved
 *
 */

#ifndef __MARKTOLIGATUREPOSITIONINGSUBTABLES_H
#define __MARKTOLIGATUREPOSITIONINGSUBTABLES_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphPositioningTables.h"
#include "AttachmentPosnSubtables.h"
#include "GlyphIterator.h"

U_NAMESPACE_BEGIN

struct MarkToLigaturePositioningSubtable : AttachmentPositioningSubtable
{
    le_int32   process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;
    LEGlyphID  findLigatureGlyph(GlyphIterator *glyphIterator) const;
};

struct ComponentRecord
{
    Offset ligatureAnchorTableOffsetArray[ANY_NUMBER];
};

struct LigatureAttachTable
{
    le_uint16 componentCount;
    ComponentRecord componentRecordArray[ANY_NUMBER];
};

struct LigatureArray
{
    le_uint16 ligatureCount;
    Offset ligatureAttachTableOffsetArray[ANY_NUMBER];
};

U_NAMESPACE_END
#endif

