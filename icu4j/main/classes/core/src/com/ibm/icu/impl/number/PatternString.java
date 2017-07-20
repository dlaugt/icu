// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.impl.number;

import java.math.BigDecimal;

import com.ibm.icu.impl.number.formatters.PaddingFormat;
import com.ibm.icu.impl.number.formatters.PaddingFormat.PadPosition;
import com.ibm.icu.text.DecimalFormatSymbols;

/**
 * Handles parsing and creation of the compact pattern string representation of a decimal format.
 */
public class PatternString {

  /**
   * Parses a pattern string into a new property bag.
   *
   * @param pattern The pattern string, like "#,##0.00"
   * @param ignoreRounding Whether to leave out rounding information (minFrac, maxFrac, and rounding
   *     increment) when parsing the pattern. This may be desirable if a custom rounding mode, such
   *     as CurrencyUsage, is to be used instead. One of {@link #IGNORE_ROUNDING_ALWAYS}, {@link
   *     #IGNORE_ROUNDING_IF_CURRENCY}, or {@link #IGNORE_ROUNDING_NEVER}.
   * @return A property bag object.
   * @throws IllegalArgumentException If there is a syntax error in the pattern string.
   */
  public static Properties parseToProperties(String pattern, int ignoreRounding) {
    Properties properties = new Properties();
    parse(pattern, properties, ignoreRounding);
    return properties;
  }

  public static Properties parseToProperties(String pattern) {
    return parseToProperties(pattern, PatternString.IGNORE_ROUNDING_NEVER);
  }

  /**
   * Parses a pattern string into an existing property bag. All properties that can be encoded into
   * a pattern string will be overwritten with either their default value or with the value coming
   * from the pattern string. Properties that cannot be encoded into a pattern string, such as
   * rounding mode, are not modified.
   *
   * @param pattern The pattern string, like "#,##0.00"
   * @param properties The property bag object to overwrite.
   * @param ignoreRounding Whether to leave out rounding information (minFrac, maxFrac, and rounding
   *     increment) when parsing the pattern. This may be desirable if a custom rounding mode, such
   *     as CurrencyUsage, is to be used instead. One of {@link #IGNORE_ROUNDING_ALWAYS}, {@link
   *     #IGNORE_ROUNDING_IF_CURRENCY}, or {@link #IGNORE_ROUNDING_NEVER}.
   * @throws IllegalArgumentException If there was a syntax error in the pattern string.
   */
  public static void parseToExistingProperties(
      String pattern, Properties properties, int ignoreRounding) {
    parse(pattern, properties, ignoreRounding);
  }

  public static void parseToExistingProperties(String pattern, Properties properties) {
    parseToExistingProperties(pattern, properties, PatternString.IGNORE_ROUNDING_NEVER);
  }

  /**
   * Creates a pattern string from a property bag.
   *
   * <p>Since pattern strings support only a subset of the functionality available in a property
   * bag, a new property bag created from the string returned by this function may not be the same
   * as the original property bag.
   *
   * @param properties The property bag to serialize.
   * @return A pattern string approximately serializing the property bag.
   */
  public static String propertiesToString(Properties properties) {
    StringBuilder sb = new StringBuilder();

    // Convenience references
    // The Math.min() calls prevent DoS
    int dosMax = 100;
    int groupingSize = Math.min(properties.getSecondaryGroupingSize(), dosMax);
    int firstGroupingSize = Math.min(properties.getGroupingSize(), dosMax);
    int paddingWidth = Math.min(properties.getFormatWidth(), dosMax);
    PadPosition paddingLocation = properties.getPadPosition();
    String paddingString = properties.getPadString();
    int minInt = Math.max(Math.min(properties.getMinimumIntegerDigits(), dosMax), 0);
    int maxInt = Math.min(properties.getMaximumIntegerDigits(), dosMax);
    int minFrac = Math.max(Math.min(properties.getMinimumFractionDigits(), dosMax), 0);
    int maxFrac = Math.min(properties.getMaximumFractionDigits(), dosMax);
    int minSig = Math.min(properties.getMinimumSignificantDigits(), dosMax);
    int maxSig = Math.min(properties.getMaximumSignificantDigits(), dosMax);
    boolean alwaysShowDecimal = properties.getDecimalSeparatorAlwaysShown();
    int exponentDigits = Math.min(properties.getMinimumExponentDigits(), dosMax);
    boolean exponentShowPlusSign = properties.getExponentSignAlwaysShown();
    String pp = properties.getPositivePrefix();
    String ppp = properties.getPositivePrefixPattern();
    String ps = properties.getPositiveSuffix();
    String psp = properties.getPositiveSuffixPattern();
    String np = properties.getNegativePrefix();
    String npp = properties.getNegativePrefixPattern();
    String ns = properties.getNegativeSuffix();
    String nsp = properties.getNegativeSuffixPattern();

    // Prefixes
    if (ppp != null) sb.append(ppp);
    AffixPatternUtils.escape(pp, sb);
    int afterPrefixPos = sb.length();

    // Figure out the grouping sizes.
    int grouping1, grouping2, grouping;
    if (groupingSize != Math.min(dosMax, Properties.DEFAULT_SECONDARY_GROUPING_SIZE)
        && firstGroupingSize != Math.min(dosMax, Properties.DEFAULT_GROUPING_SIZE)
        && groupingSize != firstGroupingSize) {
      grouping = groupingSize;
      grouping1 = groupingSize;
      grouping2 = firstGroupingSize;
    } else if (groupingSize != Math.min(dosMax, Properties.DEFAULT_SECONDARY_GROUPING_SIZE)) {
      grouping = groupingSize;
      grouping1 = 0;
      grouping2 = groupingSize;
    } else if (firstGroupingSize != Math.min(dosMax, Properties.DEFAULT_GROUPING_SIZE)) {
      grouping = groupingSize;
      grouping1 = 0;
      grouping2 = firstGroupingSize;
    } else {
      grouping = 0;
      grouping1 = 0;
      grouping2 = 0;
    }
    int groupingLength = grouping1 + grouping2 + 1;

    // Figure out the digits we need to put in the pattern.
    BigDecimal roundingInterval = properties.getRoundingIncrement();
    StringBuilder digitsString = new StringBuilder();
    int digitsStringScale = 0;
    if (maxSig != Math.min(dosMax, Properties.DEFAULT_MAXIMUM_SIGNIFICANT_DIGITS)) {
      // Significant Digits.
      while (digitsString.length() < minSig) {
        digitsString.append('@');
      }
      while (digitsString.length() < maxSig) {
        digitsString.append('#');
      }
    } else if (roundingInterval != Properties.DEFAULT_ROUNDING_INCREMENT) {
      // Rounding Interval.
      digitsStringScale = -roundingInterval.scale();
      // TODO: Check for DoS here?
      String str = roundingInterval.scaleByPowerOfTen(roundingInterval.scale()).toPlainString();
      if (str.charAt(0) == '\'') {
        // TODO: Unsupported operation exception or fail silently?
        digitsString.append(str, 1, str.length());
      } else {
        digitsString.append(str);
      }
    }
    while (digitsString.length() + digitsStringScale < minInt) {
      digitsString.insert(0, '0');
    }
    while (-digitsStringScale < minFrac) {
      digitsString.append('0');
      digitsStringScale--;
    }

    // Write the digits to the string builder
    int m0 = Math.max(groupingLength, digitsString.length() + digitsStringScale);
    m0 = (maxInt != dosMax) ? Math.max(maxInt, m0) - 1 : m0 - 1;
    int mN = (maxFrac != dosMax) ? Math.min(-maxFrac, digitsStringScale) : digitsStringScale;
    for (int magnitude = m0; magnitude >= mN; magnitude--) {
      int di = digitsString.length() + digitsStringScale - magnitude - 1;
      if (di < 0 || di >= digitsString.length()) {
        sb.append('#');
      } else {
        sb.append(digitsString.charAt(di));
      }
      if (magnitude > grouping2 && grouping > 0 && (magnitude - grouping2) % grouping == 0) {
        sb.append(',');
      } else if (magnitude > 0 && magnitude == grouping2) {
        sb.append(',');
      } else if (magnitude == 0 && (alwaysShowDecimal || mN < 0)) {
        sb.append('.');
      }
    }

    // Exponential notation
    if (exponentDigits != Math.min(dosMax, Properties.DEFAULT_MINIMUM_EXPONENT_DIGITS)) {
      sb.append('E');
      if (exponentShowPlusSign) {
        sb.append('+');
      }
      for (int i = 0; i < exponentDigits; i++) {
        sb.append('0');
      }
    }

    // Suffixes
    int beforeSuffixPos = sb.length();
    if (psp != null) sb.append(psp);
    AffixPatternUtils.escape(ps, sb);

    // Resolve Padding
    if (paddingWidth != Properties.DEFAULT_FORMAT_WIDTH) {
      while (paddingWidth - sb.length() > 0) {
        sb.insert(afterPrefixPos, '#');
        beforeSuffixPos++;
      }
      int addedLength;
      switch (paddingLocation) {
        case BEFORE_PREFIX:
          addedLength = escapePaddingString(paddingString, sb, 0);
          sb.insert(0, '*');
          afterPrefixPos += addedLength + 1;
          beforeSuffixPos += addedLength + 1;
          break;
        case AFTER_PREFIX:
          addedLength = escapePaddingString(paddingString, sb, afterPrefixPos);
          sb.insert(afterPrefixPos, '*');
          afterPrefixPos += addedLength + 1;
          beforeSuffixPos += addedLength + 1;
          break;
        case BEFORE_SUFFIX:
          escapePaddingString(paddingString, sb, beforeSuffixPos);
          sb.insert(beforeSuffixPos, '*');
          break;
        case AFTER_SUFFIX:
          sb.append('*');
          escapePaddingString(paddingString, sb, sb.length());
          break;
      }
    }

    // Negative affixes
    // Ignore if the negative prefix pattern is "-" and the negative suffix is empty
    if (np != null
        || ns != null
        || (npp == null && nsp != null)
        || (npp != null && (npp.length() != 1 || npp.charAt(0) != '-' || nsp.length() != 0))) {
      sb.append(';');
      if (npp != null) sb.append(npp);
      AffixPatternUtils.escape(np, sb);
      // Copy the positive digit format into the negative.
      // This is optional; the pattern is the same as if '#' were appended here instead.
      sb.append(sb, afterPrefixPos, beforeSuffixPos);
      if (nsp != null) sb.append(nsp);
      AffixPatternUtils.escape(ns, sb);
    }

    return sb.toString();
  }

  /** @return The number of chars inserted. */
  private static int escapePaddingString(CharSequence input, StringBuilder output, int startIndex) {
    if (input == null || input.length() == 0) input = PaddingFormat.FALLBACK_PADDING_STRING;
    int startLength = output.length();
    if (input.length() == 1) {
      if (input.equals("'")) {
        output.insert(startIndex, "''");
      } else {
        output.insert(startIndex, input);
      }
    } else {
      output.insert(startIndex, '\'');
      int offset = 1;
      for (int i = 0; i < input.length(); i++) {
        // it's okay to deal in chars here because the quote mark is the only interesting thing.
        char ch = input.charAt(i);
        if (ch == '\'') {
          output.insert(startIndex + offset, "''");
          offset += 2;
        } else {
          output.insert(startIndex + offset, ch);
          offset += 1;
        }
      }
      output.insert(startIndex + offset, '\'');
    }
    return output.length() - startLength;
  }

  /**
   * Converts a pattern between standard notation and localized notation. Localized notation means
   * that instead of using generic placeholders in the pattern, you use the corresponding
   * locale-specific characters instead. For example, in locale <em>fr-FR</em>, the period in the
   * pattern "0.000" means "decimal" in standard notation (as it does in every other locale), but it
   * means "grouping" in localized notation.
   *
   * <p>A greedy string-substitution strategy is used to substitute locale symbols. If two symbols
   * are ambiguous or have the same prefix, the result is not well-defined.
   *
   * <p>Locale symbols are not allowed to contain the ASCII quote character.
   *
   * @param input The pattern to convert.
   * @param symbols The symbols corresponding to the localized pattern.
   * @param toLocalized true to convert from standard to localized notation; false to convert from
   *     localized to standard notation.
   * @return The pattern expressed in the other notation.
   * @deprecated ICU 59 This method is provided for backwards compatibility and should not be used
   *     in any new code.
   */
  @Deprecated
  public static String convertLocalized(
      String input, DecimalFormatSymbols symbols, boolean toLocalized) {
    if (input == null) return null;

    // Construct a table of strings to be converted between localized and standard.
    String[][] table = new String[21][2];
    int standIdx = toLocalized ? 0 : 1;
    int localIdx = toLocalized ? 1 : 0;
    table[0][standIdx] = "%";
    table[0][localIdx] = symbols.getPercentString();
    table[1][standIdx] = "‰";
    table[1][localIdx] = symbols.getPerMillString();
    table[2][standIdx] = ".";
    table[2][localIdx] = symbols.getDecimalSeparatorString();
    table[3][standIdx] = ",";
    table[3][localIdx] = symbols.getGroupingSeparatorString();
    table[4][standIdx] = "-";
    table[4][localIdx] = symbols.getMinusSignString();
    table[5][standIdx] = "+";
    table[5][localIdx] = symbols.getPlusSignString();
    table[6][standIdx] = ";";
    table[6][localIdx] = Character.toString(symbols.getPatternSeparator());
    table[7][standIdx] = "@";
    table[7][localIdx] = Character.toString(symbols.getSignificantDigit());
    table[8][standIdx] = "E";
    table[8][localIdx] = symbols.getExponentSeparator();
    table[9][standIdx] = "*";
    table[9][localIdx] = Character.toString(symbols.getPadEscape());
    table[10][standIdx] = "#";
    table[10][localIdx] = Character.toString(symbols.getDigit());
    for (int i = 0; i < 10; i++) {
      table[11 + i][standIdx] = Character.toString((char) ('0' + i));
      table[11 + i][localIdx] = symbols.getDigitStringsLocal()[i];
    }

    // Special case: quotes are NOT allowed to be in any localIdx strings.
    // Substitute them with '’' instead.
    for (int i = 0; i < table.length; i++) {
      table[i][localIdx] = table[i][localIdx].replace('\'', '’');
    }

    // Iterate through the string and convert.
    // State table:
    //  0 => base state
    //  1 => first char inside a quoted sequence in input and output string
    //  2 => inside a quoted sequence in input and output string
    //  3 => first char after a close quote in input string;
    //       close quote still needs to be written to output string
    //  4 => base state in input string; inside quoted sequence in output string
    //  5 => first char inside a quoted sequence in input string;
    //       inside quoted sequence in output string
    StringBuilder result = new StringBuilder();
    int state = 0;
    outer:
    for (int offset = 0; offset < input.length(); offset++) {
      char ch = input.charAt(offset);

      // Handle a quote character (state shift)
      if (ch == '\'') {
        if (state == 0) {
          result.append('\'');
          state = 1;
          continue;
        } else if (state == 1) {
          result.append('\'');
          state = 0;
          continue;
        } else if (state == 2) {
          state = 3;
          continue;
        } else if (state == 3) {
          result.append('\'');
          result.append('\'');
          state = 1;
          continue;
        } else if (state == 4) {
          state = 5;
          continue;
        } else {
          assert state == 5;
          result.append('\'');
          result.append('\'');
          state = 4;
          continue;
        }
      }

      if (state == 0 || state == 3 || state == 4) {
        for (String[] pair : table) {
          // Perform a greedy match on this symbol string
          if (input.regionMatches(offset, pair[0], 0, pair[0].length())) {
            // Skip ahead past this region for the next iteration
            offset += pair[0].length() - 1;
            if (state == 3 || state == 4) {
              result.append('\'');
              state = 0;
            }
            result.append(pair[1]);
            continue outer;
          }
        }
        // No replacement found.  Check if a special quote is necessary
        for (String[] pair : table) {
          if (input.regionMatches(offset, pair[1], 0, pair[1].length())) {
            if (state == 0) {
              result.append('\'');
              state = 4;
            }
            result.append(ch);
            continue outer;
          }
        }
        // Still nothing.  Copy the char verbatim.  (Add a close quote if necessary)
        if (state == 3 || state == 4) {
          result.append('\'');
          state = 0;
        }
        result.append(ch);
      } else {
        assert state == 1 || state == 2 || state == 5;
        result.append(ch);
        state = 2;
      }
    }
    // Resolve final quotes
    if (state == 3 || state == 4) {
      result.append('\'');
      state = 0;
    }
    if (state != 0) {
      throw new IllegalArgumentException("Malformed localized pattern: unterminated quote");
    }
    return result.toString();
  }

  public static final int IGNORE_ROUNDING_NEVER = 0;
  public static final int IGNORE_ROUNDING_IF_CURRENCY = 1;
  public static final int IGNORE_ROUNDING_ALWAYS = 2;

  static void parse(String pattern, Properties properties, int ignoreRounding) {
    if (pattern == null || pattern.length() == 0) {
      // Backwards compatibility requires that we reset to the default values.
      // TODO: Only overwrite the properties that "saveToProperties" normally touches?
      properties.clear();
      return;
    }

    // TODO: Use whitespace characters from PatternProps
    // TODO: Use thread locals here.
    LdmlPatternInfo.PatternParseResult result = LdmlPatternInfo.parse(pattern);
    saveToProperties(properties, result, ignoreRounding);
  }

  /** Finalizes the temporary data stored in the PatternParseResult to the Builder. */
  private static void saveToProperties(
      Properties properties, LdmlPatternInfo.PatternParseResult ppr, int _ignoreRounding) {
    // Translate from PatternParseResult to Properties.
    // Note that most data from "negative" is ignored per the specification of DecimalFormat.

    LdmlPatternInfo.SubpatternParseResult positive = ppr.positive;
    LdmlPatternInfo.SubpatternParseResult negative = ppr.negative;
    String pattern = ppr.pattern;

    boolean ignoreRounding;
    if (_ignoreRounding == IGNORE_ROUNDING_NEVER) {
      ignoreRounding = false;
    } else if (_ignoreRounding == IGNORE_ROUNDING_IF_CURRENCY) {
      ignoreRounding = positive.hasCurrencySign;
    } else {
      assert _ignoreRounding == IGNORE_ROUNDING_ALWAYS;
      ignoreRounding = true;
    }

    // Grouping settings
    short grouping1 = (short) (positive.groupingSizes & 0xffff);
    short grouping2 = (short) ((positive.groupingSizes >>> 16) & 0xffff);
    short grouping3 = (short) ((positive.groupingSizes >>> 32) & 0xffff);
    if (grouping2 != -1) {
      properties.setGroupingSize(grouping1);
    } else {
      properties.setGroupingSize(Properties.DEFAULT_GROUPING_SIZE);
    }
    if (grouping3 != -1) {
      properties.setSecondaryGroupingSize(grouping2);
    } else {
      properties.setSecondaryGroupingSize(Properties.DEFAULT_SECONDARY_GROUPING_SIZE);
    }

    // For backwards compatibility, require that the pattern emit at least one min digit.
    int minInt, minFrac;
    if (positive.totalIntegerDigits == 0 && positive.maximumFractionDigits > 0) {
      // patterns like ".##"
      minInt = 0;
      minFrac = Math.max(1, positive.minimumFractionDigits);
    } else if (positive.minimumIntegerDigits == 0 && positive.minimumFractionDigits == 0) {
      // patterns like "#.##"
      minInt = 1;
      minFrac = 0;
    } else {
      minInt = positive.minimumIntegerDigits;
      minFrac = positive.minimumFractionDigits;
    }

    // Rounding settings
    // Don't set basic rounding when there is a currency sign; defer to CurrencyUsage
    if (positive.minimumSignificantDigits > 0) {
      properties.setMinimumFractionDigits(Properties.DEFAULT_MINIMUM_FRACTION_DIGITS);
      properties.setMaximumFractionDigits(Properties.DEFAULT_MAXIMUM_FRACTION_DIGITS);
      properties.setRoundingIncrement(Properties.DEFAULT_ROUNDING_INCREMENT);
      properties.setMinimumSignificantDigits(positive.minimumSignificantDigits);
      properties.setMaximumSignificantDigits(positive.maximumSignificantDigits);
    } else if (positive.rounding != null) {
      if (!ignoreRounding) {
        properties.setMinimumFractionDigits(minFrac);
        properties.setMaximumFractionDigits(positive.maximumFractionDigits);
        properties.setRoundingIncrement(positive.rounding.toBigDecimal());
      } else {
        properties.setMinimumFractionDigits(Properties.DEFAULT_MINIMUM_FRACTION_DIGITS);
        properties.setMaximumFractionDigits(Properties.DEFAULT_MAXIMUM_FRACTION_DIGITS);
        properties.setRoundingIncrement(Properties.DEFAULT_ROUNDING_INCREMENT);
      }
      properties.setMinimumSignificantDigits(Properties.DEFAULT_MINIMUM_SIGNIFICANT_DIGITS);
      properties.setMaximumSignificantDigits(Properties.DEFAULT_MAXIMUM_SIGNIFICANT_DIGITS);
    } else {
      if (!ignoreRounding) {
        properties.setMinimumFractionDigits(minFrac);
        properties.setMaximumFractionDigits(positive.maximumFractionDigits);
        properties.setRoundingIncrement(Properties.DEFAULT_ROUNDING_INCREMENT);
      } else {
        properties.setMinimumFractionDigits(Properties.DEFAULT_MINIMUM_FRACTION_DIGITS);
        properties.setMaximumFractionDigits(Properties.DEFAULT_MAXIMUM_FRACTION_DIGITS);
        properties.setRoundingIncrement(Properties.DEFAULT_ROUNDING_INCREMENT);
      }
      properties.setMinimumSignificantDigits(Properties.DEFAULT_MINIMUM_SIGNIFICANT_DIGITS);
      properties.setMaximumSignificantDigits(Properties.DEFAULT_MAXIMUM_SIGNIFICANT_DIGITS);
    }

    // If the pattern ends with a '.' then force the decimal point.
    if (positive.hasDecimal && positive.maximumFractionDigits == 0) {
      properties.setDecimalSeparatorAlwaysShown(true);
    } else {
      properties.setDecimalSeparatorAlwaysShown(false);
    }

    // Scientific notation settings
    if (positive.exponentDigits > 0) {
      properties.setExponentSignAlwaysShown(positive.exponentShowPlusSign);
      properties.setMinimumExponentDigits(positive.exponentDigits);
      if (positive.minimumSignificantDigits == 0) {
        // patterns without '@' can define max integer digits, used for engineering notation
        properties.setMinimumIntegerDigits(positive.minimumIntegerDigits);
        properties.setMaximumIntegerDigits(positive.totalIntegerDigits);
      } else {
        // patterns with '@' cannot define max integer digits
        properties.setMinimumIntegerDigits(1);
        properties.setMaximumIntegerDigits(Properties.DEFAULT_MAXIMUM_INTEGER_DIGITS);
      }
    } else {
      properties.setExponentSignAlwaysShown(Properties.DEFAULT_EXPONENT_SIGN_ALWAYS_SHOWN);
      properties.setMinimumExponentDigits(Properties.DEFAULT_MINIMUM_EXPONENT_DIGITS);
      properties.setMinimumIntegerDigits(minInt);
      properties.setMaximumIntegerDigits(Properties.DEFAULT_MAXIMUM_INTEGER_DIGITS);
    }

    // Compute the affix patterns (required for both padding and affixes)
    String posPrefix = ppr.getString(LdmlPatternInfo.PatternParseResult.POS_PREFIX);
    String posSuffix = ppr.getString(LdmlPatternInfo.PatternParseResult.POS_SUFFIX);

    // Padding settings
    if (positive.paddingEndpoints != 0) {
      // The width of the positive prefix and suffix templates are included in the padding
      int paddingWidth =
          positive.paddingWidth
              + AffixPatternUtils.unescapedLength(posPrefix)
              + AffixPatternUtils.unescapedLength(posSuffix);
      properties.setFormatWidth(paddingWidth);
      String rawPaddingString = ppr.getString(LdmlPatternInfo.PatternParseResult.POS_PADDING);
      if (rawPaddingString.length() == 1) {
        properties.setPadString(rawPaddingString);
      } else if (rawPaddingString.length() == 2) {
        if (rawPaddingString.charAt(0) == '\'') {
          properties.setPadString("'");
        } else {
          properties.setPadString(rawPaddingString);
        }
      } else {
        properties.setPadString(rawPaddingString.substring(1, rawPaddingString.length() - 1));
      }
      assert positive.paddingLocation != null;
      properties.setPadPosition(positive.paddingLocation);
    } else {
      properties.setFormatWidth(Properties.DEFAULT_FORMAT_WIDTH);
      properties.setPadString(Properties.DEFAULT_PAD_STRING);
      properties.setPadPosition(Properties.DEFAULT_PAD_POSITION);
    }

    // Set the affixes
    // Always call the setter, even if the prefixes are empty, especially in the case of the
    // negative prefix pattern, to prevent default values from overriding the pattern.
    properties.setPositivePrefixPattern(posPrefix);
    properties.setPositiveSuffixPattern(posSuffix);
    if (negative != null) {
      properties.setNegativePrefixPattern(ppr.getString(LdmlPatternInfo.PatternParseResult.NEG_PREFIX));
      properties.setNegativeSuffixPattern(ppr.getString(LdmlPatternInfo.PatternParseResult.NEG_SUFFIX));
    } else {
      properties.setNegativePrefixPattern(null);
      properties.setNegativeSuffixPattern(null);
    }

    // Set the magnitude multiplier
    if (positive.hasPercentSign) {
      properties.setMagnitudeMultiplier(2);
    } else if (positive.hasPerMilleSign) {
      properties.setMagnitudeMultiplier(3);
    } else {
      properties.setMagnitudeMultiplier(Properties.DEFAULT_MAGNITUDE_MULTIPLIER);
    }
  }
}
