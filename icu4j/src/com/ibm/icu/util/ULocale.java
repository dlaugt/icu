/*
******************************************************************************
* Copyright (C) 2003, International Business Machines Corporation and        *
* others. All Rights Reserved.                                               *
******************************************************************************
*/

package com.ibm.icu.util;

import java.io.Serializable;
import java.io.IOException;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.Locale;
import java.util.Map;
import java.util.MissingResourceException;
import java.util.Set;
import java.util.TreeMap;

import com.ibm.icu.impl.ICUResourceBundle;

/**
 * A class is analogous to {@link java.util.Locale} and provides additional
 * support for ICU protocol.  In ICU 3.0 this class is enhanced to support
 * RFC 3066 language identifiers.
 *
 * <p>Many classes and services in ICU follow a factory idiom, in which a
 * factory method or object responds to a client request with an
 * object.  The request includes a locale (the <i>requested</i>
 * locale), and the returned object is constructed using data for that
 * locale.  The system may lack data for the requested locale, in
 * which case the locale fallback mechanism will be invoked until a
 * populated locale is found (the <i>valid</i> locale).  Furthermore,
 * even when a valid locale is found, further fallback may be required
 * to reach a locale containing the specific data required by the
 * service (the <i>actual</i> locale).
 *
 * <p>This class provides selectors {@link #VALID_LOCALE} and {@link
 * #ACTUAL_LOCALE} intended for use in methods named
 * <tt>getLocale()</tt>.  These methods exist in several ICU classes,
 * including {@link com.ibm.icu.util.Calendar}, {@link
 * com.ibm.icu.util.Currency}, {@link com.ibm.icu.text.UFormat},
 * {@link com.ibm.icu.text.BreakIterator}, {@link
 * com.ibm.icu.text.Collator}, {@link
 * com.ibm.icu.text.DateFormatSymbols}, and {@link
 * com.ibm.icu.text.DecimalFormatSymbols} and their subclasses, if
 * any.  Once an object of one of these classes has been created,
 * <tt>getLocale()</tt> may be called on it to determine the valid and
 * actual locale arrived at during the object's construction.
 *
 * <p>Note: The <tt>getLocale()</tt> method will be implemented in ICU
 * 3.0; ICU 2.8 contains a partial preview implementation.  The
 * <i>actual</i> locale is returned correctly, but the <i>valid</i>
 * locale is not, in most cases.
 *
 * @see java.util.Locale
 * @author weiv
 * @author Alan Liu
 * @author Ram Viswanadha
 * @draft ICU 2.8
 */
public final class ULocale implements Serializable {
    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale ENGLISH = new ULocale("en", Locale.ENGLISH);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale FRENCH = new ULocale("fr", Locale.FRENCH);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale GERMAN = new ULocale("de", Locale.GERMAN);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale ITALIAN = new ULocale("it", Locale.ITALIAN);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale JAPANESE = new ULocale("ja", Locale.JAPANESE);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale KOREAN = new ULocale("ko", Locale.KOREAN);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale CHINESE = new ULocale("zh", Locale.CHINESE);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale SIMPLIFIED_CHINESE = new ULocale("zh_Hans", Locale.CHINESE);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale TRADITIONAL_CHINESE = new ULocale("zh_Hant", Locale.CHINESE);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale FRANCE = new ULocale("fr_FR", Locale.FRANCE);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale GERMANY = new ULocale("de_DE", Locale.GERMANY);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale ITALY = new ULocale("it_IT", Locale.ITALY);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale JAPAN = new ULocale("ja_JP", Locale.JAPAN);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale KOREA = new ULocale("ko_KR", Locale.KOREA);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale CHINA = new ULocale("zh_Hans_CN", Locale.CHINA);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale PRC = CHINA;

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale TAIWAN = new ULocale("zh_Hant_TW", Locale.TAIWAN);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale UK = new ULocale("en_GB", Locale.UK);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale US = new ULocale("en_US", Locale.US);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale CANADA = new ULocale("en_CA", Locale.CANADA);

    /** 
     * Useful constant for language.
     * @draft ICU 3.0
     */
    public static final ULocale CANADA_FRENCH = new ULocale("fr_CA", Locale.CANADA_FRENCH);

    /**
     * Handy constant.
     */
    private static final String EMPTY_STRING = "";

    // Used in both ULocale and IDParser, so moved up here.
    private static final char UNDERSCORE            = '_';

    /**
     * The root ULocale.
     * @draft ICU 2.8
     */ 
    public static final ULocale ROOT = new ULocale(EMPTY_STRING, null);
    
    /**
     * Cache the locale.
     */
    private transient Locale locale;

    /**
     * The raw localeID that we were passed in.
     */
    private String localeID;

    /**
     * Tables used in normalizing portions of the id.
     */
    /* tables updated per http://lcweb.loc.gov/standards/iso639-2/ 
       to include the revisions up to 2001/7/27 *CWB*/
    /* The 3 character codes are the terminology codes like RFC 3066.  
       This is compatible with prior ICU codes */
    /* "in" "iw" "ji" "jw" & "sh" have been withdrawn but are still in 
       the table but now at the end of the table because 
       3 character codes are duplicates.  This avoids bad searches
       going from 3 to 2 character codes.*/
    /* The range qaa-qtz is reserved for local use. */
    
    /* This list MUST be in sorted order, and MUST contain the two-letter codes
       if one exists otherwise use the three letter code */
    private static final String[] languages = {
        "aa",  "ab",  "ace", "ach", "ada", "ady", "ae",  "af",  "afa",
        "afh", "ak",  "akk", "ale", "alg", "am",  "an",  "ang", "apa",
        "ar",  "arc", "arn", "arp", "art", "arw", "as",  "ast",
        "ath", "aus", "av",  "awa", "ay",  "az",  "ba",  "bad",
        "bai", "bal", "bam", "ban", "bas", "bat", "be",  "bej",
        "bem", "ber", "bg",  "bh",  "bho", "bi",  "bik", "bin",
        "bla", "bm",  "bn",  "bnt", "bo",  "br",  "bra", "bs",
        "btk", "bua", "bug", "byn", "ca",  "cad", "cai", "car", "cau",
        "ce",  "ceb", "cel", "ch",  "chb", "chg", "chk", "chm",
        "chn", "cho", "chp", "chr", "chy", "cmc", "co",  "cop",
        "cpe", "cpf", "cpp", "cr",  "crh", "crp", "cs",  "csb", "cu",  "cus",
        "cv",  "cy",  "da",  "dak", "dar", "day", "de",  "del", "den",
        "dgr", "din", "doi", "dra", "dsb", "dua", "dum", "dv",  "dyu",
        "dz",  "ee",  "efi", "egy", "eka", "el",  "elx", "en",
        "enm", "eo",  "es",  "et",  "eu",  "ewo", "fa",
        "fan", "fat", "ff",  "fi",  "fiu", "fj",  "fo",  "fon",
        "fr",  "frm", "fro", "fur", "fy",  "ga",  "gaa", "gay",
        "gba", "gd",  "gem", "gez", "gil", "gl",  "gmh", "gn",
        "goh", "gon", "gor", "got", "grb", "grc", "gu",  "gv",
        "gwi", "ha",  "hai", "haw", "he",  "hi",  "hil", "him",
        "hit", "hmn", "ho",  "hr",  "hsb", "ht",  "hu",  "hup", "hy",  "hz",
        "ia",  "iba", "id",  "ie",  "ig",  "ii",  "ijo", "ik",
        "ilo", "inc", "ine", "inh", "io",  "ira", "iro", "is",  "it",
        "iu",  "ja",  "jbo", "jpr", "jrb", "jv",  "ka",  "kaa", "kab",
        "kac", "kam", "kar", "kaw", "kbd", "kg",  "kha", "khi",
        "kho", "ki",  "kj",  "kk",  "kl",  "km",  "kmb", "kn",
        "ko",  "kok", "kos", "kpe", "kr",  "krc", "kro", "kru", "ks",
        "ku",  "kum", "kut", "kv",  "kw",  "ky",  "la",  "lad",
        "lah", "lam", "lb",  "lez", "lg",  "li",  "ln",  "lo",  "lol",
        "loz", "lt",  "lu",  "lua", "lui", "lun", "luo", "lus",
        "lv",  "mad", "mag", "mai", "mak", "man", "map", "mas",
        "mdf", "mdr", "men", "mg",  "mga", "mh",  "mi",  "mic", "min",
        "mis", "mk",  "mkh", "ml",  "mn",  "mnc", "mni", "mno",
        "mo",  "moh", "mos", "mr",  "ms",  "mt",  "mul", "mun",
        "mus", "mwr", "my",  "myn", "myv", "na",  "nah", "nai", "nap",
        "nb",  "nd",  "nds", "ne",  "new", "ng",  "nia", "nic",
        "niu", "nl",  "nn",  "no",  "nog", "non", "nr",  "nso", "nub",
        "nv",  "ny",  "nym", "nyn", "nyo", "nzi", "oc",  "oj",
        "om",  "or",  "os",  "osa", "ota", "oto", "pa",  "paa",
        "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",
        "pi",  "pl",  "pon", "pra", "pro", "ps",  "pt",  "qu",
        "raj", "rap", "rar", "rm",  "rn",  "ro",  "roa", "rom",
        "ru",  "rw",  "sa",  "sad", "sah", "sai", "sal", "sam",
        "sas", "sat", "sc",  "sco", "sd",  "se",  "sel", "sem",
        "sg",  "sga", "sgn", "shn", "si",  "sid", "sio", "sit",
        "sk",  "sl",  "sla", "sm",  "sma", "smi", "smj", "smn",
        "sms", "sn",  "snk", "so",  "sog", "son", "sq",  "sr",
        "srr", "ss",  "ssa", "st",  "su",  "suk", "sus", "sux",
        "sv",  "sw",  "syr", "ta",  "tai", "te",  "tem", "ter",
        "tet", "tg",  "th",  "ti",  "tig", "tiv", "tk",  "tkl",
        "tl",  "tli", "tmh", "tn",  "to",  "tog", "tpi", "tr",
        "ts",  "tsi", "tt",  "tum", "tup", "tut", "tvl", "tw",
        "ty",  "tyv", "udm", "ug",  "uga", "uk",  "umb", "und", "ur",
        "uz",  "vai", "ve",  "vi",  "vo",  "vot", "wa",  "wak",
        "wal", "war", "was", "wen", "wo",  "xal", "xh",  "yao", "yap",
        "yi",  "yo",  "ypk", "za",  "zap", "zen", "zh",  "znd",
        "zu",  "zun", 
    };

    private static final String[] obsoleteLanguages = {
        "in",  "iw",  "ji",  "jw",  "sh",    /* obsolete language codes */         
    };
    
    /* This list MUST contain a three-letter code for every two-letter code in the
       list above, and they MUST ne in the same order (i.e., the same language must
       be in the same place in both lists)! */
    private static final String[] languages3 = {
        /*  "aa",  "ab",  "ace", "ach", "ada", "ady", "ae",  "af",  "afa",    */
        "aar", "abk", "ace", "ach", "ada", "ady", "ave", "afr", "afa",
        /*  "afh", "ak",  "akk", "ale", "alg", "am",  "an",  "ang", "apa",    */
        "afh", "aka", "akk", "ale", "alg", "amh", "arg", "ang", "apa",
        /*  "ar",  "arc", "arn", "arp", "art", "arw", "as",  "ast",    */
        "ara", "arc", "arn", "arp", "art", "arw", "asm", "ast",
        /*  "ath", "aus", "av",  "awa", "ay",  "az",  "ba",  "bad",    */
        "ath", "aus", "ava", "awa", "aym", "aze", "bak", "bad",
        /*  "bai", "bal", "bam", "ban", "bas", "bat", "be",  "bej",    */
        "bai", "bal", "bam", "ban", "bas", "bat", "bel", "bej",
        /*  "bem", "ber", "bg",  "bh",  "bho", "bi",  "bik", "bin",    */
        "bem", "ber", "bul", "bih", "bho", "bis", "bik", "bin",
        /*  "bla", "bm",  "bn",  "bnt", "bo",  "br",  "bra", "bs",     */
        "bla", "bm",  "ben", "bnt", "bod", "bre", "bra", "bos",
        /*  "btk", "bua", "bug", "byn", "ca",  "cad", "cai", "car", "cau",    */
        "btk", "bua", "bug", "byn", "cat", "cad", "cai", "car", "cau",
        /*  "ce",  "ceb", "cel", "ch",  "chb", "chg", "chk", "chm",    */
        "che", "ceb", "cel", "cha", "chb", "chg", "chk", "chm",
        /*  "chn", "cho", "chp", "chr", "chy", "cmc", "co",  "cop",    */
        "chn", "cho", "chp", "chr", "chy", "cmc", "cos", "cop",
        /*  "cpe", "cpf", "cpp", "cr",  "crh", "crp", "cs",  "csb", "cu",  "cus",    */
        "cpe", "cpf", "cpp", "cre", "crh", "crp", "ces", "csb", "chu", "cus",
        /*  "cv",  "cy",  "da",  "dak", "dar", "day", "de",  "del", "den",    */
        "chv", "cym", "dan", "dak", "dar", "day", "deu", "del", "den",
        /*  "dgr", "din", "doi", "dra", "dsb", "dua", "dum", "dv",  "dyu",    */
        "dgr", "din", "doi", "dra", "dsb", "dua", "dum", "div", "dyu",
        /*  "dz",  "ee",  "efi", "egy", "eka", "el",  "elx", "en",     */
        "dzo", "ewe", "efi", "egy", "eka", "ell", "elx", "eng",
        /*  "enm", "eo",  "es",  "et",  "eu",  "ewo", "fa",     */
        "enm", "epo", "spa", "est", "eus", "ewo", "fas",
        /*  "fan", "fat", "ff",  "fi",  "fiu", "fj",  "fo",  "fon",    */
        "fan", "fat", "ful", "fin", "fiu", "fij", "fao", "fon",
        /*  "fr",  "frm", "fro", "fur", "fy",  "ga",  "gaa", "gay",    */
        "fra", "frm", "fro", "fur", "fry", "gle", "gaa", "gay",
        /*  "gba", "gd",  "gem", "gez", "gil", "gl",  "gmh", "gn",     */
        "gba", "gla", "gem", "gez", "gil", "glg", "gmh", "grn",
        /*  "goh", "gon", "gor", "got", "grb", "grc", "gu",  "gv",     */
        "goh", "gon", "gor", "got", "grb", "grc", "guj", "glv",
        /*  "gwi", "ha",  "hai", "haw", "he",  "hi",  "hil", "him",    */
        "gwi", "hau", "hai", "haw", "heb", "hin", "hil", "him",
        /*  "hit", "hmn", "ho",  "hr",  "hsb", "ht",  "hu",  "hup", "hy",  "hz",     */
        "hit", "hmn", "hmo", "hrv", "hsb", "hat", "hun", "hup", "hye", "her",
        /*  "ia",  "iba", "id",  "ie",  "ig",  "ii",  "ijo", "ik",     */
        "ina", "iba", "ind", "ile", "ibo", "iii", "ijo", "ipk",
        /*  "ilo", "inc", "ine", "inh", "io",  "ira", "iro", "is",  "it",      */
        "ilo", "inc", "ine", "inh", "ido", "ira", "iro", "isl", "ita",
        /*  "iu",  "ja",  "jbo", "jpr", "jrb", "jv",  "ka",  "kaa", "kab",   */
        "iku", "jpn", "jbo", "jpr", "jrb", "jaw", "kat", "kaa", "kab",
        /*  "kac", "kam", "kar", "kaw", "kbd", "kg",  "kha", "khi",    */
        "kac", "kam", "kar", "kaw", "kbd", "kon", "kha", "khi",
        /*  "kho", "ki",  "kj",  "kk",  "kl",  "km",  "kmb", "kn",     */
        "kho", "kik", "kua", "kaz", "kal", "khm", "kmb", "kan",
        /*  "ko",  "kok", "kos", "kpe", "kr",  "krc", "kro", "kru", "ks",     */
        "kor", "kok", "kos", "kpe", "kau", "krc", "kro", "kru", "kas",
        /*  "ku",  "kum", "kut", "kv",  "kw",  "ky",  "la",  "lad",    */
        "kur", "kum", "kut", "kom", "cor", "kir", "lat", "lad",
        /*  "lah", "lam", "lb",  "lez", "lg",  "li",  "ln",  "lo",  "lol",    */
        "lah", "lam", "ltz", "lez", "lug", "lim", "lin", "lao", "lol",
        /*  "loz", "lt",  "lu",  "lua", "lui", "lun", "luo", "lus",    */
        "loz", "lit", "lub", "lua", "lui", "lun", "luo", "lus",
        /*  "lv",  "mad", "mag", "mai", "mak", "man", "map", "mas",    */
        "lav", "mad", "mag", "mai", "mak", "man", "map", "mas",
        /*  "mdf", "mdr", "men", "mg",  "mga", "mh",  "mi",  "mic", "min",    */
        "mdf", "mdr", "men", "mlg", "mga", "mah", "mri", "mic", "min",
        /*  "mis", "mk",  "mkh", "ml",  "mn",  "mnc", "mni", "mno",    */
        "mis", "mkd", "mkh", "mal", "mon", "mnc", "mni", "mno",
        /*  "mo",  "moh", "mos", "mr",  "ms",  "mt",  "mul", "mun",    */
        "mol", "moh", "mos", "mar", "msa", "mlt", "mul", "mun",
        /*  "mus", "mwr", "my",  "myn", "myv", "na",  "nah", "nai", "nap",    */
        "mus", "mwr", "mya", "myn", "myv", "nau", "nah", "nai", "nap",
        /*  "nb",  "nd",  "nds", "ne",  "new", "ng",  "nia", "nic",    */
        "nob", "nde", "nds", "nep", "new", "ndo", "nia", "nic",
        /*  "niu", "nl",  "nn",  "no",  "nog", "non", "nr",  "nso", "nub",    */
        "niu", "nld", "nno", "nor", "nog", "non", "nbl", "nso", "nub",
        /*  "nv",  "ny",  "nym", "nyn", "nyo", "nzi", "oc",  "oj",     */
        "nav", "nya", "nym", "nyn", "nyo", "nzi", "oci", "oji",
        /*  "om",  "or",  "os",  "osa", "ota", "oto", "pa",  "paa",    */
        "orm", "ori", "oss", "osa", "ota", "oto", "pan", "paa",
        /*  "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",    */
        "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",
        /*  "pi",  "pl",  "pon", "pra", "pro", "ps",  "pt",  "qu",     */
        "pli", "pol", "pon", "pra", "pro", "pus", "por", "que",
        /*  "raj", "rap", "rar", "rm",  "rn",  "ro",  "roa", "rom",    */
        "raj", "rap", "rar", "roh", "run", "ron", "roa", "rom",
        /*  "ru",  "rw",  "sa",  "sad", "sah", "sai", "sal", "sam",    */
        "rus", "kin", "san", "sad", "sah", "sai", "sal", "sam",
        /*  "sas", "sat", "sc",  "sco", "sd",  "se",  "sel", "sem",    */
        "sas", "sat", "srd", "sco", "snd", "sme", "sel", "sem",
        /*  "sg",  "sga", "sgn", "shn", "si",  "sid", "sio", "sit",    */
        "sag", "sga", "sgn", "shn", "sin", "sid", "sio", "sit",
        /*  "sk",  "sl",  "sla", "sm",  "sma", "smi", "smj", "smn",    */
        "slk", "slv", "sla", "smo", "sma", "smi", "smj", "smn",
        /*  "sms", "sn",  "snk", "so",  "sog", "son", "sq",  "sr",     */
        "sms", "sna", "snk", "som", "sog", "son", "sqi", "srp",
        /*  "srr", "ss",  "ssa", "st",  "su",  "suk", "sus", "sux",    */
        "srr", "ssw", "ssa", "sot", "sun", "suk", "sus", "sux",
        /*  "sv",  "sw",  "syr", "ta",  "tai", "te",  "tem", "ter",    */
        "swe", "swa", "syr", "tam", "tai", "tel", "tem", "ter",
        /*  "tet", "tg",  "th",  "ti",  "tig", "tiv", "tk",  "tkl",    */
        "tet", "tgk", "tha", "tir", "tig", "tiv", "tuk", "tkl",
        /*  "tl",  "tli", "tmh", "tn",  "to",  "tog", "tpi", "tr",     */
        "tgl", "tli", "tmh", "tsn", "ton", "tog", "tpi", "tur",
        /*  "ts",  "tsi", "tt",  "tum", "tup", "tut", "tvl", "tw",     */
        "tso", "tsi", "tat", "tum", "tup", "tut", "tvl", "twi",
        /*  "ty",  "tyv", "udm", "ug",  "uga", "uk",  "umb", "und", "ur",     */
        "tah", "tyv", "udm", "uig", "uga", "ukr", "umb", "und", "urd",
        /*  "uz",  "vai", "ve",  "vi",  "vo",  "vot", "wa",  "wak",    */
        "uzb", "vai", "ven", "vie", "vol", "vot", "wln", "wak",
        /*  "wal", "war", "was", "wen", "wo",  "xal", "xh",  "yao", "yap",    */
        "wal", "war", "was", "wen", "wol", "xal", "xho", "yao", "yap",
        /*  "yi",  "yo",  "ypk", "za",  "zap", "zen", "zh",  "znd",    */
        "yid", "yor", "ypk", "zha", "zap", "zen", "zho", "znd",
        /*  "zu",  "zun",                                              */
        "zul", "zun",  
    };
    
    private static final String[] obsoleteLanguages3 = {
        /* "in",  "iw",  "ji",  "jw",  "sh", */
         "ind", "heb", "yid", "jaw", "srp", 
    };
    
    /* ZR(ZAR) is now CD(COD) and FX(FXX) is PS(PSE) as per
       http://www.evertype.com/standards/iso3166/iso3166-1-en.html 
       added new codes keeping the old ones for compatibility
       updated to include 1999/12/03 revisions *CWB*/
    
    /* RO(ROM) is now RO(ROU) according to 
       http://www.iso.org/iso/en/prods-services/iso3166ma/03updates-on-iso-3166/nlv3e-rou.html
    */
    
    /* This list MUST be in sorted order, and MUST contain only two-letter codes! */
    private static final String[] countries = {
        "AD",  "AE",  "AF",  "AG",  "AI",  "AL",  "AM",  "AN",
        "AO",  "AQ",  "AR",  "AS",  "AT",  "AU",  "AW",  "AZ",
        "BA",  "BB",  "BD",  "BE",  "BF",  "BG",  "BH",  "BI",
        "BJ",  "BM",  "BN",  "BO",  "BR",  "BS",  "BT",  "BV",
        "BW",  "BY",  "BZ",  "CA",  "CC",  "CD",  "CF",  "CG",
        "CH",  "CI",  "CK",  "CL",  "CM",  "CN",  "CO",  "CR",
        "CU",  "CV",  "CX",  "CY",  "CZ",  "DE",  "DJ",  "DK",
        "DM",  "DO",  "DZ",  "EC",  "EE",  "EG",  "EH",  "ER",
        "ES",  "ET",  "FI",  "FJ",  "FK",  "FM",  "FO",  "FR",
        "GA",  "GB",  "GD",  "GE",  "GF",  "GH",  "GI",  "GL",
        "GM",  "GN",  "GP",  "GQ",  "GR",  "GS",  "GT",  "GU",
        "GW",  "GY",  "HK",  "HM",  "HN",  "HR",  "HT",  "HU",
        "ID",  "IE",  "IL",  "IN",  "IO",  "IQ",  "IR",  "IS",
        "IT",  "JM",  "JO",  "JP",  "KE",  "KG",  "KH",  "KI",
        "KM",  "KN",  "KP",  "KR",  "KW",  "KY",  "KZ",  "LA",
        "LB",  "LC",  "LI",  "LK",  "LR",  "LS",  "LT",  "LU",
        "LV",  "LY",  "MA",  "MC",  "MD",  "MG",  "MH",  "MK",
        "ML",  "MM",  "MN",  "MO",  "MP",  "MQ",  "MR",  "MS",
        "MT",  "MU",  "MV",  "MW",  "MX",  "MY",  "MZ",  "NA",
        "NC",  "NE",  "NF",  "NG",  "NI",  "NL",  "NO",  "NP",
        "NR",  "NU",  "NZ",  "OM",  "PA",  "PE",  "PF",  "PG",
        "PH",  "PK",  "PL",  "PM",  "PN",  "PR",  "PS",  "PT",
        "PW",  "PY",  "QA",  "RE",  "RO",  "RU",  "RW",  "SA",
        "SB",  "SC",  "SD",  "SE",  "SG",  "SH",  "SI",  "SJ",
        "SK",  "SL",  "SM",  "SN",  "SO",  "SR",  "ST",  "SV",
        "SY",  "SZ",  "TC",  "TD",  "TF",  "TG",  "TH",  "TJ",
        "TK",  "TL",  "TM",  "TN",  "TO",  "TR",  "TT",  "TV",
        "TW",  "TZ",  "UA",  "UG",  "UM",  "US",  "UY",  "UZ",
        "VA",  "VC",  "VE",  "VG",  "VI",  "VN",  "VU",  "WF",
        "WS",  "YE",  "YT",  "YU",  "ZA",  "ZM",  "ZW",  
    };
    
    private static final String[] obsoleteCountries = {
        "FX",  "RO",  "TP",  "ZR",   /* obsolete country codes */      
    };
    
    /* This list MUST contain a three-letter code for every two-letter code in
       the above list, and they MUST be listed in the same order! */
    private static final String[] countries3 = {
        /*  "AD",  "AE",  "AF",  "AG",  "AI",  "AL",  "AM",  "AN",     */
        "AND", "ARE", "AFG", "ATG", "AIA", "ALB", "ARM", "ANT",
        /*  "AO",  "AQ",  "AR",  "AS",  "AT",  "AU",  "AW",  "AZ",     */
        "AGO", "ATA", "ARG", "ASM", "AUT", "AUS", "ABW", "AZE",
        /*  "BA",  "BB",  "BD",  "BE",  "BF",  "BG",  "BH",  "BI",     */
        "BIH", "BRB", "BGD", "BEL", "BFA", "BGR", "BHR", "BDI",
        /*  "BJ",  "BM",  "BN",  "BO",  "BR",  "BS",  "BT",  "BV",     */
        "BEN", "BMU", "BRN", "BOL", "BRA", "BHS", "BTN", "BVT",
        /*  "BW",  "BY",  "BZ",  "CA",  "CC",  "CD",  "CF",  "CG",     */
        "BWA", "BLR", "BLZ", "CAN", "CCK", "COD", "CAF", "COG",
        /*  "CH",  "CI",  "CK",  "CL",  "CM",  "CN",  "CO",  "CR",     */
        "CHE", "CIV", "COK", "CHL", "CMR", "CHN", "COL", "CRI",
        /*  "CU",  "CV",  "CX",  "CY",  "CZ",  "DE",  "DJ",  "DK",     */
        "CUB", "CPV", "CXR", "CYP", "CZE", "DEU", "DJI", "DNK",
        /*  "DM",  "DO",  "DZ",  "EC",  "EE",  "EG",  "EH",  "ER",     */
        "DMA", "DOM", "DZA", "ECU", "EST", "EGY", "ESH", "ERI",
        /*  "ES",  "ET",  "FI",  "FJ",  "FK",  "FM",  "FO",  "FR",     */
        "ESP", "ETH", "FIN", "FJI", "FLK", "FSM", "FRO", "FRA",
        /*  "GA",  "GB",  "GD",  "GE",  "GF",  "GH",  "GI",  "GL",     */
        "GAB", "GBR", "GRD", "GEO", "GUF", "GHA", "GIB", "GRL",
        /*  "GM",  "GN",  "GP",  "GQ",  "GR",  "GS",  "GT",  "GU",     */
        "GMB", "GIN", "GLP", "GNQ", "GRC", "SGS", "GTM", "GUM",
        /*  "GW",  "GY",  "HK",  "HM",  "HN",  "HR",  "HT",  "HU",     */
        "GNB", "GUY", "HKG", "HMD", "HND", "HRV", "HTI", "HUN",
        /*  "ID",  "IE",  "IL",  "IN",  "IO",  "IQ",  "IR",  "IS",     */
        "IDN", "IRL", "ISR", "IND", "IOT", "IRQ", "IRN", "ISL",
        /*  "IT",  "JM",  "JO",  "JP",  "KE",  "KG",  "KH",  "KI",     */
        "ITA", "JAM", "JOR", "JPN", "KEN", "KGZ", "KHM", "KIR",
        /*  "KM",  "KN",  "KP",  "KR",  "KW",  "KY",  "KZ",  "LA",     */
        "COM", "KNA", "PRK", "KOR", "KWT", "CYM", "KAZ", "LAO",
        /*  "LB",  "LC",  "LI",  "LK",  "LR",  "LS",  "LT",  "LU",     */
        "LBN", "LCA", "LIE", "LKA", "LBR", "LSO", "LTU", "LUX",
        /*  "LV",  "LY",  "MA",  "MC",  "MD",  "MG",  "MH",  "MK",     */
        "LVA", "LBY", "MAR", "MCO", "MDA", "MDG", "MHL", "MKD",
        /*  "ML",  "MM",  "MN",  "MO",  "MP",  "MQ",  "MR",  "MS",     */
        "MLI", "MMR", "MNG", "MAC", "MNP", "MTQ", "MRT", "MSR",
        /*  "MT",  "MU",  "MV",  "MW",  "MX",  "MY",  "MZ",  "NA",     */
        "MLT", "MUS", "MDV", "MWI", "MEX", "MYS", "MOZ", "NAM",
        /*  "NC",  "NE",  "NF",  "NG",  "NI",  "NL",  "NO",  "NP",     */
        "NCL", "NER", "NFK", "NGA", "NIC", "NLD", "NOR", "NPL",
        /*  "NR",  "NU",  "NZ",  "OM",  "PA",  "PE",  "PF",  "PG",     */
        "NRU", "NIU", "NZL", "OMN", "PAN", "PER", "PYF", "PNG",
        /*  "PH",  "PK",  "PL",  "PM",  "PN",  "PR",  "PS",  "PT",     */
        "PHL", "PAK", "POL", "SPM", "PCN", "PRI", "PSE", "PRT",
        /*  "PW",  "PY",  "QA",  "RE",  "RO",  "RU",  "RW",  "SA",     */
        "PLW", "PRY", "QAT", "REU", "ROU", "RUS", "RWA", "SAU",
        /*  "SB",  "SC",  "SD",  "SE",  "SG",  "SH",  "SI",  "SJ",     */
        "SLB", "SYC", "SDN", "SWE", "SGP", "SHN", "SVN", "SJM",
        /*  "SK",  "SL",  "SM",  "SN",  "SO",  "SR",  "ST",  "SV",     */
        "SVK", "SLE", "SMR", "SEN", "SOM", "SUR", "STP", "SLV",
        /*  "SY",  "SZ",  "TC",  "TD",  "TF",  "TG",  "TH",  "TJ",     */
        "SYR", "SWZ", "TCA", "TCD", "ATF", "TGO", "THA", "TJK",
        /*  "TK",  "TL",  "TM",  "TN",  "TO",  "TR",  "TT",  "TV",     */
        "TKL", "TLS", "TKM", "TUN", "TON", "TUR", "TTO", "TUV",
        /*  "TW",  "TZ",  "UA",  "UG",  "UM",  "US",  "UY",  "UZ",     */
        "TWN", "TZA", "UKR", "UGA", "UMI", "USA", "URY", "UZB",
        /*  "VA",  "VC",  "VE",  "VG",  "VI",  "VN",  "VU",  "WF",     */
        "VAT", "VCT", "VEN", "VGB", "VIR", "VNM", "VUT", "WLF",
        /*  "WS",  "YE",  "YT",  "YU",  "ZA",  "ZM",  "ZW",            */
        "WSM", "YEM", "MYT", "YUG", "ZAF", "ZMB", "ZWE",
    };
    
    private static final String[] obsoleteCountries3 = {
        /*  "FX",  "RO",  "TP",  "ZR",   */
        "FXX", "ROM", "TMP", "ZAR",    
    };

    /* The left side is the result after getName is processes the name */
    /* The right side is what the locale should be converted to. */
    private static final String[][] variantsToKeywords = {
        { EMPTY_STRING,     "en_US_POSIX" }, /* .NET name */
        { "C",              "en_US_POSIX" }, /* POSIX name */
        { "art_LOJBAN",     "jbo" }, /* registered name */
        { "az_AZ_CYRL",     "az_Cyrl_AZ" }, /* .NET name */
        { "az_AZ_LATN",     "az_Latn_AZ" }, /* .NET name */
        { "ca_ES_PREEURO",  "ca_ES@currency=ESP" },
        { "cel_GAULISH",    "cel__GAULISH" }, /* registered name */
        { "de_1901",        "de__1901" }, /* registered name */
        { "de_1906",        "de__1906" }, /* registered name */
        { "de__PHONEBOOK",  "de@collation=phonebook" },
        { "de_AT_PREEURO",  "de_AT@currency=ATS" },
        { "de_DE_PREEURO",  "de_DE@currency=DEM" },
        { "de_LU_PREEURO",  "de_LU@currency=EUR" },
        { "el_GR_PREEURO",  "el_GR@currency=GRD" },
        { "en_BOONT",       "en__BOONT" }, /* registered name */
        { "en_SCOUSE",      "en__SCOUSE" }, /* registered name */
        { "en_BE_PREEURO",  "en_BE@currency=BEF" },
        { "en_IE_PREEURO",  "en_IE@currency=IEP" },
        { "es__TRADITIONAL", "es@collation=traditional" },
        { "es_ES_PREEURO",  "es_ES@currency=ESP" },
        { "eu_ES_PREEURO",  "eu_ES@currency=ESP" },
        { "fi_FI_PREEURO",  "fi_FI@currency=FIM" },
        { "fr_BE_PREEURO",  "fr_BE@currency=BEF" },
        { "fr_FR_PREEURO",  "fr_FR@currency=FRF" },
        { "fr_LU_PREEURO",  "fr_LU@currency=LUF" },
        { "ga_IE_PREEURO",  "ga_IE@currency=IEP" },
        { "gl_ES_PREEURO",  "gl_ES@currency=ESP" },
        { "hi__DIRECT",     "hi@collation=direct" },
        { "it_IT_PREEURO",  "it_IT@currency=ITL" },
        { "ja_JP_TRADITIONAL", "ja_JP@calendar=japanese" },
        { "nl_BE_PREEURO",  "nl_BE@currency=BEF" },
        { "nl_NL_PREEURO",  "nl_NL@currency=NLG" },
        { "pt_PT_PREEURO",  "pt_PT@currency=PTE" },
        { "sl_ROZAJ",       "sl__ROZAJ" }, /* registered name */
        { "sr_SP_CYRL",     "sr_Cyrl_SP" }, /* .NET name */
        { "sr_SP_LATN",     "sr_Latn_SP" }, /* .NET name */
        { "uz_UZ_CYRL",     "uz_Cyrl_UZ" }, /* .NET name */
        { "uz_UZ_LATN",     "uz_Latn_UZ" }, /* .NET name */
        { "zh_CHS",         "zh_Hans" }, /* .NET name */
        { "zh_CHT",         "zh_TW" }, /* .NET name TODO: This should be zh_Hant once the locale structure is fixed. */
        { "zh_GAN",         "zh__GAN" }, /* registered name */
        { "zh_GUOYU",       "zh" }, /* registered name */
        { "zh_HAKKA",       "zh__HAKKA" }, /* registered name */
        { "zh_MIN",         "zh__MIN" }, /* registered name */
        { "zh_MIN_NAN",     "zh__MINNAN" }, /* registered name */
        { "zh_WUU",         "zh__WUU" }, /* registered name */
        { "zh_XIANG",       "zh__XIANG" }, /* registered name */
        { "zh_YUE",         "zh__YUE" }, /* registered name */
        { "th_TH_TRADITIONAL", "th_TH@calendar=buddhist" },
        { "zh_TW_STROKE",   "zh_TW@collation=stroke" },
        { "zh__PINYIN",     "zh@collation=pinyin" }
    };

    /**
     * Private constructor used by static initializers.
     */
    private ULocale(String localeID, Locale locale) {
        this.localeID = localeID;
        this.locale = locale;
    }

    /**
     * Construct a ULocale object from a {@link java.util.Locale}.
     * @param loc a JDK locale
     * @draft ICU 2.8
     */
    public ULocale(Locale loc) {
        this.localeID = loc.toString();
        this.locale = loc;
    }

    /**
     * Construct a ULocale from a string of the form "sv_FI_ALAND".
     * By default this constructor will not normalize the localeID. 
     * 
     * @param locName string representation of the locale, e.g:
     * "en_US", "sy-Cyrl-YU"
     * @param localeID The locale identifier as a string
     * @draft ICU 2.8
     */ 
    public ULocale(String localeID) {
        this.localeID = localeID;
    }

    /**
     * Construct a ULocale from a string of the form "sv_FI_ALAND".
     * @param lang
     * @param script
     * @param country
     * @draft ICU 3.0
     */
    public ULocale(String lang, String script, String country) {
        this(lang,script,country, EMPTY_STRING);
    }

    /**
     * Construct a ULocale from a string of the form "sv_FI_ALAND".
     * @param lang
     * @param script
     * @param country
     * @param variant
     * @draft ICU 3.0
     */
    public ULocale(String lang, String script, String country, String variant) {
        StringBuffer buf = new StringBuffer();
        
        buf.append(lang.toLowerCase());
        if(script!=null && script.length() > 0){
            buf.append(UNDERSCORE);
            buf.append(script.substring(0, 1).toUpperCase());
            buf.append(script.substring(1).toLowerCase());
        }
        if(country!=null && country.length() > 0){
            buf.append(UNDERSCORE);
            buf.append(country.toUpperCase());
        }
        if(variant!=null && variant.length() > 0){
            if (country == null || country.length() == 0) {
                buf.append(UNDERSCORE);
            }
            buf.append(UNDERSCORE);
            buf.append(variant.toUpperCase());
        }
        localeID = buf.toString();
    }

    /**
     * Convert this ULocale object to a {@link java.util.Locale}.
     * @return a JDK locale that either exactly represents this object
     * or is the closest approximation.
     * @draft ICU 2.8
     */
    public Locale toLocale() {
        if (locale == null) {
            String[] names = new IDParser(localeID).getLanguageScriptCountryVariant();
            locale = new Locale(names[0], names[2], names[3]);
        }
        return locale;
    }
    
    /**
     * Keep our own default ULocale.
     */
    private static ULocale defaultULocale;

    /**
     * Return the current default ULocale.
     * @draft ICU 2.8
     */ 
    public static ULocale getDefault() {
        synchronized (ULocale.class) {
            Locale defaultLocale = Locale.getDefault();
            if (defaultULocale == null || defaultULocale.toLocale() != defaultLocale) {
                defaultULocale = new ULocale(defaultLocale);
            }
            return defaultULocale;
        }
    }

    /**
     * Sets the default ULocale.  This also sets the default Locale.
     * If the caller does not have write permission to the
     * user.language property, a security exception will be thrown,
     * and the default ULocale will remain unchanged.
     * @throws SecurityException
     *        if a security manager exists and its
     *        <code>checkPermission</code> method doesn't allow the operation.
     * @throws NullPointerException if <code>newLocale</code> is null
     * @param newLocale the new default locale
     * @see SecurityManager#checkPermission
     * @see java.util.PropertyPermission
     * @draft ICU 3.0 
     */
    public static synchronized void setDefault(ULocale newLocale){
        Locale.setDefault(newLocale.toLocale());
        defaultULocale = newLocale;
    }
    
    /**
     * This is for compatibility with Locale.  In actuality, since ULocale is
     * immutable, there is no reason to clone it, so this API returns 'this'.
     * @draft ICU 3.0
     */
    public Object clone() {
        return this;
    }

    /**
     * Override hashCode.
     * Since Locales are often used in hashtables, caches the value
     * for speed.
     * @draft ICU 3.0
     */
    public int hashCode() {
        return localeID.hashCode();
    }
    
    /**
     * Returns true if the other object is another ULocale with the
     * same full name, or is a String that matches the full name.
     * Note that since names are not canonicalized, two ULocales that
     * function identically might not compare equal.
     *
     * @return true if this Locale is equal to the specified object.
     * @draft ICU 3.0 
     */
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj instanceof String) {
            return localeID.equals((String)obj);   
        }
        if (obj instanceof ULocale) {
            return localeID.equals(((ULocale)obj).localeID);
        }
        return false;
    }
    
    /**
     * Returns a list of all installed locales.
     * @draft ICU 3.0
     */
    public static ULocale[] getAvailableLocales() {
        return ICUResourceBundle.getAvailableULocales();
    }

    /**
     * Returns a list of all 2-letter country codes defined in ISO 3166.
     * Can be used to create Locales.
     * @draft ICU 3.0
     */
    public static String[] getISOCountries() {
        return (String[])countries.clone();
    }

    /**
     * Returns a list of all 2-letter language codes defined in ISO 639.
     * Can be used to create Locales.
     * [NOTE:  ISO 639 is not a stable standard-- some languages' codes have changed.
     * The list this function returns includes both the new and the old codes for the
     * languages whose codes have changed.]
     * @draft ICU 3.0
     */
    public static String[] getISOLanguages() {
        return (String[])languages.clone();
    }

    /**
     * Returns the language code for this locale, which will either be the empty string
     * or a lowercase ISO 639 code.
     * @see #getDisplayLanguage
     * @draft ICU 3.0
     */
    public String getLanguage() {
        return getLanguage(localeID);
    }
    
    /**
     * Returns the language code for the locale ID specified,
     * which will either be the empty string
     * or a lowercase ISO 639 code.
     * @see #getDisplayLanguage
     * @draft ICU 3.0
     */
    public static String getLanguage(String localeID) {
        return new IDParser(localeID).getLanguage();
    }
     
    /**
     * Returns the script code for the specified locale.
     * @see #getDisplayScript
     * @draft ICU 3.0
     */
    public String getScript() {
        return getScript(localeID);
    }

    /**
     * Returns the script code for the specified locale.
     * @see #getDisplayScript
     * @draft ICU 3.0
     */
    public static String getScript(String localeID) {
        return new IDParser(localeID).getScript();
    }
    
    /**
     * Returns the country/region code for this locale, which will either be the empty string
     * or an upercase ISO 3166 2-letter code.
     * @see #getDisplayCountry
     * @draft ICU 3.0
     */
    public String getCountry() {
        return getCountry(localeID);
    }

    /**
     * Returns the country/region code for this locale, which will either be the empty string
     * or an upercase ISO 3166 2-letter code.
     * @param localeID
     * @see #getDisplayCountry
     * @draft ICU 3.0
     */
    public static String getCountry(String localeID) {
        return new IDParser(localeID).getCountry();
    }
    
    /**
     * Returns the variant code for this locale.
     * @see #getDisplayVariant
     * @draft ICU 3.0
     */
    public String getVariant() {
        return getVariant(localeID);
    }

    /**
     * Returns the variant code for this locale.
     * @see #getDisplayVariant
     * @draft ICU 3.0
     */
    public static String getVariant(String localeID) {
        return new IDParser(localeID).getVariant();
    }

    /**
     * Gets the (normalized) base name for this locale.
     * @return the base name as a String.
     * @draft ICU 3.0
     */
    public String getBaseName() {
        return getBaseName(localeID);
    }
    
    /**
     * Gets the (normalized) base name for the specified locale.
     * @param localeID the locale ID as a string
     * @return the base name as a String.
     * @draft ICU 3.0
     */
    public static String getBaseName(String localeID){
        return new IDParser(localeID).getBaseName();
    }

    /**
     * Gets the (normalized) full name for the this locale.
     *
     * @return String the full name of the localeID
     * @draft ICU 3.0
     */ 
    public String getName() {
        return getName(localeID);   
    }

    /**
     * Gets the (normalized) full name for the specified locale.
     *
     * @param localeID the locale to get the full name with
     * @return String the full name of the localeID
     * @draft ICU 3.0
     */
    public static String getName(String localeID){
        return new IDParser(localeID).getName();
    }

    /**
     * Return a string representation of this object.
     * @draft ICU 3.0
     */
    public String toString() {
        return localeID;
    }

    /**
     * Gets an iterator over keywords for the specified locale.
     * @return iterator over keywords
     * @draft ICU 3.0
     */
    public Iterator getKeywords() {
        return getKeywords(localeID);
    }

    /**
     * Gets an iterator over keywords for the specified locale.
     * @return an iterator over the keywords in the specified locale.
     * @draft ICU 3.0
     */
    public static Iterator getKeywords(String localeID){
        return new IDParser(localeID).getKeywords();
    }

    /**
     * Get the value for a keyword. Locale name does not need to be normalized.
     * @param keywordName name of the keyword for which we want the value. Case insensitive.
     * @return String the value of the keyword as a string
     * @draft ICU 3.0
     */
    public String getKeywordValue(String keywordName){
        return getKeywordValue(localeID, keywordName);
    }
    
    /**
     * Get the value for a keyword. Locale name does not need to be normalized.
     * @param keywordName name of the keyword for which we want the value. Case insensitive.
     * @return String the value of the keyword as a string
     * @draft ICU 3.0
     */
    public static String getKeywordValue(String localeID, String keywordName) {
        return new IDParser(localeID).getKeywordValue(keywordName);
    }

    /**
     * Utility class to parse and normalize locale ids (including POSIX style)
     */
    private static final class IDParser {
        private char[] id;
        private int index;
        private char[] buffer;
        private int blen;

        /**
         * Parsing constants.
         */
        private static final char KEYWORD_SEPARATOR     = '@';
        private static final char HYPHEN                = '-';
        private static final char KEYWORD_ASSIGN        = '=';
        private static final char COMMA                 = ',';
        private static final char ITEM_SEPARATOR        = ';';
        private static final char DOT                   = '.';

        private IDParser(String localeID){
            id = localeID.toCharArray();
            index = 0;
            buffer = new char[id.length + 5];
            blen = 0;
        }

        private void reset() {
            index = blen = 0;
        }

        // utilities for working on text in the buffer

        /**
         * Append c to the buffer.
         */
        private void append(char c) {
            try {
                buffer[blen] = c;
            }
            catch (IndexOutOfBoundsException e) {
                if (buffer.length > 512) {
                    // something is seriously wrong, let this go
                    throw e;
                }
                char[] nbuffer = new char[buffer.length * 2];
                System.arraycopy(buffer, 0, nbuffer, 0, buffer.length);
                nbuffer[blen] = c;
                buffer = nbuffer;
            }
            ++blen;
        }

        /**
         * Return the text in the buffer from start to blen as a String.
         */
        private String getString(int start) {
            if (start == blen) {
                return EMPTY_STRING;
            }
            return new String(buffer, start, blen-start);
        }

        /**
         * Set the length of the buffer to pos, then append the string.
         */
        private void set(int pos, String s) {
            this.blen = pos; // no safety
            append(s);
        }

        /**
         * Append the string to the buffer.
         */
        private void append(String s) {
            for (int i = 0; i < s.length(); ++i) {
                append(s.charAt(i));
            }
        }

        // utilities for parsing text out of the id

        /**
         * Character to indicate no more text is available in the id.
         */
        private static final char DONE = '\uffff';

        /**
         * Return the character at index in the id, and advance index.  The returned character
         * is DONE if index was at the limit of the buffer.  The index is advanced regardless
         * so that decrementing the index will always 'unget' the last character returned.
         */
        private char next() {
            if (index == id.length) {
              index++;
              return DONE; // 
            }

            return id[index++];
        }

        /**
         * Advance index until the next terminator or id separator, and leave it there.
         */
        private void skipUntilTerminatorOrIDSeparator() {
            while (!isTerminatorOrIDSeparator(next()));
            --index;
        }

        /**
         * Return true if the character at index in the id is a terminator.
         */
        private boolean atTerminator() {
            return index >= id.length || isTerminator(id[index]);
        }

        /**
         * Return true if the character is an id separator (underscore or hyphen).
         */
        private static boolean isIDSeparator(char c) {
            return c == UNDERSCORE || c == HYPHEN;
        }

        /**
         * Return true if the character is a terminator (keyword separator, dot, or DONE).
         * Dot is a terminator because of the POSIX form, where dot precedes the codepage.
         */
        private static boolean isTerminator(char c){
            return c == KEYWORD_SEPARATOR || c == DOT || c == DONE;   
        }

        /**
         * Return true if the character is a terminator or id separator.
         */
        private static boolean isTerminatorOrIDSeparator(char c) {
            return (c == KEYWORD_SEPARATOR) || (c == DOT) || 
                (c == UNDERSCORE || c == HYPHEN) || 
                (c == DONE);   
        }

        /**
         * Return true if the start of the buffer has an experimental or private language 
         * prefix, the pattern '[ixIX][-_].' shows the syntax checked.
         */
        private boolean haveExperimentalLanguagePrefix() {
            if (id.length > 2) {
                char c = id[1];
                if (c == HYPHEN || c == UNDERSCORE) {
                    c = id[0];
                    return c == 'x' || c == 'X' || c == 'i' || c == 'I';
                }
            }
            return false;
        }

        /**
         * Return true if a value separator occurs at or after index.
         */
        private boolean haveKeywordAssign() {
            // assume it is safe to start from index
            for (int i = index; i < id.length; ++i) {
                if (id[i] == KEYWORD_ASSIGN) {
                    return true;
                }
            }
            return false;
        }

        /**
         * Advance index past language, and accumulate normalized language code in buffer.
         * Index must be at 0 when this is called.  Index is left at a terminator or id 
         * separator.  Returns the start of the language code in the buffer.
         */
        private int parseLanguage() {
            if (haveExperimentalLanguagePrefix()) {
                append(Character.toLowerCase(id[0]));
                append(HYPHEN);
                index = 2;
            }
        
            char c;
            while(!isTerminatorOrIDSeparator(c = next())) {
                append(Character.toLowerCase(c));
            }
            --index;

            if (blen == 3) {
                /* convert 3 character code to 2 character code if possible *CWB*/
                String lang = getString(0);
                int offset = findIndex(languages3, lang);
                if (offset >= 0) {
                    set(0, languages[offset]);
                } else {
                    offset = findIndex(obsoleteLanguages3, lang);
                    if (offset >= 0) {
                        set(0, obsoleteLanguages[offset]);
                    }
                }
            }

            return 0;
        }

        /**
         * Advance index past language.  Index must be at 0 when this is called.  Index
         * is left at a terminator or id separator.
         */
        private void skipLanguage() {
            if (haveExperimentalLanguagePrefix()) {
                index = 2;
            }
            skipUntilTerminatorOrIDSeparator();
        }

        /**
         * Advance index past script, and accumulate normalized script in buffer.
         * Index must be immediately after the language.
         * If the item at this position is not a script (is not four characters
         * long) leave index and buffer unchanged.  Otherwise index is left at
         * a terminator or id separator.  Returns the start of the script code
         * in the buffer (this may be equal to the buffer length, if there is no
         * script).
         */
        private int parseScript() {
            if (!atTerminator()) {
                int oldIndex = index; // save original index
                ++index;

                int oldBlen = blen; // get before append hyphen, if we truncate everything is undone
                boolean first = true;
                char c;
                while(!isTerminatorOrIDSeparator(c = next())) {
                    if (first) {
                        append(UNDERSCORE); // note, adds to length
                        append(Character.toUpperCase(c));
                        first = false;
                    } else {
                        append(Character.toLowerCase(c));
                    }
                }
                --index; // unget

                /* If it's not exactly 4 characters long, then it's not a script. */
                if (index - oldIndex != 5) { // +1 to account for separator
                    index = oldIndex;
                    blen = oldBlen;
                } else {
                    oldBlen++; // index past hyphen, for clients who want to extract just the script
                }

                return oldBlen;
            }
            return blen;
        }

        /**
         * Advance index past script.
         * Index must be immediately after the language and IDSeparator.
         * If the item at this position is not a script (is not four characters
         * long) leave index.  Otherwise index is left at a terminator or
         * id separator.
         */
        private void skipScript() {
            if (!atTerminator()) {
                int oldIndex = index;
                ++index;

                skipUntilTerminatorOrIDSeparator();
                if (index - oldIndex != 5) { // +1 to account for separator
                    index = oldIndex;
                }
            }
        }

        /**
         * Advance index past country, and accumulate normalized country in buffer.
         * Index must be immediately after the script (if there is one, else language)
         * and IDSeparator.  Return the start of the country code in the buffer.
         */
        private int parseCountry() {
	    // we need to add an underscore even if we're at a terminator (except DONE) since
	    // we must add the underscore in case there's a variant.  POSIX can
	    // put variants after a terminator.  There are some odd POSIX ids that
	    // require this, e.g. 'no@ny'.
            if (atTerminator()) {
		if (index < id.length) { // might have variant
		    append(UNDERSCORE);
		}
	    } else {
                int oldIndex = index;
                ++index;

                // even if there is no country code, we insert a hyphen to mark
                // the space in case there is a following variant.  if there's no
                // variant, we'll trim it later.
                append(UNDERSCORE);

                int oldBlen = blen; // after hyphen

                char c;
                while (!isTerminatorOrIDSeparator(c = next())) {
                    append(Character.toUpperCase(c));
                }
                --index; // unget

                if (blen - oldBlen == 3) {
                    /* convert 3 character code to 2 character code if possible *CWB*/
                    int offset = findIndex(countries3, getString(oldBlen));
                    if (offset >= 0) {
                        set(oldBlen, countries[offset]);
                    } else {
                        offset = findIndex(obsoleteCountries3, getString(oldBlen));
                        if (offset >= 0) {
                            set(oldBlen, obsoleteCountries[offset]);
                        }
                    }
                }

                return oldBlen;
            }

            return blen;
        }

        /**
         * Advance index past country.
         * Index must be immediately after the script (if there is one, else language)
         * and IDSeparator.
         */
        private void skipCountry() {
            if (!atTerminator()) {
                ++index;
                skipUntilTerminatorOrIDSeparator();
            }
        }

        /**
         * Advance index past variant, and accumulate normalized variant in buffer.  This ignores
         * the codepage information from POSIX ids.  Index must be immediately after the country
         * or script.  Index is left at the keyword separator or at the end of the text.  Return
         * the start of the variant code in the buffer.
         *
         * In standard form, we can have the following forms:
         * ll__VVVV
         * ll_CC_VVVV
         * ll_Ssss_VVVV
         * ll_Ssss_CC_VVVV
         *
         * This also handles POSIX ids, which can have the following forms (pppp is code page id):
         * ll_CC.pppp          --> ll_CC
         * ll_CC.pppp@VVVV     --> ll_CC_VVVV
         * ll_CC@VVVV          --> ll_CC_VVVV
         *
         * We identify this use of '@' in POSIX ids by looking for an '=' following
         * the '@'.  If there is one, we consider '@' to start a keyword list, instead of
         * being part of a POSIX id.
         */
        private int parseVariant() {
            int oldBlen = blen;

            boolean first = true;
            char c = next();
            if (c == DOT) {
		// if we have a DOT, we ignore anything after the '@'
                while (!isTerminator(c = next())); // skip to terminator, assume no more DOTs
            } 
	    if (c != DONE && (c != KEYWORD_SEPARATOR || !haveKeywordAssign())) {
                // we have more text, and either had an id separator, or 
                // had a keyword separator in a POSIX locale
                // either way, we accumulate text until a terminator
                while (!isTerminator(c = next())) {
                    if (first) {
                        first = false;
                        append(UNDERSCORE); // separate from previous script or country
                    }
                    c = Character.toUpperCase(c);
                    if (c == HYPHEN || c == COMMA) {
                        c = UNDERSCORE;
                    }
                    append(c);
                }
            }
            --index; // unget
            
            if (blen > oldBlen) { // we had variant data
                ++oldBlen; // skip hyphen
            }
            return oldBlen;
        }

        // no need for skipvariant, to get the keywords we'll just scan directly for 
        // the keyword separator

        /**
         * Return the normalized language id, or the empty string.
         */
        public String getLanguage() {
            reset();
            return getString(parseLanguage());
        }
   
        /**
         * Return the normalized script id, or the empty string.
         */
        public String getScript() {
            reset();
            skipLanguage();
            return getString(parseScript());
        }
    
        /**
         * return the normalized country id, or the empty string.
         */
        public String getCountry() {
            reset();
            skipLanguage();
            skipScript();
            return getString(parseCountry());
        }

        /**
         * Return the normalized variant id, or the empty string.
         */
        public String getVariant() {
            reset();
            skipLanguage();
            skipScript();
            skipCountry();
            return getString(parseVariant());
        }

        /**
         * Return the language, script, country, and variant as separate strings.
         */
        public String[] getLanguageScriptCountryVariant() {
            reset();
            return new String[] {
                getString(parseLanguage()),
                getString(parseScript()),
                getString(parseCountry()),
                getString(parseVariant())
            };
        }

        public void parseBaseName() {
            reset();
            parseLanguage();
            parseScript();
            parseCountry();
            parseVariant();
            
            // catch unwanted trailing underscore after country if there was no variant
            if (blen > 1 && buffer[blen-1] == UNDERSCORE) {
                --blen;
            }
        }

        /**
         * Return the normalized base form of the locale id.  The base
         * form does not include keywords.
         */
        public String getBaseName() {
            parseBaseName();
            return getString(0);
        }

        /**
         * Return the normalized full form of the locale id.  The full
         * form includes keywords.
         */
        public String getName() {
            parseBaseName();
            parseKeywords();
            return getString(0);
        }

        // keyword utilities

        /**
         * If we have keywords, advance index to the start of the keywords and return true, 
         * otherwise return false.
         */
        private boolean setToKeywordStart() {
            for (int i = index; i < id.length; ++i) {
                if (id[i] == KEYWORD_SEPARATOR) {
                    for (int j = ++i; j < id.length; ++j) { // increment i past separator for return
                        if (id[j] == KEYWORD_ASSIGN) {
                            index = i;
                            return true;
                        }
                    }
                }
            }
            return false;
        }
        
        private static boolean isDoneOrKeywordAssign(char c) {
            return c == DONE || c == KEYWORD_ASSIGN;
        }

        private static boolean isDoneOrItemSeparator(char c) {
            return c == DONE || c == ITEM_SEPARATOR;
        }

        private String getKeyword() {
            int start = index;
            while (!isDoneOrKeywordAssign(next()));
            --index;
            return new String(id, start, index-start).trim().toLowerCase();
        }

        private String getValue() {
            int start = index;
            while (!isDoneOrItemSeparator(next()));
            --index;
            return new String(id, start, index-start).trim(); // leave case alone
        }

        /**
         * Return a map of the keywords and values.
         */
        private Map getKeywordMap() {
            Map m = null;

            if (setToKeywordStart()) {
                // trim spaces and convert to lower case, both keywords and values.
                do {
                    String key = getKeyword();
                    if (key.length() == 0) {
                        break;
                    }
                    if (next() != KEYWORD_ASSIGN) {
                        throw new IllegalArgumentException("key '" + key + "' missing a value.");
                    }
                    String value = getValue();
                    if (value.length() == 0) {
                        throw new IllegalArgumentException("key '" + key + "' missing a value.");
                    }
                    if (m == null) {
                        final Comparator comp = new Comparator() {
                                public int compare(Object lhs, Object rhs) {
                                    return ((String)lhs).compareTo(rhs);
                                }
                            };
                        m = new TreeMap(comp);
                    }
                    if (m.containsKey(key)) {
                        throw new IllegalArgumentException("key '" + key + "' already has a value.");
                    }
                    m.put(key, value);
                } while (next() == ITEM_SEPARATOR);
            }
            return m == null ? Collections.EMPTY_MAP : m;
        }

        /**
         * Parse the keywords and return start of the string in the buffer.
         */
        private int parseKeywords() {
            int oldBlen = blen;
            Map m = getKeywordMap();
            Iterator iter = m.entrySet().iterator();
            boolean first = true;
            while (iter.hasNext()) {
                append(first ? KEYWORD_SEPARATOR : ITEM_SEPARATOR);
                first = false;
                Map.Entry e = (Map.Entry)iter.next();
                append((String)e.getKey());
                append(KEYWORD_ASSIGN);
                append((String)e.getValue());
            }
            if (blen != oldBlen) {
                ++oldBlen;
            }
            return oldBlen;
        }

        /**
         * Return an iterator over the keywords.
         */
        public Iterator getKeywords() {
            return getKeywordMap().keySet().iterator();
        }

        /**
         * Return the value for the named keyword, or null if the keyword is not
         * present.
         */
        public String getKeywordValue(String keywordName) {
            return (String)getKeywordMap().get(keywordName.trim().toLowerCase());
        }
    }

    /**
     * linear search of the string array. the arrays are unfortunately ordered by the
     * two-letter target code, not the three-letter search code, which seems backwards.
     */
    private static int findIndex(String[] array, String target){
        for (int i = 0; i < array.length; i++) {
            if (target.equals(array[i])) {
                return i;
            }
        }
        return -1;
    }    

    /**
     * Gets the canonical name for the specified locale.  The specified
     * locale should not contain keywords, if it does they will be ignored.
     *
     * @param localeID the locale id
     * @return the canonicalized id
     * @draft ICU 3.0
     */
    public static String canonicalize(String localeID){
        String locStr = new IDParser(localeID).getName();
        // now we have an ID in the form xx_Yyyy_ZZ_KKKKK
        /* See if this is an already known locale */
        for (int i = 0; i < variantsToKeywords.length; i++) {
            if (variantsToKeywords[i][0].equals(locStr)) {
                locStr = variantsToKeywords[i][1];
                break;
            }
        }

        /* convert the Euro variant to appropriate ID */
        int idx = locStr.indexOf("_EURO");
        if (idx > -1) {
            locStr = locStr.substring(0,idx)+"@currency=EUR";       
        }
        
        return locStr;
    }
    
    /**
     * Returns a three-letter abbreviation for this locale's language.  If the locale
     * doesn't specify a language, this will be the empty string.  Otherwise, this will
     * be a lowercase ISO 639-2/T language code.
     * The ISO 639-2 language codes can be found on-line at
     *   <a href="ftp://dkuug.dk/i18n/iso-639-2.txt"><code>ftp://dkuug.dk/i18n/iso-639-2.txt</code></a>
     * @exception MissingResourceException Throws MissingResourceException if the
     * three-letter language abbreviation is not available for this locale.
     * @draft ICU 3.0
     */
    public String getISO3Language(){
        return getISO3Language(localeID);
    }

    /**
     * Returns a three-letter abbreviation for this locale's language.  If the locale
     * doesn't specify a language, this will be the empty string.  Otherwise, this will
     * be a lowercase ISO 639-2/T language code.
     * The ISO 639-2 language codes can be found on-line at
     *   <a href="ftp://dkuug.dk/i18n/iso-639-2.txt"><code>ftp://dkuug.dk/i18n/iso-639-2.txt</code></a>
     * @exception MissingResourceException Throws MissingResourceException if the
     * three-letter language abbreviation is not available for this locale.
     * @draft ICU 3.0
     */
    public static String getISO3Language(String localeID){
        String language = getLanguage(localeID);
        int offset = findIndex(languages, language);
        if(offset>=0){
            return languages3[offset];
        } else {
	    offset = findIndex(obsoleteLanguages, language);
	    if (offset >= 0) {
		return obsoleteLanguages3[offset];
	    }
	}
        return EMPTY_STRING;
    }
    
    /**
     * Returns a three-letter abbreviation for this locale's country.  If the locale
     * doesn't specify a country, this will be tbe the empty string.  Otherwise, this will
     * be an uppercase ISO 3166 3-letter country code.
     * @exception MissingResourceException Throws MissingResourceException if the
     * three-letter country abbreviation is not available for this locale.
     * @draft ICU 3.0
     */
    public String getISO3Country(){
        return getISO3Country(localeID);
    }
    /**
     * Returns a three-letter abbreviation for this locale's country.  If the locale
     * doesn't specify a country, this will be the the empty string.  Otherwise, this will
     * be an uppercase ISO 3166 3-letter country code.
     * @exception MissingResourceException Throws MissingResourceException if the
     * three-letter country abbreviation is not available for this locale.
     * @draft ICU 3.0
     */
    public static String getISO3Country(String localeID){
        String country = getCountry(localeID);
        int offset = findIndex(countries, country);
        if(offset>=0){
            return countries3[offset];
        }else{
            offset = findIndex(obsoleteCountries, country);
            if(offset>=0){
                return obsoleteCountries3[offset];   
            }
        }
        return EMPTY_STRING;
    }
    
    // display names

    /**
     * Utility to fetch locale display data from resource bundle tables.
     */
    private static String getTableString(String tableName, String subtableName, String item, ULocale displayLocale) {
        try {
            ICUResourceBundle bundle = (ICUResourceBundle)UResourceBundle.getBundleInstance(displayLocale);
            return getTableString(tableName, subtableName, item, bundle);
        } catch (Exception e) {
//          System.out.println("gtsu: " + e.getMessage());
        }
        return item;
    }
        
    /**
     * Utility to fetch locale display data from resource bundle tables.
     */
    private static String getTableString(String tableName, String subtableName, String item, ICUResourceBundle bundle) {
        try {
            // special case currency
            if ("currency".equals(subtableName)) {
                ICUResourceBundle table = bundle.get("Currencies");
                table = table.getWithFallback(item);
                return table.getString(1);
            } else {
                ICUResourceBundle table = bundle.get(tableName);
                if (subtableName != null) {
                    table = bundle.get(subtableName);
                }
                return table.getString(item);
            }
        }
        catch (Exception e) {
//          System.out.println("gtsi: " + e.getMessage());
        }
        return item;
    }

    /**
     * Return this locale's language localized for display in the default locale.
     * @return the localized language name.
     * @draft ICU 3.0
     */
    public String getDisplayLanguage() {
        return getDisplayLanguage(getDefault());
    }

    /**
     * Return this locale's language localized for display in the provided locale.
     * @param displayLocale the locale in which to display the name.
     * @return the localized language name.
     * @draft ICU 3.0
     */
    public String getDisplayLanguage(ULocale displayLocale) {
        return getDisplayLanguage(localeID, displayLocale);
    }
    
    /**
     * Return a locale's language localized for display in the provided locale.
     * This is a cover for the ICU4C API.
     * @param localeID the id of the locale whose language will be displayed
     * @param displayLocaleID the id of the locale in which to display the name.
     * @return the localized language name.
     * @draft ICU 3.0
     */
    public static String getDisplayLanguage(String localeID, String displayLocaleID) {
        return getDisplayLanguage(localeID, new ULocale(displayLocaleID));
    }

    /**
     * Return a locale's language localized for display in the provided locale.
     * This is a cover for the ICU4C API.
     * @param localeID the id of the locale whose language will be displayed.
     * @param displayLocale the locale in which to display the name.
     * @return the localized language name.
     * @draft ICU 3.0
     */
    public static String getDisplayLanguage(String localeID, ULocale displayLocale) {
        return getTableString("Languages", null, new IDParser(localeID).getLanguage(), displayLocale);
    }
    
    /**
     * Return this locale's script localized for display in the default locale.
     * @return the localized script name.
     * @draft ICU 3.0
     */
    public String getDisplayScript() {
        return getDisplayScript(getDefault());
    }

    /**
     * Return this locale's script localized for display in the provided locale.
     * @param displayLocale the locale in which to display the name.
     * @return the localized script name.
     * @draft ICU 3.0
     */
    public String getDisplayScript(ULocale displayLocale) {
        return getDisplayScript(localeID, displayLocale.localeID);
    }
    
    /**
     * Return a locale's script localized for display in the provided locale.
     * This is a cover for the ICU4C API.
     * @param localeID the id of the locale whose script will be displayed
     * @param displayLocaleID the id of the locale in which to display the name.
     * @return the localized script name.
     * @draft ICU 3.0
     */
    public static String getDisplayScript(String localeID, String displayLocaleID) {
        return getDisplayScript(localeID, new ULocale(displayLocaleID));
    }
    
    /**
     * Return a locale's script localized for display in the provided locale.
     * @param localeID the id of the locale whose script will be displayed.
     * @param displayLocale the locale in which to display the name.
     * @return the localized script name.
     * @draft ICU 3.0
     */
    public static String getDisplayScript(String localeID, ULocale displayLocale) {
        return getTableString("Scripts", null, new IDParser(localeID).getScript(), displayLocale);
    }

    /**
     * Return this locale's country localized for display in the default locale.
     * @return the localized country name.
     * @draft ICU 3.0
     */
    public String getDisplayCountry() {
        return getDisplayCountry(getDefault());
    }
    
    /**
     * Return this locale's country localized for display in the provided locale.
     * @param displayLocale the locale in which to display the name.
     * @return the localized country name.
     * @draft ICU 3.0
     */
    public String getDisplayCountry(ULocale displayLocale){
        return getDisplayCountry(localeID, displayLocale);   
    }
    
    /**
     * Return a locale's country localized for display in the provided locale.
     * This is a cover for the ICU4C API.
     * @param localeID the id of the locale whose country will be displayed
     * @param displayLocaleID the id of the locale in which to display the name.
     * @return the localized country name.
     * @draft ICU 3.0
     */
    public static String getDisplayCountry(String localeID, String displayLocaleID) {
        return getDisplayCountry(localeID, new ULocale(displayLocaleID));
    }

    /**
     * Return a locale's country localized for display in the provided locale.
     * This is a cover for the ICU4C API.
     * @param localeID the id of the locale whose country will be displayed.
     * @param displayLocale the locale in which to display the name.
     * @return the localized country name.
     * @draft ICU 3.0
     */
    public static String getDisplayCountry(String localeID, ULocale displayLocale) {
        return getTableString("Countries", null, new IDParser(localeID).getCountry(), displayLocale);
    }
    
    /**
     * Return this locale's variant localized for display in the default locale.
     * @return the localized variant name.
     * @draft ICU 3.0
     */
    public String getDisplayVariant() {
        return getDisplayVariant(getDefault());   
    }

    /**
     * Return this locale's variant localized for display in the provided locale.
     * @param displayLocale the locale in which to display the name.
     * @return the localized variant name.
     * @draft ICU 3.0
     */
    public String getDisplayVariant(ULocale displayLocale){
        return getDisplayVariant(localeID, displayLocale);   
    }
    
    /**
     * Return a locale's variant localized for display in the provided locale.
     * This is a cover for the ICU4C API.
     * @param localeID the id of the locale whose variant will be displayed
     * @param displayLocaleID the id of the locale in which to display the name.
     * @return the localized variant name.
     * @draft ICU 3.0
     */
    public static String getDisplayVariant(String localeID, String displayLocaleID){
        return getDisplayVariant(localeID, new ULocale(displayLocaleID));
    }
    
    /**
     * Return a locale's variant localized for display in the provided locale.
     * This is a cover for the ICU4C API.
     * @param localeID the id of the locale whose variant will be displayed.
     * @param displayLocale the locale in which to display the name.
     * @return the localized variant name.
     * @draft ICU 3.0
     */
    public static String getDisplayVariant(String localeID, ULocale displayLocale) {
        return getTableString("Variants", null, new IDParser(localeID).getVariant(), displayLocale);
    }

    /**
     * Return a keyword localized for display in the default locale.
     * @param keyword the keyword to be displayed.
     * @return the localized keyword name.
     * @see #getKeywords
     * @draft ICU 3.0
     */
    public static String getDisplayKeyword(String keyword) {
        return getDisplayKeyword(keyword, getDefault());   
    }
    
    /**
     * Return a keyword localized for display in the specified locale.
     * @param keyword the keyword to be displayed.
     * @param displayLocaleID the id of the locale in which to display the keyword.
     * @return the localized keyword name.
     * @see #getKeywords
     * @draft ICU 3.0
     */
    public static String getDisplayKeyword(String keyword, String displayLocaleID) {
        return getDisplayKeyword(keyword, new ULocale(displayLocaleID));   
    }

    /**
     * Return a keyword localized for display in the specified locale.
     * @param keyword the keyword to be displayed.
     * @param displayLocale the locale in which to display the keyword.
     * @return the localized keyword name.
     * @see #getKeywords
     * @draft ICU 3.0
     */
    public static String getDisplayKeyword(String keyword, ULocale displayLocale) {
        return getTableString("Keys", null, keyword.trim().toLowerCase(), displayLocale);
    }

    /**
     * Return a keyword value localized for display in the default locale.
     * @param keyword the keyword whose value is to be displayed.
     * @return the localized value name.
     * @draft ICU 3.0
     */
    public String getDisplayKeywordValue(String keyword) {
        return getDisplayKeywordValue(keyword, getDefault());   
    }
    
    /**
     * Return a keyword value localized for display in the specified locale.
     * @param keyword the keyword whose value is to be displayed.
     * @param displayLocale the locale in which to display the value.
     * @return the localized value name.
     * @draft ICU 3.0
     */
    public String getDisplayKeywordValue(String keyword, ULocale displayLocale) {
        return getDisplayKeywordValue(localeID, keyword, displayLocale);   
    }

    /**
     * Return a keyword value localized for display in the specified locale.
     * @param localeID the id of the locale whose keyword value is to be displayed.
     * @param keyword the keyword whose value is to be displayed.
     * @param displayLocaleID the id of the locale in which to display the value.
     * @return the localized value name.
     * @draft ICU 3.0
     */
    public String getDisplayKeywordValue(String localeID, String keyword, String displayLocaleID) {
        return getDisplayKeywordValue(localeID, keyword, new ULocale(displayLocaleID));
    }

    /**
     * Return a keyword value localized for display in the specified locale.
     * @param localeID the id of the locale whose keyword value is to be displayed.
     * @param keyword the keyword whose value is to be displayed.
     * @param displayLocaleID the id of the locale in which to display the value.
     * @return the localized value name.
     * @draft ICU 3.0
     */
    public static String getDisplayKeywordValue(String localeID, String keyword, ULocale displayLocale) {
        keyword = keyword.trim().toLowerCase();
        String value = new IDParser(localeID).getKeywordValue(keyword);
        return getTableString("Types", keyword, value, displayLocale);
    }
    
    /**
     * Return this locale name localized for display in the default locale.
     * @return the localized locale name.
     * @draft ICU 3.0
     */
    public String getDisplayName() {
        return getDisplayName(getDefault());
    }
    
    /**
     * Return this locale name localized for display in the provided locale.
     * @param displayLocale the locale in which to display the locale name.
     * @return the localized locale name.
     * @draft ICU 3.0
     */
    public String getDisplayName(ULocale displayLocale) {
        return getDisplayName(localeID, displayLocale);
    }
    
    /**
     * Return the locale ID localized for display in the provided locale.
     * @param localeID the locale whose name is to be displayed.
     * @param displayLocaleID the id of the locale in which to display the locale name.
     * @return the localized locale name.
     * @draft ICU 3.0
     */
    public static String getDisplayName(String localeID, String displayLocaleID) {
        return getDisplayName(localeID, new ULocale(displayLocaleID));
    }

    /**
     * Return the locale ID localized for display in the provided locale.
     * @param the locale whose name is to be displayed.
     * @param displayLocale the locale in which to display the locale name.
     * @return the localized locale name.
     * @draft ICU 3.0
     */
    public static String getDisplayName(String localeID, ULocale displayLocale) {
        // lang
        // lang (script, country, variant, keyword=value, ...)
        // script, country, variant, keyword=value, ...

        final String[] tableNames = { "Languages", "Scripts", "Countries", "Variants" };

        ICUResourceBundle bundle = (ICUResourceBundle)UResourceBundle.getBundleInstance(displayLocale);

        StringBuffer buf = new StringBuffer();

        IDParser parser = new IDParser(localeID);
        String[] names = parser.getLanguageScriptCountryVariant();

        boolean haveLanguage = names[0].length() > 0;
        boolean openParen = false;
        for (int i = 0; i < names.length; ++i) {
            String name = names[i];
            if (name.length() > 0) {
                name = getTableString(tableNames[i], null, name, bundle);
                if (buf.length() > 0) { // need a separator
                    if (haveLanguage & !openParen) {
                        buf.append(" (");
                        openParen = true;
                    } else {
                        buf.append(", ");
                    }
                }
                buf.append(name);
            }
        }

        Iterator keys = parser.getKeywordMap().entrySet().iterator();
        while (keys.hasNext()) {
            if (buf.length() > 0) {
                if (haveLanguage & !openParen) {
                    buf.append(" (");
                    openParen = true;
                } else {
                    buf.append(", ");
                }
            }
            Map.Entry e = (Map.Entry)keys.next();
            String key = (String)e.getKey();
            String val = (String)e.getValue();
            buf.append(getTableString("Keys", null, key, bundle));
            buf.append("=");
            buf.append(getTableString("Types", key, val, bundle));
        }

        if (openParen) {
            buf.append(")");
        }
            
        return buf.toString();
    }

    /** 
     * Selector for <tt>getLocale()</tt> indicating the locale of the
     * resource containing the data.  This is always at or above the
     * valid locale.  If the valid locale does not contain the
     * specific data being requested, then the actual locale will be
     * above the valid locale.  If the object was not constructed from
     * locale data, then the valid locale is <i>null</i>.
     *
     * @draft ICU 2.8
     */
    public static Type ACTUAL_LOCALE = new Type(0);
 
    /** 
     * Selector for <tt>getLocale()</tt> indicating the most specific
     * locale for which any data exists.  This is always at or above
     * the requested locale, and at or below the actual locale.  If
     * the requested locale does not correspond to any resource data,
     * then the valid locale will be above the requested locale.  If
     * the object was not constructed from locale data, then the
     * actual locale is <i>null</i>.
     *
     * <p>Note: The valid locale will be returned correctly in ICU
     * 3.0 or later.  In ICU 2.8, it is not returned correctly.
     * @draft ICU 2.8
     */ 
    public static Type VALID_LOCALE = new Type(1);
    
    /**
     * Opaque selector enum for <tt>getLocale()</tt>.
     * @see com.ibm.icu.util.ULocale
     * @see com.ibm.icu.util.ULocale#ACTUAL_LOCALE
     * @see com.ibm.icu.util.ULocale#VALID_LOCALE
     * @draft ICU 2.8
     */
    public static final class Type {
        private int localeType;
        private Type(int type) { localeType = type; }
    }
}
