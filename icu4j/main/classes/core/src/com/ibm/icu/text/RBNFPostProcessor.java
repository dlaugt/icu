﻿/*
 *******************************************************************************
 * Copyright (C) 2004-2011, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */

package com.ibm.icu.text;

/**
 * Post processor for RBNF output.
 */
interface RBNFPostProcessor {
    /**
     * Initialization routine for this instance, called once
     * immediately after first construction and never again.
     * @param formatter the formatter that will be using this post-processor
     * @param rules the special rules for this post-procesor
     */
    void init(RuleBasedNumberFormat formatter, String rules);

    /**
     * Work routine.  Post process the output, which was generated by the
     * ruleset with the given name.
     * @param output the output of the main RBNF processing
     * @param ruleSet the rule set originally invoked to generate the output
     */
    void process(StringBuffer output, NFRuleSet ruleSet);
}
