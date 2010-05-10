/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  uts46test.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010may05
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/bytestream.h"
#include "unicode/idna.h"
#include "unicode/std_string.h"
#include "unicode/stringpiece.h"
#include "unicode/uidna.h"
#include "unicode/unistr.h"
#include "intltest.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

class UTS46Test : public IntlTest {
public:
    UTS46Test() : trans(NULL), nontrans(NULL) {}
    virtual ~UTS46Test();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);
    void TestAPI();
    void TestSomeCases();
private:
    IDNA *trans, *nontrans;
};

extern IntlTest *createUTS46Test() {
    return new UTS46Test();
}

UTS46Test::~UTS46Test() {
    delete trans;
    delete nontrans;
}

void UTS46Test::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if(exec) {
        logln("TestSuite UTS46Test: ");
        if(trans==NULL) {
            IcuTestErrorCode errorCode(*this, "init/createUTS46Instance()");
            trans=IDNA::createUTS46Instance(
                UIDNA_USE_STD3_RULES|UIDNA_CHECK_BIDI|UIDNA_CHECK_CONTEXTJ,
                errorCode);
            nontrans=IDNA::createUTS46Instance(
                UIDNA_USE_STD3_RULES|UIDNA_CHECK_BIDI|UIDNA_CHECK_CONTEXTJ|
                UIDNA_NONTRANSITIONAL_TO_ASCII|UIDNA_NONTRANSITIONAL_TO_UNICODE,
                errorCode);
            if(errorCode.logDataIfFailureAndReset("createUTS46Instance()")) {
                name="";
                return;
            }
        }
    }
    switch (index) {
        TESTCASE(0, TestAPI);
        TESTCASE(1, TestSomeCases);
        default:
            name="";
            break;  // needed to end the loop
    }
}

void UTS46Test::TestAPI() {
    IcuTestErrorCode errorCode(*this, "TestAPI");
    UnicodeString result;
    IDNAInfo info;
    UnicodeString input=UNICODE_STRING_SIMPLE("www.eXample.cOm");
    UnicodeString expected=UNICODE_STRING_SIMPLE("www.example.com");
    trans->nameToASCII(input, result, info, errorCode);
    if( !errorCode.logIfFailureAndReset("trans->nameToASCII(www.example.com)") &&
        (info.hasErrors() || result!=expected)
    ) {
        errln("trans->nameToASCII(www.example.com) info.errors=%04lx result matches=%d",
              (long)info.getErrors(), result==expected);
    }
}

// TODO: Test various options combinations, e.g., not STD3 passing through non-LDH ASCII.
// TODO: TestToASCII with a few examples, because the following tests it indirectly.

struct TestCase {
    // Input string and options string (Nontransitional/Transitional/Both).
    const char *s, *o;
    // Expected Unicode result string.
    const char *u;
    uint32_t errors;
};

static const TestCase testCases[]={
    { "www.eXample.cOm", "B",  // all ASCII
      "www.example.com", 0 },
    { "B\\u00FCcher.de", "B",  // u-umlaut
      "b\\u00FCcher.de", 0 },
    { "\\u00D6BB", "B",  // O-umlaut
      "\\u00F6bb", 0 },
    { "fa\\u00DF.de", "N",  // sharp s
      "fa\\u00DF.de", 0 },
    { "fa\\u00DF.de", "T",  // sharp s
      "fass.de", 0 },
    { "XN--fA-hia.dE", "B",  // sharp s in Punycode
      "fa\\u00DF.de", 0 },
    { "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2.com", "N",  // Greek with final sigma
      "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2.com", 0 },
    { "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2.com", "T",  // Greek with final sigma
      "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C3.com", 0 },
    { "xn--nxasmm1c", "B",  // Greek with final sigma in Punycode
      "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2", 0 },
    { "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", "N",  // "Sri" in "Sri Lanka" has a ZWJ
      "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", 0 },
    { "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", "T",  // "Sri" in "Sri Lanka" has a ZWJ
      "www.\\u0DC1\\u0DCA\\u0DBB\\u0DD3.com", 0 },
    { "www.xn--10cl1a0b660p.com", "B",  // "Sri" in Punycode
      "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", 0 },
    { "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC", "N",  // ZWNJ
      "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC", 0 },
    { "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC", "T",  // ZWNJ
      "\\u0646\\u0627\\u0645\\u0647\\u0627\\u06CC", 0 },
    { "xn--mgba3gch31f060k.com", "B",  // ZWNJ in Punycode
      "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC.com", 0 },
    { "a.b\\uFF0Ec\\u3002d\\uFF61", "B",
      "a.b.c.d.", 0 },
    { "U\\u0308.xn--tda", "B",  // U+umlaut.u-umlaut
      "\\u00FC.\\u00FC", 0 },
    { "xn--u-ccb", "B",  // u+umlaut in Punycode
      "xn--u-ccb\\uFFFD", UIDNA_ERROR_INVALID_ACE_LABEL },
    { "a\\u2488com", "B",  // contains 1-dot
      "a\\uFFFDcom", UIDNA_ERROR_DISALLOWED },
    { "xn--a-ecp.ru", "B",  // contains 1-dot in Punycode
      "xn--a-ecp\\uFFFD.ru", UIDNA_ERROR_INVALID_ACE_LABEL },
    { "xn--0.pt", "B",  // invalid Punycode
      "xn--0\\uFFFD.pt", UIDNA_ERROR_PUNYCODE },
    { "xn--a.pt", "B",  // U+0080
      "xn--a\\uFFFD.pt", UIDNA_ERROR_INVALID_ACE_LABEL },
    { "xn--a-\\u00C4.pt", "B",  // invalid Punycode
      "xn--a-\\u00E4.pt", UIDNA_ERROR_PUNYCODE },
    { "\\u65E5\\u672C\\u8A9E\\u3002\\uFF2A\\uFF30", "B",  // Japanese with fullwidth ".jp"
      "\\u65E5\\u672C\\u8A9E.jp", 0 },
    { "\\u2615", "B", "\\u2615", UIDNA_ERROR_BIDI },  // Unicode 4.0 HOT BEVERAGE
    // many deviation characters, test the special mapping code
    { "1.a\\u00DF\\u200C\\u200Db\\u200C\\u200Dc\\u00DF\\u00DF\\u00DF\\u00DFd"
      "\\u03C2\\u03C3\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFe"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFx"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFy"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u0302\\u00DFz", "N",
      "1.a\\u00DF\\u200C\\u200Db\\u200C\\u200Dc\\u00DF\\u00DF\\u00DF\\u00DFd"
      "\\u03C2\\u03C3\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFe"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFx"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFy"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u0302\\u00DFz",
      UIDNA_ERROR_CONTEXTJ },
    { "1.a\\u00DF\\u200C\\u200Db\\u200C\\u200Dc\\u00DF\\u00DF\\u00DF\\u00DFd"
      "\\u03C2\\u03C3\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFe"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFx"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFy"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u0302\\u00DFz", "T",
      "1.assbcssssssssd"
      "\\u03C3\\u03C3sssssssssssssssse"
      "ssssssssssssssssssssx"
      "ssssssssssssssssssssy"
      "sssssssssssssss\\u015Dssz", 0 },
    // "xn--bss" with deviation characters
    { "\\u200Cx\\u200Dn\\u200C-\\u200D-b\\u00DF", "N",
      "\\u200Cx\\u200Dn\\u200C-\\u200D-b\\u00DF", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },
    { "\\u200Cx\\u200Dn\\u200C-\\u200D-b\\u00DF", "T",
      "\\u5919", 0 },
    // "xn--bssffl" written as:
    // 02E3 MODIFIER LETTER SMALL X
    // 034F COMBINING GRAPHEME JOINER (ignored)
    // 2115 DOUBLE-STRUCK CAPITAL N
    // 200B ZERO WIDTH SPACE (ignored)
    // FE63 SMALL HYPHEN-MINUS
    // 00AD SOFT HYPHEN (ignored)
    // FF0D FULLWIDTH HYPHEN-MINUS
    // 180C MONGOLIAN FREE VARIATION SELECTOR TWO (ignored)
    // 212C SCRIPT CAPITAL B
    // FE00 VARIATION SELECTOR-1 (ignored)
    // 017F LATIN SMALL LETTER LONG S
    // 2064 INVISIBLE PLUS (ignored)
    // 1D530 MATHEMATICAL FRAKTUR SMALL S
    // E01EF VARIATION SELECTOR-256 (ignored)
    // FB04 LATIN SMALL LIGATURE FFL
    { "\\u02E3\\u034F\\u2115\\u200B\\uFE63\\u00AD\\uFF0D\\u180C"
      "\\u212C\\uFE00\\u017F\\u2064\\U0001D530\\U000E01EF\\uFB04", "B",
      "\\u5921\\u591E\\u591C\\u5919", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012",
      UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901",
      UIDNA_ERROR_LABEL_TOO_LONG|UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    // label length 63: xn--1234567890123456789012345678901234567890123456789012345-9te
    { "\\u00E41234567890123456789012345678901234567890123456789012345", "B",
      "\\u00E41234567890123456789012345678901234567890123456789012345", 0 },
    { "1234567890\\u00E41234567890123456789012345678901234567890123456", "B",
      "1234567890\\u00E41234567890123456789012345678901234567890123456", UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012",
      UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901",
      UIDNA_ERROR_LABEL_TOO_LONG|UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    // hyphen errors and empty-label errors
    // "xn---q----jra"=="-q--a-umlaut-"
    { "a.b..-q--a-.e", "B", "a.b..-q--a-.e",
      UIDNA_ERROR_EMPTY_LABEL|UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN|
      UIDNA_ERROR_HYPHEN_3_4 },
    { "a.b..-q--\\u00E4-.e", "B", "a.b..-q--\\u00E4-.e",
      UIDNA_ERROR_EMPTY_LABEL|UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN|
      UIDNA_ERROR_HYPHEN_3_4 },
    { "a.b..xn---q----jra.e", "B", "a.b..-q--\\u00E4-.e",
      UIDNA_ERROR_EMPTY_LABEL|UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN|
      UIDNA_ERROR_HYPHEN_3_4 },
    { "a..c", "B", "a..c", UIDNA_ERROR_EMPTY_LABEL },
    { "a.-b.", "B", "a.-b.", UIDNA_ERROR_LEADING_HYPHEN },
    { "a.b-.c", "B", "a.b-.c", UIDNA_ERROR_TRAILING_HYPHEN },
    { "a.-.c", "B", "a.-.c", UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN },
    { "a.bc--de.f", "B", "a.bc--de.f", UIDNA_ERROR_HYPHEN_3_4 },
    { "\\u00E4.\\u00AD.c", "B", "\\u00E4..c", UIDNA_ERROR_EMPTY_LABEL },
    { "\\u00E4.-b.", "B", "\\u00E4.-b.", UIDNA_ERROR_LEADING_HYPHEN },
    { "\\u00E4.b-.c", "B", "\\u00E4.b-.c", UIDNA_ERROR_TRAILING_HYPHEN },
    { "\\u00E4.-.c", "B", "\\u00E4.-.c", UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN },
    { "\\u00E4.bc--de.f", "B", "\\u00E4.bc--de.f", UIDNA_ERROR_HYPHEN_3_4 },
    { "a.b.\\u0308c.d", "B", "a.b.\\uFFFDc.d", UIDNA_ERROR_LEADING_COMBINING_MARK },
    { "a.b.xn--c-bcb.d", "B", "a.b.xn--c-bcb\\uFFFD.d", UIDNA_ERROR_LEADING_COMBINING_MARK },
    // BiDi
    { "A0", "B", "a0", 0 },
    // TODO: revisit BIDI Rule { "0A", "B", "0a", UIDNA_ERROR_BIDI },  // does not start with L/R/AL
    { "a\\u05D0", "B", "a\\u05D0", UIDNA_ERROR_BIDI },  // first dir != last dir
    { "\\u05D0\\u05C7", "B", "\\u05D0\\u05C7", 0 },
    { "\\u05D09\\u05C7", "B", "\\u05D09\\u05C7", 0 },
    { "\\u05D0a\\u05C7", "B", "\\u05D0a\\u05C7", UIDNA_ERROR_BIDI },  // first dir != last dir
    { "\\u05D0\\u05EA", "B", "\\u05D0\\u05EA", 0 },
    { "\\u05D0\\u05F3\\u05EA", "B", "\\u05D0\\u05F3\\u05EA", 0 },
    { "a\\u05D0Tz", "B", "a\\u05D0tz", UIDNA_ERROR_BIDI },  // mixed dir
    { "\\u05D0T\\u05EA", "B", "\\u05D0t\\u05EA", UIDNA_ERROR_BIDI },  // mixed dir
    { "\\u05D07\\u05EA", "B", "\\u05D07\\u05EA", 0 },
    { "\\u05D0\\u0667\\u05EA", "B", "\\u05D0\\u0667\\u05EA", 0 },  // Arabic 7 in the middle
    { "a7\\u0667z", "B", "a7\\u0667z", UIDNA_ERROR_BIDI },  // AN digit in LTR
    { "\\u05D07\\u0667\\u05EA", "B",  // mixed EN/AN digits in RTL
      "\\u05D07\\u0667\\u05EA", UIDNA_ERROR_BIDI },
    // ZWJ
    { "\\u0BB9\\u0BCD\\u200D", "N", "\\u0BB9\\u0BCD\\u200D", UIDNA_ERROR_BIDI },  // Virama+ZWJ
    { "\\u0BB9\\u200D", "N", "\\u0BB9\\u200D", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },  // no Virama
    { "\\u200D", "N", "\\u200D", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },  // no Virama
    // ZWNJ
    { "\\u0BB9\\u0BCD\\u200C", "N", "\\u0BB9\\u0BCD\\u200C", UIDNA_ERROR_BIDI },  // Virama+ZWNJ
    { "\\u0BB9\\u200C", "N", "\\u0BB9\\u200C", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },  // no Virama
    { "\\u200C", "N", "\\u200C", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },  // no Virama
    { "\\u0644\\u0670\\u200C\\u06ED\\u06EF", "N",  // Joining types D T ZWNJ T R
      "\\u0644\\u0670\\u200C\\u06ED\\u06EF", 0 },
    { "\\u0644\\u0670\\u200C\\u06EF", "N",  // D T ZWNJ R
      "\\u0644\\u0670\\u200C\\u06EF", 0 },
    { "\\u0644\\u200C\\u06ED\\u06EF", "N",  // D ZWNJ T R
      "\\u0644\\u200C\\u06ED\\u06EF", 0 },
    { "\\u0644\\u200C\\u06EF", "N",  // D ZWNJ R
      "\\u0644\\u200C\\u06EF", 0 },
    { "\\u0644\\u0670\\u200C\\u06ED", "N",  // D T ZWNJ T
      "\\u0644\\u0670\\u200C\\u06ED", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },
    { "\\u06EF\\u200C\\u06EF", "N",  // R ZWNJ R
      "\\u06EF\\u200C\\u06EF", UIDNA_ERROR_CONTEXTJ },
    { "\\u0644\\u200C", "N",  // D ZWNJ
      "\\u0644\\u200C", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },
    // { "", "B",
    //   "", 0 },
};

void UTS46Test::TestSomeCases() {
    IcuTestErrorCode errorCode(*this, "TestSomeCases");
    int32_t i;
    for(i=0; i<LENGTHOF(testCases); ++i) {
        const TestCase &testCase=testCases[i];
        UnicodeString input(ctou(testCase.s));
        UnicodeString expected(ctou(testCase.u));
        // ToASCII/ToUnicode, transitional/nontransitional
        UnicodeString aT, uT, aN, uN;
        IDNAInfo aTInfo, uTInfo, aNInfo, uNInfo;
        trans->nameToASCII(input, aT, aTInfo, errorCode);
        trans->nameToUnicode(input, uT, uTInfo, errorCode);
        nontrans->nameToASCII(input, aN, aNInfo, errorCode);
        nontrans->nameToUnicode(input, uN, uNInfo, errorCode);
        if(errorCode.logIfFailureAndReset("first-level processing [%d/%s] %s",
                                          (int)i, testCase.o, testCase.s)
        ) {
            continue;
        }
        // ToUnicode does not set length errors.
        uint32_t uniErrors=testCase.errors&~
            (UIDNA_ERROR_EMPTY_LABEL|
             UIDNA_ERROR_LABEL_TOO_LONG|
             UIDNA_ERROR_DOMAIN_NAME_TOO_LONG);
        char mode=testCase.o[0];
        if(mode=='B' || mode=='N') {
            if(uNInfo.getErrors()!=uniErrors) {
                errln("N.nameToUnicode([%d] %s) unexpected errors %04lx",
                      (int)i, testCase.s, (long)uNInfo.getErrors());
                continue;
            }
            if(uN!=expected) {
                char buffer[300];
                prettify(uN).extract(0, 0x7fffffff, buffer, 300);
                errln("N.nameToUnicode([%d] %s) unexpected string %s",
                      (int)i, testCase.s, buffer);
                continue;
            }
        }
        if(mode=='B' || mode=='T') {
            if(uTInfo.getErrors()!=uniErrors) {
                errln("T.nameToUnicode([%d] %s) unexpected errors %04lx",
                      (int)i, testCase.s, (long)uTInfo.getErrors());
                continue;
            }
            if(uT!=expected) {
                char buffer[300];
                prettify(uT).extract(0, 0x7fffffff, buffer, 300);
                errln("T.nameToUnicode([%d] %s) unexpected string %s",
                      (int)i, testCase.s, buffer);
                continue;
            }
        }
        // labelToUnicode
        // second-level processing
        // UTF-8 if we have std::string
        // ToASCII is all-ASCII if no errors
    }
}

#endif  // UCONFIG_NO_IDNA
