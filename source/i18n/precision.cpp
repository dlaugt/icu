/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: precisison.cpp
 */

#include "unicode/utypes.h"

#include "precision.h"
#include "digitlst.h"

U_NAMESPACE_BEGIN

FixedPrecision::FixedPrecision() {
    fMin.setIntDigitCount(1);
    fMin.setFracDigitCount(0);
}

DigitList &
FixedPrecision::round(
        DigitList &value, int32_t exponent, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return value;
    }
    if (!fRoundingIncrement.isZero()) {
        if (exponent == 0) {
            value.quantize(fRoundingIncrement, status);
        } else {
            DigitList adjustedIncrement(fRoundingIncrement);
            adjustedIncrement.shiftDecimalRight(exponent);
            value.quantize(adjustedIncrement, status);
        }
        if (U_FAILURE(status)) {
            return value;
        }
    }
    int32_t leastSig = fMax.getLeastSignificantInclusive();
    if (leastSig == INT32_MIN) {
        value.round(fSignificant.getMax());
    } else {
        value.roundAtExponent(
                exponent + leastSig,
                fSignificant.getMax());
    }
    return value;
}

DigitInterval &
FixedPrecision::getInterval(
        const DigitList &value, DigitInterval &interval) const {
    value.getSmallestInterval(interval, fSignificant.getMin(), &fMin);
    interval.expandToContain(fMin);
    interval.shrinkToFitWithin(fMax);
    return interval;
}

UBool
FixedPrecision::isFastFormattable() const {
    return (fMin.getFracDigitCount() == 0 && fSignificant.isNoConstraints() && fRoundingIncrement.isZero());
}

DigitList &
ScientificPrecision::round(DigitList &value, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return value;
    }
    int32_t exponent = value.getScientificExponent(
            fMantissa.fMin.getIntDigitCount(), getMultiplier());
    return fMantissa.round(value, exponent, status);
}

int32_t
ScientificPrecision::toScientific(DigitList &value) const {
    return value.toScientific(
            fMantissa.fMin.getIntDigitCount(), getMultiplier());
}

int32_t
ScientificPrecision::getMultiplier() const {
    int32_t maxIntDigitCount = fMantissa.fMax.getIntDigitCount();
    if (maxIntDigitCount == INT32_MAX) {
        return 1;
    }
    int32_t multiplier =
        maxIntDigitCount - fMantissa.fMin.getIntDigitCount() + 1;
    return (multiplier < 1 ? 1 : multiplier);
}


U_NAMESPACE_END

