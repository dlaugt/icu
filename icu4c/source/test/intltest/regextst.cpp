/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2002, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

//
//   regextst.cpp
//
//      ICU Regular Expressions test, part of intltest.
//

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "intltest.h"
#include "regextst.h"
#include "uvector.h"
#include "stdlib.h"


//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
RegexTest::RegexTest() 
{
};


RegexTest::~RegexTest()
{
};



void RegexTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite RegexTest: ");
    switch (index) {

        case 0: name = "Basic";
            if (exec) Basic(); 
            break;
        case 1: name = "API_Match";
            if (exec) API_Match(); 
            break;
        case 2: name = "API_Replace";
            if (exec) API_Replace(); 
            break;
        case 3: name = "API_Pattern";
            if (exec) API_Pattern(); 
            break;
        case 4: name = "Extended";
            if (exec) Extended(); 
            break;
        default: name = ""; 
            break; //needed to end loop
    }
}


//---------------------------------------------------------------------------
//
//   Error Checking / Reporting macros used in all of the tests.
//
//---------------------------------------------------------------------------
#define REGEX_CHECK_STATUS {if (U_FAILURE(status)) {errln("RegexTest failure at line %d.  status=%d\n", \
__LINE__, status); return;}}

#define REGEX_ASSERT(expr) {if ((expr)==FALSE) {errln("RegexTest failure at line %d.\n", __LINE__);};}

#define REGEX_ASSERT_FAIL(expr, errcode) {UErrorCode status=U_ZERO_ERROR; (expr);\
if (status!=errcode) {errln("RegexTest failure at line %d.\n", __LINE__);};}

#define REGEX_CHECK_STATUS_L(line) {if (U_FAILURE(status)) {errln( \
    "RegexTest failure at line %d, from %d.  status=%d\n",__LINE__, (line), status); }}

#define REGEX_ASSERT_L(expr, line) {if ((expr)==FALSE) { \
    errln("RegexTest failure at line %d, from %d.", __LINE__, (line)); return;}}



//---------------------------------------------------------------------------
//
//    REGEX_TESTLM       Macro + invocation function to simplify writing quick tests
//                       for the LookingAt() and  Match() functions.
//
//       usage:
//          REGEX_TESTLM("pattern",  "input text",  lookingAt expected, matches expected);
//
//          The expected results are UBool - TRUE or FALSE.
//          The input text is unescaped.  The pattern is not.
//            
//
//---------------------------------------------------------------------------

#define REGEX_TESTLM(pat, text, looking, match) doRegexLMTest(pat, text, looking, match, __LINE__);

UBool RegexTest::doRegexLMTest(char *pat, char *text, UBool looking, UBool match, int line) {
    const UnicodeString pattern(pat);
    const UnicodeString inputText(text);
    UErrorCode          status  = U_ZERO_ERROR;
    UParseError         pe;
    RegexPattern        *REPattern = NULL;
    RegexMatcher        *REMatcher = NULL;
    UBool               retVal     = TRUE;

    UnicodeString patString(pat);
    REPattern = RegexPattern::compile(patString, 0, pe, status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in RegexPattern::compile() at line %d.  Status = %d\n", line, status);
        return FALSE;
    }
    // REPattern->dump();

    UnicodeString inputString(inputText);
    UnicodeString unEscapedInput = inputString.unescape();
    REMatcher = REPattern->matcher(unEscapedInput, status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in REPattern::matcher() at line %d.  Status = %d\n", line, status);
        return FALSE;
    }
  
    UBool actualmatch;
    actualmatch = REMatcher->lookingAt(status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in lookingAt() at line %d.  Status = %d\n", line, status);
        retVal =  FALSE;
    }
    if (actualmatch != looking) {
        errln("RegexTest: wrong return from lookingAt() at line %d.\n", line);
        retVal = FALSE;
    }

    status = U_ZERO_ERROR;
    actualmatch = REMatcher->matches(status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in matches() at line %d.  Status = %d\n", line, status);
        retVal = FALSE;
    }
    if (actualmatch != match) {
        errln("RegexTest: wrong return from matches() at line %d.\n", line);
        retVal = FALSE;
    }

    if (retVal == FALSE) {
        REPattern->dump();
    }

    delete REPattern;
    delete REMatcher;
    return retVal;
}
    



//---------------------------------------------------------------------------
//
//    REGEX_FIND       Macro + invocation function to simplify writing tests
//                       regex tests.
//
//       usage:
//          REGEX_FIND("pattern",  "input text");
//          REGEX_FIND_S("pattern",  "input text",  expected status);
//
//          The input text is unescaped.  The pattern is not.
//          The input text is marked with the expected match positions
//              <0>text  <1> more text </1>   </0>
//          The <n> </n> tags are removed before trying the match.
//          The tags mark the start and end of the match and of any capture groups. 
//            
//
//---------------------------------------------------------------------------

// REGEX_FIND is invoked via a macro, which allows capturing the source file line
//            number for use in error messages.
#define REGEX_FIND(pat, text) regex_find(pat, text, U_ZERO_ERROR, __LINE__);
#define REGEX_FIND_S(pat, text, status) regex_find(pat, text, status, __LINE__);


//  Set a value into a UVector at position specified by a decimal number in
//   a UnicodeString.   This is a utility function needed by the actual test function,
//   which follows.
void set(UVector &vec, int val, UnicodeString index) {
    UErrorCode  status=U_ZERO_ERROR;
    int  idx = 0;
    for (int i=0; i<index.length(); i++) {
        int d=u_charDigitValue(index.charAt(i));
        if (d<0) {return;}
        idx = idx*10 + d;
    }
    while (vec.size()<idx+1) {vec.addElement(-1, status);}
    vec.setElementAt(val, idx);
}
        
void RegexTest::regex_find(char *pat, char *input, UErrorCode expectedStatus, int line) {
    UnicodeString       pattern(pat);
    UnicodeString       inputString(input);
    UnicodeString       unEscapedInput;
    UnicodeString       deTaggedInput;

    UErrorCode          status         = U_ZERO_ERROR;
    UParseError         pe;
    RegexPattern        *parsePat      = NULL;
    RegexMatcher        *parseMatcher  = NULL;
    RegexPattern        *callerPattern = NULL;
    RegexMatcher        *matcher       = NULL;
    UVector             groupStarts(status);
    UVector             groupEnds(status);
    UBool               isMatch;
    UBool               failed         = FALSE;

    //
    //  Compile the caller's pattern
    //
    UnicodeString patString(pat);
    callerPattern = RegexPattern::compile(patString, 0, pe, status);
    if (status != expectedStatus) {
        errln("Line %d: error %x compiling pattern.", line, status);
        goto cleanupAndReturn;
    }
    // callerPattern->dump();

    //
    //  Find the tags in the input data, remove them, and record the group boundary
    //    positions.
    //
    parsePat = RegexPattern::compile("<(/?)([0-9]+)>", 0, pe, status);
    REGEX_CHECK_STATUS_L(line);
    
    unEscapedInput = inputString.unescape();
    parseMatcher = parsePat->matcher(unEscapedInput, status);
    REGEX_CHECK_STATUS_L(line);
    while(parseMatcher->find()) {
        parseMatcher->appendReplacement(deTaggedInput, "", status);
        REGEX_CHECK_STATUS;
        UnicodeString groupNum = parseMatcher->group(2, status);
        if (parseMatcher->group(1, status) == "/") {
            // close tag
            set(groupEnds, deTaggedInput.length(), groupNum);
        } else {
            set(groupStarts, deTaggedInput.length(), groupNum);
        }
    }
    parseMatcher->appendTail(deTaggedInput);
    REGEX_ASSERT_L(groupStarts.size() == groupEnds.size(), line);


    //
    // Do a find on the de-tagged input using the caller's pattern
    //
    matcher = callerPattern->matcher(deTaggedInput, status);
    REGEX_CHECK_STATUS_L(line);
    isMatch = matcher->find();

    //
    // Match up the groups from the find() with the groups from the tags
    //

    // number of tags should match number of groups from find operation.
    // matcher->groupCount does not include group 0, the entire match, hence the +1.
    if (isMatch == FALSE && groupStarts.size() != 0) {
        errln("Error at line %d:  Match expected, but none found.\n", line);
        failed = true;
        goto cleanupAndReturn;
    }
    int i;
    for (i=0; i<=matcher->groupCount(); i++) {
        int32_t  expectedStart = (i >= groupStarts.size()? -1 : groupStarts.elementAti(i));
        if (matcher->start(i, status) != expectedStart) {
            errln("Error at line %d: incorrect start position for group %d.  Expected %d, got %d",
                line, i, expectedStart, matcher->start(i, status));
            failed = TRUE;
            goto cleanupAndReturn;  // Good chance of subsequent bogus errors.  Stop now.
        }
        int32_t  expectedEnd = (i >= groupEnds.size()? -1 : groupEnds.elementAti(i));
        if (matcher->end(i, status) != expectedEnd) {
            errln("Error at line %d: incorrect end position for group %d.  Expected %d, got %d",
                line, i, expectedEnd, matcher->end(i, status));
            failed = TRUE;
            // Error on end position;  keep going; real error is probably yet to come as group
            //   end positions work from end of the input data towards the front.
        }
    }
    if ( matcher->groupCount()+1 < groupStarts.size()) {
        errln("Error at line %d: Expected %d capture groups, found %d.", 
            line, groupStarts.size()-1, matcher->groupCount());
            failed = TRUE;
    }

cleanupAndReturn:
    if (failed) {
        callerPattern->dump();
    }
    delete parseMatcher;
    delete parsePat;
    delete matcher;
    delete callerPattern;
}
 

//---------------------------------------------------------------------------
//
//      Basic      Check for basic functionality of regex pattern matching.
//                 Avoid the use of REGEX_FIND test macro, which has
//                 substantial dependencies on basic Regex functionality.
//
//---------------------------------------------------------------------------
void RegexTest::Basic() {


//
// Debug - slide failing test cases early
//
#if 0
    {
    REGEX_FIND("\\D+", "<0>non digits</0>");
    }
    exit(1);
#endif


    //
    // Pattern with parentheses
    //
    REGEX_TESTLM("st(abc)ring", "stabcring thing", TRUE,  FALSE);
    REGEX_TESTLM("st(abc)ring", "stabcring",       TRUE,  TRUE);
    REGEX_TESTLM("st(abc)ring", "stabcrung",       FALSE, FALSE);

    //
    // Patterns with *
    //
    REGEX_TESTLM("st(abc)*ring", "string", TRUE, TRUE);
    REGEX_TESTLM("st(abc)*ring", "stabcring", TRUE, TRUE);
    REGEX_TESTLM("st(abc)*ring", "stabcabcring", TRUE, TRUE);
    REGEX_TESTLM("st(abc)*ring", "stabcabcdring", FALSE, FALSE);
    REGEX_TESTLM("st(abc)*ring", "stabcabcabcring etc.", TRUE, FALSE);

    REGEX_TESTLM("a*", "",  TRUE, TRUE);
    REGEX_TESTLM("a*", "b", TRUE, FALSE);


    //
    //  Patterns with "."
    //
    REGEX_TESTLM(".", "abc", TRUE, FALSE);
    REGEX_TESTLM("...", "abc", TRUE, TRUE);
    REGEX_TESTLM("....", "abc", FALSE, FALSE);
    REGEX_TESTLM(".*", "abcxyz123", TRUE, TRUE);
    REGEX_TESTLM("ab.*xyz", "abcdefghij", FALSE, FALSE);
    REGEX_TESTLM("ab.*xyz", "abcdefg...wxyz", TRUE, TRUE);
    REGEX_TESTLM("ab.*xyz", "abcde...wxyz...abc..xyz", TRUE, TRUE);
    REGEX_TESTLM("ab.*xyz", "abcde...wxyz...abc..xyz...", TRUE, FALSE);

    //
    //  Patterns with * applied to chars at end of literal string
    //
    REGEX_TESTLM("abc*", "ab", TRUE, TRUE);
    REGEX_TESTLM("abc*", "abccccc", TRUE, TRUE);

    //
    //  Supplemental chars match as single chars, not a pair of surrogates.
    //
    REGEX_TESTLM(".", "\\U00011000", TRUE, TRUE);
    REGEX_TESTLM("...", "\\U00011000x\\U00012002", TRUE, TRUE);
    REGEX_TESTLM("...", "\\U00011000x\\U00012002y", TRUE, FALSE);


    //
    //  UnicodeSets in the pattern
    //
    REGEX_TESTLM("[1-6]", "1", TRUE, TRUE);
    REGEX_TESTLM("[1-6]", "3", TRUE, TRUE);
    REGEX_TESTLM("[1-6]", "7", FALSE, FALSE);
    REGEX_TESTLM("a[1-6]", "a3", TRUE, TRUE);
    REGEX_TESTLM("a[1-6]", "a3", TRUE, TRUE);
    REGEX_TESTLM("a[1-6]b", "a3b", TRUE, TRUE);

    REGEX_TESTLM("a[0-9]*b", "a123b", TRUE, TRUE);
    REGEX_TESTLM("a[0-9]*b", "abc", TRUE, FALSE);
    REGEX_TESTLM("[\\p{Nd}]*", "123456", TRUE, TRUE);
    REGEX_TESTLM("[\\p{Nd}]*", "a123456", TRUE, FALSE);   // note that * matches 0 occurences.
    REGEX_TESTLM("[a][b][[:Zs:]]*", "ab   ", TRUE, TRUE);

    //
    //   OR operator in patterns
    //
    REGEX_TESTLM("(a|b)", "a", TRUE, TRUE);
    REGEX_TESTLM("(a|b)", "b", TRUE, TRUE);
    REGEX_TESTLM("(a|b)", "c", FALSE, FALSE);
    REGEX_TESTLM("a|b", "b", TRUE, TRUE);

    REGEX_TESTLM("(a|b|c)*", "aabcaaccbcabc", TRUE, TRUE);
    REGEX_TESTLM("(a|b|c)*", "aabcaaccbcabdc", TRUE, FALSE);
    REGEX_TESTLM("(a(b|c|d)(x|y|z)*|123)", "ac", TRUE, TRUE);
    REGEX_TESTLM("(a(b|c|d)(x|y|z)*|123)", "123", TRUE, TRUE);
    REGEX_TESTLM("(a|(1|2)*)(b|c|d)(x|y|z)*|123", "123", TRUE, TRUE);
    REGEX_TESTLM("(a|(1|2)*)(b|c|d)(x|y|z)*|123", "222211111czzzzw", TRUE, FALSE);

    //
    //  +
    //
    REGEX_TESTLM("ab+", "abbc", TRUE, FALSE);
    REGEX_TESTLM("ab+c", "ac", FALSE, FALSE);
    REGEX_TESTLM("b+", "", FALSE, FALSE);
    REGEX_TESTLM("(abc|def)+", "defabc", TRUE, TRUE);
    REGEX_TESTLM(".+y", "zippity dooy dah ", TRUE, FALSE);
    REGEX_TESTLM(".+y", "zippity dooy", TRUE, TRUE);

    //
    //   ?
    //
    REGEX_TESTLM("ab?", "ab", TRUE, TRUE);
    REGEX_TESTLM("ab?", "a", TRUE, TRUE);
    REGEX_TESTLM("ab?", "ac", TRUE, FALSE);
    REGEX_TESTLM("ab?", "abb", TRUE, FALSE);
    REGEX_TESTLM("a(b|c)?d", "abd", TRUE, TRUE);
    REGEX_TESTLM("a(b|c)?d", "acd", TRUE, TRUE);
    REGEX_TESTLM("a(b|c)?d", "ad", TRUE, TRUE);
    REGEX_TESTLM("a(b|c)?d", "abcd", FALSE, FALSE);
    REGEX_TESTLM("a(b|c)?d", "ab", FALSE, FALSE);

    //
    //  Escape sequences that become single literal chars, handled internally
    //   by ICU's Unescape.
    //
    
    // REGEX_TESTLM("\101\142", "Ab", TRUE, TRUE);      // Octal     TODO: not implemented yet.
    REGEX_TESTLM("\\a", "\\u0007", TRUE, TRUE);        // BEL
    // REGEX_TESTLM("\\cL", "\\u000c", TRUE, TRUE);       // Control-L (or whatever) TODO: bug in Unescape
    // REGEX_TESTLM("\\e", "\\u001b", TRUE, TRUE);        // Escape  TODO: bug in Unescape
    REGEX_TESTLM("\\f", "\\u000c", TRUE, TRUE);        // Form Feed
    REGEX_TESTLM("\\n", "\\u000a", TRUE, TRUE);        // new line
    REGEX_TESTLM("\\r", "\\u000d", TRUE, TRUE);        //  CR
    REGEX_TESTLM("\\t", "\\u0009", TRUE, TRUE);        // Tab
    REGEX_TESTLM("\\u1234", "\\u1234", TRUE, TRUE);       
    REGEX_TESTLM("\\U00001234", "\\u1234", TRUE, TRUE);       

    REGEX_TESTLM(".*\\Ax", "xyz", TRUE, FALSE);  //  \A matches only at the beginning of input
    REGEX_TESTLM(".*\\Ax", " xyz", FALSE, FALSE);  //  \A matches only at the beginning of input

    // Escape of special chars in patterns
    REGEX_TESTLM("\\\\\\|\\(\\)\\[\\{\\~\\$\\*\\+\\?\\.", "\\\\|()[{~$*+?.", TRUE, TRUE);       


};


//---------------------------------------------------------------------------
//
//      API_Match   Test that the API for class RegexMatcher 
//                  is present and nominally working, but excluding functions
//                  implementing replace operations.
//
//---------------------------------------------------------------------------
void RegexTest::API_Match() {
    UParseError         pe;
    UErrorCode          status=U_ZERO_ERROR;
    int32_t             flags = 0;

    //
    // Debug - slide failing test cases early
    //
#if 0
    {
    }
    return;
#endif

    //
    // Simple pattern compilation
    //
    {
        UnicodeString       re("abc");
        RegexPattern        *pat2;
        pat2 = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;
        
        UnicodeString inStr1 = "abcdef this is a test";
        UnicodeString instr2 = "not abc";
        UnicodeString empty  = "";
        
        
        //
        // Matcher creation and reset.
        //
        RegexMatcher *m1 = pat2->matcher(inStr1, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(m1->lookingAt(status) == TRUE); 
        REGEX_ASSERT(m1->input() == inStr1);
        m1->reset(instr2);
        REGEX_ASSERT(m1->lookingAt(status) == FALSE);
        REGEX_ASSERT(m1->input() == instr2);
        m1->reset(inStr1);
        REGEX_ASSERT(m1->input() == inStr1);
        REGEX_ASSERT(m1->lookingAt(status) == TRUE);
        m1->reset(empty);
        REGEX_ASSERT(m1->lookingAt(status) == FALSE);
        REGEX_ASSERT(m1->input() == empty);
        REGEX_ASSERT(&m1->pattern() == pat2);
        delete m1;
        delete pat2;
    }


    //
    // Capture Group. 
    //     RegexMatcher::start();
    //     RegexMatcher::end();
    //     RegexMatcher::groupCount();
    //
    {
        int32_t             flags=0;
        UParseError         pe;
        UErrorCode          status=U_ZERO_ERROR;

        UnicodeString       re("01(23(45)67)(.*)");
        RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;
        UnicodeString data = "0123456789";
        
        RegexMatcher *matcher = pat->matcher(data, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(matcher->lookingAt(status) == TRUE); 
        int  matchStarts[] = {0,  2, 4, 8};
        int  matchEnds[]   = {10, 8, 6, 10};
        int i;
        for (i=0; i<4; i++) {
            int32_t actualStart = matcher->start(i, status);
            REGEX_CHECK_STATUS;
            if (actualStart != matchStarts[i]) {
                errln("RegexTest failure at line %d, index %d.  Expected %d, got %d\n",
                    __LINE__, i, matchStarts[i], actualStart);
            }
            int32_t actualEnd = matcher->end(i, status);
            REGEX_CHECK_STATUS;
            if (actualEnd != matchEnds[i]) {
                errln("RegexTest failure at line %d index %d.  Expected %d, got %d\n",
                    __LINE__, i, matchEnds[i], actualEnd);
            }
        }

        REGEX_ASSERT(matcher->start(0, status) == matcher->start(status));
        REGEX_ASSERT(matcher->end(0, status) == matcher->end(status));

        REGEX_ASSERT_FAIL(matcher->start(-1, status), U_INDEX_OUTOFBOUNDS_ERROR);
        REGEX_ASSERT_FAIL(matcher->start( 4, status), U_INDEX_OUTOFBOUNDS_ERROR);
        matcher->reset();
        REGEX_ASSERT_FAIL(matcher->start( 0, status), U_REGEX_INVALID_STATE);

        matcher->lookingAt(status);
        REGEX_ASSERT(matcher->group(status)    == "0123456789");
        REGEX_ASSERT(matcher->group(0, status) == "0123456789");
        REGEX_ASSERT(matcher->group(1, status) == "234567"    );
        REGEX_ASSERT(matcher->group(2, status) == "45"        );
        REGEX_ASSERT(matcher->group(3, status) == "89"        );
        REGEX_CHECK_STATUS;
        REGEX_ASSERT_FAIL(matcher->group(-1, status), U_INDEX_OUTOFBOUNDS_ERROR);
        REGEX_ASSERT_FAIL(matcher->group( 4, status), U_INDEX_OUTOFBOUNDS_ERROR);
        matcher->reset();
        REGEX_ASSERT_FAIL(matcher->group( 0, status), U_REGEX_INVALID_STATE);

        delete matcher;
        delete pat;

    }

    //
    //  find
    //
    {
        int32_t             flags=0;
        UParseError         pe;
        UErrorCode          status=U_ZERO_ERROR;

        UnicodeString       re("abc");
        RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;
        UnicodeString data = ".abc..abc...abc..";
        //                    012345678901234567
        
        RegexMatcher *matcher = pat->matcher(data, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 1);
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 6);
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 12);
        REGEX_ASSERT(matcher->find() == FALSE);
        REGEX_ASSERT(matcher->find() == FALSE);

        matcher->reset();
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 1);

        REGEX_ASSERT(matcher->find(0, status));
        REGEX_ASSERT(matcher->start(status) == 1);
        REGEX_ASSERT(matcher->find(1, status));
        REGEX_ASSERT(matcher->start(status) == 1);
        REGEX_ASSERT(matcher->find(2, status));
        REGEX_ASSERT(matcher->start(status) == 6);
        REGEX_ASSERT(matcher->find(12, status));
        REGEX_ASSERT(matcher->start(status) == 12);
        REGEX_ASSERT(matcher->find(13, status) == FALSE);
        REGEX_ASSERT(matcher->find(16, status) == FALSE);
        REGEX_ASSERT_FAIL(matcher->start(status), U_REGEX_INVALID_STATE);
        REGEX_CHECK_STATUS;

        REGEX_ASSERT_FAIL(matcher->find(-1, status), U_INDEX_OUTOFBOUNDS_ERROR);
        REGEX_ASSERT_FAIL(matcher->find(17, status), U_INDEX_OUTOFBOUNDS_ERROR);

        REGEX_ASSERT(matcher->groupCount() == 0);

        delete matcher;
        delete pat;
    }

    //
    //  Replace
    //
    {
        int32_t             flags=0;
        UParseError         pe;
        UErrorCode          status=U_ZERO_ERROR;

        UnicodeString       re("abc");
        RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;
        UnicodeString data = ".abc..abc...abc..";
        //                    012345678901234567
        RegexMatcher *matcher = pat->matcher(data, status);

        //
        //  Plain vanilla matches.
        //
        UnicodeString  dest;
        dest = matcher->replaceFirst("yz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == ".yz..abc...abc..");

        dest = matcher->replaceAll("yz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == ".yz..yz...yz..");

        //
        //  Plain vanilla non-matches.
        //
        UnicodeString d2 = ".abx..abx...abx..";
        matcher->reset(d2);
        dest = matcher->replaceFirst("yz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == ".abx..abx...abx..");

        dest = matcher->replaceAll("yz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == ".abx..abx...abx..");

        //
        // Empty source string
        //
        UnicodeString d3 = "";
        matcher->reset(d3);
        dest = matcher->replaceFirst("yz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == "");

        dest = matcher->replaceAll("yz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == "");

        //
        // Empty substitution string
        //
        matcher->reset(data);              // ".abc..abc...abc.."
        dest = matcher->replaceFirst("", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == "...abc...abc..");

        dest = matcher->replaceAll("", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == "........");

        //
        // match whole string
        //
        UnicodeString d4 = "abc";
        matcher->reset(d4);   
        dest = matcher->replaceFirst("xyz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == "xyz");

        dest = matcher->replaceAll("xyz", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == "xyz");

        //
        // Capture Group, simple case
        //
        UnicodeString       re2("a(..)");
        RegexPattern *pat2 = RegexPattern::compile(re2, flags, pe, status);
        REGEX_CHECK_STATUS;
        UnicodeString d5 = "abcdefg";
        RegexMatcher *matcher2 = pat2->matcher(d5, status);
        REGEX_CHECK_STATUS;
        dest = matcher2->replaceFirst("$1$1", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(dest == "bcbcdefg");
      
        // TODO:  need more through testing of capture substitutions.


        //
        //  Non-Grouping parentheses
        //

    }


        
}






//---------------------------------------------------------------------------
//
//      API_Replace        API test for class RegexMatcher, testing the 
//                         Replace family of functions.
//
//---------------------------------------------------------------------------
void RegexTest::API_Replace() {
    //
    //  Replace
    //
    int32_t             flags=0;
    UParseError         pe;
    UErrorCode          status=U_ZERO_ERROR;
    
    UnicodeString       re("abc");
    RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
    REGEX_CHECK_STATUS;
    UnicodeString data = ".abc..abc...abc..";
    //                    012345678901234567
    RegexMatcher *matcher = pat->matcher(data, status);
    
    //
    //  Plain vanilla matches.
    //
    UnicodeString  dest;
    dest = matcher->replaceFirst("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".yz..abc...abc..");
    
    dest = matcher->replaceAll("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".yz..yz...yz..");
    
    //
    //  Plain vanilla non-matches.
    //
    UnicodeString d2 = ".abx..abx...abx..";
    matcher->reset(d2);
    dest = matcher->replaceFirst("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".abx..abx...abx..");
    
    dest = matcher->replaceAll("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".abx..abx...abx..");
    
    //
    // Empty source string
    //
    UnicodeString d3 = "";
    matcher->reset(d3);
    dest = matcher->replaceFirst("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "");
    
    dest = matcher->replaceAll("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "");
    
    //
    // Empty substitution string
    //
    matcher->reset(data);              // ".abc..abc...abc.."
    dest = matcher->replaceFirst("", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "...abc...abc..");
    
    dest = matcher->replaceAll("", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "........");
    
    //
    // match whole string
    //
    UnicodeString d4 = "abc";
    matcher->reset(d4);   
    dest = matcher->replaceFirst("xyz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "xyz");
    
    dest = matcher->replaceAll("xyz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "xyz");
    
    //
    // Capture Group, simple case
    //
    UnicodeString       re2("a(..)");
    RegexPattern *pat2 = RegexPattern::compile(re2, flags, pe, status);
    REGEX_CHECK_STATUS;
    UnicodeString d5 = "abcdefg";
    RegexMatcher *matcher2 = pat2->matcher(d5, status);
    REGEX_CHECK_STATUS;
    dest = matcher2->replaceFirst("$1$1", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "bcbcdefg");
    
    // TODO:  need more through testing of capture substitutions.
    
    
    //
    //  Non-Grouping parentheses
    //
    
}


//---------------------------------------------------------------------------
//
//      API_Pattern       Test that the API for class RegexPattern is
//                        present and nominally working.
//
//---------------------------------------------------------------------------
void RegexTest::API_Pattern() {
    RegexPattern        pata;    // Test default constructor to not crash.
    RegexPattern        patb;

    REGEX_ASSERT(pata == patb);
    REGEX_ASSERT(pata == pata);

    UnicodeString re1("abc[a-l][m-z]");
    UnicodeString re2("def");
    UErrorCode    status = U_ZERO_ERROR;
    UParseError   pe;

    RegexPattern        *pat1 = RegexPattern::compile(re1, 0, pe, status);
    RegexPattern        *pat2 = RegexPattern::compile(re2, 0, pe, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(*pat1 == *pat1);
    REGEX_ASSERT(*pat1 != pata);

    // Assign
    patb = *pat1;
    REGEX_ASSERT(patb == *pat1);

    // Copy Construct
    RegexPattern patc(*pat1);
    REGEX_ASSERT(patc == *pat1);
    REGEX_ASSERT(patb == patc);
    REGEX_ASSERT(pat1 != pat2);
    patb = *pat2;
    REGEX_ASSERT(patb != patc);
    REGEX_ASSERT(patb == *pat2);

    // Compile with no flags.
    RegexPattern         *pat1a = RegexPattern::compile(re1, pe, status);
    REGEX_ASSERT(*pat1a == *pat1);

    // Compile with different flags should be not equal
    RegexPattern        *pat1b = RegexPattern::compile(re1, UREGEX_CASE_INSENSITIVE, pe, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(*pat1b != *pat1a);
    REGEX_ASSERT(pat1b->flags() == UREGEX_CASE_INSENSITIVE);
    REGEX_ASSERT(pat1a->flags() == 0);

    // clone
    RegexPattern *pat1c = pat1b->clone();
    REGEX_ASSERT(*pat1b == *pat1c);
    REGEX_ASSERT(*pat1a != *pat1c);


    // TODO:  Actually do some matches with the cloned/copied/assigned patterns.



    delete pat1c;
    delete pat1b;
    delete pat1a;
    delete pat1;
    delete pat2;

    //
    //   matches convenience API
    //
    REGEX_ASSERT(RegexPattern::matches(".*", "random input", pe, status) == TRUE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches("abc", "random input", pe, status) == FALSE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches(".*nput", "random input", pe, status) == TRUE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches("random input", "random input", pe, status) == TRUE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches(".*u", "random input", pe, status) == FALSE);
    REGEX_CHECK_STATUS;
    status = U_INDEX_OUTOFBOUNDS_ERROR;
    REGEX_ASSERT(RegexPattern::matches("abc", "abc", pe, status) == FALSE);
    REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);


    //
    // Split()
    //
    status = U_ZERO_ERROR;
    pat1 = RegexPattern::compile(" +",  pe, status);
    REGEX_CHECK_STATUS;
    UnicodeString  fields[10];

    int32_t n;
    n = pat1->split("Now is the time", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==4);
    REGEX_ASSERT(fields[0]=="Now");
    REGEX_ASSERT(fields[1]=="is");
    REGEX_ASSERT(fields[2]=="the");
    REGEX_ASSERT(fields[3]=="time");
    REGEX_ASSERT(fields[4]=="");

    n = pat1->split("Now is the time", fields, 2, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==2);
    REGEX_ASSERT(fields[0]=="Now");
    REGEX_ASSERT(fields[1]=="is the time");
    REGEX_ASSERT(fields[2]=="the");   // left over from previous test

    fields[1] = "*";
    n = pat1->split("Now is the time", fields, 1, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==1);
    REGEX_ASSERT(fields[0]=="Now is the time");
    REGEX_ASSERT(fields[1]=="*");

    n = pat1->split("    Now       is the time   ", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==5);
    REGEX_ASSERT(fields[0]=="");
    REGEX_ASSERT(fields[1]=="Now");
    REGEX_ASSERT(fields[2]=="is");
    REGEX_ASSERT(fields[3]=="the");
    REGEX_ASSERT(fields[4]=="time");
    REGEX_ASSERT(fields[5]=="");

    n = pat1->split("     ", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==1);
    REGEX_ASSERT(fields[0]=="");

    fields[0] = "foo";
    n = pat1->split("", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==0);
    REGEX_ASSERT(fields[0]=="foo");


}



//---------------------------------------------------------------------------
//
//      Extended       A more thorough check for features of regex patterns
//
//---------------------------------------------------------------------------
void RegexTest::Extended() {
    // Capturing parens
    REGEX_FIND(".(..).", "<0>a<1>bc</1>d</0>"); 
    REGEX_FIND(".*\\A( +hello)", "<0><1>      hello</1></0>"); 
    REGEX_FIND("(hello)|(goodbye)", "<0><1>hello</1></0>");
    REGEX_FIND("(hello)|(goodbye)", "<0><2>goodbye</2></0>");
    REGEX_FIND("abc( +(  inner(X?) +)  xyz)", "leading cruft <0>abc<1>     <2>  inner<3></3>    </2>  xyz</1></0> cruft");

    // Non-capturing parens (?: stuff).   Groups, but does not capture.
    REGEX_FIND("(?:abc)*(tail)", "<0>abcabcabc<1>tail</1></0>");

    // Non-greedy  *? quantifier
    REGEX_FIND(".*?(abc)", "<0>    abx    <1>abc</1></0> abc abc abc");
    REGEX_FIND(".*(abc)",  "<0>    abx     abc abc abc <1>abc</1></0>");

    REGEX_FIND(  "((?:abc |xyz )*?)abc ",  "<0><1>xyz </1>abc </0>abc abc ");
    REGEX_FIND(  "((?:abc |xyz )*)abc ",   "<0><1>xyz abc abc </1>abc </0>");

    // Non-greedy  +? quantifier
    REGEX_FIND( "(a+?)(a*)", "<0><1>a</1><2>aaaaaaaaaaaa</2></0>");
    REGEX_FIND( "(a+)(a*)", "<0><1>aaaaaaaaaaaaa</1><2></2></0>");

    REGEX_FIND( "((ab)+?)((ab)*)", "<0><1><2>ab</2></1><3>ababababab<4>ab</4></3></0>");
    REGEX_FIND( "((ab)+)((ab)*)", "<0><1>abababababab<2>ab</2></1><3></3></0>");

    // Non-greedy ?? quantifier
    REGEX_FIND( "(ab)(ab)\?\?(ab)\?\?(ab)\?\?(ab)\?\?c", 
                "<0><1>ab</1><4>ab</4><5>ab</5>c</0>");

    // Unicode Properties as naked elements in a pattern
    REGEX_FIND( "\\p{Lu}+", "here we go ... <0>ABC</0> and no more.");
    REGEX_FIND( "(\\p{L}+)(\\P{L}*?) (\\p{Zs}*)",  "7999<0><1>letters</1><2>4949%^&*(</2> <3>   </3></0>");

    // \w and \W
    REGEX_FIND( "\\w+", "  $%^&*( <0>hello123</0>%^&*(");
    REGEX_FIND( "\\W+", "<0>  $%^&*( </0>hello123%^&*(");

    // \b \B
    REGEX_FIND( ".*?\\b(.).*", "<0>  $%^&*( <1>h</1>ello123%^&*()gxx</0>");

                 // Finds first chars of up to 5 words
    REGEX_FIND( "(?:.*?\\b(\\w))?(?:.*?\\b(\\w))?(?:.*?\\b(\\w))?(?:.*?\\b(\\w))?(?:.*?\\b(\\w))?",
        "<0><1>T</1>the <2>q</2>ick <3>b</3>rown <4>f</4></0>ox");
    REGEX_FIND( "H.*?((?:\\B.)+)", "<0>H<1>ello</1></0> ");
    REGEX_FIND( ".*?((?:\\B.)+).*?((?:\\B.)+).*?((?:\\B.)+)",
        "<0>H<1>ello</1> <2>    </2>g<3>oodbye</3></0> ");

    REGEX_FIND("(?:.*?\\b(.))?(?:.*?\\b(.))?(?:.*?\\b(.))?(?:.*?\\b(.))?(?:.*?\\b(.))?.*",
        "<0>   \\u0301 \\u0301<1>A</1>\\u0302BC\\u0303\\u0304<2> </2>\\u0305 \\u0306"
        "<3>X</3>\\u0307Y\\u0308</0>");

    // . does not match new-lines
    REGEX_FIND(".", "\\u000a\\u000d\\u0085\\u000c\\u2028\\u2029<0>X</0>\\u000aY");
    REGEX_FIND("A.", "A\\u000a ");  // no match

    // \d for decimal digits
    REGEX_FIND("\\d*", "<0>0123456789\\u0660\\u06F9\\u0969\\u0A66\\u1369"
        "\\u17E2\\uFF10\\U0001D7CE\\U0001D7FF</0>non-digits");  
    REGEX_FIND("\\D+", "<0>non digits</0>");
    REGEX_FIND("\\D*(\\d*)(\\D*)", "<0>non-digits<1>3456666</1><2>more non digits</2></0>");

}



