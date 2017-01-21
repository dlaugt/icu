// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.impl.number;

import java.math.BigDecimal;
import java.text.ParseException;

import com.ibm.icu.impl.number.formatters.PaddingFormat.PaddingLocation;

/**
 * Handles parsing and creation of the compact pattern string representation of a decimal format.
 */
public class PatternString {

  /**
   * Parses a pattern string into a new property bag.
   *
   * @param pattern The pattern string, like "#,##0.00"
   * @return A property bag object.
   * @throws ParseException If there is a syntax error in the pattern string.
   */
  public static Properties parseToProperties(String pattern) throws ParseException {
    Properties properties = new Properties();
    LdmlDecimalPatternParser.parse(pattern, properties);
    return properties;
  }

  /**
   * Parses a pattern string into an existing property bag. The object will be cleared before any
   * data is written to it.
   *
   * @param pattern The pattern string, like "#,##0.00"
   * @param properties The property bag object to overwrite.
   * @throws ParseException If there was a syntax error in the pattern string.
   */
  public static void parseToExistingProperties(String pattern, Properties properties)
      throws ParseException {
    properties.clear();
    LdmlDecimalPatternParser.parse(pattern, properties);
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
    int paddingWidth = Math.min(properties.getPaddingWidth(), dosMax);
    PaddingLocation paddingLocation = properties.getPaddingLocation();
    CharSequence paddingString = properties.getPaddingString();
    int minInt = Math.max(Math.min(properties.getMinimumIntegerDigits(), dosMax), 0);
    int maxInt = Math.min(properties.getMaximumIntegerDigits(), dosMax);
    int minFrac = Math.max(Math.min(properties.getMinimumFractionDigits(), dosMax), 0);
    int maxFrac = Math.min(properties.getMaximumFractionDigits(), dosMax);
    int minSig = Math.min(properties.getMinimumSignificantDigits(), dosMax);
    int maxSig = Math.min(properties.getMaximumSignificantDigits(), dosMax);
    boolean alwaysShowDecimal = properties.getAlwaysShowDecimal();
    int exponentDigits = Math.min(properties.getExponentDigits(), dosMax);
    boolean exponentShowPlusSign = properties.getExponentShowPlusSign();
    CharSequence pp = properties.getPositivePrefix();
    CharSequence ppp = properties.getPositivePrefixPattern();
    CharSequence ps = properties.getPositiveSuffix();
    CharSequence psp = properties.getPositiveSuffixPattern();
    CharSequence np = properties.getNegativePrefix();
    CharSequence npp = properties.getNegativePrefixPattern();
    CharSequence ns = properties.getNegativeSuffix();
    CharSequence nsp = properties.getNegativeSuffixPattern();

    // Prefixes
    if (ppp != null) sb.append(ppp);
    escape(pp, sb);
    int afterPrefixPos = sb.length();

    // Figure out the grouping sizes.
    int grouping1, grouping2;
    if (groupingSize != Math.min(dosMax, Properties.DEFAULT_SECONDARY_GROUPING_SIZE)
        && firstGroupingSize != Math.min(dosMax, Properties.DEFAULT_GROUPING_SIZE)
        && groupingSize != firstGroupingSize) {
      grouping1 = groupingSize;
      grouping2 = firstGroupingSize;
    } else if (groupingSize != Math.min(dosMax, Properties.DEFAULT_SECONDARY_GROUPING_SIZE)) {
      grouping1 = 0;
      grouping2 = groupingSize;
    } else if (firstGroupingSize != Math.min(dosMax, Properties.DEFAULT_GROUPING_SIZE)) {
      grouping1 = 0;
      grouping2 = firstGroupingSize;
    } else {
      grouping1 = 0;
      grouping2 = 0;
    }
    int groupingLength = grouping1 + grouping2 + 1;

    // Figure out the digits we need to put in the pattern.
    BigDecimal roundingInterval = properties.getRoundingInterval();
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
    } else if (roundingInterval != Properties.DEFAULT_ROUNDING_INTERVAL) {
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
      if (magnitude > 0 && magnitude == grouping1 + grouping2) {
        sb.append(',');
      } else if (magnitude > 0 && magnitude == grouping2) {
        sb.append(',');
      } else if (magnitude == 0 && (alwaysShowDecimal || mN < 0)) {
        sb.append('.');
      }
    }

    // Exponential notation
    if (exponentDigits != Math.min(dosMax, Properties.DEFAULT_EXPONENT_DIGITS)) {
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
    escape(ps, sb);

    // Resolve Padding
    if (paddingWidth != Properties.DEFAULT_PADDING_WIDTH) {
      while (paddingWidth - sb.length() > 0) {
        sb.insert(afterPrefixPos, '#');
        beforeSuffixPos++;
      }
      switch (paddingLocation) {
        case BEFORE_PREFIX:
          escape(paddingString, sb, 0);
          sb.insert(0, '*');
          break;
        case AFTER_PREFIX:
          escape(paddingString, sb, afterPrefixPos);
          sb.insert(afterPrefixPos, '*');
          break;
        case BEFORE_SUFFIX:
          escape(paddingString, sb, beforeSuffixPos);
          sb.insert(0, '*');
          break;
        case AFTER_SUFFIX:
          sb.append('*');
          escape(paddingString, sb);
          break;
      }
    }

    // Negative affixes
    if (np != null || npp != null || ns != null || nsp != null) {
      sb.append(';');
      if (npp != null) sb.append(npp);
      escape(np, sb);
      sb.append('#');
      if (nsp != null) sb.append(nsp);
      escape(ns, sb);
    }

    return sb.toString();
  }

  private static void escape(CharSequence input, StringBuilder sb) {
    if (input == null) return;
    int length = input.length();
    if (length == 0) return;
    if (length > 1) sb.append('\'');
    for (int i = 0; i < length; i++) {
      char ch = input.charAt(i);
      if (ch == '\'') {
        sb.append("''");
      } else {
        sb.append(ch);
      }
    }
    if (length > 1) sb.append('\'');
  }

  private static void escape(CharSequence input, StringBuilder sb, int insertIndex) {
    // Although this triggers a new object creation, it reduces the number of calls to insert (and
    // therefore System.arraycopy).
    StringBuilder temp = new StringBuilder();
    escape(input, temp);
    sb.insert(insertIndex, temp);
  }

  /** Implements a recursive descent parser for decimal format patterns. */
  static class LdmlDecimalPatternParser {

    /**
     * An internal, intermediate data structure used for storing parse results before they are
     * finalized into a DecimalFormatPattern.Builder.
     */
    private static class PatternParseResult {
      SubpatternParseResult positive = new SubpatternParseResult();
      SubpatternParseResult negative = null;

      /** Finalizes the temporary data stored in the PatternParseResult to the Builder. */
      void saveToProperties(Properties properties) {
        // Translate from PatternState to Properties.
        // Note that most data from "negative" is ignored per the specification of DecimalFormat.

        // Grouping settings
        if (positive.groupingSizes[1] != -1) {
          properties.setGroupingSize(positive.groupingSizes[0]);
        }
        if (positive.groupingSizes[2] != -1) {
          properties.setSecondaryGroupingSize(positive.groupingSizes[1]);
        }

        // Rounding settings
        if (positive.minimumSignificantDigits > 0) {
          properties.setMinimumSignificantDigits(positive.minimumSignificantDigits);
          properties.setMaximumSignificantDigits(positive.maximumSignificantDigits);
        } else {
          properties.setMinimumFractionDigits(positive.minimumFractionDigits);
          properties.setMaximumFractionDigits(positive.maximumFractionDigits);
          if (!positive.rounding.isEmpty()) {
            properties.setRoundingInterval(positive.rounding.toBigDecimal());
          }
        }
        properties.setMinimumIntegerDigits(positive.minimumIntegerDigits);

        // If the pattern ends with a '.' then force the decimal point.
        if (positive.hasDecimal && positive.maximumFractionDigits == 0) {
          properties.setAlwaysShowDecimal(true);
        }

        // Currency settings
        // TODO: Force enable currency support here?
        if (positive.hasCurrencySign) {
          // properties.setUseCurrency(true);
        }

        // Scientific notation settings
        if (positive.exponentDigits > 0) {
          properties.setExponentShowPlusSign(positive.exponentShowPlusSign);
          properties.setExponentDigits(positive.exponentDigits);
          properties.setMaximumIntegerDigits(positive.totalIntegerDigits);
        }

        // Padding settings
        if (positive.padding.length() > 0) {
          // The width of the positive prefix and suffix templates are included in the padding
          properties.setPaddingWidth(
              positive.paddingWidth + positive.prefix.length() + positive.suffix.length());
          properties.setPaddingString(positive.padding.toString());
          assert positive.paddingLocation != null;
          properties.setPaddingLocation(positive.paddingLocation);
        }

        // Set the affixes
        // Always call the setter, even if the prefixes are empty, especially in the case of the
        // negative prefix pattern, to prevent default values from overriding the pattern.
        properties.setPositivePrefixPattern(positive.prefix);
        properties.setPositiveSuffixPattern(positive.suffix);
        if (negative != null) {
          properties.setNegativePrefixPattern(negative.prefix);
          properties.setNegativeSuffixPattern(negative.suffix);
        }

        // Set the magnitude multiplier
        if (positive.hasPercentSign) {
          properties.setMagnitudeMultiplier(2);
        } else if (positive.hasPerMilleSign) {
          properties.setMagnitudeMultiplier(3);
        }
      }
    }

    private static class SubpatternParseResult {
      int[] groupingSizes = new int[] {-1, -1, -1};
      int minimumIntegerDigits = 0;
      int totalIntegerDigits = 0;
      int minimumFractionDigits = 0;
      int maximumFractionDigits = 0;
      int minimumSignificantDigits = 0;
      int maximumSignificantDigits = 0;
      boolean hasDecimal = false;
      int paddingWidth = 0;
      PaddingLocation paddingLocation = null;
      RoundingInterval rounding = new RoundingInterval();
      boolean exponentShowPlusSign = false;
      int exponentDigits = 0;
      boolean hasPercentSign = false;
      boolean hasPerMilleSign = false;
      boolean hasCurrencySign = false;

      StringBuilder padding = new StringBuilder();
      StringBuilder prefix = new StringBuilder();
      StringBuilder suffix = new StringBuilder();
    }

    private static class RoundingInterval {
      long integer = 0L;
      long fraction = 0L;
      long fractionDivisor = 1;

      void appendInteger(int n) throws ParseException {
        if (integer > Long.MAX_VALUE / 10) {
          throw new ParseException("Rounding interval in decimal pattern is too large", 0);
        }
        integer *= 10;
        integer += n;
      }

      void appendFraction(int n) throws ParseException {
        if (fraction > Long.MAX_VALUE / 10) {
          throw new ParseException("Rounding interval in decimal pattern is too large", 0);
        }
        fraction *= 10;
        fraction += n;
        fractionDivisor *= 10;
      }

      boolean isEmpty() {
        return integer == 0 && fraction == 0;
      }

      BigDecimal toBigDecimal() {
        BigDecimal d = new BigDecimal(fraction);
        d = d.divide(new BigDecimal(fractionDivisor));
        d = d.add(new BigDecimal(integer));
        return d;
      }
    }

    /** An internal class used for tracking the cursor during parsing of a pattern string. */
    private static class ParserState {
      final String pattern;
      int offset;

      ParserState(String pattern) {
        this.pattern = pattern;
        this.offset = 0;
      }

      int peek() {
        if (offset == pattern.length()) {
          return -1;
        } else {
          return pattern.codePointAt(offset);
        }
      }

      int next() {
        int codePoint = peek();
        offset += Character.charCount(codePoint);
        return codePoint;
      }

      ParseException toParseException(String message) {
        StringBuilder sb = new StringBuilder();
        sb.append("Unexpected character in decimal format pattern: ");
        sb.append(message);
        sb.append(": ");
        if (peek() == -1) {
          sb.append("EOL");
        } else {
          sb.append("'");
          sb.append(Character.toChars(peek()));
          sb.append("'");
        }
        return new ParseException(sb.toString(), offset);
      }
    }

    static void parse(String pattern, Properties properties) throws ParseException {
      // TODO: Use whitespace characters from PatternProps
      ParserState state = new ParserState(pattern);
      PatternParseResult result = new PatternParseResult();
      consumePattern(state, result);
      result.saveToProperties(properties);
    }

    private static void consumePattern(ParserState state, PatternParseResult result)
        throws ParseException {
      // pattern := subpattern (';' subpattern)?
      consumeSubpattern(state, result.positive);
      if (state.peek() == ';') {
        state.next(); // consume the ';'
        result.negative = new SubpatternParseResult();
        consumeSubpattern(state, result.negative);
      }
      if (state.peek() != -1) {
        throw state.toParseException("pattern");
      }
    }

    private static void consumeSubpattern(ParserState state, SubpatternParseResult result)
        throws ParseException {
      // subpattern := literals? number exponent? literals?
      consumePadding(state, result, PaddingLocation.BEFORE_PREFIX);
      consumeAffix(state, result, result.prefix);
      consumePadding(state, result, PaddingLocation.AFTER_PREFIX);
      consumeFormat(state, result);
      consumeExponent(state, result);
      consumePadding(state, result, PaddingLocation.BEFORE_SUFFIX);
      consumeAffix(state, result, result.suffix);
      consumePadding(state, result, PaddingLocation.AFTER_SUFFIX);
    }

    private static void consumePadding(
        ParserState state, SubpatternParseResult result, PaddingLocation paddingLocation)
        throws ParseException {
      if (state.peek() != '*') {
        return;
      }
      result.paddingLocation = paddingLocation;
      state.next(); // consume the '*'
      consumeLiteral(state, result.padding);
    }

    private static void consumeAffix(
        ParserState state, SubpatternParseResult result, StringBuilder destination)
        throws ParseException {
      // literals := { literal }
      while (true) {
        switch (state.peek()) {
          case '#':
          case '@':
          case ';':
          case '*':
          case '.':
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case -1:
            // Characters that cannot appear unquoted in a literal
            return;

          case '%':
            result.hasPercentSign = true;
            break;

          case '‰':
            result.hasPerMilleSign = true;
            break;

          case '¤':
            result.hasCurrencySign = true;
            break;
        }
        consumeLiteral(state, destination);
      }
    }

    private static void consumeLiteral(ParserState state, StringBuilder destination)
        throws ParseException {
      if (state.peek() == -1) {
        throw state.toParseException("expected unquoted literal but found end of string");
      } else if (state.peek() == '\'') {
        destination.appendCodePoint(state.next()); // consume the starting quote
        while (state.peek() != '\'') {
          if (state.peek() == -1) {
            throw state.toParseException("expected quoted literal but found end of string");
          } else {
            destination.appendCodePoint(state.next()); // consume a quoted character
          }
        }
        destination.appendCodePoint(state.next()); // consume the ending quote
      } else {
        // consume a non-quoted literal character
        destination.appendCodePoint(state.next());
      }
    }

    private static void consumeFormat(ParserState state, SubpatternParseResult result)
        throws ParseException {
      consumeIntegerFormat(state, result);
      if (state.peek() == '.') {
        state.next(); // consume the decimal point
        result.hasDecimal = true;
        result.paddingWidth += 1;
        consumeFractionFormat(state, result);
      }
    }

    private static void consumeIntegerFormat(ParserState state, SubpatternParseResult result)
        throws ParseException {
      boolean seenSignificantDigitMarker = false;

      while (true) {
        switch (state.peek()) {
          case ',':
            result.paddingWidth += 1;
            result.groupingSizes[2] = result.groupingSizes[1];
            result.groupingSizes[1] = result.groupingSizes[0];
            result.groupingSizes[0] = 0;
            break;

          case '#':
            result.paddingWidth += 1;
            result.groupingSizes[0] += 1;
            result.totalIntegerDigits += (seenSignificantDigitMarker ? 0 : 1);
            // no change to result.minimumIntegerDigits
            // no change to result.minimumSignificantDigits
            result.maximumSignificantDigits += (seenSignificantDigitMarker ? 1 : 0);
            result.rounding.appendInteger(0);
            break;

          case '@':
            seenSignificantDigitMarker = true;
            result.paddingWidth += 1;
            result.groupingSizes[0] += 1;
            result.totalIntegerDigits += 1;
            // no change to result.minimumIntegerDigits
            result.minimumSignificantDigits += 1;
            result.maximumSignificantDigits += 1;
            result.rounding.appendInteger(0); // TODO: does this make sense?
            break;

          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            // TODO: Crash here if we've seen the significant digit marker?
            result.paddingWidth += 1;
            result.groupingSizes[0] += 1;
            result.totalIntegerDigits += 1;
            result.minimumIntegerDigits += 1;
            // no change to result.minimumSignificantDigits
            result.maximumSignificantDigits += (seenSignificantDigitMarker ? 1 : 0);
            result.rounding.appendInteger(state.peek() - '0');
            break;

          default:
            return;
        }
        state.next(); // consume the symbol
      }
    }

    private static void consumeFractionFormat(ParserState state, SubpatternParseResult result)
        throws ParseException {
      while (true) {
        switch (state.peek()) {
          case '#':
            result.paddingWidth += 1;
            // no change to result.minimumFractionDigits
            result.maximumFractionDigits += 1;
            result.rounding.appendFraction(0);
            break;

          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            result.paddingWidth += 1;
            result.minimumFractionDigits += 1;
            result.maximumFractionDigits += 1;
            result.rounding.appendFraction(state.peek() - '0');
            break;

          default:
            return;
        }
        state.next(); // consume the symbol
      }
    }

    private static void consumeExponent(ParserState state, SubpatternParseResult result) {
      if (state.peek() != 'E') {
        return;
      }
      state.next(); // consume the E
      if (state.peek() == '+') {
        state.next(); // consume the +
        result.exponentShowPlusSign = true;
      }
      while (state.peek() == '0') {
        state.next(); // consume the 0
        result.exponentDigits += 1;
      }
    }
  }
}
