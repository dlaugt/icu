// © 2018 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING && !UPRV_INCOMPLETE_CPP11_SUPPORT

#include "numbertest.h"
#include "numparse_unisets.h"
#include "unicode/dcfmtsym.h"

using icu::numparse::impl::unisets::get;

void UniSetsTest::runIndexedTest(int32_t index, UBool exec, const char*&name, char*) {
    if (exec) {
        logln("TestSuite UniSetsTest: ");
    }
    TESTCASE_AUTO_BEGIN;
        TESTCASE_AUTO(testSetCoverage);
    TESTCASE_AUTO_END;
}

void UniSetsTest::testSetCoverage() {
    UErrorCode status = U_ZERO_ERROR;

    // Lenient comma/period should be supersets of strict comma/period;
    // it also makes the coverage logic cheaper.
    assertTrue(
            "COMMA should be superset of STRICT_COMMA",
            get(unisets::COMMA)->containsAll(*get(unisets::STRICT_COMMA)));
    assertTrue(
            "PERIOD should be superset of STRICT_PERIOD",
            get(unisets::PERIOD)->containsAll(*get(unisets::STRICT_PERIOD)));

    UnicodeSet decimals;
    decimals.addAll(*get(unisets::STRICT_COMMA));
    decimals.addAll(*get(unisets::STRICT_PERIOD));
    decimals.freeze();
    UnicodeSet grouping;
    grouping.addAll(decimals);
    grouping.addAll(*get(unisets::OTHER_GROUPING_SEPARATORS));
    decimals.freeze();

    const UnicodeSet &plusSign = *get(unisets::PLUS_SIGN);
    const UnicodeSet &minusSign = *get(unisets::MINUS_SIGN);
    const UnicodeSet &percent = *get(unisets::PERCENT_SIGN);
    const UnicodeSet &permille = *get(unisets::PERMILLE_SIGN);
    const UnicodeSet &infinity = *get(unisets::INFINITY_KEY);
    const UnicodeSet &nanLead = *get(unisets::NAN_LEAD);
    const UnicodeSet &scientificLead = *get(unisets::SCIENTIFIC_LEAD);

    int32_t localeCount;
    const Locale* allAvailableLocales = Locale::getAvailableLocales(localeCount);
    for (int32_t i = 0; i < localeCount; i++) {
        Locale locale = allAvailableLocales[i];
        DecimalFormatSymbols dfs(locale, status);
        UnicodeString localeName;
        locale.getDisplayName(localeName);
        assertSuccess(UnicodeString("Making DFS for ") + localeName, status);

#define ASSERT_IN_SET(name, foo) assertInSet(localeName, UnicodeString("" #name ""), name, foo)
        ASSERT_IN_SET(decimals, dfs.getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol));
        ASSERT_IN_SET(grouping, dfs.getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol));
        ASSERT_IN_SET(plusSign, dfs.getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol));
        ASSERT_IN_SET(minusSign, dfs.getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol));
        ASSERT_IN_SET(percent, dfs.getConstSymbol(DecimalFormatSymbols::kPercentSymbol));
        ASSERT_IN_SET(permille, dfs.getConstSymbol(DecimalFormatSymbols::kPerMillSymbol));
        ASSERT_IN_SET(infinity, dfs.getConstSymbol(DecimalFormatSymbols::kInfinitySymbol));
        ASSERT_IN_SET(nanLead, dfs.getConstSymbol(DecimalFormatSymbols::kNaNSymbol).char32At(0));
        ASSERT_IN_SET(nanLead,
                u_foldCase(dfs.getConstSymbol(DecimalFormatSymbols::kNaNSymbol).char32At(0), 0));
        ASSERT_IN_SET(scientificLead,
                u_foldCase(dfs.getConstSymbol(DecimalFormatSymbols::kExponentialSymbol).char32At(0), 0));
    }
}

void UniSetsTest::assertInSet(const UnicodeString &localeName, const UnicodeString &setName,
                              const UnicodeSet &set, const UnicodeString &str) {
    if (str.countChar32(0, str.length()) != 1) {
        // Ignore locale strings with more than one code point (usually a bidi mark)
        return;
    }
    assertInSet(localeName, setName, set, str.char32At(0));
}

void UniSetsTest::assertInSet(const UnicodeString &localeName, const UnicodeString &setName,
                              const UnicodeSet &set, UChar32 cp) {
    // If this test case fails, add the specified code point to the corresponding set in
    // UnicodeSetStaticCache.java and numparse_unisets.cpp
    assertTrue(
            localeName + UnicodeString(u" ") + UnicodeString(cp) + UnicodeString(u" is missing in ") +
            setName, set.contains(cp));
}


#endif
