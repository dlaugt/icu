/*
**********************************************************************
* Copyright (c) 2004-2014, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: April 20, 2004
* Since: ICU 3.0
**********************************************************************
*/
package com.ibm.icu.text;

import java.io.ObjectStreamException;
import java.text.FieldPosition;
import java.text.ParsePosition;

import com.ibm.icu.util.CurrencyAmount;
import com.ibm.icu.util.Measure;
import com.ibm.icu.util.ULocale;

/**
 * Temporary internal concrete subclass of MeasureFormat implementing
 * parsing and formatting of CurrencyAmount objects.  This class is
 * likely to be redesigned and rewritten in the near future.
 *
 * <p>This class currently delegates to DecimalFormat for parsing and
 * formatting.
 *
 * @see com.ibm.icu.text.UFormat
 * @see com.ibm.icu.text.DecimalFormat
 * @author Alan Liu
 */
class CurrencyFormat extends MeasureFormat {
    // Generated by serialver from JDK 1.4.1_01
    static final long serialVersionUID = -931679363692504634L;
    
    private NumberFormat fmt;
    private transient final MeasureFormat mf;

    public CurrencyFormat(ULocale locale) {
        // Needed for getLocale(ULocale.VALID_LOCALE).
        setLocale(locale, locale);
        mf = MeasureFormat.getInstance(locale, FormatWidth.WIDE);
        fmt = NumberFormat.getCurrencyInstance(locale.toLocale());
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public Object clone() {
        CurrencyFormat result = (CurrencyFormat) super.clone();
        result.fmt = (NumberFormat) fmt.clone();
        return result;
    }

    /**
     * Override Format.format().
     * @see java.text.Format#format(java.lang.Object, java.lang.StringBuffer, java.text.FieldPosition)
     */
    public StringBuffer format(Object obj, StringBuffer toAppendTo, FieldPosition pos) {
        if (!(obj instanceof CurrencyAmount)) {
            throw new IllegalArgumentException("Invalid type: " + obj.getClass().getName());
        }
        CurrencyAmount currency = (CurrencyAmount) obj;
            
        fmt.setCurrency(currency.getCurrency());
        return fmt.format(currency.getNumber(), toAppendTo, pos);
    }

    /**
     * Override Format.parseObject().
     * @see java.text.Format#parseObject(java.lang.String, java.text.ParsePosition)
     */
    @Override
    public CurrencyAmount parseObject(String source, ParsePosition pos) {
        return fmt.parseCurrency(source, pos);
    }
    
    // boilerplate code to make CurrencyFormat otherwise follow the contract of
    // MeasureFormat
    
    /**
     * {@inheritDoc}
     */
    @Override
    public StringBuilder formatMeasures(
            StringBuilder appendTo, FieldPosition fieldPosition, Measure... measures) {
        return mf.formatMeasures(appendTo, fieldPosition, measures);
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public MeasureFormat.FormatWidth getWidth() {
        return mf.getWidth();
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public NumberFormat getNumberFormat() {
        return mf.getNumberFormat();
    }
    
    // End boilerplate.
    
    // Serialization
    
    private Object writeReplace() throws ObjectStreamException {
        return mf.toCurrencyProxy();
    }
    
    // Preserve backward serialize backward compatibility.
    private Object readResolve() throws ObjectStreamException {
        return new CurrencyFormat(fmt.getLocale(ULocale.ACTUAL_LOCALE));
    }
}
