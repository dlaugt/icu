
/*
********************************************************************
* COPYRIGHT: 
* (C) Copyright Taligent, Inc., 1997
* (C) Copyright International Business Machines Corporation, 1997 - 1998
* Licensed Material - Program-Property of IBM - All Rights Reserved. 
* US Government Users Restricted Rights - Use, duplication, or disclosure 
* restricted by GSA ADP Schedule Contract with IBM Corp. 
*
********************************************************************
* Modification History:
*   Date        Name        Description
*   07/15/99    helena      Ported to HPUX 10/11 CC.
*/

#include "numfmtst.h"
#include "dcfmtsym.h"
#include "decimfmt.h"
#include "locid.h"
 
// *****************************************************************************
// class NumberFormatTest
// *****************************************************************************

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

#define CHECK(status,str) if (FAILURE(status)) { errln(UnicodeString("FAIL: ") + str); return; }

void NumberFormatTest::runIndexedTest( int32_t index, bool_t exec, char* &name, char* par )
{
    // if (exec) logln((UnicodeString)"TestSuite DateFormatTest");
    switch (index) {
        CASE(0,TestCurrencySign)
        CASE(1,TestCurrency)
        CASE(2,TestParse)
        CASE(3,TestRounding487)
        CASE(4,TestQuotes)
        CASE(5,TestExponential)
        CASE(6,TestPatterns)

        // Upgrade to alphaWorks - liu 5/99
        CASE(7,TestExponent)
        CASE(8,TestScientific)
        CASE(9,TestPad)
        CASE(10,TestPatterns2)

        default: name = ""; break;
    }
}
 
// -------------------------------------

// Test various patterns
void
NumberFormatTest::TestPatterns()
{
    UErrorCode status = ZERO_ERROR;
    DecimalFormatSymbols sym(Locale::US, status);
    if (FAILURE(status)) { errln("FAIL: Could not construct DecimalFormatSymbols"); return; }

    const char* pat[]    = { "#.#", "#.", ".#", "#" };
    int32_t pat_length = sizeof(pat) / sizeof(pat[0]);
    const char* newpat[] = { "#0.#", "#0.", "#.0", "#" };
    const char* num[]    = { "0",   "0.", ".0", "0" };
    for (int32_t i=0; i<pat_length; ++i)
    {
        status = ZERO_ERROR;
        DecimalFormat fmt(pat[i], sym, status);
        if (FAILURE(status)) { errln((UnicodeString)"FAIL: DecimalFormat constructor failed for " + pat[i]); continue; }
        UnicodeString newp; fmt.toPattern(newp);
        if (!(newp == newpat[i]))
            errln((UnicodeString)"FAIL: Pattern " + pat[i] + " should transmute to " + newpat[i] +
                  "; " + newp + " seen instead");

        UnicodeString s; (*(NumberFormat*)&fmt).format(T_INT32(0), s);
        if (!(s == num[i]))
        {
            errln((UnicodeString)"FAIL: Pattern " + pat[i] + " should format zero as " + num[i] +
                  "; " + s + " seen instead");
            logln((UnicodeString)"Min integer digits = " + fmt.getMinimumIntegerDigits());
        }
    }
}

// -------------------------------------

// Test exponential pattern
void
NumberFormatTest::TestExponential()
{
    UErrorCode status = ZERO_ERROR;
    DecimalFormatSymbols sym(Locale::US, status);
    if (FAILURE(status)) { errln("FAIL: Bad status returned by DecimalFormatSymbols ct"); return; }
    char* pat[] = { "0.####E0", "00.000E00", "##0.######E000", "0.###E0;[0.###E0]"  };
    int32_t pat_length = sizeof(pat) / sizeof(pat[0]);
    double val[] = { 0.01234, 123456789, 1.23e300, -3.141592653e-271 };
    int32_t val_length = sizeof(val) / sizeof(val[0]);
    int32_t lval[] = { 0, -1, 1, 123456789 };
    int32_t lval_length = sizeof(lval) / sizeof(lval[0]);
    char* valFormat[] =
    {
        // 0.####E0
        "1.234E-2", "1.2346E8", "1.23E300", "-3.1416E-271",
        // 00.000E00
        "12.340E-03", "12.346E07", "12.300E299", "-31.416E-272",
        // ##0.######E000
        "12.34E-003", "123.4568E006", "1.23E300", "-314.1593E-273",
        // 0.###E0;[0.###E0]
        "1.234E-2", "1.235E8", "1.23E300", "[3.142E-271]"
    };
    char* lvalFormat[] =
    {
        // 0.####E0
        "0E0", "-1E0", "1E0", "1.2346E8",
        // 00.000E00
        "00.000E00", "-10.000E-01", "10.000E-01", "12.346E07",
        // ##0.######E000
        "0E000", "-1E000", "1E000", "123.4568E006",
        // 0.###E0;[0.###E0]
        "0E0", "[1E0]", "1E0", "1.235E8"
    };
    double valParse[] =
    {
        0.01234, 123460000, 1.23E300, -3.1416E-271,
        0.01234, 123460000, 1.23E300, -3.1416E-271,
        0.01234, 123456800, 1.23E300, -3.141593E-271,
        0.01234, 123500000, 1.23E300, -3.142E-271,
    };
    int32_t lvalParse[] =
    {
        0, -1, 1, 123460000,
        0, -1, 1, 123460000,
        0, -1, 1, 123456800,
        0, -1, 1, 123500000,
    };
    int32_t ival = 0, ilval = 0;
    for (int32_t p=0; p<pat_length; ++p)
    {
        DecimalFormat fmt(pat[p], sym, status);
        if (FAILURE(status)) { errln("FAIL: Bad status returned by DecimalFormat ct"); continue; }
        UnicodeString pattern;
        logln((UnicodeString)"Pattern \"" + pat[p] + "\" -toPattern-> \"" +
          fmt.toPattern(pattern) + "\"");
        int32_t v;
        for (v=0; v<val_length; ++v)
        {
            UnicodeString s; (*(NumberFormat*)&fmt).format(val[v], s);
            logln((UnicodeString)" " + val[v] + " -format-> " + s);
            if (s != valFormat[v+ival])
                errln((UnicodeString)"FAIL: Expected " + valFormat[v+ival]);

            ParsePosition pos(0);
            Formattable af;
            fmt.parse(s, af, pos);
            double a;
            if (af.getType() == Formattable::kLong) a = af.getLong();
            else if (af.getType() == Formattable::kDouble) a = af.getDouble();
            else errln((UnicodeString)"FAIL: Non-numeric Formattable returned");
            if (pos.getIndex() == s.size())
            {
                logln((UnicodeString)"  -parse-> " + a);
                if (a != valParse[v+ival])
                errln((UnicodeString)"FAIL: Expected " + valParse[v+ival]);
            }
            else
                errln((UnicodeString)"FAIL: Partial parse (" + pos.getIndex() + " chars) -> " + a);
        }
        for (v=0; v<lval_length; ++v)
        {
            UnicodeString s; (*(NumberFormat*)&fmt).format(lval[v], s);
            logln((UnicodeString)" " + lval[v] + "L -format-> " + s);
            if (s != lvalFormat[v+ilval])
                errln((UnicodeString)"ERROR: Expected " + lvalFormat[v+ilval]);

            ParsePosition pos(0);
            Formattable af;
            fmt.parse(s, af, pos);
            int32_t a;
            if (af.getType() == Formattable::kLong) a = af.getLong();
            else errln((UnicodeString)"FAIL: Non-long Formattable returned");
            if (pos.getIndex() == s.size())
            {
                logln((UnicodeString)"  -parse-> " + a);
                if (a != lvalParse[v+ilval])
                errln((UnicodeString)"FAIL: Expected " + lvalParse[v+ilval]);
            }
            else
                errln((UnicodeString)"FAIL: Partial parse (" + pos.getIndex() + " chars) -> " + a);
        }
        ival += val_length;
        ilval += lval_length;
    }
}

// -------------------------------------

// Test the handling of quotes
void
NumberFormatTest::TestQuotes()
{
    UErrorCode status = ZERO_ERROR;
    UnicodeString *pat;
    DecimalFormatSymbols *sym = new DecimalFormatSymbols(Locale::US, status);
    pat = new UnicodeString("a'fo''o'b#");
    DecimalFormat *fmt = new DecimalFormat(*pat, *sym, status);
    UnicodeString s; 
    ((NumberFormat*)fmt)->format(T_INT32(123), s);
    logln((UnicodeString)"Pattern \"" + *pat + "\"");
    logln((UnicodeString)" Format 123 -> " + escape(s));
    if (!(s=="afo'ob123")) 
        errln((UnicodeString)"FAIL: Expected afo'ob123");
    
    s.truncate(0);
    delete fmt;
    delete pat;

    pat = new UnicodeString("a''b#");
    fmt = new DecimalFormat(*pat, *sym, status);
    ((NumberFormat*)fmt)->format(T_INT32(123), s);
    logln((UnicodeString)"Pattern \"" + *pat + "\"");
    logln((UnicodeString)" Format 123 -> " + escape(s));
    if (!(s=="a'b123")) 
        errln((UnicodeString)"FAIL: Expected a'b123");
    delete fmt;
    delete pat;
    delete sym;
}

/**
 * Test the handling of the currency symbol in patterns.
 */
void
NumberFormatTest::TestCurrencySign()
{
    UErrorCode status = ZERO_ERROR;
    DecimalFormatSymbols* sym = new DecimalFormatSymbols(Locale::US, status);
    DecimalFormat *fmt = new DecimalFormat("\xA4#,##0.00;-\xA4#,##0.00", *sym, status);
    UnicodeString s; ((NumberFormat*)fmt)->format(1234.56, s);
    UnicodeString pat;
    logln((UnicodeString)"Pattern \"" + fmt->toPattern(pat) + "\"");
    logln((UnicodeString)" Format " + 1234.56 + " -> " + escape(s));
    if (s != "$1,234.56") errln((UnicodeString)"FAIL: Expected $1,234.56");
    s.truncate(0);
    ((NumberFormat*)fmt)->format(- 1234.56, s);
    logln((UnicodeString)" Format " + (-1234.56) + " -> " + escape(s));
    if (s != "-$1,234.56") errln((UnicodeString)"FAIL: Expected -$1,234.56");
    delete fmt;
    fmt = new DecimalFormat("\xA4\xA4 #,##0.00;\xA4\xA4 -#,##0.00", *sym, status);
    s.truncate(0);
    ((NumberFormat*)fmt)->format(1234.56, s);
    logln((UnicodeString)"Pattern \"" + fmt->toPattern(pat) + "\"");
    logln((UnicodeString)" Format " + 1234.56 + " -> " + escape(s));
    if (s != "USD 1,234.56") errln((UnicodeString)"FAIL: Expected USD 1,234.56");
    s.truncate(0);
    ((NumberFormat*)fmt)->format(-1234.56, s);
    logln((UnicodeString)" Format " + (-1234.56) + " -> " + escape(s));
    if (s != "USD -1,234.56") errln((UnicodeString)"FAIL: Expected USD -1,234.56");
    delete fmt;
    delete sym;
    if (FAILURE(status)) errln((UnicodeString)"FAIL: Status " + (int32_t)status);
}
 
// -------------------------------------

static char toHexString(int32_t i) { return i + (i < 10 ? '0' : ('A' - 10)); }

UnicodeString&
NumberFormatTest::escape(UnicodeString& s)
{
    UnicodeString buf;
    for (int32_t i=0; i<s.size(); ++i)
    {
        UChar c = s[(UTextOffset)i];
        if (c <= (UChar)0x7F) buf += c;
        else {
            buf += '\\'; buf += 'U';
            buf += toHexString((c & 0xF000) >> 12);
            buf += toHexString((c & 0x0F00) >> 8);
            buf += toHexString((c & 0x00F0) >> 4);
            buf += toHexString(c & 0x000F);
        }
    }
    return (s = buf);
}

 
// -------------------------------------
 
/**
 * Test localized currency patterns.
 */
void
NumberFormatTest::TestCurrency()
{
    UErrorCode status = ZERO_ERROR;
    NumberFormat* currencyFmt = NumberFormat::createCurrencyInstance(Locale::CANADA_FRENCH, status);
    UnicodeString s; currencyFmt->format(1.50, s);
    logln((UnicodeString)"Un pauvre ici a..........." + s);
    if (!(s=="1,50 $")) errln((UnicodeString)"FAIL: Expected 1,50 $");
    delete currencyFmt;
    s.truncate(0);
    currencyFmt = NumberFormat::createCurrencyInstance(Locale::GERMANY, status);
    currencyFmt->format(1.50, s);
    logln((UnicodeString)"Un pauvre en Allemagne a.." + s);
    if (!(s=="1,50 DM")) errln((UnicodeString)"FAIL: Expected 1,50 DM");
    delete currencyFmt;
    s.truncate(0);
    currencyFmt = NumberFormat::createCurrencyInstance(Locale::FRANCE, status);
    currencyFmt->format(1.50, s);
    logln((UnicodeString)"Un pauvre en France a....." + s);
    if (!(s=="1,50 F")) errln((UnicodeString)"FAIL: Expected 1,50 F");
    delete currencyFmt;
    if (FAILURE(status)) errln((UnicodeString)"FAIL: Status " + (int32_t)status);
}
 
// -------------------------------------

/**
 * Do rudimentary testing of parsing.
 */
void
NumberFormatTest::TestParse()
{
    UErrorCode status = ZERO_ERROR;
    UnicodeString arg("0");
    DecimalFormat* format = new DecimalFormat("00", status);
    //try {
        Formattable n; format->parse(arg, n, status);
        logln((UnicodeString)"parse(" + arg + ") = " + n.getLong());
        if (n.getType() != Formattable::kLong ||
            n.getLong() != 0) errln((UnicodeString)"FAIL: Expected 0");
    delete format;
    if (FAILURE(status)) errln((UnicodeString)"FAIL: Status " + (int32_t)status);
    //}
    //catch(Exception e) {
    //    errln((UnicodeString)"Exception caught: " + e);
    //}
}
 
// -------------------------------------

/**
 * Test proper rounding by the format method.
 */
void
NumberFormatTest::TestRounding487()
{
    UErrorCode status = ZERO_ERROR;
    NumberFormat *nf = NumberFormat::createInstance(status);
    roundingTest(*nf, 0.00159999, 4, "0.0016");
    roundingTest(*nf, 0.00995, 4, "0.01");

    roundingTest(*nf, 12.3995, 3, "12.4");

    roundingTest(*nf, 12.4999, 0, "12");
    roundingTest(*nf, - 19.5, 0, "-20");
    delete nf;
    if (FAILURE(status)) errln((UnicodeString)"FAIL: Status " + (int32_t)status);
}
 
// -------------------------------------
 
void
NumberFormatTest::roundingTest(NumberFormat& nf, double x, int32_t maxFractionDigits, const char* expected)
{
    nf.setMaximumFractionDigits(maxFractionDigits);
    UnicodeString out; nf.format(x, out);
    logln((UnicodeString)"" + x + " formats with " + maxFractionDigits + " fractional digits to " + out);
    if (!(out==expected)) errln((UnicodeString)"FAIL: Expected " + expected);
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::expect(NumberFormat& fmt, const UnicodeString& str, int32_t n) {
    UErrorCode status = ZERO_ERROR;
    Formattable num;
    fmt.parse(str, num, status);
    CHECK(status, "NumberFormat.parse");
    UnicodeString pat;
    ((DecimalFormat*) &fmt)->toPattern(pat);
    if (num.getType() == Formattable::kLong &&
        num.getLong() == n) {
        logln(UnicodeString("Ok   \"") + str + "\" x " +
              pat + " = " +
              toString(num));
    } else {
        errln(UnicodeString("FAIL \"") + str + "\" x " +
              pat + " = " +
              toString(num) + ", expected " + n + "L");
    }
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::expect(NumberFormat& fmt, const Formattable& n,
                              const UnicodeString& exp) {
    UnicodeString saw;
    FieldPosition pos;
    UErrorCode status = ZERO_ERROR;
    fmt.format(n, saw, pos, status);
    CHECK(status, "format");
    UnicodeString pat;
    ((DecimalFormat*) &fmt)->toPattern(pat);
    if (saw == exp) {
        logln(UnicodeString("Ok   ") + toString(n) + " x " +
              pat + " = \"" +
              saw + "\"");
    } else {
        errln(UnicodeString("FAIL ") + toString(n) + " x " +
              pat + " = \"" +
              saw + "\", expected \"" + exp + "\"");
    }
}

void NumberFormatTest::expect(NumberFormat* fmt, const Formattable& n,
                              const UnicodeString& exp,
                              UErrorCode status) {
    CHECK(status, "construct format");
    expect(*fmt, n, exp);
    delete fmt;
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestExponent() {
    UErrorCode status = ZERO_ERROR;
    DecimalFormatSymbols US(Locale::US, status);
    CHECK(status, "DecimalFormatSymbols constructor");
    DecimalFormat fmt1(UnicodeString("0.###E0"), US, status);
    CHECK(status, "DecimalFormat(0.###E0)");
    DecimalFormat fmt2(UnicodeString("0.###E+0"), US, status);
    CHECK(status, "DecimalFormat(0.###E+0)");
    int32_t n = 1234;
    expect(fmt1, n, "1.234E3");
    expect(fmt2, n, "1.234E+3");
    expect(fmt1, "1.234E3", n);
    expect(fmt1, "1.234E+3", n); // Either format should parse "E+3"
    expect(fmt2, "1.234E+3", n);
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestScientific() {
    UErrorCode status = ZERO_ERROR;
    DecimalFormatSymbols US(Locale::US, status);
    CHECK(status, "DecimalFormatSymbols constructor");

    // Test pattern round-trip
    char* PAT[] = { "#E0", "0.####E0", "00.000E00", "##0.####E000",
                    "0.###E0;[0.###E0]" };
    int32_t PAT_length = sizeof(PAT) / sizeof(PAT[0]);
    int32_t DIGITS[] = {
        // min int, max int, min frac, max frac
        0, 1, 0, 0, // "#E0"
        1, 1, 0, 4, // "0.####E0"
        2, 2, 3, 3, // "00.000E00"
        1, 3, 0, 4, // "##0.####E000"
        1, 1, 0, 3, // "0.###E0;[0.###E0]"
    };
    for (int32_t i=0; i<PAT_length; ++i) {
        UnicodeString pat(PAT[i]);
        DecimalFormat df(pat, US, status);
        CHECK(status, "DecimalFormat constructor");
        UnicodeString pat2;
        df.toPattern(pat2);
        if (pat == pat2) {
            logln(UnicodeString("Ok   Pattern rt \"") +
                  pat + "\" -> \"" +
                  pat2 + "\"");
        } else {
            errln(UnicodeString("FAIL Pattern rt \"") +
                  pat + "\" -> \"" +
                  pat2 + "\"");
        }
        // Make sure digit counts match what we expect
        if (df.getMinimumIntegerDigits() != DIGITS[4*i] ||
            df.getMaximumIntegerDigits() != DIGITS[4*i+1] ||
            df.getMinimumFractionDigits() != DIGITS[4*i+2] ||
            df.getMaximumFractionDigits() != DIGITS[4*i+3]) {
            errln(UnicodeString("FAIL \"" + pat +
                                "\" min/max int; min/max frac = ") +
                  df.getMinimumIntegerDigits() + "/" +
                  df.getMaximumIntegerDigits() + ";" +
                  df.getMinimumFractionDigits() + "/" +
                  df.getMaximumFractionDigits() + ", expect " +
                  DIGITS[4*i] + "/" +
                  DIGITS[4*i+1] + ";" +
                  DIGITS[4*i+2] + "/" +
                  DIGITS[4*i+3]);
        }
    }

    expect(new DecimalFormat("#E0", US, status),
           12345.0,
           "1.2345E4", status);
    expect(new DecimalFormat("0E0", US, status),
           12345.0,
           "1E4", status);
    expect(NumberFormat::createScientificInstance(Locale::US, status),
           12345.678901,
           "1.2345678901E4", status);
    expect(new DecimalFormat("##0.###E0", US, status),
           12345.0,
           "12.34E3", status);
    expect(new DecimalFormat("##0.###E0", US, status),
           12345.00001,
           "12.35E3", status);
    expect(new DecimalFormat("##0.####E0", US, status),
           (int32_t) 12345,
           "12.345E3", status);
    expect(NumberFormat::createScientificInstance(Locale::FRANCE, status),
           12345.678901,
           "1,2345678901E4", status);
    expect(new DecimalFormat("##0.####E0", US, status),
           789.12345e-9,
           "789.12E-9", status);
    expect(new DecimalFormat("##0.####E0", US, status),
           780.e-9,
           "780E-9", status);
    expect(new DecimalFormat(".###E0", US, status),
           45678.0,
           ".457E5", status);
    expect(new DecimalFormat(".###E0", US, status),
           (int32_t) 0,
           ".0E0", status);
    /*
    expect(new DecimalFormat[] { new DecimalFormat("#E0", US),
                                 new DecimalFormat("##E0", US),
                                 new DecimalFormat("####E0", US),
                                 new DecimalFormat("0E0", US),    
                                 new DecimalFormat("00E0", US),   
                                 new DecimalFormat("000E0", US), 
                               },
           new Long(45678000),
           new String[] { "4.5678E7",
                          "45.678E6",
                          "4567.8E4",
                          "5E7",
                          "46E6",  
                          "457E5",
                        }
           );
    !
    ! Unroll this test into individual tests below...
    !
    */
    expect(new DecimalFormat("#E0", US, status),
           (int32_t) 45678000, "4.5678E7", status);
    expect(new DecimalFormat("##E0", US, status),
           (int32_t) 45678000, "45.678E6", status);
    expect(new DecimalFormat("####E0", US, status),
           (int32_t) 45678000, "4567.8E4", status);
    expect(new DecimalFormat("0E0", US, status),
           (int32_t) 45678000, "5E7", status);
    expect(new DecimalFormat("00E0", US, status),
           (int32_t) 45678000, "46E6", status);
    expect(new DecimalFormat("000E0", US, status),
           (int32_t) 45678000, "457E5", status);
    /*
    expect(new DecimalFormat("###E0", US, status),
           new Object[] { new Double(0.0000123), "12.3E-6",
                          new Double(0.000123), "123E-6",
                          new Double(0.00123), "1.23E-3",
                          new Double(0.0123), "12.3E-3",
                          new Double(0.123), "123E-3",
                          new Double(1.23), "1.23E0",
                          new Double(12.3), "12.3E0",
                          new Double(123), "123E0",
                          new Double(1230), "1.23E3",
                         });
    !
    ! Unroll this test into individual tests below...
    !
    */
    expect(new DecimalFormat("###E0", US, status),
           0.0000123, "12.3E-6", status);
    expect(new DecimalFormat("###E0", US, status),
           0.000123, "123E-6", status);
    expect(new DecimalFormat("###E0", US, status),
           0.00123, "1.23E-3", status);
    expect(new DecimalFormat("###E0", US, status),
           0.0123, "12.3E-3", status);
    expect(new DecimalFormat("###E0", US, status),
           0.123, "123E-3", status);
    expect(new DecimalFormat("###E0", US, status),
           1.23, "1.23E0", status);
    expect(new DecimalFormat("###E0", US, status),
           12.3, "12.3E0", status);
    expect(new DecimalFormat("###E0", US, status),
           123.0, "123E0", status);
    expect(new DecimalFormat("###E0", US, status),
           1230.0, "1.23E3", status);
    /*
    expect(new DecimalFormat("0.#E+00", US, status),
           new Object[] { new Double(0.00012), "1.2E-04",
                          new Long(12000),     "1.2E+04",
                         });
    !
    ! Unroll this test into individual tests below...
    !
    */
    expect(new DecimalFormat("0.#E+00", US, status),
           0.00012, "1.2E-04", status);
    expect(new DecimalFormat("0.#E+00", US, status),
           (int32_t) 12000, "1.2E+04", status);
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestPad() {
    UErrorCode status = ZERO_ERROR;
    DecimalFormatSymbols US(Locale::US, status);
    CHECK(status, "DecimalFormatSymbols constructor");

    expect(new DecimalFormat("*^##.##", US, status),
           int32_t(0), "^^^^0", status);
    expect(new DecimalFormat("*^##.##", US, status),
           -1.3, "^-1.3", status);
    expect(new DecimalFormat("##0.0####E0*_ g-m/s^2", US, status),
           int32_t(0), "0.0E0______ g-m/s^2", status);
    expect(new DecimalFormat("##0.0####E0*_ g-m/s^2", US, status),
           1.0/3, "333.333E-3_ g-m/s^2", status);
    expect(new DecimalFormat("##0.0####*_ g-m/s^2", US, status),
           int32_t(0), "0.0______ g-m/s^2", status);
    expect(new DecimalFormat("##0.0####*_ g-m/s^2", US, status),
           1.0/3, "0.33333__ g-m/s^2", status);
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestPatterns2() {
    UErrorCode status = ZERO_ERROR;
    DecimalFormatSymbols US(Locale::US, status);
    CHECK(status, "DecimalFormatSymbols constructor");

    DecimalFormat fmt("#", US, status);
    CHECK(status, "DecimalFormat constructor");

    expectPad(fmt, "*^#", DecimalFormat::kPadBeforePrefix, 1, '^');
    expectPad(fmt, "$*^#", DecimalFormat::kPadAfterPrefix, 2, '^');
    expectPad(fmt, "#*^", DecimalFormat::kPadBeforeSuffix, 1, '^');
    expectPad(fmt, "#$*^", DecimalFormat::kPadAfterSuffix, 2, '^');
    expectPad(fmt, "$*^$#", ILLEGAL);
    expectPad(fmt, "#$*^$", ILLEGAL);
    expectPad(fmt, "'pre'#,##0*x'post'", DecimalFormat::kPadBeforeSuffix,
              12, 'x');
    expectPad(fmt, "''#0*x", DecimalFormat::kPadBeforeSuffix,
              3, 'x');
    expectPad(fmt, "'I''ll'*a###.##", DecimalFormat::kPadAfterPrefix,
              10, 'a');

    fmt.applyPattern("AA#,##0.00ZZ", status);
    CHECK(status, "applyPattern");
    fmt.setPadCharacter('^');

    fmt.setFormatWidth(10);

    fmt.setPadPosition(DecimalFormat::kPadBeforePrefix);
    expectPat(fmt, "*^AA#,##0.00ZZ");

    fmt.setPadPosition(DecimalFormat::kPadBeforeSuffix);
    expectPat(fmt, "AA#,##0.00*^ZZ");

    fmt.setPadPosition(DecimalFormat::kPadAfterSuffix);
    expectPat(fmt, "AA#,##0.00ZZ*^");

    //            12  3456789012
    UnicodeString exp("AA*^#,##0.00ZZ");
    fmt.setFormatWidth(12);
    fmt.setPadPosition(DecimalFormat::kPadAfterPrefix);
    expectPat(fmt, exp);

    fmt.setFormatWidth(13);
    //              12  34567890123
    expectPat(fmt, "AA*^##,##0.00ZZ");

    fmt.setFormatWidth(14);
    //              12  345678901234
    expectPat(fmt, "AA*^###,##0.00ZZ");

    fmt.setFormatWidth(15);
    //              12  3456789012345
    expectPat(fmt, "AA*^####,##0.00ZZ"); // This is the interesting case

    fmt.setFormatWidth(16);
    //              12  34567890123456
    expectPat(fmt, "AA*^#,###,##0.00ZZ");
}

void NumberFormatTest::expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                                 int32_t pos) {
    expectPad(fmt, pat, pos, 0, (UChar)0);
}

void NumberFormatTest::expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                                 int32_t pos, int32_t width, UChar pad) {
    int32_t apos = 0, awidth = 0;
    UChar apad = 0;
    UErrorCode status = ZERO_ERROR;
    fmt.applyPattern(pat, status);
    if (SUCCESS(status)) {
        apos = fmt.getPadPosition();
        awidth = fmt.getFormatWidth();
        apad = fmt.getPadCharacter();
    } else {
        apos = -1;
        awidth = width;
        apad = pad;
    }
    if (apos == pos && awidth == width && apad == pad) {
        logln(UnicodeString("Ok   \"") + pat + "\" pos=" + apos +
              ((pos == ILLEGAL) ? UnicodeString() :
               (UnicodeString(" width=") + awidth + " pad=" + UnicodeString(apad))));
    } else {
        errln(UnicodeString("FAIL \"") + pat + "\" pos=" + apos +
              " width=" + awidth + " pad=" + UnicodeString(apad) +
              ", expected " + pos + " " + width + " " + UnicodeString(pad));
    }
}

void NumberFormatTest::expectPat(DecimalFormat& fmt, const UnicodeString& exp) {
    UnicodeString pat;
    fmt.toPattern(pat);
    if (pat == exp) {
        logln(UnicodeString("Ok   \"") + pat + '"');
    } else {
        errln(UnicodeString("FAIL \"") + pat + "\", expected \"" + exp + '"');
    }
}
