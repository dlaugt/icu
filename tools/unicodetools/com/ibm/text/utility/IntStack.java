/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/unicodetools/com/ibm/text/utility/IntStack.java,v $
* $Date: 2001/08/31 00:19:16 $
* $Revision: 1.2 $
*
*******************************************************************************
*/

package com.ibm.text.utility;

// =============================================================
// Simple stack mechanism, with push, pop and access
// =============================================================

public final class IntStack {
    private int[] values;
    private int top = 0;

    public IntStack(int initialSize) {
        values = new int[initialSize];
    }

    public void push(int value) {
        if (top >= values.length) { // must grow?
            int[] temp = new int[values.length*2];
            System.arraycopy(values,0,temp,0,values.length);
            values = temp;
        }
        values[top++] = value;
    }

    public int pop() {
        if (top > 0) return values[--top];
        throw new IllegalArgumentException("Stack underflow");
    }

    public int get(int index) {
        if (0 <= index && index < top) return values[index];
        throw new IllegalArgumentException("Stack index out of bounds");
    }

    public int getTop() {
        return top;
    }

    public boolean isEmpty() {
        return top == 0;
    }
}