/*********************************************************************
 * Copyright (C) 2000, International Business Machines Corporation and
 * others. All Rights Reserved.
 *********************************************************************
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/text/ChineseDateFormatSymbols.java,v $
 * $Date: 2002/02/16 03:06:04 $
 * $Revision: 1.2 $
 */
package com.ibm.icu.text;
import com.ibm.icu.util.*;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.MissingResourceException;

/**
 * A subclass of {@link DateFormatSymbols} for {@link ChineseDateFormat}.
 * This class contains additional symbols corresponding to the
 * <code>ChineseCalendar.IS_LEAP_MONTH</code> field.
 *
 * @see ChineseDateFormat
 * @see com.ibm.icu.util.ChineseCalendar
 * @author Alan Liu
 */
public class ChineseDateFormatSymbols extends DateFormatSymbols {
    
    /**
     * Package-private array that ChineseDateFormat needs to be able to
     * read.
     */
    String isLeapMonth[]; // Do NOT add =null initializer

    public ChineseDateFormatSymbols() {
        this(Locale.getDefault());
    }

    public ChineseDateFormatSymbols(Locale locale) {
        super(ChineseCalendar.class, locale);
    }

    public ChineseDateFormatSymbols(Calendar cal, Locale locale) {
        super(cal==null?null:cal.getClass(), locale);
    }

    // New API
    public String getLeapMonth(int isLeapMonth) {
        return this.isLeapMonth[isLeapMonth];
    }

    /**
     * Override DateFormatSymbols.
     */
    protected void constructCalendarSpecific(ResourceBundle bundle) {
        super.constructCalendarSpecific(bundle);
        if (bundle != null) {
            try {
                isLeapMonth = bundle.getStringArray("IsLeapMonth");
            } catch (MissingResourceException e) {}
        }
    }
}
