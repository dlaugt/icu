/*
 ******************************************************************************
 * Copyright (C) 1996-2008, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */

/*
 * @author Shaopeng Jia
 */

package com.ibm.icu.impl;

import com.ibm.icu.impl.PropsVectors.CompactHandler;

public class PVecToTrieCompactHandler implements CompactHandler {
    public IntTrieBuilder builder;
    public int capacity;
    public int initialValue;
    public boolean latinLinear;

    public void setRowIndexForErrorValue() {
    }

    public void setRowIndexForInitialValue(int rowIndex) {
        initialValue = rowIndex;
    }

    public void setRowIndexForRange(int start, int end, int rowIndex) {
        builder.setRange(start, end + 1, rowIndex, true);
    }

    public void startRealValues(int rowIndex) {
        if (rowIndex > 0xffff) {
            // too many rows for a 16-bit trie
            throw new IndexOutOfBoundsException();
        } else {
            builder = new IntTrieBuilder(null, capacity, initialValue,
                    initialValue, latinLinear);
        } 
    }

}
