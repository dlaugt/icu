// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <cstdlib>
#include <cmath>
#include <limits>
#include <stdlib.h>

#include "unicode/plurrule.h"
#include "cmemory.h"
#include "number_decnum.h"
#include "putilimp.h"
#include "number_decimalquantity.h"
#include "number_roundingutils.h"
#include "double-conversion.h"
#include "charstr.h"
#include "number_utils.h"
#include "uassert.h"

using namespace icu;
using namespace icu::number;
using namespace icu::number::impl;

using icu::double_conversion::DoubleToStringConverter;
using icu::double_conversion::StringToDoubleConverter;

namespace {

int8_t NEGATIVE_FLAG = 1;
int8_t INFINITY_FLAG = 2;
int8_t NAN_FLAG = 4;

/** Helper function for safe subtraction (no overflow). */
inline int32_t safeSubtract(int32_t a, int32_t b) {
    // Note: In C++, signed integer subtraction is undefined behavior.
    int32_t diff = static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b));
    if (b < 0 && diff < a) { return INT32_MAX; }
    if (b > 0 && diff > a) { return INT32_MIN; }
    return diff;
}

static double DOUBLE_MULTIPLIERS[] = {
        1e0,
        1e1,
        1e2,
        1e3,
        1e4,
        1e5,
        1e6,
        1e7,
        1e8,
        1e9,
        1e10,
        1e11,
        1e12,
        1e13,
        1e14,
        1e15,
        1e16,
        1e17,
        1e18,
        1e19,
        1e20,
        1e21};

}  // namespace

icu::IFixedDecimal::~IFixedDecimal() = default;

DecimalQuantity::DecimalQuantity() {
    setBcdToZero();
    flags = 0;
}

DecimalQuantity::~DecimalQuantity() {
    if (usingBytes) {
        uprv_free(fBCD.bcdBytes.ptr);
        fBCD.bcdBytes.ptr = nullptr;
        usingBytes = false;
    }
}

DecimalQuantity::DecimalQuantity(const DecimalQuantity &other) {
    *this = other;
}

DecimalQuantity::DecimalQuantity(DecimalQuantity&& src) U_NOEXCEPT {
    *this = std::move(src);
}

DecimalQuantity &DecimalQuantity::operator=(const DecimalQuantity &other) {
    if (this == &other) {
        return *this;
    }
    copyBcdFrom(other);
    copyFieldsFrom(other);
    return *this;
}

DecimalQuantity& DecimalQuantity::operator=(DecimalQuantity&& src) U_NOEXCEPT {
    if (this == &src) {
        return *this;
    }
    moveBcdFrom(src);
    copyFieldsFrom(src);
    return *this;
}

void DecimalQuantity::copyFieldsFrom(const DecimalQuantity& other) {
    bogus = other.bogus;
    lOptPos = other.lOptPos;
    lReqPos = other.lReqPos;
    rReqPos = other.rReqPos;
    rOptPos = other.rOptPos;
    scale = other.scale;
    precision = other.precision;
    flags = other.flags;
    origDouble = other.origDouble;
    origDelta = other.origDelta;
    isApproximate = other.isApproximate;
}

void DecimalQuantity::clear() {
    lOptPos = INT32_MAX;
    lReqPos = 0;
    rReqPos = 0;
    rOptPos = INT32_MIN;
    flags = 0;
    setBcdToZero(); // sets scale, precision, hasDouble, origDouble, origDelta, and BCD data
}

void DecimalQuantity::setIntegerLength(int32_t minInt, int32_t maxInt) {
    // Validation should happen outside of DecimalQuantity, e.g., in the Precision class.
    U_ASSERT(minInt >= 0);
    U_ASSERT(maxInt >= minInt);

    // Special behavior: do not set minInt to be less than what is already set.
    // This is so significant digits rounding can set the integer length.
    if (minInt < lReqPos) {
        minInt = lReqPos;
    }

    // Save values into internal state
    // Negation is safe for minFrac/maxFrac because -Integer.MAX_VALUE > Integer.MIN_VALUE
    lOptPos = maxInt;
    lReqPos = minInt;
}

void DecimalQuantity::setFractionLength(int32_t minFrac, int32_t maxFrac) {
    // Validation should happen outside of DecimalQuantity, e.g., in the Precision class.
    U_ASSERT(minFrac >= 0);
    U_ASSERT(maxFrac >= minFrac);

    // Save values into internal state
    // Negation is safe for minFrac/maxFrac because -Integer.MAX_VALUE > Integer.MIN_VALUE
    rReqPos = -minFrac;
    rOptPos = -maxFrac;
}

uint64_t DecimalQuantity::getPositionFingerprint() const {
    uint64_t fingerprint = 0;
    fingerprint ^= lOptPos;
    fingerprint ^= (lReqPos << 16);
    fingerprint ^= (static_cast<uint64_t>(rReqPos) << 32);
    fingerprint ^= (static_cast<uint64_t>(rOptPos) << 48);
    return fingerprint;
}

void DecimalQuantity::roundToIncrement(double roundingIncrement, RoundingMode roundingMode,
                                       int32_t maxFrac, UErrorCode& status) {
    // TODO(13701): This is innefficient.  Improve?
    // TODO(13701): Should we convert to decNumber instead?
    roundToInfinity();
    double temp = toDouble();
    temp /= roundingIncrement;
    // Use another DecimalQuantity to perform the actual rounding...
    DecimalQuantity dq;
    dq.setToDouble(temp);
    dq.roundToMagnitude(0, roundingMode, status);
    temp = dq.toDouble();
    temp *= roundingIncrement;
    setToDouble(temp);
    // Since we reset the value to a double, we need to specify the rounding boundary
    // in order to get the DecimalQuantity out of approximation mode.
    // NOTE: In Java, we have minMaxFrac, but in C++, the two are differentiated.
    roundToMagnitude(-maxFrac, roundingMode, status);
}

void DecimalQuantity::multiplyBy(const DecNum& multiplicand, UErrorCode& status) {
    if (isInfinite() || isZero() || isNaN()) {
        return;
    }
    // Convert to DecNum, multiply, and convert back.
    DecNum decnum;
    toDecNum(decnum, status);
    if (U_FAILURE(status)) { return; }
    decnum.multiplyBy(multiplicand, status);
    if (U_FAILURE(status)) { return; }
    setToDecNum(decnum, status);
}

void DecimalQuantity::divideBy(const DecNum& divisor, UErrorCode& status) {
    if (isInfinite() || isZero() || isNaN()) {
        return;
    }
    // Convert to DecNum, multiply, and convert back.
    DecNum decnum;
    toDecNum(decnum, status);
    if (U_FAILURE(status)) { return; }
    decnum.divideBy(divisor, status);
    if (U_FAILURE(status)) { return; }
    setToDecNum(decnum, status);
}

void DecimalQuantity::negate() {
    flags ^= NEGATIVE_FLAG;
}

int32_t DecimalQuantity::getMagnitude() const {
    U_ASSERT(precision != 0);
    return scale + precision - 1;
}

bool DecimalQuantity::adjustMagnitude(int32_t delta) {
    if (precision != 0) {
        // i.e., scale += delta; origDelta += delta
        bool overflow = uprv_add32_overflow(scale, delta, &scale);
        overflow = uprv_add32_overflow(origDelta, delta, &origDelta) || overflow;
        // Make sure that precision + scale won't overflow, either
        int32_t dummy;
        overflow = overflow || uprv_add32_overflow(scale, precision, &dummy);
        return overflow;
    }
    return false;
}

double DecimalQuantity::getPluralOperand(PluralOperand operand) const {
    // If this assertion fails, you need to call roundToInfinity() or some other rounding method.
    // See the comment at the top of this file explaining the "isApproximate" field.
    U_ASSERT(!isApproximate);

    switch (operand) {
        case PLURAL_OPERAND_I:
            // Invert the negative sign if necessary
            return static_cast<double>(isNegative() ? -toLong(true) : toLong(true));
        case PLURAL_OPERAND_F:
            return static_cast<double>(toFractionLong(true));
        case PLURAL_OPERAND_T:
            return static_cast<double>(toFractionLong(false));
        case PLURAL_OPERAND_V:
            return fractionCount();
        case PLURAL_OPERAND_W:
            return fractionCountWithoutTrailingZeros();
        default:
            return std::abs(toDouble());
    }
}

bool DecimalQuantity::hasIntegerValue() const {
    return scale >= 0;
}

int32_t DecimalQuantity::getUpperDisplayMagnitude() const {
    // If this assertion fails, you need to call roundToInfinity() or some other rounding method.
    // See the comment in the header file explaining the "isApproximate" field.
    U_ASSERT(!isApproximate);

    int32_t magnitude = scale + precision;
    int32_t result = (lReqPos > magnitude) ? lReqPos : (lOptPos < magnitude) ? lOptPos : magnitude;
    return result - 1;
}

int32_t DecimalQuantity::getLowerDisplayMagnitude() const {
    // If this assertion fails, you need to call roundToInfinity() or some other rounding method.
    // See the comment in the header file explaining the "isApproximate" field.
    U_ASSERT(!isApproximate);

    int32_t magnitude = scale;
    int32_t result = (rReqPos < magnitude) ? rReqPos : (rOptPos > magnitude) ? rOptPos : magnitude;
    return result;
}

int8_t DecimalQuantity::getDigit(int32_t magnitude) const {
    // If this assertion fails, you need to call roundToInfinity() or some other rounding method.
    // See the comment at the top of this file explaining the "isApproximate" field.
    U_ASSERT(!isApproximate);

    return getDigitPos(magnitude - scale);
}

int32_t DecimalQuantity::fractionCount() const {
    return -getLowerDisplayMagnitude();
}

int32_t DecimalQuantity::fractionCountWithoutTrailingZeros() const {
    return -scale > 0 ? -scale : 0;  // max(-scale, 0)
}

bool DecimalQuantity::isNegative() const {
    return (flags & NEGATIVE_FLAG) != 0;
}

int8_t DecimalQuantity::signum() const {
    return isNegative() ? -1 : isZero() ? 0 : 1;
}

bool DecimalQuantity::isInfinite() const {
    return (flags & INFINITY_FLAG) != 0;
}

bool DecimalQuantity::isNaN() const {
    return (flags & NAN_FLAG) != 0;
}

bool DecimalQuantity::isZero() const {
    return precision == 0;
}

DecimalQuantity &DecimalQuantity::setToInt(int32_t n) {
    setBcdToZero();
    flags = 0;
    if (n == INT32_MIN) {
        flags |= NEGATIVE_FLAG;
        // leave as INT32_MIN; handled below in _setToInt()
    } else if (n < 0) {
        flags |= NEGATIVE_FLAG;
        n = -n;
    }
    if (n != 0) {
        _setToInt(n);
        compact();
    }
    return *this;
}

void DecimalQuantity::_setToInt(int32_t n) {
    if (n == INT32_MIN) {
        readLongToBcd(-static_cast<int64_t>(n));
    } else {
        readIntToBcd(n);
    }
}

DecimalQuantity &DecimalQuantity::setToLong(int64_t n) {
    setBcdToZero();
    flags = 0;
    if (n < 0 && n > INT64_MIN) {
        flags |= NEGATIVE_FLAG;
        n = -n;
    }
    if (n != 0) {
        _setToLong(n);
        compact();
    }
    return *this;
}

void DecimalQuantity::_setToLong(int64_t n) {
    if (n == INT64_MIN) {
        DecNum decnum;
        UErrorCode localStatus = U_ZERO_ERROR;
        decnum.setTo("9.223372036854775808E+18", localStatus);
        if (U_FAILURE(localStatus)) { return; } // unexpected
        flags |= NEGATIVE_FLAG;
        readDecNumberToBcd(decnum);
    } else if (n <= INT32_MAX) {
        readIntToBcd(static_cast<int32_t>(n));
    } else {
        readLongToBcd(n);
    }
}

DecimalQuantity &DecimalQuantity::setToDouble(double n) {
    setBcdToZero();
    flags = 0;
    // signbit() from <math.h> handles +0.0 vs -0.0
    if (std::signbit(n)) {
        flags |= NEGATIVE_FLAG;
        n = -n;
    }
    if (std::isnan(n) != 0) {
        flags |= NAN_FLAG;
    } else if (std::isfinite(n) == 0) {
        flags |= INFINITY_FLAG;
    } else if (n != 0) {
        _setToDoubleFast(n);
        compact();
    }
    return *this;
}

void DecimalQuantity::_setToDoubleFast(double n) {
    isApproximate = true;
    origDouble = n;
    origDelta = 0;

    // Make sure the double is an IEEE 754 double.  If not, fall back to the slow path right now.
    // TODO: Make a fast path for other types of doubles.
    if (!std::numeric_limits<double>::is_iec559) {
        convertToAccurateDouble();
        // Turn off the approximate double flag, since the value is now exact.
        isApproximate = false;
        origDouble = 0.0;
        return;
    }

    // To get the bits from the double, use memcpy, which takes care of endianness.
    uint64_t ieeeBits;
    uprv_memcpy(&ieeeBits, &n, sizeof(n));
    int32_t exponent = static_cast<int32_t>((ieeeBits & 0x7ff0000000000000L) >> 52) - 0x3ff;

    // Not all integers can be represented exactly for exponent > 52
    if (exponent <= 52 && static_cast<int64_t>(n) == n) {
        _setToLong(static_cast<int64_t>(n));
        return;
    }

    // 3.3219... is log2(10)
    auto fracLength = static_cast<int32_t> ((52 - exponent) / 3.32192809489);
    if (fracLength >= 0) {
        int32_t i = fracLength;
        // 1e22 is the largest exact double.
        for (; i >= 22; i -= 22) n *= 1e22;
        n *= DOUBLE_MULTIPLIERS[i];
    } else {
        int32_t i = fracLength;
        // 1e22 is the largest exact double.
        for (; i <= -22; i += 22) n /= 1e22;
        n /= DOUBLE_MULTIPLIERS[-i];
    }
    auto result = static_cast<int64_t>(std::round(n));
    if (result != 0) {
        _setToLong(result);
        scale -= fracLength;
    }
}

void DecimalQuantity::convertToAccurateDouble() {
    U_ASSERT(origDouble != 0);
    int32_t delta = origDelta;

    // Call the slow oracle function (Double.toString in Java, DoubleToAscii in C++).
    char buffer[DoubleToStringConverter::kBase10MaximalLength + 1];
    bool sign; // unused; always positive
    int32_t length;
    int32_t point;
    DoubleToStringConverter::DoubleToAscii(
        origDouble,
        DoubleToStringConverter::DtoaMode::SHORTEST,
        0,
        buffer,
        sizeof(buffer),
        &sign,
        &length,
        &point
    );

    setBcdToZero();
    readDoubleConversionToBcd(buffer, length, point);
    scale += delta;
    explicitExactDouble = true;
}

DecimalQuantity &DecimalQuantity::setToDecNumber(StringPiece n, UErrorCode& status) {
    setBcdToZero();
    flags = 0;

    // Compute the decNumber representation
    DecNum decnum;
    decnum.setTo(n, status);

    _setToDecNum(decnum, status);
    return *this;
}

DecimalQuantity& DecimalQuantity::setToDecNum(const DecNum& decnum, UErrorCode& status) {
    setBcdToZero();
    flags = 0;

    _setToDecNum(decnum, status);
    return *this;
}

void DecimalQuantity::_setToDecNum(const DecNum& decnum, UErrorCode& status) {
    if (U_FAILURE(status)) { return; }
    if (decnum.isNegative()) {
        flags |= NEGATIVE_FLAG;
    }
    if (!decnum.isZero()) {
        readDecNumberToBcd(decnum);
        compact();
    }
}

int64_t DecimalQuantity::toLong(bool truncateIfOverflow) const {
    // NOTE: Call sites should be guarded by fitsInLong(), like this:
    // if (dq.fitsInLong()) { /* use dq.toLong() */ } else { /* use some fallback */ }
    // Fallback behavior upon truncateIfOverflow is to truncate at 17 digits.
    uint64_t result = 0L;
    int32_t upperMagnitude = std::min(scale + precision, lOptPos) - 1;
    if (truncateIfOverflow) {
        upperMagnitude = std::min(upperMagnitude, 17);
    }
    for (int32_t magnitude = upperMagnitude; magnitude >= 0; magnitude--) {
        result = result * 10 + getDigitPos(magnitude - scale);
    }
    if (isNegative()) {
        return static_cast<int64_t>(0LL - result); // i.e., -result
    }
    return static_cast<int64_t>(result);
}

uint64_t DecimalQuantity::toFractionLong(bool includeTrailingZeros) const {
    uint64_t result = 0L;
    int32_t magnitude = -1;
    int32_t lowerMagnitude = std::max(scale, rOptPos);
    if (includeTrailingZeros) {
        lowerMagnitude = std::min(lowerMagnitude, rReqPos);
    }
    for (; magnitude >= lowerMagnitude && result <= 1e18L; magnitude--) {
        result = result * 10 + getDigitPos(magnitude - scale);
    }
    // Remove trailing zeros; this can happen during integer overflow cases.
    if (!includeTrailingZeros) {
        while (result > 0 && (result % 10) == 0) {
            result /= 10;
        }
    }
    return result;
}

bool DecimalQuantity::fitsInLong(bool ignoreFraction) const {
    if (isZero()) {
        return true;
    }
    if (scale < 0 && !ignoreFraction) {
        return false;
    }
    int magnitude = getMagnitude();
    if (magnitude < 18) {
        return true;
    }
    if (magnitude > 18) {
        return false;
    }
    // Hard case: the magnitude is 10^18.
    // The largest int64 is: 9,223,372,036,854,775,807
    for (int p = 0; p < precision; p++) {
        int8_t digit = getDigit(18 - p);
        static int8_t INT64_BCD[] = { 9, 2, 2, 3, 3, 7, 2, 0, 3, 6, 8, 5, 4, 7, 7, 5, 8, 0, 8 };
        if (digit < INT64_BCD[p]) {
            return true;
        } else if (digit > INT64_BCD[p]) {
            return false;
        }
    }
    // Exactly equal to max long plus one.
    return isNegative();
}

double DecimalQuantity::toDouble() const {
    // If this assertion fails, you need to call roundToInfinity() or some other rounding method.
    // See the comment in the header file explaining the "isApproximate" field.
    U_ASSERT(!isApproximate);

    if (isNaN()) {
        return NAN;
    } else if (isInfinite()) {
        return isNegative() ? -INFINITY : INFINITY;
    }

    // We are processing well-formed input, so we don't need any special options to StringToDoubleConverter.
    StringToDoubleConverter converter(0, 0, 0, "", "");
    UnicodeString numberString = this->toScientificString();
    int32_t count;
    return converter.StringToDouble(
            reinterpret_cast<const uint16_t*>(numberString.getBuffer()),
            numberString.length(),
            &count);
}

void DecimalQuantity::toDecNum(DecNum& output, UErrorCode& status) const {
    // Special handling for zero
    if (precision == 0) {
        output.setTo("0", status);
    }

    // Use the BCD constructor. We need to do a little bit of work to convert, though.
    // The decNumber constructor expects most-significant first, but we store least-significant first.
    MaybeStackArray<uint8_t, 20> ubcd(precision);
    for (int32_t m = 0; m < precision; m++) {
        ubcd[precision - m - 1] = static_cast<uint8_t>(getDigitPos(m));
    }
    output.setTo(ubcd.getAlias(), precision, scale, isNegative(), status);
}

void DecimalQuantity::truncate() {
    if (scale < 0) {
        shiftRight(-scale);
        scale = 0;
        compact();
    }
}

void DecimalQuantity::roundToMagnitude(int32_t magnitude, RoundingMode roundingMode, UErrorCode& status) {
    // The position in the BCD at which rounding will be performed; digits to the right of position
    // will be rounded away.
    // TODO: Andy: There was a test failure because of integer overflow here. Should I do
    // "safe subtraction" everywhere in the code?  What's the nicest way to do it?
    int position = safeSubtract(magnitude, scale);

    if (position <= 0 && !isApproximate) {
        // All digits are to the left of the rounding magnitude.
    } else if (precision == 0) {
        // No rounding for zero.
    } else {
        // Perform rounding logic.
        // "leading" = most significant digit to the right of rounding
        // "trailing" = least significant digit to the left of rounding
        int8_t leadingDigit = getDigitPos(safeSubtract(position, 1));
        int8_t trailingDigit = getDigitPos(position);

        // Compute which section of the number we are in.
        // EDGE means we are at the bottom or top edge, like 1.000 or 1.999 (used by doubles)
        // LOWER means we are between the bottom edge and the midpoint, like 1.391
        // MIDPOINT means we are exactly in the middle, like 1.500
        // UPPER means we are between the midpoint and the top edge, like 1.916
        roundingutils::Section section = roundingutils::SECTION_MIDPOINT;
        if (!isApproximate) {
            if (leadingDigit < 5) {
                section = roundingutils::SECTION_LOWER;
            } else if (leadingDigit > 5) {
                section = roundingutils::SECTION_UPPER;
            } else {
                for (int p = safeSubtract(position, 2); p >= 0; p--) {
                    if (getDigitPos(p) != 0) {
                        section = roundingutils::SECTION_UPPER;
                        break;
                    }
                }
            }
        } else {
            int32_t p = safeSubtract(position, 2);
            int32_t minP = uprv_max(0, precision - 14);
            if (leadingDigit == 0) {
                section = roundingutils::SECTION_LOWER_EDGE;
                for (; p >= minP; p--) {
                    if (getDigitPos(p) != 0) {
                        section = roundingutils::SECTION_LOWER;
                        break;
                    }
                }
            } else if (leadingDigit == 4) {
                for (; p >= minP; p--) {
                    if (getDigitPos(p) != 9) {
                        section = roundingutils::SECTION_LOWER;
                        break;
                    }
                }
            } else if (leadingDigit == 5) {
                for (; p >= minP; p--) {
                    if (getDigitPos(p) != 0) {
                        section = roundingutils::SECTION_UPPER;
                        break;
                    }
                }
            } else if (leadingDigit == 9) {
                section = roundingutils::SECTION_UPPER_EDGE;
                for (; p >= minP; p--) {
                    if (getDigitPos(p) != 9) {
                        section = roundingutils::SECTION_UPPER;
                        break;
                    }
                }
            } else if (leadingDigit < 5) {
                section = roundingutils::SECTION_LOWER;
            } else {
                section = roundingutils::SECTION_UPPER;
            }

            bool roundsAtMidpoint = roundingutils::roundsAtMidpoint(roundingMode);
            if (safeSubtract(position, 1) < precision - 14 ||
                (roundsAtMidpoint && section == roundingutils::SECTION_MIDPOINT) ||
                (!roundsAtMidpoint && section < 0 /* i.e. at upper or lower edge */)) {
                // Oops! This means that we have to get the exact representation of the double, because
                // the zone of uncertainty is along the rounding boundary.
                convertToAccurateDouble();
                roundToMagnitude(magnitude, roundingMode, status); // start over
                return;
            }

            // Turn off the approximate double flag, since the value is now confirmed to be exact.
            isApproximate = false;
            origDouble = 0.0;
            origDelta = 0;

            if (position <= 0) {
                // All digits are to the left of the rounding magnitude.
                return;
            }

            // Good to continue rounding.
            if (section == -1) { section = roundingutils::SECTION_LOWER; }
            if (section == -2) { section = roundingutils::SECTION_UPPER; }
        }

        bool roundDown = roundingutils::getRoundingDirection((trailingDigit % 2) == 0,
                isNegative(),
                section,
                roundingMode,
                status);
        if (U_FAILURE(status)) {
            return;
        }

        // Perform truncation
        if (position >= precision) {
            setBcdToZero();
            scale = magnitude;
        } else {
            shiftRight(position);
        }

        // Bubble the result to the higher digits
        if (!roundDown) {
            if (trailingDigit == 9) {
                int bubblePos = 0;
                // Note: in the long implementation, the most digits BCD can have at this point is 15,
                // so bubblePos <= 15 and getDigitPos(bubblePos) is safe.
                for (; getDigitPos(bubblePos) == 9; bubblePos++) {}
                shiftRight(bubblePos); // shift off the trailing 9s
            }
            int8_t digit0 = getDigitPos(0);
            U_ASSERT(digit0 != 9);
            setDigitPos(0, static_cast<int8_t>(digit0 + 1));
            precision += 1; // in case an extra digit got added
        }

        compact();
    }
}

void DecimalQuantity::roundToInfinity() {
    if (isApproximate) {
        convertToAccurateDouble();
    }
}

void DecimalQuantity::appendDigit(int8_t value, int32_t leadingZeros, bool appendAsInteger) {
    U_ASSERT(leadingZeros >= 0);

    // Zero requires special handling to maintain the invariant that the least-significant digit
    // in the BCD is nonzero.
    if (value == 0) {
        if (appendAsInteger && precision != 0) {
            scale += leadingZeros + 1;
        }
        return;
    }

    // Deal with trailing zeros
    if (scale > 0) {
        leadingZeros += scale;
        if (appendAsInteger) {
            scale = 0;
        }
    }

    // Append digit
    shiftLeft(leadingZeros + 1);
    setDigitPos(0, value);

    // Fix scale if in integer mode
    if (appendAsInteger) {
        scale += leadingZeros + 1;
    }
}

UnicodeString DecimalQuantity::toPlainString() const {
    U_ASSERT(!isApproximate);
    UnicodeString sb;
    if (isNegative()) {
        sb.append(u'-');
    }
    if (precision == 0 || getMagnitude() < 0) {
        sb.append(u'0');
    }
    for (int m = getUpperDisplayMagnitude(); m >= getLowerDisplayMagnitude(); m--) {
        if (m == -1) { sb.append(u'.'); }
        sb.append(getDigit(m) + u'0');
    }
    return sb;
}

UnicodeString DecimalQuantity::toScientificString() const {
    U_ASSERT(!isApproximate);
    UnicodeString result;
    if (isNegative()) {
        result.append(u'-');
    }
    if (precision == 0) {
        result.append(u"0E+0", -1);
        return result;
    }
    // NOTE: It is not safe to add to lOptPos (aka maxInt) or subtract from
    // rOptPos (aka -maxFrac) due to overflow.
    int32_t upperPos = std::min(precision + scale, lOptPos) - scale - 1;
    int32_t lowerPos = std::max(scale, rOptPos) - scale;
    int32_t p = upperPos;
    result.append(u'0' + getDigitPos(p));
    if ((--p) >= lowerPos) {
        result.append(u'.');
        for (; p >= lowerPos; p--) {
            result.append(u'0' + getDigitPos(p));
        }
    }
    result.append(u'E');
    int32_t _scale = upperPos + scale;
    if (_scale < 0) {
        _scale *= -1;
        result.append(u'-');
    } else {
        result.append(u'+');
    }
    if (_scale == 0) {
        result.append(u'0');
    }
    int32_t insertIndex = result.length();
    while (_scale > 0) {
        std::div_t res = std::div(_scale, 10);
        result.insert(insertIndex, u'0' + res.rem);
        _scale = res.quot;
    }
    return result;
}

////////////////////////////////////////////////////
/// End of DecimalQuantity_AbstractBCD.java      ///
/// Start of DecimalQuantity_DualStorageBCD.java ///
////////////////////////////////////////////////////

int8_t DecimalQuantity::getDigitPos(int32_t position) const {
    if (usingBytes) {
        if (position < 0 || position >= precision) { return 0; }
        return fBCD.bcdBytes.ptr[position];
    } else {
        if (position < 0 || position >= 16) { return 0; }
        return (int8_t) ((fBCD.bcdLong >> (position * 4)) & 0xf);
    }
}

void DecimalQuantity::setDigitPos(int32_t position, int8_t value) {
    U_ASSERT(position >= 0);
    if (usingBytes) {
        ensureCapacity(position + 1);
        fBCD.bcdBytes.ptr[position] = value;
    } else if (position >= 16) {
        switchStorage();
        ensureCapacity(position + 1);
        fBCD.bcdBytes.ptr[position] = value;
    } else {
        int shift = position * 4;
        fBCD.bcdLong = (fBCD.bcdLong & ~(0xfL << shift)) | ((long) value << shift);
    }
}

void DecimalQuantity::shiftLeft(int32_t numDigits) {
    if (!usingBytes && precision + numDigits > 16) {
        switchStorage();
    }
    if (usingBytes) {
        ensureCapacity(precision + numDigits);
        int i = precision + numDigits - 1;
        for (; i >= numDigits; i--) {
            fBCD.bcdBytes.ptr[i] = fBCD.bcdBytes.ptr[i - numDigits];
        }
        for (; i >= 0; i--) {
            fBCD.bcdBytes.ptr[i] = 0;
        }
    } else {
        fBCD.bcdLong <<= (numDigits * 4);
    }
    scale -= numDigits;
    precision += numDigits;
}

void DecimalQuantity::shiftRight(int32_t numDigits) {
    if (usingBytes) {
        int i = 0;
        for (; i < precision - numDigits; i++) {
            fBCD.bcdBytes.ptr[i] = fBCD.bcdBytes.ptr[i + numDigits];
        }
        for (; i < precision; i++) {
            fBCD.bcdBytes.ptr[i] = 0;
        }
    } else {
        fBCD.bcdLong >>= (numDigits * 4);
    }
    scale += numDigits;
    precision -= numDigits;
}

void DecimalQuantity::setBcdToZero() {
    if (usingBytes) {
        uprv_free(fBCD.bcdBytes.ptr);
        fBCD.bcdBytes.ptr = nullptr;
        usingBytes = false;
    }
    fBCD.bcdLong = 0L;
    scale = 0;
    precision = 0;
    isApproximate = false;
    origDouble = 0;
    origDelta = 0;
}

void DecimalQuantity::readIntToBcd(int32_t n) {
    U_ASSERT(n != 0);
    // ints always fit inside the long implementation.
    uint64_t result = 0L;
    int i = 16;
    for (; n != 0; n /= 10, i--) {
        result = (result >> 4) + ((static_cast<uint64_t>(n) % 10) << 60);
    }
    U_ASSERT(!usingBytes);
    fBCD.bcdLong = result >> (i * 4);
    scale = 0;
    precision = 16 - i;
}

void DecimalQuantity::readLongToBcd(int64_t n) {
    U_ASSERT(n != 0);
    if (n >= 10000000000000000L) {
        ensureCapacity();
        int i = 0;
        for (; n != 0L; n /= 10L, i++) {
            fBCD.bcdBytes.ptr[i] = static_cast<int8_t>(n % 10);
        }
        U_ASSERT(usingBytes);
        scale = 0;
        precision = i;
    } else {
        uint64_t result = 0L;
        int i = 16;
        for (; n != 0L; n /= 10L, i--) {
            result = (result >> 4) + ((n % 10) << 60);
        }
        U_ASSERT(i >= 0);
        U_ASSERT(!usingBytes);
        fBCD.bcdLong = result >> (i * 4);
        scale = 0;
        precision = 16 - i;
    }
}

void DecimalQuantity::readDecNumberToBcd(const DecNum& decnum) {
    const decNumber* dn = decnum.getRawDecNumber();
    if (dn->digits > 16) {
        ensureCapacity(dn->digits);
        for (int32_t i = 0; i < dn->digits; i++) {
            fBCD.bcdBytes.ptr[i] = dn->lsu[i];
        }
    } else {
        uint64_t result = 0L;
        for (int32_t i = 0; i < dn->digits; i++) {
            result |= static_cast<uint64_t>(dn->lsu[i]) << (4 * i);
        }
        fBCD.bcdLong = result;
    }
    scale = dn->exponent;
    precision = dn->digits;
}

void DecimalQuantity::readDoubleConversionToBcd(
        const char* buffer, int32_t length, int32_t point) {
    // NOTE: Despite the fact that double-conversion's API is called
    // "DoubleToAscii", they actually use '0' (as opposed to u8'0').
    if (length > 16) {
        ensureCapacity(length);
        for (int32_t i = 0; i < length; i++) {
            fBCD.bcdBytes.ptr[i] = buffer[length-i-1] - '0';
        }
    } else {
        uint64_t result = 0L;
        for (int32_t i = 0; i < length; i++) {
            result |= static_cast<uint64_t>(buffer[length-i-1] - '0') << (4 * i);
        }
        fBCD.bcdLong = result;
    }
    scale = point - length;
    precision = length;
}

void DecimalQuantity::compact() {
    if (usingBytes) {
        int32_t delta = 0;
        for (; delta < precision && fBCD.bcdBytes.ptr[delta] == 0; delta++);
        if (delta == precision) {
            // Number is zero
            setBcdToZero();
            return;
        } else {
            // Remove trailing zeros
            shiftRight(delta);
        }

        // Compute precision
        int32_t leading = precision - 1;
        for (; leading >= 0 && fBCD.bcdBytes.ptr[leading] == 0; leading--);
        precision = leading + 1;

        // Switch storage mechanism if possible
        if (precision <= 16) {
            switchStorage();
        }

    } else {
        if (fBCD.bcdLong == 0L) {
            // Number is zero
            setBcdToZero();
            return;
        }

        // Compact the number (remove trailing zeros)
        // TODO: Use a more efficient algorithm here and below. There is a logarithmic one.
        int32_t delta = 0;
        for (; delta < precision && getDigitPos(delta) == 0; delta++);
        fBCD.bcdLong >>= delta * 4;
        scale += delta;

        // Compute precision
        int32_t leading = precision - 1;
        for (; leading >= 0 && getDigitPos(leading) == 0; leading--);
        precision = leading + 1;
    }
}

void DecimalQuantity::ensureCapacity() {
    ensureCapacity(40);
}

void DecimalQuantity::ensureCapacity(int32_t capacity) {
    if (capacity == 0) { return; }
    int32_t oldCapacity = usingBytes ? fBCD.bcdBytes.len : 0;
    if (!usingBytes) {
        // TODO: There is nothing being done to check for memory allocation failures.
        // TODO: Consider indexing by nybbles instead of bytes in C++, so that we can
        // make these arrays half the size.
        fBCD.bcdBytes.ptr = static_cast<int8_t*>(uprv_malloc(capacity * sizeof(int8_t)));
        fBCD.bcdBytes.len = capacity;
        // Initialize the byte array to zeros (this is done automatically in Java)
        uprv_memset(fBCD.bcdBytes.ptr, 0, capacity * sizeof(int8_t));
    } else if (oldCapacity < capacity) {
        auto bcd1 = static_cast<int8_t*>(uprv_malloc(capacity * 2 * sizeof(int8_t)));
        uprv_memcpy(bcd1, fBCD.bcdBytes.ptr, oldCapacity * sizeof(int8_t));
        // Initialize the rest of the byte array to zeros (this is done automatically in Java)
        uprv_memset(bcd1 + oldCapacity, 0, (capacity - oldCapacity) * sizeof(int8_t));
        uprv_free(fBCD.bcdBytes.ptr);
        fBCD.bcdBytes.ptr = bcd1;
        fBCD.bcdBytes.len = capacity * 2;
    }
    usingBytes = true;
}

void DecimalQuantity::switchStorage() {
    if (usingBytes) {
        // Change from bytes to long
        uint64_t bcdLong = 0L;
        for (int i = precision - 1; i >= 0; i--) {
            bcdLong <<= 4;
            bcdLong |= fBCD.bcdBytes.ptr[i];
        }
        uprv_free(fBCD.bcdBytes.ptr);
        fBCD.bcdBytes.ptr = nullptr;
        fBCD.bcdLong = bcdLong;
        usingBytes = false;
    } else {
        // Change from long to bytes
        // Copy the long into a local variable since it will get munged when we allocate the bytes
        uint64_t bcdLong = fBCD.bcdLong;
        ensureCapacity();
        for (int i = 0; i < precision; i++) {
            fBCD.bcdBytes.ptr[i] = static_cast<int8_t>(bcdLong & 0xf);
            bcdLong >>= 4;
        }
        U_ASSERT(usingBytes);
    }
}

void DecimalQuantity::copyBcdFrom(const DecimalQuantity &other) {
    setBcdToZero();
    if (other.usingBytes) {
        ensureCapacity(other.precision);
        uprv_memcpy(fBCD.bcdBytes.ptr, other.fBCD.bcdBytes.ptr, other.precision * sizeof(int8_t));
    } else {
        fBCD.bcdLong = other.fBCD.bcdLong;
    }
}

void DecimalQuantity::moveBcdFrom(DecimalQuantity &other) {
    setBcdToZero();
    if (other.usingBytes) {
        usingBytes = true;
        fBCD.bcdBytes.ptr = other.fBCD.bcdBytes.ptr;
        fBCD.bcdBytes.len = other.fBCD.bcdBytes.len;
        // Take ownership away from the old instance:
        other.fBCD.bcdBytes.ptr = nullptr;
        other.usingBytes = false;
    } else {
        fBCD.bcdLong = other.fBCD.bcdLong;
    }
}

const char16_t* DecimalQuantity::checkHealth() const {
    if (usingBytes) {
        if (precision == 0) { return u"Zero precision but we are in byte mode"; }
        int32_t capacity = fBCD.bcdBytes.len;
        if (precision > capacity) { return u"Precision exceeds length of byte array"; }
        if (getDigitPos(precision - 1) == 0) { return u"Most significant digit is zero in byte mode"; }
        if (getDigitPos(0) == 0) { return u"Least significant digit is zero in long mode"; }
        for (int i = 0; i < precision; i++) {
            if (getDigitPos(i) >= 10) { return u"Digit exceeding 10 in byte array"; }
            if (getDigitPos(i) < 0) { return u"Digit below 0 in byte array"; }
        }
        for (int i = precision; i < capacity; i++) {
            if (getDigitPos(i) != 0) { return u"Nonzero digits outside of range in byte array"; }
        }
    } else {
        if (precision == 0 && fBCD.bcdLong != 0) {
            return u"Value in bcdLong even though precision is zero";
        }
        if (precision > 16) { return u"Precision exceeds length of long"; }
        if (precision != 0 && getDigitPos(precision - 1) == 0) {
            return u"Most significant digit is zero in long mode";
        }
        if (precision != 0 && getDigitPos(0) == 0) {
            return u"Least significant digit is zero in long mode";
        }
        for (int i = 0; i < precision; i++) {
            if (getDigitPos(i) >= 10) { return u"Digit exceeding 10 in long"; }
            if (getDigitPos(i) < 0) { return u"Digit below 0 in long (?!)"; }
        }
        for (int i = precision; i < 16; i++) {
            if (getDigitPos(i) != 0) { return u"Nonzero digits outside of range in long"; }
        }
    }

    // No error
    return nullptr;
}

bool DecimalQuantity::operator==(const DecimalQuantity& other) const {
    // FIXME: Make a faster implementation.
    return toString() == other.toString();
}

UnicodeString DecimalQuantity::toString() const {
    MaybeStackArray<char, 30> digits(precision + 1);
    for (int32_t i = 0; i < precision; i++) {
        digits[i] = getDigitPos(precision - i - 1) + '0';
    }
    digits[precision] = 0; // terminate buffer
    char buffer8[100];
    snprintf(
            buffer8,
            sizeof(buffer8),
            "<DecimalQuantity %d:%d:%d:%d %s %s%s%s%d>",
            (lOptPos > 999 ? 999 : lOptPos),
            lReqPos,
            rReqPos,
            (rOptPos < -999 ? -999 : rOptPos),
            (usingBytes ? "bytes" : "long"),
            (isNegative() ? "-" : ""),
            (precision == 0 ? "0" : digits.getAlias()),
            "E",
            scale);
    return UnicodeString(buffer8, -1, US_INV);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
