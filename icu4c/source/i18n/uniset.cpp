/*
**********************************************************************
*   Copyright (C) 1999-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   10/20/99    alan        Creation.
**********************************************************************
*/

#include "unicode/uniset.h"
#include "unicode/parsepos.h"
#include "unicode/unicode.h"
#include "unicode/uscript.h"
#include "symtable.h"
#include "cmemory.h"
#include "rbt_rule.h"
#include "umutex.h"
#include "ucln_in.h"
#include "upropset.h"

// HIGH_VALUE > all valid values. 110000 for codepoints
#define UNICODESET_HIGH 0x0110000

// LOW <= all valid values. ZERO for codepoints
#define UNICODESET_LOW 0x000000

// initial storage. Must be >= 0
#define START_EXTRA 16

// extra amount for growth. Must be >= 0
#define GROW_EXTRA START_EXTRA

// Define UChar constants using hex for EBCDIC compatibility
// Used #define to reduce private static exports and memory access time.
#define SET_OPEN        ((UChar)0x005B) /*[*/
#define SET_CLOSE       ((UChar)0x005D) /*]*/
#define HYPHEN          ((UChar)0x002D) /*-*/
#define COMPLEMENT      ((UChar)0x005E) /*^*/
#define COLON           ((UChar)0x003A) /*:*/
#define BACKSLASH       ((UChar)0x005C) /*\*/
#define INTERSECTION    ((UChar)0x0026) /*&*/
#define UPPER_U         ((UChar)0x0055) /*U*/
#define LOWER_U         ((UChar)0x0075) /*u*/

// TEMPORARY: Remove when deprecated category code constructor is removed.
static const UChar CATEGORY_NAMES[] = {
    // Must be kept in sync with uchar.h/UCharCategory
    0x43, 0x6E, /* "Cn" */
    0x4C, 0x75, /* "Lu" */
    0x4C, 0x6C, /* "Ll" */
    0x4C, 0x74, /* "Lt" */
    0x4C, 0x6D, /* "Lm" */
    0x4C, 0x6F, /* "Lo" */
    0x4D, 0x6E, /* "Mn" */
    0x4D, 0x65, /* "Me" */
    0x4D, 0x63, /* "Mc" */
    0x4E, 0x64, /* "Nd" */
    0x4E, 0x6C, /* "Nl" */
    0x4E, 0x6F, /* "No" */
    0x5A, 0x73, /* "Zs" */
    0x5A, 0x6C, /* "Zl" */
    0x5A, 0x70, /* "Zp" */
    0x43, 0x63, /* "Cc" */
    0x43, 0x66, /* "Cf" */
    0x43, 0x6F, /* "Co" */
    0x43, 0x73, /* "Cs" */
    0x50, 0x64, /* "Pd" */
    0x50, 0x73, /* "Ps" */
    0x50, 0x65, /* "Pe" */
    0x50, 0x63, /* "Pc" */
    0x50, 0x6F, /* "Po" */
    0x53, 0x6D, /* "Sm" */
    0x53, 0x63, /* "Sc" */
    0x53, 0x6B, /* "Sk" */
    0x53, 0x6F, /* "So" */
    0x50, 0x69, /* "Pi" */
    0x50, 0x66, /* "Pf" */
    0x00
};

/**
 * Delimiter string used in patterns to close a category reference:
 * ":]".  Example: "[:Lu:]".
 */
static const UChar CATEGORY_CLOSE[] = {COLON, SET_CLOSE, 0x0000}; /* ":]" */

/**
 * Cleanup function for transliterator component; delegates to
 * Transliterator::cleanupRegistry().
 */
U_CFUNC UBool unicodeset_cleanup(void) {
    UnicodePropertySet::cleanup();
    return TRUE;
}

U_NAMESPACE_BEGIN

/**
 * Minimum value that can be stored in a UnicodeSet.
 */
const UChar32 UnicodeSet::MIN_VALUE = UNICODESET_LOW;

/**
 * Maximum value that can be stored in a UnicodeSet.
 */
const UChar32 UnicodeSet::MAX_VALUE = UNICODESET_HIGH - 1;

//----------------------------------------------------------------
// Constructors &c
//----------------------------------------------------------------

/**
 * Constructs an empty set.
 */
UnicodeSet::UnicodeSet() :
    len(1), capacity(1 + START_EXTRA), bufferCapacity(0),
    buffer(0)
{
    list = new UChar32[capacity];
    list[0] = UNICODESET_HIGH;
}

/**
 * Constructs a set containing the given range. If <code>end >
 * start</code> then an empty set is created.
 *
 * @param start first character, inclusive, of range
 * @param end last character, inclusive, of range
 */
UnicodeSet::UnicodeSet(UChar32 start, UChar32 end) :
    len(1), capacity(1 + START_EXTRA), bufferCapacity(0),
    buffer(0)
{
    list = new UChar32[capacity];
    list[0] = UNICODESET_HIGH;
    complement(start, end);
}

/**
 * Constructs a set from the given pattern, optionally ignoring
 * white space.  See the class description for the syntax of the
 * pattern language.
 * @param pattern a string specifying what characters are in the set
 */
UnicodeSet::UnicodeSet(const UnicodeString& pattern,
                       UErrorCode& status) :
    len(0), capacity(START_EXTRA), bufferCapacity(0),
    buffer(0)
{
    list = new UChar32[capacity];
    applyPattern(pattern, status);
}

// For internal use by RuleBasedTransliterator
UnicodeSet::UnicodeSet(const UnicodeString& pattern, ParsePosition& pos,
                       const SymbolTable& symbols,
                       UErrorCode& status) :
    len(0), capacity(START_EXTRA), bufferCapacity(0),
    buffer(0)
{
    list = new UChar32[capacity];
    applyPattern(pattern, pos, &symbols, status);
}

/**
 * DEPRECATED Constructs a set from the given Unicode character category.
 * @param category an integer indicating the character category as
 * defined in uchar.h.
 * @deprecated To be removed after 2002-DEC-31
 */
UnicodeSet::UnicodeSet(int8_t category, UErrorCode& status) :
    len(0), capacity(START_EXTRA), bufferCapacity(0), list(0),
    buffer(0)
{
    static const UChar OPEN[] = { 91, 58, 0 }; // "[:"
    static const UChar CLOSE[]= { 58, 93, 0 }; // ":]"
    if (U_SUCCESS(status)) {
        if (category < 0 || category >= U_CHAR_CATEGORY_COUNT) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            UnicodeString pat(FALSE, CATEGORY_NAMES + category*2, 2);
            pat.insert(0, OPEN);
            pat.append(CLOSE);
            list = new UChar32[capacity];
            applyPattern(pat, status);
        }
    }
}

/**
 * Constructs a set that is identical to the given UnicodeSet.
 */
UnicodeSet::UnicodeSet(const UnicodeSet& o) :
    UnicodeFilter(o),
    capacity(o.len + GROW_EXTRA), bufferCapacity(0),
    buffer(0)
{
    list = new UChar32[capacity];
    *this = o;
}

/**
 * Destructs the set.
 */
UnicodeSet::~UnicodeSet() {
    delete[] list;
    delete[] buffer;
}

/**
 * Assigns this object to be a copy of another.
 */
UnicodeSet& UnicodeSet::operator=(const UnicodeSet& o) {
    ensureCapacity(o.len);
    len = o.len;
    uprv_memcpy(list, o.list, len*sizeof(UChar32));
    pat = o.pat;
    return *this;
}

/**
 * Compares the specified object with this set for equality.  Returns
 * <tt>true</tt> if the two sets
 * have the same size, and every member of the specified set is
 * contained in this set (or equivalently, every member of this set is
 * contained in the specified set).
 *
 * @param o set to be compared for equality with this set.
 * @return <tt>true</tt> if the specified set is equal to this set.
 */
UBool UnicodeSet::operator==(const UnicodeSet& o) const {
    if (len != o.len) return FALSE;
    for (int32_t i = 0; i < len; ++i) {
        if (list[i] != o.list[i]) return FALSE;
    }
    return TRUE;
}

/**
 * Returns a copy of this object.  All UnicodeMatcher objects have
 * to support cloning in order to allow classes using
 * UnicodeMatchers, such as Transliterator, to implement cloning.
 */
UnicodeMatcher* UnicodeSet::clone() const {
    return new UnicodeSet(*this);
}

/**
 * Returns the hash code value for this set.
 *
 * @return the hash code value for this set.
 * @see Object#hashCode()
 */
int32_t UnicodeSet::hashCode(void) const {
    int32_t result = len;
    for (int32_t i = 0; i < len; ++i) {
        result *= 1000003;
        result += list[i];
    }
    return result;
}

//----------------------------------------------------------------
// Public API
//----------------------------------------------------------------

/**
 * Make this object represent the range <code>start - end</code>.
 * If <code>end > start</code> then this object is set to an
 * an empty range.
 *
 * @param start first character in the set, inclusive
 * @rparam end last character in the set, inclusive
 */
void UnicodeSet::set(UChar32 start, UChar32 end) {
    clear();
    complement(start, end);
}

/**
 * Modifies this set to represent the set specified by the given
 * pattern, optionally ignoring white space.  See the class
 * description for the syntax of the pattern language.
 * @param pattern a string specifying what characters are in the set
 * @param ignoreSpaces if <code>true</code>, all spaces in the
 * pattern are ignored.  Spaces are those characters for which
 * <code>Character.isSpaceChar()</code> is <code>true</code>.
 * Characters preceded by '\\' are escaped, losing any special
 * meaning they otherwise have.  Spaces may be included by
 * escaping them.
 * @exception <code>IllegalArgumentException</code> if the pattern
 * contains a syntax error.
 */
void UnicodeSet::applyPattern(const UnicodeString& pattern,
                              UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }

    ParsePosition pos(0);
    applyPattern(pattern, pos, NULL, status);
    if (U_FAILURE(status)) return;

    // Skip over trailing whitespace
    int32_t i = pos.getIndex();
    int32_t n = pattern.length();
    while (i<n && Unicode::isWhitespace(pattern.charAt(i))) {
        ++i;
    }

    if (i != n) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

/**
 * Return true if the given position, in the given pattern, appears
 * to be the start of a UnicodeSet pattern.
 */
UBool UnicodeSet::resemblesPattern(const UnicodeString& pattern, int32_t pos) {
    return ((pos+1) < pattern.length() &&
            pattern.charAt(pos) == (UChar)91/*[*/) ||
        UnicodePropertySet::resemblesPattern(pattern, pos);
}

/**
 * Append the <code>toPattern()</code> representation of a
 * character to the given <code>StringBuffer</code>.
 */
void UnicodeSet::_appendToPat(UnicodeString& buf, UChar32 c, UBool useHexEscape) {
    if (useHexEscape) {
        // Use hex escape notation (\uxxxx or \Uxxxxxxxx) for anything
        // unprintable
        if (_escapeUnprintable(buf, c)) {
            return;
        }
    }
    // Okay to let ':' pass through
    switch (c) {
    case SET_OPEN:
    case SET_CLOSE:
    case HYPHEN:
    case COMPLEMENT:
    case INTERSECTION:
    case BACKSLASH:
    case 123/*{*/:
    case 125/*}*/:
        buf.append(BACKSLASH);
        break;
    default:
        // Escape whitespace
        if (Unicode::isWhitespace(c)) {
            buf.append(BACKSLASH);
        }
        break;
    }
    buf.append((UChar) c);
}

static const UChar HEX[16] = {48,49,50,51,52,53,54,55,  // 0-7
                              56,57,65,66,67,68,69,70}; // 8-9 A-F

/**
 * Return true if the character is NOT printable ASCII.
 *
 * This method should really be in UnicodeString (or similar).  For
 * now, we implement it here and share it with friend classes.
 */
UBool UnicodeSet::_isUnprintable(UChar32 c) {
    return !(c == 0x0A || (c >= 0x20 && c <= 0x7E));
}

/**
 * Escape unprintable characters using \uxxxx notation for U+0000 to
 * U+FFFF and \Uxxxxxxxx for U+10000 and above.  If the character is
 * printable ASCII, then do nothing and return FALSE.  Otherwise,
 * append the escaped notation and return TRUE.
 *
 * This method should really be in UnicodeString.  For now, we
 * implement it here and share it with friend classes.
 */
UBool UnicodeSet::_escapeUnprintable(UnicodeString& result, UChar32 c) {
    if (_isUnprintable(c)) {
        result.append(BACKSLASH);
        if (c & ~0xFFFF) {
            result.append(UPPER_U);
            result.append(HEX[0xF&(c>>28)]);
            result.append(HEX[0xF&(c>>24)]);
            result.append(HEX[0xF&(c>>20)]);
            result.append(HEX[0xF&(c>>16)]);
        } else {
            result.append(LOWER_U);
        }
        result.append(HEX[0xF&(c>>12)]);
        result.append(HEX[0xF&(c>>8)]);
        result.append(HEX[0xF&(c>>4)]);
        result.append(HEX[0xF&c]);
        return TRUE;
    }
    return FALSE;
}

/**
 * Returns a string representation of this set.  If the result of
 * calling this function is passed to a UnicodeSet constructor, it
 * will produce another set that is equal to this one.
 */
UnicodeString& UnicodeSet::toPattern(UnicodeString& result,
                                     UBool escapeUnprintable) const {
    result.truncate(0);
    return _toPattern(result, escapeUnprintable);
}

/**
 * Append a string representation of this set to result.  This will be
 * a cleaned version of the string passed to applyPattern(), if there
 * is one.  Otherwise it will be generated.
 */
UnicodeString& UnicodeSet::_toPattern(UnicodeString& result,
                                      UBool escapeUnprintable) const {
    if (pat.length() > 0) {
        int32_t i;
        int32_t backslashCount = 0;
        for (i=0; i<pat.length(); ++i) {
            UChar c = pat.charAt(i);
            if (escapeUnprintable && _isUnprintable(c)) {
                // If the unprintable character is preceded by an odd
                // number of backslashes, then it has been escaped.
                // Before unescaping it, we delete the final
                // backslash.
                if ((backslashCount % 2) == 1) {
                    result.truncate(result.length() - 1);
                }
                _escapeUnprintable(result, c);
                backslashCount = 0;
            } else {
                result.append(c);
                if (c == BACKSLASH) {
                    ++backslashCount;
                } else {
                    backslashCount = 0;
                }
            }
        }
        return result;
    }
    
    return _generatePattern(result, escapeUnprintable);
}

/**
 * Generate and append a string representation of this set to result.
 * This does not use this.pat, the cleaned up copy of the string
 * passed to applyPattern().
 */
UnicodeString& UnicodeSet::_generatePattern(UnicodeString& result,
                                            UBool escapeUnprintable) const {
    result.append(SET_OPEN);

//  // Check against the predefined categories.  We implicitly build
//  // up ALL category sets the first time toPattern() is called.
//  for (int8_t cat=0; cat<Unicode::GENERAL_TYPES_COUNT; ++cat) {
//      if (*this == getCategorySet(cat)) {
//          result.append(COLON);
//          result.append(CATEGORY_NAMES, cat*2, 2);
//          return result.append(CATEGORY_CLOSE);
//      }
//  }

    int32_t count = getRangeCount();

    // If the set contains at least 2 intervals and includes both
    // MIN_VALUE and MAX_VALUE, then the inverse representation will
    // be more economical.
    if (count > 1 &&
        getRangeStart(0) == MIN_VALUE &&
        getRangeEnd(count-1) == MAX_VALUE) {

        // Emit the inverse
        result.append(COMPLEMENT);

        for (int32_t i = 1; i < count; ++i) {
            UChar32 start = getRangeEnd(i-1)+1;
            UChar32 end = getRangeStart(i)-1;
            _appendToPat(result, start, escapeUnprintable);
            if (start != end) {
                result.append(HYPHEN);
                _appendToPat(result, end, escapeUnprintable);
            }
        }
    }

    // Default; emit the ranges as pairs
    else {
        for (int32_t i = 0; i < count; ++i) {
            UChar32 start = getRangeStart(i);
            UChar32 end = getRangeEnd(i);
            _appendToPat(result, start, escapeUnprintable);
            if (start != end) {
                result.append(HYPHEN);
                _appendToPat(result, end, escapeUnprintable);
            }
        }
    }
    
    return result.append(SET_CLOSE);
}

/**
 * Returns the number of elements in this set (its cardinality),
 * <em>n</em>, where <code>0 <= </code><em>n</em><code> <= 65536</code>.
 *
 * @return the number of elements in this set (its cardinality).
 */
int32_t UnicodeSet::size(void) const {
    int32_t n = 0;
    int32_t count = getRangeCount();
    for (int32_t i = 0; i < count; ++i) {
        n += getRangeEnd(i) - getRangeStart(i) + 1;
    }
    return n;
}

/**
 * Returns <tt>true</tt> if this set contains no elements.
 *
 * @return <tt>true</tt> if this set contains no elements.
 */
UBool UnicodeSet::isEmpty(void) const {
    return len == 1;
}

/**
 * Returns <tt>true</tt> if this set contains every character
 * in the specified range of chars.
 * If <code>end > start</code> then the results of this method
 * are undefined.
 *
 * @return <tt>true</tt> if this set contains the specified range
 * of chars.
 */
UBool UnicodeSet::contains(UChar32 start, UChar32 end) const {
    int32_t i = -1;
    for (;;) {
        if (start < list[++i]) break;
    }
    return ((i & 1) != 0 && end < list[i]);
}

/**
 * Returns <tt>true</tt> if this set contains the specified char.
 *
 * @return <tt>true</tt> if this set contains the specified char.
 */
UBool UnicodeSet::contains(UChar32 c) const {
    // Set i to the index of the start item greater than ch
    // We know we will terminate without length test!
    // LATER: for large sets, add binary search
    int32_t i = -1;
    for (;;) {
        if (c < list[++i]) break;
    }
    return ((i & 1) != 0); // return true if odd
}

/**
 * Returns <tt>true</tt> if this set contains any character whose low byte
 * is the given value.  This is used by <tt>RuleBasedTransliterator</tt> for
 * indexing.
 */
UBool UnicodeSet::matchesIndexValue(uint8_t v) const {
    /* The index value v, in the range [0,255], is contained in this set if
     * it is contained in any pair of this set.  Pairs either have the high
     * bytes equal, or unequal.  If the high bytes are equal, then we have
     * aaxx..aayy, where aa is the high byte.  Then v is contained if xx <=
     * v <= yy.  If the high bytes are unequal we have aaxx..bbyy, bb>aa.
     * Then v is contained if xx <= v || v <= yy.  (This is identical to the
     * time zone month containment logic.)
     */
    for (int32_t i=0; i<getRangeCount(); ++i) {
        UChar32 low = getRangeStart(i);
        UChar32 high = getRangeEnd(i);
        if ((low & ~0xFF) == (high & ~0xFF)) {
            if ((low & 0xFF) <= v && v <= (high & 0xFF)) {
                return TRUE;
            }
        } else if ((low & 0xFF) <= v || v <= (high & 0xFF)) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Implementation of UnicodeMatcher::matches().
 */
UMatchDegree UnicodeSet::matches(const Replaceable& text,
                                 int32_t& offset,
                                 int32_t limit,
                                 UBool incremental) const {
    if (offset == limit) {
        if (contains(TransliterationRule::ETHER)) {
            return incremental ? U_PARTIAL_MATCH : U_MATCH;
        } else {
            return U_MISMATCH;
        }
    } else {
        return UnicodeFilter::matches(text, offset, limit, incremental);
    }
}

/**
 * Adds the specified range to this set if it is not already
 * present.  If this set already contains the specified range,
 * the call leaves this set unchanged.  If <code>end > start</code>
 * then an empty range is added, leaving the set unchanged.
 *
 * @param start first character, inclusive, of range to be added
 * to this set.
 * @param end last character, inclusive, of range to be added
 * to this set.
 */
void UnicodeSet::add(UChar32 start, UChar32 end) {
    if (start <= end) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        add(range, 2, 0);
    }
}

/**
 * Adds the specified character to this set if it is not already
 * present.  If this set already contains the specified character,
 * the call leaves this set unchanged.
 */
void UnicodeSet::add(UChar32 c) {
    add(c, c);
}

/**
 * Retain only the elements in this set that are contained in the
 * specified range.  If <code>end > start</code> then an empty range is
 * retained, leaving the set empty.
 *
 * @param start first character, inclusive, of range to be retained
 * to this set.
 * @param end last character, inclusive, of range to be retained
 * to this set.
 */
void UnicodeSet::retain(UChar32 start, UChar32 end) {
    if (start <= end) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        retain(range, 2, 0);
    } else {
        clear();
    }
}

void UnicodeSet::retain(UChar32 c) {
    retain(c, c);
}

/**
 * Removes the specified range from this set if it is present.
 * The set will not contain the specified range once the call
 * returns.  If <code>end > start</code> then an empty range is
 * removed, leaving the set unchanged.
 * 
 * @param start first character, inclusive, of range to be removed
 * from this set.
 * @param end last character, inclusive, of range to be removed
 * from this set.
 */
void UnicodeSet::remove(UChar32 start, UChar32 end) {
    if (start <= end) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        retain(range, 2, 2);
    }
}

/**
 * Removes the specified character from this set if it is present.
 * The set will not contain the specified range once the call
 * returns.
 */
void UnicodeSet::remove(UChar32 c) {
    remove(c, c);
}

/**
 * Complements the specified range in this set.  Any character in
 * the range will be removed if it is in this set, or will be
 * added if it is not in this set.  If <code>end > start</code>
 * then an empty range is xor'ed, leaving the set unchanged.
 *
 * @param start first character, inclusive, of range to be removed
 * from this set.
 * @param end last character, inclusive, of range to be removed
 * from this set.
 */
void UnicodeSet::complement(UChar32 start, UChar32 end) {
    if (start <= end) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        exclusiveOr(range, 2, 0);
    }
}

void UnicodeSet::complement(UChar32 c) {
    complement(c, c);
}

/**
 * Returns <tt>true</tt> if the specified set is a <i>subset</i>
 * of this set.
 *
 * @param c set to be checked for containment in this set.
 * @return <tt>true</tt> if this set contains all of the elements of the
 *         specified set.
 */
UBool UnicodeSet::containsAll(const UnicodeSet& c) const {
    // The specified set is a subset if all of its pairs are contained in
    // this set.  It's possible to code this more efficiently in terms of
    // direct manipulation of the inversion lists if the need arises.
    int32_t n = c.getRangeCount();
    for (int i=0; i<n; ++i) {
        if (!contains(c.getRangeStart(i), c.getRangeEnd(i))) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Adds all of the elements in the specified set to this set if
 * they're not already present.  This operation effectively
 * modifies this set so that its value is the <i>union</i> of the two
 * sets.  The behavior of this operation is unspecified if the specified
 * collection is modified while the operation is in progress.
 *
 * @param c set whose elements are to be added to this set.
 * @see #add(char, char)
 */
void UnicodeSet::addAll(const UnicodeSet& c) {
    add(c.list, c.len, 0);
}

/**
 * Retains only the elements in this set that are contained in the
 * specified set.  In other words, removes from this set all of
 * its elements that are not contained in the specified set.  This
 * operation effectively modifies this set so that its value is
 * the <i>intersection</i> of the two sets.
 *
 * @param c set that defines which elements this set will retain.
 */
void UnicodeSet::retainAll(const UnicodeSet& c) {
    retain(c.list, c.len, 0);
}

/**
 * Removes from this set all of its elements that are contained in the
 * specified set.  This operation effectively modifies this
 * set so that its value is the <i>asymmetric set difference</i> of
 * the two sets.
 *
 * @param c set that defines which elements will be removed from
 *          this set.
 */
void UnicodeSet::removeAll(const UnicodeSet& c) {
    retain(c.list, c.len, 2);
}

/**
 * Complements in this set all elements contained in the specified
 * set.  Any character in the other set will be removed if it is
 * in this set, or will be added if it is not in this set.
 *
 * @param c set that defines which elements will be xor'ed from
 *          this set.
 */
void UnicodeSet::complementAll(const UnicodeSet& c) {
    exclusiveOr(c.list, c.len, 0);
}

/**
 * Inverts this set.  This operation modifies this set so that its
 * value is its complement.  This is equivalent to the pseudo
 * code: <code>this = new UnicodeSet(UnicodeSet.MIN_VALUE,
 * UnicodeSet.MAX_VALUE).removeAll(this)</code>.
 */
void UnicodeSet::complement(void) {
    if (list[0] == UNICODESET_LOW) { 
        ensureBufferCapacity(len-1);
        uprv_memcpy(buffer, list + 1, (len-1)*sizeof(UChar32));
        --len;
    } else {
        ensureBufferCapacity(len+1);
        uprv_memcpy(buffer + 1, list, len*sizeof(UChar32));
        buffer[0] = UNICODESET_LOW;
        ++len;
    }
    swapBuffers();
    pat.truncate(0);
}

/**
 * Removes all of the elements from this set.  This set will be
 * empty after this call returns.
 */
void UnicodeSet::clear(void) {
    list[0] = UNICODESET_HIGH;
    len = 1;
    pat.truncate(0);
}

/**
 * Iteration method that returns the number of ranges contained in
 * this set.
 * @see #getRangeStart
 * @see #getRangeEnd
 */
int32_t UnicodeSet::getRangeCount() const {
    return len/2;
}

/**
 * Iteration method that returns the first character in the
 * specified range of this set.
 * @see #getRangeCount
 * @see #getRangeEnd
 */
UChar32 UnicodeSet::getRangeStart(int32_t index) const {
    return list[index*2];
}

/**
 * Iteration method that returns the last character in the
 * specified range of this set.
 * @see #getRangeStart
 * @see #getRangeEnd
 */
UChar32 UnicodeSet::getRangeEnd(int32_t index) const {
    return list[index*2 + 1] - 1;
}

/**
 * Reallocate this objects internal structures to take up the least
 * possible space, without changing this object's value.
 */
void UnicodeSet::compact() {
    if (len != capacity) {
        capacity = len;
        UChar32* temp = new UChar32[capacity];
        uprv_memcpy(temp, list, len*sizeof(UChar32));
        delete[] list;
        list = temp;
    }
    delete[] buffer;
    buffer = NULL;
}

//----------------------------------------------------------------
// Implementation: Pattern parsing
//----------------------------------------------------------------

/**
 * Parses the given pattern, starting at the given position.  The
 * character at pattern.charAt(pos.getIndex()) must be '[', or the
 * parse fails.  Parsing continues until the corresponding closing
 * ']'.  If a syntax error is encountered between the opening and
 * closing brace, the parse fails.  Upon return from a successful
 * parse, the ParsePosition is updated to point to the character
 * following the closing ']', and a StringBuffer containing a
 * pairs list for the parsed pattern is returned.  This method calls
 * itself recursively to parse embedded subpatterns.
 *
 * @param pattern the string containing the pattern to be parsed.
 * The portion of the string from pos.getIndex(), which must be a
 * '[', to the corresponding closing ']', is parsed.
 * @param pos upon entry, the position at which to being parsing.
 * The character at pattern.charAt(pos.getIndex()) must be a '['.
 * Upon return from a U_SUCCESSful parse, pos.getIndex() is either
 * the character after the closing ']' of the parsed pattern, or
 * pattern.length() if the closing ']' is the last character of
 * the pattern string.
 * @return a StringBuffer containing a pairs list for the parsed
 * substring of <code>pattern</code>
 * @exception IllegalArgumentException if the parse fails.
 */
void UnicodeSet::applyPattern(const UnicodeString& pattern,
                              ParsePosition& pos,
                              const SymbolTable* symbols,
                              UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }

    // Need to build the pattern in a temporary string because
    // _applyPattern calls add() etc., which set pat to empty.
    UnicodeString rebuiltPat;
    _applyPattern(pattern, pos, symbols, rebuiltPat, status);
    pat = rebuiltPat;
}

void UnicodeSet::_applyPattern(const UnicodeString& pattern,
                               ParsePosition& pos,
                               const SymbolTable* symbols,
                               UnicodeString& rebuiltPat,
                               UErrorCode& status) {

    if (U_FAILURE(status)) {
        return;
    }

    // If the pattern contains any of the following, we save a
    // rebuilt (variable-substituted) copy of the source pattern:
    // - a category
    // - an intersection or subtraction operator
    // - an anchor (trailing '$', indicating RBT ether)
    UBool rebuildPattern = FALSE;
    UnicodeString newPat(SET_OPEN);
    int32_t nestedPatStart; // see below for usage
    UBool nestedPatDone; // see below for usage

    UBool invert = FALSE;
    clear();

    const UChar32 NONE = (UChar32) -1;
    UChar32 lastChar = NONE; // This is either a char (0..10FFFF) or NONE
    UChar lastOp = 0;

    /* This loop iterates over the characters in the pattern.  We start at
     * the position specified by pos.  We exit the loop when either a
     * matching closing ']' is seen, or we read all characters of the
     * pattern.  In the latter case an error will be thrown.
     */

    /* Pattern syntax:
     *  pat := '[' '^'? elem* ']'
     *  elem := a | a '-' a | set | set op set
     *  set := pat | (a set variable)
     *  op := '&' | '-'
     *  a := (a character, possibly defined by a var)
     */

    // mode 0: No chars parsed yet; next must be '['
    // mode 1: '[' seen; if next is '^' or ':' then special
    // mode 2: '[' '^'? seen; parse pattern and close with ']'
    // mode 3: '[:' seen; parse category and close with ':]'
    // mode 4: ']' seen; parse complete
    // mode 5: Top-level property pattern seen
    int8_t mode = 0;
    int32_t i = pos.getIndex();
    int32_t limit = pattern.length();
    UnicodeSet nestedAux;
    const UnicodeSet* nestedSet; // never owned
    UnicodeString scratch;
    /* In the case of an embedded SymbolTable variable, we look it up and
     * then take characters from the resultant char[] array.  These chars
     * are subjected to an extra level of lookup in the SymbolTable in case
     * they are stand-ins for a nested UnicodeSet.  */
    const UnicodeString* varValueBuffer = NULL;
    int32_t ivarValueBuffer = 0;
    int32_t anchor = 0;
    UChar32 c;
    while (i<limit) {
        /* If the next element is a single character, c will be set to it,
         * and nestedSet will be null.  In this case isLiteral indicates
         * whether the character should assume special meaning if it has
         * one.  If the next element is a nested set, either via a variable
         * reference, or via an embedded "[..]"  or "[:..:]" pattern, then
         * nestedSet will be set to the pairs list for the nested set, and
         * c's value should be ignored.
         */
        nestedSet = NULL;
        UBool isLiteral = FALSE;
        if (varValueBuffer != NULL) {
            if (ivarValueBuffer < varValueBuffer->length()) {
                c = varValueBuffer->char32At(ivarValueBuffer);
                ivarValueBuffer += UTF_CHAR_LENGTH(c);
                nestedSet = symbols->lookupSet(c); // may be NULL
                nestedPatDone = FALSE;
            } else {
                varValueBuffer = NULL;
                c = pattern.char32At(i);
                i += UTF_CHAR_LENGTH(c);
            }
        } else {
            c = pattern.char32At(i);
            i += UTF_CHAR_LENGTH(c);
        }

        // Ignore whitespace.  This is not Unicode whitespace, but Java
        // whitespace, a subset of Unicode whitespace.
        if (u_isspace(c)) {
            continue;
        }

        // Keep track of the count of characters after an alleged anchor
        if (anchor > 0) {
            ++anchor;
        }

        // Parse the opening '[' and optional following '^'
        switch (mode) {
        case 0:
            if (UnicodePropertySet::resemblesPattern(pattern, i-1)) {
                mode = 3;
                break; // Fall through
            } else if (c == SET_OPEN) {
                mode = 1; // Next look for '^' or ':'
                continue;
            } else {
                // throw new IllegalArgumentException("Missing opening '['");
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
        case 1:
            mode = 2;
            switch (c) {
            case COMPLEMENT:
                invert = TRUE;
                newPat.append(c);
                continue; // Back to top to fetch next character
            case HYPHEN:
                isLiteral = TRUE; // Treat leading '-' as a literal
                break; // Fall through
            }
            // else fall through and parse this character normally
        }

        // After opening matter is parsed ("[", "[^", or "[:"), the mode
        // will be 2 if we want a closing ']', or 3 if we should parse a
        // category and close with ":]".

        // Only process escapes, variable references, and nested sets
        // if we are _not_ retrieving characters from the variable
        // buffer.  Characters in the variable buffer have already
        // benn through escape and variable reference processing.
        if (varValueBuffer == NULL) {
            /**
             * Handle property set patterns.
             */
            if (UnicodePropertySet::resemblesPattern(pattern, i-1)) {
                ParsePosition pp(i-1);
                UnicodeSet *s = UnicodePropertySet::createFromPattern(pattern, pp);
                if (s == NULL) {
                    // assert(pp.getIndex() == i-1);
                    //throw new IllegalArgumentException("Invalid property pattern " +
                    //                                   pattern.substring(i-1));
                    status = U_INVALID_PROPERTY_PATTERN;
                    return;
                }
                // TODO This is very inefficient.  We create a new UnicodeSet,
                // then do an assignment, then delete it.  Clean this up in
                // the future so that either (1) we just use the new set
                // directly, and delete it when we're done, or (2) even better,
                // UnicodePropertySet takes an existing set.
                nestedAux = *s;
                delete s;
                nestedSet = &nestedAux;
                nestedPatStart = newPat.length();
                nestedPatDone = TRUE; // we're going to do it just below
                
                // If we have a top-level property pattern, then trim
                // off the opening '[' and use the property pattern
                // as the entire pattern.
                if (mode == 3) {
                    newPat.truncate(0);
                }
                UnicodeString str;
                pattern.extractBetween(i-1, pp.getIndex(), str);
                newPat.append(str);
                rebuildPattern = TRUE;
                
                i = pp.getIndex(); // advance past property pattern
                
                if (mode == 3) {
                    // Entire pattern is a category; leave parse
                    // loop.  This is one of 2 ways we leave this
                    // loop if the pattern is well-formed.
                    *this = nestedAux;
                    mode = 5;
                    break;
                }
            }
            
            /* Handle escapes.  If a character is escaped, then it assumes its
             * literal value.  This is true for all characters, both special
             * characters and characters with no special meaning.  We also
             * interpret '\\uxxxx' Unicode escapes here (as literals).
             */
            else if (c == BACKSLASH) {
                UChar32 escaped = pattern.unescapeAt(i);
                if (escaped == (UChar32) -1) {
                    status = U_ILLEGAL_ARGUMENT_ERROR;
                    return;
                }
                isLiteral = TRUE;
                c = escaped;
            }

            /* Parse variable references.  These are treated as literals.  If a
             * variable refers to a UnicodeSet, its stand in character is
             * returned in the UChar[] buffer.
             * Variable names are only parsed if varNameToChar is not null.
             * Set variables are only looked up if varCharToSet is not null.
             */
            else if (symbols != NULL && !isLiteral && c == SymbolTable::SYMBOL_REF) {
                pos.setIndex(i);
                UnicodeString name = symbols->parseReference(pattern, pos, limit);
                if (name.length() != 0) {
                    varValueBuffer = symbols->lookup(name);
                    if (varValueBuffer == NULL) {
                        //throw new IllegalArgumentException("Undefined variable: "
                        //                                   + name);
                        status = U_ILLEGAL_ARGUMENT_ERROR;
                        return;
                    }
                    ivarValueBuffer = 0;
                    i = pos.getIndex(); // Make i point PAST last char of var name
                } else {
                    // Got a null; this means we have an isolated $.
                    // Tentatively assume this is an anchor.
                    anchor = 1;
                }
                continue; // Back to the top to get varValueBuffer[0]
            }

            /* An opening bracket indicates the first bracket of a nested
             * subpattern.
             */
            else if (!isLiteral && c == SET_OPEN) {
                // Record position before nested pattern
                nestedPatStart = newPat.length();

                // Recurse to get the pairs for this nested set.
                // Backup i to '['.
                pos.setIndex(--i);
                switch (lastOp) {
                case HYPHEN:
                case INTERSECTION:
                    newPat.append(lastOp);
                    break;
                }
                nestedAux._applyPattern(pattern, pos, symbols, newPat, status);
                nestedSet = &nestedAux;
                nestedPatDone =  TRUE;
                if (U_FAILURE(status)) {
                    return;
                }
                i = pos.getIndex();
            }
        }

        /* At this point we have either a character c, or a nested set.  If
         * we have encountered a nested set, either embedded in the pattern,
         * or as a variable, we have a non-null nestedSet, and c should be
         * ignored.  Otherwise c is the current character, and isLiteral
         * indicates whether it is an escaped literal (or variable) or a
         * normal unescaped character.  Unescaped characters '-', '&', and
         * ']' have special meanings.
         */
        if (nestedSet != NULL) {
            if (lastChar != NONE) {
                if (lastOp != 0) {
                    // throw new IllegalArgumentException("Illegal rhs for " + lastChar + lastOp);
                    status = U_ILLEGAL_ARGUMENT_ERROR;
                    return;
                }
                add(lastChar, lastChar);
                if (nestedPatDone) {
                    // If there was a character before the nested set,
                    // then we need to insert it in newPat before the
                    // pattern for the nested set.  This position was
                    // recorded in nestedPatStart.
                    UnicodeString s;
                    _appendToPat(s, lastChar, FALSE);
                    newPat.insert(nestedPatStart, s);
                } else {
                    _appendToPat(newPat, lastChar, FALSE);
                }
                lastChar = NONE;
            }
            switch (lastOp) {
            case HYPHEN:
                removeAll(*nestedSet);
                break;
            case INTERSECTION:
                retainAll(*nestedSet);
                break;
            case 0:
                addAll(*nestedSet);
                break;
            }

            // Get the pattern for the nested set, if we haven't done so
            // already.
            if (!nestedPatDone) {
                if (lastOp != 0) {
                    newPat.append(lastOp);
                }
                nestedSet->_toPattern(newPat, FALSE);
            }
            rebuildPattern = TRUE;

            lastOp = 0;

        } else if (!isLiteral && c == SET_CLOSE) {
            // Final closing delimiter.  This is one of 2 ways we
            // leave this loop if the pattern is well-formed.
            if (anchor > 2 || anchor == 1) {
                //throw new IllegalArgumentException("Syntax error near $" + pattern);
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            if (anchor == 2) {
                rebuildPattern = TRUE;
                newPat.append((UChar)SymbolTable::SYMBOL_REF);
                add(TransliterationRule::ETHER);
            }
            mode = 4;
            break;
        } else if (lastOp == 0 && !isLiteral && (c == HYPHEN || c == INTERSECTION)) {
            // assert(c <= 0xFFFF);
            lastOp = (UChar) c;
        } else if (lastOp == HYPHEN) {
            if (lastChar >= c) {
                // Don't allow redundant (a-a) or empty (b-a) ranges;
                // these are most likely typos.
                //throw new IllegalArgumentException("Invalid range " + lastChar +
                //                                       '-' + c);
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            add(lastChar, c);
            _appendToPat(newPat, lastChar, FALSE);
            newPat.append(HYPHEN);
            _appendToPat(newPat, c, FALSE);
            lastOp = 0;
            lastChar = NONE;
        } else if (lastOp != 0) {
            // We have <set>&<char> or <char>&<char>
            // throw new IllegalArgumentException("Unquoted " + lastOp);
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        } else {
            if (lastChar != NONE) {
                // We have <char><char>
                add(lastChar, lastChar);
                _appendToPat(newPat, lastChar, FALSE);
            }
            lastChar = c;
        }
    }

    if (mode < 4) {
        // throw new IllegalArgumentException("Missing ']'");
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    // Treat a trailing '$' as indicating ETHER.  This code is only
    // executed if symbols == NULL; otherwise other code parses the
    // anchor.
    if (lastChar == (UChar)SymbolTable::SYMBOL_REF) {
        rebuildPattern = TRUE;
        newPat.append(lastChar);
        add(TransliterationRule::ETHER);
    }

    else if (lastChar != NONE) {
        add(lastChar, lastChar);
        _appendToPat(newPat, lastChar, FALSE);
    }

    // Handle unprocessed stuff preceding the closing ']'
    if (lastOp == HYPHEN) {
        // Trailing '-' is treated as literal
        add(lastOp, lastOp);
        newPat.append(HYPHEN);
    } else if (lastOp == INTERSECTION) {
        // throw new IllegalArgumentException("Unquoted trailing " + lastOp);
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if (mode == 4) {
        newPat.append(SET_CLOSE);
    }

    /**
     * If we saw a '^' after the initial '[' of this pattern, then perform
     * the complement.  (Inversion after '[:' is handled elsewhere.)
     */
    if (invert) {
        complement();
    }

    pos.setIndex(i);

    // Use the rebuilt pattern (newPat) only if necessary.  Prefer the
    // generated pattern.
    if (rebuildPattern) {
        rebuiltPat.append(newPat);
    } else {
        _generatePattern(rebuiltPat, FALSE);
    }
}

//----------------------------------------------------------------
// Implementation: Utility methods
//----------------------------------------------------------------

void UnicodeSet::ensureCapacity(int32_t newLen) {
    if (newLen <= capacity) return;
    capacity = newLen + GROW_EXTRA;
    UChar32* temp = new UChar32[capacity];
    uprv_memcpy(temp, list, len*sizeof(UChar32));
    delete[] list;
    list = temp;
}

void UnicodeSet::ensureBufferCapacity(int32_t newLen) {
    if (buffer != NULL && newLen <= bufferCapacity) return;
    delete[] buffer;
    bufferCapacity = newLen + GROW_EXTRA;
    buffer = new UChar32[bufferCapacity];
}

/**
 * Swap list and buffer.
 */
void UnicodeSet::swapBuffers(void) {
    // swap list and buffer
    UChar32* temp = list;
    list = buffer;
    buffer = temp;

    int32_t c = capacity;
    capacity = bufferCapacity;
    bufferCapacity = c;
}

//----------------------------------------------------------------
// Implementation: Fundamental operators
//----------------------------------------------------------------

inline UChar32 max(UChar32 a, UChar32 b) {
    return (a > b) ? a : b;
}

// polarity = 0, 3 is normal: x xor y
// polarity = 1, 2: x xor ~y == x === y

void UnicodeSet::exclusiveOr(const UChar32* other, int32_t otherLen, int8_t polarity) {
    ensureBufferCapacity(len + otherLen);
    int32_t i = 0, j = 0, k = 0;
    UChar32 a = list[i++];
    UChar32 b;
    if (polarity == 1 || polarity == 2) {
        b = UNICODESET_LOW;
        if (other[j] == UNICODESET_LOW) { // skip base if already LOW
            ++j;
            b = other[j];
        }
    } else {
        b = other[j++];
    }
    // simplest of all the routines
    // sort the values, discarding identicals!
    for (;;) {
        if (a < b) {
            buffer[k++] = a;
            a = list[i++];
        } else if (b < a) {
            buffer[k++] = b;
            b = other[j++];
        } else if (a != UNICODESET_HIGH) { // at this point, a == b
            // discard both values!
            a = list[i++];
            b = other[j++];
        } else { // DONE!
            buffer[k++] = UNICODESET_HIGH;
            len = k;
            break;
        }
    }
    swapBuffers();
    pat.truncate(0);
}

// polarity = 0 is normal: x union y
// polarity = 2: x union ~y
// polarity = 1: ~x union y
// polarity = 3: ~x union ~y

void UnicodeSet::add(const UChar32* other, int32_t otherLen, int8_t polarity) {
    ensureBufferCapacity(len + otherLen);
    int32_t i = 0, j = 0, k = 0;
    UChar32 a = list[i++];
    UChar32 b = other[j++];
    // change from xor is that we have to check overlapping pairs
    // polarity bit 1 means a is second, bit 2 means b is.
    for (;;) {
        switch (polarity) {
          case 0: // both first; take lower if unequal
            if (a < b) { // take a
                // Back up over overlapping ranges in buffer[]
                if (k > 0 && a <= buffer[k-1]) {
                    // Pick latter end value in buffer[] vs. list[]
                    a = max(list[i], buffer[--k]);
                } else {
                    // No overlap
                    buffer[k++] = a;
                    a = list[i];
                }
                i++; // Common if/else code factored out
                polarity ^= 1;
            } else if (b < a) { // take b
                if (k > 0 && b <= buffer[k-1]) {
                    b = max(other[j], buffer[--k]);
                } else {
                    buffer[k++] = b;
                    b = other[j];
                }
                j++;
                polarity ^= 2;
            } else { // a == b, take a, drop b
                if (a == UNICODESET_HIGH) goto loop_end;
                // This is symmetrical; it doesn't matter if
                // we backtrack with a or b. - liu
                if (k > 0 && a <= buffer[k-1]) {
                    a = max(list[i], buffer[--k]);
                } else {
                    // No overlap
                    buffer[k++] = a;
                    a = list[i];
                }
                i++;
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 3: // both second; take higher if unequal, and drop other
            if (b <= a) { // take a
                if (a == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = a;
            } else { // take b
                if (b == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = b;
            }
            a = list[i++];
            polarity ^= 1;   // factored common code
            b = other[j++];
            polarity ^= 2;
            break;
          case 1: // a second, b first; if b < a, overlap
            if (a < b) { // no overlap, take a
                buffer[k++] = a; a = list[i++]; polarity ^= 1;
            } else if (b < a) { // OVERLAP, drop b
                b = other[j++];
                polarity ^= 2;
            } else { // a == b, drop both!
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 2: // a first, b second; if a < b, overlap
            if (b < a) { // no overlap, take b
                buffer[k++] = b;
                b = other[j++];
                polarity ^= 2;
            } else  if (a < b) { // OVERLAP, drop a
                a = list[i++];
                polarity ^= 1;
            } else { // a == b, drop both!
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
        }
    }
 loop_end:
    buffer[k++] = UNICODESET_HIGH;    // terminate
    len = k;
    swapBuffers();
    pat.truncate(0);
}

// polarity = 0 is normal: x intersect y
// polarity = 2: x intersect ~y == set-minus
// polarity = 1: ~x intersect y
// polarity = 3: ~x intersect ~y

void UnicodeSet::retain(const UChar32* other, int32_t otherLen, int8_t polarity) {
    ensureBufferCapacity(len + otherLen);
    int32_t i = 0, j = 0, k = 0;
    UChar32 a = list[i++];
    UChar32 b = other[j++];
    // change from xor is that we have to check overlapping pairs
    // polarity bit 1 means a is second, bit 2 means b is.
    for (;;) {
        switch (polarity) {
          case 0: // both first; drop the smaller
            if (a < b) { // drop a
                a = list[i++];
                polarity ^= 1;
            } else if (b < a) { // drop b
                b = other[j++];
                polarity ^= 2;
            } else { // a == b, take one, drop other
                if (a == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 3: // both second; take lower if unequal
            if (a < b) { // take a
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
            } else if (b < a) { // take b
                buffer[k++] = b;
                b = other[j++];
                polarity ^= 2;
            } else { // a == b, take one, drop other
                if (a == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 1: // a second, b first;
            if (a < b) { // NO OVERLAP, drop a
                a = list[i++];
                polarity ^= 1;
            } else if (b < a) { // OVERLAP, take b
                buffer[k++] = b;
                b = other[j++];
                polarity ^= 2;
            } else { // a == b, drop both!
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 2: // a first, b second; if a < b, overlap
            if (b < a) { // no overlap, drop b
                b = other[j++];
                polarity ^= 2;
            } else  if (a < b) { // OVERLAP, take a
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
            } else { // a == b, drop both!
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
        }
    }
 loop_end:
    buffer[k++] = UNICODESET_HIGH;    // terminate
    len = k;
    swapBuffers();
    pat.truncate(0);
}

U_NAMESPACE_END

