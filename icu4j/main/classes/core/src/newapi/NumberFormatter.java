// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package newapi;

import java.util.Locale;

import com.ibm.icu.impl.number.DecimalFormatProperties;
import com.ibm.icu.text.DecimalFormatSymbols;
import com.ibm.icu.util.ULocale;

import newapi.impl.MacroProps;

public final class NumberFormatter {

    private static final UnlocalizedNumberFormatter BASE = new UnlocalizedNumberFormatter();

    public static enum UnitWidth {
        NARROW, // ¤¤¤¤¤ or narrow measure unit
        SHORT, // ¤ or short measure unit (DEFAULT)
        ISO_CODE, // ¤¤; undefined for measure unit
        FULL_NAME, // ¤¤¤ or wide unit
        HIDDEN, // no unit is displayed, but other unit effects are obeyed (like currency rounding)
        // TODO: For hidden, what to do if currency symbol appears in the middle, as in Portugal ?
    }

    public static enum DecimalMarkDisplay {
        AUTO, ALWAYS,
    }

    public static enum SignDisplay {
        AUTO, ALWAYS, NEVER, ACCOUNTING, ACCOUNTING_ALWAYS,
    }

    /**
     * Use a default threshold of 3. This means that the third time .format() is called, the data structures get built
     * using the "safe" code path. The first two calls to .format() will trigger the unsafe code path.
     */
    static final long DEFAULT_THRESHOLD = 3;

    public static UnlocalizedNumberFormatter with() {
        return BASE;
    }

    public static LocalizedNumberFormatter withLocale(Locale locale) {
        return BASE.locale(locale);
    }

    public static LocalizedNumberFormatter withLocale(ULocale locale) {
        return BASE.locale(locale);
    }

    /**
     * @internal
     * @deprecated ICU 60 This API is ICU internal only.
     */
    @Deprecated
    public static UnlocalizedNumberFormatter fromDecimalFormat(DecimalFormatProperties properties,
            DecimalFormatSymbols symbols, DecimalFormatProperties exportedProperties) {
        MacroProps macros = NumberPropertyMapper.oldToNew(properties, symbols, exportedProperties);
        return NumberFormatter.with().macros(macros);
    }
}
