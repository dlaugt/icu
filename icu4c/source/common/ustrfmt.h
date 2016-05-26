/*
**********************************************************************
*   Copyright (C) 2016 and later: Unicode, Inc. and others.
*   License & terms of use: http://www.unicode.org/copyright.html
**********************************************************************
*/

#ifndef USTRFMT_H
#define USTRFMT_H

#include "unicode/utypes.h"

U_CAPI int32_t U_EXPORT2
uprv_itou (UChar * buffer, int32_t capacity, uint32_t i, uint32_t radix, int32_t minwidth);


#endif
