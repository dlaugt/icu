/*****************************************************************************************
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/test/format/Attic/IntlTestNumberFormatAPI.java,v $ 
 * $Date: 2001/10/19 12:13:23 $ 
 * $Revision: 1.1 $
 *
 *****************************************************************************************
 **/

/** 
 * Port From:   JDK 1.4b1 : java.text.Format.IntlTestNumberFormatAPI
 * Source File: java/text/format/IntlTestNumberFormatAPI.java
 **/

/*
    @test 1.4 98/03/06
    @summary test International Number Format API
*/
/*
(C) Copyright Taligent, Inc. 1996, 1997 - All Rights Reserved
(C) Copyright IBM Corp. 1996, 1997, 2001 - All Rights Reserved

  The original version of this source code and documentation is copyrighted and
owned by Taligent, Inc., a wholly-owned subsidiary of IBM. These materials are
provided under terms of a License Agreement between Taligent and Sun. This
technology is protected by multiple US and International patents. This notice and
attribution to Taligent may not be removed.
  Taligent is a registered trademark of Taligent, Inc.
*/

package com.ibm.icu.test.format;

import com.ibm.text.*;
import com.ibm.util.*;
import java.util.Locale;
import java.text.FieldPosition;
import java.text.ParsePosition;
import java.text.ParseException;

public class IntlTestNumberFormatAPI extends com.ibm.test.TestFmwk
{
    public static void main(String[] args) throws Exception {
        new IntlTestNumberFormatAPI().run(args);
    }

    // This test checks various generic API methods in DecimalFormat to achieve 100% API coverage.
    public void TestAPI()
    {
        logln("NumberFormat API test---"); logln("");
        Locale.setDefault(Locale.ENGLISH);

        // ======= Test constructors

        logln("Testing NumberFormat constructors");

        NumberFormat def = NumberFormat.getInstance();

        NumberFormat fr = NumberFormat.getInstance(Locale.FRENCH);

        NumberFormat cur = NumberFormat.getCurrencyInstance();

        NumberFormat cur_fr = NumberFormat.getCurrencyInstance(Locale.FRENCH);

        NumberFormat per = NumberFormat.getPercentInstance();

        NumberFormat per_fr = NumberFormat.getPercentInstance(Locale.FRENCH);
        
        NumberFormat integer = NumberFormat.getIntegerInstance();
        
        NumberFormat int_fr = NumberFormat.getIntegerInstance(Locale.FRENCH);

        // ======= Test equality

        logln("Testing equality operator");

        if( per_fr.equals(cur_fr) ) {
            errln("ERROR: == failed");
        }

        // ======= Test various format() methods

        logln("Testing various format() methods");

//        final double d = -10456.0037; // this appears as -10456.003700000001 on NT
//        final double d = -1.04560037e-4; // this appears as -1.0456003700000002E-4 on NT
        final double d = -10456.00370000000000; // this works!
        final long l = 100000000;

        String res1 = new String();
        String res2 = new String();
        StringBuffer res3 = new StringBuffer();
        StringBuffer res4 = new StringBuffer();
        StringBuffer res5 = new StringBuffer();
        StringBuffer res6 = new StringBuffer();
        FieldPosition pos1 = new FieldPosition(0);
        FieldPosition pos2 = new FieldPosition(0);
        FieldPosition pos3 = new FieldPosition(0);
        FieldPosition pos4 = new FieldPosition(0);

        res1 = cur_fr.format(d);
        logln( "" + d + " formatted to " + res1);

        res2 = cur_fr.format(l);
        logln("" + l + " formatted to " + res2);

        res3 = cur_fr.format(d, res3, pos1);
        logln( "" + d + " formatted to " + res3);

        res4 = cur_fr.format(l, res4, pos2);
        logln("" + l + " formatted to " + res4);

        res5 = cur_fr.format(d, res5, pos3);
        logln("" + d + " formatted to " + res5);

        res6 = cur_fr.format(l, res6, pos4);
        logln("" + l + " formatted to " + res6);


        // ======= Test parse()

        logln("Testing parse()");

//        String text = new String("-10,456.0037");
        String text = new String("-10456,0037");
        ParsePosition pos = new ParsePosition(0);
        ParsePosition pos01 = new ParsePosition(0);
        double d1 = ((Number)fr.parseObject(text, pos)).doubleValue();
        if(d1 != d) {
            errln("ERROR: Roundtrip failed (via parse()) for " + text);
        }
        logln(text + " parsed into " + d1);

        double d2 = fr.parse(text, pos01).doubleValue();
        if(d2 != d) {
            errln("ERROR: Roundtrip failed (via parse()) for " + text);
        }
        logln(text + " parsed into " + d2);

        double d3 = 0;
        try {
            d3 = fr.parse(text).doubleValue();
        }
        catch (ParseException e) {
            errln("ERROR: parse() failed");
        }
        if(d3 != d) {
            errln("ERROR: Roundtrip failed (via parse()) for " + text);
        }
        logln(text + " parsed into " + d3);


        // ======= Test getters and setters

        logln("Testing getters and setters");

        final Locale[] locales = NumberFormat.getAvailableLocales();
        long count = locales.length;
        logln("Got " + count + " locales" );
        for(int i = 0; i < count; i++) {
            String name;
            name = locales[i].getDisplayName();
            logln(name);
        }

        fr.setParseIntegerOnly( def.isParseIntegerOnly() );
        if(fr.isParseIntegerOnly() != def.isParseIntegerOnly() ) {
                errln("ERROR: setParseIntegerOnly() failed");
        }

        fr.setGroupingUsed( def.isGroupingUsed() );
        if(fr.isGroupingUsed() != def.isGroupingUsed() ) {
                errln("ERROR: setGroupingUsed() failed");
        }

        fr.setMaximumIntegerDigits( def.getMaximumIntegerDigits() );
        if(fr.getMaximumIntegerDigits() != def.getMaximumIntegerDigits() ) {
                errln("ERROR: setMaximumIntegerDigits() failed");
        }

        fr.setMinimumIntegerDigits( def.getMinimumIntegerDigits() );
        if(fr.getMinimumIntegerDigits() != def.getMinimumIntegerDigits() ) {
                errln("ERROR: setMinimumIntegerDigits() failed");
        }

        fr.setMaximumFractionDigits( def.getMaximumFractionDigits() );
        if(fr.getMaximumFractionDigits() != def.getMaximumFractionDigits() ) {
                errln("ERROR: setMaximumFractionDigits() failed");
        }

        fr.setMinimumFractionDigits( def.getMinimumFractionDigits() );
        if(fr.getMinimumFractionDigits() != def.getMinimumFractionDigits() ) {
                errln("ERROR: setMinimumFractionDigits() failed");
        }

        // ======= Test getStaticClassID()

//        logln("Testing instanceof()");

//        try {
//            NumberFormat test = new DecimalFormat();

//            if (! (test instanceof DecimalFormat)) {
//                errln("ERROR: instanceof failed");
//            }
//        }
//        catch (Exception e) {
//            errln("ERROR: Couldn't create a DecimalFormat");
//        }
    }
}
