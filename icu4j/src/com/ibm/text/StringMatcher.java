/*
 *******************************************************************************
 * Copyright (C) 2001, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/text/Attic/StringMatcher.java,v $ 
 * $Date: 2001/10/30 18:04:08 $ 
 * $Revision: 1.3 $
 *
 *****************************************************************************************
 */
package com.ibm.text;

class StringMatcher implements UnicodeMatcher {

    private String pattern;

    private boolean isSegment;

    private int matchStart;

    private int matchLimit;

    private final RuleBasedTransliterator.Data data;

    public StringMatcher(String theString,
                         boolean isSeg,
                         RuleBasedTransliterator.Data theData) {
        data = theData;
        isSegment = isSeg;
        pattern = theString;
        matchStart = matchLimit = -1;
    }

    public StringMatcher(String theString,
                         int start,
                         int limit,
                         boolean isSeg,
                         RuleBasedTransliterator.Data theData) {
        this(theString.substring(start, limit), isSeg, theData);
    }

    /**
     * Implement UnicodeMatcher
     */
    public int matches(Replaceable text,
                       int[] offset,
                       int limit,
                       boolean incremental) {
        int i;
        int[] cursor = new int[] { offset[0] };
        if (limit < cursor[0]) {
            // Match in the reverse direction
            for (i=pattern.length()-1; i>=0; --i) {
                char keyChar = pattern.charAt(i);
                UnicodeMatcher subm = data.lookup(keyChar);
                if (subm == null) {
                    if (cursor[0] >= limit &&
                        keyChar == text.charAt(cursor[0])) {
                        --cursor[0];
                    } else {
                        return U_MISMATCH;
                    }
                } else {
                    int m =
                        subm.matches(text, cursor, limit, incremental);
                    if (m != U_MATCH) {
                        return m;
                    }
                }
            }
            // Record the match position, but adjust for a normal
            // forward start, limit, and only if a prior match does not
            // exist -- we want the rightmost match.
            if (matchStart < 0) {
                matchStart = cursor[0]+1;
                matchLimit = offset[0]+1;
            }
        } else {
            for (i=0; i<pattern.length(); ++i) {
                if (incremental && cursor[0] == limit) {
                    // We've reached the context limit without a mismatch and
                    // without completing our match.
                    return U_PARTIAL_MATCH;
                }
                char keyChar = pattern.charAt(i);
                UnicodeMatcher subm = data.lookup(keyChar);
                if (subm == null) {
                    // Don't need the cursor < limit check if
                    // incremental is true (because it's done above); do need
                    // it otherwise.
                    if (cursor[0] < limit &&
                        keyChar == text.charAt(cursor[0])) {
                        ++cursor[0];
                    } else {
                        return U_MISMATCH;
                    }
                } else {
                    int m =
                        subm.matches(text, cursor, limit, incremental);
                    if (m != U_MATCH) {
                        return m;
                    }
                }
            }
            // Record the match position
            matchStart = offset[0];
            matchLimit = cursor[0];
        }

        offset[0] = cursor[0];
        return U_MATCH;
    }

    /**
     * Implement UnicodeMatcher
     */
    public String toPattern(boolean escapeUnprintable) {
        StringBuffer result = new StringBuffer();
        StringBuffer quoteBuf = new StringBuffer();
        if (isSegment) {
            result.append('(');
        }
        for (int i=0; i<pattern.length(); ++i) {
            char keyChar = pattern.charAt(i);
            UnicodeMatcher m = data.lookup(keyChar);
            if (m == null) {
                TransliterationRule.appendToRule(result, keyChar, false, escapeUnprintable, quoteBuf);
            } else {
                TransliterationRule.appendToRule(result, m.toPattern(escapeUnprintable),
                                                 true, escapeUnprintable, quoteBuf);
            }
        }
        if (isSegment) {
            result.append(')');
        }
        // Flush quoteBuf out to result
        TransliterationRule.appendToRule(result, -1,
                                         true, escapeUnprintable, quoteBuf);
        return result.toString();
    }

    /**
     * Implement UnicodeMatcher
     */
    public boolean matchesIndexValue(int v) {
        if (pattern.length() == 0) {
            return true;
        }
        int c = UTF16.charAt(pattern, 0);
        UnicodeMatcher m = data.lookup(c);
        return (m == null) ? ((c & 0xFF) == v) : m.matchesIndexValue(v);
    }

    /**
     * Remove any match data.  This must be called before performing a
     * set of matches with this segment.
     */
    public void resetMatch() {
        matchStart = matchLimit = -1;
    }

    /**
     * Return the start offset, in the match text, of the <em>rightmost</em>
     * match.  This method may get moved up into the UnicodeMatcher if
     * it turns out to be useful to generalize this.
     */
    public int getMatchStart() {
        return matchStart;
    }

    /**
     * Return the limit offset, in the match text, of the <em>rightmost</em>
     * match.  This method may get moved up into the UnicodeMatcher if
     * it turns out to be useful to generalize this.
     */
    public int getMatchLimit() {
        return matchLimit;
    }
}

//eof
