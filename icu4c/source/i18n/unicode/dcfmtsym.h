/*
********************************************************************************
*   Copyright (C) 1997-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File DCFMTSYM.H
*
* Modification History:
* 
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/18/97    clhuang     Updated per C++ implementation.
*   03/27/97    helena      Updated to pass the simple test after code review.
*   08/26/97    aliu        Added currency/intl currency symbol support.
*   07/22/98    stephen     Changed to match C++ style 
*                            currencySymbol -> fCurrencySymbol
*                            Constants changed from CAPS to kCaps
*   06/24/99    helena      Integrated Alan's NF enhancements and Java2 bug fixes
*   09/22/00    grhoten     Marked deprecation tags with a pointer to replacement
*                            functions.
********************************************************************************
*/
 
#ifndef DCFMTSYM_H
#define DCFMTSYM_H
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"
#include "unicode/locid.h"

U_NAMESPACE_BEGIN

/**
 * This class represents the set of symbols needed by DecimalFormat
 * to format numbers. DecimalFormat creates for itself an instance of
 * DecimalFormatSymbols from its locale data.  If you need to change any
 * of these symbols, you can get the DecimalFormatSymbols object from
 * your DecimalFormat and modify it.
 * <P>
 * Here are the special characters used in the parts of the
 * subpattern, with notes on their usage.
 * <pre>
 * \code
 *        Symbol   Meaning
 *          0      a digit
 *          #      a digit, zero shows as absent
 *          .      placeholder for decimal separator
 *          ,      placeholder for grouping separator.
 *          ;      separates formats.
 *          -      default negative prefix.
 *          %      divide by 100 and show as percentage
 *          X      any other characters can be used in the prefix or suffix
 *          '      used to quote special characters in a prefix or suffix.
 * \endcode
 *  </pre>
 * [Notes]
 * <P>
 * If there is no explicit negative subpattern, - is prefixed to the
 * positive form. That is, "0.00" alone is equivalent to "0.00;-0.00".
 * <P>
 * The grouping separator is commonly used for thousands, but in some
 * countries for ten-thousands. The interval is a constant number of
 * digits between the grouping characters, such as 100,000,000 or 1,0000,0000.
 * If you supply a pattern with multiple grouping characters, the interval
 * between the last one and the end of the integer is the one that is
 * used. So "#,##,###,####" == "######,####" == "##,####,####".
 * <P>
 * This class only handles localized digits where the 10 digits are
 * contiguous in Unicode, from 0 to 9. Other digits sets (such as
 * superscripts) would need a different subclass.
 */
class U_I18N_API DecimalFormatSymbols : public UObject {
public:
    /**
     * Constants for specifying a number format symbol.
     * @stable ICU 2.0
     */
    enum ENumberFormatSymbol {
        /** The decimal separator */
        kDecimalSeparatorSymbol,
        /** The grouping separator */
        kGroupingSeparatorSymbol,
        /** The pattern separator */
        kPatternSeparatorSymbol,
        /** The percent sign */
        kPercentSymbol,
        /** Zero*/
        kZeroDigitSymbol,
        /** Character representing a digit in the pattern */
        kDigitSymbol,
        /** The minus sign */
        kMinusSignSymbol,
        /** The plus sign */
        kPlusSignSymbol,
        /** The currency symbol */
        kCurrencySymbol,
        /** The international currency symbol */
        kIntlCurrencySymbol,
        /** The monetary separator */
        kMonetarySeparatorSymbol,
        /** The exponential symbol */
        kExponentialSymbol,
        /** Per mill symbol - replaces kPermillSymbol */
        kPerMillSymbol,
        /** Escape padding character */
        kPadEscapeSymbol,
        /** Infinity symbol */
        kInfinitySymbol,
        /** Nan symbol */
        kNaNSymbol,
        /** Significant digit symbol
         * @draft ICU 3.0 */
        kSignificantDigitSymbol,
        /** count symbol constants */
        kFormatSymbolCount
    };

    /**
     * Create a DecimalFormatSymbols object for the given locale.
     *
     * @param locale    The locale to get symbols for.
     * @param status    Input/output parameter, set to success or
     *                  failure code upon return.
     * @stable ICU 2.0
     */
    DecimalFormatSymbols(const Locale& locale, UErrorCode& status);

    /**
     * Create a DecimalFormatSymbols object for the default locale.
     * This constructor will not fail.  If the resource file data is
     * not available, it will use hard-coded last-resort data and
     * set status to U_USING_FALLBACK_ERROR.
     *
     * @param status    Input/output parameter, set to success or
     *                  failure code upon return.
     * @stable ICU 2.0
     */
    DecimalFormatSymbols( UErrorCode& status);

    /**
     * Copy constructor.
     * @stable ICU 2.0
     */
    DecimalFormatSymbols(const DecimalFormatSymbols&);

    /**
     * Assignment operator.
     * @stable ICU 2.0
     */
    DecimalFormatSymbols& operator=(const DecimalFormatSymbols&);

    /**
     * Destructor.
     * @stable ICU 2.0
     */
    ~DecimalFormatSymbols();

    /**
     * Return true if another object is semantically equal to this one.
     *
     * @param other    the object to be compared with.
     * @return         true if another object is semantically equal to this one.
     * @stable ICU 2.0
     */
    UBool operator==(const DecimalFormatSymbols& other) const;

    /**
     * Return true if another object is semantically unequal to this one.
     *
     * @param other    the object to be compared with.
     * @return         true if another object is semantically unequal to this one.
     * @stable ICU 2.0
     */
    UBool operator!=(const DecimalFormatSymbols& other) const { return !operator==(other); }

    /**
     * Get one of the format symbols by its enum constant.
     * Each symbol is stored as a string so that graphemes
     * (characters with modifyer letters) can be used.
     *
     * @param symbol    Constant to indicate a number format symbol.
     * @return    the format symbols by the param 'symbol'
     * @stable ICU 2.0
     */
    inline UnicodeString getSymbol(ENumberFormatSymbol symbol) const;

    /**
     * Set one of the format symbols by its enum constant.
     * Each symbol is stored as a string so that graphemes
     * (characters with modifyer letters) can be used.
     *
     * @param symbol    Constant to indicate a number format symbol.
     * @param value     value of the format sybmol
     * @stable ICU 2.0
     */
    void setSymbol(ENumberFormatSymbol symbol, const UnicodeString &value);

    /**
     * Returns the locale for which this object was constructed.
     * @stable ICU 2.6
     */
    inline Locale getLocale() const;

    /**
     * Returns the locale for this object. Two flavors are available:
     * valid and actual locale.
     * @draft ICU 2.8 likely to change in ICU 3.0, based on feedback
     */
    Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 2.2
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 2.2
     */
    static UClassID getStaticClassID();

private:
    DecimalFormatSymbols(); // default constructor not implemented

    /**
     * Initializes the symbols from the LocaleElements resource bundle.
     * Note: The organization of LocaleElements badly needs to be
     * cleaned up.
     *
     * @param locale               The locale to get symbols for.
     * @param success              Input/output parameter, set to success or
     *                             failure code upon return.
     * @param useLastResortData    determine if use last resort data
     */
    void initialize(const Locale& locale, UErrorCode& success, UBool useLastResortData = FALSE);

    /**
     * Initialize the symbols from the given array of UnicodeStrings.
     * The array must be of the correct size.
     * 
     * @param numberElements    the number format symbols
     * @param numberElementsLength length of numberElements
     */
    void initialize(const UnicodeString* numberElements, int32_t numberElementsLength);

    /**
     * Initialize the symbols with default values.
     */
    void initialize();

    void setCurrencyForSymbols();

public:
    /**
     * _Internal_ function - more efficient version of getSymbol,
     * returning a const reference to one of the symbol strings.
     * The returned reference becomes invalid when the symbol is changed
     * or when the DecimalFormatSymbols are destroyed.
     * ### TODO markus 2002oct11: Consider proposing getConstSymbol() to be really public.
     *
     * @param symbol Constant to indicate a number format symbol.
     * @return the format symbol by the param 'symbol'
     * @internal
     */
    inline const UnicodeString &getConstSymbol(ENumberFormatSymbol symbol) const;

private:
    /**
     * Private symbol strings.
     * They are either loaded from a resource bundle or otherwise owned.
     * setSymbol() clones the symbol string.
     * Readonly aliases can only come from a resource bundle, so that we can always
     * use fastCopyFrom() with them.
     *
     * If DecimalFormatSymbols becomes subclassable and the status of fSymbols changes
     * from private to protected,
     * or when fSymbols can be set any other way that allows them to be readonly aliases
     * to non-resource bundle strings,
     * then regular UnicodeString copies must be used instead of fastCopyFrom().
     *
     * @internal
     */
    UnicodeString fSymbols[kFormatSymbolCount];

    /**
     * Non-symbol variable for getConstSymbol(). Always empty.
     * @internal
     */
    UnicodeString fNoSymbol;

    Locale locale;

    char actualLocale[ULOC_FULLNAME_CAPACITY];
    char validLocale[ULOC_FULLNAME_CAPACITY];
};

// -------------------------------------

inline UnicodeString
DecimalFormatSymbols::getSymbol(ENumberFormatSymbol symbol) const {
    const UnicodeString *strPtr;
    if(symbol < kFormatSymbolCount) {
        strPtr = &fSymbols[symbol];
    } else {
        strPtr = &fNoSymbol;
    }
    return *strPtr;
}

inline const UnicodeString &
DecimalFormatSymbols::getConstSymbol(ENumberFormatSymbol symbol) const {
    const UnicodeString *strPtr;
    if(symbol < kFormatSymbolCount) {
        strPtr = &fSymbols[symbol];
    } else {
        strPtr = &fNoSymbol;
    }
    return *strPtr;
}

// -------------------------------------

inline void
DecimalFormatSymbols::setSymbol(ENumberFormatSymbol symbol, const UnicodeString &value) {
    if(symbol<kFormatSymbolCount) {
        fSymbols[symbol]=value;
    }
}

// -------------------------------------

inline Locale
DecimalFormatSymbols::getLocale() const {
    return locale;
}


U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _DCFMTSYM
//eof
