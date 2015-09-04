/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

/**
 * @author rocketman
 *
 */
public final class VisibleDigits {
    static final int NEGATIVE = 1;
    static final int INFINITE = 2;
    static final int NAN = 4;
    public static final byte[] NO_DIGITS = new byte[0];
    public static final VisibleDigits NOT_A_NUMBER = createSpecial(NAN);
    public static final VisibleDigits NEGATIVE_INFINITY = createSpecial(NEGATIVE | INFINITE);
    public static final VisibleDigits POSITIVE_INFINITY = createSpecial(INFINITE);
    
    
    private byte[] fDigits;
    private DigitInterval fInterval;
    private int fExponent;
    private int fFlags;
    private long fAbsIntValue;
    private double fAbsDoubleValue;
    private boolean fAbsValuesSet;
    
    static VisibleDigits create(
            byte[] frozenDigits,
            int exponent,
            DigitInterval frozenInterval,
            int flags,
            long absIntValue,
            double absDoubleValue,
            boolean absValueSet) {
        return new VisibleDigits(
                frozenDigits,
                exponent,
                frozenInterval,
                flags,
                absIntValue,
                absDoubleValue,
                absValueSet && !isOverMaxDigits(exponent, frozenDigits, frozenInterval));
    }
    
    private static VisibleDigits createSpecial(int flags) {
        return new VisibleDigits(NO_DIGITS, 0, DigitInterval.SINGLE_INT_DIGIT, flags, 0L, 0.0, false);
    }
    
    private VisibleDigits(byte[] digits, int exponent, DigitInterval interval, int flags, long absIntValue,
            double absDoubleValue, boolean absValuesSet) {
        fDigits = digits;
        fExponent = exponent;
        fInterval = interval;
        fFlags = flags;
        fAbsIntValue = absIntValue;
        fAbsDoubleValue = absDoubleValue;
        fAbsValuesSet = absValuesSet;
    }

    public boolean isNegative() {
        return (fFlags & NEGATIVE) != 0;
    }
    
    public boolean isNaN() {
        return (fFlags & NAN) != 0;
    }
    
    public boolean isInfinite() {
        return (fFlags & INFINITE) != 0;
    }
    
    public boolean isNaNOrInfinity() {
        return (fFlags & (NAN | INFINITE)) != 0;
    }
    
    public int getDigitByExponent(int digitPos) {
        if (digitPos < fExponent || digitPos >= fExponent + fDigits.length) {
            return 0;
        }
        return fDigits[digitPos - fExponent] & 0xFF;    
    }
    
    public DigitInterval getInterval() {
        return fInterval;
    }
    
    public String toString() {
        StringBuilder result = new StringBuilder();
        if (isNegative()) {
            result.append('-');
        }
        for (int i = fInterval.getMostSignificantExclusive() - 1; i >= fInterval.getLeastSignificantInclusive(); --i) {
            if (i == -1) {
                result.append('.');
            }
            result.append((char) ('0' + getDigitByExponent(i)));
        }
        return result.toString();
    }
    
    private static boolean isOverMaxDigits(int exponent, byte[] digits, DigitInterval interval) {
        return (exponent + digits.length > interval.getIntDigitCount());
    }
    
    private double computeAbsDoubleValue() {
        // Take care of NaN and infinity
        if (isNaN()) {
            return Double.NaN;
        }
        if (isInfinite()) {
            return Double.POSITIVE_INFINITY;
        }
        return Double.parseDouble(absValueString());
    }
    
    private String absValueString() {
        int i;
        StringBuilder builder = new StringBuilder();
        int mostSig = fInterval.getMostSignificantExclusive();
        int mostSigNonZero = fExponent + fDigits.length;
        int end = mostSig > mostSigNonZero ? mostSigNonZero : mostSig;
        int leastSig = fInterval.getLeastSignificantInclusive();
        int start = leastSig > fExponent ? leastSig : fExponent;
        if (end <= start) {
            return "0";
        }
        for (i = end - 1; i >= start; --i) {
            if (i == -1) {
                builder.append('.');
            }
            builder.append((char) ('0' + fDigits[i - fExponent]));            
        }
        for (; i >= 0; --i) {
            builder.append('0');
        }
        return builder.toString();
    }
    
    public static final class VFixedDecimal {
        
        public final double source;
        public final long integerValue;
        public final long decimalDigits;
        public final long decimalDigitsWithoutTrailingZeros;
        public final int visibleDecimalDigitCount;
        public final boolean hasIntegerValue;
        
        public VFixedDecimal(double n, long i, long f, long t, int v, boolean hasInt) {
            source = n;
            integerValue = i;
            decimalDigits = f;
            decimalDigitsWithoutTrailingZeros = t;
            visibleDecimalDigitCount = v;
            hasIntegerValue = hasInt;
        }
        
        public static VFixedDecimal ZERO = new VFixedDecimal(0.0, 0, 0, 0, 0, false);
    }

    public VFixedDecimal getFixedDecimal() {
        double source = 0.0;
        long intValue = 0;
        long f = 0;
        long t = 0;
        int v = 0;
        boolean hasIntValue = false;
        if (isNaNOrInfinity()) {
            return VFixedDecimal.ZERO;
        }

        // source
        if (fAbsValuesSet) {
            source = fAbsDoubleValue;
        } else {
            source = computeAbsDoubleValue();
        }

        // visible decimal digits
        v = fInterval.getFracDigitCount();

        // intValue

        // If we initialized from an int64 just use that instead of
        // calculating
        if (fAbsValuesSet) {
            intValue = fAbsIntValue;
        } else {
            int startPos = fInterval.getIntDigitCount();
            if (startPos > 18) {
                startPos = 18;
            }
            // process the integer digits
            for (int i = startPos - 1; i >= 0; --i) {
                intValue = intValue * 10L + getDigitByExponent(i);
            }
            if (intValue == 0 && startPos > 0) {
                intValue = 100000000000000000L;
            }
        }

        // f (decimal digits)
        // skip over any leading 0's in fraction digits.
        int idx = -1;
        for (; idx >= -v && getDigitByExponent(idx) == 0; --idx);

        // Only process up to first 18 non zero fraction digits for decimalDigits
        // since that is all we can fit into an int64.
        for (int i = idx; i >= -v && i > idx - 18; --i) {
            f = f * 10L + getDigitByExponent(i);
        }

        // If we have no decimal digits, we don't have an integer value
        hasIntValue = (f == 0L);

        // t (decimal digits without trailing zeros)
        t = f;
        while (t > 0 && t % 10L == 0) {
            t /= 10;
        }
        return new VFixedDecimal(source, intValue, f, t, v, hasIntValue);
    }

    
    public VisibleDigits withNegative() {
        return new VisibleDigits(
                fDigits,
                fExponent,
                fInterval,
                fFlags | NEGATIVE,
                fAbsIntValue,
                fAbsDoubleValue,
                fAbsValuesSet);
    }
}
