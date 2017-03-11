// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.text;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.math.BigInteger;
import java.math.RoundingMode;
import java.text.AttributedCharacterIterator;
import java.text.FieldPosition;
import java.text.ParseException;
import java.text.ParsePosition;

import com.ibm.icu.impl.number.AffixPatternUtils;
import com.ibm.icu.impl.number.Endpoint;
import com.ibm.icu.impl.number.Format.SingularFormat;
import com.ibm.icu.impl.number.FormatQuantity4;
import com.ibm.icu.impl.number.Parse;
import com.ibm.icu.impl.number.PatternString;
import com.ibm.icu.impl.number.Properties;
import com.ibm.icu.impl.number.formatters.PaddingFormat.PaddingLocation;
import com.ibm.icu.impl.number.formatters.PositiveDecimalFormat;
import com.ibm.icu.impl.number.formatters.ScientificFormat;
import com.ibm.icu.impl.number.rounders.SignificantDigitsRounder;
import com.ibm.icu.math.BigDecimal;
import com.ibm.icu.math.MathContext;
import com.ibm.icu.text.PluralRules.IFixedDecimal;
import com.ibm.icu.util.Currency;
import com.ibm.icu.util.Currency.CurrencyUsage;
import com.ibm.icu.util.CurrencyAmount;
import com.ibm.icu.util.ULocale;

/** @author sffc */
public class DecimalFormat extends NumberFormat {

  /**
   * New serialization in ICU 59: declare different version from ICU 58.
   */
  private static final long serialVersionUID = 864413376551465019L;

  /// INSTANCE VARIABLES ///
  // Access level: package-private, so that subclasses can use them.
  // properties should be final, but clone won't work if we make it final.
  // All fields are transient because custom serialization is used.

  /**
   * The property bag corresponding to user-specified settings and settings from the pattern
   * string.  In principle this should be final, but serialize and clone won't work if it is
   * final.  Does not need to be volatile because the reference never changes.
   */
  /* final */ transient Properties properties;

  /**
   * The symbols for the current locale.  Volatile because threads may read and write at the same
   * time.
   */
  volatile transient DecimalFormatSymbols symbols;

  /**
   * The pre-computed formatter object.  Setters cause this to be re-computed atomically.  The
   * {@link #format} method uses the formatter directly without needing to synchronize.  Volatile
   * because threads may read and write at the same time.
   */
  volatile transient SingularFormat formatter;

  /**
   * The effective properties as exported from the formatter object.  Volatile because threads may
   * read and write at the same time.
   */
  volatile transient Properties exportedProperties;

  /** @stable ICU 2.0 */
  public DecimalFormat() {
    // Use the locale's default pattern
    ULocale def = ULocale.getDefault(ULocale.Category.FORMAT);
    String pattern = getPattern(def, 0);
    symbols = getDefaultSymbols();
    properties = new Properties();
    exportedProperties = new Properties();
    // Regression: ignore pattern rounding information if the pattern has currency symbols.
    boolean ignorePatternRounding = AffixPatternUtils.hasCurrencySymbols(pattern);
    setPropertiesFromPattern(pattern, ignorePatternRounding);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public DecimalFormat(String pattern) {
    symbols = getDefaultSymbols();
    properties = new Properties();
    exportedProperties = new Properties();
    // Regression: ignore pattern rounding information if the pattern has currency symbols.
    boolean ignorePatternRounding = AffixPatternUtils.hasCurrencySymbols(pattern);
    setPropertiesFromPattern(pattern, ignorePatternRounding);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public DecimalFormat(String pattern, DecimalFormatSymbols symbols) {
    this.symbols = (DecimalFormatSymbols) symbols.clone();
    properties = new Properties();
    exportedProperties = new Properties();
    // Regression: ignore pattern rounding information if the pattern has currency symbols.
    boolean ignorePatternRounding = AffixPatternUtils.hasCurrencySymbols(pattern);
    setPropertiesFromPattern(pattern, ignorePatternRounding);
    refreshFormatter();
  }

  /** @stable ICU 4.2 */
  public DecimalFormat(
      String pattern, DecimalFormatSymbols symbols, CurrencyPluralInfo infoInput, int style) {
    this.symbols = (DecimalFormatSymbols) symbols.clone();
    properties = new Properties();
    exportedProperties = new Properties();
    properties.setCurrencyPluralInfo(infoInput);
    refreshFormatter();
  }

  /** Package-private constructor used by NumberFormat. */
  DecimalFormat(String pattern, DecimalFormatSymbols symbols, int choice) {
    this.symbols = (DecimalFormatSymbols) symbols.clone();
    properties = new Properties();
    exportedProperties = new Properties();
    // If choice is a currency type, ignore the rounding information.
    if (choice == CURRENCYSTYLE
        || choice == ISOCURRENCYSTYLE
        || choice == ACCOUNTINGCURRENCYSTYLE
        || choice == CASHCURRENCYSTYLE
        || choice == STANDARDCURRENCYSTYLE
        || choice == PLURALCURRENCYSTYLE
        || AffixPatternUtils.hasCurrencySymbols(pattern)){
      setPropertiesFromPattern(pattern, true);
    } else {
      setPropertiesFromPattern(pattern, false);
    }
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  @Override
  public Object clone() {
    DecimalFormat other = (DecimalFormat) super.clone();
    other.symbols = (DecimalFormatSymbols) symbols.clone();
    other.properties = properties.clone();
    other.exportedProperties = new Properties();
    other.refreshFormatter();
    return other;
  }

  /**
   * Custom serialization: save property bag and symbols; the formatter object can be re-created
   * from just that amount of information.
   */
  private void writeObject(ObjectOutputStream oos) throws IOException {
    oos.defaultWriteObject();
    // ICU 59 custom serialization.
    // Extra int for possible future use
    oos.writeInt(0);
    // 1) Property Bag
    oos.writeObject(properties);
    // 2) DecimalFormatSymbols
    oos.writeObject(symbols);
  }

  /**
   * Custom serialization: re-create object from serialized property bag and symbols.
   */
  private void readObject(ObjectInputStream ois) throws IOException, ClassNotFoundException {
    ois.defaultReadObject();
    // Extra int for possible future use
    ois.readInt();
    // 1) Property Bag
    properties = (Properties) ois.readObject();
    // 2) DecimalFormatSymbols
    symbols = (DecimalFormatSymbols) ois.readObject();
    // Re-build transient fields
    exportedProperties = new Properties();
    refreshFormatter();
  }

  static DecimalFormatSymbols getDefaultSymbols() {
    return DecimalFormatSymbols.getInstance();
  }

  /** @stable ICU 2.0 */
  public synchronized void applyPattern(String pattern) {
    setPropertiesFromPattern(pattern, false);
    // Backwards compatibility: clear out user-specified prefix and suffix,
    // as well as CurrencyPluralInfo.
    properties.setPositivePrefix(null);
    properties.setNegativePrefix(null);
    properties.setPositiveSuffix(null);
    properties.setNegativeSuffix(null);
    properties.setCurrencyPluralInfo(null);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized void applyLocalizedPattern(String localizedPattern) {
    String pattern = PatternString.convertLocalized(localizedPattern, symbols, false);
    applyPattern(pattern);
  }

  /**
   * {@inheritDoc}
   *
   * @stable ICU 2.0
   */
  @Override
  public StringBuffer format(double number, StringBuffer result, FieldPosition fieldPosition) {
    FormatQuantity4 fq = new FormatQuantity4(number);
    formatter.format(fq, result, fieldPosition);
    fq.populateUFieldPosition(fieldPosition);
    return result;
  }

  /** @stable ICU 2.0 */
  @Override
  public StringBuffer format(long number, StringBuffer result, FieldPosition fieldPosition) {
    FormatQuantity4 fq = new FormatQuantity4(number);
    formatter.format(fq, result, fieldPosition);
    fq.populateUFieldPosition(fieldPosition);
    return result;
  }

  /** @stable ICU 2.0 */
  @Override
  public StringBuffer format(BigInteger number, StringBuffer result, FieldPosition fieldPosition) {
    FormatQuantity4 fq = new FormatQuantity4(number);
    formatter.format(fq, result, fieldPosition);
    fq.populateUFieldPosition(fieldPosition);
    return result;
  }

  /** @stable ICU 2.0 */
  @Override
  public StringBuffer format(
      java.math.BigDecimal number, StringBuffer result, FieldPosition fieldPosition) {
    FormatQuantity4 fq = new FormatQuantity4(number);
    formatter.format(fq, result, fieldPosition);
    fq.populateUFieldPosition(fieldPosition);
    return result;
  }

  /** @stable ICU 2.0 */
  @Override
  public StringBuffer format(BigDecimal number, StringBuffer result, FieldPosition fieldPosition) {
    FormatQuantity4 fq = new FormatQuantity4(number.toBigDecimal());
    formatter.format(fq, result, fieldPosition);
    fq.populateUFieldPosition(fieldPosition);
    return result;
  }

  /** @stable ICU 3.6 */
  @Override
  public AttributedCharacterIterator formatToCharacterIterator(Object obj) {
    if (!(obj instanceof Number)) throw new IllegalArgumentException();
    Number number = (Number) obj;
    FormatQuantity4 fq = new FormatQuantity4(number);
    AttributedCharacterIterator result = formatter.formatToCharacterIterator(fq);
    return result;
  }

  protected static final ThreadLocal<Properties> threadLocalCurrencyProperties =
      new ThreadLocal<Properties>() {
    @Override
    protected Properties initialValue() {
      return new Properties();
    }
  };

  @Override
  public StringBuffer format(CurrencyAmount currAmt, StringBuffer toAppendTo, FieldPosition pos) {
    // TODO: This is ugly. Currency should be a free parameter, not in property bag. Fix in ICU 60.
    Properties cprops = threadLocalCurrencyProperties.get();
    synchronized(this) {
      cprops.copyFrom(properties);
    }
    cprops.setCurrency(currAmt.getCurrency());
    FormatQuantity4 fq = new FormatQuantity4(currAmt.getNumber());
    // TODO: Use a static format path here
    SingularFormat fmt = Endpoint.fromBTA(cprops, symbols);
    fmt.format(fq, toAppendTo, pos);
    fq.populateUFieldPosition(pos);
    return toAppendTo;
  }

  /** @stable ICU 2.0 */
  @Override
  public Number parse(String text, ParsePosition parsePosition) {
    // Backwards compatibility: use currency parse mode if this is a currency instance
    Number result = Parse.parse(text, parsePosition, properties, symbols);
    // Backwards compatibility: return com.ibm.icu.math.BigDecimal
    if (result instanceof java.math.BigDecimal) {
      result = new com.ibm.icu.math.BigDecimal((java.math.BigDecimal) result);
    }
    return result;
  }

  /** @stable ICU 49 */
  @Override
  public CurrencyAmount parseCurrency(CharSequence text, ParsePosition parsePosition) {
    try {
      CurrencyAmount result = Parse.parseCurrency(text, parsePosition, properties, symbols);
      if (result == null) return null;
      Number number = result.getNumber();
      // Backwards compatibility: return com.ibm.icu.math.BigDecimal
      if (number instanceof java.math.BigDecimal) {
        number = new com.ibm.icu.math.BigDecimal((java.math.BigDecimal) number);
        result = new CurrencyAmount(number, result.getCurrency());
      }
      return result;
    } catch (ParseException e) {
      return null;
    }
  }

  /**
   * Returns a copy of the decimal format symbols used by this format.
   *
   * @return desired DecimalFormatSymbols
   * @see DecimalFormatSymbols
   * @stable ICU 2.0
   */
  public synchronized DecimalFormatSymbols getDecimalFormatSymbols() {
    return (DecimalFormatSymbols) symbols.clone();
  }

  /**
   * Sets the decimal format symbols used by this format. The format uses a copy of the provided
   * symbols.
   *
   * @param newSymbols desired DecimalFormatSymbols
   * @see DecimalFormatSymbols
   * @stable ICU 2.0
   */
  public synchronized void setDecimalFormatSymbols(DecimalFormatSymbols newSymbols) {
    symbols = (DecimalFormatSymbols) newSymbols.clone();
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized String getPositivePrefix() {
    String result = exportedProperties.getPositivePrefix();
    return (result == null) ? "" : result;
  }

  /** @stable ICU 2.0 */
  public synchronized void setPositivePrefix(String newValue) {
    properties.setPositivePrefix(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized String getNegativePrefix() {
    String result = exportedProperties.getNegativePrefix();
    return (result == null) ? "" : result;
  }

  /** @stable ICU 2.0 */
  public synchronized void setNegativePrefix(String newValue) {
    properties.setNegativePrefix(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized String getPositiveSuffix() {
    String result = exportedProperties.getPositiveSuffix();
    return (result == null) ? "" : result;
  }

  /** @stable ICU 2.0 */
  public synchronized void setPositiveSuffix(String newValue) {
    properties.setPositiveSuffix(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized String getNegativeSuffix() {
    String result = exportedProperties.getNegativeSuffix();
    return (result == null) ? "" : result;
  }

  /** @stable ICU 2.0 */
  public synchronized void setNegativeSuffix(String newValue) {
    properties.setNegativeSuffix(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized int getMultiplier() {
    if (properties.getMultiplier() != null) {
      return properties.getMultiplier().intValue();
    } else {
      return (int) Math.pow(10, properties.getMagnitudeMultiplier());
    }
  }

  /** @stable ICU 2.0 */
  public synchronized void setMultiplier(int newValue) {
    if (newValue == 0) {
      throw new IllegalArgumentException("Multiplier must be nonzero.");
    }

    // Try to convert to a magnitude multiplier first
    int delta = 0;
    int value = newValue;
    while (newValue != 1) {
      delta++;
      int temp = value / 10;
      if (temp * 10 != value) {
        delta = -1;
        break;
      }
      value = temp;
    }
    if (delta != -1) {
      properties.setMagnitudeMultiplier(delta);
    } else {
      properties.setMultiplier(java.math.BigDecimal.valueOf(newValue));
    }
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized java.math.BigDecimal getRoundingIncrement() {
    return exportedProperties.getRoundingInterval();
  }

  /** @stable ICU 2.0 */
  public synchronized void setRoundingIncrement(java.math.BigDecimal newValue) {
    // Backwards compatibility: ignore rounding increment if zero,
    // and instead set maximum fraction digits.
    if (newValue != null && newValue.compareTo(java.math.BigDecimal.ZERO) == 0) {
      properties.setMaximumFractionDigits(Integer.MAX_VALUE);
      return;
    }

    properties.setRoundingInterval(newValue);
    refreshFormatter();
  }

  /** @stable ICU 3.6 */
  public synchronized void setRoundingIncrement(BigDecimal newValue) {
    java.math.BigDecimal javaBigDecimal = (newValue == null) ? null : newValue.toBigDecimal();
    setRoundingIncrement(javaBigDecimal);
  }

  /** @stable ICU 2.0 */
  public synchronized void setRoundingIncrement(double newValue) {
    if (newValue == 0) {
      setRoundingIncrement((java.math.BigDecimal) null);
    } else {
      java.math.BigDecimal javaBigDecimal = java.math.BigDecimal.valueOf(newValue);
      setRoundingIncrement(javaBigDecimal);
    }
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized int getRoundingMode() {
    RoundingMode mode = exportedProperties.getRoundingMode();
    return (mode == null) ? 0 : mode.ordinal();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized void setRoundingMode(int roundingMode) {
    properties.setRoundingMode(RoundingMode.valueOf(roundingMode));
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized int getFormatWidth() {
    return exportedProperties.getPaddingWidth();
  }

  /** @stable ICU 2.0 */
  public synchronized void setFormatWidth(int width) {
    properties.setPaddingWidth(width);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized char getPadCharacter() {
    CharSequence paddingString = exportedProperties.getPaddingString();
    if (paddingString == null) {
      return '.'; // TODO: Is this the correct behavior?
    } else {
      return paddingString.charAt(0);
    }
  }

  /** @stable ICU 2.0 */
  public synchronized void setPadCharacter(char padChar) {
    properties.setPaddingString(Character.toString(padChar));
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized int getPadPosition() {
    PaddingLocation loc = exportedProperties.getPaddingLocation();
    return (loc == null) ? PAD_BEFORE_PREFIX : loc.toOld();
  }

  /** @stable ICU 2.0 */
  public synchronized void setPadPosition(int padPos) {
    properties.setPaddingLocation(PaddingLocation.fromOld(padPos));
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized boolean isScientificNotation() {
    return ScientificFormat.useScientificNotation(properties);
  }

  /** @stable ICU 2.0 */
  public synchronized void setScientificNotation(boolean useScientific) {
    if (useScientific) {
      properties.setExponentDigits(1);
    } else {
      properties.setExponentDigits(Properties.DEFAULT_EXPONENT_DIGITS);
    }
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized byte getMinimumExponentDigits() {
    return (byte) exportedProperties.getExponentDigits();
  }

  /** @stable ICU 2.0 */
  public synchronized void setMinimumExponentDigits(byte minExpDig) {
    properties.setExponentDigits(minExpDig);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized boolean isExponentSignAlwaysShown() {
    return exportedProperties.getExponentShowPlusSign();
  }

  /** @stable ICU 2.0 */
  public synchronized void setExponentSignAlwaysShown(boolean expSignAlways) {
    properties.setExponentShowPlusSign(expSignAlways);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized int getGroupingSize() {
    return exportedProperties.getGroupingSize();
  }

  /** @stable ICU 2.0 */
  public synchronized void setGroupingSize(int newValue) {
    properties.setGroupingSize(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  public synchronized int getSecondaryGroupingSize() {
    return exportedProperties.getSecondaryGroupingSize();
  }

  /** @stable ICU 2.0 */
  public synchronized void setSecondaryGroupingSize(int newValue) {
    properties.setSecondaryGroupingSize(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized boolean isGroupingUsed() {
    return PositiveDecimalFormat.useGrouping(properties);
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized void setGroupingUsed(boolean newValue) {
    if (newValue) {
      // Set to a reasonable default value
      properties.setGroupingSize(3);
    } else {
      properties.setGroupingSize(Properties.DEFAULT_GROUPING_SIZE);
      properties.setSecondaryGroupingSize(Properties.DEFAULT_SECONDARY_GROUPING_SIZE);
    }
    refreshFormatter();
  }

  /** Remember the ICU math context form in order to be able to return it from the API. */
  private int icuMathContextForm = MathContext.PLAIN;

  /** @stable ICU 4.2 */
  public synchronized MathContext getMathContextICU() {
    java.math.MathContext mathContext = getMathContext();
    return new MathContext(
        mathContext.getPrecision(),
        icuMathContextForm,
        false,
        mathContext.getRoundingMode().ordinal());
  }

  /** @stable ICU 4.2 */
  public synchronized void setMathContextICU(MathContext newValue) {
    icuMathContextForm = newValue.getForm();
    java.math.MathContext mathContext;
    if (newValue.getLostDigits()) {
      // The getLostDigits() feature in ICU MathContext means "throw an ArithmeticException if
      // rounding causes digits to be lost". That feature is called RoundingMode.UNNECESSARY in
      // Java MathContext.
      mathContext = new java.math.MathContext(
          newValue.getDigits(),
          RoundingMode.UNNECESSARY);
    } else {
      mathContext = new java.math.MathContext(
          newValue.getDigits(),
          RoundingMode.valueOf(newValue.getRoundingMode()));
    }
    setMathContext(mathContext);
  }

  /** @stable ICU 4.2 */
  public synchronized java.math.MathContext getMathContext() {
    java.math.MathContext mathContext = exportedProperties.getMathContext();
    assert mathContext != null;
    return mathContext;
  }

  /** @stable ICU 4.2 */
  public synchronized void setMathContext(java.math.MathContext newValue) {
    properties.setMathContext(newValue);
    refreshFormatter();
  }

  /** @stable ICU 54 */
  public synchronized void setDecimalPatternMatchRequired(boolean value) {
    properties.setDecimalPatternMatchRequired(value);
    refreshFormatter();
  }

  /** @stable ICU 54 */
  public synchronized boolean isDecimalPatternMatchRequired() {
    return properties.getDecimalPatternMatchRequired();
  }

  /** @stable ICU 2.0 */
  public synchronized boolean isDecimalSeparatorAlwaysShown() {
    return exportedProperties.getAlwaysShowDecimal();
  }

  /** @stable ICU 2.0 */
  public synchronized void setDecimalSeparatorAlwaysShown(boolean newValue) {
    properties.setAlwaysShowDecimal(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized int getMaximumIntegerDigits() {
    return exportedProperties.getMaximumIntegerDigits();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized int getMinimumIntegerDigits() {
    return exportedProperties.getMinimumIntegerDigits();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized void setMaximumIntegerDigits(int newValue) {
    properties.setMaximumIntegerDigits(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized void setMinimumIntegerDigits(int newValue) {
    properties.setMinimumIntegerDigits(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized int getMaximumFractionDigits() {
    return exportedProperties.getMaximumFractionDigits();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized int getMinimumFractionDigits() {
    return exportedProperties.getMinimumFractionDigits();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized void setMaximumFractionDigits(int newValue) {
    properties.setMaximumFractionDigits(newValue);
    refreshFormatter();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized void setMinimumFractionDigits(int newValue) {
    properties.setMinimumFractionDigits(newValue);
    refreshFormatter();
  }

  /** @stable ICU 3.0 */
  public synchronized int getMinimumSignificantDigits() {
    return exportedProperties.getMinimumSignificantDigits();
  }

  /** @stable ICU 3.0 */
  public synchronized int getMaximumSignificantDigits() {
    return exportedProperties.getMaximumSignificantDigits();
  }

  /** @stable ICU 3.0 */
  public synchronized void setMinimumSignificantDigits(int min) {
    properties.setMinimumSignificantDigits(min);
    refreshFormatter();
  }

  /** @stable ICU 3.0 */
  public synchronized void setMaximumSignificantDigits(int max) {
    properties.setMaximumSignificantDigits(max);
    refreshFormatter();
  }

  /** @stable ICU 3.0 */
  public synchronized boolean areSignificantDigitsUsed() {
    return SignificantDigitsRounder.useSignificantDigits(properties);
  }

  /** @stable ICU 3.0 */
  public synchronized void setSignificantDigitsUsed(boolean useSignificantDigits) {
    if (useSignificantDigits) {
      // These are the default values from the old implementation.
      properties.setMinimumSignificantDigits(1);
      properties.setMaximumSignificantDigits(6);
    } else {
      properties.setMinimumSignificantDigits(Properties.DEFAULT_MINIMUM_SIGNIFICANT_DIGITS);
      properties.setMaximumSignificantDigits(Properties.DEFAULT_MAXIMUM_SIGNIFICANT_DIGITS);
    }
    refreshFormatter();
  }

  @Override
  public synchronized Currency getCurrency() {
    return properties.getCurrency();
  }

  /** @stable ICU 2.2 */
  @Override
  public synchronized void setCurrency(Currency theCurrency) {
    properties.setCurrency(theCurrency);
    // Backwards compatibility: also set the currency in the DecimalFormatSymbols
    if (theCurrency != null) {
      symbols.setCurrency(theCurrency);
      String symbol = theCurrency.getName(symbols.getULocale(), Currency.SYMBOL_NAME, null);
      symbols.setCurrencySymbol(symbol);
    }
    refreshFormatter();
  }

  /** @stable ICU 54 */
  public synchronized void setCurrencyUsage(CurrencyUsage newUsage) {
    properties.setCurrencyUsage(newUsage);
    refreshFormatter();
  }

  /** @stable ICU 54 */
  public synchronized CurrencyUsage getCurrencyUsage() {
    // CurrencyUsage is not exported, so we have to get it from the input property bag.
    // TODO: Should we export CurrencyUsage instead?
    CurrencyUsage usage = properties.getCurrencyUsage();
    if (usage == null) {
      usage = CurrencyUsage.STANDARD;
    }
    return usage;
  }

  /** @stable ICU 4.2 */
  public CurrencyPluralInfo getCurrencyPluralInfo() {
    // CurrencyPluralInfo also is not exported.
    return properties.getCurrencyPluralInfo();
  }

  /** @stable ICU 4.2 */
  public void setCurrencyPluralInfo(CurrencyPluralInfo newInfo) {
    properties.setCurrencyPluralInfo(newInfo);
    refreshFormatter();
  }

  /** @stable ICU 3.6 */
  public synchronized void setParseBigDecimal(boolean value) {
    properties.setParseToBigDecimal(value);
    // refreshFormatter() not needed
  }

  /** @stable ICU 3.6 */
  public synchronized boolean isParseBigDecimal() {
    return properties.getParseToBigDecimal();
  }

  /**
   * Setting max parse digits has no effect since ICU 59.
   *
   * @stable ICU 51
   */
  public synchronized void setParseMaxDigits(int _) {
  }

  /**
   * Setting max parse digits has no effect since ICU 59.
   * Always returns 1000.
   *
   * @stable ICU 51
   */
  public synchronized int getParseMaxDigits() {
    return 1000;
  }

  @Override
  public synchronized void setParseStrict(boolean parseStrict) {
    Parse.ParseMode mode = parseStrict ? Parse.ParseMode.STRICT : Parse.ParseMode.LENIENT;
    properties.setParseMode(mode);
    // refreshFormatter() not needed
  }

  @Override
  public synchronized boolean isParseStrict() {
    return properties.getParseMode() == Parse.ParseMode.STRICT;
  }

  @Override
  public synchronized void setParseIntegerOnly(boolean parseIntegerOnly) {
    properties.setParseIntegerOnly(parseIntegerOnly);
    // refreshFormatter() not needed
  }

  @Override
  public synchronized boolean isParseIntegerOnly() {
    return properties.getParseIntegerOnly();
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized boolean equals(Object obj) {
    if (obj == null) return false;
    if (obj == this) return true;
    if (!(obj instanceof DecimalFormat)) return false;
    DecimalFormat other = (DecimalFormat) obj;
    return properties.equals(other.properties) && symbols.equals(other.symbols);
  }

  /** @stable ICU 2.0 */
  @Override
  public synchronized int hashCode() {
    return properties.hashCode();
  }

  private static final ThreadLocal<Properties> threadLocalToPatternProperties =
      new ThreadLocal<Properties>() {
    @Override
    protected Properties initialValue() {
      return new Properties();
    }
  };

  /** @stable ICU 2.0 */
  public synchronized String toPattern() {
    // Pull some properties from exportedProperties and others from properties
    // to keep affix patterns intact.  In particular, pull rounding properties
    // so that CurrencyUsage is reflected properly.
    // TODO: Consider putting this logic in PatternString.java instead.
    Properties tprops = threadLocalToPatternProperties.get();
    tprops.copyFrom(properties);
    if (com.ibm.icu.impl.number.formatters.CurrencyFormat.useCurrency(properties)) {
      tprops.setMinimumFractionDigits(exportedProperties.getMinimumFractionDigits());
      tprops.setMaximumFractionDigits(exportedProperties.getMaximumFractionDigits());
      tprops.setRoundingInterval(exportedProperties.getRoundingInterval());
    }
    return PatternString.propertiesToString(tprops);
  }

  /** @stable ICU 2.0 */
  public synchronized String toLocalizedPattern() {
    String pattern = toPattern();
    return PatternString.convertLocalized(pattern, symbols, true);
  }

  /**
   * @internal
   * @deprecated This API is ICU internal only.
   */
  @Deprecated
  public IFixedDecimal getFixedDecimal(double number) {
    FormatQuantity4 fq = new FormatQuantity4(number);
    formatter.format(fq);
    return fq;
  }

  /**
   * Rebuilds the formatter object from the property bag.
   */
  void refreshFormatter() {
    formatter = Endpoint.fromBTA(properties, symbols);
    exportedProperties.clear();
    formatter.export(exportedProperties);
  }

  /**
   * Updates the property bag with settings from the given pattern.
   * @param pattern The pattern string to parse.
   * @param ignoreRounding Whether to read rounding information from the string.  Set to false if
   * CurrencyUsage is to be used instead.
   * @see PatternString#parseToExistingProperties
   */
  void setPropertiesFromPattern(String pattern, boolean ignoreRounding) {
    PatternString.parseToExistingProperties(pattern, properties, ignoreRounding);
  }

  /**
   * @internal
   * @deprecated This API is a technical preview.
   */
  @Deprecated
  public synchronized void setProperties(PropertySetter func) {
    func.set(properties);
    refreshFormatter();
  }

  public static interface PropertySetter {
    public void set(Properties props);
  }

  /**
   * {@icu} Constant for {@link #getPadPosition()} and {@link #setPadPosition(int)} to specify pad
   * characters inserted before the prefix.
   *
   * @see #setPadPosition
   * @see #getPadPosition
   * @see #PAD_AFTER_PREFIX
   * @see #PAD_BEFORE_SUFFIX
   * @see #PAD_AFTER_SUFFIX
   * @stable ICU 2.0
   */
  public static final int PAD_BEFORE_PREFIX = 0;

  /**
   * {@icu} Constant for {@link #getPadPosition()} and {@link #setPadPosition(int)} to specify pad
   * characters inserted after the prefix.
   *
   * @see #setPadPosition
   * @see #getPadPosition
   * @see #PAD_BEFORE_PREFIX
   * @see #PAD_BEFORE_SUFFIX
   * @see #PAD_AFTER_SUFFIX
   * @stable ICU 2.0
   */
  public static final int PAD_AFTER_PREFIX = 1;

  /**
   * {@icu} Constant for {@link #getPadPosition()} and {@link #setPadPosition(int)} to specify pad
   * characters inserted before the suffix.
   *
   * @see #setPadPosition
   * @see #getPadPosition
   * @see #PAD_BEFORE_PREFIX
   * @see #PAD_AFTER_PREFIX
   * @see #PAD_AFTER_SUFFIX
   * @stable ICU 2.0
   */
  public static final int PAD_BEFORE_SUFFIX = 2;

  /**
   * {@icu} Constant for {@link #getPadPosition()} and {@link #setPadPosition(int)} to specify pad
   * characters inserted after the suffix.
   *
   * @see #setPadPosition
   * @see #getPadPosition
   * @see #PAD_BEFORE_PREFIX
   * @see #PAD_AFTER_PREFIX
   * @see #PAD_BEFORE_SUFFIX
   * @stable ICU 2.0
   */
  public static final int PAD_AFTER_SUFFIX = 3;
}
