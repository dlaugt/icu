/*
 *******************************************************************************
 * Copyright (C) 1996-2000, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/util/Attic/Holiday.java,v $ 
 * $Date: 2000/03/10 04:17:58 $ 
 * $Revision: 1.3 $
 *
 *****************************************************************************************
 */

package com.ibm.util;

import java.util.Locale;
import java.util.ResourceBundle;
import java.util.Date;
import java.util.MissingResourceException;

/**
 * An abstract class representing a holiday.  This class simply adds
 * a holiday name to the DateRule interface for calculating the date
 * of an event.
 *
 */
public abstract class Holiday implements DateRule
{
    public static Holiday[] getHolidays()
    {
        return getHolidays(Locale.getDefault());
    }

    public static Holiday[] getHolidays(Locale locale)
    {
        Holiday[] result = noHolidays;

        try {
            ResourceBundle bundle = ResourceBundle.getBundle(
                                    "com.ibm.util.resources.HolidayBundle",
                                    locale);

            result = (Holiday[]) bundle.getObject("holidays");
        }
        catch (MissingResourceException e) {
        }
        return result;
    }

    /**
     * Return the first occurrance of this holiday on or after the given date
     *
     * @param start Only holidays on or after this date are returned.
     *
     * @return      The date on which this holiday occurs, or null if it
     *              does not occur on or after the start date.
     *
     * @see #firstBetween
     */
    public Date firstAfter(Date start) {
        return rule.firstAfter(start);
    }

    /**
     * Return the first occurrance of this holiday that is on or after
     * the given start date and before the given end date.
     *
     * @param start Only occurrances on or after this date are returned.
     * @param end   Only occurrances before this date are returned.
     *
     * @return      The date on which this event occurs, or null if it
     *              does not occur between the start and end dates.
     *
     * @see #firstAfter
     */
    public Date firstBetween(Date start, Date end) {
        return rule.firstBetween(start, end);
    }

    /**
     * Checks whether this holiday falls on the given date.  This does
     * <em>not</em> take time of day into account; instead it checks
     * whether the holiday and the given date are on the same day.
     *
     * @param date  The date to check.
     * @return      true if this holiday occurs on the given date.
     *
     */
    public boolean isOn(Date date) {
        //System.out.println(name + ".isOn(" + date.toString() + "):");
        return rule.isOn(date);
    }

    /**
     * Check whether this holiday occurs at least once between the two
     * dates given.
     */
    public boolean isBetween(Date start, Date end) {
        return rule.isBetween(start, end);
    }

    /**
     * Construct a new Holiday object.  This is for use by subclasses only.
     * This constructs a new holiday with the given name and date rules.
     *
     * @param name  The name of this holiday.  The getDisplayName method
     *              uses this string as a key to look up the holiday's name a
     *              resource bundle object named HolidayBundle.
     *
     * @param rule  The date rules used for determining when this holiday
     *              falls.  Holiday's implementation of the DateRule inteface
     *              simply delegates to this DateRule object.
     */
    protected Holiday(String name, DateRule rule)
    {
        this.name = name;
        this.rule = rule;
    }

    /**
     * Return the name of this holiday in the language of the default locale
     */
    public String getDisplayName() {
        return getDisplayName(Locale.getDefault());
    }

    /**
     * Return the name of this holiday in the language of the specified locale
     * The <code>name</code> parameter passed to this object's constructor is used
     * as a key to look up the holiday's localized name in a ResourceBundle object
     * named HolidayBundle.
     *
     * @param loc   A locale specifying the language in which the name is desired.
     *
     * @see ResourceBundle
     */
    public String getDisplayName(Locale locale)
    {
        String name = this.name;

        try {
            ResourceBundle bundle = ResourceBundle.getBundle(
                                    "com.ibm.util.resources.HolidayBundle",
                                    locale);
            name = bundle.getString(name);
        }
        catch (MissingResourceException e) {
            //System.out.println("Using default display name for " + name);
        }
        return name;
    }

    public DateRule getRule() {
        return rule;
    }

    public void setRule(DateRule rule) {
        this.rule = rule;
    }

    private String      name;
    private DateRule    rule;

    private static Holiday[] noHolidays = {};

}
