/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2000-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*
* File stdnmtst.c
*
* Modification History:
*
*   Date          Name        Description
*   08/05/2000    Yves       Creation 
*******************************************************************************
*/

#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "cintltst.h"

#define ARRAY_SIZE(array) (int32_t)(sizeof array  / sizeof array[0])

static void TestStandardName(void);
static void TestStandardNames(void);
static void TestCanonicalName(void);

void addStandardNamesTest(TestNode** root);


void
addStandardNamesTest(TestNode** root)
{
  addTest(root, &TestStandardName,  "stdnmtst/TestStandardName");
  addTest(root, &TestStandardNames, "stdnmtst/TestStandardNames");
  addTest(root, &TestCanonicalName, "stdnmtst/TestCanonicalName");
}

static int dotestname(const char *name, const char *standard, const char *expected) {
    int res = 1;

    UErrorCode error;
    const char *tag;

    error = U_ZERO_ERROR;
    tag = ucnv_getStandardName(name, standard, &error);
    if (!tag && expected) {
        log_err("FAIL: could not find %s standard name for %s\n", standard, name);
        res = 0;
    } else if (expected && (name == tag || uprv_strcmp(expected, tag))) {
        log_err("FAIL: expected %s for %s standard name for %s, got %s\n", expected, standard, name, tag);
        res = 0;
    }

    return res;
}

static void TestStandardName()
{
    int res = 1;

    uint16_t i, count;
    UErrorCode err;

    /* Iterate over all standards. */

    for (i = 0, count = ucnv_countStandards(); i < count-1; ++i) {
        const char *standard;

        err = U_ZERO_ERROR;
        standard = ucnv_getStandard(i, &err);
        if (U_FAILURE(err)) {
            log_err("FAIL: ucnv_getStandard(%d), error=%s\n", i, u_errorName(err));
            res = 0;
        } else if (!standard || !*standard) {
            log_err("FAIL: %s standard name at index %d\n", (standard ? "empty" :
                "null"), i);
            res = 0;
        }
    }
    err = U_ZERO_ERROR;
    /* "" must be last */
    if (*ucnv_getStandard((uint16_t)(count-1), &err) != 0) {
        log_err("FAIL: ucnv_getStandard(%d) should return ""\n", count-1);
        res = 0;
    }
    err = U_ZERO_ERROR;
    if (ucnv_getStandard(++i, &err)) {
        log_err("FAIL: ucnv_getStandard(%d) should return NULL\n", i);
        res = 0;
    }

    if (res) {
        log_verbose("PASS: iterating over standard names works\n");
    }

    /* Test for some expected results. */

    if (dotestname("ibm-1208", "MIME", "UTF-8") &&
        /*dotestname("cp1252", "MIME", "windows-1252") &&*/
        dotestname("ascii", "MIME", "US-ASCII") &&
        dotestname("ISO_2022", "MIME", "ISO-2022") &&
        dotestname("ISO-2022", "MIME", "ISO-2022") &&
        dotestname("ascii", "IANA", "ANSI_X3.4-1968") &&
        dotestname("cp850", "IANA", "IBM850") &&
        dotestname("crazy", "MIME", NULL) &&
        dotestname("ASCII", "crazy", NULL) &&
        dotestname("LMBCS-1", "MIME", NULL))
    {
        log_verbose("PASS: getting IANA and MIME standard names works\n");
    }
}

static int dotestconv(const char *name, const char *standard, const char *expected) {
    int res = 1;

    UErrorCode error;
    const char *tag;

    error = U_ZERO_ERROR;
    tag = ucnv_getCanonicalName(name, standard, &error);
    if (tag && !expected) {
        log_err("FAIL: Unexpectedly found %s canonical name for %s, got %s\n", standard, name, tag);
        res = 0;
    } else if (!tag && expected) {
        log_err("FAIL: could not find %s canonical name for %s\n", (standard ? "\"\"" : standard), name);
        res = 0;
    } else if (expected && (name == tag || uprv_strcmp(expected, tag)) && error == U_ZERO_ERROR) {
        log_err("FAIL: expected %s for %s canonical name for %s, got %s\n", expected, standard, name, tag);
        res = 0;
    }

    return res;
}

static void TestCanonicalName()
{
    /* Test for some expected results. */

    if (dotestconv("UTF-8", "IANA", "UTF-8") &&     /* default name */
        dotestconv("UTF-8", "MIME", "UTF-8") &&     /* default name */
        dotestconv("ibm-1208", "IBM", "UTF-8") &&   /* default name */
        dotestconv("ibm-5305", "IBM", "UTF-8") &&   /* non-default name */
        dotestconv("ibm-5305", "MIME", NULL) &&     /* mapping does not exist */
        dotestconv("ascii", "MIME", NULL) &&        /* mapping does not exist */
        dotestconv("ibm-1208", "IANA", NULL) &&     /* mapping does not exist */
        dotestconv("ibm-5305", "IANA", NULL) &&     /* mapping does not exist */
        dotestconv("cp1208", "", "UTF-8") &&        /* default name due to ordering */
        dotestconv("UTF16_BigEndian", "", "UTF-16BE") &&        /* non-default name due to ordering */
        dotestconv("ISO-2022", "MIME", "ISO_2022") &&/* default name */
        dotestconv("Shift_JIS", "MIME", "ibm-943_P14A-1999") &&/* ambiguous alias */
        dotestconv("Shift_JIS", "", "ibm-943_P130-1999") &&/* ambiguous alias */
        dotestconv("ibm-943", "", "ibm-943_P14A-1999") &&/* ambiguous alias */
        dotestconv("ibm-943", "IBM", "ibm-943_P130-1999") &&/* ambiguous alias */
        dotestconv("ibm-1363", "", "ibm-1363_P11B-1998") &&/* ambiguous alias */
        dotestconv("ibm-1363", "IBM", "ibm-1363_P110-1997") &&/* ambiguous alias */
        dotestconv("crazy", "MIME", NULL) &&
        dotestconv("ASCII", "crazy", NULL))
    {
        log_verbose("PASS: getting IANA and MIME canonical names works\n");
    }
}


static UBool doTestNames(const char *name, const char *standard, const char **expected, int32_t size) {
    UErrorCode err = U_ZERO_ERROR;
    UEnumeration *myEnum = ucnv_openStandardNames(name, standard, &err);
    int32_t enumCount = uenum_count(myEnum, &err);
    int32_t idx, repeatTimes = 3;
    if (size != enumCount) {
        log_err("FAIL: different size arrays. Got %d. Expected %d\n", enumCount, size);
        return 0;
    }
    if (size < 0 && myEnum) {
        log_err("FAIL: size < 0, but recieved an actual object\n");
        return 0;
    }
    log_verbose("\n%s %s\n", name, standard);
    while (repeatTimes-- > 0) {
        for (idx = 0; idx < enumCount; idx++) {
            int32_t len;
            const char *enumName = uenum_next(myEnum, &len, &err);
            const char *testName = expected[idx];
            if (uprv_strcmp(enumName, testName) != 0 || U_FAILURE(err)
                || len != (int32_t)uprv_strlen(expected[idx]))
            {
                log_err("FAIL: uenum_next(%d) == \"%s\". expected \"%s\", len=%d, error=%s\n",
                    idx, enumName, testName, len, u_errorName(err));
            }
            log_verbose("%s\n", enumName);
            err = U_ZERO_ERROR;
        }
        log_verbose("\n    reset\n");
        uenum_reset(myEnum, &err);
        if (U_FAILURE(err)) {
            log_err("FAIL: uenum_reset() for %s{%s} failed with %s\n",
                name, standard, u_errorName(err));
            err = U_ZERO_ERROR;
        }
    }
    uenum_close(myEnum);
    return 1;
}

static UBool doTestUCharNames(const char *name, const char *standard, const char **expected, int32_t size) {
    UErrorCode err = U_ZERO_ERROR;
    UEnumeration *myEnum = ucnv_openStandardNames(name, standard, &err);
    int32_t enumCount = uenum_count(myEnum, &err);
    int32_t idx, repeatTimes = 3;
    if (size != enumCount) {
        log_err("FAIL: different size arrays. Got %d. Expected %d\n", enumCount, size);
        return 0;
    }
    if (size < 0 && myEnum) {
        log_err("FAIL: size < 0, but recieved an actual object\n");
        return 0;
    }
    log_verbose("\n%s %s\n", name, standard);
    while (repeatTimes-- > 0) {
        for (idx = 0; idx < enumCount; idx++) {
            UChar testName[256];
            int32_t len;
            const UChar *enumName = uenum_unext(myEnum, &len, &err);
            u_uastrncpy(testName, expected[idx], sizeof(testName)/sizeof(testName[0]));
            if (u_strcmp(enumName, testName) != 0 || U_FAILURE(err)
                || len != (int32_t)uprv_strlen(expected[idx]))
            {
                log_err("FAIL: uenum_next(%d) == \"%s\". expected \"%s\", len=%d, error=%s\n",
                    idx, enumName, testName, len, u_errorName(err));
            }
            log_verbose("%s\n", expected[idx]);
            err = U_ZERO_ERROR;
        }
        log_verbose("\n    reset\n");
        uenum_reset(myEnum, &err);
        if (U_FAILURE(err)) {
            log_err("FAIL: uenum_reset() for %s{%s} failed with %s\n",
                name, standard, u_errorName(err));
            err = U_ZERO_ERROR;
        }
    }
    uenum_close(myEnum);
    return 1;
}

static void TestStandardNames()
{
    static const char *asciiIANA[] = {
        "ANSI_X3.4-1968",
        "US-ASCII",
        "ASCII",
        "ANSI_X3.4-1986",
        "ISO_646.irv:1991",
        "ISO646-US",
        "us",
        "csASCII",
        "iso-ir-6",
        "cp367",
    };
    static const char *asciiMIME[] = {
        "US-ASCII"
    };

    static const char *iso2022MIME[] = {
        "ISO-2022",
    };

    doTestNames("ASCII", "IANA", asciiIANA, ARRAY_SIZE(asciiIANA));
    doTestNames("US-ASCII", "IANA", asciiIANA, ARRAY_SIZE(asciiIANA));
    doTestNames("ASCII", "MIME", asciiMIME, ARRAY_SIZE(asciiMIME));
    doTestNames("ascii", "mime", asciiMIME, ARRAY_SIZE(asciiMIME));

    doTestNames("ASCII", "crazy", asciiMIME, -1);
    doTestNames("crazy", "MIME", asciiMIME, -1);

    doTestNames("LMBCS-1", "MIME", asciiMIME, 0);

    doTestNames("ISO_2022", "MIME", iso2022MIME, ARRAY_SIZE(iso2022MIME));
    doTestNames("ISO-2022", "MIME", iso2022MIME, ARRAY_SIZE(iso2022MIME));

    log_verbose(" Testing unext()\n");
    doTestUCharNames("ASCII", "IANA", asciiIANA, ARRAY_SIZE(asciiIANA));

}
