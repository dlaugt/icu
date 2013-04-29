/*
*******************************************************************************
* Copyright (C) 2013, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* collationruleparser.h
*
* created on: 2013apr10
* created by: Markus W. Scherer
*/

#ifndef __COLLATIONRULEPARSER_H__
#define __COLLATIONRULEPARSER_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uniset.h"
#include "unicode/unistr.h"

struct UParseError;

U_NAMESPACE_BEGIN

struct CollationData;

class Locale;
class Normalizer2;

struct CollationSettings;

class U_I18N_API CollationRuleParser : public UMemory {
public:
    /** Sentinel value. The token integer should be zero. */
    static const int32_t NO_RELATION = 0;
    static const int32_t PRIMARY = 1;
    static const int32_t SECONDARY = 2;
    static const int32_t TERTIARY = 3;
    static const int32_t QUATERNARY = 4;
    /** Used for reset-at (without DIFF) and identical relation (with DIFF). */
    static const int32_t IDENTICAL = 5;
    // Strength values 6 & 7 are unused.
    static const int32_t STRENGTH_MASK = 7;

    /** Special reset positions. */
    enum Position {
        FIRST_TERTIARY_IGNORABLE,
        LAST_TERTIARY_IGNORABLE,
        FIRST_SECONDARY_IGNORABLE,
        LAST_SECONDARY_IGNORABLE,
        FIRST_PRIMARY_IGNORABLE,
        LAST_PRIMARY_IGNORABLE,
        FIRST_VARIABLE,
        LAST_VARIABLE,
        FIRST_IMPLICIT,
        LAST_IMPLICIT,
        FIRST_REGULAR,
        LAST_REGULAR,
        FIRST_TRAILING,
        LAST_TRAILING
    };

    /**
     * First character of contractions that encode special reset positions.
     * U+FFFE cannot be tailored via rule syntax.
     *
     * The second contraction character is POS_BASE + Position.
     */
    static const UChar POS_LEAD = 0xfffe;
    /**
     * Base for the second character of contractions that encode special reset positions.
     * Braille characters U+28xx are printable and normalization-inert.
     * @see POS_LEAD
     */
    static const UChar POS_BASE = 0x2800;

    class U_I18N_API Sink : public UObject {
    public:
        virtual ~Sink();
        /**
         * Adds a reset.
         * strength=IDENTICAL for &str.
         * strength=PRIMARY/SECONDARY/TERTIARY for &[before n]str where n=1/2/3.
         */
        virtual void addReset(int32_t strength, const UnicodeString &str,
                              const char *&errorReason, UErrorCode &errorCode) = 0;
        /**
         * Adds a relation with strength and prefix | str / extension.
         */
        virtual void addRelation(int32_t strength, const UnicodeString &prefix,
                                 const UnicodeString &str, const UnicodeString &extension,
                                 const char *&errorReason, UErrorCode &errorCode) = 0;

        virtual void suppressContractions(const UnicodeSet &set,
                                          const char *&errorReason, UErrorCode &errorCode) = 0;
    };

    class U_I18N_API Importer : public UObject {
    public:
        virtual ~Importer();
        virtual const UnicodeString *getRules(
                const char *localeID, const char *collationType,
                const char *&errorReason, UErrorCode &errorCode) = 0;
    };

    /**
     * Constructor.
     * The Sink must be set before parsing.
     * The Importer can be set, otherwise [import locale] syntax is not supported.
     */
    CollationRuleParser(UErrorCode &errorCode);
    ~CollationRuleParser();

    /**
     * Sets the pointer to a Sink object.
     * The pointer is aliased: Pointer copy without cloning or taking ownership.
     */
    void setSink(Sink *sinkAlias) {
        sink = sinkAlias;
    }

    /**
     * Sets the pointer to an Importer object.
     * The pointer is aliased: Pointer copy without cloning or taking ownership.
     */
    void setImporter(Importer *importerAlias) {
        importer = importerAlias;
    }

    void parse(const UnicodeString &ruleString,
               const CollationData *base,
               CollationSettings &outSettings,
               UParseError *outParseError,
               UErrorCode &errorCode);

    const char *getErrorReason() const { return errorReason; }

    UBool modifiesSettings() const { return TRUE; }  // TODO
    UBool modifiesMappings() const { return TRUE; }  // TODO

    const UnicodeSet &getOptimizeSet() const { return optimizeSet; }

    /**
     * Gets a script or reorder code from its string representation.
     * @return the script/reorder code, or
     * -1==UCHAR_INVALID_CODE==USCRIPT_INVALID_CODE if not recognized
     */
    static int32_t getReorderCode(const char *word);

private:
    void parse(const UnicodeString &ruleString, UErrorCode &errorCode);
    void parseRuleChain(UErrorCode &errorCode);
    int32_t parseResetAndPosition(UErrorCode &errorCode);
    int32_t parseRelationOperator(UErrorCode &errorCode);
    void parseRelationStrings(int32_t strength, int32_t i, UErrorCode &errorCode);
    void parseStarredCharacters(int32_t strength, int32_t i, UErrorCode &errorCode);
    int32_t parseTailoringString(int32_t i, UErrorCode &errorCode);
    int32_t parseString(int32_t i, UBool allowDash, UErrorCode &errorCode);

    /**
     * Sets str to a contraction of U+FFFE and (U+2800 + Position).
     * @return rule index after the special reset position
     */
    int32_t parseSpecialPosition(int32_t i, UErrorCode &errorCode);
    void parseSetting(UErrorCode &errorCode);
    void parseReordering(UErrorCode &errorCode);
    static UColAttributeValue getOnOffValue(const UnicodeString &s);

    int32_t parseUnicodeSet(int32_t i, UnicodeSet &set, UErrorCode &errorCode);
    int32_t readWords(int32_t i);
    int32_t skipComment(int32_t i) const;

    void resetTailoringStrings();

    void setParseError(const char *reason, UErrorCode &errorCode);

    /**
     * ASCII [:P:] and [:S:]:
     * [\u0021-\u002F \u003A-\u0040 \u005B-\u0060 \u007B-\u007E]
     */
    static UBool isSyntaxChar(UChar32 c);
    int32_t skipWhiteSpace(int32_t i) const;

    const Normalizer2 &nfd, &fcc;

    const UnicodeString *rules;
    const CollationData *baseData;
    CollationSettings *settings;
    UParseError *parseError;
    const char *errorReason;

    Sink *sink;
    Importer *importer;

    int32_t ruleIndex;

    UnicodeString raw;
    // Tailoring strings are normalized to FCC:
    // We need a canonical form so that we can find duplicates,
    // and we want to tailor only strings that pass the FCD test.
    // FCD itself is not a unique form.
    // FCC also preserves most composites which helps with storing
    // tokenized rules in a compact form.
    UnicodeString prefix, str, extension;

    UnicodeSet optimizeSet;
};

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
#endif  // __COLLATIONRULEPARSER_H__
