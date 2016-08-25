// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
 ******************************************************************************
 * Copyright (C) 2007-2010, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */

package com.ibm.icu.impl.duration;

import java.util.Collection;

import com.ibm.icu.impl.duration.impl.PeriodFormatterDataService;
import com.ibm.icu.impl.duration.impl.ResourceBasedPeriodFormatterDataService;

/**
 * An implementation of PeriodFormatterService that constructs a
 * BasicPeriodFormatterFactory.
 */
public class BasicPeriodFormatterService implements PeriodFormatterService {
    private static BasicPeriodFormatterService instance;
    private PeriodFormatterDataService ds;

    /**
     * Return the default service instance. This uses the default data service.
     * 
     * @return an BasicPeriodFormatterService
     */
    public static BasicPeriodFormatterService getInstance() {
        if (instance == null) {
            PeriodFormatterDataService ds = ResourceBasedPeriodFormatterDataService
                    .getInstance();
            instance = new BasicPeriodFormatterService(ds);
        }
        return instance;
    }

    /**
     * Construct a BasicPeriodFormatterService using the given
     * PeriodFormatterDataService.
     * 
     * @param ds the data service to use
     */
    public BasicPeriodFormatterService(PeriodFormatterDataService ds) {
        this.ds = ds;
    }

    public DurationFormatterFactory newDurationFormatterFactory() {
        return new BasicDurationFormatterFactory(this);
    }

    public PeriodFormatterFactory newPeriodFormatterFactory() {
        return new BasicPeriodFormatterFactory(ds);
    }

    public PeriodBuilderFactory newPeriodBuilderFactory() {
        return new BasicPeriodBuilderFactory(ds);
    }

    public Collection<String> getAvailableLocaleNames() {
        return ds.getAvailableLocales();
    }
}
