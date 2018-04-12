// © 2018 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING && !UPRV_INCOMPLETE_CPP11_SUPPORT
#ifndef __NUMPARSE_AFFIXES_H__
#define __NUMPARSE_AFFIXES_H__

#include "numparse_types.h"
#include "numparse_symbols.h"
#include "numparse_currency.h"
#include "number_affixutils.h"
#include "number_currencysymbols.h"

#include <array>

U_NAMESPACE_BEGIN namespace numparse {
namespace impl {

// Forward-declaration of implementation classes for friending
class AffixPatternMatcherBuilder;
class AffixPatternMatcher;

using ::icu::number::impl::AffixPatternProvider;
using ::icu::number::impl::TokenConsumer;
using ::icu::number::impl::CurrencySymbols;


class CodePointMatcher : public NumberParseMatcher, public UMemory {
  public:
    CodePointMatcher() = default;  // WARNING: Leaves the object in an unusable state

    CodePointMatcher(UChar32 cp);

    bool match(StringSegment& segment, ParsedNumber& result, UErrorCode& status) const override;

    bool smokeTest(const StringSegment& segment) const override;

    UnicodeString toString() const override;

  private:
    UChar32 fCp;
};


/**
 * A warehouse to retain ownership of CodePointMatchers.
 */
class CodePointMatcherWarehouse : public UMemory {
  private:
    static constexpr int32_t CODE_POINT_STACK_CAPACITY = 5; // Number of entries directly on the stack
    static constexpr int32_t CODE_POINT_BATCH_SIZE = 10; // Number of entries per heap allocation

  public:
    CodePointMatcherWarehouse();

    // A custom destructor is needed to free the memory from MaybeStackArray.
    // A custom move constructor and move assignment seem to be needed because of the custom destructor.

    ~CodePointMatcherWarehouse();

    CodePointMatcherWarehouse(CodePointMatcherWarehouse&& src) U_NOEXCEPT;

    CodePointMatcherWarehouse& operator=(CodePointMatcherWarehouse&& src) U_NOEXCEPT;

    NumberParseMatcher& nextCodePointMatcher(UChar32 cp);

  private:
    std::array<CodePointMatcher, CODE_POINT_STACK_CAPACITY> codePoints; // By value
    MaybeStackArray<CodePointMatcher*, 3> codePointsOverflow; // On heap in "batches"
    int32_t codePointCount; // Total for both the ones by value and on heap
    int32_t codePointNumBatches; // Number of batches in codePointsOverflow
};


struct AffixTokenMatcherSetupData {
    const CurrencySymbols& currencySymbols;
    const DecimalFormatSymbols& dfs;
    IgnorablesMatcher& ignorables;
    const Locale& locale;
};


/**
 * Small helper class that generates matchers for individual tokens for AffixPatternMatcher.
 *
 * In Java, this is called AffixTokenMatcherFactory (a "factory"). However, in C++, it is called a
 * "warehouse", because in addition to generating the matchers, it also retains ownership of them. The
 * warehouse must stay in scope for the whole lifespan of the AffixPatternMatcher that uses matchers from
 * the warehouse.
 *
 * @author sffc
 */
class AffixTokenMatcherWarehouse : public UMemory {
  public:
    AffixTokenMatcherWarehouse() = default;  // WARNING: Leaves the object in an unusable state

    AffixTokenMatcherWarehouse(const AffixTokenMatcherSetupData* setupData);

    NumberParseMatcher& minusSign();

    NumberParseMatcher& plusSign();

    NumberParseMatcher& percent();

    NumberParseMatcher& permille();

    NumberParseMatcher& currency(UErrorCode& status);

    IgnorablesMatcher& ignorables();

    NumberParseMatcher& nextCodePointMatcher(UChar32 cp);

  private:
    // NOTE: The following field may be unsafe to access after construction is done!
    const AffixTokenMatcherSetupData* fSetupData;

    // NOTE: These are default-constructed and should not be used until initialized.
    MinusSignMatcher fMinusSign;
    PlusSignMatcher fPlusSign;
    PercentMatcher fPercent;
    PermilleMatcher fPermille;
    CombinedCurrencyMatcher fCurrency;

    // Use a child class for code point matchers, since it requires non-default operators.
    CodePointMatcherWarehouse fCodePoints;

    friend class AffixPatternMatcherBuilder;
    friend class AffixPatternMatcher;
};


class AffixPatternMatcherBuilder : public TokenConsumer, public MutableMatcherCollection {
  public:
    AffixPatternMatcherBuilder(const UnicodeString& pattern, AffixTokenMatcherWarehouse& warehouse,
                               IgnorablesMatcher* ignorables);

    void consumeToken(::icu::number::impl::AffixPatternType type, UChar32 cp, UErrorCode& status) override;

    /** NOTE: You can build only once! */
    AffixPatternMatcher build();

  private:
    ArraySeriesMatcher::MatcherArray fMatchers;
    int32_t fMatchersLen;
    int32_t fLastTypeOrCp;

    const UnicodeString& fPattern;
    AffixTokenMatcherWarehouse& fWarehouse;
    IgnorablesMatcher* fIgnorables;

    void addMatcher(NumberParseMatcher& matcher) override;
};


class AffixPatternMatcher : public ArraySeriesMatcher {
  public:
    AffixPatternMatcher() = default;  // WARNING: Leaves the object in an unusable state

    static AffixPatternMatcher fromAffixPattern(const UnicodeString& affixPattern,
                                                AffixTokenMatcherWarehouse& warehouse,
                                                parse_flags_t parseFlags, bool* success,
                                                UErrorCode& status);

    UnicodeString getPattern() const;

    bool operator==(const AffixPatternMatcher& other) const;

  private:
    CompactUnicodeString<4> fPattern;

    AffixPatternMatcher(MatcherArray& matchers, int32_t matchersLen, const UnicodeString& pattern);

    friend class AffixPatternMatcherBuilder;
};


class AffixMatcher : public NumberParseMatcher, public UMemory {
  public:
    AffixMatcher() = default;  // WARNING: Leaves the object in an unusable state

    AffixMatcher(AffixPatternMatcher* prefix, AffixPatternMatcher* suffix, result_flags_t flags);

    bool match(StringSegment& segment, ParsedNumber& result, UErrorCode& status) const override;

    void postProcess(ParsedNumber& result) const override;

    bool smokeTest(const StringSegment& segment) const override;

    int8_t compareTo(const AffixMatcher& rhs) const;

    UnicodeString toString() const override;

  private:
    AffixPatternMatcher* fPrefix;
    AffixPatternMatcher* fSuffix;
    result_flags_t fFlags;
};


/**
 * A C++-only class to retain ownership of the AffixMatchers needed for parsing.
 */
class AffixMatcherWarehouse {
  public:
    AffixMatcherWarehouse() = default;  // WARNING: Leaves the object in an unusable state

    AffixMatcherWarehouse(AffixTokenMatcherWarehouse* tokenWarehouse);

    void createAffixMatchers(const AffixPatternProvider& patternInfo, MutableMatcherCollection& output,
                             const IgnorablesMatcher& ignorables, parse_flags_t parseFlags,
                             UErrorCode& status);

  private:
    // 9 is the limit: positive, zero, and negative, each with prefix, suffix, and prefix+suffix
    AffixMatcher fAffixMatchers[9];
    // 6 is the limit: positive, zero, and negative, a prefix and a suffix for each
    AffixPatternMatcher fAffixPatternMatchers[6];
    // Reference to the warehouse for tokens used by the AffixPatternMatchers
    AffixTokenMatcherWarehouse* fTokenWarehouse;

    friend class AffixMatcher;

    static bool isInteresting(const AffixPatternProvider& patternInfo, const IgnorablesMatcher& ignorables,
                              parse_flags_t parseFlags, UErrorCode& status);
};


} // namespace impl
} // namespace numparse
U_NAMESPACE_END

#endif //__NUMPARSE_AFFIXES_H__
#endif /* #if !UCONFIG_NO_FORMATTING */
