//
//  regexcmp.h
//
//  Copyright (C) 2002, International Business Machines Corporation and others.
//  All Rights Reserved.
//
//  This file contains declarations for the class RegexCompile
//
//  This class is internal to the regular expression implementation.
//  For the public Regular Expression API, see the file "unicode/regex.h"
//


#ifndef RBBISCAN_H
#define RBBISCAN_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/uobject.h"
#include "unicode/uniset.h"
#include "unicode/parseerr.h"
#include "uhash.h"
#include "uvector.h"



U_NAMESPACE_BEGIN


//--------------------------------------------------------------------------------
//
//  class RegexCompile    Contains the regular expression compiler.
//
//--------------------------------------------------------------------------------
static const int    kStackSize = 100;               // The size of the state stack for
                                                    //   pattern parsing.  Corresponds roughly
                                                    //   to the depth of parentheses nesting
                                                    //   that is allowed in the rules.

enum EParseAction {dummy01, dummy02};               // Placeholder enum for the specifier for
                                                    //   actions that are specified in the
                                                    //   rule parsing state table.
struct  RegexTableEl;
class   RegexPattern;


class RegexCompile : public UMemory {
public:

    struct RegexPatternChar {
        UChar32             fChar;
        UBool               fQuoted;
    };

    RegexCompile(UErrorCode &e);
    
    void       compile(RegexPattern &rxp, const UnicodeString &pat, UParseError &pp, UErrorCode &e);


    virtual    ~RegexCompile();

    void        nextChar(RegexPatternChar &c);      // Get the next char from the input stream.

    static void cleanup();                       // Memory cleanup


private:

    UBool       doParseActions(EParseAction a);
    void        error(UErrorCode e);                   // error reporting convenience function.

    UChar32     nextCharLL();
    UChar32     peekCharLL();
    UnicodeSet  *scanSet();
    UnicodeSet  *scanProp();
    void        handleCloseParen();
    int32_t     blockTopLoc(UBool reserve);          // Locate a position in the compiled pattern
                                                     //  at the top of the just completed block
                                                     //  or operation, and optionally ensure that
                                                     //  there is space to add an opcode there.
    void        compileSet(UnicodeSet *theSet);      // Generate the compiled pattern for
                                                     //   a reference to a UnicodeSet.
    void        compileInterval(int32_t InitOp,       // Generate the code for a {min,max} quantifier.
                               int32_t LoopOp);
    void        literalChar();                       // Compile a literal char
    void        fixLiterals(UBool split=FALSE);      // Fix literal strings.
    void        insertOp(int32_t where);             // Open up a slot for a new op in the
                                                     //   generated code at the specified location.
    UBool       possibleNullMatch(int32_t start,     // Test a range of compiled pattern for
                                  int32_t end);      //   for possibly matching an empty string.


    UErrorCode                    *fStatus;
    RegexPattern                  *fRXPat;
    UParseError                   *fParseErr;

    //
    //  Data associated with low level character scanning
    //
    int32_t                       fScanIndex;        // Index of current character being processed
                                                     //   in the rule input string.
    int32_t                       fNextIndex;        // Index of the next character, which
                                                     //   is the first character not yet scanned.
    UBool                         fQuoteMode;        // Scan is in a quoted region
    UBool                         fFreeForm;         // Scan mode is free-form, ignore spaces.
    int                           fLineNum;          // Line number in input file.
    int                           fCharNum;          // Char position within the line.
    UChar32                       fLastChar;         // Previous char, needed to count CR-LF
                                                     //   as a single line, not two.
    UChar32                       fPeekChar;         // Saved char, if we've scanned ahead.


    RegexPatternChar              fC;                // Current char for parse state machine
                                                     //   processing.

    int32_t                       fStringOpStart;    // While a literal string is being scanned
                                                     //   holds the start index within RegexPattern.
                                                     //   fLiteralText where the string is being stored.

    RegexTableEl                  **fStateTable;     // State Transition Table for regex Rule
                                                     //   parsing.  index by p[state][char-class]

    uint16_t                      fStack[kStackSize];  // State stack, holds state pushes
    int                           fStackPtr;           //  and pops as specified in the state
                                                       //  transition rules.

    int32_t                       fPatternLength;    // Length of the input pattern string.

    UVector32                     fParenStack;       // parentheses stack.  Each frame consists of
                                                     //   the positions of compiled pattern operations
                                                     //   needing fixup, followed by negative value.  The  
                                                     //   first entry in each frame is the position of the
                                                     //   spot reserved for use when a quantifier
                                                     //   needs to add a SAVE at the start of a (block)
                                                     //   The negative value (-1, -2,...) indicates
                                                     //   the kind of paren that opened the frame.  Some
                                                     //   need special handling on close.


    int32_t                       fMatchOpenParen;   // The position in the compiled pattern
                                                     //   of the slot reserved for a state save
                                                     //   at the start of the most recently processed
                                                     //   parenthesized block.
    int32_t                       fMatchCloseParen;  // The position in the pattern of the first
                                                     //   location after the most recently processed
                                                     //   parenthesized block.

    int32_t                       fIntervalLow;      // {lower, upper} interval quantifier values.
    int32_t                       fIntervalUpper;    // Placed here temporarily, when pattern is
                                                     //   initially scanned.  Each new interval
                                                     //   encountered overwrites these values.

                                                     //   -1 for the upper interval value means none
                                                     //   was specified (unlimited occurences.)

};

U_NAMESPACE_END
#endif   // !UCONFIG_NO_REGULAR_EXPRESSIONS
#endif   // RBBISCAN_H
