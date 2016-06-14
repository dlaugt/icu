/**
*******************************************************************************
* Copyright (C) 1996-2006, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
package com.ibm.icu.dev.test.lang;
import org.junit.Test;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.lang.UCharacterCategory;

/**
* Testing UCharacterCategory
* @author Syn Wee Quek
* @since April 02 2002
*/
public class UCharacterCategoryTest extends TestFmwk
{
    // constructor -----------------------------------------------------------
    
    /**
    * Private constructor to prevent initialisation
    */
    public UCharacterCategoryTest()
    {
    }
    
      // public methods --------------------------------------------------------
      
    /**
    * Gets the name of the argument category
    * @returns category name
    */
    @Test
    public void TestToString()
    {
          String name[] = {"Unassigned",
                           "Letter, Uppercase",
                           "Letter, Lowercase",
                           "Letter, Titlecase",
                           "Letter, Modifier",
                           "Letter, Other",
                           "Mark, Non-Spacing",
                           "Mark, Enclosing",
                           "Mark, Spacing Combining",
                           "Number, Decimal Digit",
                           "Number, Letter",
                           "Number, Other",
                           "Separator, Space",
                           "Separator, Line",
                           "Separator, Paragraph",
                           "Other, Control",
                           "Other, Format",
                           "Other, Private Use",
                           "Other, Surrogate",
                           "Punctuation, Dash",
                           "Punctuation, Open",
                           "Punctuation, Close",
                           "Punctuation, Connector",
                           "Punctuation, Other",
                           "Symbol, Math",
                           "Symbol, Currency",
                           "Symbol, Modifier",
                           "Symbol, Other", 
                           "Punctuation, Initial quote",
                           "Punctuation, Final quote"};
        for (int i = UCharacterCategory.UNASSIGNED; 
                 i < UCharacterCategory.CHAR_CATEGORY_COUNT; i ++) {
             if (!UCharacterCategory.toString(i).equals(name[i])) {
                 errln("Error toString for category " + i + " expected " +
                       name[i]);
             }
        }
    }
}
