/*
*******************************************************************************
* Copyright (C) 2010-2012, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* utf16collationiterator.h
*
* created on: 2010oct27
* created by: Markus W. Scherer
*/

#ifndef __UTF16COLLATIONITERATOR_H__
#define __UTF16COLLATIONITERATOR_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "normalizer2impl.h"

U_NAMESPACE_BEGIN

/**
 * UTF-16 collation element and character iterator.
 * Handles normalized UTF-16 text inline, with length or NUL-terminated.
 * Unnormalized text is handled by a subclass.
 */
class U_I18N_API UTF16CollationIterator : public CollationIterator {
public:
    UTF16CollationIterator(const CollationData *d, int8_t iterFlags,
                           const UChar *s, const UChar *lim)
            // Optimization: Skip initialization of fields that are not used
            // until they are set together with other state changes.
            : CollationIterator(d, iterFlags),
              start(s), pos(s), limit(lim) {}

    virtual ~UTF16CollationIterator();

    // TODO: setText(start, pos, limit)  ?

protected:
    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    virtual UBool foundNULTerminator();

    virtual UChar32 nextCodePoint(UErrorCode &errorCode);

    virtual UChar32 previousCodePoint(UErrorCode &errorCode);

    /**
     * Returns the next code point, or <0 if none, assuming pos==limit.
     * Post-increment semantics.
     */
    virtual UChar32 handleNextCodePoint(UErrorCode &errorCode);

    /**
     * Returns the previous code point, or <0 if none, assuming pos==start.
     * Pre-decrement semantics.
     */
    virtual UChar32 handlePreviousCodePoint(UErrorCode &errorCode);

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual const void *saveLimitAndSetAfter(UChar32 c);

    virtual void restoreLimit(const void *savedLimit);

    // UTF-16 string pointers.
    // limit can be NULL for NUL-terminated strings.
    // This class assumes that whole code points are stored within [start..limit[.
    // That is, a trail surrogate at start or a lead surrogate at limit-1
    // will be assumed to be surrogate code points rather than attempting to pair it
    // with a surrogate retrieved from the subclass.
    const UChar *start, *pos, *limit;
    // TODO: getter for limit, so that caller can find out length of NUL-terminated text?
};

/**
 * Checks the input text for FCD, passes already-FCD segments into the base iterator,
 * and normalizes other segments on the fly.
 */
class U_I18N_API FCDUTF16CollationIterator : public UTF16CollationIterator {
public:
    FCDUTF16CollationIterator(const CollationData *data, int8_t iterFlags,
                              const UChar *s, const UChar *lim,
                              UErrorCode &errorCode);

    inline void setSmallSteps(UBool small) { smallSteps = small; }

protected:
    virtual UChar32 handleNextCodePoint(UErrorCode &errorCode);

    inline UChar32 simpleNext() {
        UChar32 c = *pos++;
        UChar trail;
        if(U16_IS_LEAD(c) && pos != limit && U16_IS_TRAIL(trail = *pos)) {
            ++pos;
            return U16_GET_SUPPLEMENTARY(c, trail);
        } else {
            return c;
        }
    }

    UChar32 nextCodePointDecompHangul(UErrorCode &errorCode);

    virtual UChar32 handlePreviousCodePoint(UErrorCode &errorCode);

    inline UChar32 simplePrevious() {
        UChar32 c = *--pos;
        UChar lead;
        if(U16_IS_TRAIL(c) && pos != start && U16_IS_LEAD(lead = *(pos - 1))) {
            --pos;
            return U16_GET_SUPPLEMENTARY(lead, c);
        } else {
            return c;
        }
    }

    UChar32 previousCodePointDecompHangul(UErrorCode &errorCode);

    virtual const void *saveLimitAndSetAfter(UChar32 c);

    virtual void restoreLimit(const void *savedLimit);

private:
    // Text pointers: The input text is [rawStart, rawLimit[
    // where rawLimit can be NULL for NUL-terminated text.
    // segmentStart and segmentLimit point into the text and indicate
    // the start and exclusive end of the text segment currently being processed.
    // They are at FCD boundaries.
    // Either the current text segment already passes the FCD test
    // and segmentStart==start<=pos<=limit==segmentLimit,
    // or the current segment had to be normalized so that
    // [segmentStart, segmentLimit[ turned into the normalized string,
    // corresponding to buffer.getStart()==start<=pos<=limit==buffer.getLimit().
    const UChar *rawStart;
    const UChar *segmentStart;
    const UChar *segmentLimit;
    // rawLimit==NULL for a NUL-terminated string.
    const UChar *rawLimit;
    // Normally zero.
    // Between calls to saveLimitAndSetAfter() and restoreLimit(),
    // it tracks the positive number of normalized UChars
    // between the start pointer and the temporary iteration limit.
    int32_t lengthBeforeLimit;
    // We make small steps for string comparisons and larger steps for sort key generation.
    UBool smallSteps;

    const Normalizer2Impl &nfcImpl;
    UnicodeString normalized;
    ReorderingBuffer buffer;
};

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
#endif  // __UTF16COLLATIONITERATOR_H__
