// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
 *******************************************************************************
 * Copyright (C) 2008-2015, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.dev.test.localespi;

import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.TimeZone;

import org.junit.Test;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.text.TimeZoneNames;
import com.ibm.icu.text.TimeZoneNames.NameType;
import com.ibm.icu.util.ULocale;

public class TimeZoneNameTest extends TestFmwk {

    private static final Set<String> ProblematicZones = new HashSet<String>();
    static {
        // Since tzdata2013e, Pacific/Johnston is defined as below:
        //
        //     Link Pacific/Honolulu Pacific/Johnston
        //
        // JDK TimeZone.getDisplayName no longer passes Pacific/Johnston to a
        // TimeZoneNameProvider implementation. As of CLDR 25M1, Pacific/Johnston
        // has a different set of names from Pacific/Honolulu. This test case
        // expects JRE calls a TimeZoneNameProvider without such normalization
        // (and I believe it's a JDK bug). For now, we ignore the test failure
        // caused by Pacific/Johnston with this the JDK problem.
        ProblematicZones.add("Pacific/Johnston");
    }

    @Test
    public void TestTimeZoneNames() {
        Locale[] locales = Locale.getAvailableLocales();
        String[] tzids = TimeZone.getAvailableIDs();

        for (Locale loc : locales) {
            boolean warningOnly = false;
            if (TestUtil.isExcluded(loc)) {
                warningOnly = true;
            }

            for (String tzid : tzids) {
                // Java has a problem when a provider does not supply all 4 names
                // for a zone. For this reason, ICU TimeZoneName provider does not return
                // localized names unless these 4 names are available. 

                String icuStdLong = getIcuDisplayName(tzid, false, TimeZone.LONG, loc);
                String icuDstLong = getIcuDisplayName(tzid, true, TimeZone.LONG, loc);
                String icuStdShort = getIcuDisplayName(tzid, false, TimeZone.SHORT, loc);
                String icuDstShort = getIcuDisplayName(tzid, true, TimeZone.SHORT, loc);

                if (icuStdLong != null && icuDstLong != null && icuStdShort != null && icuDstShort != null) {
                    checkDisplayNamePair(TimeZone.SHORT, tzid, loc, warningOnly || ProblematicZones.contains(tzid));
                    checkDisplayNamePair(TimeZone.LONG, tzid, loc, warningOnly || ProblematicZones.contains(tzid));
                } else {
                    logln("Localized long standard name is not available for "
                            + tzid + " in locale " + loc + " in ICU");
                }
            }
        }
    }

    private void checkDisplayNamePair(int style, String tzid, Locale loc, boolean warnOnly) {
        /* Note: There are two problems here.
         * 
         * It looks Java 6 requires a TimeZoneNameProvider to return both standard name and daylight name
         * for a zone.  If the provider implementation only returns either of them, Java 6 also ignore
         * the other.  In ICU, there are zones which do not have daylight names, especially zones which
         * do not use daylight time.  This test case does not check a standard name if its daylight name
         * is not available because of the Java 6 implementation problem.
         * 
         * Another problem is that ICU always use a standard name for a zone which does not use daylight
         * saving time even daylight name is requested.
         */

        String icuStdName = getIcuDisplayName(tzid, false, style, loc);
        String icuDstName = getIcuDisplayName(tzid, true, style, loc);
        if (icuStdName != null && icuDstName != null && !icuStdName.equals(icuDstName)) {
            checkDisplayName(false, style, tzid, loc, icuStdName, warnOnly);
            checkDisplayName(true, style, tzid, loc, icuDstName, warnOnly);
        }
    }

    private String getIcuDisplayName(String tzid, boolean daylight, int style, Locale loc) {
        String icuName = null;
        boolean[] isSystemID = new boolean[1];
        String canonicalID = com.ibm.icu.util.TimeZone.getCanonicalID(tzid, isSystemID);
        if (isSystemID[0]) {
            long date = System.currentTimeMillis();
            TimeZoneNames tznames = TimeZoneNames.getInstance(ULocale.forLocale(loc));
            switch (style) {
            case TimeZone.LONG:
                icuName = daylight ?
                        tznames.getDisplayName(canonicalID, NameType.LONG_DAYLIGHT, date) :
                        tznames.getDisplayName(canonicalID, NameType.LONG_STANDARD, date);
                break;
            case TimeZone.SHORT:
                icuName = daylight ?
                        tznames.getDisplayName(canonicalID, NameType.SHORT_DAYLIGHT, date) :
                        tznames.getDisplayName(canonicalID, NameType.SHORT_STANDARD, date);
                break;
            }
        }
        return icuName;
    }

    private void checkDisplayName(boolean daylight, int style,  String tzid, Locale loc, String icuname, boolean warnOnly) {
        String styleStr = (style == TimeZone.SHORT) ? "SHORT" : "LONG";
        TimeZone tz = TimeZone.getTimeZone(tzid);
        String name = tz.getDisplayName(daylight, style, loc);

        if (TestUtil.isICUExtendedLocale(loc)) {
            // The name should be taken from ICU
            if (!name.equals(icuname)) {
                if (warnOnly) {
                    logln("WARNING: TimeZone name by ICU is " + icuname + ", but got " + name
                            + " for time zone " + tz.getID() + " in locale " + loc
                            + " (daylight=" + daylight + ", style=" + styleStr + ")");
                    
                } else {
                    errln("FAIL: TimeZone name by ICU is " + icuname + ", but got " + name
                            + " for time zone " + tz.getID() + " in locale " + loc
                            + " (daylight=" + daylight + ", style=" + styleStr + ")");
                }
            }
        } else {
            if (!name.equals(icuname)) {
                logln("INFO: TimeZone name by ICU is " + icuname + ", but got " + name
                        + " for time zone " + tz.getID() + " in locale " + loc
                        + " (daylight=" + daylight + ", style=" + styleStr + ")");
            }
            // Try explicit ICU locale (xx_yy_ICU)
            Locale icuLoc = TestUtil.toICUExtendedLocale(loc);
            name = tz.getDisplayName(daylight, style, icuLoc);
            if (!name.equals(icuname)) {
                if (warnOnly) {
                    logln("WARNING: TimeZone name by ICU is " + icuname + ", but got " + name
                            + " for time zone " + tz.getID() + " in locale " + icuLoc
                            + " (daylight=" + daylight + ", style=" + styleStr + ")");
                } else {
                    errln("FAIL: TimeZone name by ICU is " + icuname + ", but got " + name
                            + " for time zone " + tz.getID() + " in locale " + icuLoc
                            + " (daylight=" + daylight + ", style=" + styleStr + ")");
                }
            }
        }
    }
}
