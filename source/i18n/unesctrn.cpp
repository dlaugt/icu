/*
**********************************************************************
*   Copyright (c) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/19/2001  aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uchar.h"
#include "unesctrn.h"
#include "util.h"

#include "cmemory.h"

U_NAMESPACE_BEGIN

/**
 * Special character marking the end of the spec[] array.
 */
static const UChar END = 0xFFFF;

// Unicode: "U+10FFFF" hex, min=4, max=6
static const UChar SPEC_Unicode[] = {
    2, 0, 16, 4, 6, 85/*U*/, 43/*+*/,
    END
};

// Java: "\\uFFFF" hex, min=4, max=4
static const UChar SPEC_Java[] = {
    2, 0, 16, 4, 4, 92/*\*/, 117/*u*/,
    END
};

// C: "\\uFFFF" hex, min=4, max=4; \\U0010FFFF hex, min=8, max=8
static const UChar SPEC_C[] = {
    2, 0, 16, 4, 4, 92/*\*/, 117/*u*/,
    2, 0, 16, 8, 8, 92/*\*/, 85/*U*/,
    END
};

// XML: "&#x10FFFF;" hex, min=1, max=6
static const UChar SPEC_XML[] = {
    3, 1, 16, 1, 6, 38/*&*/, 35/*#*/, 120/*x*/, 59/*;*/,
    END
};

// XML10: "&#1114111;" dec, min=1, max=7 (not really "Hex-Any")
static const UChar SPEC_XML10[] = {
    2, 1, 10, 1, 7, 38/*&*/, 35/*#*/, 59/*;*/,
    END
};

// Perl: "\\x{263A}" hex, min=1, max=6
static const UChar SPEC_Perl[] = {
    3, 1, 16, 1, 6, 92/*\*/, 120/*x*/, 123/*{*/, 125/*}*/,
    END
};

// All: Java, C, Perl, XML, XML10, Unicode
static const UChar SPEC_Any[] = {
    2, 0, 16, 4, 6, 85/*U*/, 43/*+*/,                      // Unicode
    2, 0, 16, 4, 4, 92/*\*/, 117/*u*/,                     // Java
    2, 0, 16, 8, 8, 92/*\*/, 85/*U*/,                      // C (surrogates)
    3, 1, 16, 1, 6, 38/*&*/, 35/*#*/, 120/*x*/, 59/*;*/,   // XML
    2, 1, 10, 1, 7, 38/*&*/, 35/*#*/, 59/*;*/,             // XML10
    3, 1, 16, 1, 6, 92/*\*/, 120/*x*/, 123/*{*/, 125/*}*/, // Perl
    END
};

const char UnescapeTransliterator::fgClassID=0;

/**
 * Factory methods.  Ignore the context.
 */
Transliterator* UnescapeTransliterator::_createUnicode(const UnicodeString& ID, Token /*context*/) {
    return new UnescapeTransliterator(ID, SPEC_Unicode);
}
Transliterator* UnescapeTransliterator::_createJava(const UnicodeString& ID, Token /*context*/) {
    return new UnescapeTransliterator(ID, SPEC_Java);
}
Transliterator* UnescapeTransliterator::_createC(const UnicodeString& ID, Token /*context*/) {
    return new UnescapeTransliterator(ID, SPEC_C);
}
Transliterator* UnescapeTransliterator::_createXML(const UnicodeString& ID, Token /*context*/) {
    return new UnescapeTransliterator(ID, SPEC_XML);
}
Transliterator* UnescapeTransliterator::_createXML10(const UnicodeString& ID, Token /*context*/) {
    return new UnescapeTransliterator(ID, SPEC_XML10);
}
Transliterator* UnescapeTransliterator::_createPerl(const UnicodeString& ID, Token /*context*/) {
    return new UnescapeTransliterator(ID, SPEC_Perl);
}
Transliterator* UnescapeTransliterator::_createAny(const UnicodeString& ID, Token /*context*/) {
    return new UnescapeTransliterator(ID, SPEC_Any);
}

/**
 * Registers standard variants with the system.  Called by
 * Transliterator during initialization.
 */
void UnescapeTransliterator::registerIDs() {
    Token t = integerToken(0);

    Transliterator::_registerFactory("Hex-Any/Unicode", _createUnicode, t);

    Transliterator::_registerFactory("Hex-Any/Java", _createJava, t);

    Transliterator::_registerFactory("Hex-Any/C", _createC, t);

    Transliterator::_registerFactory("Hex-Any/XML", _createXML, t);

    Transliterator::_registerFactory("Hex-Any/XML10", _createXML10, t);

    Transliterator::_registerFactory("Hex-Any/Perl", _createPerl, t);

    Transliterator::_registerFactory("Hex-Any", _createAny, t);
}

/**
 * Constructor.  Takes the encoded spec array.
 */
UnescapeTransliterator::UnescapeTransliterator(const UnicodeString& newID,
                                               const UChar *newSpec) :
    Transliterator(newID, NULL)
{
    this->spec = copySpec(newSpec);
}

/**
 * Copy constructor.
 */
UnescapeTransliterator::UnescapeTransliterator(const UnescapeTransliterator& o) :
    Transliterator(o) {
    this->spec = copySpec(o.spec);
}

UnescapeTransliterator::~UnescapeTransliterator() {
    uprv_free(spec);
}

/**
 * Transliterator API.
 */
Transliterator* UnescapeTransliterator::clone() const {
    return new UnescapeTransliterator(*this);
}

UChar* UnescapeTransliterator::copySpec(const UChar* spec) {
    int32_t len = 0;
    while (spec[len] != END) {
        ++len;
    }
    ++len;
    UChar *result = (UChar *)uprv_malloc(len*sizeof(UChar));
    uprv_memcpy(result, spec, len*sizeof(result[0]));
    return result;
}

/**
 * Implements {@link Transliterator#handleTransliterate}.
 */
void UnescapeTransliterator::handleTransliterate(Replaceable& text, UTransPosition& pos,
                                                 UBool isIncremental) const {
    int32_t start = pos.start;
    int32_t limit = pos.limit;
    int32_t i, j, ipat;

    while (start < limit) {
        // Loop over the forms in spec[].  Exit this loop when we
        // match one of the specs.  Exit the outer loop if a
        // partial match is detected and isIncremental is true.
        for (j=0, ipat=0; spec[ipat] != END; ++j) {

            // Read the header
            int32_t prefixLen = spec[ipat++];
            int32_t suffixLen = spec[ipat++];
            int8_t  radix     = (int8_t) spec[ipat++];
            int32_t minDigits = spec[ipat++];
            int32_t maxDigits = spec[ipat++];

            // s is a copy of start that is advanced over the
            // characters as we parse them.
            int32_t s = start;
            UBool match = TRUE;

            for (i=0; i<prefixLen; ++i) {
                if (s >= limit) {
                    if (i > 0) {
                        // We've already matched a character.  This is
                        // a partial match, so we return if in
                        // incremental mode.  In non-incremental mode,
                        // go to the next spec.
                        if (isIncremental) {
                            goto exit;
                        }
                        match = FALSE;
                        break;
                    }
                }
                UChar c = text.charAt(s++);
                if (c != spec[ipat + i]) {
                    match = FALSE;
                    break;
                }
            }

            if (match) {
                UChar32 u = 0;
                int32_t digitCount = 0;
                for (;;) {
                    if (s >= limit) {
                        // Check for partial match in incremental mode.
                        if (s > start && isIncremental) {
                            goto exit;
                        }
                        break;
                    }
                    UChar32 ch = text.char32At(s);
                    int32_t digit = u_digit(ch, radix);
                    if (digit < 0) {
                        break;
                    }
                    s += UTF_CHAR_LENGTH(ch);
                    u = (u * radix) + digit;
                    if (++digitCount == maxDigits) {
                        break;
                    }
                }

                match = (digitCount >= minDigits);

                if (match) {
                    for (i=0; i<suffixLen; ++i) {
                        if (s >= limit) {
                            // Check for partial match in incremental mode.
                            if (s > start && isIncremental) {
                                goto exit;
                            }
                            match = FALSE;
                            break;
                        }
                        UChar c = text.charAt(s++);
                        if (c != spec[ipat + prefixLen + i]) {
                            match = FALSE;
                            break;
                        }
                    }

                    if (match) {
                        // At this point, we have a match
                        UnicodeString str(u);
                        text.handleReplaceBetween(start, s, str);
                        limit -= s - start - str.length();
                        // The following break statement leaves the
                        // loop that is traversing the forms in
                        // spec[].  We then parse the next input
                        // character.
                        break;
                    }
                }
            }

            ipat += prefixLen + suffixLen;
        }

        if (start < limit) {
            start += UTF_CHAR_LENGTH(text.char32At(start));
        }
    }

  exit:
    pos.contextLimit += limit - pos.limit;
    pos.limit = limit;
    pos.start = start;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

//eof
