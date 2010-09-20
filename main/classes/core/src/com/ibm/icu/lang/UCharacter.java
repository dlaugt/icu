/**
*******************************************************************************
* Copyright (C) 1996-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

package com.ibm.icu.lang;

import java.lang.ref.SoftReference;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Locale;
import java.util.Map;

import com.ibm.icu.impl.IllegalIcuArgumentException;
import com.ibm.icu.impl.Norm2AllModes;
import com.ibm.icu.impl.Normalizer2Impl;
import com.ibm.icu.impl.Trie2;
import com.ibm.icu.impl.UBiDiProps;
import com.ibm.icu.impl.UCaseProps;
import com.ibm.icu.impl.UCharacterName;
import com.ibm.icu.impl.UCharacterNameChoice;
import com.ibm.icu.impl.UCharacterProperty;
import com.ibm.icu.impl.UCharacterUtility;
import com.ibm.icu.impl.UPropertyAliases;
import com.ibm.icu.lang.UCharacterEnums.ECharacterCategory;
import com.ibm.icu.lang.UCharacterEnums.ECharacterDirection;
import com.ibm.icu.text.BreakIterator;
import com.ibm.icu.text.UTF16;
import com.ibm.icu.util.RangeValueIterator;
import com.ibm.icu.util.ULocale;
import com.ibm.icu.util.ValueIterator;
import com.ibm.icu.util.VersionInfo;

/**
 * {@icuenhanced java.lang.Character}.{@icu _usage_}
 *
 * <p>The UCharacter class provides extensions to the
 * <a href="http://java.sun.com/j2se/1.5/docs/api/java/lang/Character.html">
 * java.lang.Character</a> class. These extensions provide support for
 * more Unicode properties and together with the <a href=../text/UTF16.html>UTF16</a>
 * class, provide support for supplementary characters (those with code
 * points above U+FFFF).
 * Each ICU release supports the latest version of Unicode available at that time.
 *
 * <p>Code points are represented in these API using ints. While it would be
 * more convenient in Java to have a separate primitive datatype for them,
 * ints suffice in the meantime.
 *
 * <p>To use this class please add the jar file name icu4j.jar to the
 * class path, since it contains data files which supply the information used
 * by this file.<br>
 * E.g. In Windows <br>
 * <code>set CLASSPATH=%CLASSPATH%;$JAR_FILE_PATH/ucharacter.jar</code>.<br>
 * Otherwise, another method would be to copy the files uprops.dat and
 * unames.icu from the icu4j source subdirectory
 * <i>$ICU4J_SRC/src/com.ibm.icu.impl.data</i> to your class directory
 * <i>$ICU4J_CLASS/com.ibm.icu.impl.data</i>.
 *
 * <p>Aside from the additions for UTF-16 support, and the updated Unicode
 * properties, the main differences between UCharacter and Character are:
 * <ul>
 * <li> UCharacter is not designed to be a char wrapper and does not have
 *      APIs to which involves management of that single char.<br>
 *      These include:
 *      <ul>
 *        <li> char charValue(),
 *        <li> int compareTo(java.lang.Character, java.lang.Character), etc.
 *      </ul>
 * <li> UCharacter does not include Character APIs that are deprecated, nor
 *      does it include the Java-specific character information, such as
 *      boolean isJavaIdentifierPart(char ch).
 * <li> Character maps characters 'A' - 'Z' and 'a' - 'z' to the numeric
 *      values '10' - '35'. UCharacter also does this in digit and
 *      getNumericValue, to adhere to the java semantics of these
 *      methods.  New methods unicodeDigit, and
 *      getUnicodeNumericValue do not treat the above code points
 *      as having numeric values.  This is a semantic change from ICU4J 1.3.1.
 * </ul>
 * <p>
 * Further detail on differences can be determined using the program
 *        <a href=
 * "http://source.icu-project.org/repos/icu/icu4j/trunk/src/com/ibm/icu/dev/test/lang/UCharacterCompare.java">
 *        com.ibm.icu.dev.test.lang.UCharacterCompare</a>
 * </p>
 * <p>
 * In addition to Java compatibility functions, which calculate derived properties,
 * this API provides low-level access to the Unicode Character Database.
 * </p>
 * <p>
 * Unicode assigns each code point (not just assigned character) values for
 * many properties.
 * Most of them are simple boolean flags, or constants from a small enumerated list.
 * For some properties, values are strings or other relatively more complex types.
 * </p>
 * <p>
 * For more information see
 * <a href="http://www.unicode/org/ucd/">"About the Unicode Character Database"</a>
 * (http://www.unicode.org/ucd/)
 * and the <a href="http://www.icu-project.org/userguide/properties.html">ICU
 * User Guide chapter on Properties</a>
 * (http://www.icu-project.org/userguide/properties.html).
 * </p>
 * <p>
 * There are also functions that provide easy migration from C/POSIX functions
 * like isblank(). Their use is generally discouraged because the C/POSIX
 * standards do not define their semantics beyond the ASCII range, which means
 * that different implementations exhibit very different behavior.
 * Instead, Unicode properties should be used directly.
 * </p>
 * <p>
 * There are also only a few, broad C/POSIX character classes, and they tend
 * to be used for conflicting purposes. For example, the "isalpha()" class
 * is sometimes used to determine word boundaries, while a more sophisticated
 * approach would at least distinguish initial letters from continuation
 * characters (the latter including combining marks).
 * (In ICU, BreakIterator is the most sophisticated API for word boundaries.)
 * Another example: There is no "istitle()" class for titlecase characters.
 * </p>
 * <p>
 * ICU 3.4 and later provides API access for all twelve C/POSIX character classes.
 * ICU implements them according to the Standard Recommendations in
 * Annex C: Compatibility Properties of UTS #18 Unicode Regular Expressions
 * (http://www.unicode.org/reports/tr18/#Compatibility_Properties).
 * </p>
 * <p>
 * API access for C/POSIX character classes is as follows:
 * <pre>{@code
 * - alpha:     isUAlphabetic(c) or hasBinaryProperty(c, UProperty.ALPHABETIC)
 * - lower:     isULowercase(c) or hasBinaryProperty(c, UProperty.LOWERCASE)
 * - upper:     isUUppercase(c) or hasBinaryProperty(c, UProperty.UPPERCASE)
 * - punct:     ((1<<getType(c)) & ((1<<DASH_PUNCTUATION)|(1<<START_PUNCTUATION)|
 *               (1<<END_PUNCTUATION)|(1<<CONNECTOR_PUNCTUATION)|(1<<OTHER_PUNCTUATION)|
 *               (1<<INITIAL_PUNCTUATION)|(1<<FINAL_PUNCTUATION)))!=0
 * - digit:     isDigit(c) or getType(c)==DECIMAL_DIGIT_NUMBER
 * - xdigit:    hasBinaryProperty(c, UProperty.POSIX_XDIGIT)
 * - alnum:     hasBinaryProperty(c, UProperty.POSIX_ALNUM)
 * - space:     isUWhiteSpace(c) or hasBinaryProperty(c, UProperty.WHITE_SPACE)
 * - blank:     hasBinaryProperty(c, UProperty.POSIX_BLANK)
 * - cntrl:     getType(c)==CONTROL
 * - graph:     hasBinaryProperty(c, UProperty.POSIX_GRAPH)
 * - print:     hasBinaryProperty(c, UProperty.POSIX_PRINT)}</pre>
 * </p>
 * <p>
 * The C/POSIX character classes are also available in UnicodeSet patterns,
 * using patterns like [:graph:] or \p{graph}.
 * </p>
 *
 * {@icunote} There are several ICU (and Java) whitespace functions.
 * Comparison:<ul>
 * <li> isUWhiteSpace=UCHAR_WHITE_SPACE: Unicode White_Space property;
 *       most of general categories "Z" (separators) + most whitespace ISO controls
 *       (including no-break spaces, but excluding IS1..IS4 and ZWSP)
 * <li> isWhitespace: Java isWhitespace; Z + whitespace ISO controls but excluding no-break spaces
 * <li> isSpaceChar: just Z (including no-break spaces)</ul>
 * </p>
 * <p>
 * This class is not subclassable.
 * </p>
 * @author Syn Wee Quek
 * @stable ICU 2.1
 * @see com.ibm.icu.lang.UCharacterEnums
 */

public final class UCharacter implements ECharacterCategory, ECharacterDirection
{
    // public inner classes ----------------------------------------------

    /**
     * {@icuenhanced java.lang.Character.UnicodeBlock}.{@icu _usage_}
     *
     * A family of character subsets representing the character blocks in the
     * Unicode specification, generated from Unicode Data file Blocks.txt.
     * Character blocks generally define characters used for a specific script
     * or purpose. A character is contained by at most one Unicode block.
     *
     * {@icunote} All fields named XXX_ID are specific to ICU.
     *
     * @stable ICU 2.4
     */
    public static final class UnicodeBlock extends Character.Subset
    {
        // block id corresponding to icu4c -----------------------------------

        /**
         * @stable ICU 2.4
         */
        public static final int INVALID_CODE_ID = -1;
        /**
         * @stable ICU 2.4
         */
        public static final int BASIC_LATIN_ID = 1;
        /**
         * @stable ICU 2.4
         */
        public static final int LATIN_1_SUPPLEMENT_ID = 2;
        /**
         * @stable ICU 2.4
         */
        public static final int LATIN_EXTENDED_A_ID = 3;
        /**
         * @stable ICU 2.4
         */
        public static final int LATIN_EXTENDED_B_ID = 4;
        /**
         * @stable ICU 2.4
         */
        public static final int IPA_EXTENSIONS_ID = 5;
        /**
         * @stable ICU 2.4
         */
        public static final int SPACING_MODIFIER_LETTERS_ID = 6;
        /**
         * @stable ICU 2.4
         */
        public static final int COMBINING_DIACRITICAL_MARKS_ID = 7;
        /**
         * Unicode 3.2 renames this block to "Greek and Coptic".
         * @stable ICU 2.4
         */
        public static final int GREEK_ID = 8;
        /**
         * @stable ICU 2.4
         */
        public static final int CYRILLIC_ID = 9;
        /**
         * @stable ICU 2.4
         */
        public static final int ARMENIAN_ID = 10;
        /**
         * @stable ICU 2.4
         */
        public static final int HEBREW_ID = 11;
        /**
         * @stable ICU 2.4
         */
        public static final int ARABIC_ID = 12;
        /**
         * @stable ICU 2.4
         */
        public static final int SYRIAC_ID = 13;
        /**
         * @stable ICU 2.4
         */
        public static final int THAANA_ID = 14;
        /**
         * @stable ICU 2.4
         */
        public static final int DEVANAGARI_ID = 15;
        /**
         * @stable ICU 2.4
         */
        public static final int BENGALI_ID = 16;
        /**
         * @stable ICU 2.4
         */
        public static final int GURMUKHI_ID = 17;
        /**
         * @stable ICU 2.4
         */
        public static final int GUJARATI_ID = 18;
        /**
         * @stable ICU 2.4
         */
        public static final int ORIYA_ID = 19;
        /**
         * @stable ICU 2.4
         */
        public static final int TAMIL_ID = 20;
        /**
         * @stable ICU 2.4
         */
        public static final int TELUGU_ID = 21;
        /**
         * @stable ICU 2.4
         */
        public static final int KANNADA_ID = 22;
        /**
         * @stable ICU 2.4
         */
        public static final int MALAYALAM_ID = 23;
        /**
         * @stable ICU 2.4
         */
        public static final int SINHALA_ID = 24;
        /**
         * @stable ICU 2.4
         */
        public static final int THAI_ID = 25;
        /**
         * @stable ICU 2.4
         */
        public static final int LAO_ID = 26;
        /**
         * @stable ICU 2.4
         */
        public static final int TIBETAN_ID = 27;
        /**
         * @stable ICU 2.4
         */
        public static final int MYANMAR_ID = 28;
        /**
         * @stable ICU 2.4
         */
        public static final int GEORGIAN_ID = 29;
        /**
         * @stable ICU 2.4
         */
        public static final int HANGUL_JAMO_ID = 30;
        /**
         * @stable ICU 2.4
         */
        public static final int ETHIOPIC_ID = 31;
        /**
         * @stable ICU 2.4
         */
        public static final int CHEROKEE_ID = 32;
        /**
         * @stable ICU 2.4
         */
        public static final int UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_ID = 33;
        /**
         * @stable ICU 2.4
         */
        public static final int OGHAM_ID = 34;
        /**
         * @stable ICU 2.4
         */
        public static final int RUNIC_ID = 35;
        /**
         * @stable ICU 2.4
         */
        public static final int KHMER_ID = 36;
        /**
         * @stable ICU 2.4
         */
        public static final int MONGOLIAN_ID = 37;
        /**
         * @stable ICU 2.4
         */
        public static final int LATIN_EXTENDED_ADDITIONAL_ID = 38;
        /**
         * @stable ICU 2.4
         */
        public static final int GREEK_EXTENDED_ID = 39;
        /**
         * @stable ICU 2.4
         */
        public static final int GENERAL_PUNCTUATION_ID = 40;
        /**
         * @stable ICU 2.4
         */
        public static final int SUPERSCRIPTS_AND_SUBSCRIPTS_ID = 41;
        /**
         * @stable ICU 2.4
         */
        public static final int CURRENCY_SYMBOLS_ID = 42;
        /**
         * Unicode 3.2 renames this block to "Combining Diacritical Marks for
         * Symbols".
         * @stable ICU 2.4
         */
        public static final int COMBINING_MARKS_FOR_SYMBOLS_ID = 43;
        /**
         * @stable ICU 2.4
         */
        public static final int LETTERLIKE_SYMBOLS_ID = 44;
        /**
         * @stable ICU 2.4
         */
        public static final int NUMBER_FORMS_ID = 45;
        /**
         * @stable ICU 2.4
         */
        public static final int ARROWS_ID = 46;
        /**
         * @stable ICU 2.4
         */
        public static final int MATHEMATICAL_OPERATORS_ID = 47;
        /**
         * @stable ICU 2.4
         */
        public static final int MISCELLANEOUS_TECHNICAL_ID = 48;
        /**
         * @stable ICU 2.4
         */
        public static final int CONTROL_PICTURES_ID = 49;
        /**
         * @stable ICU 2.4
         */
        public static final int OPTICAL_CHARACTER_RECOGNITION_ID = 50;
        /**
         * @stable ICU 2.4
         */
        public static final int ENCLOSED_ALPHANUMERICS_ID = 51;
        /**
         * @stable ICU 2.4
         */
        public static final int BOX_DRAWING_ID = 52;
        /**
         * @stable ICU 2.4
         */
        public static final int BLOCK_ELEMENTS_ID = 53;
        /**
         * @stable ICU 2.4
         */
        public static final int GEOMETRIC_SHAPES_ID = 54;
        /**
         * @stable ICU 2.4
         */
        public static final int MISCELLANEOUS_SYMBOLS_ID = 55;
        /**
         * @stable ICU 2.4
         */
        public static final int DINGBATS_ID = 56;
        /**
         * @stable ICU 2.4
         */
        public static final int BRAILLE_PATTERNS_ID = 57;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_RADICALS_SUPPLEMENT_ID = 58;
        /**
         * @stable ICU 2.4
         */
        public static final int KANGXI_RADICALS_ID = 59;
        /**
         * @stable ICU 2.4
         */
        public static final int IDEOGRAPHIC_DESCRIPTION_CHARACTERS_ID = 60;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_SYMBOLS_AND_PUNCTUATION_ID = 61;
        /**
         * @stable ICU 2.4
         */
        public static final int HIRAGANA_ID = 62;
        /**
         * @stable ICU 2.4
         */
        public static final int KATAKANA_ID = 63;
        /**
         * @stable ICU 2.4
         */
        public static final int BOPOMOFO_ID = 64;
        /**
         * @stable ICU 2.4
         */
        public static final int HANGUL_COMPATIBILITY_JAMO_ID = 65;
        /**
         * @stable ICU 2.4
         */
        public static final int KANBUN_ID = 66;
        /**
         * @stable ICU 2.4
         */
        public static final int BOPOMOFO_EXTENDED_ID = 67;
        /**
         * @stable ICU 2.4
         */
        public static final int ENCLOSED_CJK_LETTERS_AND_MONTHS_ID = 68;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_COMPATIBILITY_ID = 69;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A_ID = 70;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_UNIFIED_IDEOGRAPHS_ID = 71;
        /**
         * @stable ICU 2.4
         */
        public static final int YI_SYLLABLES_ID = 72;
        /**
         * @stable ICU 2.4
         */
        public static final int YI_RADICALS_ID = 73;
        /**
         * @stable ICU 2.4
         */
        public static final int HANGUL_SYLLABLES_ID = 74;
        /**
         * @stable ICU 2.4
         */
        public static final int HIGH_SURROGATES_ID = 75;
        /**
         * @stable ICU 2.4
         */
        public static final int HIGH_PRIVATE_USE_SURROGATES_ID = 76;
        /**
         * @stable ICU 2.4
         */
        public static final int LOW_SURROGATES_ID = 77;
        /**
         * Same as public static final int PRIVATE_USE.
         * Until Unicode 3.1.1; the corresponding block name was "Private Use";
         * and multiple code point ranges had this block.
         * Unicode 3.2 renames the block for the BMP PUA to "Private Use Area"
         * and adds separate blocks for the supplementary PUAs.
         * @stable ICU 2.4
         */
        public static final int PRIVATE_USE_AREA_ID = 78;
        /**
         * Same as public static final int PRIVATE_USE_AREA.
         * Until Unicode 3.1.1; the corresponding block name was "Private Use";
         * and multiple code point ranges had this block.
         * Unicode 3.2 renames the block for the BMP PUA to "Private Use Area"
         * and adds separate blocks for the supplementary PUAs.
         * @stable ICU 2.4
         */
        public static final int PRIVATE_USE_ID = PRIVATE_USE_AREA_ID;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_COMPATIBILITY_IDEOGRAPHS_ID = 79;
        /**
         * @stable ICU 2.4
         */
        public static final int ALPHABETIC_PRESENTATION_FORMS_ID = 80;
        /**
         * @stable ICU 2.4
         */
        public static final int ARABIC_PRESENTATION_FORMS_A_ID = 81;
        /**
         * @stable ICU 2.4
         */
        public static final int COMBINING_HALF_MARKS_ID = 82;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_COMPATIBILITY_FORMS_ID = 83;
        /**
         * @stable ICU 2.4
         */
        public static final int SMALL_FORM_VARIANTS_ID = 84;
        /**
         * @stable ICU 2.4
         */
        public static final int ARABIC_PRESENTATION_FORMS_B_ID = 85;
        /**
         * @stable ICU 2.4
         */
        public static final int SPECIALS_ID = 86;
        /**
         * @stable ICU 2.4
         */
        public static final int HALFWIDTH_AND_FULLWIDTH_FORMS_ID = 87;
        /**
         * @stable ICU 2.4
         */
        public static final int OLD_ITALIC_ID = 88;
        /**
         * @stable ICU 2.4
         */
        public static final int GOTHIC_ID = 89;
        /**
         * @stable ICU 2.4
         */
        public static final int DESERET_ID = 90;
        /**
         * @stable ICU 2.4
         */
        public static final int BYZANTINE_MUSICAL_SYMBOLS_ID = 91;
        /**
         * @stable ICU 2.4
         */
        public static final int MUSICAL_SYMBOLS_ID = 92;
        /**
         * @stable ICU 2.4
         */
        public static final int MATHEMATICAL_ALPHANUMERIC_SYMBOLS_ID = 93;
        /**
         * @stable ICU 2.4
         */
        public static final int CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B_ID = 94;
        /**
         * @stable ICU 2.4
         */
        public static final int
            CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT_ID = 95;
        /**
         * @stable ICU 2.4
         */
        public static final int TAGS_ID = 96;

        // New blocks in Unicode 3.2

        /**
         * Unicode 4.0.1 renames the "Cyrillic Supplementary" block to "Cyrillic Supplement".
         * @stable ICU 2.4
         */
        public static final int CYRILLIC_SUPPLEMENTARY_ID = 97;
        /**
         * Unicode 4.0.1 renames the "Cyrillic Supplementary" block to "Cyrillic Supplement".
         * @stable ICU 3.0
         */

        public static final int CYRILLIC_SUPPLEMENT_ID = 97;
        /**
         * @stable ICU 2.4
         */
        public static final int TAGALOG_ID = 98;
        /**
         * @stable ICU 2.4
         */
        public static final int HANUNOO_ID = 99;
        /**
         * @stable ICU 2.4
         */
        public static final int BUHID_ID = 100;
        /**
         * @stable ICU 2.4
         */
        public static final int TAGBANWA_ID = 101;
        /**
         * @stable ICU 2.4
         */
        public static final int MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A_ID = 102;
        /**
         * @stable ICU 2.4
         */
        public static final int SUPPLEMENTAL_ARROWS_A_ID = 103;
        /**
         * @stable ICU 2.4
         */
        public static final int SUPPLEMENTAL_ARROWS_B_ID = 104;
        /**
         * @stable ICU 2.4
         */
        public static final int MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B_ID = 105;
        /**
         * @stable ICU 2.4
         */
        public static final int SUPPLEMENTAL_MATHEMATICAL_OPERATORS_ID = 106;
        /**
         * @stable ICU 2.4
         */
        public static final int KATAKANA_PHONETIC_EXTENSIONS_ID = 107;
        /**
         * @stable ICU 2.4
         */
        public static final int VARIATION_SELECTORS_ID = 108;
        /**
         * @stable ICU 2.4
         */
        public static final int SUPPLEMENTARY_PRIVATE_USE_AREA_A_ID = 109;
        /**
         * @stable ICU 2.4
         */
        public static final int SUPPLEMENTARY_PRIVATE_USE_AREA_B_ID = 110;

        /**
         * @stable ICU 2.6
         */
        public static final int LIMBU_ID = 111; /*[1900]*/
        /**
         * @stable ICU 2.6
         */
        public static final int TAI_LE_ID = 112; /*[1950]*/
        /**
         * @stable ICU 2.6
         */
        public static final int KHMER_SYMBOLS_ID = 113; /*[19E0]*/
        /**
         * @stable ICU 2.6
         */
        public static final int PHONETIC_EXTENSIONS_ID = 114; /*[1D00]*/
        /**
         * @stable ICU 2.6
         */
        public static final int MISCELLANEOUS_SYMBOLS_AND_ARROWS_ID = 115; /*[2B00]*/
        /**
         * @stable ICU 2.6
         */
        public static final int YIJING_HEXAGRAM_SYMBOLS_ID = 116; /*[4DC0]*/
        /**
         * @stable ICU 2.6
         */
        public static final int LINEAR_B_SYLLABARY_ID = 117; /*[10000]*/
        /**
         * @stable ICU 2.6
         */
        public static final int LINEAR_B_IDEOGRAMS_ID = 118; /*[10080]*/
        /**
         * @stable ICU 2.6
         */
        public static final int AEGEAN_NUMBERS_ID = 119; /*[10100]*/
        /**
         * @stable ICU 2.6
         */
        public static final int UGARITIC_ID = 120; /*[10380]*/
        /**
         * @stable ICU 2.6
         */
        public static final int SHAVIAN_ID = 121; /*[10450]*/
        /**
         * @stable ICU 2.6
         */
        public static final int OSMANYA_ID = 122; /*[10480]*/
        /**
         * @stable ICU 2.6
         */
        public static final int CYPRIOT_SYLLABARY_ID = 123; /*[10800]*/
        /**
         * @stable ICU 2.6
         */
        public static final int TAI_XUAN_JING_SYMBOLS_ID = 124; /*[1D300]*/
        /**
         * @stable ICU 2.6
         */
        public static final int VARIATION_SELECTORS_SUPPLEMENT_ID = 125; /*[E0100]*/

        /* New blocks in Unicode 4.1 */

        /**
         * @stable ICU 3.4
         */
        public static final int ANCIENT_GREEK_MUSICAL_NOTATION_ID = 126; /*[1D200]*/

        /**
         * @stable ICU 3.4
         */
        public static final int ANCIENT_GREEK_NUMBERS_ID = 127; /*[10140]*/

        /**
         * @stable ICU 3.4
         */
        public static final int ARABIC_SUPPLEMENT_ID = 128; /*[0750]*/

        /**
         * @stable ICU 3.4
         */
        public static final int BUGINESE_ID = 129; /*[1A00]*/

        /**
         * @stable ICU 3.4
         */
        public static final int CJK_STROKES_ID = 130; /*[31C0]*/

        /**
         * @stable ICU 3.4
         */
        public static final int COMBINING_DIACRITICAL_MARKS_SUPPLEMENT_ID = 131; /*[1DC0]*/

        /**
         * @stable ICU 3.4
         */
        public static final int COPTIC_ID = 132; /*[2C80]*/

        /**
         * @stable ICU 3.4
         */
        public static final int ETHIOPIC_EXTENDED_ID = 133; /*[2D80]*/

        /**
         * @stable ICU 3.4
         */
        public static final int ETHIOPIC_SUPPLEMENT_ID = 134; /*[1380]*/

        /**
         * @stable ICU 3.4
         */
        public static final int GEORGIAN_SUPPLEMENT_ID = 135; /*[2D00]*/

        /**
         * @stable ICU 3.4
         */
        public static final int GLAGOLITIC_ID = 136; /*[2C00]*/

        /**
         * @stable ICU 3.4
         */
        public static final int KHAROSHTHI_ID = 137; /*[10A00]*/

        /**
         * @stable ICU 3.4
         */
        public static final int MODIFIER_TONE_LETTERS_ID = 138; /*[A700]*/

        /**
         * @stable ICU 3.4
         */
        public static final int NEW_TAI_LUE_ID = 139; /*[1980]*/

        /**
         * @stable ICU 3.4
         */
        public static final int OLD_PERSIAN_ID = 140; /*[103A0]*/

        /**
         * @stable ICU 3.4
         */
        public static final int PHONETIC_EXTENSIONS_SUPPLEMENT_ID = 141; /*[1D80]*/

        /**
         * @stable ICU 3.4
         */
        public static final int SUPPLEMENTAL_PUNCTUATION_ID = 142; /*[2E00]*/

        /**
         * @stable ICU 3.4
         */
        public static final int SYLOTI_NAGRI_ID = 143; /*[A800]*/

        /**
         * @stable ICU 3.4
         */
        public static final int TIFINAGH_ID = 144; /*[2D30]*/

        /**
         * @stable ICU 3.4
         */
        public static final int VERTICAL_FORMS_ID = 145; /*[FE10]*/

        /* New blocks in Unicode 5.0 */

        /**
         * @stable ICU 3.6
         */
        public static final int NKO_ID = 146; /*[07C0]*/
        /**
         * @stable ICU 3.6
         */
        public static final int BALINESE_ID = 147; /*[1B00]*/
        /**
         * @stable ICU 3.6
         */
        public static final int LATIN_EXTENDED_C_ID = 148; /*[2C60]*/
        /**
         * @stable ICU 3.6
         */
        public static final int LATIN_EXTENDED_D_ID = 149; /*[A720]*/
        /**
         * @stable ICU 3.6
         */
        public static final int PHAGS_PA_ID = 150; /*[A840]*/
        /**
         * @stable ICU 3.6
         */
        public static final int PHOENICIAN_ID = 151; /*[10900]*/
        /**
         * @stable ICU 3.6
         */
        public static final int CUNEIFORM_ID = 152; /*[12000]*/
        /**
         * @stable ICU 3.6
         */
        public static final int CUNEIFORM_NUMBERS_AND_PUNCTUATION_ID = 153; /*[12400]*/
        /**
         * @stable ICU 3.6
         */
        public static final int COUNTING_ROD_NUMERALS_ID = 154; /*[1D360]*/

        /**
         * @stable ICU 4.0
         */
        public static final int SUNDANESE_ID = 155; /* [1B80] */

        /**
         * @stable ICU 4.0
         */
        public static final int LEPCHA_ID = 156; /* [1C00] */

        /**
         * @stable ICU 4.0
         */
        public static final int OL_CHIKI_ID = 157; /* [1C50] */

        /**
         * @stable ICU 4.0
         */
        public static final int CYRILLIC_EXTENDED_A_ID = 158; /* [2DE0] */

        /**
         * @stable ICU 4.0
         */
        public static final int VAI_ID = 159; /* [A500] */

        /**
         * @stable ICU 4.0
         */
        public static final int CYRILLIC_EXTENDED_B_ID = 160; /* [A640] */

        /**
         * @stable ICU 4.0
         */
        public static final int SAURASHTRA_ID = 161; /* [A880] */

        /**
         * @stable ICU 4.0
         */
        public static final int KAYAH_LI_ID = 162; /* [A900] */

        /**
         * @stable ICU 4.0
         */
        public static final int REJANG_ID = 163; /* [A930] */

        /**
         * @stable ICU 4.0
         */
        public static final int CHAM_ID = 164; /* [AA00] */

        /**
         * @stable ICU 4.0
         */
        public static final int ANCIENT_SYMBOLS_ID = 165; /* [10190] */

        /**
         * @stable ICU 4.0
         */
        public static final int PHAISTOS_DISC_ID = 166; /* [101D0] */

        /**
         * @stable ICU 4.0
         */
        public static final int LYCIAN_ID = 167; /* [10280] */

        /**
         * @stable ICU 4.0
         */
        public static final int CARIAN_ID = 168; /* [102A0] */

        /**
         * @stable ICU 4.0
         */
        public static final int LYDIAN_ID = 169; /* [10920] */

        /**
         * @stable ICU 4.0
         */
        public static final int MAHJONG_TILES_ID = 170; /* [1F000] */

        /**
         * @stable ICU 4.0
         */
        public static final int DOMINO_TILES_ID = 171; /* [1F030] */

        /* New blocks in Unicode 5.2 */

        /** @stable ICU 4.4 */
        public static final int SAMARITAN_ID = 172; /*[0800]*/
        /** @stable ICU 4.4 */
        public static final int UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED_ID = 173; /*[18B0]*/
        /** @stable ICU 4.4 */
        public static final int TAI_THAM_ID = 174; /*[1A20]*/
        /** @stable ICU 4.4 */
        public static final int VEDIC_EXTENSIONS_ID = 175; /*[1CD0]*/
        /** @stable ICU 4.4 */
        public static final int LISU_ID = 176; /*[A4D0]*/
        /** @stable ICU 4.4 */
        public static final int BAMUM_ID = 177; /*[A6A0]*/
        /** @stable ICU 4.4 */
        public static final int COMMON_INDIC_NUMBER_FORMS_ID = 178; /*[A830]*/
        /** @stable ICU 4.4 */
        public static final int DEVANAGARI_EXTENDED_ID = 179; /*[A8E0]*/
        /** @stable ICU 4.4 */
        public static final int HANGUL_JAMO_EXTENDED_A_ID = 180; /*[A960]*/
        /** @stable ICU 4.4 */
        public static final int JAVANESE_ID = 181; /*[A980]*/
        /** @stable ICU 4.4 */
        public static final int MYANMAR_EXTENDED_A_ID = 182; /*[AA60]*/
        /** @stable ICU 4.4 */
        public static final int TAI_VIET_ID = 183; /*[AA80]*/
        /** @stable ICU 4.4 */
        public static final int MEETEI_MAYEK_ID = 184; /*[ABC0]*/
        /** @stable ICU 4.4 */
        public static final int HANGUL_JAMO_EXTENDED_B_ID = 185; /*[D7B0]*/
        /** @stable ICU 4.4 */
        public static final int IMPERIAL_ARAMAIC_ID = 186; /*[10840]*/
        /** @stable ICU 4.4 */
        public static final int OLD_SOUTH_ARABIAN_ID = 187; /*[10A60]*/
        /** @stable ICU 4.4 */
        public static final int AVESTAN_ID = 188; /*[10B00]*/
        /** @stable ICU 4.4 */
        public static final int INSCRIPTIONAL_PARTHIAN_ID = 189; /*[10B40]*/
        /** @stable ICU 4.4 */
        public static final int INSCRIPTIONAL_PAHLAVI_ID = 190; /*[10B60]*/
        /** @stable ICU 4.4 */
        public static final int OLD_TURKIC_ID = 191; /*[10C00]*/
        /** @stable ICU 4.4 */
        public static final int RUMI_NUMERAL_SYMBOLS_ID = 192; /*[10E60]*/
        /** @stable ICU 4.4 */
        public static final int KAITHI_ID = 193; /*[11080]*/
        /** @stable ICU 4.4 */
        public static final int EGYPTIAN_HIEROGLYPHS_ID = 194; /*[13000]*/
        /** @stable ICU 4.4 */
        public static final int ENCLOSED_ALPHANUMERIC_SUPPLEMENT_ID = 195; /*[1F100]*/
        /** @stable ICU 4.4 */
        public static final int ENCLOSED_IDEOGRAPHIC_SUPPLEMENT_ID = 196; /*[1F200]*/
        /** @stable ICU 4.4 */
        public static final int CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C_ID = 197; /*[2A700]*/

        /* New blocks in Unicode 6.0 */

        /** @stable ICU 4.6 */
        public static final int MANDAIC_ID = 198; /*[0840]*/
        /** @stable ICU 4.6 */
        public static final int BATAK_ID = 199; /*[1BC0]*/
        /** @stable ICU 4.6 */
        public static final int ETHIOPIC_EXTENDED_A_ID = 200; /*[AB00]*/
        /** @stable ICU 4.6 */
        public static final int BRAHMI_ID = 201; /*[11000]*/
        /** @stable ICU 4.6 */
        public static final int BAMUM_SUPPLEMENT_ID = 202; /*[16800]*/
        /** @stable ICU 4.6 */
        public static final int KANA_SUPPLEMENT_ID = 203; /*[1B000]*/
        /** @stable ICU 4.6 */
        public static final int PLAYING_CARDS_ID = 204; /*[1F0A0]*/
        /** @stable ICU 4.6 */
        public static final int MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS_ID = 205; /*[1F300]*/
        /** @stable ICU 4.6 */
        public static final int EMOTICONS_ID = 206; /*[1F600]*/
        /** @stable ICU 4.6 */
        public static final int TRANSPORT_AND_MAP_SYMBOLS_ID = 207; /*[1F680]*/
        /** @stable ICU 4.6 */
        public static final int ALCHEMICAL_SYMBOLS_ID = 208; /*[1F700]*/
        /** @stable ICU 4.6 */
        public static final int CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D_ID = 209; /*[2B740]*/

        /**
         * @stable ICU 2.4
         */
        public static final int COUNT = 210;

        // blocks objects ---------------------------------------------------

        /**
         * Array of UnicodeBlocks, for easy access in getInstance(int)
         */
        private final static UnicodeBlock BLOCKS_[] = new UnicodeBlock[COUNT];

        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock NO_BLOCK
            = new UnicodeBlock("NO_BLOCK", 0);

        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BASIC_LATIN
            = new UnicodeBlock("BASIC_LATIN", BASIC_LATIN_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock LATIN_1_SUPPLEMENT
            = new UnicodeBlock("LATIN_1_SUPPLEMENT", LATIN_1_SUPPLEMENT_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock LATIN_EXTENDED_A
            = new UnicodeBlock("LATIN_EXTENDED_A", LATIN_EXTENDED_A_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock LATIN_EXTENDED_B
            = new UnicodeBlock("LATIN_EXTENDED_B", LATIN_EXTENDED_B_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock IPA_EXTENSIONS
            = new UnicodeBlock("IPA_EXTENSIONS", IPA_EXTENSIONS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SPACING_MODIFIER_LETTERS
            = new UnicodeBlock("SPACING_MODIFIER_LETTERS", SPACING_MODIFIER_LETTERS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock COMBINING_DIACRITICAL_MARKS
            = new UnicodeBlock("COMBINING_DIACRITICAL_MARKS", COMBINING_DIACRITICAL_MARKS_ID);
        /**
         * Unicode 3.2 renames this block to "Greek and Coptic".
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GREEK
            = new UnicodeBlock("GREEK", GREEK_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CYRILLIC
            = new UnicodeBlock("CYRILLIC", CYRILLIC_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ARMENIAN
            = new UnicodeBlock("ARMENIAN", ARMENIAN_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HEBREW
            = new UnicodeBlock("HEBREW", HEBREW_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ARABIC
            = new UnicodeBlock("ARABIC", ARABIC_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SYRIAC
            = new UnicodeBlock("SYRIAC", SYRIAC_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock THAANA
            = new UnicodeBlock("THAANA", THAANA_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock DEVANAGARI
            = new UnicodeBlock("DEVANAGARI", DEVANAGARI_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BENGALI
            = new UnicodeBlock("BENGALI", BENGALI_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GURMUKHI
            = new UnicodeBlock("GURMUKHI", GURMUKHI_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GUJARATI
            = new UnicodeBlock("GUJARATI", GUJARATI_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ORIYA
            = new UnicodeBlock("ORIYA", ORIYA_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock TAMIL
            = new UnicodeBlock("TAMIL", TAMIL_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock TELUGU
            = new UnicodeBlock("TELUGU", TELUGU_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock KANNADA
            = new UnicodeBlock("KANNADA", KANNADA_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MALAYALAM
            = new UnicodeBlock("MALAYALAM", MALAYALAM_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SINHALA
            = new UnicodeBlock("SINHALA", SINHALA_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock THAI
            = new UnicodeBlock("THAI", THAI_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock LAO
            = new UnicodeBlock("LAO", LAO_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock TIBETAN
            = new UnicodeBlock("TIBETAN", TIBETAN_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MYANMAR
            = new UnicodeBlock("MYANMAR", MYANMAR_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GEORGIAN
            = new UnicodeBlock("GEORGIAN", GEORGIAN_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HANGUL_JAMO
            = new UnicodeBlock("HANGUL_JAMO", HANGUL_JAMO_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ETHIOPIC
            = new UnicodeBlock("ETHIOPIC", ETHIOPIC_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CHEROKEE
            = new UnicodeBlock("CHEROKEE", CHEROKEE_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS
            = new UnicodeBlock("UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS",
                               UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock OGHAM
            = new UnicodeBlock("OGHAM", OGHAM_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock RUNIC
            = new UnicodeBlock("RUNIC", RUNIC_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock KHMER
            = new UnicodeBlock("KHMER", KHMER_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MONGOLIAN
            = new UnicodeBlock("MONGOLIAN", MONGOLIAN_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock LATIN_EXTENDED_ADDITIONAL
            = new UnicodeBlock("LATIN_EXTENDED_ADDITIONAL", LATIN_EXTENDED_ADDITIONAL_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GREEK_EXTENDED
            = new UnicodeBlock("GREEK_EXTENDED", GREEK_EXTENDED_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GENERAL_PUNCTUATION
            = new UnicodeBlock("GENERAL_PUNCTUATION", GENERAL_PUNCTUATION_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SUPERSCRIPTS_AND_SUBSCRIPTS
            = new UnicodeBlock("SUPERSCRIPTS_AND_SUBSCRIPTS", SUPERSCRIPTS_AND_SUBSCRIPTS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CURRENCY_SYMBOLS
            = new UnicodeBlock("CURRENCY_SYMBOLS", CURRENCY_SYMBOLS_ID);
        /**
         * Unicode 3.2 renames this block to "Combining Diacritical Marks for
         * Symbols".
         * @stable ICU 2.4
         */
        public static final UnicodeBlock COMBINING_MARKS_FOR_SYMBOLS
            = new UnicodeBlock("COMBINING_MARKS_FOR_SYMBOLS", COMBINING_MARKS_FOR_SYMBOLS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock LETTERLIKE_SYMBOLS
            = new UnicodeBlock("LETTERLIKE_SYMBOLS", LETTERLIKE_SYMBOLS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock NUMBER_FORMS
            = new UnicodeBlock("NUMBER_FORMS", NUMBER_FORMS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ARROWS
            = new UnicodeBlock("ARROWS", ARROWS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MATHEMATICAL_OPERATORS
            = new UnicodeBlock("MATHEMATICAL_OPERATORS", MATHEMATICAL_OPERATORS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MISCELLANEOUS_TECHNICAL
            = new UnicodeBlock("MISCELLANEOUS_TECHNICAL", MISCELLANEOUS_TECHNICAL_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CONTROL_PICTURES
            = new UnicodeBlock("CONTROL_PICTURES", CONTROL_PICTURES_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock OPTICAL_CHARACTER_RECOGNITION
            = new UnicodeBlock("OPTICAL_CHARACTER_RECOGNITION", OPTICAL_CHARACTER_RECOGNITION_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ENCLOSED_ALPHANUMERICS
            = new UnicodeBlock("ENCLOSED_ALPHANUMERICS", ENCLOSED_ALPHANUMERICS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BOX_DRAWING
            = new UnicodeBlock("BOX_DRAWING", BOX_DRAWING_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BLOCK_ELEMENTS
            = new UnicodeBlock("BLOCK_ELEMENTS", BLOCK_ELEMENTS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GEOMETRIC_SHAPES
            = new UnicodeBlock("GEOMETRIC_SHAPES", GEOMETRIC_SHAPES_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MISCELLANEOUS_SYMBOLS
            = new UnicodeBlock("MISCELLANEOUS_SYMBOLS", MISCELLANEOUS_SYMBOLS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock DINGBATS
            = new UnicodeBlock("DINGBATS", DINGBATS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BRAILLE_PATTERNS
            = new UnicodeBlock("BRAILLE_PATTERNS", BRAILLE_PATTERNS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_RADICALS_SUPPLEMENT
            = new UnicodeBlock("CJK_RADICALS_SUPPLEMENT", CJK_RADICALS_SUPPLEMENT_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock KANGXI_RADICALS
            = new UnicodeBlock("KANGXI_RADICALS", KANGXI_RADICALS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock IDEOGRAPHIC_DESCRIPTION_CHARACTERS
            = new UnicodeBlock("IDEOGRAPHIC_DESCRIPTION_CHARACTERS",
                               IDEOGRAPHIC_DESCRIPTION_CHARACTERS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_SYMBOLS_AND_PUNCTUATION
            = new UnicodeBlock("CJK_SYMBOLS_AND_PUNCTUATION", CJK_SYMBOLS_AND_PUNCTUATION_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HIRAGANA
            = new UnicodeBlock("HIRAGANA", HIRAGANA_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock KATAKANA
            = new UnicodeBlock("KATAKANA", KATAKANA_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BOPOMOFO
            = new UnicodeBlock("BOPOMOFO", BOPOMOFO_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HANGUL_COMPATIBILITY_JAMO
            = new UnicodeBlock("HANGUL_COMPATIBILITY_JAMO", HANGUL_COMPATIBILITY_JAMO_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock KANBUN
            = new UnicodeBlock("KANBUN", KANBUN_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BOPOMOFO_EXTENDED
            = new UnicodeBlock("BOPOMOFO_EXTENDED", BOPOMOFO_EXTENDED_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ENCLOSED_CJK_LETTERS_AND_MONTHS
            = new UnicodeBlock("ENCLOSED_CJK_LETTERS_AND_MONTHS",
                               ENCLOSED_CJK_LETTERS_AND_MONTHS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_COMPATIBILITY
            = new UnicodeBlock("CJK_COMPATIBILITY", CJK_COMPATIBILITY_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A
            = new UnicodeBlock("CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A",
                               CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_UNIFIED_IDEOGRAPHS
            = new UnicodeBlock("CJK_UNIFIED_IDEOGRAPHS", CJK_UNIFIED_IDEOGRAPHS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock YI_SYLLABLES
            = new UnicodeBlock("YI_SYLLABLES", YI_SYLLABLES_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock YI_RADICALS
            = new UnicodeBlock("YI_RADICALS", YI_RADICALS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HANGUL_SYLLABLES
            = new UnicodeBlock("HANGUL_SYLLABLES", HANGUL_SYLLABLES_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HIGH_SURROGATES
            = new UnicodeBlock("HIGH_SURROGATES", HIGH_SURROGATES_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HIGH_PRIVATE_USE_SURROGATES
            = new UnicodeBlock("HIGH_PRIVATE_USE_SURROGATES", HIGH_PRIVATE_USE_SURROGATES_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock LOW_SURROGATES
            = new UnicodeBlock("LOW_SURROGATES", LOW_SURROGATES_ID);
        /**
         * Same as public static final int PRIVATE_USE.
         * Until Unicode 3.1.1; the corresponding block name was "Private Use";
         * and multiple code point ranges had this block.
         * Unicode 3.2 renames the block for the BMP PUA to "Private Use Area"
         * and adds separate blocks for the supplementary PUAs.
         * @stable ICU 2.4
         */
        public static final UnicodeBlock PRIVATE_USE_AREA
            = new UnicodeBlock("PRIVATE_USE_AREA",  78);
        /**
         * Same as public static final int PRIVATE_USE_AREA.
         * Until Unicode 3.1.1; the corresponding block name was "Private Use";
         * and multiple code point ranges had this block.
         * Unicode 3.2 renames the block for the BMP PUA to "Private Use Area"
         * and adds separate blocks for the supplementary PUAs.
         * @stable ICU 2.4
         */
        public static final UnicodeBlock PRIVATE_USE
            = PRIVATE_USE_AREA;
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_COMPATIBILITY_IDEOGRAPHS
            = new UnicodeBlock("CJK_COMPATIBILITY_IDEOGRAPHS", CJK_COMPATIBILITY_IDEOGRAPHS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ALPHABETIC_PRESENTATION_FORMS
            = new UnicodeBlock("ALPHABETIC_PRESENTATION_FORMS", ALPHABETIC_PRESENTATION_FORMS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ARABIC_PRESENTATION_FORMS_A
            = new UnicodeBlock("ARABIC_PRESENTATION_FORMS_A", ARABIC_PRESENTATION_FORMS_A_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock COMBINING_HALF_MARKS
            = new UnicodeBlock("COMBINING_HALF_MARKS", COMBINING_HALF_MARKS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_COMPATIBILITY_FORMS
            = new UnicodeBlock("CJK_COMPATIBILITY_FORMS", CJK_COMPATIBILITY_FORMS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SMALL_FORM_VARIANTS
            = new UnicodeBlock("SMALL_FORM_VARIANTS", SMALL_FORM_VARIANTS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock ARABIC_PRESENTATION_FORMS_B
            = new UnicodeBlock("ARABIC_PRESENTATION_FORMS_B", ARABIC_PRESENTATION_FORMS_B_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SPECIALS
            = new UnicodeBlock("SPECIALS", SPECIALS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HALFWIDTH_AND_FULLWIDTH_FORMS
            = new UnicodeBlock("HALFWIDTH_AND_FULLWIDTH_FORMS", HALFWIDTH_AND_FULLWIDTH_FORMS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock OLD_ITALIC
            = new UnicodeBlock("OLD_ITALIC", OLD_ITALIC_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock GOTHIC
            = new UnicodeBlock("GOTHIC", GOTHIC_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock DESERET
            = new UnicodeBlock("DESERET", DESERET_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BYZANTINE_MUSICAL_SYMBOLS
            = new UnicodeBlock("BYZANTINE_MUSICAL_SYMBOLS", BYZANTINE_MUSICAL_SYMBOLS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MUSICAL_SYMBOLS
            = new UnicodeBlock("MUSICAL_SYMBOLS", MUSICAL_SYMBOLS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MATHEMATICAL_ALPHANUMERIC_SYMBOLS
            = new UnicodeBlock("MATHEMATICAL_ALPHANUMERIC_SYMBOLS",
                               MATHEMATICAL_ALPHANUMERIC_SYMBOLS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B
            = new UnicodeBlock("CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B",
                               CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock
            CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT
            = new UnicodeBlock("CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT",
                               CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock TAGS
            = new UnicodeBlock("TAGS", TAGS_ID);

        // New blocks in Unicode 3.2

        /**
         * Unicode 4.0.1 renames the "Cyrillic Supplementary" block to "Cyrillic Supplement".
         * @stable ICU 2.4
         */
        public static final UnicodeBlock CYRILLIC_SUPPLEMENTARY
            = new UnicodeBlock("CYRILLIC_SUPPLEMENTARY", CYRILLIC_SUPPLEMENTARY_ID);
        /**
         * Unicode 4.0.1 renames the "Cyrillic Supplementary" block to "Cyrillic Supplement".
         * @stable ICU 3.0
         */
        public static final UnicodeBlock CYRILLIC_SUPPLEMENT
            = new UnicodeBlock("CYRILLIC_SUPPLEMENT", CYRILLIC_SUPPLEMENT_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock TAGALOG
            = new UnicodeBlock("TAGALOG", TAGALOG_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock HANUNOO
            = new UnicodeBlock("HANUNOO", HANUNOO_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock BUHID
            = new UnicodeBlock("BUHID", BUHID_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock TAGBANWA
            = new UnicodeBlock("TAGBANWA", TAGBANWA_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A
            = new UnicodeBlock("MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A",
                               MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SUPPLEMENTAL_ARROWS_A
            = new UnicodeBlock("SUPPLEMENTAL_ARROWS_A", SUPPLEMENTAL_ARROWS_A_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SUPPLEMENTAL_ARROWS_B
            = new UnicodeBlock("SUPPLEMENTAL_ARROWS_B", SUPPLEMENTAL_ARROWS_B_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B
            = new UnicodeBlock("MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B",
                               MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SUPPLEMENTAL_MATHEMATICAL_OPERATORS
            = new UnicodeBlock("SUPPLEMENTAL_MATHEMATICAL_OPERATORS",
                               SUPPLEMENTAL_MATHEMATICAL_OPERATORS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock KATAKANA_PHONETIC_EXTENSIONS
            = new UnicodeBlock("KATAKANA_PHONETIC_EXTENSIONS", KATAKANA_PHONETIC_EXTENSIONS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock VARIATION_SELECTORS
            = new UnicodeBlock("VARIATION_SELECTORS", VARIATION_SELECTORS_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SUPPLEMENTARY_PRIVATE_USE_AREA_A
            = new UnicodeBlock("SUPPLEMENTARY_PRIVATE_USE_AREA_A",
                               SUPPLEMENTARY_PRIVATE_USE_AREA_A_ID);
        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock SUPPLEMENTARY_PRIVATE_USE_AREA_B
            = new UnicodeBlock("SUPPLEMENTARY_PRIVATE_USE_AREA_B",
                               SUPPLEMENTARY_PRIVATE_USE_AREA_B_ID);

        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock LIMBU
            = new UnicodeBlock("LIMBU", LIMBU_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock TAI_LE
            = new UnicodeBlock("TAI_LE", TAI_LE_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock KHMER_SYMBOLS
            = new UnicodeBlock("KHMER_SYMBOLS", KHMER_SYMBOLS_ID);

        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock PHONETIC_EXTENSIONS
            = new UnicodeBlock("PHONETIC_EXTENSIONS", PHONETIC_EXTENSIONS_ID);

        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock MISCELLANEOUS_SYMBOLS_AND_ARROWS
            = new UnicodeBlock("MISCELLANEOUS_SYMBOLS_AND_ARROWS",
                               MISCELLANEOUS_SYMBOLS_AND_ARROWS_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock YIJING_HEXAGRAM_SYMBOLS
            = new UnicodeBlock("YIJING_HEXAGRAM_SYMBOLS", YIJING_HEXAGRAM_SYMBOLS_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock LINEAR_B_SYLLABARY
            = new UnicodeBlock("LINEAR_B_SYLLABARY", LINEAR_B_SYLLABARY_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock LINEAR_B_IDEOGRAMS
            = new UnicodeBlock("LINEAR_B_IDEOGRAMS", LINEAR_B_IDEOGRAMS_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock AEGEAN_NUMBERS
            = new UnicodeBlock("AEGEAN_NUMBERS", AEGEAN_NUMBERS_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock UGARITIC
            = new UnicodeBlock("UGARITIC", UGARITIC_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock SHAVIAN
            = new UnicodeBlock("SHAVIAN", SHAVIAN_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock OSMANYA
            = new UnicodeBlock("OSMANYA", OSMANYA_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock CYPRIOT_SYLLABARY
            = new UnicodeBlock("CYPRIOT_SYLLABARY", CYPRIOT_SYLLABARY_ID);
        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock TAI_XUAN_JING_SYMBOLS
            = new UnicodeBlock("TAI_XUAN_JING_SYMBOLS", TAI_XUAN_JING_SYMBOLS_ID);

        /**
         * @stable ICU 2.6
         */
        public static final UnicodeBlock VARIATION_SELECTORS_SUPPLEMENT
            = new UnicodeBlock("VARIATION_SELECTORS_SUPPLEMENT", VARIATION_SELECTORS_SUPPLEMENT_ID);

        /* New blocks in Unicode 4.1 */

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock ANCIENT_GREEK_MUSICAL_NOTATION =
            new UnicodeBlock("ANCIENT_GREEK_MUSICAL_NOTATION",
                             ANCIENT_GREEK_MUSICAL_NOTATION_ID); /*[1D200]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock ANCIENT_GREEK_NUMBERS =
            new UnicodeBlock("ANCIENT_GREEK_NUMBERS", ANCIENT_GREEK_NUMBERS_ID); /*[10140]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock ARABIC_SUPPLEMENT =
            new UnicodeBlock("ARABIC_SUPPLEMENT", ARABIC_SUPPLEMENT_ID); /*[0750]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock BUGINESE =
            new UnicodeBlock("BUGINESE", BUGINESE_ID); /*[1A00]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock CJK_STROKES =
            new UnicodeBlock("CJK_STROKES", CJK_STROKES_ID); /*[31C0]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock COMBINING_DIACRITICAL_MARKS_SUPPLEMENT =
            new UnicodeBlock("COMBINING_DIACRITICAL_MARKS_SUPPLEMENT",
                             COMBINING_DIACRITICAL_MARKS_SUPPLEMENT_ID); /*[1DC0]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock COPTIC = new UnicodeBlock("COPTIC", COPTIC_ID); /*[2C80]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock ETHIOPIC_EXTENDED =
            new UnicodeBlock("ETHIOPIC_EXTENDED", ETHIOPIC_EXTENDED_ID); /*[2D80]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock ETHIOPIC_SUPPLEMENT =
            new UnicodeBlock("ETHIOPIC_SUPPLEMENT", ETHIOPIC_SUPPLEMENT_ID); /*[1380]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock GEORGIAN_SUPPLEMENT =
            new UnicodeBlock("GEORGIAN_SUPPLEMENT", GEORGIAN_SUPPLEMENT_ID); /*[2D00]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock GLAGOLITIC =
            new UnicodeBlock("GLAGOLITIC", GLAGOLITIC_ID); /*[2C00]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock KHAROSHTHI =
            new UnicodeBlock("KHAROSHTHI", KHAROSHTHI_ID); /*[10A00]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock MODIFIER_TONE_LETTERS =
            new UnicodeBlock("MODIFIER_TONE_LETTERS", MODIFIER_TONE_LETTERS_ID); /*[A700]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock NEW_TAI_LUE =
            new UnicodeBlock("NEW_TAI_LUE", NEW_TAI_LUE_ID); /*[1980]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock OLD_PERSIAN =
            new UnicodeBlock("OLD_PERSIAN", OLD_PERSIAN_ID); /*[103A0]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock PHONETIC_EXTENSIONS_SUPPLEMENT =
            new UnicodeBlock("PHONETIC_EXTENSIONS_SUPPLEMENT",
                             PHONETIC_EXTENSIONS_SUPPLEMENT_ID); /*[1D80]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock SUPPLEMENTAL_PUNCTUATION =
            new UnicodeBlock("SUPPLEMENTAL_PUNCTUATION", SUPPLEMENTAL_PUNCTUATION_ID); /*[2E00]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock SYLOTI_NAGRI =
            new UnicodeBlock("SYLOTI_NAGRI", SYLOTI_NAGRI_ID); /*[A800]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock TIFINAGH =
            new UnicodeBlock("TIFINAGH", TIFINAGH_ID); /*[2D30]*/

        /**
         * @stable ICU 3.4
         */
        public static final UnicodeBlock VERTICAL_FORMS =
            new UnicodeBlock("VERTICAL_FORMS", VERTICAL_FORMS_ID); /*[FE10]*/

        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock NKO = new UnicodeBlock("NKO", NKO_ID); /*[07C0]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock BALINESE =
            new UnicodeBlock("BALINESE", BALINESE_ID); /*[1B00]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock LATIN_EXTENDED_C =
            new UnicodeBlock("LATIN_EXTENDED_C", LATIN_EXTENDED_C_ID); /*[2C60]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock LATIN_EXTENDED_D =
            new UnicodeBlock("LATIN_EXTENDED_D", LATIN_EXTENDED_D_ID); /*[A720]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock PHAGS_PA =
            new UnicodeBlock("PHAGS_PA", PHAGS_PA_ID); /*[A840]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock PHOENICIAN =
            new UnicodeBlock("PHOENICIAN", PHOENICIAN_ID); /*[10900]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock CUNEIFORM =
            new UnicodeBlock("CUNEIFORM", CUNEIFORM_ID); /*[12000]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock CUNEIFORM_NUMBERS_AND_PUNCTUATION =
            new UnicodeBlock("CUNEIFORM_NUMBERS_AND_PUNCTUATION",
                             CUNEIFORM_NUMBERS_AND_PUNCTUATION_ID); /*[12400]*/
        /**
         * @stable ICU 3.6
         */
        public static final UnicodeBlock COUNTING_ROD_NUMERALS =
            new UnicodeBlock("COUNTING_ROD_NUMERALS", COUNTING_ROD_NUMERALS_ID); /*[1D360]*/

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock SUNDANESE =
            new UnicodeBlock("SUNDANESE", SUNDANESE_ID); /* [1B80] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock LEPCHA =
            new UnicodeBlock("LEPCHA", LEPCHA_ID); /* [1C00] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock OL_CHIKI =
            new UnicodeBlock("OL_CHIKI", OL_CHIKI_ID); /* [1C50] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock CYRILLIC_EXTENDED_A =
            new UnicodeBlock("CYRILLIC_EXTENDED_A", CYRILLIC_EXTENDED_A_ID); /* [2DE0] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock VAI = new UnicodeBlock("VAI", VAI_ID); /* [A500] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock CYRILLIC_EXTENDED_B =
            new UnicodeBlock("CYRILLIC_EXTENDED_B", CYRILLIC_EXTENDED_B_ID); /* [A640] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock SAURASHTRA =
            new UnicodeBlock("SAURASHTRA", SAURASHTRA_ID); /* [A880] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock KAYAH_LI =
            new UnicodeBlock("KAYAH_LI", KAYAH_LI_ID); /* [A900] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock REJANG =
            new UnicodeBlock("REJANG", REJANG_ID); /* [A930] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock CHAM =
            new UnicodeBlock("CHAM", CHAM_ID); /* [AA00] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock ANCIENT_SYMBOLS =
            new UnicodeBlock("ANCIENT_SYMBOLS", ANCIENT_SYMBOLS_ID); /* [10190] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock PHAISTOS_DISC =
            new UnicodeBlock("PHAISTOS_DISC", PHAISTOS_DISC_ID); /* [101D0] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock LYCIAN =
            new UnicodeBlock("LYCIAN", LYCIAN_ID); /* [10280] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock CARIAN =
            new UnicodeBlock("CARIAN", CARIAN_ID); /* [102A0] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock LYDIAN =
            new UnicodeBlock("LYDIAN", LYDIAN_ID); /* [10920] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock MAHJONG_TILES =
            new UnicodeBlock("MAHJONG_TILES", MAHJONG_TILES_ID); /* [1F000] */

        /**
         * @stable ICU 4.0
         */
        public static final UnicodeBlock DOMINO_TILES =
            new UnicodeBlock("DOMINO_TILES", DOMINO_TILES_ID); /* [1F030] */

        /* New blocks in Unicode 5.2 */

        /** @stable ICU 4.4 */
        public static final UnicodeBlock SAMARITAN =
            new UnicodeBlock("SAMARITAN", SAMARITAN_ID); /*[0800]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED =
            new UnicodeBlock("UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED",
                             UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED_ID); /*[18B0]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock TAI_THAM =
            new UnicodeBlock("TAI_THAM", TAI_THAM_ID); /*[1A20]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock VEDIC_EXTENSIONS =
            new UnicodeBlock("VEDIC_EXTENSIONS", VEDIC_EXTENSIONS_ID); /*[1CD0]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock LISU =
            new UnicodeBlock("LISU", LISU_ID); /*[A4D0]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock BAMUM =
            new UnicodeBlock("BAMUM", BAMUM_ID); /*[A6A0]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock COMMON_INDIC_NUMBER_FORMS =
            new UnicodeBlock("COMMON_INDIC_NUMBER_FORMS", COMMON_INDIC_NUMBER_FORMS_ID); /*[A830]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock DEVANAGARI_EXTENDED =
            new UnicodeBlock("DEVANAGARI_EXTENDED", DEVANAGARI_EXTENDED_ID); /*[A8E0]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock HANGUL_JAMO_EXTENDED_A =
            new UnicodeBlock("HANGUL_JAMO_EXTENDED_A", HANGUL_JAMO_EXTENDED_A_ID); /*[A960]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock JAVANESE =
            new UnicodeBlock("JAVANESE", JAVANESE_ID); /*[A980]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock MYANMAR_EXTENDED_A =
            new UnicodeBlock("MYANMAR_EXTENDED_A", MYANMAR_EXTENDED_A_ID); /*[AA60]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock TAI_VIET =
            new UnicodeBlock("TAI_VIET", TAI_VIET_ID); /*[AA80]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock MEETEI_MAYEK =
            new UnicodeBlock("MEETEI_MAYEK", MEETEI_MAYEK_ID); /*[ABC0]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock HANGUL_JAMO_EXTENDED_B =
            new UnicodeBlock("HANGUL_JAMO_EXTENDED_B", HANGUL_JAMO_EXTENDED_B_ID); /*[D7B0]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock IMPERIAL_ARAMAIC =
            new UnicodeBlock("IMPERIAL_ARAMAIC", IMPERIAL_ARAMAIC_ID); /*[10840]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock OLD_SOUTH_ARABIAN =
            new UnicodeBlock("OLD_SOUTH_ARABIAN", OLD_SOUTH_ARABIAN_ID); /*[10A60]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock AVESTAN =
            new UnicodeBlock("AVESTAN", AVESTAN_ID); /*[10B00]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock INSCRIPTIONAL_PARTHIAN =
            new UnicodeBlock("INSCRIPTIONAL_PARTHIAN", INSCRIPTIONAL_PARTHIAN_ID); /*[10B40]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock INSCRIPTIONAL_PAHLAVI =
            new UnicodeBlock("INSCRIPTIONAL_PAHLAVI", INSCRIPTIONAL_PAHLAVI_ID); /*[10B60]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock OLD_TURKIC =
            new UnicodeBlock("OLD_TURKIC", OLD_TURKIC_ID); /*[10C00]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock RUMI_NUMERAL_SYMBOLS =
            new UnicodeBlock("RUMI_NUMERAL_SYMBOLS", RUMI_NUMERAL_SYMBOLS_ID); /*[10E60]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock KAITHI =
            new UnicodeBlock("KAITHI", KAITHI_ID); /*[11080]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock EGYPTIAN_HIEROGLYPHS =
            new UnicodeBlock("EGYPTIAN_HIEROGLYPHS", EGYPTIAN_HIEROGLYPHS_ID); /*[13000]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock ENCLOSED_ALPHANUMERIC_SUPPLEMENT =
            new UnicodeBlock("ENCLOSED_ALPHANUMERIC_SUPPLEMENT",
                             ENCLOSED_ALPHANUMERIC_SUPPLEMENT_ID); /*[1F100]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock ENCLOSED_IDEOGRAPHIC_SUPPLEMENT =
            new UnicodeBlock("ENCLOSED_IDEOGRAPHIC_SUPPLEMENT",
                             ENCLOSED_IDEOGRAPHIC_SUPPLEMENT_ID); /*[1F200]*/
        /** @stable ICU 4.4 */
        public static final UnicodeBlock CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C =
            new UnicodeBlock("CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C",
                             CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C_ID); /*[2A700]*/

        /* New blocks in Unicode 6.0 */

        /** @stable ICU 4.6 */
        public static final UnicodeBlock MANDAIC =
            new UnicodeBlock("MANDAIC", MANDAIC_ID); /*[0840]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock BATAK =
            new UnicodeBlock("BATAK", BATAK_ID); /*[1BC0]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock ETHIOPIC_EXTENDED_A =
            new UnicodeBlock("ETHIOPIC_EXTENDED_A", ETHIOPIC_EXTENDED_A_ID); /*[AB00]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock BRAHMI =
            new UnicodeBlock("BRAHMI", BRAHMI_ID); /*[11000]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock BAMUM_SUPPLEMENT =
            new UnicodeBlock("BAMUM_SUPPLEMENT", BAMUM_SUPPLEMENT_ID); /*[16800]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock KANA_SUPPLEMENT =
            new UnicodeBlock("KANA_SUPPLEMENT", KANA_SUPPLEMENT_ID); /*[1B000]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock PLAYING_CARDS =
            new UnicodeBlock("PLAYING_CARDS", PLAYING_CARDS_ID); /*[1F0A0]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS =
            new UnicodeBlock("MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS",
                             MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS_ID); /*[1F300]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock EMOTICONS =
            new UnicodeBlock("EMOTICONS", EMOTICONS_ID); /*[1F600]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock TRANSPORT_AND_MAP_SYMBOLS =
            new UnicodeBlock("TRANSPORT_AND_MAP_SYMBOLS", TRANSPORT_AND_MAP_SYMBOLS_ID); /*[1F680]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock ALCHEMICAL_SYMBOLS =
            new UnicodeBlock("ALCHEMICAL_SYMBOLS", ALCHEMICAL_SYMBOLS_ID); /*[1F700]*/
        /** @stable ICU 4.6 */
        public static final UnicodeBlock CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D =
            new UnicodeBlock("CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D",
                             CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D_ID); /*[2B740]*/

        /**
         * @stable ICU 2.4
         */
        public static final UnicodeBlock INVALID_CODE
            = new UnicodeBlock("INVALID_CODE", INVALID_CODE_ID);

        static {
            for (int blockId = 0; blockId < COUNT; ++blockId) {
                if (BLOCKS_[blockId] == null) {
                    throw new java.lang.IllegalStateException(
                        "UnicodeBlock.BLOCKS_[" + blockId + "] not initialized");
                }
            }
        }

        // public methods --------------------------------------------------

        /**
         * {@icu} Returns the only instance of the UnicodeBlock with the argument ID.
         * If no such ID exists, a INVALID_CODE UnicodeBlock will be returned.
         * @param id UnicodeBlock ID
         * @return the only instance of the UnicodeBlock with the argument ID
         *         if it exists, otherwise a INVALID_CODE UnicodeBlock will be
         *         returned.
         * @stable ICU 2.4
         */
        public static UnicodeBlock getInstance(int id)
        {
            if (id >= 0 && id < BLOCKS_.length) {
                return BLOCKS_[id];
            }
            return INVALID_CODE;
        }

        /**
         * Returns the Unicode allocation block that contains the code point,
         * or null if the code point is not a member of a defined block.
         * @param ch code point to be tested
         * @return the Unicode allocation block that contains the code point
         * @stable ICU 2.4
         */
        public static UnicodeBlock of(int ch)
        {
            if (ch > MAX_VALUE) {
                return INVALID_CODE;
            }

            return UnicodeBlock.getInstance(
                UCharacterProperty.INSTANCE.getIntPropertyValue(ch, UProperty.BLOCK));
        }

        /**
         * Cover the JDK 1.5 API.  Return the Unicode block with the
         * given name. {@icunote} Unlike JDK 1.5, this only matches
         * against the official UCD name and the Java block name
         * (ignoring case).
         * @param blockName the name of the block to match
         * @return the UnicodeBlock with that name
         * @throws IllegalArgumentException if the blockName could not be matched
         * @stable ICU 3.0
         */
        public static final UnicodeBlock forName(String blockName) {
            Map<String, UnicodeBlock> m = null;
            if (mref != null) {
                m = mref.get();
            }
            if (m == null) {
                m = new HashMap<String, UnicodeBlock>(BLOCKS_.length);
                for (int i = 0; i < BLOCKS_.length; ++i) {
                    UnicodeBlock b = BLOCKS_[i];
                    String name = trimBlockName(
                        getPropertyValueName(UProperty.BLOCK, b.getID(),
                                             UProperty.NameChoice.LONG));
                    m.put(name, b);
                }
                mref = new SoftReference<Map<String, UnicodeBlock>>(m);
            }
            UnicodeBlock b = m.get(trimBlockName(blockName));
            if (b == null) {
                throw new IllegalArgumentException();
            }
            return b;
        }
        private static SoftReference<Map<String, UnicodeBlock>> mref;

        private static String trimBlockName(String name) {
            String upper = name.toUpperCase();
            StringBuilder result = new StringBuilder(upper.length());
            for (int i = 0; i < upper.length(); i++) {
                char c = upper.charAt(i);
                if (c != ' ' && c != '_' && c != '-') {
                    result.append(c);
                }
            }
            return result.toString();
        }

        /**
         * {icu} Returns the type ID of this Unicode block
         * @return integer type ID of this Unicode block
         * @stable ICU 2.4
         */
        public int getID()
        {
            return m_id_;
        }

        // private data members ---------------------------------------------

        /**
         * Identification code for this UnicodeBlock
         */
        private int m_id_;

        // private constructor ----------------------------------------------

        /**
         * UnicodeBlock constructor
         * @param name name of this UnicodeBlock
         * @param id unique id of this UnicodeBlock
         * @exception NullPointerException if name is <code>null</code>
         */
        private UnicodeBlock(String name, int id)
        {
            super(name);
            m_id_ = id;
            if (id >= 0) {
                BLOCKS_[id] = this;
            }
        }
    }

    /**
     * East Asian Width constants.
     * @see UProperty#EAST_ASIAN_WIDTH
     * @see UCharacter#getIntPropertyValue
     * @stable ICU 2.4
     */
    public static interface EastAsianWidth
    {
        /**
         * @stable ICU 2.4
         */
        public static final int NEUTRAL = 0;
        /**
         * @stable ICU 2.4
         */
        public static final int AMBIGUOUS = 1;
        /**
         * @stable ICU 2.4
         */
        public static final int HALFWIDTH = 2;
        /**
         * @stable ICU 2.4
         */
        public static final int FULLWIDTH = 3;
        /**
         * @stable ICU 2.4
         */
        public static final int NARROW = 4;
        /**
         * @stable ICU 2.4
         */
        public static final int WIDE = 5;
        /**
         * @stable ICU 2.4
         */
        public static final int COUNT = 6;
    }

    /**
     * Decomposition Type constants.
     * @see UProperty#DECOMPOSITION_TYPE
     * @stable ICU 2.4
     */
    public static interface DecompositionType
    {
        /**
         * @stable ICU 2.4
         */
        public static final int NONE = 0;
        /**
         * @stable ICU 2.4
         */
        public static final int CANONICAL = 1;
        /**
         * @stable ICU 2.4
         */
        public static final int COMPAT = 2;
        /**
         * @stable ICU 2.4
         */
        public static final int CIRCLE = 3;
        /**
         * @stable ICU 2.4
         */
        public static final int FINAL = 4;
        /**
         * @stable ICU 2.4
         */
        public static final int FONT = 5;
        /**
         * @stable ICU 2.4
         */
        public static final int FRACTION = 6;
        /**
         * @stable ICU 2.4
         */
        public static final int INITIAL = 7;
        /**
         * @stable ICU 2.4
         */
        public static final int ISOLATED = 8;
        /**
         * @stable ICU 2.4
         */
        public static final int MEDIAL = 9;
        /**
         * @stable ICU 2.4
         */
        public static final int NARROW = 10;
        /**
         * @stable ICU 2.4
         */
        public static final int NOBREAK = 11;
        /**
         * @stable ICU 2.4
         */
        public static final int SMALL = 12;
        /**
         * @stable ICU 2.4
         */
        public static final int SQUARE = 13;
        /**
         * @stable ICU 2.4
         */
        public static final int SUB = 14;
        /**
         * @stable ICU 2.4
         */
        public static final int SUPER = 15;
        /**
         * @stable ICU 2.4
         */
        public static final int VERTICAL = 16;
        /**
         * @stable ICU 2.4
         */
        public static final int WIDE = 17;
        /**
         * @stable ICU 2.4
         */
        public static final int COUNT = 18;
    }

    /**
     * Joining Type constants.
     * @see UProperty#JOINING_TYPE
     * @stable ICU 2.4
     */
    public static interface JoiningType
    {
        /**
         * @stable ICU 2.4
         */
        public static final int NON_JOINING = 0;
        /**
         * @stable ICU 2.4
         */
        public static final int JOIN_CAUSING = 1;
        /**
         * @stable ICU 2.4
         */
        public static final int DUAL_JOINING = 2;
        /**
         * @stable ICU 2.4
         */
        public static final int LEFT_JOINING = 3;
        /**
         * @stable ICU 2.4
         */
        public static final int RIGHT_JOINING = 4;
        /**
         * @stable ICU 2.4
         */
        public static final int TRANSPARENT = 5;
        /**
         * @stable ICU 2.4
         */
        public static final int COUNT = 6;
    }

    /**
     * Joining Group constants.
     * @see UProperty#JOINING_GROUP
     * @stable ICU 2.4
     */
    public static interface JoiningGroup
    {
        /**
         * @stable ICU 2.4
         */
        public static final int NO_JOINING_GROUP = 0;
        /**
         * @stable ICU 2.4
         */
        public static final int AIN = 1;
        /**
         * @stable ICU 2.4
         */
        public static final int ALAPH = 2;
        /**
         * @stable ICU 2.4
         */
        public static final int ALEF = 3;
        /**
         * @stable ICU 2.4
         */
        public static final int BEH = 4;
        /**
         * @stable ICU 2.4
         */
        public static final int BETH = 5;
        /**
         * @stable ICU 2.4
         */
        public static final int DAL = 6;
        /**
         * @stable ICU 2.4
         */
        public static final int DALATH_RISH = 7;
        /**
         * @stable ICU 2.4
         */
        public static final int E = 8;
        /**
         * @stable ICU 2.4
         */
        public static final int FEH = 9;
        /**
         * @stable ICU 2.4
         */
        public static final int FINAL_SEMKATH = 10;
        /**
         * @stable ICU 2.4
         */
        public static final int GAF = 11;
        /**
         * @stable ICU 2.4
         */
        public static final int GAMAL = 12;
        /**
         * @stable ICU 2.4
         */
        public static final int HAH = 13;
        /** @stable ICU 4.6 */
        public static final int TEH_MARBUTA_GOAL = 14;
        /**
         * @stable ICU 2.4
         */
        public static final int HAMZA_ON_HEH_GOAL = TEH_MARBUTA_GOAL;
        /**
         * @stable ICU 2.4
         */
        public static final int HE = 15;
        /**
         * @stable ICU 2.4
         */
        public static final int HEH = 16;
        /**
         * @stable ICU 2.4
         */
        public static final int HEH_GOAL = 17;
        /**
         * @stable ICU 2.4
         */
        public static final int HETH = 18;
        /**
         * @stable ICU 2.4
         */
        public static final int KAF = 19;
        /**
         * @stable ICU 2.4
         */
        public static final int KAPH = 20;
        /**
         * @stable ICU 2.4
         */
        public static final int KNOTTED_HEH = 21;
        /**
         * @stable ICU 2.4
         */
        public static final int LAM = 22;
        /**
         * @stable ICU 2.4
         */
        public static final int LAMADH = 23;
        /**
         * @stable ICU 2.4
         */
        public static final int MEEM = 24;
        /**
         * @stable ICU 2.4
         */
        public static final int MIM = 25;
        /**
         * @stable ICU 2.4
         */
        public static final int NOON = 26;
        /**
         * @stable ICU 2.4
         */
        public static final int NUN = 27;
        /**
         * @stable ICU 2.4
         */
        public static final int PE = 28;
        /**
         * @stable ICU 2.4
         */
        public static final int QAF = 29;
        /**
         * @stable ICU 2.4
         */
        public static final int QAPH = 30;
        /**
         * @stable ICU 2.4
         */
        public static final int REH = 31;
        /**
         * @stable ICU 2.4
         */
        public static final int REVERSED_PE = 32;
        /**
         * @stable ICU 2.4
         */
        public static final int SAD = 33;
        /**
         * @stable ICU 2.4
         */
        public static final int SADHE = 34;
        /**
         * @stable ICU 2.4
         */
        public static final int SEEN = 35;
        /**
         * @stable ICU 2.4
         */
        public static final int SEMKATH = 36;
        /**
         * @stable ICU 2.4
         */
        public static final int SHIN = 37;
        /**
         * @stable ICU 2.4
         */
        public static final int SWASH_KAF = 38;
        /**
         * @stable ICU 2.4
         */
        public static final int SYRIAC_WAW = 39;
        /**
         * @stable ICU 2.4
         */
        public static final int TAH = 40;
        /**
         * @stable ICU 2.4
         */
        public static final int TAW = 41;
        /**
         * @stable ICU 2.4
         */
        public static final int TEH_MARBUTA = 42;
        /**
         * @stable ICU 2.4
         */
        public static final int TETH = 43;
        /**
         * @stable ICU 2.4
         */
        public static final int WAW = 44;
        /**
         * @stable ICU 2.4
         */
        public static final int YEH = 45;
        /**
         * @stable ICU 2.4
         */
        public static final int YEH_BARREE = 46;
        /**
         * @stable ICU 2.4
         */
        public static final int YEH_WITH_TAIL = 47;
        /**
         * @stable ICU 2.4
         */
        public static final int YUDH = 48;
        /**
         * @stable ICU 2.4
         */
        public static final int YUDH_HE = 49;
        /**
         * @stable ICU 2.4
         */
        public static final int ZAIN = 50;
        /**
         * @stable ICU 2.6
         */
        public static final int FE = 51;
        /**
         * @stable ICU 2.6
         */
        public static final int KHAPH = 52;
        /**
         * @stable ICU 2.6
         */
        public static final int ZHAIN = 53;
        /**
         * @stable ICU 4.0
         */
        public static final int BURUSHASKI_YEH_BARREE = 54;
        /** @stable ICU 4.4 */
        public static final int FARSI_YEH = 55;
        /** @stable ICU 4.4 */
        public static final int NYA = 56;
        /**
         * @stable ICU 2.4
         */
        public static final int COUNT = 57;
    }

    /**
     * Grapheme Cluster Break constants.
     * @see UProperty#GRAPHEME_CLUSTER_BREAK
     * @stable ICU 3.4
     */
    public static interface GraphemeClusterBreak {
        /**
         * @stable ICU 3.4
         */
        public static final int OTHER = 0;
        /**
         * @stable ICU 3.4
         */
        public static final int CONTROL = 1;
        /**
         * @stable ICU 3.4
         */
        public static final int CR = 2;
        /**
         * @stable ICU 3.4
         */
        public static final int EXTEND = 3;
        /**
         * @stable ICU 3.4
         */
        public static final int L = 4;
        /**
         * @stable ICU 3.4
         */
        public static final int LF = 5;
        /**
         * @stable ICU 3.4
         */
        public static final int LV = 6;
        /**
         * @stable ICU 3.4
         */
        public static final int LVT = 7;
        /**
         * @stable ICU 3.4
         */
        public static final int T = 8;
        /**
         * @stable ICU 3.4
         */
        public static final int V = 9;
        /**
         * @stable ICU 4.0
         */
        public static final int SPACING_MARK = 10;
        /**
         * @stable ICU 4.0
         */
        public static final int PREPEND = 11;
        /**
         * @stable ICU 3.4
         */
        public static final int COUNT = 12;
    }

    /**
     * Word Break constants.
     * @see UProperty#WORD_BREAK
     * @stable ICU 3.4
     */
    public static interface WordBreak {
        /**
         * @stable ICU 3.8
         */
        public static final int OTHER = 0;
        /**
         * @stable ICU 3.8
         */
        public static final int ALETTER = 1;
        /**
         * @stable ICU 3.8
         */
        public static final int FORMAT = 2;
        /**
         * @stable ICU 3.8
         */
        public static final int KATAKANA = 3;
        /**
         * @stable ICU 3.8
         */
        public static final int MIDLETTER = 4;
        /**
         * @stable ICU 3.8
         */
        public static final int MIDNUM = 5;
        /**
         * @stable ICU 3.8
         */
        public static final int NUMERIC = 6;
        /**
         * @stable ICU 3.8
         */
        public static final int EXTENDNUMLET = 7;
        /**
         * @stable ICU 4.0
         */
        public static final int CR = 8;
        /**
         * @stable ICU 4.0
         */
        public static final int EXTEND = 9;
        /**
         * @stable ICU 4.0
         */
        public static final int LF = 10;
        /**
         * @stable ICU 4.0
         */
        public static final int MIDNUMLET = 11;
        /**
         * @stable ICU 4.0
         */
        public static final int NEWLINE = 12;
        /**
         * @stable ICU 4.0
         */
        public static final int COUNT = 13;
    }

    /**
     * Sentence Break constants.
     * @see UProperty#SENTENCE_BREAK
     * @stable ICU 3.4
     */
    public static interface SentenceBreak {
        /**
         * @stable ICU 3.8
         */
        public static final int OTHER = 0;
        /**
         * @stable ICU 3.8
         */
        public static final int ATERM = 1;
        /**
         * @stable ICU 3.8
         */
        public static final int CLOSE = 2;
        /**
         * @stable ICU 3.8
         */
        public static final int FORMAT = 3;
        /**
         * @stable ICU 3.8
         */
        public static final int LOWER = 4;
        /**
         * @stable ICU 3.8
         */
        public static final int NUMERIC = 5;
        /**
         * @stable ICU 3.8
         */
        public static final int OLETTER = 6;
        /**
         * @stable ICU 3.8
         */
        public static final int SEP = 7;
        /**
         * @stable ICU 3.8
         */
        public static final int SP = 8;
        /**
         * @stable ICU 3.8
         */
        public static final int STERM = 9;
        /**
         * @stable ICU 3.8
         */
        public static final int UPPER = 10;
        /**
         * @stable ICU 4.0
         */
        public static final int CR = 11;
        /**
         * @stable ICU 4.0
         */
        public static final int EXTEND = 12;
        /**
         * @stable ICU 4.0
         */
        public static final int LF = 13;
        /**
         * @stable ICU 4.0
         */
        public static final int SCONTINUE = 14;
        /**
         * @stable ICU 4.0
         */
        public static final int COUNT = 15;
    }

    /**
     * Line Break constants.
     * @see UProperty#LINE_BREAK
     * @stable ICU 2.4
     */
    public static interface LineBreak
    {
        /**
         * @stable ICU 2.4
         */
        public static final int UNKNOWN = 0;
        /**
         * @stable ICU 2.4
         */
        public static final int AMBIGUOUS = 1;
        /**
         * @stable ICU 2.4
         */
        public static final int ALPHABETIC = 2;
        /**
         * @stable ICU 2.4
         */
        public static final int BREAK_BOTH = 3;
        /**
         * @stable ICU 2.4
         */
        public static final int BREAK_AFTER = 4;
        /**
         * @stable ICU 2.4
         */
        public static final int BREAK_BEFORE = 5;
        /**
         * @stable ICU 2.4
         */
        public static final int MANDATORY_BREAK = 6;
        /**
         * @stable ICU 2.4
         */
        public static final int CONTINGENT_BREAK = 7;
        /**
         * @stable ICU 2.4
         */
        public static final int CLOSE_PUNCTUATION = 8;
        /**
         * @stable ICU 2.4
         */
        public static final int COMBINING_MARK = 9;
        /**
         * @stable ICU 2.4
         */
        public static final int CARRIAGE_RETURN = 10;
        /**
         * @stable ICU 2.4
         */
        public static final int EXCLAMATION = 11;
        /**
         * @stable ICU 2.4
         */
        public static final int GLUE = 12;
        /**
         * @stable ICU 2.4
         */
        public static final int HYPHEN = 13;
        /**
         * @stable ICU 2.4
         */
        public static final int IDEOGRAPHIC = 14;
        /**
         * @see #INSEPARABLE
         * @stable ICU 2.4
         */
        public static final int INSEPERABLE = 15;
        /**
         * Renamed from the misspelled "inseperable" in Unicode 4.0.1.
         * @stable ICU 3.0
         */
        public static final int INSEPARABLE = 15;
        /**
         * @stable ICU 2.4
         */
        public static final int INFIX_NUMERIC = 16;
        /**
         * @stable ICU 2.4
         */
        public static final int LINE_FEED = 17;
        /**
         * @stable ICU 2.4
         */
        public static final int NONSTARTER = 18;
        /**
         * @stable ICU 2.4
         */
        public static final int NUMERIC = 19;
        /**
         * @stable ICU 2.4
         */
        public static final int OPEN_PUNCTUATION = 20;
        /**
         * @stable ICU 2.4
         */
        public static final int POSTFIX_NUMERIC = 21;
        /**
         * @stable ICU 2.4
         */
        public static final int PREFIX_NUMERIC = 22;
        /**
         * @stable ICU 2.4
         */
        public static final int QUOTATION = 23;
        /**
         * @stable ICU 2.4
         */
        public static final int COMPLEX_CONTEXT = 24;
        /**
         * @stable ICU 2.4
         */
        public static final int SURROGATE = 25;
        /**
         * @stable ICU 2.4
         */
        public static final int SPACE = 26;
        /**
         * @stable ICU 2.4
         */
        public static final int BREAK_SYMBOLS = 27;
        /**
         * @stable ICU 2.4
         */
        public static final int ZWSPACE = 28;

        /**
         * @stable ICU 2.6
         */
        public static final int NEXT_LINE = 29;       /*[NL]*/

        /* from here on: new in Unicode 4/ICU 2.6 */

        /**
         * @stable ICU 2.6
         */
        public static final int  WORD_JOINER = 30;      /*[WJ]*/

        /* from here on: new in Unicode 4.1/ICU 3.4 */

        /**
         * @stable ICU 3.4
         */
        public static final int  H2 = 31;
        /**
         * @stable ICU 3.4
         */
        public static final int  H3 = 32;
        /**
         * @stable ICU 3.4
         */
        public static final int  JL = 33;
        /**
         * @stable ICU 3.4
         */
        public static final int  JT = 34;
        /**
         * @stable ICU 3.4
         */
        public static final int  JV = 35;
        /** @stable ICU 4.4 */
        public static final int  CLOSE_PARENTHESIS = 36; /*[CP]*/

        /* new in Unicode 5.2/ICU 4.4 */

        /**
         * @stable ICU 2.4
         */
        public static final int COUNT = 37;
    }

    /**
     * Numeric Type constants.
     * @see UProperty#NUMERIC_TYPE
     * @stable ICU 2.4
     */
    public static interface NumericType
    {
        /**
         * @stable ICU 2.4
         */
        public static final int NONE = 0;
        /**
         * @stable ICU 2.4
         */
        public static final int DECIMAL = 1;
        /**
         * @stable ICU 2.4
         */
        public static final int DIGIT = 2;
        /**
         * @stable ICU 2.4
         */
        public static final int NUMERIC = 3;
        /**
         * @stable ICU 2.4
         */
        public static final int COUNT = 4;
    }

    /**
     * Hangul Syllable Type constants.
     *
     * @see UProperty#HANGUL_SYLLABLE_TYPE
     * @stable ICU 2.6
     */
    public static interface HangulSyllableType
    {
        /**
         * @stable ICU 2.6
         */
        public static final int NOT_APPLICABLE      = 0;   /*[NA]*/ /*See note !!*/
        /**
         * @stable ICU 2.6
         */
        public static final int LEADING_JAMO        = 1;   /*[L]*/
        /**
         * @stable ICU 2.6
         */
        public static final int VOWEL_JAMO          = 2;   /*[V]*/
        /**
         * @stable ICU 2.6
         */
        public static final int TRAILING_JAMO       = 3;   /*[T]*/
        /**
         * @stable ICU 2.6
         */
        public static final int LV_SYLLABLE         = 4;   /*[LV]*/
        /**
         * @stable ICU 2.6
         */
        public static final int LVT_SYLLABLE        = 5;   /*[LVT]*/
        /**
         * @stable ICU 2.6
         */
        public static final int COUNT               = 6;
    }

    // public data members -----------------------------------------------

    /**
     * The lowest Unicode code point value.
     * @stable ICU 2.1
     */
    public static final int MIN_VALUE = UTF16.CODEPOINT_MIN_VALUE;

    /**
     * The highest Unicode code point value (scalar value) according to the
     * Unicode Standard.
     * This is a 21-bit value (21 bits, rounded up).<br>
     * Up-to-date Unicode implementation of java.lang.Character.MAX_VALUE
     * @stable ICU 2.1
     */
    public static final int MAX_VALUE = UTF16.CODEPOINT_MAX_VALUE;

    /**
     * The minimum value for Supplementary code points
     * @stable ICU 2.1
     */
    public static final int SUPPLEMENTARY_MIN_VALUE =
        UTF16.SUPPLEMENTARY_MIN_VALUE;

    /**
     * Unicode value used when translating into Unicode encoding form and there
     * is no existing character.
     * @stable ICU 2.1
     */
    public static final int REPLACEMENT_CHAR = '\uFFFD';

    /**
     * Special value that is returned by getUnicodeNumericValue(int) when no
     * numeric value is defined for a code point.
     * @stable ICU 2.4
     * @see #getUnicodeNumericValue
     */
    public static final double NO_NUMERIC_VALUE = -123456789;

    /**
     * Compatibility constant for Java Character's MIN_RADIX.
     * @stable ICU 3.4
     */
    public static final int MIN_RADIX = java.lang.Character.MIN_RADIX;

    /**
     * Compatibility constant for Java Character's MAX_RADIX.
     * @stable ICU 3.4
     */
    public static final int MAX_RADIX = java.lang.Character.MAX_RADIX;

    /**
     * Do not lowercase non-initial parts of words when titlecasing.
     * Option bit for titlecasing APIs that take an options bit set.
     *
     * By default, titlecasing will titlecase the first cased character
     * of a word and lowercase all other characters.
     * With this option, the other characters will not be modified.
     *
     * @see #toTitleCase
     * @stable ICU 3.8
     */
    public static final int TITLECASE_NO_LOWERCASE = 0x100;

    /**
     * Do not adjust the titlecasing indexes from BreakIterator::next() indexes;
     * titlecase exactly the characters at breaks from the iterator.
     * Option bit for titlecasing APIs that take an options bit set.
     *
     * By default, titlecasing will take each break iterator index,
     * adjust it by looking for the next cased character, and titlecase that one.
     * Other characters are lowercased.
     *
     * This follows Unicode 4 & 5 section 3.13 Default Case Operations:
     *
     * R3  toTitlecase(X): Find the word boundaries based on Unicode Standard Annex
     * #29, "Text Boundaries." Between each pair of word boundaries, find the first
     * cased character F. If F exists, map F to default_title(F); then map each
     * subsequent character C to default_lower(C).
     *
     * @see #toTitleCase
     * @see #TITLECASE_NO_LOWERCASE
     * @stable ICU 3.8
     */
    public static final int TITLECASE_NO_BREAK_ADJUSTMENT = 0x200;

    // public methods ----------------------------------------------------

    /**
     * Returnss the numeric value of a decimal digit code point.
     * <br>This method observes the semantics of
     * <code>java.lang.Character.digit()</code>.  Note that this
     * will return positive values for code points for which isDigit
     * returns false, just like java.lang.Character.
     * <br><em>Semantic Change:</em> In release 1.3.1 and
     * prior, this did not treat the European letters as having a
     * digit value, and also treated numeric letters and other numbers as
     * digits.
     * This has been changed to conform to the java semantics.
     * <br>A code point is a valid digit if and only if:
     * <ul>
     *   <li>ch is a decimal digit or one of the european letters, and
     *   <li>the value of ch is less than the specified radix.
     * </ul>
     * @param ch the code point to query
     * @param radix the radix
     * @return the numeric value represented by the code point in the
     * specified radix, or -1 if the code point is not a decimal digit
     * or if its value is too large for the radix
     * @stable ICU 2.1
     */
    public static int digit(int ch, int radix)
    {
        if (2 <= radix && radix <= 36) {
            int value = digit(ch);
            if (value < 0) {
                // ch is not a decimal digit, try latin letters
                value = UCharacterProperty.getEuropeanDigit(ch);
            }
            return (value < radix) ? value : -1;
        } else {
            return -1;  // invalid radix
        }
    }

    /**
     * Returnss the numeric value of a decimal digit code point.
     * <br>This is a convenience overload of <code>digit(int, int)</code>
     * that provides a decimal radix.
     * <br><em>Semantic Change:</em> In release 1.3.1 and prior, this
     * treated numeric letters and other numbers as digits.  This has
     * been changed to conform to the java semantics.
     * @param ch the code point to query
     * @return the numeric value represented by the code point,
     * or -1 if the code point is not a decimal digit or if its
     * value is too large for a decimal radix
     * @stable ICU 2.1
     */
    public static int digit(int ch)
    {
        return UCharacterProperty.INSTANCE.digit(ch);
    }

    /**
     * Returns the numeric value of the code point as a nonnegative
     * integer.
     * <br>If the code point does not have a numeric value, then -1 is returned.
     * <br>
     * If the code point has a numeric value that cannot be represented as a
     * nonnegative integer (for example, a fractional value), then -2 is
     * returned.
     * @param ch the code point to query
     * @return the numeric value of the code point, or -1 if it has no numeric
     * value, or -2 if it has a numeric value that cannot be represented as a
     * nonnegative integer
     * @stable ICU 2.1
     */
    public static int getNumericValue(int ch)
    {
        return UCharacterProperty.INSTANCE.getNumericValue(ch);
    }

    /**
     * {@icu} Returns the numeric value for a Unicode code point as defined in the
     * Unicode Character Database.</p>
     * <p>A "double" return type is necessary because some numeric values are
     * fractions, negative, or too large for int.</p>
     * <p>For characters without any numeric values in the Unicode Character
     * Database, this function will return NO_NUMERIC_VALUE.</p>
     * <p><em>API Change:</em> In release 2.2 and prior, this API has a
     * return type int and returns -1 when the argument ch does not have a
     * corresponding numeric value. This has been changed to synch with ICU4C
     * </p>
     * This corresponds to the ICU4C function u_getNumericValue.
     * @param ch Code point to get the numeric value for.
     * @return numeric value of ch, or NO_NUMERIC_VALUE if none is defined.
     * @stable ICU 2.4
     */
    public static double getUnicodeNumericValue(int ch)
    {
        return UCharacterProperty.INSTANCE.getUnicodeNumericValue(ch);
    }

    /**
     * Compatibility override of Java deprecated method.  This
     * method will always remain deprecated.
     * Same as java.lang.Character.isSpace().
     * @param ch the code point
     * @return true if the code point is a space character as
     * defined by java.lang.Character.isSpace.
     * @deprecated ICU 3.4 (Java)
     */
    public static boolean isSpace(int ch) {
        return ch <= 0x20 &&
            (ch == 0x20 || ch == 0x09 || ch == 0x0a || ch == 0x0c || ch == 0x0d);
    }

    /**
     * Returns a value indicating a code point's Unicode category.
     * Up-to-date Unicode implementation of java.lang.Character.getType()
     * except for the above mentioned code points that had their category
     * changed.<br>
     * Return results are constants from the interface
     * <a href=UCharacterCategory.html>UCharacterCategory</a><br>
     * <em>NOTE:</em> the UCharacterCategory values are <em>not</em> compatible with
     * those returned by java.lang.Character.getType.  UCharacterCategory values
     * match the ones used in ICU4C, while java.lang.Character type
     * values, though similar, skip the value 17.</p>
     * @param ch code point whose type is to be determined
     * @return category which is a value of UCharacterCategory
     * @stable ICU 2.1
     */
    public static int getType(int ch)
    {
        return UCharacterProperty.INSTANCE.getType(ch);
    }

    /**
     * Determines if a code point has a defined meaning in the up-to-date
     * Unicode standard.
     * E.g. supplementary code points though allocated space are not defined in
     * Unicode yet.<br>
     * Up-to-date Unicode implementation of java.lang.Character.isDefined()
     * @param ch code point to be determined if it is defined in the most
     *        current version of Unicode
     * @return true if this code point is defined in unicode
     * @stable ICU 2.1
     */
    public static boolean isDefined(int ch)
    {
        return getType(ch) != 0;
    }

    /**
     * Determines if a code point is a Java digit.
     * <br>This method observes the semantics of
     * <code>java.lang.Character.isDigit()</code>. It returns true for decimal
     * digits only.
     * <br><em>Semantic Change:</em> In release 1.3.1 and prior, this treated
     * numeric letters and other numbers as digits.
     * This has been changed to conform to the java semantics.
     * @param ch code point to query
     * @return true if this code point is a digit
     * @stable ICU 2.1
     */
    public static boolean isDigit(int ch)
    {
        return getType(ch) == UCharacterCategory.DECIMAL_DIGIT_NUMBER;
    }

    /**
     * Determines if the specified code point is an ISO control character.
     * A code point is considered to be an ISO control character if it is in
     * the range &#92u0000 through &#92u001F or in the range &#92u007F through
     * &#92u009F.<br>
     * Up-to-date Unicode implementation of java.lang.Character.isISOControl()
     * @param ch code point to determine if it is an ISO control character
     * @return true if code point is a ISO control character
     * @stable ICU 2.1
     */
    public static boolean isISOControl(int ch)
    {
        return ch >= 0 && ch <= APPLICATION_PROGRAM_COMMAND_ &&
            ((ch <= UNIT_SEPARATOR_) || (ch >= DELETE_));
    }

    /**
     * Determines if the specified code point is a letter.
     * Up-to-date Unicode implementation of java.lang.Character.isLetter()
     * @param ch code point to determine if it is a letter
     * @return true if code point is a letter
     * @stable ICU 2.1
     */
    public static boolean isLetter(int ch)
    {
        // if props == 0, it will just fall through and return false
        return ((1 << getType(ch))
        & ((1 << UCharacterCategory.UPPERCASE_LETTER)
           | (1 << UCharacterCategory.LOWERCASE_LETTER)
           | (1 << UCharacterCategory.TITLECASE_LETTER)
           | (1 << UCharacterCategory.MODIFIER_LETTER)
           | (1 << UCharacterCategory.OTHER_LETTER))) != 0;
    }

    /**
     * Determines if the specified code point is a letter or digit.
     * {@icunote} This method, unlike java.lang.Character does not regard the ascii
     * characters 'A' - 'Z' and 'a' - 'z' as digits.
     * @param ch code point to determine if it is a letter or a digit
     * @return true if code point is a letter or a digit
     * @stable ICU 2.1
     */
    public static boolean isLetterOrDigit(int ch)
    {
        return ((1 << getType(ch))
        & ((1 << UCharacterCategory.UPPERCASE_LETTER)
           | (1 << UCharacterCategory.LOWERCASE_LETTER)
           | (1 << UCharacterCategory.TITLECASE_LETTER)
           | (1 << UCharacterCategory.MODIFIER_LETTER)
           | (1 << UCharacterCategory.OTHER_LETTER)
           | (1 << UCharacterCategory.DECIMAL_DIGIT_NUMBER))) != 0;
    }

    /**
     * Compatibility override of Java deprecated method.  This
     * method will always remain deprecated.  Delegates to
     * java.lang.Character.isJavaIdentifierStart.
     * @param cp the code point
     * @return true if the code point can start a java identifier.
     * @deprecated ICU 3.4 (Java)
     */
    public static boolean isJavaLetter(int cp) {
        return isJavaIdentifierStart(cp);
    }

    /**
     * Compatibility override of Java deprecated method.  This
     * method will always remain deprecated.  Delegates to
     * java.lang.Character.isJavaIdentifierPart.
     * @param cp the code point
     * @return true if the code point can continue a java identifier.
     * @deprecated ICU 3.4 (Java)
     */
    public static boolean isJavaLetterOrDigit(int cp) {
        return isJavaIdentifierPart(cp);
    }

    /**
     * Compatibility override of Java method, delegates to
     * java.lang.Character.isJavaIdentifierStart.
     * @param cp the code point
     * @return true if the code point can start a java identifier.
     * @stable ICU 3.4
     */
    public static boolean isJavaIdentifierStart(int cp) {
        // note, downcast to char for jdk 1.4 compatibility
        return java.lang.Character.isJavaIdentifierStart((char)cp);
    }

    /**
     * Compatibility override of Java method, delegates to
     * java.lang.Character.isJavaIdentifierPart.
     * @param cp the code point
     * @return true if the code point can continue a java identifier.
     * @stable ICU 3.4
     */
    public static boolean isJavaIdentifierPart(int cp) {
        // note, downcast to char for jdk 1.4 compatibility
        return java.lang.Character.isJavaIdentifierPart((char)cp);
    }

    /**
     * Determines if the specified code point is a lowercase character.
     * UnicodeData only contains case mappings for code points where they are
     * one-to-one mappings; it also omits information about context-sensitive
     * case mappings.<br> For more information about Unicode case mapping
     * please refer to the
     * <a href=http://www.unicode.org/unicode/reports/tr21/>Technical report
     * #21</a>.<br>
     * Up-to-date Unicode implementation of java.lang.Character.isLowerCase()
     * @param ch code point to determine if it is in lowercase
     * @return true if code point is a lowercase character
     * @stable ICU 2.1
     */
    public static boolean isLowerCase(int ch)
    {
        // if props == 0, it will just fall through and return false
        return getType(ch) == UCharacterCategory.LOWERCASE_LETTER;
    }

    /**
     * Determines if the specified code point is a white space character.
     * A code point is considered to be an whitespace character if and only
     * if it satisfies one of the following criteria:
     * <ul>
     * <li> It is a Unicode Separator character (categories "Z" = "Zs" or "Zl" or "Zp"), but is not
     *      also a non-breaking space (&#92u00A0 or &#92u2007 or &#92u202F).
     * <li> It is &#92u0009, HORIZONTAL TABULATION.
     * <li> It is &#92u000A, LINE FEED.
     * <li> It is &#92u000B, VERTICAL TABULATION.
     * <li> It is &#92u000C, FORM FEED.
     * <li> It is &#92u000D, CARRIAGE RETURN.
     * <li> It is &#92u001C, FILE SEPARATOR.
     * <li> It is &#92u001D, GROUP SEPARATOR.
     * <li> It is &#92u001E, RECORD SEPARATOR.
     * <li> It is &#92u001F, UNIT SEPARATOR.
     * </ul>
     *
     * This API tries to sync with the semantics of Java's
     * java.lang.Character.isWhitespace(), but it may not return
     * the exact same results because of the Unicode version
     * difference.
     * <p>Note: Unicode 4.0.1 changed U+200B ZERO WIDTH SPACE from a Space Separator (Zs)
     * to a Format Control (Cf). Since then, isWhitespace(0x200b) returns false.
     * See http://www.unicode.org/versions/Unicode4.0.1/
     * @param ch code point to determine if it is a white space
     * @return true if the specified code point is a white space character
     * @stable ICU 2.1
     */
    public static boolean isWhitespace(int ch)
    {
        // exclude no-break spaces
        // if props == 0, it will just fall through and return false
        return ((1 << getType(ch)) &
                ((1 << UCharacterCategory.SPACE_SEPARATOR)
                 | (1 << UCharacterCategory.LINE_SEPARATOR)
                 | (1 << UCharacterCategory.PARAGRAPH_SEPARATOR))) != 0
        && (ch != NO_BREAK_SPACE_) && (ch != FIGURE_SPACE_) && (ch != NARROW_NO_BREAK_SPACE_)
        // TAB VT LF FF CR FS GS RS US NL are all control characters
        // that are white spaces.
        || (ch >= 0x9 && ch <= 0xd) || (ch >= 0x1c && ch <= 0x1f);
    }

    /**
     * Determines if the specified code point is a Unicode specified space
     * character, i.e. if code point is in the category Zs, Zl and Zp.
     * Up-to-date Unicode implementation of java.lang.Character.isSpaceChar().
     * @param ch code point to determine if it is a space
     * @return true if the specified code point is a space character
     * @stable ICU 2.1
     */
    public static boolean isSpaceChar(int ch)
    {
        // if props == 0, it will just fall through and return false
        return ((1 << getType(ch)) & ((1 << UCharacterCategory.SPACE_SEPARATOR)
                      | (1 << UCharacterCategory.LINE_SEPARATOR)
                      | (1 << UCharacterCategory.PARAGRAPH_SEPARATOR)))
        != 0;
    }

    /**
     * Determines if the specified code point is a titlecase character.
     * UnicodeData only contains case mappings for code points where they are
     * one-to-one mappings; it also omits information about context-sensitive
     * case mappings.<br>
     * For more information about Unicode case mapping please refer to the
     * <a href=http://www.unicode.org/unicode/reports/tr21/>
     * Technical report #21</a>.<br>
     * Up-to-date Unicode implementation of java.lang.Character.isTitleCase().
     * @param ch code point to determine if it is in title case
     * @return true if the specified code point is a titlecase character
     * @stable ICU 2.1
     */
    public static boolean isTitleCase(int ch)
    {
        // if props == 0, it will just fall through and return false
        return getType(ch) == UCharacterCategory.TITLECASE_LETTER;
    }

    /**
     * Determines if the specified code point may be any part of a Unicode
     * identifier other than the starting character.
     * A code point may be part of a Unicode identifier if and only if it is
     * one of the following:
     * <ul>
     * <li> Lu Uppercase letter
     * <li> Ll Lowercase letter
     * <li> Lt Titlecase letter
     * <li> Lm Modifier letter
     * <li> Lo Other letter
     * <li> Nl Letter number
     * <li> Pc Connecting punctuation character
     * <li> Nd decimal number
     * <li> Mc Spacing combining mark
     * <li> Mn Non-spacing mark
     * <li> Cf formatting code
     * </ul>
     * Up-to-date Unicode implementation of
     * java.lang.Character.isUnicodeIdentifierPart().<br>
     * See <a href=http://www.unicode.org/unicode/reports/tr8/>UTR #8</a>.
     * @param ch code point to determine if is can be part of a Unicode
     *        identifier
     * @return true if code point is any character belonging a unicode
     *         identifier suffix after the first character
     * @stable ICU 2.1
     */
    public static boolean isUnicodeIdentifierPart(int ch)
    {
        // if props == 0, it will just fall through and return false
        // cat == format
        return ((1 << getType(ch))
        & ((1 << UCharacterCategory.UPPERCASE_LETTER)
           | (1 << UCharacterCategory.LOWERCASE_LETTER)
           | (1 << UCharacterCategory.TITLECASE_LETTER)
           | (1 << UCharacterCategory.MODIFIER_LETTER)
           | (1 << UCharacterCategory.OTHER_LETTER)
           | (1 << UCharacterCategory.LETTER_NUMBER)
           | (1 << UCharacterCategory.CONNECTOR_PUNCTUATION)
           | (1 << UCharacterCategory.DECIMAL_DIGIT_NUMBER)
           | (1 << UCharacterCategory.COMBINING_SPACING_MARK)
           | (1 << UCharacterCategory.NON_SPACING_MARK))) != 0
        || isIdentifierIgnorable(ch);
    }

    /**
     * Determines if the specified code point is permissible as the first
     * character in a Unicode identifier.
     * A code point may start a Unicode identifier if it is of type either
     * <ul>
     * <li> Lu Uppercase letter
     * <li> Ll Lowercase letter
     * <li> Lt Titlecase letter
     * <li> Lm Modifier letter
     * <li> Lo Other letter
     * <li> Nl Letter number
     * </ul>
     * Up-to-date Unicode implementation of
     * java.lang.Character.isUnicodeIdentifierStart().<br>
     * See <a href=http://www.unicode.org/unicode/reports/tr8/>UTR #8</a>.
     * @param ch code point to determine if it can start a Unicode identifier
     * @return true if code point is the first character belonging a unicode
     *              identifier
     * @stable ICU 2.1
     */
    public static boolean isUnicodeIdentifierStart(int ch)
    {
        /*int cat = getType(ch);*/
        // if props == 0, it will just fall through and return false
        return ((1 << getType(ch))
        & ((1 << UCharacterCategory.UPPERCASE_LETTER)
           | (1 << UCharacterCategory.LOWERCASE_LETTER)
           | (1 << UCharacterCategory.TITLECASE_LETTER)
           | (1 << UCharacterCategory.MODIFIER_LETTER)
           | (1 << UCharacterCategory.OTHER_LETTER)
           | (1 << UCharacterCategory.LETTER_NUMBER))) != 0;
    }

    /**
     * Determines if the specified code point should be regarded as an
     * ignorable character in a Java identifier.
     * A character is Java-identifier-ignorable if it has the general category
     * Cf Formatting Control, or it is a non-Java-whitespace ISO control:
     * U+0000..U+0008, U+000E..U+001B, U+007F..U+009F.<br>
     * Up-to-date Unicode implementation of
     * java.lang.Character.isIdentifierIgnorable().<br>
     * See <a href=http://www.unicode.org/unicode/reports/tr8/>UTR #8</a>.
     * <p>Note that Unicode just recommends to ignore Cf (format controls).
     * @param ch code point to be determined if it can be ignored in a Unicode
     *        identifier.
     * @return true if the code point is ignorable
     * @stable ICU 2.1
     */
    public static boolean isIdentifierIgnorable(int ch)
    {
        // see java.lang.Character.isIdentifierIgnorable() on range of
        // ignorable characters.
        if (ch <= 0x9f) {
            return isISOControl(ch)
                && !((ch >= 0x9 && ch <= 0xd)
                     || (ch >= 0x1c && ch <= 0x1f));
        }
        return getType(ch) == UCharacterCategory.FORMAT;
    }

    /**
     * Determines if the specified code point is an uppercase character.
     * UnicodeData only contains case mappings for code point where they are
     * one-to-one mappings; it also omits information about context-sensitive
     * case mappings.<br>
     * For language specific case conversion behavior, use
     * toUpperCase(locale, str). <br>
     * For example, the case conversion for dot-less i and dotted I in Turkish,
     * or for final sigma in Greek.
     * For more information about Unicode case mapping please refer to the
     * <a href=http://www.unicode.org/unicode/reports/tr21/>
     * Technical report #21</a>.<br>
     * Up-to-date Unicode implementation of java.lang.Character.isUpperCase().
     * @param ch code point to determine if it is in uppercase
     * @return true if the code point is an uppercase character
     * @stable ICU 2.1
     */
    public static boolean isUpperCase(int ch)
    {
        // if props == 0, it will just fall through and return false
        return getType(ch) == UCharacterCategory.UPPERCASE_LETTER;
    }

    /**
     * The given code point is mapped to its lowercase equivalent; if the code
     * point has no lowercase equivalent, the code point itself is returned.
     * Up-to-date Unicode implementation of java.lang.Character.toLowerCase()
     *
     * <p>This function only returns the simple, single-code point case mapping.
     * Full case mappings should be used whenever possible because they produce
     * better results by working on whole strings.
     * They take into account the string context and the language and can map
     * to a result string with a different length as appropriate.
     * Full case mappings are applied by the case mapping functions
     * that take String parameters rather than code points (int).
     * See also the User Guide chapter on C/POSIX migration:
     * http://www.icu-project.org/userguide/posix.html#case_mappings
     *
     * @param ch code point whose lowercase equivalent is to be retrieved
     * @return the lowercase equivalent code point
     * @stable ICU 2.1
     */
    public static int toLowerCase(int ch) {
        return UCaseProps.INSTANCE.tolower(ch);
    }

    /**
     * Converts argument code point and returns a String object representing
     * the code point's value in UTF16 format.
     * The result is a string whose length is 1 for non-supplementary code
     * points, 2 otherwise.<br>
     * com.ibm.ibm.icu.UTF16 can be used to parse Strings generated by this
     * function.<br>
     * Up-to-date Unicode implementation of java.lang.Character.toString()
     * @param ch code point
     * @return string representation of the code point, null if code point is not
     *         defined in unicode
     * @stable ICU 2.1
     */
    public static String toString(int ch)
    {
        if (ch < MIN_VALUE || ch > MAX_VALUE) {
            return null;
        }

        if (ch < SUPPLEMENTARY_MIN_VALUE) {
            return String.valueOf((char)ch);
        }

        StringBuilder result = new StringBuilder();
        result.append(UTF16.getLeadSurrogate(ch));
        result.append(UTF16.getTrailSurrogate(ch));
        return result.toString();
    }

    /**
     * Converts the code point argument to titlecase.
     * If no titlecase is available, the uppercase is returned. If no uppercase
     * is available, the code point itself is returned.
     * Up-to-date Unicode implementation of java.lang.Character.toTitleCase()
     *
     * <p>This function only returns the simple, single-code point case mapping.
     * Full case mappings should be used whenever possible because they produce
     * better results by working on whole strings.
     * They take into account the string context and the language and can map
     * to a result string with a different length as appropriate.
     * Full case mappings are applied by the case mapping functions
     * that take String parameters rather than code points (int).
     * See also the User Guide chapter on C/POSIX migration:
     * http://www.icu-project.org/userguide/posix.html#case_mappings
     *
     * @param ch code point  whose title case is to be retrieved
     * @return titlecase code point
     * @stable ICU 2.1
     */
    public static int toTitleCase(int ch) {
        return UCaseProps.INSTANCE.totitle(ch);
    }

    /**
     * Converts the character argument to uppercase.
     * If no uppercase is available, the character itself is returned.
     * Up-to-date Unicode implementation of java.lang.Character.toUpperCase()
     *
     * <p>This function only returns the simple, single-code point case mapping.
     * Full case mappings should be used whenever possible because they produce
     * better results by working on whole strings.
     * They take into account the string context and the language and can map
     * to a result string with a different length as appropriate.
     * Full case mappings are applied by the case mapping functions
     * that take String parameters rather than code points (int).
     * See also the User Guide chapter on C/POSIX migration:
     * http://www.icu-project.org/userguide/posix.html#case_mappings
     *
     * @param ch code point whose uppercase is to be retrieved
     * @return uppercase code point
     * @stable ICU 2.1
     */
    public static int toUpperCase(int ch) {
        return UCaseProps.INSTANCE.toupper(ch);
    }

    // extra methods not in java.lang.Character --------------------------

    /**
     * {@icu} Determines if the code point is a supplementary character.
     * A code point is a supplementary character if and only if it is greater
     * than <a href=#SUPPLEMENTARY_MIN_VALUE>SUPPLEMENTARY_MIN_VALUE</a>
     * @param ch code point to be determined if it is in the supplementary
     *        plane
     * @return true if code point is a supplementary character
     * @stable ICU 2.1
     */
    public static boolean isSupplementary(int ch)
    {
        return ch >= UCharacter.SUPPLEMENTARY_MIN_VALUE &&
            ch <= UCharacter.MAX_VALUE;
    }

    /**
     * {@icu} Determines if the code point is in the BMP plane.
     * @param ch code point to be determined if it is not a supplementary
     *        character
     * @return true if code point is not a supplementary character
     * @stable ICU 2.1
     */
    public static boolean isBMP(int ch)
    {
        return (ch >= 0 && ch <= LAST_CHAR_MASK_);
    }

    /**
     * {@icu} Determines whether the specified code point is a printable character
     * according to the Unicode standard.
     * @param ch code point to be determined if it is printable
     * @return true if the code point is a printable character
     * @stable ICU 2.1
     */
    public static boolean isPrintable(int ch)
    {
        int cat = getType(ch);
        // if props == 0, it will just fall through and return false
        return (cat != UCharacterCategory.UNASSIGNED &&
        cat != UCharacterCategory.CONTROL &&
        cat != UCharacterCategory.FORMAT &&
        cat != UCharacterCategory.PRIVATE_USE &&
        cat != UCharacterCategory.SURROGATE &&
        cat != UCharacterCategory.GENERAL_OTHER_TYPES);
    }

    /**
     * {@icu} Determines whether the specified code point is of base form.
     * A code point of base form does not graphically combine with preceding
     * characters, and is neither a control nor a format character.
     * @param ch code point to be determined if it is of base form
     * @return true if the code point is of base form
     * @stable ICU 2.1
     */
    public static boolean isBaseForm(int ch)
    {
        int cat = getType(ch);
        // if props == 0, it will just fall through and return false
        return cat == UCharacterCategory.DECIMAL_DIGIT_NUMBER ||
            cat == UCharacterCategory.OTHER_NUMBER ||
            cat == UCharacterCategory.LETTER_NUMBER ||
            cat == UCharacterCategory.UPPERCASE_LETTER ||
            cat == UCharacterCategory.LOWERCASE_LETTER ||
            cat == UCharacterCategory.TITLECASE_LETTER ||
            cat == UCharacterCategory.MODIFIER_LETTER ||
            cat == UCharacterCategory.OTHER_LETTER ||
            cat == UCharacterCategory.NON_SPACING_MARK ||
            cat == UCharacterCategory.ENCLOSING_MARK ||
            cat == UCharacterCategory.COMBINING_SPACING_MARK;
    }

    /**
     * {@icu} Returns the Bidirection property of a code point.
     * For example, 0x0041 (letter A) has the LEFT_TO_RIGHT directional
     * property.<br>
     * Result returned belongs to the interface
     * <a href=UCharacterDirection.html>UCharacterDirection</a>
     * @param ch the code point to be determined its direction
     * @return direction constant from UCharacterDirection.
     * @stable ICU 2.1
     */
    public static int getDirection(int ch)
    {
        return UBiDiProps.INSTANCE.getClass(ch);
    }

    /**
     * Determines whether the code point has the "mirrored" property.
     * This property is set for characters that are commonly used in
     * Right-To-Left contexts and need to be displayed with a "mirrored"
     * glyph.
     * @param ch code point whose mirror is to be determined
     * @return true if the code point has the "mirrored" property
     * @stable ICU 2.1
     */
    public static boolean isMirrored(int ch)
    {
        return UBiDiProps.INSTANCE.isMirrored(ch);
    }

    /**
     * {@icu} Maps the specified code point to a "mirror-image" code point.
     * For code points with the "mirrored" property, implementations sometimes
     * need a "poor man's" mapping to another code point such that the default
     * glyph may serve as the mirror-image of the default glyph of the
     * specified code point.<br>
     * This is useful for text conversion to and from codepages with visual
     * order, and for displays without glyph selection capabilities.
     * @param ch code point whose mirror is to be retrieved
     * @return another code point that may serve as a mirror-image substitute,
     *         or ch itself if there is no such mapping or ch does not have the
     *         "mirrored" property
     * @stable ICU 2.1
     */
    public static int getMirror(int ch)
    {
        return UBiDiProps.INSTANCE.getMirror(ch);
    }

    /**
     * {@icu} Returns the combining class of the argument codepoint
     * @param ch code point whose combining is to be retrieved
     * @return the combining class of the codepoint
     * @stable ICU 2.1
     */
    public static int getCombiningClass(int ch)
    {
        if (ch < MIN_VALUE || ch > MAX_VALUE) {
            throw new IllegalArgumentException("Codepoint out of bounds");
        }
        Normalizer2Impl impl = Norm2AllModes.getNFCInstance().impl;
        return impl.getCC(impl.getNorm16(ch));
    }

    /**
     * {@icu} A code point is illegal if and only if
     * <ul>
     * <li> Out of bounds, less than 0 or greater than UCharacter.MAX_VALUE
     * <li> A surrogate value, 0xD800 to 0xDFFF
     * <li> Not-a-character, having the form 0x xxFFFF or 0x xxFFFE
     * </ul>
     * Note: legal does not mean that it is assigned in this version of Unicode.
     * @param ch code point to determine if it is a legal code point by itself
     * @return true if and only if legal.
     * @stable ICU 2.1
     */
    public static boolean isLegal(int ch)
    {
        if (ch < MIN_VALUE) {
            return false;
        }
        if (ch < UTF16.SURROGATE_MIN_VALUE) {
            return true;
        }
        if (ch <= UTF16.SURROGATE_MAX_VALUE) {
            return false;
        }
        if (UCharacterUtility.isNonCharacter(ch)) {
            return false;
        }
        return (ch <= MAX_VALUE);
    }

    /**
     * {@icu} A string is legal iff all its code points are legal.
     * A code point is illegal if and only if
     * <ul>
     * <li> Out of bounds, less than 0 or greater than UCharacter.MAX_VALUE
     * <li> A surrogate value, 0xD800 to 0xDFFF
     * <li> Not-a-character, having the form 0x xxFFFF or 0x xxFFFE
     * </ul>
     * Note: legal does not mean that it is assigned in this version of Unicode.
     * @param str containing code points to examin
     * @return true if and only if legal.
     * @stable ICU 2.1
     */
    public static boolean isLegal(String str)
    {
        int size = str.length();
        int codepoint;
        for (int i = 0; i < size; i ++)
        {
        codepoint = UTF16.charAt(str, i);
        if (!isLegal(codepoint)) {
            return false;
        }
        if (isSupplementary(codepoint)) {
            i ++;
        }
        }
        return true;
    }

    /**
     * {@icu} Returns the version of Unicode data used.
     * @return the unicode version number used
     * @stable ICU 2.1
     */
    public static VersionInfo getUnicodeVersion()
    {
        return UCharacterProperty.INSTANCE.m_unicodeVersion_;
    }

    /**
     * {@icu} Returns the most current Unicode name of the argument code point, or
     * null if the character is unassigned or outside the range
     * UCharacter.MIN_VALUE and UCharacter.MAX_VALUE or does not have a name.
     * <br>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param ch the code point for which to get the name
     * @return most current Unicode name
     * @stable ICU 2.1
     */
    public static String getName(int ch)
    {
        return UCharacterName.INSTANCE.getName(ch, UCharacterNameChoice.UNICODE_CHAR_NAME);
    }

    /**
     * {@icu} Returns the names for each of the characters in a string
     * @param s string to format
     * @param separator string to go between names
     * @return string of names
     * @stable ICU 3.8
     */
    public static String getName(String s, String separator) {
        if (s.length() == 1) { // handle common case
            return getName(s.charAt(0));
        }
        int cp;
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < s.length(); i += UTF16.getCharCount(cp)) {
            cp = UTF16.charAt(s,i);
            if (i != 0) sb.append(separator);
            sb.append(UCharacter.getName(cp));
        }
        return sb.toString();
    }

    /**
     * {@icu} Returns the earlier version 1.0 Unicode name of the argument code
     * point, or null if the character is unassigned or outside the range
     * UCharacter.MIN_VALUE and UCharacter.MAX_VALUE or does not have a name.
     * <br>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param ch the code point for which to get the name
     * @return version 1.0 Unicode name
     * @stable ICU 2.1
     */
    public static String getName1_0(int ch)
    {
        return UCharacterName.INSTANCE.getName(ch,
                             UCharacterNameChoice.UNICODE_10_CHAR_NAME);
    }

    /**
     * {@icu} Returns a name for a valid codepoint. Unlike, getName(int) and
     * getName1_0(int), this method will return a name even for codepoints that
     * are not assigned a name in UnicodeData.txt.
     * </p>
     * The names are returned in the following order.
     * <ul>
     * <li> Most current Unicode name if there is any
     * <li> Unicode 1.0 name if there is any
     * <li> Extended name in the form of
     *      "<codepoint_type-codepoint_hex_digits>". E.g. <noncharacter-fffe>
     * </ul>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param ch the code point for which to get the name
     * @return a name for the argument codepoint
     * @stable ICU 2.6
     */
    public static String getExtendedName(int ch) {
        return UCharacterName.INSTANCE.getName(ch, UCharacterNameChoice.EXTENDED_CHAR_NAME);
    }

    /**
     * {@icu} Returns the corrected name from NameAliases.txt if there is one.
     * Returns null if the character is unassigned or outside the range
     * UCharacter.MIN_VALUE and UCharacter.MAX_VALUE or does not have a name.
     * <br>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param ch the code point for which to get the name alias
     * @return Unicode name alias, or null
     * @draft ICU 4.4
     * @provisional This API might change or be removed in a future release.
     */
    public static String getNameAlias(int ch)
    {
        return UCharacterName.INSTANCE.getName(ch, UCharacterNameChoice.CHAR_NAME_ALIAS);
    }

    /**
     * {@icu} Returns the ISO 10646 comment for a character.
     * The ISO 10646 comment is an informative field in the Unicode Character
     * Database (UnicodeData.txt field 11) and is from the ISO 10646 names list.
     *
     * Note: Unicode 5.2 removes all ISO comment data, resulting in empty strings
     * returned for all characters.
     *
     * @param ch The code point for which to get the ISO comment.
     *           It must be the case that {@code 0 <= ch <= 0x10ffff}.
     * @return The ISO comment, or null if there is no comment for this
     *         character.
     * @stable ICU 2.4
     */
    public static String getISOComment(int ch)
    {
        if (ch < UCharacter.MIN_VALUE || ch > UCharacter.MAX_VALUE) {
            return null;
        }

        String result = UCharacterName.INSTANCE.getGroupName(ch,
                                           UCharacterNameChoice.ISO_COMMENT_);
        return result;
    }

    /**
     * {@icu} <p>Finds a Unicode code point by its most current Unicode name and
     * return its code point value. All Unicode names are in uppercase.</p>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param name most current Unicode character name whose code point is to
     *        be returned
     * @return code point or -1 if name is not found
     * @stable ICU 2.1
     */
    public static int getCharFromName(String name){
        return UCharacterName.INSTANCE.getCharFromName(
                     UCharacterNameChoice.UNICODE_CHAR_NAME, name);
    }

    /**
     * {@icu} <p>Find a Unicode character by its version 1.0 Unicode name and return
     * its code point value. All Unicode names are in uppercase.</p>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param name Unicode 1.0 code point name whose code point is to
     *             returned
     * @return code point or -1 if name is not found
     * @stable ICU 2.1
     */
    public static int getCharFromName1_0(String name){
        return UCharacterName.INSTANCE.getCharFromName(
                     UCharacterNameChoice.UNICODE_10_CHAR_NAME, name);
    }

    /**
     * {@icu} <p>Find a Unicode character by either its name and return its code
     * point value. All Unicode names are in uppercase.
     * Extended names are all lowercase except for numbers and are contained
     * within angle brackets.</p>
     * The names are searched in the following order
     * <ul>
     * <li> Most current Unicode name if there is any
     * <li> Unicode 1.0 name if there is any
     * <li> Extended name in the form of
     *      "<codepoint_type-codepoint_hex_digits>". E.g. <noncharacter-FFFE>
     * </ul>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param name codepoint name
     * @return code point associated with the name or -1 if the name is not
     *         found.
     * @stable ICU 2.6
     */
    public static int getCharFromExtendedName(String name){
        return UCharacterName.INSTANCE.getCharFromName(
                     UCharacterNameChoice.EXTENDED_CHAR_NAME, name);
    }

    /**
     * {@icu} <p>Find a Unicode character by its corrected name alias and return
     * its code point value. All Unicode names are in uppercase.</p>
     * Note calling any methods related to code point names, e.g. get*Name*()
     * incurs a one-time initialisation cost to construct the name tables.
     * @param name Unicode name alias whose code point is to be returned
     * @return code point or -1 if name is not found
     * @draft ICU 4.4
     * @provisional This API might change or be removed in a future release.
     */
    public static int getCharFromNameAlias(String name){
        return UCharacterName.INSTANCE.getCharFromName(UCharacterNameChoice.CHAR_NAME_ALIAS, name);
    }

    /**
     * {@icu} Return the Unicode name for a given property, as given in the
     * Unicode database file PropertyAliases.txt.  Most properties
     * have more than one name.  The nameChoice determines which one
     * is returned.
     *
     * In addition, this function maps the property
     * UProperty.GENERAL_CATEGORY_MASK to the synthetic names "gcm" /
     * "General_Category_Mask".  These names are not in
     * PropertyAliases.txt.
     *
     * @param property UProperty selector.
     *
     * @param nameChoice UProperty.NameChoice selector for which name
     * to get.  All properties have a long name.  Most have a short
     * name, but some do not.  Unicode allows for additional names; if
     * present these will be returned by UProperty.NameChoice.LONG + i,
     * where i=1, 2,...
     *
     * @return a name, or null if Unicode explicitly defines no name
     * ("n/a") for a given property/nameChoice.  If a given nameChoice
     * throws an exception, then all larger values of nameChoice will
     * throw an exception.  If null is returned for a given
     * nameChoice, then other nameChoice values may return non-null
     * results.
     *
     * @exception IllegalArgumentException thrown if property or
     * nameChoice are invalid.
     *
     * @see UProperty
     * @see UProperty.NameChoice
     * @stable ICU 2.4
     */
    public static String getPropertyName(int property,
                                         int nameChoice) {
        return UPropertyAliases.INSTANCE.getPropertyName(property, nameChoice);
    }

    /**
     * {@icu} Return the UProperty selector for a given property name, as
     * specified in the Unicode database file PropertyAliases.txt.
     * Short, long, and any other variants are recognized.
     *
     * In addition, this function maps the synthetic names "gcm" /
     * "General_Category_Mask" to the property
     * UProperty.GENERAL_CATEGORY_MASK.  These names are not in
     * PropertyAliases.txt.
     *
     * @param propertyAlias the property name to be matched.  The name
     * is compared using "loose matching" as described in
     * PropertyAliases.txt.
     *
     * @return a UProperty enum.
     *
     * @exception IllegalArgumentException thrown if propertyAlias
     * is not recognized.
     *
     * @see UProperty
     * @stable ICU 2.4
     */
    public static int getPropertyEnum(String propertyAlias) {
        int propEnum = UPropertyAliases.INSTANCE.getPropertyEnum(propertyAlias);
        if (propEnum == UProperty.UNDEFINED) {
            throw new IllegalIcuArgumentException("Invalid name: " + propertyAlias);
        }
        return propEnum;
    }

    /**
     * {@icu} Return the Unicode name for a given property value, as given in
     * the Unicode database file PropertyValueAliases.txt.  Most
     * values have more than one name.  The nameChoice determines
     * which one is returned.
     *
     * Note: Some of the names in PropertyValueAliases.txt can only be
     * retrieved using UProperty.GENERAL_CATEGORY_MASK, not
     * UProperty.GENERAL_CATEGORY.  These include: "C" / "Other", "L" /
     * "Letter", "LC" / "Cased_Letter", "M" / "Mark", "N" / "Number", "P"
     * / "Punctuation", "S" / "Symbol", and "Z" / "Separator".
     *
     * @param property UProperty selector constant.
     * UProperty.INT_START &lt;= property &lt; UProperty.INT_LIMIT or
     * UProperty.BINARY_START &lt;= property &lt; UProperty.BINARY_LIMIT or
     * UProperty.MASK_START &lt; = property &lt; UProperty.MASK_LIMIT.
     * If out of range, null is returned.
     *
     * @param value selector for a value for the given property.  In
     * general, valid values range from 0 up to some maximum.  There
     * are a few exceptions: (1.) UProperty.BLOCK values begin at the
     * non-zero value BASIC_LATIN.getID().  (2.)
     * UProperty.CANONICAL_COMBINING_CLASS values are not contiguous
     * and range from 0..240.  (3.)  UProperty.GENERAL_CATEGORY_MASK values
     * are mask values produced by left-shifting 1 by
     * UCharacter.getType().  This allows grouped categories such as
     * [:L:] to be represented.  Mask values are non-contiguous.
     *
     * @param nameChoice UProperty.NameChoice selector for which name
     * to get.  All values have a long name.  Most have a short name,
     * but some do not.  Unicode allows for additional names; if
     * present these will be returned by UProperty.NameChoice.LONG + i,
     * where i=1, 2,...
     *
     * @return a name, or null if Unicode explicitly defines no name
     * ("n/a") for a given property/value/nameChoice.  If a given
     * nameChoice throws an exception, then all larger values of
     * nameChoice will throw an exception.  If null is returned for a
     * given nameChoice, then other nameChoice values may return
     * non-null results.
     *
     * @exception IllegalArgumentException thrown if property, value,
     * or nameChoice are invalid.
     *
     * @see UProperty
     * @see UProperty.NameChoice
     * @stable ICU 2.4
     */
    public static String getPropertyValueName(int property,
                                              int value,
                                              int nameChoice)
    {
        if ((property == UProperty.CANONICAL_COMBINING_CLASS
             || property == UProperty.LEAD_CANONICAL_COMBINING_CLASS
             || property == UProperty.TRAIL_CANONICAL_COMBINING_CLASS)
            && value >= UCharacter.getIntPropertyMinValue(
                              UProperty.CANONICAL_COMBINING_CLASS)
            && value <= UCharacter.getIntPropertyMaxValue(
                              UProperty.CANONICAL_COMBINING_CLASS)
            && nameChoice >= 0 && nameChoice < UProperty.NameChoice.COUNT) {
            // this is hard coded for the valid cc
            // because PropertyValueAliases.txt does not contain all of them
            try {
                return UPropertyAliases.INSTANCE.getPropertyValueName(property, value,
                                                    nameChoice);
            }
            catch (IllegalArgumentException e) {
                return null;
            }
        }
        return UPropertyAliases.INSTANCE.getPropertyValueName(property, value, nameChoice);
    }

    /**
     * {@icu} Return the property value integer for a given value name, as
     * specified in the Unicode database file PropertyValueAliases.txt.
     * Short, long, and any other variants are recognized.
     *
     * Note: Some of the names in PropertyValueAliases.txt will only be
     * recognized with UProperty.GENERAL_CATEGORY_MASK, not
     * UProperty.GENERAL_CATEGORY.  These include: "C" / "Other", "L" /
     * "Letter", "LC" / "Cased_Letter", "M" / "Mark", "N" / "Number", "P"
     * / "Punctuation", "S" / "Symbol", and "Z" / "Separator".
     *
     * @param property UProperty selector constant.
     * UProperty.INT_START &lt;= property &lt; UProperty.INT_LIMIT or
     * UProperty.BINARY_START &lt;= property &lt; UProperty.BINARY_LIMIT or
     * UProperty.MASK_START &lt; = property &lt; UProperty.MASK_LIMIT.
     * Only these properties can be enumerated.
     *
     * @param valueAlias the value name to be matched.  The name is
     * compared using "loose matching" as described in
     * PropertyValueAliases.txt.
     *
     * @return a value integer.  Note: UProperty.GENERAL_CATEGORY
     * values are mask values produced by left-shifting 1 by
     * UCharacter.getType().  This allows grouped categories such as
     * [:L:] to be represented.
     *
     * @see UProperty
     * @throws IllegalArgumentException if property is not a valid UProperty
     *         selector
     * @stable ICU 2.4
     */
    public static int getPropertyValueEnum(int property, String valueAlias) {
        int propEnum = UPropertyAliases.INSTANCE.getPropertyValueEnum(property, valueAlias);
        if (propEnum == UProperty.UNDEFINED) {
            throw new IllegalIcuArgumentException("Invalid name: " + valueAlias);
        }
        return propEnum;
    }

    /**
     * {@icu} Returns a code point corresponding to the two UTF16 characters.
     * @param lead the lead char
     * @param trail the trail char
     * @return code point if surrogate characters are valid.
     * @exception IllegalArgumentException thrown when argument characters do
     *            not form a valid codepoint
     * @stable ICU 2.1
     */
    public static int getCodePoint(char lead, char trail)
    {
        if (UTF16.isLeadSurrogate(lead) && UTF16.isTrailSurrogate(trail)) {
            return UCharacterProperty.getRawSupplementary(lead, trail);
        }
        throw new IllegalArgumentException("Illegal surrogate characters");
    }

    /**
     * {@icu} Returns the code point corresponding to the UTF16 character.
     * @param char16 the UTF16 character
     * @return code point if argument is a valid character.
     * @exception IllegalArgumentException thrown when char16 is not a valid
     *            codepoint
     * @stable ICU 2.1
     */
    public static int getCodePoint(char char16)
    {
        if (UCharacter.isLegal(char16)) {
            return char16;
        }
        throw new IllegalArgumentException("Illegal codepoint");
    }

    /**
     * Implementation of UCaseProps.ContextIterator, iterates over a String.
     * See ustrcase.c/utf16_caseContextIterator().
     */
    private static class StringContextIterator implements UCaseProps.ContextIterator {
        /**
         * Constructor.
         * @param s String to iterate over.
         */
        StringContextIterator(String s) {
            this.s=s;
            limit=s.length();
            cpStart=cpLimit=index=0;
            dir=0;
        }

        /**
         * Set the iteration limit for nextCaseMapCP() to an index within the string.
         * If the limit parameter is negative or past the string, then the
         * string length is restored as the iteration limit.
         *
         * This limit does not affect the next() function which always
         * iterates to the very end of the string.
         *
         * @param lim The iteration limit.
         */
        public void setLimit(int lim) {
            if(0<=lim && lim<=s.length()) {
                limit=lim;
            } else {
                limit=s.length();
            }
        }

        /**
         * Move to the iteration limit without fetching code points up to there.
         */
        public void moveToLimit() {
            cpStart=cpLimit=limit;
        }

        /**
         * Iterate forward through the string to fetch the next code point
         * to be case-mapped, and set the context indexes for it.
         * Performance optimization, to save on function calls and redundant
         * tests. Combines UTF16.charAt(), UTF16.getCharCount(), and setIndex().
         *
         * When the iteration limit is reached (and -1 is returned),
         * getCPStart() will be at the iteration limit.
         *
         * Iteration with next() does not affect the position for nextCaseMapCP().
         *
         * @return The next code point to be case-mapped, or <0 when the iteration is done.
         */
        public int nextCaseMapCP() {
            cpStart=cpLimit;
            if(cpLimit<limit) {
                int c=s.charAt(cpLimit++);
                if(UTF16.LEAD_SURROGATE_MIN_VALUE<=c || c<=UTF16.TRAIL_SURROGATE_MAX_VALUE) {
                    char c2;
                    if( c<=UTF16.LEAD_SURROGATE_MAX_VALUE && cpLimit<limit &&
                        UTF16.TRAIL_SURROGATE_MIN_VALUE<=(c2=s.charAt(cpLimit)) &&
                        c2<=UTF16.TRAIL_SURROGATE_MAX_VALUE
                    ) {
                        // supplementary code point
                        ++cpLimit;
                        c=UCharacterProperty.getRawSupplementary((char)c, c2);
                    // else unpaired surrogate code point
                    }
                // else BMP code point
                }
                return c;
            } else {
                return -1;
            }
        }

        /**
         * Returns the start of the code point that was last returned
         * by nextCaseMapCP().
         */
        public int getCPStart() {
            return cpStart;
        }

        /**
         * Returns the limit of the code point that was last returned
         * by nextCaseMapCP().
         */
        public int getCPLimit() {
            return cpLimit;
        }

        // implement UCaseProps.ContextIterator
        // The following code is not used anywhere in this private class
        public void reset(int direction) {
            if(direction>0) {
                /* reset for forward iteration */
                dir=1;
                index=cpLimit;
            } else if(direction<0) {
                /* reset for backward iteration */
                dir=-1;
                index=cpStart;
            } else {
                // not a valid direction
                dir=0;
                index=0;
            }
        }

        public int next() {
            int c;

            if(dir>0 && index<s.length()) {
                c=UTF16.charAt(s, index);
                index+=UTF16.getCharCount(c);
                return c;
            } else if(dir<0 && index>0) {
                c=UTF16.charAt(s, index-1);
                index-=UTF16.getCharCount(c);
                return c;
            }
            return -1;
        }

        // variables
        protected String s;
        protected int index, limit, cpStart, cpLimit;
        protected int dir; // 0=initial state  >0=forward  <0=backward
    }

    /**
     * Returns the uppercase version of the argument string.
     * Casing is dependent on the default locale and context-sensitive.
     * @param str source string to be performed on
     * @return uppercase version of the argument string
     * @stable ICU 2.1
     */
    public static String toUpperCase(String str)
    {
        return toUpperCase(ULocale.getDefault(), str);
    }

    /**
     * Returns the lowercase version of the argument string.
     * Casing is dependent on the default locale and context-sensitive
     * @param str source string to be performed on
     * @return lowercase version of the argument string
     * @stable ICU 2.1
     */
    public static String toLowerCase(String str)
    {
        return toLowerCase(ULocale.getDefault(), str);
    }

    /**
     * <p>Returns the titlecase version of the argument string.</p>
     * <p>Position for titlecasing is determined by the argument break
     * iterator, hence the user can customize his break iterator for
     * a specialized titlecasing. In this case only the forward iteration
     * needs to be implemented.
     * If the break iterator passed in is null, the default Unicode algorithm
     * will be used to determine the titlecase positions.
     * </p>
     * <p>Only positions returned by the break iterator will be title cased,
     * character in between the positions will all be in lower case.</p>
     * <p>Casing is dependent on the default locale and context-sensitive</p>
     * @param str source string to be performed on
     * @param breakiter break iterator to determine the positions in which
     *        the character should be title cased.
     * @return lowercase version of the argument string
     * @stable ICU 2.6
     */
    public static String toTitleCase(String str, BreakIterator breakiter)
    {
        return toTitleCase(ULocale.getDefault(), str, breakiter);
    }

    /**
     * Returns the uppercase version of the argument string.
     * Casing is dependent on the argument locale and context-sensitive.
     * @param locale which string is to be converted in
     * @param str source string to be performed on
     * @return uppercase version of the argument string
     * @stable ICU 2.1
     */
    public static String toUpperCase(Locale locale, String str)
    {
        return toUpperCase(ULocale.forLocale(locale), str);
    }

    /**
     * Returns the uppercase version of the argument string.
     * Casing is dependent on the argument locale and context-sensitive.
     * @param locale which string is to be converted in
     * @param str source string to be performed on
     * @return uppercase version of the argument string
     * @stable ICU 3.2
     */
    public static String toUpperCase(ULocale locale, String str) {
        StringContextIterator iter = new StringContextIterator(str);
        StringBuffer result = new StringBuffer(str.length());
        int[] locCache = new int[1];
        int c;

        if (locale == null) {
            locale = ULocale.getDefault();
        }
        locCache[0]=0;

        while((c=iter.nextCaseMapCP())>=0) {
            c = UCaseProps.INSTANCE.toFullUpper(c, iter, result, locale, locCache);

            /* decode the result */
            if(c<0) {
                /* (not) original code point */
                c=~c;
            } else if(c<=UCaseProps.MAX_STRING_LENGTH) {
                /* mapping already appended to result */
                continue;
            /* } else { append single-code point mapping */
            }
            if(c<=0xffff) {
                result.append((char)c);
            } else {
                UTF16.append(result, c);
            }
        }
        return result.toString();
    }

    /**
     * Returns the lowercase version of the argument string.
     * Casing is dependent on the argument locale and context-sensitive
     * @param locale which string is to be converted in
     * @param str source string to be performed on
     * @return lowercase version of the argument string
     * @stable ICU 2.1
     */
    public static String toLowerCase(Locale locale, String str)
    {
        return toLowerCase(ULocale.forLocale(locale), str);
    }

    /**
     * Returns the lowercase version of the argument string.
     * Casing is dependent on the argument locale and context-sensitive
     * @param locale which string is to be converted in
     * @param str source string to be performed on
     * @return lowercase version of the argument string
     * @stable ICU 3.2
     */
    public static String toLowerCase(ULocale locale, String str) {
        StringContextIterator iter = new StringContextIterator(str);
        StringBuffer result = new StringBuffer(str.length());
        int[] locCache = new int[1];
        int c;

        if (locale == null) {
            locale = ULocale.getDefault();
        }
        locCache[0]=0;

        while((c=iter.nextCaseMapCP())>=0) {
            c = UCaseProps.INSTANCE.toFullLower(c, iter, result, locale, locCache);

            /* decode the result */
            if(c<0) {
                /* (not) original code point */
                c=~c;
            } else if(c<=UCaseProps.MAX_STRING_LENGTH) {
                /* mapping already appended to result */
                continue;
            /* } else { append single-code point mapping */
            }
            if(c<=0xffff) {
                result.append((char)c);
            } else {
                UTF16.append(result, c);
            }
        }
        return result.toString();
    }

    /**
     * <p>Returns the titlecase version of the argument string.</p>
     * <p>Position for titlecasing is determined by the argument break
     * iterator, hence the user can customize his break iterator for
     * a specialized titlecasing. In this case only the forward iteration
     * needs to be implemented.
     * If the break iterator passed in is null, the default Unicode algorithm
     * will be used to determine the titlecase positions.
     * </p>
     * <p>Only positions returned by the break iterator will be title cased,
     * character in between the positions will all be in lower case.</p>
     * <p>Casing is dependent on the argument locale and context-sensitive</p>
     * @param locale which string is to be converted in
     * @param str source string to be performed on
     * @param breakiter break iterator to determine the positions in which
     *        the character should be title cased.
     * @return lowercase version of the argument string
     * @stable ICU 2.6
     */
    public static String toTitleCase(Locale locale, String str,
                                     BreakIterator breakiter)
    {
        return toTitleCase(ULocale.forLocale(locale), str, breakiter);
    }

    /**
     * <p>Returns the titlecase version of the argument string.</p>
     * <p>Position for titlecasing is determined by the argument break
     * iterator, hence the user can customize his break iterator for
     * a specialized titlecasing. In this case only the forward iteration
     * needs to be implemented.
     * If the break iterator passed in is null, the default Unicode algorithm
     * will be used to determine the titlecase positions.
     * </p>
     * <p>Only positions returned by the break iterator will be title cased,
     * character in between the positions will all be in lower case.</p>
     * <p>Casing is dependent on the argument locale and context-sensitive</p>
     * @param locale which string is to be converted in
     * @param str source string to be performed on
     * @param titleIter break iterator to determine the positions in which
     *        the character should be title cased.
     * @return lowercase version of the argument string
     * @stable ICU 3.2
     */
    public static String toTitleCase(ULocale locale, String str,
                                     BreakIterator titleIter) {
        return toTitleCase(locale, str, titleIter, 0);
    }

    /**
     * <p>Returns the titlecase version of the argument string.</p>
     * <p>Position for titlecasing is determined by the argument break
     * iterator, hence the user can customize his break iterator for
     * a specialized titlecasing. In this case only the forward iteration
     * needs to be implemented.
     * If the break iterator passed in is null, the default Unicode algorithm
     * will be used to determine the titlecase positions.
     * </p>
     * <p>Only positions returned by the break iterator will be title cased,
     * character in between the positions will all be in lower case.</p>
     * <p>Casing is dependent on the argument locale and context-sensitive</p>
     * @param locale which string is to be converted in
     * @param str source string to be performed on
     * @param titleIter break iterator to determine the positions in which
     *        the character should be title cased.
     * @param options bit set to modify the titlecasing operation
     * @return lowercase version of the argument string
     * @stable ICU 3.8
     * @see #TITLECASE_NO_LOWERCASE
     * @see #TITLECASE_NO_BREAK_ADJUSTMENT
     */
    public static String toTitleCase(ULocale locale, String str,
                                     BreakIterator titleIter,
                                     int options) {
        StringContextIterator iter = new StringContextIterator(str);
        StringBuffer result = new StringBuffer(str.length());
        int[] locCache = new int[1];
        int c, nc, srcLength = str.length();

        if (locale == null) {
            locale = ULocale.getDefault();
        }
        locCache[0]=0;

        if(titleIter == null) {
            titleIter = BreakIterator.getWordInstance(locale);
        }
        titleIter.setText(str);

        int prev, titleStart, index;
        boolean isFirstIndex;
        boolean isDutch = locale.getLanguage().equals("nl");
        boolean FirstIJ = true;

        /* set up local variables */
        prev=0;
        isFirstIndex=true;

        /* titlecasing loop */
        while(prev<srcLength) {
            /* find next index where to titlecase */
            if(isFirstIndex) {
                isFirstIndex=false;
                index=titleIter.first();
            } else {
                index=titleIter.next();
            }
            if(index==BreakIterator.DONE || index>srcLength) {
                index=srcLength;
            }

            /*
             * Unicode 4 & 5 section 3.13 Default Case Operations:
             *
             * R3  toTitlecase(X): Find the word boundaries based on Unicode Standard Annex
             * #29, "Text Boundaries." Between each pair of word boundaries, find the first
             * cased character F. If F exists, map F to default_title(F); then map each
             * subsequent character C to default_lower(C).
             *
             * In this implementation, segment [prev..index[ into 3 parts:
             * a) uncased characters (copy as-is) [prev..titleStart[
             * b) first case letter (titlecase)         [titleStart..titleLimit[
             * c) subsequent characters (lowercase)                 [titleLimit..index[
             */
            if(prev<index) {
                /* find and copy uncased characters [prev..titleStart[ */
                iter.setLimit(index);
                c=iter.nextCaseMapCP();
                if((options&TITLECASE_NO_BREAK_ADJUSTMENT)==0
                   && UCaseProps.NONE==UCaseProps.INSTANCE.getType(c)) {
                    while((c=iter.nextCaseMapCP())>=0
                          && UCaseProps.NONE==UCaseProps.INSTANCE.getType(c)) {}
                    titleStart=iter.getCPStart();
                    if(prev<titleStart) {
                        result.append(str, prev, titleStart);
                    }
                } else {
                    titleStart=prev;
                }

                if(titleStart<index) {
                    FirstIJ = true;
                    /* titlecase c which is from titleStart */
                    c = UCaseProps.INSTANCE.toFullTitle(c, iter, result, locale, locCache);

                    /* decode the result and lowercase up to index */
                    for(;;) {
                        if(c<0) {
                            /* (not) original code point */
                            c=~c;
                            if(c<=0xffff) {
                                result.append((char)c);
                            } else {
                                UTF16.append(result, c);
                            }
                        } else if(c<=UCaseProps.MAX_STRING_LENGTH) {
                            /* mapping already appended to result */
                        } else {
                            /* append single-code point mapping */
                            if(c<=0xffff) {
                                result.append((char)c);
                            } else {
                                UTF16.append(result, c);
                            }
                        }

                        if((options&TITLECASE_NO_LOWERCASE)!=0) {
                            /* Optionally just copy the rest of the word unchanged. */

                            int titleLimit=iter.getCPLimit();
                            if(titleLimit<index) {
                              // TODO: With Java 5, this would want to be
                              // result.append(str, titleLimit, index);
                                String appendStr = str.substring(titleLimit,index);
                                /* Special Case - Dutch IJ Titlecasing */
                                if ( isDutch && c == 0x0049 && appendStr.startsWith("j")) {
                                   appendStr = "J" + appendStr.substring(1);
                                }
                                result.append(appendStr);
                            }
                            iter.moveToLimit();
                            break;
                        } else if((nc=iter.nextCaseMapCP())>=0) {
                            if (isDutch && (nc == 0x004A ||  nc == 0x006A)
                                && (c == 0x0049) && (FirstIJ == true)) {
                                c = 0x004A; /* J */
                                FirstIJ = false;
                            } else {
                                /* Normal operation: Lowercase the rest of the word. */
                                c = UCaseProps.INSTANCE.toFullLower(nc, iter, result, locale,
                                                                    locCache);
                            }
                        } else {
                            break;
                        }
                    }
                }
            }

            prev=index;
        }
        return result.toString();
    }

    /**
     * {@icu} The given character is mapped to its case folding equivalent according
     * to UnicodeData.txt and CaseFolding.txt; if the character has no case
     * folding equivalent, the character itself is returned.
     *
     * <p>This function only returns the simple, single-code point case mapping.
     * Full case mappings should be used whenever possible because they produce
     * better results by working on whole strings.
     * They can map to a result string with a different length as appropriate.
     * Full case mappings are applied by the case mapping functions
     * that take String parameters rather than code points (int).
     * See also the User Guide chapter on C/POSIX migration:
     * http://www.icu-project.org/userguide/posix.html#case_mappings
     *
     * @param ch             the character to be converted
     * @param defaultmapping Indicates if all mappings defined in
     *                       CaseFolding.txt is to be used, otherwise the
     *                       mappings for dotted I  and dotless i marked with
     *                       'I' in CaseFolding.txt will be skipped.
     * @return               the case folding equivalent of the character, if
     *                       any; otherwise the character itself.
     * @see                  #foldCase(String, boolean)
     * @stable ICU 2.1
     */
    public static int foldCase(int ch, boolean defaultmapping) {
        return foldCase(ch, defaultmapping ? FOLD_CASE_DEFAULT : FOLD_CASE_EXCLUDE_SPECIAL_I);
    }

    /**
     * {@icu} The given string is mapped to its case folding equivalent according to
     * UnicodeData.txt and CaseFolding.txt; if any character has no case
     * folding equivalent, the character itself is returned.
     * "Full", multiple-code point case folding mappings are returned here.
     * For "simple" single-code point mappings use the API
     * foldCase(int ch, boolean defaultmapping).
     * @param str            the String to be converted
     * @param defaultmapping Indicates if all mappings defined in
     *                       CaseFolding.txt is to be used, otherwise the
     *                       mappings for dotted I and dotless i marked with
     *                       'I' in CaseFolding.txt will be skipped.
     * @return               the case folding equivalent of the character, if
     *                       any; otherwise the character itself.
     * @see                  #foldCase(int, boolean)
     * @stable ICU 2.1
     */
    public static String foldCase(String str, boolean defaultmapping) {
        return foldCase(str, defaultmapping ? FOLD_CASE_DEFAULT : FOLD_CASE_EXCLUDE_SPECIAL_I);
    }

    /**
     * {@icu} Option value for case folding: use default mappings defined in
     * CaseFolding.txt.
     * @stable ICU 2.6
     */
    public static final int FOLD_CASE_DEFAULT    =      0x0000;
    /**
     * {@icu} Option value for case folding: exclude the mappings for dotted I
     * and dotless i marked with 'I' in CaseFolding.txt.
     * @stable ICU 2.6
     */
    public static final int FOLD_CASE_EXCLUDE_SPECIAL_I = 0x0001;

    /**
     * {@icu} The given character is mapped to its case folding equivalent according
     * to UnicodeData.txt and CaseFolding.txt; if the character has no case
     * folding equivalent, the character itself is returned.
     *
     * <p>This function only returns the simple, single-code point case mapping.
     * Full case mappings should be used whenever possible because they produce
     * better results by working on whole strings.
     * They can map to a result string with a different length as appropriate.
     * Full case mappings are applied by the case mapping functions
     * that take String parameters rather than code points (int).
     * See also the User Guide chapter on C/POSIX migration:
     * http://www.icu-project.org/userguide/posix.html#case_mappings
     *
     * @param ch the character to be converted
     * @param options A bit set for special processing. Currently the recognised options
     * are FOLD_CASE_EXCLUDE_SPECIAL_I and FOLD_CASE_DEFAULT
     * @return the case folding equivalent of the character, if any; otherwise the
     * character itself.
     * @see #foldCase(String, boolean)
     * @stable ICU 2.6
     */
    public static int foldCase(int ch, int options) {
        return UCaseProps.INSTANCE.fold(ch, options);
    }

    /**
     * {@icu} The given string is mapped to its case folding equivalent according to
     * UnicodeData.txt and CaseFolding.txt; if any character has no case
     * folding equivalent, the character itself is returned.
     * "Full", multiple-code point case folding mappings are returned here.
     * For "simple" single-code point mappings use the API
     * foldCase(int ch, boolean defaultmapping).
     * @param str the String to be converted
     * @param options A bit set for special processing. Currently the recognised options
     *                are FOLD_CASE_EXCLUDE_SPECIAL_I and FOLD_CASE_DEFAULT
     * @return the case folding equivalent of the character, if any; otherwise the
     *         character itself.
     * @see #foldCase(int, boolean)
     * @stable ICU 2.6
     */
    public static final String foldCase(String str, int options) {
        StringBuffer result = new StringBuffer(str.length());
        int c, i, length;

        length = str.length();
        for(i=0; i<length;) {
            c=UTF16.charAt(str, i);
            i+=UTF16.getCharCount(c);
            c = UCaseProps.INSTANCE.toFullFolding(c, result, options);

            /* decode the result */
            if(c<0) {
                /* (not) original code point */
                c=~c;
            } else if(c<=UCaseProps.MAX_STRING_LENGTH) {
                /* mapping already appended to result */
                continue;
            /* } else { append single-code point mapping */
            }
            if(c<=0xffff) {
                result.append((char)c);
            } else {
                UTF16.append(result, c);
            }
        }
        return result.toString();
    }

    /**
     * {@icu} Return numeric value of Han code points.
     * <br> This returns the value of Han 'numeric' code points,
     * including those for zero, ten, hundred, thousand, ten thousand,
     * and hundred million.
     * This includes both the standard and 'checkwriting'
     * characters, the 'big circle' zero character, and the standard
     * zero character.
     * @param ch code point to query
     * @return value if it is a Han 'numeric character,' otherwise return -1.
     * @stable ICU 2.4
     */
    public static int getHanNumericValue(int ch)
    {
        // TODO: Are these all covered by Unicode numeric value data?
        switch(ch)
        {
        case IDEOGRAPHIC_NUMBER_ZERO_ :
        case CJK_IDEOGRAPH_COMPLEX_ZERO_ :
        return 0; // Han Zero
        case CJK_IDEOGRAPH_FIRST_ :
        case CJK_IDEOGRAPH_COMPLEX_ONE_ :
        return 1; // Han One
        case CJK_IDEOGRAPH_SECOND_ :
        case CJK_IDEOGRAPH_COMPLEX_TWO_ :
        return 2; // Han Two
        case CJK_IDEOGRAPH_THIRD_ :
        case CJK_IDEOGRAPH_COMPLEX_THREE_ :
        return 3; // Han Three
        case CJK_IDEOGRAPH_FOURTH_ :
        case CJK_IDEOGRAPH_COMPLEX_FOUR_ :
        return 4; // Han Four
        case CJK_IDEOGRAPH_FIFTH_ :
        case CJK_IDEOGRAPH_COMPLEX_FIVE_ :
        return 5; // Han Five
        case CJK_IDEOGRAPH_SIXTH_ :
        case CJK_IDEOGRAPH_COMPLEX_SIX_ :
        return 6; // Han Six
        case CJK_IDEOGRAPH_SEVENTH_ :
        case CJK_IDEOGRAPH_COMPLEX_SEVEN_ :
        return 7; // Han Seven
        case CJK_IDEOGRAPH_EIGHTH_ :
        case CJK_IDEOGRAPH_COMPLEX_EIGHT_ :
        return 8; // Han Eight
        case CJK_IDEOGRAPH_NINETH_ :
        case CJK_IDEOGRAPH_COMPLEX_NINE_ :
        return 9; // Han Nine
        case CJK_IDEOGRAPH_TEN_ :
        case CJK_IDEOGRAPH_COMPLEX_TEN_ :
        return 10;
        case CJK_IDEOGRAPH_HUNDRED_ :
        case CJK_IDEOGRAPH_COMPLEX_HUNDRED_ :
        return 100;
        case CJK_IDEOGRAPH_THOUSAND_ :
        case CJK_IDEOGRAPH_COMPLEX_THOUSAND_ :
        return 1000;
        case CJK_IDEOGRAPH_TEN_THOUSAND_ :
        return 10000;
        case CJK_IDEOGRAPH_HUNDRED_MILLION_ :
        return 100000000;
        }
        return -1; // no value
    }

    /**
     * {@icu} <p>Returns an iterator for character types, iterating over codepoints.</p>
     * Example of use:<br>
     * <pre>
     * RangeValueIterator iterator = UCharacter.getTypeIterator();
     * RangeValueIterator.Element element = new RangeValueIterator.Element();
     * while (iterator.next(element)) {
     *     System.out.println("Codepoint \\u" +
     *                        Integer.toHexString(element.start) +
     *                        " to codepoint \\u" +
     *                        Integer.toHexString(element.limit - 1) +
     *                        " has the character type " +
     *                        element.value);
     * }
     * </pre>
     * @return an iterator
     * @stable ICU 2.6
     */
    public static RangeValueIterator getTypeIterator()
    {
        return new UCharacterTypeIterator();
    }

    private static final class UCharacterTypeIterator implements RangeValueIterator {
        UCharacterTypeIterator() {
            reset();
        }

        // implements RangeValueIterator
        public boolean next(Element element) {
            if(trieIterator.hasNext() && !(range=trieIterator.next()).leadSurrogate) {
                element.start=range.startCodePoint;
                element.limit=range.endCodePoint+1;
                element.value=range.value;
                return true;
            } else {
                return false;
            }
        }

        // implements RangeValueIterator
        public void reset() {
            trieIterator=UCharacterProperty.INSTANCE.m_trie_.iterator(MASK_TYPE);
        }

        private Iterator<Trie2.Range> trieIterator;
        private Trie2.Range range;

        private static final class MaskType implements Trie2.ValueMapper {
            // Extracts the general category ("character type") from the trie value.
            public int map(int value) {
                return value & UCharacterProperty.TYPE_MASK;
            }
        }
        private static final MaskType MASK_TYPE=new MaskType();
    }

    /**
     * {@icu} <p>Returns an iterator for character names, iterating over codepoints.</p>
     * <p>This API only gets the iterator for the modern, most up-to-date
     * Unicode names. For older 1.0 Unicode names use get1_0NameIterator() or
     * for extended names use getExtendedNameIterator().</p>
     * Example of use:<br>
     * <pre>
     * ValueIterator iterator = UCharacter.getNameIterator();
     * ValueIterator.Element element = new ValueIterator.Element();
     * while (iterator.next(element)) {
     *     System.out.println("Codepoint \\u" +
     *                        Integer.toHexString(element.codepoint) +
     *                        " has the name " + (String)element.value);
     * }
     * </pre>
     * <p>The maximal range which the name iterator iterates is from
     * UCharacter.MIN_VALUE to UCharacter.MAX_VALUE.</p>
     * @return an iterator
     * @stable ICU 2.6
     */
    public static ValueIterator getNameIterator(){
        return new UCharacterNameIterator(UCharacterName.INSTANCE,
                      UCharacterNameChoice.UNICODE_CHAR_NAME);
    }

    /**
     * {@icu} <p>Returns an iterator for character names, iterating over codepoints.</p>
     * <p>This API only gets the iterator for the older 1.0 Unicode names.
     * For modern, most up-to-date Unicode names use getNameIterator() or
     * for extended names use getExtendedNameIterator().</p>
     * Example of use:<br>
     * <pre>
     * ValueIterator iterator = UCharacter.get1_0NameIterator();
     * ValueIterator.Element element = new ValueIterator.Element();
     * while (iterator.next(element)) {
     *     System.out.println("Codepoint \\u" +
     *                        Integer.toHexString(element.codepoint) +
     *                        " has the name " + (String)element.value);
     * }
     * </pre>
     * <p>The maximal range which the name iterator iterates is from
     * @return an iterator
     * @stable ICU 2.6
     */
    public static ValueIterator getName1_0Iterator(){
        return new UCharacterNameIterator(UCharacterName.INSTANCE,
                      UCharacterNameChoice.UNICODE_10_CHAR_NAME);
    }

    /**
     * {@icu} <p>Returns an iterator for character names, iterating over codepoints.</p>
     * <p>This API only gets the iterator for the extended names.
     * For modern, most up-to-date Unicode names use getNameIterator() or
     * for older 1.0 Unicode names use get1_0NameIterator().</p>
     * Example of use:<br>
     * <pre>
     * ValueIterator iterator = UCharacter.getExtendedNameIterator();
     * ValueIterator.Element element = new ValueIterator.Element();
     * while (iterator.next(element)) {
     *     System.out.println("Codepoint \\u" +
     *                        Integer.toHexString(element.codepoint) +
     *                        " has the name " + (String)element.value);
     * }
     * </pre>
     * <p>The maximal range which the name iterator iterates is from
     * @return an iterator
     * @stable ICU 2.6
     */
    public static ValueIterator getExtendedNameIterator(){
        return new UCharacterNameIterator(UCharacterName.INSTANCE,
                      UCharacterNameChoice.EXTENDED_CHAR_NAME);
    }

    /**
     * {@icu} Returns the "age" of the code point.</p>
     * <p>The "age" is the Unicode version when the code point was first
     * designated (as a non-character or for Private Use) or assigned a
     * character.
     * <p>This can be useful to avoid emitting code points to receiving
     * processes that do not accept newer characters.</p>
     * <p>The data is from the UCD file DerivedAge.txt.</p>
     * @param ch The code point.
     * @return the Unicode version number
     * @stable ICU 2.6
     */
    public static VersionInfo getAge(int ch)
    {
        if (ch < MIN_VALUE || ch > MAX_VALUE) {
        throw new IllegalArgumentException("Codepoint out of bounds");
        }
        return UCharacterProperty.INSTANCE.getAge(ch);
    }

    /**
     * {@icu} <p>Check a binary Unicode property for a code point.</p>
     * <p>Unicode, especially in version 3.2, defines many more properties
     * than the original set in UnicodeData.txt.</p>
     * <p>This API is intended to reflect Unicode properties as defined in
     * the Unicode Character Database (UCD) and Unicode Technical Reports
     * (UTR).</p>
     * <p>For details about the properties see
     * <a href=http://www.unicode.org/>http://www.unicode.org/</a>.</p>
     * <p>For names of Unicode properties see the UCD file
     * PropertyAliases.txt.</p>
     * <p>This API does not check the validity of the codepoint.</p>
     * <p>Important: If ICU is built with UCD files from Unicode versions
     * below 3.2, then properties marked with "new" are not or
     * not fully available.</p>
     * @param ch code point to test.
     * @param property selector constant from com.ibm.icu.lang.UProperty,
     *        identifies which binary property to check.
     * @return true or false according to the binary Unicode property value
     *         for ch. Also false if property is out of bounds or if the
     *         Unicode version does not have data for the property at all, or
     *         not for this code point.
     * @see com.ibm.icu.lang.UProperty
     * @stable ICU 2.6
     */
    public static boolean hasBinaryProperty(int ch, int property)
    {
    if (ch < MIN_VALUE || ch > MAX_VALUE) {
        throw new IllegalArgumentException("Codepoint out of bounds");
        }
        return UCharacterProperty.INSTANCE.hasBinaryProperty(ch, property);
    }

    /**
     * {@icu} <p>Check if a code point has the Alphabetic Unicode property.</p>
     * <p>Same as UCharacter.hasBinaryProperty(ch, UProperty.ALPHABETIC).</p>
     * <p>Different from UCharacter.isLetter(ch)!</p>
     * @stable ICU 2.6
     * @param ch codepoint to be tested
     */
    public static boolean isUAlphabetic(int ch)
    {
    return hasBinaryProperty(ch, UProperty.ALPHABETIC);
    }

    /**
     * {@icu} <p>Check if a code point has the Lowercase Unicode property.</p>
     * <p>Same as UCharacter.hasBinaryProperty(ch, UProperty.LOWERCASE).</p>
     * <p>This is different from UCharacter.isLowerCase(ch)!</p>
     * @param ch codepoint to be tested
     * @stable ICU 2.6
     */
    public static boolean isULowercase(int ch)
    {
    return hasBinaryProperty(ch, UProperty.LOWERCASE);
    }

    /**
     * {@icu} <p>Check if a code point has the Uppercase Unicode property.</p>
     * <p>Same as UCharacter.hasBinaryProperty(ch, UProperty.UPPERCASE).</p>
     * <p>This is different from UCharacter.isUpperCase(ch)!</p>
     * @param ch codepoint to be tested
     * @stable ICU 2.6
     */
    public static boolean isUUppercase(int ch)
    {
    return hasBinaryProperty(ch, UProperty.UPPERCASE);
    }

    /**
     * {@icu} <p>Check if a code point has the White_Space Unicode property.</p>
     * <p>Same as UCharacter.hasBinaryProperty(ch, UProperty.WHITE_SPACE).</p>
     * <p>This is different from both UCharacter.isSpace(ch) and
     * UCharacter.isWhitespace(ch)!</p>
     * @param ch codepoint to be tested
     * @stable ICU 2.6
     */
    public static boolean isUWhiteSpace(int ch)
    {
    return hasBinaryProperty(ch, UProperty.WHITE_SPACE);
    }

    /**
     * {@icu} <p>Returns the property value for an Unicode property type of a code point.
     * Also returns binary and mask property values.</p>
     * <p>Unicode, especially in version 3.2, defines many more properties than
     * the original set in UnicodeData.txt.</p>
     * <p>The properties APIs are intended to reflect Unicode properties as
     * defined in the Unicode Character Database (UCD) and Unicode Technical
     * Reports (UTR). For details about the properties see
     * http://www.unicode.org/.</p>
     * <p>For names of Unicode properties see the UCD file PropertyAliases.txt.
     * </p>
     * <pre>
     * Sample usage:
     * int ea = UCharacter.getIntPropertyValue(c, UProperty.EAST_ASIAN_WIDTH);
     * int ideo = UCharacter.getIntPropertyValue(c, UProperty.IDEOGRAPHIC);
     * boolean b = (ideo == 1) ? true : false;
     * </pre>
     * @param ch code point to test.
     * @param type UProperty selector constant, identifies which binary
     *        property to check. Must be
     *        UProperty.BINARY_START &lt;= type &lt; UProperty.BINARY_LIMIT or
     *        UProperty.INT_START &lt;= type &lt; UProperty.INT_LIMIT or
     *        UProperty.MASK_START &lt;= type &lt; UProperty.MASK_LIMIT.
     * @return numeric value that is directly the property value or,
     *         for enumerated properties, corresponds to the numeric value of
     *         the enumerated constant of the respective property value
     *         enumeration type (cast to enum type if necessary).
     *         Returns 0 or 1 (for false / true) for binary Unicode properties.
     *         Returns a bit-mask for mask properties.
     *         Returns 0 if 'type' is out of bounds or if the Unicode version
     *         does not have data for the property at all, or not for this code
     *         point.
     * @see UProperty
     * @see #hasBinaryProperty
     * @see #getIntPropertyMinValue
     * @see #getIntPropertyMaxValue
     * @see #getUnicodeVersion
     * @stable ICU 2.4
     */
    public static int getIntPropertyValue(int ch, int type)
    {
        return UCharacterProperty.INSTANCE.getIntPropertyValue(ch, type);
    }
    /**
     * {@icu} Returns a string version of the property value.
     * @param propertyEnum The property enum value.
     * @param codepoint The codepoint value.
     * @param nameChoice The choice of the name.
     * @return value as string
     * @internal
     * @deprecated This API is ICU internal only.
     */
    ///CLOVER:OFF
    public static String getStringPropertyValue(int propertyEnum, int codepoint, int nameChoice) {
        if ((propertyEnum >= UProperty.BINARY_START && propertyEnum < UProperty.BINARY_LIMIT) ||
                (propertyEnum >= UProperty.INT_START && propertyEnum < UProperty.INT_LIMIT)) {
            return getPropertyValueName(propertyEnum, getIntPropertyValue(codepoint, propertyEnum),
                                        nameChoice);
        }
        if (propertyEnum == UProperty.NUMERIC_VALUE) {
                return String.valueOf(getUnicodeNumericValue(codepoint));
        }
        // otherwise must be string property
        switch (propertyEnum) {
        case UProperty.AGE: return getAge(codepoint).toString();
        case UProperty.ISO_COMMENT: return getISOComment(codepoint);
        case UProperty.BIDI_MIRRORING_GLYPH: return UTF16.valueOf(getMirror(codepoint));
        case UProperty.CASE_FOLDING: return foldCase(UTF16.valueOf(codepoint), true);
        case UProperty.LOWERCASE_MAPPING: return toLowerCase(UTF16.valueOf(codepoint));
        case UProperty.NAME: return getName(codepoint);
        case UProperty.SIMPLE_CASE_FOLDING: return UTF16.valueOf(foldCase(codepoint,true));
        case UProperty.SIMPLE_LOWERCASE_MAPPING: return UTF16.valueOf(toLowerCase(codepoint));
        case UProperty.SIMPLE_TITLECASE_MAPPING: return UTF16.valueOf(toTitleCase(codepoint));
        case UProperty.SIMPLE_UPPERCASE_MAPPING: return UTF16.valueOf(toUpperCase(codepoint));
        case UProperty.TITLECASE_MAPPING: return toTitleCase(UTF16.valueOf(codepoint),null);
        case UProperty.UNICODE_1_NAME: return getName1_0(codepoint);
        case UProperty.UPPERCASE_MAPPING: return toUpperCase(UTF16.valueOf(codepoint));
        }
        throw new IllegalArgumentException("Illegal Property Enum");
    }
    ///CLOVER:ON

    /**
     * {@icu} Returns the minimum value for an integer/binary Unicode property type.
     * Can be used together with UCharacter.getIntPropertyMaxValue(int)
     * to allocate arrays of com.ibm.icu.text.UnicodeSet or similar.
     * @param type UProperty selector constant, identifies which binary
     *        property to check. Must be
     *        UProperty.BINARY_START &lt;= type &lt; UProperty.BINARY_LIMIT or
     *        UProperty.INT_START &lt;= type &lt; UProperty.INT_LIMIT.
     * @return Minimum value returned by UCharacter.getIntPropertyValue(int)
     *         for a Unicode property. 0 if the property
     *         selector 'type' is out of range.
     * @see UProperty
     * @see #hasBinaryProperty
     * @see #getUnicodeVersion
     * @see #getIntPropertyMaxValue
     * @see #getIntPropertyValue
     * @stable ICU 2.4
     */
    public static int getIntPropertyMinValue(int type){

        return 0; // undefined; and: all other properties have a minimum value of 0
    }


    /**
     * {@icu} Returns the maximum value for an integer/binary Unicode property.
     * Can be used together with UCharacter.getIntPropertyMinValue(int)
     * to allocate arrays of com.ibm.icu.text.UnicodeSet or similar.
     * Examples for min/max values (for Unicode 3.2):
     * <ul>
     * <li> UProperty.BIDI_CLASS:    0/18
     * (UCharacterDirection.LEFT_TO_RIGHT/UCharacterDirection.BOUNDARY_NEUTRAL)
     * <li> UProperty.SCRIPT:        0/45 (UScript.COMMON/UScript.TAGBANWA)
     * <li> UProperty.IDEOGRAPHIC:   0/1  (false/true)
     * </ul>
     * For undefined UProperty constant values, min/max values will be 0/-1.
     * @param type UProperty selector constant, identifies which binary
     *        property to check. Must be
     *        UProperty.BINARY_START &lt;= type &lt; UProperty.BINARY_LIMIT or
     *        UProperty.INT_START &lt;= type &lt; UProperty.INT_LIMIT.
     * @return Maximum value returned by u_getIntPropertyValue for a Unicode
     *         property. &lt;= 0 if the property selector 'type' is out of range.
     * @see UProperty
     * @see #hasBinaryProperty
     * @see #getUnicodeVersion
     * @see #getIntPropertyMaxValue
     * @see #getIntPropertyValue
     * @stable ICU 2.4
     */
    public static int getIntPropertyMaxValue(int type)
    {
        return UCharacterProperty.INSTANCE.getIntPropertyMaxValue(type);
    }

    /**
     * Provide the java.lang.Character forDigit API, for convenience.
     * @stable ICU 3.0
     */
    public static char forDigit(int digit, int radix) {
        return java.lang.Character.forDigit(digit, radix);
    }

    // JDK 1.5 API coverage

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#LEAD_SURROGATE_MIN_VALUE
     * @stable ICU 3.0
     */
    public static final char MIN_HIGH_SURROGATE = UTF16.LEAD_SURROGATE_MIN_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#LEAD_SURROGATE_MAX_VALUE
     * @stable ICU 3.0
     */
    public static final char MAX_HIGH_SURROGATE = UTF16.LEAD_SURROGATE_MAX_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#TRAIL_SURROGATE_MIN_VALUE
     * @stable ICU 3.0
     */
    public static final char MIN_LOW_SURROGATE = UTF16.TRAIL_SURROGATE_MIN_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#TRAIL_SURROGATE_MAX_VALUE
     * @stable ICU 3.0
     */
    public static final char MAX_LOW_SURROGATE = UTF16.TRAIL_SURROGATE_MAX_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#SURROGATE_MIN_VALUE
     * @stable ICU 3.0
     */
    public static final char MIN_SURROGATE = UTF16.SURROGATE_MIN_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#SURROGATE_MAX_VALUE
     * @stable ICU 3.0
     */
    public static final char MAX_SURROGATE = UTF16.SURROGATE_MAX_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#SUPPLEMENTARY_MIN_VALUE
     * @stable ICU 3.0
     */
    public static final int  MIN_SUPPLEMENTARY_CODE_POINT = UTF16.SUPPLEMENTARY_MIN_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#CODEPOINT_MAX_VALUE
     * @stable ICU 3.0
     */
    public static final int  MAX_CODE_POINT = UTF16.CODEPOINT_MAX_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @see UTF16#CODEPOINT_MIN_VALUE
     * @stable ICU 3.0
     */
    public static final int  MIN_CODE_POINT = UTF16.CODEPOINT_MIN_VALUE;

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @param cp the code point to check
     * @return true if cp is a valid code point
     * @stable ICU 3.0
     */
    public static final boolean isValidCodePoint(int cp) {
        return cp >= 0 && cp <= MAX_CODE_POINT;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @param cp the code point to check
     * @return true if cp is a supplementary code point
     * @stable ICU 3.0
     */
    public static final boolean isSupplementaryCodePoint(int cp) {
        return cp >= UTF16.SUPPLEMENTARY_MIN_VALUE
            && cp <= UTF16.CODEPOINT_MAX_VALUE;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @param ch the char to check
     * @return true if ch is a high (lead) surrogate
     * @stable ICU 3.0
     */
    public static boolean isHighSurrogate(char ch) {
        return ch >= MIN_HIGH_SURROGATE && ch <= MAX_HIGH_SURROGATE;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.
     * @param ch the char to check
     * @return true if ch is a low (trail) surrogate
     * @stable ICU 3.0
     */
    public static boolean isLowSurrogate(char ch) {
        return ch >= MIN_LOW_SURROGATE && ch <= MAX_LOW_SURROGATE;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return true if the chars
     * form a valid surrogate pair.
     * @param high the high (lead) char
     * @param low the low (trail) char
     * @return true if high, low form a surrogate pair
     * @stable ICU 3.0
     */
    public static final boolean isSurrogatePair(char high, char low) {
        return isHighSurrogate(high) && isLowSurrogate(low);
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the number of chars needed
     * to represent the code point.  This does not check the
     * code point for validity.
     * @param cp the code point to check
     * @return the number of chars needed to represent the code point
     * @see UTF16#getCharCount
     * @stable ICU 3.0
     */
    public static int charCount(int cp) {
        return UTF16.getCharCount(cp);
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the code point represented by
     * the characters.  This does not check the surrogate pair for validity.
     * @param high the high (lead) surrogate
     * @param low the low (trail) surrogate
     * @return the code point formed by the surrogate pair
     * @stable ICU 3.0
     */
    public static final int toCodePoint(char high, char low) {
        return UCharacterProperty.getRawSupplementary(high, low);
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the code point at index.
     * <br/><b>Note</b>: the semantics of this API is different from the related UTF16
     * API.  This examines only the characters at index and index+1.
     * @param seq the characters to check
     * @param index the index of the first or only char forming the code point
     * @return the code point at the index
     * @stable ICU 3.0
     */
    public static final int codePointAt(CharSequence seq, int index) {
        char c1 = seq.charAt(index++);
        if (isHighSurrogate(c1)) {
            if (index < seq.length()) {
                char c2 = seq.charAt(index);
                if (isLowSurrogate(c2)) {
                    return toCodePoint(c1, c2);
                }
            }
        }
        return c1;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the code point at index.
     * <br/><b>Note</b>: the semantics of this API is different from the related UTF16
     * API.  This examines only the characters at index and index+1.
     * @param text the characters to check
     * @param index the index of the first or only char forming the code point
     * @return the code point at the index
     * @stable ICU 3.0
     */
    public static final int codePointAt(char[] text, int index) {
        char c1 = text[index++];
        if (isHighSurrogate(c1)) {
            if (index < text.length) {
                char c2 = text[index];
                if (isLowSurrogate(c2)) {
                    return toCodePoint(c1, c2);
                }
            }
        }
        return c1;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the code point at index.
     * <br/><b>Note</b>: the semantics of this API is different from the related UTF16
     * API.  This examines only the characters at index and index+1.
     * @param text the characters to check
     * @param index the index of the first or only char forming the code point
     * @param limit the limit of the valid text
     * @return the code point at the index
     * @stable ICU 3.0
     */
    public static final int codePointAt(char[] text, int index, int limit) {
        if (index >= limit || limit > text.length) {
            throw new IndexOutOfBoundsException();
        }
        char c1 = text[index++];
        if (isHighSurrogate(c1)) {
            if (index < limit) {
                char c2 = text[index];
                if (isLowSurrogate(c2)) {
                    return toCodePoint(c1, c2);
                }
            }
        }
        return c1;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the code point before index.
     * <br/><b>Note</b>: the semantics of this API is different from the related UTF16
     * API.  This examines only the characters at index-1 and index-2.
     * @param seq the characters to check
     * @param index the index after the last or only char forming the code point
     * @return the code point before the index
     * @stable ICU 3.0
     */
    public static final int codePointBefore(CharSequence seq, int index) {
        char c2 = seq.charAt(--index);
        if (isLowSurrogate(c2)) {
            if (index > 0) {
                char c1 = seq.charAt(--index);
                if (isHighSurrogate(c1)) {
                    return toCodePoint(c1, c2);
                }
            }
        }
        return c2;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the code point before index.
     * <br/><b>Note</b>: the semantics of this API is different from the related UTF16
     * API.  This examines only the characters at index-1 and index-2.
     * @param text the characters to check
     * @param index the index after the last or only char forming the code point
     * @return the code point before the index
     * @stable ICU 3.0
     */
    public static final int codePointBefore(char[] text, int index) {
        char c2 = text[--index];
        if (isLowSurrogate(c2)) {
            if (index > 0) {
                char c1 = text[--index];
                if (isHighSurrogate(c1)) {
                    return toCodePoint(c1, c2);
                }
            }
        }
        return c2;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Return the code point before index.
     * <br/><b>Note</b>: the semantics of this API is different from the related UTF16
     * API.  This examines only the characters at index-1 and index-2.
     * @param text the characters to check
     * @param index the index after the last or only char forming the code point
     * @param limit the start of the valid text
     * @return the code point before the index
     * @stable ICU 3.0
     */
    public static final int codePointBefore(char[] text, int index, int limit) {
        if (index <= limit || limit < 0) {
            throw new IndexOutOfBoundsException();
        }
        char c2 = text[--index];
        if (isLowSurrogate(c2)) {
            if (index > limit) {
                char c1 = text[--index];
                if (isHighSurrogate(c1)) {
                    return toCodePoint(c1, c2);
                }
            }
        }
        return c2;
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Writes the chars representing the
     * code point into the destination at the given index.
     * @param cp the code point to convert
     * @param dst the destination array into which to put the char(s) representing the code point
     * @param dstIndex the index at which to put the first (or only) char
     * @return the count of the number of chars written (1 or 2)
     * @throws IllegalArgumentException if cp is not a valid code point
     * @stable ICU 3.0
     */
    public static final int toChars(int cp, char[] dst, int dstIndex) {
        if (cp >= 0) {
            if (cp < MIN_SUPPLEMENTARY_CODE_POINT) {
                dst[dstIndex] = (char)cp;
                return 1;
            }
            if (cp <= MAX_CODE_POINT) {
                dst[dstIndex] = UTF16.getLeadSurrogate(cp);
                dst[dstIndex+1] = UTF16.getTrailSurrogate(cp);
                return 2;
            }
        }
        throw new IllegalArgumentException();
    }

    /**
     * Cover the JDK 1.5 API, for convenience.  Returns a char array
     * representing the code point.
     * @param cp the code point to convert
     * @return an array containing the char(s) representing the code point
     * @throws IllegalArgumentException if cp is not a valid code point
     * @stable ICU 3.0
     */
    public static final char[] toChars(int cp) {
        if (cp >= 0) {
            if (cp < MIN_SUPPLEMENTARY_CODE_POINT) {
                return new char[] { (char)cp };
            }
            if (cp <= MAX_CODE_POINT) {
                return new char[] {
                    UTF16.getLeadSurrogate(cp),
                    UTF16.getTrailSurrogate(cp)
                };
            }
        }
        throw new IllegalArgumentException();
    }

    /**
     * Cover the JDK API, for convenience.  Return a byte representing the directionality of
     * the character.
     *
     * {@icunote} Unlike the JDK, this returns DIRECTIONALITY_LEFT_TO_RIGHT for undefined
     * or out-of-bounds characters.
     *
     * {@icunote} The return value must be tested using the constants defined in {@link
     * UCharacterEnums.ECharacterDirection} since the values are different from the ones
     * defined by <code>java.lang.Character</code>.
     * @param cp the code point to check
     * @return the directionality of the code point
     * @see #getDirection
     * @stable ICU 3.0
     */
    public static byte getDirectionality(int cp)
    {
        return (byte)getDirection(cp);
    }

    /**
     * Cover the JDK API, for convenience.  Count the number of code points in the range of text.
     * @param text the characters to check
     * @param start the start of the range
     * @param limit the limit of the range
     * @return the number of code points in the range
     * @stable ICU 3.0
     */
    public static int codePointCount(CharSequence text, int start, int limit) {
        if (start < 0 || limit < start || limit > text.length()) {
            throw new IndexOutOfBoundsException("start (" + start +
                ") or limit (" + limit +
                ") invalid or out of range 0, " + text.length());
        }

        int len = limit - start;
        while (limit > start) {
            char ch = text.charAt(--limit);
            while (ch >= MIN_LOW_SURROGATE && ch <= MAX_LOW_SURROGATE && limit > start) {
                ch = text.charAt(--limit);
                if (ch >= MIN_HIGH_SURROGATE && ch <= MAX_HIGH_SURROGATE) {
                    --len;
                    break;
                }
            }
        }
        return len;
    }

    /**
     * Cover the JDK API, for convenience.  Count the number of code points in the range of text.
     * @param text the characters to check
     * @param start the start of the range
     * @param limit the limit of the range
     * @return the number of code points in the range
     * @stable ICU 3.0
     */
    public static int codePointCount(char[] text, int start, int limit) {
        if (start < 0 || limit < start || limit > text.length) {
            throw new IndexOutOfBoundsException("start (" + start +
                                                ") or limit (" + limit +
                                                ") invalid or out of range 0, " + text.length);
        }

        int len = limit - start;
        while (limit > start) {
            char ch = text[--limit];
            while (ch >= MIN_LOW_SURROGATE && ch <= MAX_LOW_SURROGATE && limit > start) {
                ch = text[--limit];
                if (ch >= MIN_HIGH_SURROGATE && ch <= MAX_HIGH_SURROGATE) {
                    --len;
                    break;
                }
            }
        }
        return len;
    }

    /**
     * Cover the JDK API, for convenience.  Adjust the char index by a code point offset.
     * @param text the characters to check
     * @param index the index to adjust
     * @param codePointOffset the number of code points by which to offset the index
     * @return the adjusted index
     * @stable ICU 3.0
     */
    public static int offsetByCodePoints(CharSequence text, int index, int codePointOffset) {
        if (index < 0 || index > text.length()) {
            throw new IndexOutOfBoundsException("index ( " + index +
                                                ") out of range 0, " + text.length());
        }

        if (codePointOffset < 0) {
            while (++codePointOffset <= 0) {
                char ch = text.charAt(--index);
                while (ch >= MIN_LOW_SURROGATE && ch <= MAX_LOW_SURROGATE && index > 0) {
                    ch = text.charAt(--index);
                    if (ch < MIN_HIGH_SURROGATE || ch > MAX_HIGH_SURROGATE) {
                        if (++codePointOffset > 0) {
                            return index+1;
                        }
                    }
                }
            }
        } else {
            int limit = text.length();
            while (--codePointOffset >= 0) {
                char ch = text.charAt(index++);
                while (ch >= MIN_HIGH_SURROGATE && ch <= MAX_HIGH_SURROGATE && index < limit) {
                    ch = text.charAt(index++);
                    if (ch < MIN_LOW_SURROGATE || ch > MAX_LOW_SURROGATE) {
                        if (--codePointOffset < 0) {
                            return index-1;
                        }
                    }
                }
            }
        }

        return index;
    }

    /**
     * Cover the JDK API, for convenience.  Adjust the char index by a code point offset.
     * @param text the characters to check
     * @param start the start of the range to check
     * @param count the length of the range to check
     * @param index the index to adjust
     * @param codePointOffset the number of code points by which to offset the index
     * @return the adjusted index
     * @stable ICU 3.0
     */
    public static int offsetByCodePoints(char[] text, int start, int count, int index,
                                         int codePointOffset) {
        int limit = start + count;
        if (start < 0 || limit < start || limit > text.length || index < start || index > limit) {
            throw new IndexOutOfBoundsException("index ( " + index +
                                                ") out of range " + start +
                                                ", " + limit +
                                                " in array 0, " + text.length);
        }

        if (codePointOffset < 0) {
            while (++codePointOffset <= 0) {
                char ch = text[--index];
                if (index < start) {
                    throw new IndexOutOfBoundsException("index ( " + index +
                                                        ") < start (" + start +
                                                        ")");
                }
                while (ch >= MIN_LOW_SURROGATE && ch <= MAX_LOW_SURROGATE && index > start) {
                    ch = text[--index];
                    if (ch < MIN_HIGH_SURROGATE || ch > MAX_HIGH_SURROGATE) {
                        if (++codePointOffset > 0) {
                            return index+1;
                        }
                    }
                }
            }
        } else {
            while (--codePointOffset >= 0) {
                char ch = text[index++];
                if (index > limit) {
                    throw new IndexOutOfBoundsException("index ( " + index +
                                                        ") > limit (" + limit +
                                                        ")");
                }
                while (ch >= MIN_HIGH_SURROGATE && ch <= MAX_HIGH_SURROGATE && index < limit) {
                    ch = text[index++];
                    if (ch < MIN_LOW_SURROGATE || ch > MAX_LOW_SURROGATE) {
                        if (--codePointOffset < 0) {
                            return index-1;
                        }
                    }
                }
            }
        }

        return index;
    }

    // private variables -------------------------------------------------

    /**
     * To get the last character out from a data type
     */
    private static final int LAST_CHAR_MASK_ = 0xFFFF;

//    /**
//     * To get the last byte out from a data type
//     */
//    private static final int LAST_BYTE_MASK_ = 0xFF;
//
//    /**
//     * Shift 16 bits
//     */
//    private static final int SHIFT_16_ = 16;
//
//    /**
//     * Shift 24 bits
//     */
//    private static final int SHIFT_24_ = 24;
//
//    /**
//     * Decimal radix
//     */
//    private static final int DECIMAL_RADIX_ = 10;

    /**
     * No break space code point
     */
    private static final int NO_BREAK_SPACE_ = 0xA0;

    /**
     * Figure space code point
     */
    private static final int FIGURE_SPACE_ = 0x2007;

    /**
     * Narrow no break space code point
     */
    private static final int NARROW_NO_BREAK_SPACE_ = 0x202F;

    /**
     * Ideographic number zero code point
     */
    private static final int IDEOGRAPHIC_NUMBER_ZERO_ = 0x3007;

    /**
     * CJK Ideograph, First code point
     */
    private static final int CJK_IDEOGRAPH_FIRST_ = 0x4e00;

    /**
     * CJK Ideograph, Second code point
     */
    private static final int CJK_IDEOGRAPH_SECOND_ = 0x4e8c;

    /**
     * CJK Ideograph, Third code point
     */
    private static final int CJK_IDEOGRAPH_THIRD_ = 0x4e09;

    /**
     * CJK Ideograph, Fourth code point
     */
    private static final int CJK_IDEOGRAPH_FOURTH_ = 0x56d8;

    /**
     * CJK Ideograph, FIFTH code point
     */
    private static final int CJK_IDEOGRAPH_FIFTH_ = 0x4e94;

    /**
     * CJK Ideograph, Sixth code point
     */
    private static final int CJK_IDEOGRAPH_SIXTH_ = 0x516d;

    /**
     * CJK Ideograph, Seventh code point
     */
    private static final int CJK_IDEOGRAPH_SEVENTH_ = 0x4e03;

    /**
     * CJK Ideograph, Eighth code point
     */
    private static final int CJK_IDEOGRAPH_EIGHTH_ = 0x516b;

    /**
     * CJK Ideograph, Nineth code point
     */
    private static final int CJK_IDEOGRAPH_NINETH_ = 0x4e5d;

    /**
     * Application Program command code point
     */
    private static final int APPLICATION_PROGRAM_COMMAND_ = 0x009F;

    /**
     * Unit separator code point
     */
    private static final int UNIT_SEPARATOR_ = 0x001F;

    /**
     * Delete code point
     */
    private static final int DELETE_ = 0x007F;

    /**
     * Han digit characters
     */
    private static final int CJK_IDEOGRAPH_COMPLEX_ZERO_     = 0x96f6;
    private static final int CJK_IDEOGRAPH_COMPLEX_ONE_      = 0x58f9;
    private static final int CJK_IDEOGRAPH_COMPLEX_TWO_      = 0x8cb3;
    private static final int CJK_IDEOGRAPH_COMPLEX_THREE_    = 0x53c3;
    private static final int CJK_IDEOGRAPH_COMPLEX_FOUR_     = 0x8086;
    private static final int CJK_IDEOGRAPH_COMPLEX_FIVE_     = 0x4f0d;
    private static final int CJK_IDEOGRAPH_COMPLEX_SIX_      = 0x9678;
    private static final int CJK_IDEOGRAPH_COMPLEX_SEVEN_    = 0x67d2;
    private static final int CJK_IDEOGRAPH_COMPLEX_EIGHT_    = 0x634c;
    private static final int CJK_IDEOGRAPH_COMPLEX_NINE_     = 0x7396;
    private static final int CJK_IDEOGRAPH_TEN_              = 0x5341;
    private static final int CJK_IDEOGRAPH_COMPLEX_TEN_      = 0x62fe;
    private static final int CJK_IDEOGRAPH_HUNDRED_          = 0x767e;
    private static final int CJK_IDEOGRAPH_COMPLEX_HUNDRED_  = 0x4f70;
    private static final int CJK_IDEOGRAPH_THOUSAND_         = 0x5343;
    private static final int CJK_IDEOGRAPH_COMPLEX_THOUSAND_ = 0x4edf;
    private static final int CJK_IDEOGRAPH_TEN_THOUSAND_     = 0x824c;
    private static final int CJK_IDEOGRAPH_HUNDRED_MILLION_  = 0x5104;

    // private constructor -----------------------------------------------
    ///CLOVER:OFF
    /**
     * Private constructor to prevent instantiation
     */
    private UCharacter()
    {
    }
    ///CLOVER:ON
}
