// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.dev.test.number;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import com.ibm.icu.impl.number.DecimalQuantity;
import com.ibm.icu.impl.number.DecimalQuantity_DualStorageBCD;
import com.ibm.icu.impl.number.NumberStringBuilder;
import com.ibm.icu.impl.number.PatternStringParser;
import com.ibm.icu.text.DecimalFormatSymbols;
import com.ibm.icu.util.Currency;
import com.ibm.icu.util.ULocale;

import newapi.NumberFormatter.SignDisplay;
import newapi.NumberFormatter.UnitWidth;
import newapi.impl.MicroProps;
import newapi.impl.MutablePatternModifier;

public class MutablePatternModifierTest {

    @Test
    public void basic() {
        MutablePatternModifier mod = new MutablePatternModifier(false);
        mod.setPatternInfo(PatternStringParser.parseToPatternInfo("a0b"));
        mod.setPatternAttributes(SignDisplay.AUTO, false);
        mod.setSymbols(
                DecimalFormatSymbols.getInstance(ULocale.ENGLISH),
                Currency.getInstance("USD"),
                UnitWidth.SHORT,
                null);

        mod.setNumberProperties(false, null);
        assertEquals("a", getPrefix(mod));
        assertEquals("b", getSuffix(mod));
        mod.setPatternAttributes(SignDisplay.ALWAYS, false);
        assertEquals("+a", getPrefix(mod));
        assertEquals("b", getSuffix(mod));
        mod.setNumberProperties(true, null);
        assertEquals("-a", getPrefix(mod));
        assertEquals("b", getSuffix(mod));
        mod.setPatternAttributes(SignDisplay.NEVER, false);
        assertEquals("a", getPrefix(mod));
        assertEquals("b", getSuffix(mod));

        mod.setPatternInfo(PatternStringParser.parseToPatternInfo("a0b;c-0d"));
        mod.setPatternAttributes(SignDisplay.AUTO, false);
        mod.setNumberProperties(false, null);
        assertEquals("a", getPrefix(mod));
        assertEquals("b", getSuffix(mod));
        mod.setPatternAttributes(SignDisplay.ALWAYS, false);
        assertEquals("c+", getPrefix(mod));
        assertEquals("d", getSuffix(mod));
        mod.setNumberProperties(true, null);
        assertEquals("c-", getPrefix(mod));
        assertEquals("d", getSuffix(mod));
        mod.setPatternAttributes(SignDisplay.NEVER, false);
        assertEquals("c-", getPrefix(mod)); // TODO: What should this behavior be?
        assertEquals("d", getSuffix(mod));
    }

    @Test
    public void mutableEqualsImmutable() {
        MutablePatternModifier mod = new MutablePatternModifier(false);
        mod.setPatternInfo(PatternStringParser.parseToPatternInfo("a0b;c-0d"));
        mod.setPatternAttributes(SignDisplay.AUTO, false);
        mod.setSymbols(DecimalFormatSymbols.getInstance(ULocale.ENGLISH), null, UnitWidth.SHORT, null);
        DecimalQuantity fq = new DecimalQuantity_DualStorageBCD(1);

        NumberStringBuilder nsb1 = new NumberStringBuilder();
        MicroProps micros1 = new MicroProps(false);
        mod.addToChain(micros1);
        mod.processQuantity(fq);
        micros1.modMiddle.apply(nsb1, 0, 0);

        NumberStringBuilder nsb2 = new NumberStringBuilder();
        MicroProps micros2 = new MicroProps(true);
        mod.createImmutable().applyToMicros(micros2, fq);
        micros2.modMiddle.apply(nsb2, 0, 0);

        NumberStringBuilder nsb3 = new NumberStringBuilder();
        MicroProps micros3 = new MicroProps(false);
        mod.addToChain(micros3);
        mod.setPatternAttributes(SignDisplay.ALWAYS, false);
        mod.processQuantity(fq);
        micros3.modMiddle.apply(nsb3, 0, 0);

        assertTrue(nsb1 + " vs. " + nsb2, nsb1.contentEquals(nsb2));
        assertFalse(nsb1 + " vs. " + nsb3, nsb1.contentEquals(nsb3));
    }

    private static String getPrefix(MutablePatternModifier mod) {
        NumberStringBuilder nsb = new NumberStringBuilder();
        mod.apply(nsb, 0, 0);
        return nsb.subSequence(0, mod.getPrefixLength()).toString();
    }

    private static String getSuffix(MutablePatternModifier mod) {
        NumberStringBuilder nsb = new NumberStringBuilder();
        mod.apply(nsb, 0, 0);
        return nsb.subSequence(mod.getPrefixLength(), nsb.length()).toString();
    }
}
