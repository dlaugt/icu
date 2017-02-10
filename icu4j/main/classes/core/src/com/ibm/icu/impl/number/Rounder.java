// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.impl.number;

import java.math.RoundingMode;

import com.ibm.icu.impl.number.formatters.CompactDecimalFormat;
import com.ibm.icu.impl.number.formatters.ScientificFormat;

/**
 * The base class for a Rounder used by ICU Decimal Format.
 *
 * <p>A Rounder must implement the method {@link #apply}. Most implementations should have the code
 * <code>applyDefaults(input);</code> in their apply function.
 *
 * <p>In order to be used by {@link CompactDecimalFormat} and {@link ScientificFormat}, among
 * others, your rounder must be stable upon <em>decreasing</em> the magnitude of the input number.
 * For example, if your rounder converts "999" to "1000", it must also convert "99.9" to "100" and
 * "0.999" to "1". (The opposite does not need to be the case: you can round "0.999" to "1" but keep
 * "999" as "999".)
 */
public abstract class Rounder extends Format.BeforeFormat {

  public static interface IBasicRoundingProperties {

    static int DEFAULT_MINIMUM_INTEGER_DIGITS = -1;

    /** @see #setMinimumIntegerDigits */
    public int getMinimumIntegerDigits();

    /**
     * Sets the minimum number of digits to display before the decimal point. If the number has
     * fewer than this number of digits, the number will be padded with zeros. The pattern "#00.0#",
     * for example, corresponds to 2 minimum integer digits, and the number 5.3 would be formatted
     * as "05.3" in locale <em>en-US</em>.
     *
     * @param minimumIntegerDigits The minimum number of integer digits to output.
     * @return The property bag, for chaining.
     */
    public IBasicRoundingProperties setMinimumIntegerDigits(int minimumIntegerDigits);

    static int DEFAULT_MAXIMUM_INTEGER_DIGITS = -1;

    /** @see #setMaximumIntegerDigits */
    public int getMaximumIntegerDigits();

    /**
     * Sets the maximum number of digits to display before the decimal point. If the number has more
     * than this number of digits, the extra digits will be truncated. For example, if maximum
     * integer digits is 2, and you attempt to format the number 1970, you will get "70" in locale
     * <em>en-US</em>. It is not possible to specify the maximum integer digits using a pattern
     * string, except in the special case of a scientific format pattern.
     *
     * @param maximumIntegerDigits The maximum number of integer digits to output.
     * @return The property bag, for chaining.
     */
    public IBasicRoundingProperties setMaximumIntegerDigits(int maximumIntegerDigits);

    static int DEFAULT_MINIMUM_FRACTION_DIGITS = -1;

    /** @see #setMinimumFractionDigits */
    public int getMinimumFractionDigits();

    /**
     * Sets the minimum number of digits to display after the decimal point. If the number has fewer
     * than this number of digits, the number will be padded with zeros. The pattern "#00.0#", for
     * example, corresponds to 1 minimum fraction digit, and the number 456 would be formatted as
     * "456.0" in locale <em>en-US</em>.
     *
     * @param minimumFractionDigits The minimum number of fraction digits to output.
     * @return The property bag, for chaining.
     */
    public IBasicRoundingProperties setMinimumFractionDigits(int minimumFractionDigits);

    static int DEFAULT_MAXIMUM_FRACTION_DIGITS = -1;

    /** @see #setMaximumFractionDigits */
    public int getMaximumFractionDigits();

    /**
     * Sets the maximum number of digits to display after the decimal point. If the number has fewer
     * than this number of digits, the number will be rounded off using the rounding mode specified
     * by {@link #setRoundingMode(RoundingMode)}. The pattern "#00.0#", for example, corresponds to
     * 2 maximum fraction digits, and the number 456.789 would be formatted as "456.79" in locale
     * <em>en-US</em> with the default rounding mode. Note that the number 456.999 would be
     * formatted as "457.0" given the same configurations.
     *
     * @param maximumFractionDigits The maximum number of fraction digits to output.
     * @return The property bag, for chaining.
     */
    public IBasicRoundingProperties setMaximumFractionDigits(int maximumFractionDigits);

    static RoundingMode DEFAULT_ROUNDING_MODE = RoundingMode.HALF_EVEN;

    /** @see #setRoundingMode */
    public RoundingMode getRoundingMode();

    /**
     * Sets the rounding mode, which determines under which conditions extra decimal places are
     * rounded either up or down. See {@link RoundingMode} for details on the choices of rounding
     * mode. The default if not set explicitly is {@link RoundingMode#HALF_EVEN}.
     *
     * @param roundingMode The rounding mode to use when rounding is required.
     * @return The property bag, for chaining.
     * @see RoundingMode
     */
    public IBasicRoundingProperties setRoundingMode(RoundingMode roundingMode);
  }

  public static interface MultiplierGenerator {
    public int getMultiplier(int magnitude);
  }

  // Properties available to all rounding strategies
  protected final RoundingMode roundingMode;
  protected final int minInt;
  protected final int maxInt;
  protected final int minFrac;
  protected final int maxFrac;

  /**
   * Constructor that uses integer and fraction digit lengths from IBasicRoundingProperties.
   *
   * @param properties
   */
  protected Rounder(IBasicRoundingProperties properties) {
    roundingMode = properties.getRoundingMode();
    int _maxInt = properties.getMaximumIntegerDigits();
    int _minInt = properties.getMinimumIntegerDigits();
    int _maxFrac = properties.getMaximumFractionDigits();
    int _minFrac = properties.getMinimumFractionDigits();
    maxInt = _maxInt < 0 ? Integer.MAX_VALUE : _maxInt;
    minInt = _minInt < 0 ? 0 : _minInt < maxInt ? _minInt : maxInt;
    maxFrac = _maxFrac < 0 ? Integer.MAX_VALUE : _maxFrac;
    minFrac = _minFrac < 0 ? 0 : _minFrac < maxFrac ? _minFrac : maxFrac;
  }

  /**
   * Perform rounding and specification of integer and fraction digit lengths on the input quantity.
   * Calling this method will change the state of the FormatQuantity.
   *
   * @param input The {@link FormatQuantity} to be modified and rounded.
   */
  public abstract void apply(FormatQuantity input);

  /**
   * Rounding can affect the magnitude. First we attempt to adjust according to the original
   * magnitude, and if the magnitude changes, we adjust according to a magnitude one greater. Note
   * that this algorithm assumes that increasing the multiplier never increases the number of digits
   * that can be displayed.
   *
   * @param input The quantity to be rounded.
   * @param mg The implementation that returns magnitude adjustment based on a given starting
   *     magnitude.
   * @return The multiplier that was chosen to best fit the input.
   */
  public int chooseMultiplierAndApply(FormatQuantity input, MultiplierGenerator mg) {
    FormatQuantity copy = input.clone();

    int magnitude = input.getMagnitude();
    int multiplier = mg.getMultiplier(magnitude);
    input.adjustMagnitude(multiplier);
    apply(input);
    if (input.getMagnitude() == magnitude + multiplier + 1) {
      magnitude += 1;
      input.copyFrom(copy);
      multiplier = mg.getMultiplier(magnitude);
      input.adjustMagnitude(multiplier);
      assert input.getMagnitude() == magnitude + multiplier - 1;
      apply(input);
      assert input.getMagnitude() == magnitude + multiplier;
    }

    return multiplier;
  }

  /**
   * Implementations can call this method to perform default logic for min/max digits. This method
   * performs logic for handling of a zero input.
   *
   * @param input The digits being formatted.
   */
  protected void applyDefaults(FormatQuantity input) {
    if (input.isZero()) {
      if (minInt == 0 && maxFrac > 0) {
        // Force zeros after the decimal point
        input.setIntegerFractionLength(minInt, maxInt, Math.max(1, minFrac), maxFrac);
      } else {
        // Force zeros before the decimal point
        input.setIntegerFractionLength(Math.max(1, minInt), Math.max(1, maxInt), minFrac, maxFrac);
      }
    } else {
      input.setIntegerFractionLength(minInt, maxInt, minFrac, maxFrac);
    }
  }

  private static final ThreadLocal<Properties> threadLocalProperties =
      new ThreadLocal<Properties>() {
        @Override
        protected Properties initialValue() {
          return new Properties();
        }
      };

  /**
   * Gets a thread-local property bag that can be used to deliver properties to a constructor.
   * Rounders themselves are guaranteed to not internally use a copy of this property bag.
   *
   * @return A clean, thread-local property bag.
   */
  public static Properties getThreadLocalProperties() {
    return threadLocalProperties.get().clear();
  }

  @Override
  public void before(FormatQuantity input, ModifierHolder mods) {
    apply(input);
  }

  @Override
  public void export(Properties properties) {
    properties.setRoundingMode(roundingMode);
    properties.setMinimumFractionDigits(minFrac);
    properties.setMinimumIntegerDigits(minInt);
    properties.setMaximumFractionDigits(maxFrac);
    properties.setMaximumIntegerDigits(maxInt);
  }
}
