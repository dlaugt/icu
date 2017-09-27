// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef NUMBERFORMAT_AFFIXPATTERNUTILS_H
#define NUMBERFORMAT_AFFIXPATTERNUTILS_H

#include <cstdint>
#include "number_types.h"
#include "unicode/stringpiece.h"
#include "unicode/unistr.h"
#include "number_stringbuilder.h"

U_NAMESPACE_BEGIN namespace number {
namespace impl {

enum AffixPatternState {
    STATE_BASE = 0,
    STATE_FIRST_QUOTE = 1,
    STATE_INSIDE_QUOTE = 2,
    STATE_AFTER_QUOTE = 3,
    STATE_FIRST_CURR = 4,
    STATE_SECOND_CURR = 5,
    STATE_THIRD_CURR = 6,
    STATE_FOURTH_CURR = 7,
    STATE_FIFTH_CURR = 8,
    STATE_OVERFLOW_CURR = 9
};

// enum AffixPatternType defined in internals.h

struct AffixTag {
    int32_t offset;
    UChar32 codePoint;
    AffixPatternState state;
    AffixPatternType type;

    AffixTag() : offset(0), state(STATE_BASE) {}

    AffixTag(int32_t offset) : offset(offset) {}

    AffixTag(int32_t offset, UChar32 codePoint, AffixPatternState state, AffixPatternType type)
        : offset(offset), codePoint(codePoint), state(state), type(type)
        {}
};

class SymbolProvider {
  public:
    // TODO: Could this be more efficient if it returned by reference?
    virtual UnicodeString getSymbol(AffixPatternType type) const = 0;
};

/**
 * Performs manipulations on affix patterns: the prefix and suffix strings associated with a decimal
 * format pattern. For example:
 *
 * <table>
 * <tr><th>Affix Pattern</th><th>Example Unescaped (Formatted) String</th></tr>
 * <tr><td>abc</td><td>abc</td></tr>
 * <tr><td>ab-</td><td>ab−</td></tr>
 * <tr><td>ab'-'</td><td>ab-</td></tr>
 * <tr><td>ab''</td><td>ab'</td></tr>
 * </table>
 *
 * To manually iterate over tokens in a literal string, use the following pattern, which is designed
 * to be efficient.
 *
 * <pre>
 * long tag = 0L;
 * while (AffixPatternUtils.hasNext(tag, patternString)) {
 *   tag = AffixPatternUtils.nextToken(tag, patternString);
 *   int typeOrCp = AffixPatternUtils.getTypeOrCp(tag);
 *   switch (typeOrCp) {
 *     case AffixPatternUtils.TYPE_MINUS_SIGN:
 *       // Current token is a minus sign.
 *       break;
 *     case AffixPatternUtils.TYPE_PLUS_SIGN:
 *       // Current token is a plus sign.
 *       break;
 *     case AffixPatternUtils.TYPE_PERCENT:
 *       // Current token is a percent sign.
 *       break;
 *     // ... other types ...
 *     default:
 *       // Current token is an arbitrary code point.
 *       // The variable typeOrCp is the code point.
 *       break;
 *   }
 * }
 * </pre>
 */
class AffixUtils {

  public:

    /**
     * Estimates the number of code points present in an unescaped version of the affix pattern string
     * (one that would be returned by {@link #unescape}), assuming that all interpolated symbols
     * consume one code point and that currencies consume as many code points as their symbol width.
     * Used for computing padding width.
     *
     * @param patternString The original string whose width will be estimated.
     * @return The length of the unescaped string.
     */
    static int32_t estimateLength(const CharSequence &patternString, UErrorCode &status);

    /**
     * Takes a string and escapes (quotes) characters that have special meaning in the affix pattern
     * syntax. This function does not reverse-lookup symbols.
     *
     * <p>Example input: "-$x"; example output: "'-'$x"
     *
     * @param input The string to be escaped.
     * @return The resulting UnicodeString.
     */
    static UnicodeString escape(const CharSequence &input);

    static Field getFieldForType(AffixPatternType type);

    /**
     * Executes the unescape state machine. Replaces the unquoted characters "-", "+", "%", "‰", and
     * "¤" with the corresponding symbols provided by the {@link SymbolProvider}, and inserts the
     * result into the NumberStringBuilder at the requested location.
     *
     * <p>Example input: "'-'¤x"; example output: "-$x"
     *
     * @param affixPattern The original string to be unescaped.
     * @param output The NumberStringBuilder to mutate with the result.
     * @param position The index into the NumberStringBuilder to insert the the string.
     * @param provider An object to generate locale symbols.
     */
    static int32_t
    unescape(const CharSequence &affixPattern, NumberStringBuilder &output, int32_t position,
             const SymbolProvider &provider, UErrorCode &status);

    /**
   * Sames as {@link #unescape}, but only calculates the code point count.  More efficient than {@link #unescape}
   * if you only need the length but not the string itself.
     *
     * @param affixPattern The original string to be unescaped.
     * @param provider An object to generate locale symbols.
     * @return The same return value as if you called {@link #unescape}.
     */
    static int32_t unescapedCodePointCount(const CharSequence &affixPattern,
                                           const SymbolProvider &provider, UErrorCode &status);

    /**
     * Checks whether the given affix pattern contains at least one token of the given type, which is
     * one of the constants "TYPE_" in {@link AffixPatternUtils}.
     *
     * @param affixPattern The affix pattern to check.
     * @param type The token type.
     * @return true if the affix pattern contains the given token type; false otherwise.
     */
    static bool
    containsType(const CharSequence &affixPattern, AffixPatternType type, UErrorCode &status);

    /**
     * Checks whether the specified affix pattern has any unquoted currency symbols ("¤").
     *
     * @param affixPattern The string to check for currency symbols.
     * @return true if the literal has at least one unquoted currency symbol; false otherwise.
     */
    static bool hasCurrencySymbols(const CharSequence &affixPattern, UErrorCode &status);

    /**
     * Replaces all occurrences of tokens with the given type with the given replacement char.
     *
     * @param affixPattern The source affix pattern (does not get modified).
     * @param type The token type.
     * @param replacementChar The char to substitute in place of chars of the given token type.
     * @return A string containing the new affix pattern.
     */
    static UnicodeString
    replaceType(const CharSequence &affixPattern, AffixPatternType type, char16_t replacementChar,
                UErrorCode &status);

    /**
     * Returns the next token from the affix pattern.
     *
     * @param tag A bitmask used for keeping track of state from token to token. The initial value
     *     should be 0L.
     * @param patternString The affix pattern.
     * @return The bitmask tag to pass to the next call of this method to retrieve the following token
     *     (never negative), or -1 if there were no more tokens in the affix pattern.
     * @see #hasNext
     */
    static AffixTag nextToken(AffixTag tag, const CharSequence &patternString, UErrorCode &status);

    /**
     * Returns whether the affix pattern string has any more tokens to be retrieved from a call to
     * {@link #nextToken}.
     *
     * @param tag The bitmask tag of the previous token, as returned by {@link #nextToken}.
     * @param string The affix pattern.
     * @return true if there are more tokens to consume; false otherwise.
     */
    static bool hasNext(const AffixTag &tag, const CharSequence &string);

  private:
    /**
     * Encodes the given values into a tag struct.
     * The order of the arguments is consistent with Java, but the order of the stored
     * fields is not necessarily the same.
     */
    static inline AffixTag
    makeTag(int32_t offset, AffixPatternType type, AffixPatternState state, UChar32 cp) {
        return {offset, cp, state, type};
    }
};

} // namespace impl
} // namespace number
U_NAMESPACE_END


#endif //NUMBERFORMAT_AFFIXPATTERNUTILS_H
