/*
*******************************************************************************
*
*   Copyright (C) 1999-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  utf.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep09
*   created by: Markus W. Scherer
*/

/**
 * \file
 * \brief C API: UChar and UChar32 data types and code point macros
 *
 * This file defines the UChar and UChar32 data types for Unicode code units
 * and code points, as well as macros for checking whether a code point is
 * a surrogate or a non-character.
 *
 * utf.h is included by utypes.h and itself includes utf8.h and utf16.h after some
 * common definitions. Those files define macros for efficiently getting code points
 * in and out of UTF-8/16 strings.
 * utf16.h macros have "U16_" prefixes.
 * utf8.h defines similar macros with "U8_" prefixes for UTF-8 string handling.
 *
 * ICU processes 16-bit Unicode strings.
 * Most of the time, such strings are well-formed UTF-16.
 * Single, unpaired surrogates must be handled as well, and are treated in ICU
 * like regular code points where possible.
 * (Pairs of surrogate code points are indistinguishable from supplementary
 * code points encoded as pairs of supplementary code units.)
 *
 * In fact, almost all Unicode code points in normal text (>99%)
 * are on the BMP (<=U+ffff) and even <=U+d7ff.
 * ICU functions handle supplementary code points (U+10000..U+10ffff)
 * but are optimized for the much more frequently occurring BMP code points.
 *
 * utf.h defines UChar to be an unsigned 16-bit integer. If this matches wchar_t, then
 * UChar is defined to be exactly wchar_t, otherwise uint16_t.
 *
 * UChar32 is defined to be a signed 32-bit integer (int32_t), large enough for a 21-bit
 * Unicode code point (Unicode scalar value, 0..0x10ffff).
 * Before ICU 2.4, the definition of UChar32 was similarly platform-dependent as
 * the definition of UChar. For details see the documentation for UChar32 itself.
 *
 * utf.h also defines a small number of C macros for single Unicode code points.
 * These are simple checks for surrogates and non-characters.
 * For actual Unicode character properties see uchar.h.
 *
 * By default, string operations must be done with error checking in case
 * a string is not well-formed UTF-16.
 * The macros will detect if a surrogate code unit is unpaired
 * (lead unit without trail unit or vice versa) and just return the unit itself
 * as the code point.
 * (It is an accidental property of Unicode and UTF-16 that all
 * malformed sequences can be expressed unambiguously with a distinct subrange
 * of Unicode code points.)
 *
 * When it is safe to assume that text is well-formed UTF-16
 * (does not contain single, unpaired surrogates), then one can use
 * U16_..._UNSAFE macros.
 * These do not check for proper code unit sequences or truncated text and may
 * yield wrong results or even cause a crash if they are used with "malformed"
 * text.
 * In practice, U16_..._UNSAFE macros will produce slightly less code but
 * should not be faster because the processing is only different when a
 * surrogate code unit is detected, which will be rare.
 *
 * Similarly for UTF-8, there are "safe" macros without a suffix,
 * and U8_..._UNSAFE versions.
 * The performance differences are much larger here because UTF-8 provides so
 * many opportunities for malformed sequences.
 * The unsafe UTF-8 macros are entirely implemented inside the macro definitions
 * and are fast, while the safe UTF-8 macros call functions for all but the
 * trivial (ASCII) cases.
 *
 * Unlike with UTF-16, malformed sequences cannot be expressed with distinct
 * code point values (0..U+10ffff). They are indicated with negative values instead.
 *
 * For more information see the ICU User Guide Strings chapter
 * (http://oss.software.ibm.com/icu/userguide/).
 *
 * <em>Usage:</em>
 * ICU coding guidelines for if() statements should be followed when using these macros.
 * Compound statements (curly braces {}) must be used  for if-else-while... 
 * bodies and all macro statements should be terminated with semicolon.
 *
 * @draft ICU 2.4
 */

#ifndef __UTF_H__
#define __UTF_H__

/* wchar_t-related definitions ---------------------------------------------- */

/*
 * ANSI C headers:
 * stddef.h defines wchar_t
 */
#include "unicode/umachine.h"
#include <stddef.h>
/* include the utfXX.h after the following definitions */

/**
 * \def U_HAVE_WCHAR_H
 * Indicates whether <wchar.h> is available (1) or not (0). Set to 1 by default.
 *
 * @stable ICU 2.0
 */
#ifndef U_HAVE_WCHAR_H
#   define U_HAVE_WCHAR_H 1
#endif

/**
 * \def U_SIZEOF_WCHAR_T
 * U_SIZEOF_WCHAR_T==sizeof(wchar_t) (0 means it is not defined or autoconf could not set it)
 *
 * @stable ICU 2.0
 */
#if U_SIZEOF_WCHAR_T==0
#   undef U_SIZEOF_WCHAR_T
#   define U_SIZEOF_WCHAR_T 4
#endif

/**
 * \def U_WCHAR_IS_UTF16
 * Defined if wchar_t uses UTF-16.
 *
 * @stable ICU 2.0
 */
/**
 * \def U_WCHAR_IS_UTF32
 * Defined if wchar_t uses UTF-32.
 *
 * @stable ICU 2.0
 */
#if !defined(U_WCHAR_IS_UTF16) && !defined(U_WCHAR_IS_UTF32)
#   ifdef __STDC_ISO_10646__ 
#       if (U_SIZEOF_WCHAR_T==2)
#           define U_WCHAR_IS_UTF16
#       elif (U_SIZEOF_WCHAR_T==4)
#           define  U_WCHAR_IS_UTF32
#       endif
#   elif defined __UCS2__
#       if (__OS390__ || __OS400__) && (U_SIZEOF_WCHAR_T==2)
#           define U_WCHAR_IS_UTF16
#       endif
#   elif defined __UCS4__
#       if (U_SIZEOF_WCHAR_T==4)
#           define U_WCHAR_IS_UTF32
#       endif
#   elif defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#       define U_WCHAR_IS_UTF16    
#   endif
#endif

/* UChar and UChar32 definitions -------------------------------------------- */

/** Number of bytes in a UChar. @stable ICU 2.0 */
#define U_SIZEOF_UCHAR 2

/**
 * \var UChar
 * Define UChar to be wchar_t if that is 16 bits wide; always assumed to be unsigned.
 * If wchar_t is not 16 bits wide, then define UChar to be uint16_t.
 * This makes the definition of UChar platform-dependent
 * but allows direct string type compatibility with platforms with
 * 16-bit wchar_t types.
 *
 * @stable ICU 2.0
 */

/* Define UChar to be compatible with wchar_t if possible. */
#if U_SIZEOF_WCHAR_T==2
    typedef wchar_t UChar;
#else
    typedef uint16_t UChar;
#endif

/**
 * Define UChar32 as a type for single Unicode code points.
 * UChar32 is a signed 32-bit integer (same as int32_t).
 *
 * The Unicode code point range is 0..0x10ffff.
 * All other values (negative or >=0x110000) are illegal as Unicode code points.
 * They may be used as sentinel values to indicate "done", "error"
 * or similar non-code point conditions.
 *
 * Before ICU 2.4 (Jitterbug 2146), UChar32 was defined
 * to be wchar_t if that is 32 bits wide (wchar_t may be signed or unsigned)
 * or else to be uint32_t.
 * That is, the definition of UChar32 was platform-dependent.
 *
 * @see U_SENTINEL
 * @draft ICU 2.4
 */
typedef int32_t UChar32;

/* single-code point definitions -------------------------------------------- */

/**
 * This value is intended for sentinel values for APIs that
 * (take or) return single code points (UChar32).
 * It is outside of the Unicode code point range 0..0x10ffff.
 * 
 * For example, a "done" or "error" value in a new API
 * could be indicated with U_SENTINEL.
 *
 * ICU APIs designed before ICU 2.4 usually define service-specific "done"
 * values, mostly 0xffff.
 * Those may need to be distinguished from
 * actual U+ffff text contents by calling functions like
 * CharacterIterator::hasNext() or UnicodeString::length().
 *
 * @return -1
 * @see UChar32
 * @draft ICU 2.4
 */
#define U_SENTINEL (-1)

/**
 * Is this code point a Unicode noncharacter?
 * @param c 32-bit code point
 * @return TRUE or FALSE
 * @draft ICU 2.4
 */
#define U_IS_UNICODE_NONCHAR(c) \
    ((c)>=0xfdd0 && \
     ((uint32_t)(c)<=0xfdef || ((c)&0xfffe)==0xfffe) && \
     (uint32_t)(c)<=0x10ffff)

/**
 * Is c a Unicode code point value (0..U+10ffff)
 * that can be assigned a character?
 *
 * Code points that are not characters include:
 * - single surrogate code points (U+d800..U+dfff, 2048 code points)
 * - the last two code points on each plane (U+__fffe and U+__ffff, 34 code points)
 * - U+fdd0..U+fdef (new with Unicode 3.1, 32 code points)
 * - the highest Unicode code point value is U+10ffff
 *
 * This means that all code points below U+d800 are character code points,
 * and that boundary is tested first for performance.
 *
 * @param c 32-bit code point
 * @return TRUE or FALSE
 * @draft ICU 2.4
 */
#define U_IS_UNICODE_CHAR(c) \
    ((uint32_t)(c)<0xd800 || \
        ((uint32_t)(c)>0xdfff && \
         (uint32_t)(c)<=0x10ffff && \
         !U_IS_UNICODE_NONCHAR(c)))

/**
 * Is this code point a lead surrogate (U+d800..U+dbff)?
 * @param c 32-bit code point
 * @return TRUE or FALSE
 * @draft ICU 2.4
 */
#define U_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)

/**
 * Is this code point a trail surrogate (U+dc00..U+dfff)?
 * @param c 32-bit code point
 * @return TRUE or FALSE
 * @draft ICU 2.4
 */
#define U_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)

/**
 * Is this code point a surrogate (U+d800..U+dfff)?
 * @param c 32-bit code point
 * @return TRUE or FALSE
 * @draft ICU 2.4
 */
#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)

/**
 * Assuming c is a surrogate code point (U_IS_SURROGATE(c)),
 * is it a lead surrogate?
 * @param c 32-bit code point
 * @return TRUE or FALSE
 * @draft ICU 2.4
 */
#define U_IS_SURROGATE_LEAD(c) (((c)&0x400)==0)

/* include the utfXX.h ------------------------------------------------------ */

#include "unicode/utf8.h"
#include "unicode/utf16.h"

/* utf_old.h contains deprecated, pre-ICU 2.4 definitions */
#include "unicode/utf_old.h"

#endif
