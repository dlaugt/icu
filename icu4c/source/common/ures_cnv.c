/*
*******************************************************************************
*
*   Copyright (C) 1997-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ures_cnv.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004aug25
*   created by: Markus W. Scherer
*
*   Character conversion functions moved here from uresbund.c
*/

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/ures.h"
#include "ustr_cnv.h"

U_CAPI UResourceBundle * U_EXPORT2
ures_openU(const UChar *myPath, 
           const char *localeID, 
           UErrorCode *status) {
    char path[2048];
    int32_t length;

    if(status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(myPath==NULL) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    length=u_strlen(myPath);
    if(length>=sizeof(path)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else if(uprv_isInvariantUString(myPath, length)) {
        /*
         * the invariant converter is sufficient for package and tree names
         * and is more efficient
         */
        u_UCharsToChars(myPath, path, length+1); /* length+1 to include the NUL */
    } else {
#if !UCONFIG_NO_CONVERSION
        /* use the default converter to support variant-character paths */
        UConverter *cnv=u_getDefaultConverter(status);
        if(U_FAILURE(*status)) {
            return NULL;
        }

        length=ucnv_fromUChars(cnv, path, (int32_t)sizeof(path), myPath, length, status);
        u_releaseDefaultConverter(cnv);
        if(U_FAILURE(*status)) {
            return NULL;
        }
        if(length>=sizeof(path)) {
            /* not NUL-terminated - path too long */
            *status=U_ILLEGAL_ARGUMENT_ERROR;
            return NULL;
        }
#else
        /* the default converter is not available */
        *status=U_UNSUPPORTED_ERROR;
        return NULL;
#endif
    }

    return ures_open(path, localeID, status);
}
