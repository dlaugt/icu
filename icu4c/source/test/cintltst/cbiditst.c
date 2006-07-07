/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*   file name:  cbiditst.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep27
*   created by: Markus W. Scherer
*/

#include "cintltst.h"
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/ubidi.h"
#include "unicode/ushape.h"
#include "cmemory.h"
#include "cbiditst.h"
#include "cstring.h"
/* the following include is needed for sprintf */
#include <stdio.h>

#define MAXLEN      MAX_STRING_LENGTH
#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

/* prototypes ---------------------------------------------------------------*/

static void
charFromDirPropTest(void);

static void
doBiDiTest(void);

static void
doTests(UBiDi *pBiDi, UBiDi *pLine, UBool countRunsFirst);

static void
doTest(UBiDi *pBiDi, int testNumber, BiDiTestData *test, int32_t lineStart, UBool countRunsFirst);

static void
testReordering(UBiDi *pBiDi, int testNumber);

static void
doInverseBiDiTest(void);

static void
testManyInverseBiDi(UBiDi *pBiDi, UBiDiLevel direction);

static void
testInverseBiDi(UBiDi *pBiDi, const UChar *src, int32_t srcLength, UBiDiLevel direction, UErrorCode *pErrorCode);

static void
testWriteReverse(void);

static void
doArabicShapingTest(void);

static void
doLamAlefSpecialVLTRArabicShapingTest(void);

static void
doTashkeelSpecialVLTRArabicShapingTest(void);

static void
doLOGICALArabicDeShapingTest(void);

static void TestReorder(void);

static void TestFailureRecovery(void);

static void TestMultipleParagraphs(void);

/* new BIDI API */
static void doReorderingModeBidiTest(void);
static void doReorderRunsTest(void);
static void doBidiStreamingTest(void);
static void doBidiClassOverrideTest(void);
static const char* inverseBasic(UBiDi *pBiDi, const UChar *src, int32_t srcLen,
                                uint32_t option, UBiDiLevel level, char *result);
static UBool assertRoundTrip(UBiDi *pBiDi, int32_t tc, int32_t outIndex,
                             const char *srcChars, const char *destChars,
                             const UChar *dest, int32_t destLen, int mode,
                             int option, UBiDiLevel level);
static UBool checkResultLength(UBiDi *pBiDi, const char *srcChars,
                               const char *destChars, const UChar *dest,
                               int32_t destLen, const char *mode,
                               const char *option, UBiDiLevel level);
static UBool testMaps(UBiDi *pBiDi, int32_t stringIndex, const char *src,
                      const char *dest, const char *mode, const char* option,
                      UBiDiLevel level, UBool forward);

/* helpers ------------------------------------------------------------------ */

static const char *levelString="...............................................................";

static void
initCharFromDirProps(void);

static UChar *
getStringFromDirProps(const uint8_t *dirProps, int32_t length, UChar *buffer);

static void
printUnicode(const UChar *s, int32_t length, const UBiDiLevel *levels);

/* regression tests ---------------------------------------------------------*/

void addComplexTest(TestNode** root);

void
addComplexTest(TestNode** root) {
    addTest(root, charFromDirPropTest, "complex/bidi/charFromDirPropTest");
    addTest(root, doBiDiTest, "complex/bidi/BiDiTest");
    addTest(root, doInverseBiDiTest, "complex/bidi/inverse");
    addTest(root, TestReorder,"complex/bidi/TestReorder");
    addTest(root, TestFailureRecovery,"complex/bidi/TestFailureRecovery");
    addTest(root, TestMultipleParagraphs,"complex/bidi/multipleParagraphs");
    addTest(root, doArabicShapingTest, "complex/arabic-shaping/ArabicShapingTest");
    addTest(root, doLamAlefSpecialVLTRArabicShapingTest, "complex/arabic-shaping/lamalef");
    addTest(root, doTashkeelSpecialVLTRArabicShapingTest, "complex/arabic-shaping/tashkeel");
    addTest(root, doLOGICALArabicDeShapingTest, "complex/arabic-shaping/unshaping");
/* new BIDI API */
    addTest(root, doReorderingModeBidiTest, "complex/new-bidi-api/TestReorderingMode");
    addTest(root, doReorderRunsTest, "complex/new-bidi-api/TestReorderRunsOnly");
    addTest(root, doBidiStreamingTest, "complex/new-bidi-api/TestStreamingMode");
    addTest(root, doBidiClassOverrideTest, "complex/new-bidi-api/TestClassOverride");
}

/* verify that the exemplar characters have the expected bidi classes */
static void
charFromDirPropTest(void) {
    int32_t i;

    initCharFromDirProps();

    for(i=0; i<U_CHAR_DIRECTION_COUNT; ++i) {
        if(u_charDirection(charFromDirProp[i])!=(UCharDirection)i) {
            log_err("\nu_charDirection(charFromDirProp[%d]=U+%04x)==%d!=%d\n",
                    i, charFromDirProp[i], u_charDirection(charFromDirProp[i]), i);
        }
    }
}

static void
doBiDiTest() {
    UBiDi *pBiDi, *pLine=NULL;
    UErrorCode errorCode=U_ZERO_ERROR;

    log_verbose("\n*** bidi regression test ***\n");

    pBiDi=ubidi_openSized(MAXLEN, 0, &errorCode);
    if(pBiDi!=NULL) {
        pLine=ubidi_open();
        if(pLine!=NULL) {
            doTests(pBiDi, pLine, FALSE);
            doTests(pBiDi, pLine, TRUE);
        } else {
            log_err("ubidi_open() returned NULL, out of memory\n");
        }
    } else {
        log_err("ubidi_openSized() returned NULL, errorCode %s\n", myErrorName(errorCode));
    }

    if(pLine!=NULL) {
        ubidi_close(pLine);
    }
    if(pBiDi!=NULL) {
        ubidi_close(pBiDi);
    }

    log_verbose("\n*** bidi regression test finished ***\n");
}

static void
doTests(UBiDi *pBiDi, UBiDi *pLine, UBool countRunsFirst) {
    int i;
    UChar string[MAXLEN];
    UErrorCode errorCode;
    int32_t lineStart;
    UBiDiLevel paraLevel;

    for(i=0; i<bidiTestCount; ++i) {
        errorCode=U_ZERO_ERROR;
        getStringFromDirProps(tests[i].text, tests[i].length, string);
        paraLevel=tests[i].paraLevel;
        ubidi_setPara(pBiDi, string, -1, paraLevel, NULL, &errorCode);
        if(U_SUCCESS(errorCode)) {
            log_verbose("ubidi_setPara(tests[%d], paraLevel %d) ok, direction %d paraLevel=%d\n",
                    i, paraLevel, ubidi_getDirection(pBiDi), ubidi_getParaLevel(pBiDi));
            lineStart=tests[i].lineStart;
            if(lineStart==-1) {
                doTest(pBiDi, i, tests+i, 0, countRunsFirst);
            } else {
                ubidi_setLine(pBiDi, lineStart, tests[i].lineLimit, pLine, &errorCode);
                if(U_SUCCESS(errorCode)) {
                    log_verbose("ubidi_setLine(%d, %d) ok, direction %d paraLevel=%d\n",
                            lineStart, tests[i].lineLimit, ubidi_getDirection(pLine), ubidi_getParaLevel(pLine));
                    doTest(pLine, i, tests+i, lineStart, countRunsFirst);
                } else {
                    log_err("ubidi_setLine(tests[%d], %d, %d) failed with errorCode %s\n",
                            i, lineStart, tests[i].lineLimit, myErrorName(errorCode));
                }
            }
        } else {
            log_err("ubidi_setPara(tests[%d], paraLevel %d) failed with errorCode %s\n",
                    i, paraLevel, myErrorName(errorCode));
        }
    }
}

static const UChar pseudoToUChar[] = {
//     0/8     1/9     2/A     3/B     4/C     5/D     6/E     7/F
      0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07, //00-07
      0x08,   0x09,   0x0A,   0x0B,   0x0C,   0x0D,   0x0E,   0x0F, //08-0F
      0x10,   0x11,   0x12,   0x13,   0x14,   0x15,   0x16,   0x17, //10-17
      0x18,   0x19,   0x1A,   0x1B,   0x1C,   0x1D,   0x1E, 0xE01F, //18-1F
      0x20,   0x21,   0x22,   0x23,   0x24,   0x25, 0x200F,   0x27, //20-27
      0x28,   0x29,   0x2A,   0x2B,   0x2C,   0x2D,   0x2E,   0x2F, //28-2F
      0x30,   0x31,   0x32,   0x33,   0x34,   0x35, 0x0666, 0x0667, //30-37
    0x0668, 0x0669,   0x3A,   0x3B,   0x3C,   0x3D,   0x3E,   0x3F, //38-3F
    0x200E, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x05D7, //40-47
    0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF, //48-4F
    0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, //50-57
    0x05E8, 0x05E9, 0x05EA, 0x202A,   0x5C, 0x202B, 0x202C, 0x001F, //58-5F
    0x0300,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,   0x67, //60-67
      0x68,   0x69,   0x6A,   0x6B,   0x6C,   0x6D,   0x6E,   0x6F, //68-6F
      0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77, //70-77
      0x78,   0x79,   0x7A, 0x202D, 0x2029, 0x202E, 0x007F, 0xE07F, //78-7F
      0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87, //80-87
      0x88,   0x89,   0x8A,   0x8B,   0x8C,   0x8D,   0x8E,   0x8F, //88-8F
      0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97, //90-97
      0x98,   0x99,   0x9A,   0x9B,   0x9C,   0x9D,   0x9E,   0x9F, //98-9F
      0xA0,   0xA1,   0xA2,   0xA3,   0xA4,   0xA5,   0xA6,   0xA7, //A0-A7
      0xA8,   0xA9,   0xAA,   0xAB,   0xAC,   0xAD,   0xAE,   0xAF, //A8-AF
      0xB0,   0xB1,   0xB2,   0xB3,   0xB4,   0xB5,   0xB6,   0xB7, //B0-B7
      0xB8,   0xB9,   0xBA,   0xBB,   0xBC,   0xBD,   0xBE,   0xBF, //B8-BF
      0xC0,   0xC1,   0xC2,   0xC3,   0xC4,   0xC5,   0xC6,   0xC7, //C0-C7
      0xC8,   0xC9,   0xCA,   0xCB,   0xCC,   0xCD,   0xCE,   0xCF, //C8-CF
      0xD0,   0xD1,   0xD2,   0xD3,   0xD4,   0xD5,   0xD6,   0xD7, //D0-D7
      0xD8,   0xD9,   0xDA,   0xDB,   0xDC,   0xDD,   0xDE,   0xDF, //D8-DF
      0xE0,   0xE1,   0xE2,   0xE3,   0xE4,   0xE5,   0xE6,   0xE7, //E0-E7
      0xE8,   0xE9,   0xEA,   0xEB,   0xEC,   0xED,   0xEE,   0xEF, //E8-EF
      0xF0,   0xF1,   0xF2,   0xF3,   0xF4,   0xF5,   0xF6,   0xF7, //F0-F7
      0xF8,   0xF9,   0xFA,   0xFB,   0xFC,   0xFD,   0xFE,   0xFF};//F8-FF

static const unsigned char UCharToPseudo[] = {
//     0/8     1/9     2/A     3/B     4/C     5/D     6/E     7/F
       '`',   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07, //00-07
      0x08,   0x09,   0x0A,   0x0B,   0x0C,   0x0D,    '@',    '&', //08-0F
      0x10,   0x11,   0x12,   0x13,   0x14,   0x15,   0x16,   0x17, //10-17
      0x18,   0x19,   0x1A,   0x1B,   0x1C,   0x1D,   0x1E,    '_', //18-1F
      0x20,   0x21,   0x22,   0x23,   0x24,   0x25,   0x26,   0x27, //20-27
      0x28,    '|',    '[',    ']',    '^',    '{',    '}',   0x2F, //28-2F
      0x30,    'A',    'B',    'C',    'D',    'E',    'F',   0x37, //30-37
      0x38,   0x39,   0x3A,   0x3B,   0x3C,   0x3D,   0x3E,   0x3F, //38-3F
      0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47, //40-47
      0x48,   0x49,   0x4A,   0x4B,   0x4C,   0x4D,   0x4E,   0x4F, //48-4F
      0x50,   0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57, //50-57
      0x58,   0x59,   0x5A,   0x5B,   0x5C,   0x5D,   0x5E,   0x5F, //58-5F
      0x60,   0x61,   0x62,   0x63,   0x64,   0x65,    '6',    '7', //60-67
       '8',    '9',   0x6A,   0x6B,   0x6C,   0x6D,   0x6E,   0x6F, //68-6F
      0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77, //70-77
      0x78,   0x79,   0x7A,   0x7B,   0x7C,   0x7D,   0x7E,    '~', //78-7F
      0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87, //80-87
      0x88,   0x89,   0x8A,   0x8B,   0x8C,   0x8D,   0x8E,   0x8F, //88-8F
      0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97, //90-97
      0x98,   0x99,   0x9A,   0x9B,   0x9C,   0x9D,   0x9E,   0x9F, //98-9F
      0xA0,   0xA1,   0xA2,   0xA3,   0xA4,   0xA5,   0xA6,   0xA7, //A0-A7
      0xA8,   0xA9,   0xAA,   0xAB,   0xAC,   0xAD,   0xAE,   0xAF, //A8-AF
      0xB0,   0xB1,   0xB2,   0xB3,   0xB4,   0xB5,   0xB6,   0xB7, //B0-B7
      0xB8,   0xB9,   0xBA,   0xBB,   0xBC,   0xBD,   0xBE,   0xBF, //B8-BF
      0xC0,   0xC1,   0xC2,   0xC3,   0xC4,   0xC5,   0xC6,   0xC7, //C0-C7
      0xC8,   0xC9,   0xCA,   0xCB,   0xCC,   0xCD,   0xCE,   0xCF, //C8-CF
      0xD0,   0xD1,   0xD2,   0xD3,   0xD4,   0xD5,   0xD6,    'G', //D0-D7
       'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O', //D8-DF
       'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W', //E0-E7
       'X',    'Y',    'Z',   0xEB,   0xEC,   0xED,   0xEE,   0xEF, //E8-EF
      0xF0,   0xF1,   0xF2,   0xF3,   0xF4,   0xF5,   0xF6,   0xF7, //F0-F7
      0xF8,   0xF9,   0xFA,   0xFB,   0xFC,   0xFD,   0xFE,   0xFF};//F8-FF

static int pseudoToU16( const int length, const char * input, UChar * output )
/*  This function converts a pseudo-Bidi string into a UChar string.
    It returns the length of the UChar string.

    The rules for pseudo-Bidi are as follows:
    - [ == LRE
    - ] == RLE
    - { == LRO
    - } == RLO
    - ^ == PDF
    - @ == LRM
    - & == RLM
    - A-F == Arabic Letters 0631-0636
    - G-Z == Hebrew letters 05d7-05ea
    - 0-5 == western digits 0030-0035
    - 6-9 == Arabic-Indic digits 0666-0669
    - ` == Combining Grave Accent 0300 (NSM)
    - ~ == Delete 007f (BN)
    - | == Paragraph Separator 2029 (B)
    - _ == Info Separator 1 001f (S)
    All other characters represent themselves as Latin-1, with the corresponding
    Bidi properties.

*/
{
    int             i;

    for (i = 0; i < length; i++)
        output[i] = pseudoToUChar[(unsigned char)input[i]];
    return length;
}

/*----------------------------------------------------------------------*/

static int u16ToPseudo( const int length, const UChar * input, char * output )
/*  This function converts a UChar string into a pseudo-Bidi string.
    It returns the length of the pseudo-Bidi string.
*/
{
    int             i;
    UChar           u;
    unsigned char   v;

    for (i = 0; i < length; i++)
    {
        u = input[i];
        v = UCharToPseudo[u & 0x00ff];
        if (pseudoToUChar[v] == u)
            output[i] = v;
        else  output[i] = (char)(u & 0x00ff);   /* keep 8 right bits */
    }
    output[length] = '\0';
    return length;
}

static char * formatLevels(UBiDi *bidi, char *buffer) {
    UErrorCode ec = U_ZERO_ERROR;
    const UBiDiLevel* gotLevels = ubidi_getLevels(bidi, &ec);
    int len = ubidi_getLength(bidi);
    char c;
    int i, k;

    if(U_FAILURE(ec)) {
        strcpy(buffer, "BAD LEVELS");
        return buffer;
    }
    for (i=0; i<len; i++) {
        k = gotLevels[i];
        if (k<=9)
            c = '0' + k;
        else if (k <= 35)
            c = ('A' - 10) + k;
        else if (k <= 61)
            c = ('a' - 36) + k;
        else c = '?';
        buffer[i] = c;
    }
    buffer[len] = '\0';
    return buffer;
}

static void TestReorder(){
    static const char* const logicalOrder[] ={
            "del(KC)add(K.C.&)",
            "del(QDVT) add(BVDL)",
            "del(PQ)add(R.S.)T)U.&",
            "del(LV)add(L.V.) L.V.&",
            "day  0  R  DPDHRVR dayabbr",
            "day  1  H  DPHPDHDA dayabbr",
            "day  2   L  DPBLENDA dayabbr",
            "day  3  J  DPJQVM  dayabbr",
            "day  4   I  DPIQNF    dayabbr",
            "day  5  M  DPMEG  dayabbr",
            "helloDPMEG",
    };
    static const char* const visualOrder[]={
            "del(CK)add(&.C.K)",
            "del(TVDQ) add(LDVB)",
            "del(QP)add(&.U(T(.S.R",
            "del(VL)add(&.V.L (.V.L",
            "day  0  RVRHDPD  R dayabbr",
            "day  1  ADHDPHPD  H dayabbr",
            "day  2   ADNELBPD  L dayabbr",
            "day  3  MVQJPD  J  dayabbr",
            "day  4   FNQIPD  I    dayabbr",
            "day  5  GEMPD  M  dayabbr",
            "helloGEMPD",
    };
    static const char* const visualOrder1[]={
            ")K.C.&(dda)KC(led",
            ")BVDL(dda )QDVT(led",
            "R.S.(T(U.&(dda)PQ(led",
            "L.V.( L.V.&(dda)LV(led",
            "rbbayad R  DPDHRVR  0  yad",
            "rbbayad H  DPHPDHDA  1  yad",
            "rbbayad L  DPBLENDA   2  yad",
            "rbbayad  J  DPJQVM  3  yad",
            "rbbayad    I  DPIQNF   4  yad",
            "rbbayad  M  DPMEG  5  yad",
            "DPMEGolleh",
    };

    static const char* const visualOrder2[]={
            "@)@K.C.&@(dda)@KC@(led",
            "@)@BVDL@(dda )@QDVT@(led",
            "R.S.)T)U.&@(dda)@PQ@(led",
            "L.V.) L.V.&@(dda)@LV@(led",
            "rbbayad @R  DPDHRVR@  0  yad",
            "rbbayad @H  DPHPDHDA@  1  yad",
            "rbbayad @L  DPBLENDA@   2  yad",
            "rbbayad  @J  DPJQVM@  3  yad",
            "rbbayad    @I  DPIQNF@   4  yad",
            "rbbayad  @M  DPMEG@  5  yad",
            "DPMEGolleh",
    };
    static const char* const visualOrder3[]={
            ")K.C.&(KC)dda(led",
            ")BVDL(ddaQDVT) (led",
            "R.S.)T)U.&(PQ)dda(led",
            "L.V.) L.V.&(LV)dda(led",
            "rbbayad DPDHRVR   R  0 yad",
            "rbbayad DPHPDHDA   H  1 yad",
            "rbbayad DPBLENDA     L 2 yad",
            "rbbayad  DPJQVM   J  3 yad",
            "rbbayad    DPIQNF     I 4 yad",
            "rbbayad  DPMEG   M  5 yad",
            "DPMEGolleh"
    };
    static const char* const visualOrder4[]={
            "del(add(CK(.C.K)",
            "del( (TVDQadd(LDVB)",
            "del(add(QP(.U(T(.S.R",
            "del(add(VL(.V.L (.V.L",
            "day 0  R   RVRHDPD dayabbr",
            "day 1  H   ADHDPHPD dayabbr",
            "day 2 L     ADNELBPD dayabbr",
            "day 3  J   MVQJPD  dayabbr",
            "day 4 I     FNQIPD    dayabbr",
            "day 5  M   GEMPD  dayabbr",
            "helloGEMPD"
    };
    char formatChars[MAXLEN];
    UErrorCode ec = U_ZERO_ERROR;
    UBiDi* bidi = ubidi_open();
    int i;
    for(i=0;i<LENGTHOF(logicalOrder);i++){
        int32_t srcSize = (int32_t)strlen(logicalOrder[i]);
        int32_t destSize = srcSize*2;
        UChar src[MAXLEN];
        UChar dest[MAXLEN];
        char chars[MAXLEN];
        pseudoToU16(srcSize,logicalOrder[i],src);
        ec = U_ZERO_ERROR;
        ubidi_setPara(bidi,src,srcSize,UBIDI_DEFAULT_LTR ,NULL,&ec);
        if(U_FAILURE(ec)){
            log_err("ubidi_setPara(tests[%d], paraLevel %d) failed with errorCode %s\n",
                    i, UBIDI_DEFAULT_LTR, u_errorName(ec));
        }
        /* try pre-flighting */
        destSize = ubidi_writeReordered(bidi,dest,0,UBIDI_DO_MIRRORING,&ec);
        if(ec!=U_BUFFER_OVERFLOW_ERROR){
            log_err("Pre-flighting did not give expected error: Expected: U_BUFFER_OVERFLOW_ERROR. Got: %s \n",u_errorName(ec));
        }else if(destSize!=srcSize){
            log_err("Pre-flighting did not give expected size: Expected: %d. Got: %d \n",srcSize,destSize);
        }else{
            ec= U_ZERO_ERROR;
        }
        destSize=ubidi_writeReordered(bidi,dest,destSize+1,UBIDI_DO_MIRRORING,&ec);
        u16ToPseudo(destSize,dest,chars);
        if(destSize!=srcSize){
            log_err("ubidi_writeReordered() destSize and srcSize do not match\n");
        }else if(strcmp(visualOrder[i],chars)!=0){
            log_err("ubidi_writeReordered() did not give expected results for UBIDI_DO_MIRRORING.\n"
                    "Input   : %s\nExpected: %s\nGot     : %s\nLevels  : %s\nAt Index: %d\n",
                    logicalOrder[i],visualOrder[i],chars,formatLevels(bidi, formatChars),i);
        }
    }

    for(i=0;i<LENGTHOF(logicalOrder);i++){
        int32_t srcSize = (int32_t)strlen(logicalOrder[i]);
        int32_t destSize = srcSize*2;
        UChar src[MAXLEN];
        UChar dest[MAXLEN];
        char chars[MAXLEN];
        pseudoToU16(srcSize,logicalOrder[i],src);
        ec = U_ZERO_ERROR;
        ubidi_setPara(bidi,src,srcSize,UBIDI_DEFAULT_LTR ,NULL,&ec);
        if(U_FAILURE(ec)){
            log_err("ubidi_setPara(tests[%d], paraLevel %d) failed with errorCode %s\n",
                    i, UBIDI_DEFAULT_LTR, u_errorName(ec));
        }
        /* try pre-flighting */
        destSize = ubidi_writeReordered(bidi,dest,0,UBIDI_DO_MIRRORING+UBIDI_OUTPUT_REVERSE,&ec);
        if(ec!=U_BUFFER_OVERFLOW_ERROR){
            log_err("Pre-flighting did not give expected error: Expected: U_BUFFER_OVERFLOW_ERROR. Got: %s \n",u_errorName(ec));
        }else if(destSize!=srcSize){
            log_err("Pre-flighting did not give expected size: Expected: %d. Got: %d \n",srcSize,destSize);
        }else{
            ec= U_ZERO_ERROR;
        }
        destSize=ubidi_writeReordered(bidi,dest,destSize+1,UBIDI_DO_MIRRORING+UBIDI_OUTPUT_REVERSE,&ec);
        u16ToPseudo(destSize,dest,chars);
        if(destSize!=srcSize){
            log_err("ubidi_writeReordered() destSize and srcSize do not match\n");
        }else if(strcmp(visualOrder1[i],chars)!=0){
            log_err("ubidi_writeReordered() did not give expected results for UBIDI_DO_MIRRORING+UBIDI_OUTPUT_REVERSE.\n"
                    "Input   : %s\nExpected: %s\nGot     : %s\nLevels  : %s\nAt Index: %d\n",
                    logicalOrder[i],visualOrder1[i],chars,formatLevels(bidi, formatChars),i);
        }
    }

    for(i=0;i<LENGTHOF(logicalOrder);i++){
        int32_t srcSize = (int32_t)strlen(logicalOrder[i]);
        int32_t destSize = srcSize*2;
        UChar src[MAXLEN];
        UChar dest[MAXLEN];
        char chars[MAXLEN];
        pseudoToU16(srcSize,logicalOrder[i],src);
        ec = U_ZERO_ERROR;
        ubidi_setInverse(bidi,TRUE);
        ubidi_setPara(bidi,src,srcSize,UBIDI_DEFAULT_LTR ,NULL,&ec);
        if(U_FAILURE(ec)){
            log_err("ubidi_setPara(tests[%d], paraLevel %d) failed with errorCode %s\n",
                    i, UBIDI_DEFAULT_LTR, u_errorName(ec));
        }
                /* try pre-flighting */
        destSize = ubidi_writeReordered(bidi,dest,0,UBIDI_INSERT_LRM_FOR_NUMERIC+UBIDI_OUTPUT_REVERSE,&ec);
        if(ec!=U_BUFFER_OVERFLOW_ERROR){
            log_err("Pre-flighting did not give expected error: Expected: U_BUFFER_OVERFLOW_ERROR. Got: %s \n",u_errorName(ec));
        }else{
            ec= U_ZERO_ERROR;
        }
        destSize=ubidi_writeReordered(bidi,dest,destSize+1,UBIDI_INSERT_LRM_FOR_NUMERIC+UBIDI_OUTPUT_REVERSE,&ec);
        u16ToPseudo(destSize,dest,chars);
        if(strcmp(visualOrder2[i],chars)!=0){
            log_err("ubidi_writeReordered() did not give expected results for UBIDI_INSERT_LRM_FOR_NUMERIC+UBIDI_OUTPUT_REVERSE.\n"
                    "Input   : %s\nExpected: %s\nGot     : %s\nLevels  : %s\nAt Index: %d\n",
                    logicalOrder[i],visualOrder2[i],chars,formatLevels(bidi, formatChars),i);
        }
    }
        /* Max Explicit level */
    for(i=0;i<LENGTHOF(logicalOrder);i++){
        int32_t srcSize = (int32_t)strlen(logicalOrder[i]);
        int32_t destSize = srcSize*2;
        UChar src[MAXLEN];
        UChar dest[MAXLEN];
        char chars[MAXLEN];
        UBiDiLevel levels[UBIDI_MAX_EXPLICIT_LEVEL]={1,2,3,4,5,6,7,8,9,10};
        pseudoToU16(srcSize,logicalOrder[i],src);
        ec = U_ZERO_ERROR;
        ubidi_setPara(bidi,src,srcSize,UBIDI_DEFAULT_LTR,levels,&ec);
        if(U_FAILURE(ec)){
            log_err("ubidi_setPara(tests[%d], paraLevel %d) failed with errorCode %s\n",
                    i, UBIDI_MAX_EXPLICIT_LEVEL, u_errorName(ec));
        }
                /* try pre-flighting */
        destSize = ubidi_writeReordered(bidi,dest,0,UBIDI_OUTPUT_REVERSE,&ec);
        if(ec!=U_BUFFER_OVERFLOW_ERROR){
            log_err("Pre-flighting did not give expected error: Expected: U_BUFFER_OVERFLOW_ERROR. Got: %s \n",u_errorName(ec));
        }else if(destSize!=srcSize){
            log_err("Pre-flighting did not give expected size: Expected: %d. Got: %d \n",srcSize,destSize);
        }else{
            ec = U_ZERO_ERROR;
        }
        destSize=ubidi_writeReordered(bidi,dest,destSize+1,UBIDI_OUTPUT_REVERSE,&ec);
        u16ToPseudo(destSize,dest,chars);
        if(destSize!=srcSize){
            log_err("ubidi_writeReordered() destSize and srcSize do not match. Dest Size = %d Source Size = %d\n",destSize,srcSize );
        }else if(strcmp(visualOrder3[i],chars)!=0){
            log_err("ubidi_writeReordered() did not give expected results for UBIDI_OUTPUT_REVERSE.\n"
                    "Input   : %s\nExpected: %s\nGot     : %s\nLevels  : %s\nAt Index: %d\n",
                    logicalOrder[i],visualOrder3[i],chars,formatLevels(bidi, formatChars),i);
        }
    }
    for(i=0;i<LENGTHOF(logicalOrder);i++){
        int32_t srcSize = (int32_t)strlen(logicalOrder[i]);
        int32_t destSize = srcSize*2;
        UChar src[MAXLEN];
        UChar dest[MAXLEN];
        char chars[MAXLEN];
        UBiDiLevel levels[UBIDI_MAX_EXPLICIT_LEVEL]={1,2,3,4,5,6,7,8,9,10};
        pseudoToU16(srcSize,logicalOrder[i],src);
        ec = U_ZERO_ERROR;
        ubidi_setPara(bidi,src,srcSize,UBIDI_DEFAULT_LTR,levels,&ec);
        if(U_FAILURE(ec)){
            log_err("ubidi_setPara(tests[%d], paraLevel %d) failed with errorCode %s\n",
                    i, UBIDI_MAX_EXPLICIT_LEVEL, u_errorName(ec));
        }
        /* try pre-flighting */
        destSize = ubidi_writeReordered(bidi,dest,0,UBIDI_DO_MIRRORING+UBIDI_REMOVE_BIDI_CONTROLS,&ec);
        if(ec!=U_BUFFER_OVERFLOW_ERROR){
            log_err("Pre-flighting did not give expected error: Expected: U_BUFFER_OVERFLOW_ERROR. Got: %s \n",u_errorName(ec));
        }else{
            ec= U_ZERO_ERROR;
        }
        destSize=ubidi_writeReordered(bidi,dest,destSize+1,UBIDI_DO_MIRRORING+UBIDI_REMOVE_BIDI_CONTROLS,&ec);
        u16ToPseudo(destSize,dest,chars);
        if(strcmp(visualOrder4[i],chars)!=0){
            log_err("ubidi_writeReordered() did not give expected results for UBIDI_DO_MIRRORING+UBIDI_REMOVE_BIDI_CONTROLS.\n"
                    "Input   : %s\nExpected: %s\nGot     : %s\nLevels  : %s\nAt Index: %d\n",
                    logicalOrder[i],visualOrder4[i],chars,formatLevels(bidi, formatChars),i);
        }
    }
    ubidi_close(bidi);
}

static void
doTest(UBiDi *pBiDi, int testNumber, BiDiTestData *test, int32_t lineStart, UBool countRunsFirst) {
    const uint8_t *dirProps=test->text+lineStart;
    const UBiDiLevel *levels=test->levels;
    const uint8_t *visualMap=test->visualMap;
    int32_t i, len=ubidi_getLength(pBiDi), logicalIndex, runCount = 0;
    UErrorCode errorCode=U_ZERO_ERROR;
    UBiDiLevel level, level2;

    if (countRunsFirst) {
        log_verbose("Calling ubidi_countRuns() first.\n");

        runCount = ubidi_countRuns(pBiDi, &errorCode);

        if(U_FAILURE(errorCode)) {
            log_err("ubidi_countRuns(tests[%d]): error %s\n", testNumber, myErrorName(errorCode));
            return;
        }
    } else {
        log_verbose("Calling ubidi_getLogicalMap() first.\n");
    }

    testReordering(pBiDi, testNumber);

    for(i=0; i<len; ++i) {
        log_verbose("%3d %3d %.*s%-3s @%d\n",
                i, ubidi_getLevelAt(pBiDi, i), ubidi_getLevelAt(pBiDi, i), levelString,
                dirPropNames[dirProps[i]],
                ubidi_getVisualIndex(pBiDi, i, &errorCode));
    }

    log_verbose("\n-----levels:");
    for(i=0; i<len; ++i) {
        if(i>0) {
            log_verbose(",");
        }
        log_verbose(" %d", ubidi_getLevelAt(pBiDi, i));
    }

    log_verbose("\n--reordered:");
    for(i=0; i<len; ++i) {
        if(i>0) {
            log_verbose(",");
        }
        log_verbose(" %d", ubidi_getVisualIndex(pBiDi, i, &errorCode));
    }
    log_verbose("\n");

    if(test->direction!=ubidi_getDirection(pBiDi)) {
        log_err("ubidi_getDirection(tests[%d]): wrong direction %d\n", testNumber, ubidi_getDirection(pBiDi));
    }

    if(test->resultLevel!=ubidi_getParaLevel(pBiDi)) {
        log_err("ubidi_getParaLevel(tests[%d]): wrong paragraph level %d\n", testNumber, ubidi_getParaLevel(pBiDi));
    }

    for(i=0; i<len; ++i) {
        if(levels[i]!=ubidi_getLevelAt(pBiDi, i)) {
            log_err("ubidi_getLevelAt(tests[%d], %d): wrong level %d, expected %d\n", testNumber, i, ubidi_getLevelAt(pBiDi, i), levels[i]);
            return;
        }
    }

    for(i=0; i<len; ++i) {
        logicalIndex=ubidi_getVisualIndex(pBiDi, i, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("ubidi_getVisualIndex(tests[%d], %d): error %s\n", testNumber, i, myErrorName(errorCode));
            return;
        }
        if(visualMap[i]!=logicalIndex) {
            log_err("ubidi_getVisualIndex(tests[%d], %d): wrong index %d\n", testNumber, i, logicalIndex);
            return;
        }
    }

    if (! countRunsFirst) {
        runCount=ubidi_countRuns(pBiDi, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("ubidi_countRuns(tests[%d]): error %s\n", testNumber, myErrorName(errorCode));
            return;
        }
    }

    for(logicalIndex=0; logicalIndex<len;) {
        level=ubidi_getLevelAt(pBiDi, logicalIndex);
        ubidi_getLogicalRun(pBiDi, logicalIndex, &logicalIndex, &level2);
        if(level!=level2) {
            log_err("ubidi_getLogicalRun(tests[%d], run ending at index %d): wrong level %d\n", testNumber, logicalIndex, level2);
        }
        if(--runCount<0) {
            log_err("\nubidi_getLogicalRun(tests[%d]): wrong number of runs compared to %d=ubidi_getRunCount()\n", testNumber, ubidi_countRuns(pBiDi, &errorCode));
            return;
        }
    }
    if(runCount!=0) {
        log_err("\nubidi_getLogicalRun(tests[%d]): wrong number of runs compared to %d=ubidi_getRunCount()\n", testNumber, ubidi_countRuns(pBiDi, &errorCode));
        return;
    }

    log_verbose("\n\n");
}

static void
testReordering(UBiDi *pBiDi, int testNumber) {
    int32_t
        logicalMap1[MAXLEN], logicalMap2[MAXLEN], logicalMap3[MAXLEN],
        visualMap1[MAXLEN], visualMap2[MAXLEN], visualMap3[MAXLEN], visualMap4[MAXLEN];
    UErrorCode errorCode=U_ZERO_ERROR;
    const UBiDiLevel *levels;
    int32_t i, length=ubidi_getLength(pBiDi),
               destLength=ubidi_getResultLength(pBiDi);
    int32_t runCount, visualIndex, logicalStart, runLength;
    UBool odd;

    if(length<=0) {
        return;
    }

    /* get the logical and visual maps from the object */
    ubidi_getLogicalMap(pBiDi, logicalMap1, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ubidi_getLogicalMap(tests[%d]): error %s\n", testNumber, myErrorName(errorCode));
        return;
    }

    ubidi_getVisualMap(pBiDi, visualMap1, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ubidi_getVisualMap(tests[%d]): error %s\n", testNumber, myErrorName(errorCode));
        return;
    }

    /* invert them both */
    ubidi_invertMap(logicalMap1, visualMap2, length);
    ubidi_invertMap(visualMap1, logicalMap2, destLength);

    /* get them from the levels array, too */
    levels=ubidi_getLevels(pBiDi, &errorCode);

    if(U_FAILURE(errorCode)) {
        log_err("ubidi_getLevels(tests[%d]): error %s\n", testNumber, myErrorName(errorCode));
        return;
    }

    ubidi_reorderLogical(levels, length, logicalMap3);
    ubidi_reorderVisual(levels, length, visualMap3);

    /* get the visual map from the runs, too */
    runCount=ubidi_countRuns(pBiDi, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ubidi_countRuns(tests[%d]): error %s\n", testNumber, myErrorName(errorCode));
        return;
    }
    log_verbose("\n----%2d runs:", runCount);
    visualIndex=0;
    for(i=0; i<runCount; ++i) {
        odd=(UBool)ubidi_getVisualRun(pBiDi, i, &logicalStart, &runLength);
        log_verbose(" (%c @%d[%d])", odd ? 'R' : 'L', logicalStart, runLength);
        if(UBIDI_LTR==odd) {
            do { /* LTR */
                visualMap4[visualIndex++]=logicalStart++;
            } while(--runLength>0);
        } else {
            logicalStart+=runLength;   /* logicalLimit */
            do { /* RTL */
                visualMap4[visualIndex++]=--logicalStart;
            } while(--runLength>0);
        }
    }
    log_verbose("\n");

    /* print all the maps */
    log_verbose("logical maps:\n");
    for(i=0; i<length; ++i) {
        log_verbose("%4d", logicalMap1[i]);
    }
    log_verbose("\n");
    for(i=0; i<length; ++i) {
        log_verbose("%4d", logicalMap2[i]);
    }
    log_verbose("\n");
    for(i=0; i<length; ++i) {
        log_verbose("%4d", logicalMap3[i]);
    }

    log_verbose("\nvisual maps:\n");
    for(i=0; i<destLength; ++i) {
        log_verbose("%4d", visualMap1[i]);
    }
    log_verbose("\n");
    for(i=0; i<destLength; ++i) {
        log_verbose("%4d", visualMap2[i]);
    }
    log_verbose("\n");
    for(i=0; i<length; ++i) {
        log_verbose("%4d", visualMap3[i]);
    }
    log_verbose("\n");
    for(i=0; i<length; ++i) {
        log_verbose("%4d", visualMap4[i]);
    }
    log_verbose("\n");

    /* check that the indexes are the same between these and ubidi_getLogical/VisualIndex() */
    for(i=0; i<length; ++i) {
        if(logicalMap1[i]!=logicalMap2[i]) {
            log_err("bidi reordering error in tests[%d]: logicalMap1[i]!=logicalMap2[i] at i=%d\n", testNumber, i);
            break;
        }
        if(logicalMap1[i]!=logicalMap3[i]) {
            log_err("bidi reordering error in tests[%d]: logicalMap1[i]!=logicalMap3[i] at i=%d\n", testNumber, i);
            break;
        }

        if(visualMap1[i]!=visualMap2[i]) {
            log_err("bidi reordering error in tests[%d]: visualMap1[i]!=visualMap2[i] at i=%d\n", testNumber, i);
            break;
        }
        if(visualMap1[i]!=visualMap3[i]) {
            log_err("bidi reordering error in tests[%d]: visualMap1[i]!=visualMap3[i] at i=%d\n", testNumber, i);
            break;
        }
        if(visualMap1[i]!=visualMap4[i]) {
            log_err("bidi reordering error in tests[%d]: visualMap1[i]!=visualMap4[i] at i=%d\n", testNumber, i);
            break;
        }

        if(logicalMap1[i]!=ubidi_getVisualIndex(pBiDi, i, &errorCode)) {
            log_err("bidi reordering error in tests[%d]: logicalMap1[i]!=ubidi_getVisualIndex(i) at i=%d\n", testNumber, i);
            break;
        }
        if(U_FAILURE(errorCode)) {
            log_err("ubidi_getVisualIndex(tests[%d], %d): error %s\n", testNumber, i, myErrorName(errorCode));
            break;
        }
        if(visualMap1[i]!=ubidi_getLogicalIndex(pBiDi, i, &errorCode)) {
            log_err("bidi reordering error in tests[%d]: visualMap1[i]!=ubidi_getLogicalIndex(i) at i=%d\n", testNumber, i);
            break;
        }
        if(U_FAILURE(errorCode)) {
            log_err("ubidi_getLogicalIndex(tests[%d], %d): error %s\n", testNumber, i, myErrorName(errorCode));
            break;
        }
    }
}

static void TestFailureRecovery(void) {
    UErrorCode status;

    status = U_FILE_ACCESS_ERROR;
    if (ubidi_writeReordered(NULL, NULL, 0, 0, &status) != 0) {
        log_err("ubidi_writeReordered did not return 0 when passed a failing UErrorCode\n");
    }
    if (ubidi_writeReverse(NULL, 0, NULL, 0, 0, &status) != 0) {
        log_err("ubidi_writeReverse did not return 0 when passed a failing UErrorCode\n");
    }
    status = U_ZERO_ERROR;
    if (ubidi_writeReordered(NULL, NULL, 0, 0, &status) != 0 || status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("ubidi_writeReordered did not fail as expected\n");
    }
    status = U_ZERO_ERROR;
    if (ubidi_writeReverse(NULL, 0, NULL, 0, 0, &status) != 0 || status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("ubidi_writeReverse did not fail as expected\n");
    }
}

static void TestMultipleParagraphs(void) {
    static const char* const text = "__ABC\\u001c"          /* Para #0 offset 0 */
                                    "__\\u05d0DE\\u001c"    /*       1        6 */
                                    "__123\\u001c"          /*       2       12 */
                                    "\\u000d\\u000a"        /*       3       18 */
                                    "FG\\u000d"             /*       4       20 */
                                    "\\u000d"               /*       5       23 */
                                    "HI\\u000d\\u000a"      /*       6       24 */
                                    "\\u000d\\u000a"        /*       7       28 */
                                    "\\u000a"               /*       8       30 */
                                    "\\u000a"               /*       9       31 */
                                    "JK\\u001c";            /*      10       32 */
    static const int32_t paraCount=11;
    static const int32_t paraBounds[]={0, 6, 12, 18, 20, 23, 24, 28, 30, 31, 32, 35};
    static const UBiDiLevel paraLevels[]={UBIDI_LTR, UBIDI_RTL, UBIDI_DEFAULT_LTR, UBIDI_DEFAULT_RTL, 22, 23};
    static const UBiDiLevel multiLevels[6][11] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                                                  {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0},
                                                  {22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22},
                                                  {23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23}};
    static const char* const text2 = "\\u05d0 1-2\\u001c\\u0630 1-2\\u001c1-2";
    static const UBiDiLevel levels2[] = {1,1,2,2,2,0, 1,1,2,1,2,0, 2,2,2};
    UBiDiLevel gotLevel;
    const UBiDiLevel* gotLevels;
    UBool orderParagraphsLTR;
    UChar src[MAXLEN];
    UErrorCode errorCode=U_ZERO_ERROR;
    UBiDi* pBidi=ubidi_open();
    UBiDi* pLine;
    int32_t srcSize, count, paraStart, paraLimit, paraIndex, length;
    int i, j, k;

    u_unescape(text, src, MAXLEN);
    srcSize=u_strlen(src);
    ubidi_setPara(pBidi, src, srcSize, UBIDI_LTR, NULL, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ubidi_setPara failed, paraLevel=%d, errorCode %s\n",
                UBIDI_LTR, u_errorName(errorCode));
        ubidi_close(pBidi);
        return;
    }
    /* check paragraph count and boundaries */
    if (paraCount!=(count=ubidi_countParagraphs(pBidi))) {
        log_err("ubidi_countParagraphs returned %d, should be %d\n",
                count, paraCount);
    }
    for (i=0; i<paraCount; i++) {
        ubidi_getParagraphByIndex(pBidi, i, &paraStart, &paraLimit, NULL, &errorCode);
        if ((paraStart!=paraBounds[i]) || (paraLimit!=paraBounds[i+1])) {
            log_err("Found boundaries of paragraph %d: %d-%d; expected: %d-%d\n",
                    i, paraStart, paraLimit, paraBounds[i], paraBounds[i+1]);
        }
    }
    errorCode=U_ZERO_ERROR;
    /* check with last paragraph not terminated by B */
    src[srcSize-1]='L';
    ubidi_setPara(pBidi, src, srcSize, UBIDI_LTR, NULL, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("2nd ubidi_setPara failed, paraLevel=%d, errorCode %s\n",
                UBIDI_LTR, u_errorName(errorCode));
        ubidi_close(pBidi);
        return;
    }
    if (paraCount!=(count=ubidi_countParagraphs(pBidi))) {
        log_err("2nd ubidi_countParagraphs returned %d, should be %d\n",
                count, paraCount);
    }
    i=paraCount-1;
    ubidi_getParagraphByIndex(pBidi, i, &paraStart, &paraLimit, NULL, &errorCode);
    if ((paraStart!=paraBounds[i]) || (paraLimit!=paraBounds[i+1])) {
        log_err("2nd Found boundaries of paragraph %d: %d-%d; expected: %d-%d\n",
                i, paraStart, paraLimit, paraBounds[i], paraBounds[i+1]);
    }
    errorCode=U_ZERO_ERROR;
    /* check paraLevel for all paragraphs under various paraLevel specs */
    for (k=0; k<6; k++) {
        ubidi_setPara(pBidi, src, srcSize, paraLevels[k], NULL, &errorCode);
        for (i=0; i<paraCount; i++) {
            paraIndex=ubidi_getParagraph(pBidi, paraBounds[i], NULL, NULL, &gotLevel, &errorCode);
            if (paraIndex!=i) {
                log_err("For paraLevel=%d paragraph=%d, found paragraph index=%d expected=%d\n",
                        paraLevels[k], i, paraIndex, i);
            }
            if (gotLevel!=multiLevels[k][i]) {
                log_err("For paraLevel=%d paragraph=%d, found level=%d expected %d\n",
                        paraLevels[k], i, gotLevel, multiLevels[k][i]);
            }
        }
        gotLevel=ubidi_getParaLevel(pBidi);
        if (gotLevel!=multiLevels[k][0]) {
            log_err("For paraLevel=%d getParaLevel=%d, expected %d\n",
                    paraLevels[k], gotLevel, multiLevels[k][0]);
        }
    }
    errorCode=U_ZERO_ERROR;
    /* check that the result of ubidi_getParaLevel changes if the first
     * paragraph has a different level
     */
    src[0]=0x05d2;                      /* Hebrew letter Gimel */
    ubidi_setPara(pBidi, src, srcSize, UBIDI_DEFAULT_LTR, NULL, &errorCode);
    gotLevel=ubidi_getParaLevel(pBidi);
    if (gotLevel!=UBIDI_RTL) {
        log_err("For paraLevel=UBIDI_DEFAULT_LTR getParaLevel=%d, expected=%d\n",
                        gotLevel, UBIDI_RTL);
    }
    errorCode=U_ZERO_ERROR;
    /* check that line cannot overlap paragraph boundaries */
    pLine=ubidi_open();
    i=paraBounds[1];
    k=paraBounds[2]+1;
    ubidi_setLine(pBidi, i, k, pLine, &errorCode);
    if (U_SUCCESS(errorCode)) {
        log_err("For line limits %d-%d got success %s\n",
                i, k, u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    i=paraBounds[1];
    k=paraBounds[2];
    ubidi_setLine(pBidi, i, k, pLine, &errorCode);
    if (U_FAILURE(errorCode)) {
        log_err("For line limits %d-%d got error %s\n",
                i, k, u_errorName(errorCode));
        errorCode=U_ZERO_ERROR;
    }
    /* check level of block separator at end of paragraph when orderParagraphsLTR==FALSE */
    ubidi_setPara(pBidi, src, srcSize, UBIDI_RTL, NULL, &errorCode);
    /* get levels through para Bidi block */
    gotLevels=ubidi_getLevels(pBidi, &errorCode);
    if (U_FAILURE(errorCode)) {
        log_err("Error on Para getLevels %s\n", u_errorName(errorCode));
        ubidi_close(pLine);
        ubidi_close(pBidi);
        return;
    }
    for (i=26; i<32; i++) {
        if (gotLevels[i]!=UBIDI_RTL) {
            log_err("For char %d(%04x), level=%d, expected=%d\n",
                    i, src[i], gotLevels[i], UBIDI_RTL);
        }
    }
    /* get levels through para Line block */
    i=paraBounds[1];
    k=paraBounds[2];
    ubidi_setLine(pBidi, i, k, pLine, &errorCode);
    if (U_FAILURE(errorCode)) {
        log_err("For line limits %d-%d got error %s\n",
                i, k, u_errorName(errorCode));
        ubidi_close(pLine);
        ubidi_close(pBidi);
        return;
    }
    paraIndex=ubidi_getParagraph(pLine, i, &paraStart, &paraLimit, &gotLevel, &errorCode);
    gotLevels=ubidi_getLevels(pLine, &errorCode);
    if (U_FAILURE(errorCode)) {
        log_err("Error on Line getLevels %s\n", u_errorName(errorCode));
        ubidi_close(pLine);
        ubidi_close(pBidi);
        return;
    }
    length=ubidi_getLength(pLine);
    if ((gotLevel!=UBIDI_RTL) || (gotLevels[length-1]!=UBIDI_RTL)) {
        log_err("For paragraph %d with limits %d-%d, paraLevel=%d expected=%d, "
                "level of separator=%d expected=%d\n",
                paraIndex, paraStart, paraLimit, gotLevel, UBIDI_RTL, gotLevels[length-1], UBIDI_RTL);
    }
    orderParagraphsLTR=ubidi_isOrderParagraphsLTR(pBidi);
    if (orderParagraphsLTR) {
        log_err("Found orderParagraphsLTR=%d expected=%d\n", orderParagraphsLTR, FALSE);
    }
    ubidi_orderParagraphsLTR(pBidi, TRUE);
    orderParagraphsLTR=ubidi_isOrderParagraphsLTR(pBidi);
    if (!orderParagraphsLTR) {
        log_err("Found orderParagraphsLTR=%d expected=%d\n", orderParagraphsLTR, TRUE);
    }
    /* check level of block separator at end of paragraph when orderParagraphsLTR==TRUE */
    ubidi_setPara(pBidi, src, srcSize, UBIDI_RTL, NULL, &errorCode);
    /* get levels through para Bidi block */
    gotLevels=ubidi_getLevels(pBidi, &errorCode);
    for (i=26; i<32; i++) {
        if (gotLevels[i]!=0) {
            log_err("For char %d(%04x), level=%d, expected=%d\n",
                    i, src[i], gotLevels[i], 0);
        }
    }
    errorCode=U_ZERO_ERROR;
    /* get levels through para Line block */
    i=paraBounds[1];
    k=paraBounds[2];
    ubidi_setLine(pBidi, paraStart, paraLimit, pLine, &errorCode);
    paraIndex=ubidi_getParagraph(pLine, i, &paraStart, &paraLimit, &gotLevel, &errorCode);
    gotLevels=ubidi_getLevels(pLine, &errorCode);
    length=ubidi_getLength(pLine);
    if ((gotLevel!=UBIDI_RTL) || (gotLevels[length-1]!=0)) {
        log_err("For paragraph %d with limits %d-%d, paraLevel=%d expected=%d, "
                "level of separator=%d expected=%d\n",
                paraIndex, paraStart, paraLimit, gotLevel, UBIDI_RTL, gotLevels[length-1], 0);
        log_verbose("levels=");
        for (count=0; count<length; count++) {
            log_verbose(" %d", gotLevels[count]);
        }
        log_verbose("\n");
    }

    /* test that the concatenation of separate invocations of the bidi code
     * on each individual paragraph in order matches the levels array that
     * results from invoking bidi once over the entire multiparagraph tests
     * (with orderParagraphsLTR false, of course)
     */
    u_unescape(text, src, MAXLEN);      /* restore original content */
    srcSize=u_strlen(src);
    ubidi_orderParagraphsLTR(pBidi, FALSE);
    ubidi_setPara(pBidi, src, srcSize, UBIDI_DEFAULT_RTL, NULL, &errorCode);
    gotLevels=ubidi_getLevels(pBidi, &errorCode);
    for (i=0; i<paraCount; i++) {
        /* use pLine for individual paragraphs */
        paraStart = paraBounds[i];
        length = paraBounds[i+1] - paraStart;
        ubidi_setPara(pLine, src+paraStart, length, UBIDI_DEFAULT_RTL, NULL, &errorCode);
        for (j=0; j<length; j++) {
            if ((k=ubidi_getLevelAt(pLine, j)) != (gotLevel=gotLevels[paraStart+j])) {
                log_err("Checking paragraph concatenation: for paragraph=%d, "
                        "char=%d(%04x), level=%d, expected=%d\n",
                        i, j, src[paraStart+j], k, gotLevel);
            }
        }
    }

    /* ensure that leading numerics in a paragraph are not treated as arabic
       numerals because of arabic text in a preceding paragraph
     */
    u_unescape(text2, src, MAXLEN);
    srcSize=u_strlen(src);
    ubidi_orderParagraphsLTR(pBidi, TRUE);
    ubidi_setPara(pBidi, src, srcSize, UBIDI_RTL, NULL, &errorCode);
    gotLevels=ubidi_getLevels(pBidi, &errorCode);
    for (i=0; i<srcSize; i++) {
        if (gotLevels[i]!=levels2[i]) {
            log_err("Checking leading numerics: for char %d(%04x), level=%d, expected=%d\n",
                    i, src[i], gotLevels[i], levels2[i]);
        }
    }

    /* check handling of whitespace before end of paragraph separator when
     * orderParagraphsLTR==TRUE, when last paragraph has, and lacks, a terminating B
     */
    u_memset(src, 0x0020, MAXLEN);
    srcSize = 5;
    ubidi_orderParagraphsLTR(pBidi, TRUE);
    for (i=0x001c; i<=0x0020; i+=(0x0020-0x001c)) {
        src[4]=i;                       /* with and without terminating B */
        for (j=0x0041; j<=0x05d0; j+=(0x05d0-0x0041)) {
            src[0]=j;                   /* leading 'A' or Alef */
            for (gotLevel=4; gotLevel<=5; gotLevel++) {
                /* test even and odd paraLevel */
                ubidi_setPara(pBidi, src, srcSize, gotLevel, NULL, &errorCode);
                gotLevels=ubidi_getLevels(pBidi, &errorCode);
                for (k=1; k<=3; k++) {
                    if (gotLevels[k]!=gotLevel) {
                        log_err("Checking trailing spaces: for leading_char=%04x, "
                                "last_char=%04x, index=%d, level=%d, expected=%d\n",
                                src[0], src[4], k, gotLevels[k], gotLevel);
                    }
                }
            }
        }
    }

    ubidi_close(pLine);
    ubidi_close(pBidi);
}


/* inverse BiDi ------------------------------------------------------------- */

static const UChar
    string0[]={ 0x6c, 0x61, 0x28, 0x74, 0x69, 0x6e, 0x20, 0x5d0, 0x5d1, 0x29, 0x5d2, 0x5d3 },
    string1[]={ 0x6c, 0x61, 0x74, 0x20, 0x5d0, 0x5d1, 0x5d2, 0x20, 0x31, 0x32, 0x33 },
    string2[]={ 0x6c, 0x61, 0x74, 0x20, 0x5d0, 0x28, 0x5d1, 0x5d2, 0x20, 0x31, 0x29, 0x32, 0x33 },
    string3[]={ 0x31, 0x32, 0x33, 0x20, 0x5d0, 0x5d1, 0x5d2, 0x20, 0x34, 0x35, 0x36 },
    string4[]={ 0x61, 0x62, 0x20, 0x61, 0x62, 0x20, 0x661, 0x662 };

#define STRING_TEST_CASE(s) { (s), LENGTHOF(s) }

static const struct {
    const UChar *s;
    int32_t length;
} testCases[]={
    STRING_TEST_CASE(string0),
    STRING_TEST_CASE(string1),
    STRING_TEST_CASE(string2),
    STRING_TEST_CASE(string3)
};

static int countRoundtrips=0, countNonRoundtrips=0;

static void
doInverseBiDiTest() {
    UBiDi *pBiDi;
    UErrorCode errorCode;
    int i;

    pBiDi=ubidi_open();
    if(pBiDi==NULL) {
        log_err("unable to open a UBiDi object (out of memory)\n");
        return;
    }

    log_verbose("inverse BiDi: testInverseBiDi(L) with %u test cases ---\n", LENGTHOF(testCases));
     for(i=0; i<LENGTHOF(testCases); ++i) {
        errorCode=U_ZERO_ERROR;
        testInverseBiDi(pBiDi, testCases[i].s, testCases[i].length, 0, &errorCode);
    }

    log_verbose("inverse BiDi: testInverseBiDi(R) with %u test cases ---\n", LENGTHOF(testCases));
    for(i=0; i<LENGTHOF(testCases); ++i) {
        errorCode=U_ZERO_ERROR;
        testInverseBiDi(pBiDi, testCases[i].s, testCases[i].length, 1, &errorCode);
    }

    testManyInverseBiDi(pBiDi, 0);
    testManyInverseBiDi(pBiDi, 1);

    ubidi_close(pBiDi);

    log_verbose("inverse BiDi: rountrips: %5u\nnon-roundtrips: %5u\n", countRoundtrips, countNonRoundtrips);

    testWriteReverse();
}

#define COUNT_REPEAT_SEGMENTS 6

static const UChar repeatSegments[COUNT_REPEAT_SEGMENTS][2]={
    { 0x61, 0x62 },     /* L */
    { 0x5d0, 0x5d1 },   /* R */
    { 0x627, 0x628 },   /* AL */
    { 0x31, 0x32 },     /* EN */
    { 0x661, 0x662 },   /* AN */
    { 0x20, 0x20 }      /* WS (N) */
};

static void
testManyInverseBiDi(UBiDi *pBiDi, UBiDiLevel direction) {
    UChar text[8]={ 0, 0, 0x20, 0, 0, 0x20, 0, 0 };
    int i, j, k;
    UErrorCode errorCode;

    log_verbose("inverse BiDi: testManyInverseBiDi(%c) - test permutations of text snippets ---\n", direction==0 ? 'L' : 'R');
    for(i=0; i<COUNT_REPEAT_SEGMENTS; ++i) {
        text[0]=repeatSegments[i][0];
        text[1]=repeatSegments[i][1];
        for(j=0; j<COUNT_REPEAT_SEGMENTS; ++j) {
            text[3]=repeatSegments[j][0];
            text[4]=repeatSegments[j][1];
            for(k=0; k<COUNT_REPEAT_SEGMENTS; ++k) {
                text[6]=repeatSegments[k][0];
                text[7]=repeatSegments[k][1];

                errorCode=U_ZERO_ERROR;
                log_verbose("inverse BiDi: testManyInverseBiDi()[%u %u %u]\n", i, j, k);
                testInverseBiDi(pBiDi, text, 8, direction, &errorCode);
            }
        }
    }
}

static void
testInverseBiDi(UBiDi *pBiDi, const UChar *src, int32_t srcLength,
                UBiDiLevel direction, UErrorCode *pErrorCode) {
    UChar visualLTR[MAXLEN], logicalDest[MAXLEN], visualDest[MAXLEN];
    int32_t ltrLength, logicalLength, visualLength;

    if(direction==0) {
        log_verbose("inverse BiDi: testInverseBiDi(L)\n");

        /* convert visual to logical */
        ubidi_setInverse(pBiDi, TRUE);
        ubidi_setPara(pBiDi, src, srcLength, 0, NULL, pErrorCode);
        logicalLength=ubidi_writeReordered(pBiDi, logicalDest, LENGTHOF(logicalDest),
                                           UBIDI_DO_MIRRORING|UBIDI_INSERT_LRM_FOR_NUMERIC, pErrorCode);
        log_verbose("  v ");
        printUnicode(src, srcLength, ubidi_getLevels(pBiDi, pErrorCode));
        log_verbose("\n");

        /* convert back to visual LTR */
        ubidi_setInverse(pBiDi, FALSE);
        ubidi_setPara(pBiDi, logicalDest, logicalLength, 0, NULL, pErrorCode);
        visualLength=ubidi_writeReordered(pBiDi, visualDest, LENGTHOF(visualDest),
                                          UBIDI_DO_MIRRORING|UBIDI_REMOVE_BIDI_CONTROLS, pErrorCode);
    } else {
        log_verbose("inverse BiDi: testInverseBiDi(R)\n");

        /* reverse visual from RTL to LTR */
        ltrLength=ubidi_writeReverse(src, srcLength, visualLTR, LENGTHOF(visualLTR), 0, pErrorCode);
        log_verbose("  vr");
        printUnicode(src, srcLength, NULL);
        log_verbose("\n");

        /* convert visual RTL to logical */
        ubidi_setInverse(pBiDi, TRUE);
        ubidi_setPara(pBiDi, visualLTR, ltrLength, 0, NULL, pErrorCode);
        logicalLength=ubidi_writeReordered(pBiDi, logicalDest, LENGTHOF(logicalDest),
                                           UBIDI_DO_MIRRORING|UBIDI_INSERT_LRM_FOR_NUMERIC, pErrorCode);
        log_verbose("  vl");
        printUnicode(visualLTR, ltrLength, ubidi_getLevels(pBiDi, pErrorCode));
        log_verbose("\n");

        /* convert back to visual RTL */
        ubidi_setInverse(pBiDi, FALSE);
        ubidi_setPara(pBiDi, logicalDest, logicalLength, 0, NULL, pErrorCode);
        visualLength=ubidi_writeReordered(pBiDi, visualDest, LENGTHOF(visualDest),
                                          UBIDI_DO_MIRRORING|UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_OUTPUT_REVERSE, pErrorCode);
    }
    log_verbose("  l ");
    printUnicode(logicalDest, logicalLength, ubidi_getLevels(pBiDi, pErrorCode));
    log_verbose("\n");
    log_verbose("  v ");
    printUnicode(visualDest, visualLength, NULL);
    log_verbose("\n");

    /* check and print results */
    if(U_FAILURE(*pErrorCode)) {
        log_err("inverse BiDi: *** error %s\n"
                "                 turn on verbose mode to see details\n", u_errorName(*pErrorCode));
    } else if(srcLength==visualLength && uprv_memcmp(src, visualDest, srcLength*U_SIZEOF_UCHAR)==0) {
        ++countRoundtrips;
        log_verbose(" + roundtripped\n");
    } else {
        ++countNonRoundtrips;
        log_verbose(" * did not roundtrip\n");
        log_err("inverse BiDi: transformation visual->logical->visual did not roundtrip the text;\n"
                "                 turn on verbose mode to see details\n");
    }
}

static void
testWriteReverse() {
    /* U+064e and U+0650 are combining marks (Mn) */
    static const UChar forward[]={
        0x200f, 0x627, 0x64e, 0x650, 0x20, 0x28, 0x31, 0x29
    }, reverseKeepCombining[]={
        0x29, 0x31, 0x28, 0x20, 0x627, 0x64e, 0x650, 0x200f
    }, reverseRemoveControlsKeepCombiningDoMirror[]={
        0x28, 0x31, 0x29, 0x20, 0x627, 0x64e, 0x650
    };
    UChar reverse[10];
    UErrorCode errorCode;
    int32_t length;

    /* test ubidi_writeReverse() with "interesting" options */
    errorCode=U_ZERO_ERROR;
    length=ubidi_writeReverse(forward, LENGTHOF(forward),
                              reverse, LENGTHOF(reverse),
                              UBIDI_KEEP_BASE_COMBINING,
                              &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(reverseKeepCombining) || uprv_memcmp(reverse, reverseKeepCombining, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in ubidi_writeReverse(UBIDI_KEEP_BASE_COMBINING): length=%d (should be %d), error code %s\n",
                length, LENGTHOF(reverseKeepCombining), u_errorName(errorCode));
    }

    uprv_memset(reverse, 0xa5, LENGTHOF(reverse)*U_SIZEOF_UCHAR);
    errorCode=U_ZERO_ERROR;
    length=ubidi_writeReverse(forward, LENGTHOF(forward),
                              reverse, LENGTHOF(reverse),
                              UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_DO_MIRRORING|UBIDI_KEEP_BASE_COMBINING,
                              &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(reverseRemoveControlsKeepCombiningDoMirror) || uprv_memcmp(reverse, reverseRemoveControlsKeepCombiningDoMirror, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in ubidi_writeReverse(UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_DO_MIRRORING|UBIDI_KEEP_BASE_COMBINING):\n"
                "    length=%d (should be %d), error code %s\n",
                length, LENGTHOF(reverseRemoveControlsKeepCombiningDoMirror), u_errorName(errorCode));
    }
}

/* arabic shaping ----------------------------------------------------------- */

static void
doArabicShapingTest() {
    static const UChar
    source[]={
        0x31,   /* en:1 */
        0x627,  /* arabic:alef */
        0x32,   /* en:2 */
        0x6f3,  /* an:3 */
        0x61,   /* latin:a */
        0x34,   /* en:4 */
        0
    }, en2an[]={
        0x661, 0x627, 0x662, 0x6f3, 0x61, 0x664, 0
    }, an2en[]={
        0x31, 0x627, 0x32, 0x33, 0x61, 0x34, 0
    }, logical_alen2an_init_lr[]={
        0x31, 0x627, 0x662, 0x6f3, 0x61, 0x34, 0
    }, logical_alen2an_init_al[]={
        0x6f1, 0x627, 0x6f2, 0x6f3, 0x61, 0x34, 0
    }, reverse_alen2an_init_lr[]={
        0x661, 0x627, 0x32, 0x6f3, 0x61, 0x34, 0
    }, reverse_alen2an_init_al[]={
        0x6f1, 0x627, 0x32, 0x6f3, 0x61, 0x6f4, 0
    };
    UChar dest[8];
    UErrorCode errorCode;
    int32_t length;

    /* test number shaping */

    /* european->arabic */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(source) || uprv_memcmp(dest, en2an, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(en2an)\n");
    }

    /* arabic->european */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, -1,
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_AN2EN|U_SHAPE_DIGIT_TYPE_AN_EXTENDED,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=u_strlen(source) || uprv_memcmp(dest, an2en, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(an2en)\n");
    }

    /* european->arabic with context, logical order, initial state not AL */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_ALEN2AN_INIT_LR|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(source) || uprv_memcmp(dest, logical_alen2an_init_lr, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(logical_alen2an_init_lr)\n");
    }

    /* european->arabic with context, logical order, initial state AL */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_ALEN2AN_INIT_AL|U_SHAPE_DIGIT_TYPE_AN_EXTENDED,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(source) || uprv_memcmp(dest, logical_alen2an_init_al, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(logical_alen2an_init_al)\n");
    }

    /* european->arabic with context, reverse order, initial state not AL */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_ALEN2AN_INIT_LR|U_SHAPE_DIGIT_TYPE_AN|U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(source) || uprv_memcmp(dest, reverse_alen2an_init_lr, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(reverse_alen2an_init_lr)\n");
    }

    /* european->arabic with context, reverse order, initial state AL */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_ALEN2AN_INIT_AL|U_SHAPE_DIGIT_TYPE_AN_EXTENDED|U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(source) || uprv_memcmp(dest, reverse_alen2an_init_al, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(reverse_alen2an_init_al)\n");
    }

    /* test noop */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         0,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=LENGTHOF(source) || uprv_memcmp(dest, source, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(noop)\n");
    }

    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, 0,
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(U_FAILURE(errorCode) || length!=0) {
        log_err("failure in u_shapeArabic(en2an, sourceLength=0), returned %d/%s\n", u_errorName(errorCode), LENGTHOF(source));
    }

    /* preflight digit shaping */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         NULL, 0,
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length!=LENGTHOF(source)) {
        log_err("failure in u_shapeArabic(en2an preflighting), returned %d/%s instead of %d/U_BUFFER_OVERFLOW_ERROR\n",
                length, u_errorName(errorCode), LENGTHOF(source));
    }

    /* test illegal arguments */
    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(NULL, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("failure in u_shapeArabic(source=NULL), returned %s instead of U_ILLEGAL_ARGUMENT_ERROR\n", u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, -2,
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("failure in u_shapeArabic(sourceLength=-2), returned %s instead of U_ILLEGAL_ARGUMENT_ERROR\n", u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         NULL, LENGTHOF(dest),
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("failure in u_shapeArabic(dest=NULL), returned %s instead of U_ILLEGAL_ARGUMENT_ERROR\n", u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, -1,
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("failure in u_shapeArabic(destSize=-1), returned %s instead of U_ILLEGAL_ARGUMENT_ERROR\n", u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_RESERVED|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("failure in u_shapeArabic(U_SHAPE_DIGITS_RESERVED), returned %s instead of U_ILLEGAL_ARGUMENT_ERROR\n", u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_RESERVED,
                         &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("failure in u_shapeArabic(U_SHAPE_DIGIT_TYPE_RESERVED), returned %s instead of U_ILLEGAL_ARGUMENT_ERROR\n", u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    length=u_shapeArabic(source, LENGTHOF(source),
                         (UChar *)(source+2), LENGTHOF(dest), /* overlap source and destination */
                         U_SHAPE_DIGITS_EN2AN|U_SHAPE_DIGIT_TYPE_AN,
                         &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("failure in u_shapeArabic(U_SHAPE_DIGIT_TYPE_RESERVED), returned %s instead of U_ILLEGAL_ARGUMENT_ERROR\n", u_errorName(errorCode));
    }
}

static void
doLamAlefSpecialVLTRArabicShapingTest() {
    static const UChar
    source[]={
/*a*/   0x20 ,0x646,0x622,0x644,0x627,0x20,
/*b*/   0x646,0x623,0x64E,0x644,0x627,0x20,
/*c*/   0x646,0x627,0x670,0x644,0x627,0x20,
/*d*/   0x646,0x622,0x653,0x644,0x627,0x20,
/*e*/   0x646,0x625,0x655,0x644,0x627,0x20,
/*f*/   0x646,0x622,0x654,0x644,0x627,0x20,
/*g*/   0xFEFC,0x639
    }, shape_near[]={
        0x20,0xfee5,0x20,0xfef5,0xfe8d,0x20,0xfee5,0x20,0xfe76,0xfef7,0xfe8d,0x20,
        0xfee5,0x20,0x670,0xfefb,0xfe8d,0x20,0xfee5,0x20,0x653,0xfef5,0xfe8d,0x20,
        0xfee5,0x20,0x655,0xfef9,0xfe8d,0x20,0xfee5,0x20,0x654,0xfef5,0xfe8d,0x20,
        0xfefc,0xfecb
    }, shape_at_end[]={
        0x20,0xfee5,0xfef5,0xfe8d,0x20,0xfee5,0xfe76,0xfef7,0xfe8d,0x20,0xfee5,0x670,
        0xfefb,0xfe8d,0x20,0xfee5,0x653,0xfef5,0xfe8d,0x20,0xfee5,0x655,0xfef9,0xfe8d,
        0x20,0xfee5,0x654,0xfef5,0xfe8d,0x20,0xfefc,0xfecb,0x20,0x20,0x20,0x20,0x20,0x20
    }, shape_at_begin[]={
        0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xfee5,0xfef5,0xfe8d,0x20,0xfee5,0xfe76,
        0xfef7,0xfe8d,0x20,0xfee5,0x670,0xfefb,0xfe8d,0x20,0xfee5,0x653,0xfef5,0xfe8d,
        0x20,0xfee5,0x655,0xfef9,0xfe8d,0x20,0xfee5,0x654,0xfef5,0xfe8d,0x20,0xfefc,0xfecb
    }, shape_grow_shrink[]={
        0x20,0xfee5,0xfef5,0xfe8d,0x20,0xfee5,0xfe76,0xfef7,0xfe8d,0x20,0xfee5,
        0x670,0xfefb,0xfe8d,0x20,0xfee5,0x653,0xfef5,0xfe8d,0x20,0xfee5,0x655,0xfef9,
        0xfe8d,0x20,0xfee5,0x654,0xfef5,0xfe8d,0x20,0xfefc,0xfecb
    }, shape_excepttashkeel_near[]={
        0x20,0xfee5,0x20,0xfef5,0xfe8d,0x20,0xfee5,0x20,0xfe76,0xfef7,0xfe8d,0x20,
        0xfee5,0x20,0x670,0xfefb,0xfe8d,0x20,0xfee5,0x20,0x653,0xfef5,0xfe8d,0x20,
        0xfee5,0x20,0x655,0xfef9,0xfe8d,0x20,0xfee5,0x20,0x654,0xfef5,0xfe8d,0x20,
        0xfefc,0xfecb
    }, shape_excepttashkeel_at_end[]={
        0x20,0xfee5,0xfef5,0xfe8d,0x20,0xfee5,0xfe76,0xfef7,0xfe8d,0x20,0xfee5,
        0x670,0xfefb,0xfe8d,0x20,0xfee5,0x653,0xfef5,0xfe8d,0x20,0xfee5,0x655,0xfef9,
        0xfe8d,0x20,0xfee5,0x654,0xfef5,0xfe8d,0x20,0xfefc,0xfecb,0x20,0x20,0x20,
        0x20,0x20,0x20
    }, shape_excepttashkeel_at_begin[]={
        0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xfee5,0xfef5,0xfe8d,0x20,0xfee5,0xfe76,
        0xfef7,0xfe8d,0x20,0xfee5,0x670,0xfefb,0xfe8d,0x20,0xfee5,0x653,0xfef5,0xfe8d,
        0x20,0xfee5,0x655,0xfef9,0xfe8d,0x20,0xfee5,0x654,0xfef5,0xfe8d,0x20,0xfefc,0xfecb
    }, shape_excepttashkeel_grow_shrink[]={
        0x20,0xfee5,0xfef5,0xfe8d,0x20,0xfee5,0xfe76,0xfef7,0xfe8d,0x20,0xfee5,0x670,
        0xfefb,0xfe8d,0x20,0xfee5,0x653,0xfef5,0xfe8d,0x20,0xfee5,0x655,0xfef9,0xfe8d,
        0x20,0xfee5,0x654,0xfef5,0xfe8d,0x20,0xfefc,0xfecb
    };

    UChar dest[38];
    UErrorCode errorCode;
    int32_t length;

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE|U_SHAPE_LENGTH_FIXED_SPACES_NEAR|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_near) || uprv_memcmp(dest, shape_near, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_near)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE|U_SHAPE_LENGTH_FIXED_SPACES_AT_END|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_at_end) || uprv_memcmp(dest, shape_at_end, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_at_end)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE|U_SHAPE_LENGTH_FIXED_SPACES_AT_BEGINNING|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_at_begin) || uprv_memcmp(dest, shape_at_begin, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_at_begin)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE|U_SHAPE_LENGTH_GROW_SHRINK|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || uprv_memcmp(dest, shape_grow_shrink, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_grow_shrink)\n");
    }

    /* ==================== U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED ==================== */

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED|U_SHAPE_LENGTH_FIXED_SPACES_NEAR|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_excepttashkeel_near) || uprv_memcmp(dest, shape_excepttashkeel_near, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_excepttashkeel_near)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED|U_SHAPE_LENGTH_FIXED_SPACES_AT_END|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_excepttashkeel_at_end) || uprv_memcmp(dest,shape_excepttashkeel_at_end , length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_excepttashkeel_at_end)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED|U_SHAPE_LENGTH_FIXED_SPACES_AT_BEGINNING|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_excepttashkeel_at_begin) || uprv_memcmp(dest, shape_excepttashkeel_at_begin, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_excepttashkeel_at_begin)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED|U_SHAPE_LENGTH_GROW_SHRINK|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || uprv_memcmp(dest, shape_excepttashkeel_grow_shrink, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(LAMALEF shape_excepttashkeel_grow_shrink)\n");
    }
}

static void
doTashkeelSpecialVLTRArabicShapingTest() {
    static const UChar
    source[]={
        0x64A,0x628,0x631,0x639,0x20,
        0x64A,0x628,0x651,0x631,0x64E,0x639,0x20,
        0x64C,0x64A,0x628,0x631,0x64F,0x639,0x20,
        0x628,0x670,0x631,0x670,0x639,0x20,
        0x628,0x653,0x631,0x653,0x639,0x20,
        0x628,0x654,0x631,0x654,0x639,0x20,
        0x628,0x655,0x631,0x655,0x639,0x20,
    }, shape_near[]={
        0xfef2,0xfe91,0xfeae,0xfecb,0x20,0xfef2,0xfe91,0xfe7c,0xfeae,0xfe77,0xfecb,
        0x20,0xfe72,0xfef2,0xfe91,0xfeae,0xfe79,0xfecb,0x20,0xfe8f,0x670,0xfeae,0x670,
        0xfecb,0x20,0xfe8f,0x653,0xfeae,0x653,0xfecb,0x20,0xfe8f,0x654,0xfeae,0x654,
        0xfecb,0x20,0xfe8f,0x655,0xfeae,0x655,0xfecb,0x20
    }, shape_excepttashkeel_near[]={
        0xfef2,0xfe91,0xfeae,0xfecb,0x20,0xfef2,0xfe91,0xfe7c,0xfeae,0xfe76,0xfecb,0x20,
        0xfe72,0xfef2,0xfe91,0xfeae,0xfe78,0xfecb,0x20,0xfe8f,0x670,0xfeae,0x670,0xfecb,
        0x20,0xfe8f,0x653,0xfeae,0x653,0xfecb,0x20,0xfe8f,0x654,0xfeae,0x654,0xfecb,0x20,
        0xfe8f,0x655,0xfeae,0x655,0xfecb,0x20
    };

    UChar dest[43];
    UErrorCode errorCode;
    int32_t length;

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE|U_SHAPE_LENGTH_FIXED_SPACES_NEAR|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_near) || uprv_memcmp(dest, shape_near, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(TASHKEEL shape_near)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED|U_SHAPE_LENGTH_FIXED_SPACES_NEAR|
                         U_SHAPE_TEXT_DIRECTION_VISUAL_LTR,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(shape_excepttashkeel_near) || uprv_memcmp(dest, shape_excepttashkeel_near, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(TASHKEEL shape_excepttashkeel_near)\n");
    }
}

static void
doLOGICALArabicDeShapingTest() {
    static const UChar
    source[]={
        0x0020,0x0020,0x0020,0xFE8D,0xFEF5,0x0020,0xFEE5,0x0020,0xFE8D,0xFEF7,0x0020,
        0xFED7,0xFEFC,0x0020,0xFEE1,0x0020,0xFE8D,0xFEDF,0xFECC,0xFEAE,0xFE91,0xFEF4,
        0xFE94,0x0020,0xFE8D,0xFEDF,0xFEA4,0xFEAE,0xFE93,0x0020,0x0020,0x0020,0x0020
    }, unshape_near[]={
        0x20,0x20,0x20,0x627,0x644,0x622,0x646,0x20,0x627,0x644,0x623,0x642,0x644,0x627,
        0x645,0x20,0x627,0x644,0x639,0x631,0x628,0x64a,0x629,0x20,0x627,0x644,0x62d,0x631,
        0x629,0x20,0x20,0x20,0x20
    }, unshape_at_end[]={
        0x20,0x20,0x20,0x627,0x644,0x622,0x20,0x646,0x20,0x627,0x644,0x623,0x20,0x642,
        0x644,0x627,0x20,0x645,0x20,0x627,0x644,0x639,0x631,0x628,0x64a,0x629,0x20,0x627,
        0x644,0x62d,0x631,0x629,0x20
    }, unshape_at_begin[]={
        0x627,0x644,0x622,0x20,0x646,0x20,0x627,0x644,0x623,0x20,0x642,0x644,0x627,0x20,
        0x645,0x20,0x627,0x644,0x639,0x631,0x628,0x64a,0x629,0x20,0x627,0x644,0x62d,0x631,
        0x629,0x20,0x20,0x20,0x20
    }, unshape_grow_shrink[]={
        0x20,0x20,0x20,0x627,0x644,0x622,0x20,0x646,0x20,0x627,0x644,0x623,0x20,0x642,
        0x644,0x627,0x20,0x645,0x20,0x627,0x644,0x639,0x631,0x628,0x64a,0x629,0x20,0x627,
        0x644,0x62d,0x631,0x629,0x20,0x20,0x20,0x20
    };

    UChar dest[36];
    UErrorCode errorCode;
    int32_t length;

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_UNSHAPE|U_SHAPE_LENGTH_FIXED_SPACES_NEAR|
                         U_SHAPE_TEXT_DIRECTION_LOGICAL,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(unshape_near) || uprv_memcmp(dest, unshape_near, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(unshape_near)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_UNSHAPE|U_SHAPE_LENGTH_FIXED_SPACES_AT_END|
                         U_SHAPE_TEXT_DIRECTION_LOGICAL,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(unshape_at_end) || uprv_memcmp(dest, unshape_at_end, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(unshape_at_end)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_UNSHAPE|U_SHAPE_LENGTH_FIXED_SPACES_AT_BEGINNING|
                         U_SHAPE_TEXT_DIRECTION_LOGICAL,
                         &errorCode);

    if(U_FAILURE(errorCode) || length!=LENGTHOF(unshape_at_begin) || uprv_memcmp(dest, unshape_at_begin, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(unshape_at_begin)\n");
    }

    errorCode=U_ZERO_ERROR;

    length=u_shapeArabic(source, LENGTHOF(source),
                         dest, LENGTHOF(dest),
                         U_SHAPE_LETTERS_UNSHAPE|U_SHAPE_LENGTH_GROW_SHRINK|
                         U_SHAPE_TEXT_DIRECTION_LOGICAL,
                         &errorCode);

    if(U_FAILURE(errorCode) || uprv_memcmp(dest, unshape_grow_shrink, length*U_SIZEOF_UCHAR)!=0) {
        log_err("failure in u_shapeArabic(unshape_grow_shrink)\n");
    }

}

/* helpers ------------------------------------------------------------------ */

static void
initCharFromDirProps() {
    static const UVersionInfo ucd401={ 4, 0, 1, 0 };
    static UVersionInfo ucdVersion={ 0, 0, 0, 0 };

    /* lazy initialization */
    if(ucdVersion[0]>0) {
        return;
    }

    u_getUnicodeVersion(ucdVersion);
    if(uprv_memcmp(ucdVersion, ucd401, sizeof(UVersionInfo))>=0) {
        /* Unicode 4.0.1 changes bidi classes for +-/ */
        charFromDirProp[U_EUROPEAN_NUMBER_SEPARATOR]=0x2b; /* change ES character from / to + */
    }
}

/* return a string with characters according to the desired directional properties */
static UChar *
getStringFromDirProps(const uint8_t *dirProps, int32_t length, UChar *buffer) {
    int32_t i;

    initCharFromDirProps();

    /* this part would have to be modified for UTF-x */
    for(i=0; i<length; ++i) {
        buffer[i]=charFromDirProp[dirProps[i]];
    }
    buffer[length]=0;
    return buffer;
}

static void
printUnicode(const UChar *s, int32_t length, const UBiDiLevel *levels) {
    int32_t i;

    log_verbose("{ ");
    for(i=0; i<length; ++i) {
        if(levels!=NULL) {
            log_verbose("%4x.%u  ", s[i], levels[i]);
        } else {
            log_verbose("%4x    ", s[i]);
        }
    }
    log_verbose(" }");
}

/* new BIDI API */

/* Reordering Mode BiDi --------------------------------------------------------- */

static const UBiDiLevel paraLevels[] = { UBIDI_LTR, UBIDI_RTL };

static UBool
assertSuccessful(const char* message, UErrorCode* rc) {
    if (rc != NULL && U_FAILURE(*rc)) {
        log_err("%s() failed with error %s.\n", message, myErrorName(*rc));
        *rc = U_ZERO_ERROR;
        return FALSE;
    }
    return TRUE;
}

static UBool
assertStringsEqual(const char* expected, const char* actual, const char* src,
                   const char* mode, const char* option, UBiDi* pBiDi) {
    if (uprv_strcmp(expected, actual)) {
        char formatChars[MAXLEN];
        log_err("\nActual and expected output mismatch.\n"
            "%20s %s\n%20s %s\n%20s %s\n%20s %s\n%20s %d %s\n%20s %u\n%20s %d %s\n",
            "Input:", src,
            "Actual output:", actual,
            "Expected output:", expected,
            "Levels:", formatLevels(pBiDi, formatChars),
            "Reordering mode:", ubidi_getReorderingMode(pBiDi), mode,
            "Paragraph level:", ubidi_getParaLevel(pBiDi),
            "Reordering option:", ubidi_getReorderingOptions(pBiDi), option);
        return FALSE;
    }
    return TRUE;
}

static UBiDi*
getBiDiObject() {
    UBiDi* pBiDi = ubidi_open();
    if (pBiDi == NULL) {
        log_err("Unable to allocate a UBiDi object.\n");
        exit(1);
    }
    return pBiDi;
}

#define MAKE_ITEMS(val) val, #val

static const struct {
    uint32_t value;
    const char* description;
}
modes[] = {
    { MAKE_ITEMS(UBIDI_REORDER_GROUP_NUMBERS_WITH_R) },
    { MAKE_ITEMS(UBIDI_REORDER_INVERSE_LIKE_DIRECT) },
    { MAKE_ITEMS(UBIDI_REORDER_NUMBERS_SPECIAL) },
    { MAKE_ITEMS(UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL) },
    { MAKE_ITEMS(UBIDI_REORDER_INVERSE_NUMBERS_AS_L) }
},
options[] = {
    { MAKE_ITEMS(UBIDI_OPTION_INSERT_MARKS) },
    { MAKE_ITEMS(0) }
};

#define TC_COUNT                LENGTHOF(textIn)
#define MODES_COUNT             LENGTHOF(modes)
#define OPTIONS_COUNT           LENGTHOF(options)
#define LEVELS_COUNT            LENGTHOF(paraLevels)

static const char* const textIn[] = {
// (0) 123
    "123",
// (1) .123->4.5
    ".123->4.5",
// (2) 678
    "678",
// (3) .678->8.9
    ".678->8.9",
// (4) JIH1.2,3MLK
    "JIH1.2,3MLK",
// (5) FE.>12->
    "FE.>12->",
// (6) JIH.>12->a
    "JIH.>12->a",
// (7) CBA.>67->89=a
    "CBA.>67->89=a",
// (8) CBA.123->xyz
    "CBA.123->xyz",
// (9) .>12->xyz
    ".>12->xyz",
// (10) a.>67->xyz
    "a.>67->xyz",
// (11) 123JIH
    "123JIH",
// (12) 123 JIH
    "123 JIH"
};

static const char* const textOut[] = {
// TC 0: 123
    "123",                                                              // (0)
// TC 1: .123->4.5
    ".123->4.5",                                                        // (1)
    "4.5<-123.",                                                        // (2)
// TC 2: 678
    "678",                                                              // (3)
// TC 3: .678->8.9
    ".8.9<-678",                                                        // (4)
    "8.9<-678.",                                                        // (5)
    ".678->8.9",                                                        // (6)
// TC 4: MLK1.2,3JIH
    "KLM1.2,3HIJ",                                                      // (7)
// TC 5: FE.>12->
    "12<.EF->",                                                         // (8)
    "<-12<.EF",                                                         // (9)
    "EF.>@12->",                                                        // (10)
// TC 6: JIH.>12->a
    "12<.HIJ->a",                                                       // (11)
    "a<-12<.HIJ",                                                       // (12)
    "HIJ.>@12->a",                                                      // (13)
    "a&<-12<.HIJ",                                                      // (14)
// TC 7: CBA.>67->89=a
    "ABC.>@67->89=a",                                                   // (15)
    "a=89<-67<.ABC",                                                    // (16)
    "a&=89<-67<.ABC",                                                   // (17)
    "89<-67<.ABC=a",                                                    // (18)
// TC 8: CBA.123->xyz
    "123.ABC->xyz",                                                     // (19)
    "xyz<-123.ABC",                                                     // (20)
    "ABC.@123->xyz",                                                    // (21)
    "xyz&<-123.ABC",                                                    // (22)
// TC 9: .>12->xyz
    ".>12->xyz",                                                        // (23)
    "xyz<-12<.",                                                        // (24)
    "xyz&<-12<.",                                                       // (25)
// TC 10: a.>67->xyz
    "a.>67->xyz",                                                       // (26)
    "a.>@67@->xyz",                                                     // (27)
    "xyz<-67<.a",                                                       // (28)
// TC 11: 123JIH
    "123HIJ",                                                           // (29)
    "HIJ123",                                                           // (30)
// TC 12: 123 JIH
    "123 HIJ",                                                          // (31)
    "HIJ 123",                                                          // (32)
};

#define NO                  UBIDI_MAP_NOWHERE
#define MAX_MAP_LENGTH      20

static const int forwardMap[][MAX_MAP_LENGTH] = {
// TC 0: 123
    { 0, 1, 2 },                                                        // (0)
// TC 1: .123->4.5
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },                                      // (1)
    { 8, 5, 6, 7, 4, 3, 0, 1, 2 },                                      // (2)
// TC 2: 678
    { 0, 1, 2 },                                                        // (3)
// TC 3: .678->8.9
    { 0, 6, 7, 8, 5, 4, 1, 2, 3 },                                      // (4)
    { 8, 5, 6, 7, 4, 3, 0, 1, 2 },                                      // (5)
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },                                      // (6)
// TC 4: MLK1.2,3JIH
    { 10, 9, 8, 3, 4, 5, 6, 7, 2, 1, 0 },                               // (7)
// TC 5: FE.>12->
    { 5, 4, 3, 2, 0, 1, 6, 7 },                                         // (8)
    { 7, 6, 5, 4, 2, 3, 1, 0 },                                         // (9)
    { 1, 0, 2, 3, 5, 6, 7, 8 },                                         // (10)
// TC 6: JIH.>12->a
    { 6, 5, 4, 3, 2, 0, 1, 7, 8, 9 },                                   // (11)
    { 9, 8, 7, 6, 5, 3, 4, 2, 1, 0 },                                   // (12)
    { 2, 1, 0, 3, 4, 6, 7, 8, 9, 10 },                                  // (13)
    { 10, 9, 8, 7, 6, 4, 5, 3, 2, 0 },                                  // (14)
// TC 7: CBA.>67->89=a
    { 2, 1, 0, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13 },                      // (15)
    { 12, 11, 10, 9, 8, 6, 7, 5, 4, 2, 3, 1, 0 },                       // (16)
    { 13, 12, 11, 10, 9, 7, 8, 6, 5, 3, 4, 2, 0 },                      // (17)
    { 10, 9, 8, 7, 6, 4, 5, 3, 2, 0, 1, 11, 12 },                       // (18)
// TC 8: CBA.123->xyz
    { 6, 5, 4, 3, 0, 1, 2, 7, 8, 9, 10, 11 },                           // (19)
    { 11, 10, 9, 8, 5, 6, 7, 4, 3, 0, 1, 2 },                           // (20)
    { 2, 1, 0, 3, 5, 6, 7, 8, 9, 10, 11, 12 },                          // (21)
    { 12, 11, 10, 9, 6, 7, 8, 5, 4, 0, 1, 2 },                          // (22)
// TC 9: .>12->xyz
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },                                      // (23)
    { 8, 7, 5, 6, 4, 3, 0, 1, 2 },                                      // (24)
    { 9, 8, 6, 7, 5, 4, 0, 1, 2 },                                      // (25)
// TC 10: a.>67->xyz
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },                                   // (26)
    { 0, 1, 2, 4, 5, 7, 8, 9, 10, 11 },                                 // (27)
    { 9, 8, 7, 5, 6, 4, 3, 0, 1, 2 },                                   // (28)
// TC 11: 123JIH
    { 0, 1, 2, 5, 4, 3 },                                               // (29)
    { 3, 4, 5, 2, 1, 0 },                                               // (30)
// TC 12: 123 JIH
    { 0, 1, 2, 3, 6, 5, 4 },                                            // (31)
    { 4, 5, 6, 3, 2, 1, 0 },                                            // (32)
};

static const int inverseMap[][MAX_MAP_LENGTH] = {
// TC 0: 123
    { 0, 1, 2 },                                                        // (0)
// TC 1: .123->4.5
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },                                      // (1)
    { 6, 7, 8, 5, 4, 1, 2, 3, 0 },                                      // (2)
// TC 2: 678
    { 0, 1, 2 },                                                        // (3)
// TC 3: .678->8.9
    { 0, 6, 7, 8, 5, 4, 1, 2, 3 },                                      // (4)
    { 6, 7, 8, 5, 4, 1, 2, 3, 0 },                                      // (5)
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },                                      // (6)
// TC 4: MLK1.2,3JIH
    { 10, 9, 8, 3, 4, 5, 6, 7, 2, 1, 0 },                               // (7)
// TC 5: FE.>12->
    { 4, 5, 3, 2, 1, 0, 6, 7 },                                         // (8)
    { 7, 6, 4, 5, 3, 2, 1, 0 },                                         // (9)
    { 1, 0, 2, 3, NO, 4, 5, 6, 7 },                                     // (10)
// TC 6: JIH.>12->a
    { 5, 6, 4, 3, 2, 1, 0, 7, 8, 9 },                                   // (11)
    { 9, 8, 7, 5, 6, 4, 3, 2, 1, 0 },                                   // (12)
    { 2, 1, 0, 3, 4, NO, 5, 6, 7, 8, 9 },                               // (13)
    { 9, NO, 8, 7, 5, 6, 4, 3, 2, 1, 0 },                               // (14)
// TC 7: CBA.>67->89=a
    { 2, 1, 0, 3, 4, NO, 5, 6, 7, 8, 9, 10, 11, 12 },                   // (15)
    { 12, 11, 9, 10, 8, 7, 5, 6, 4, 3, 2, 1, 0 },                       // (16)
    { 12, NO, 11, 9, 10, 8, 7, 5, 6, 4, 3, 2, 1, 0 },                   // (17)
    { 9, 10, 8, 7, 5, 6, 4, 3, 2, 1, 0, 11, 12 },                       // (18)
// TC 8: CBA.123->xyz
    { 4, 5, 6, 3, 2, 1, 0, 7, 8, 9, 10, 11 },                           // (19)
    { 9, 10, 11, 8, 7, 4, 5, 6, 3, 2, 1, 0 },                           // (20)
    { 2, 1, 0, 3, NO, 4, 5, 6, 7, 8, 9, 10, 11 },                       // (21)
    { 9, 10, 11, NO, 8, 7, 4, 5, 6, 3, 2, 1, 0 },                       // (22)
// TC 9: .>12->xyz
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },                                      // (23)
    { 6, 7, 8, 5, 4, 2, 3, 1, 0 },                                      // (24)
    { 6, 7, 8, NO, 5, 4, 2, 3, 1, 0 },                                  // (25)
// TC 10: a.>67->xyz
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },                                   // (26)
    { 0, 1, 2, NO, 3, 4, NO, 5, 6, 7, 8, 9 },                           // (27)
    { 7, 8, 9, 6, 5, 3, 4, 2, 1, 0 },                                   // (28)
// TC 11: 123JIH
    { 0, 1, 2, 5, 4, 3 },                                               // (29)
    { 5, 4, 3, 0, 1, 2 },                                               // (30)
// TC 12: 123 JIH
    { 0, 1, 2, 3, 6, 5, 4 },                                            // (31)
    { 6, 5, 4, 3, 0, 1, 2 },                                            // (32)
};

static const char outIndices[TC_COUNT][MODES_COUNT - 1][OPTIONS_COUNT]
            [LEVELS_COUNT] = {
    { // TC 0: 123
        {{ 0,  0}, { 0,  0}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{ 0,  0}, { 0,  0}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{ 0,  0}, { 0,  0}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{ 0,  0}, { 0,  0}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 1: .123->4.5
        {{ 1,  2}, { 1,  2}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{ 1,  2}, { 1,  2}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{ 1,  2}, { 1,  2}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{ 1,  2}, { 1,  2}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 2: 678
        {{ 3,  3}, { 3,  3}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{ 3,  3}, { 3,  3}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{ 3,  3}, { 3,  3}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{ 3,  3}, { 3,  3}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 3: .678->8.9
        {{ 6,  5}, { 6,  5}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{ 4,  5}, { 4,  5}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{ 6,  5}, { 6,  5}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{ 6,  5}, { 6,  5}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 4: MLK1.2,3JIH
        {{ 7,  7}, { 7,  7}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{ 7,  7}, { 7,  7}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{ 7,  7}, { 7,  7}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{ 7,  7}, { 7,  7}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 5: FE.>12->
        {{ 8,  9}, { 8,  9}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{10,  9}, { 8,  9}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{ 8,  9}, { 8,  9}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{10,  9}, { 8,  9}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 6: JIH.>12->a
        {{11, 12}, {11, 12}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{13, 14}, {11, 12}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{11, 12}, {11, 12}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{13, 14}, {11, 12}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 7: CBA.>67->89=a
        {{18, 16}, {18, 16}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{18, 17}, {18, 16}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{18, 16}, {18, 16}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{15, 17}, {18, 16}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 8: CBA.>124->xyz
        {{19, 20}, {19, 20}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{21, 22}, {19, 20}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{19, 20}, {19, 20}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{21, 22}, {19, 20}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 9: .>12->xyz
        {{23, 24}, {23, 24}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{23, 25}, {23, 24}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{23, 24}, {23, 24}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{23, 25}, {23, 24}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 10: a.>67->xyz
        {{26, 26}, {26, 26}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{26, 27}, {26, 28}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{26, 28}, {26, 28}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{26, 27}, {26, 28}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 11: 124JIH
        {{30, 30}, {30, 30}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{29, 30}, {29, 30}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{30, 30}, {30, 30}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{30, 30}, {30, 30}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    },
    { // TC 12: 124 JIH
        {{32, 32}, {32, 32}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
        {{31, 32}, {31, 32}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
        {{31, 32}, {31, 32}}, // UBIDI_REORDER_NUMBERS_SPECIAL
        {{31, 32}, {31, 32}}  // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
    }
};

static UBool
assertRoundTrip(UBiDi *pBiDi, int32_t tc, int32_t outIndex, const char *srcChars,
                const char *destChars, const UChar *dest, int32_t destLen,
                int mode, int option, UBiDiLevel level) {

    static const char roundtrip[TC_COUNT][MODES_COUNT][OPTIONS_COUNT]
                [LEVELS_COUNT] = {
        { // TC 0: 123
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 1: .123->4.5
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 2: 678
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 3: .678->8.9
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 0,  0}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 4: MLK1.2,3JIH
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 5: FE.>12->
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 0,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 6: JIH.>12->a
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 0,  0}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 7: CBA.>67->89=a
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 0,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 0,  0}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 8: CBA.>123->xyz
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 0,  0}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 9: .>12->xyz
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  0}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 10: a.>67->xyz
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  0}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 11: 123JIH
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        },
        { // TC 12: 123 JIH
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_GROUP_NUMBERS_WITH_R
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_LIKE_DIRECT
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}, // UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL
            {{ 1,  1}, { 1,  1}}  // UBIDI_REORDER_INVERSE_NUMBERS_AS_L
        }
    };

    #define SET_ROUND_TRIP_MODE(mode) \
        ubidi_setReorderingMode(pBiDi, mode); \
        desc = #mode; \
        break;

    UErrorCode rc = U_ZERO_ERROR;
    UChar dest2[MAXLEN];
    int32_t destLen2;
    const char* desc;
    char destChars2[MAXLEN];
    char destChars3[MAXLEN];

    switch (modes[mode].value) {
        case UBIDI_REORDER_NUMBERS_SPECIAL:
            SET_ROUND_TRIP_MODE(UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL)
        case UBIDI_REORDER_GROUP_NUMBERS_WITH_R:
            SET_ROUND_TRIP_MODE(UBIDI_REORDER_GROUP_NUMBERS_WITH_R)
        case UBIDI_REORDER_RUNS_ONLY:
            SET_ROUND_TRIP_MODE(UBIDI_REORDER_RUNS_ONLY)
        case UBIDI_REORDER_INVERSE_NUMBERS_AS_L:
            SET_ROUND_TRIP_MODE(UBIDI_REORDER_DEFAULT)
        case UBIDI_REORDER_INVERSE_LIKE_DIRECT:
            SET_ROUND_TRIP_MODE(UBIDI_REORDER_DEFAULT)
        case UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL:
            SET_ROUND_TRIP_MODE(UBIDI_REORDER_NUMBERS_SPECIAL)
        default:
            SET_ROUND_TRIP_MODE(UBIDI_REORDER_INVERSE_LIKE_DIRECT)
    }
    ubidi_setReorderingOptions(pBiDi, UBIDI_OPTION_REMOVE_CONTROLS);

    ubidi_setPara(pBiDi, dest, destLen, level, NULL, &rc);
    assertSuccessful("ubidi_setPara", &rc);
    *dest2 = 0;
    destLen2 = ubidi_writeReordered(pBiDi, dest2, MAXLEN, UBIDI_DO_MIRRORING,
                                    &rc);
    assertSuccessful("ubidi_writeReordered", &rc);

    u16ToPseudo(destLen, dest, destChars3);
    u16ToPseudo(destLen2, dest2, destChars2);
    if (strcmp(srcChars, destChars2)) {
        if (roundtrip[tc][mode][option][level]) {
            log_err("\nRound trip failed for case=%d mode=%d option=%d.\n"
                    "%20s %s\n%20s %s\n%20s %s\n%20s %s\n%20s %s"
                    "\n%20s %u\n", tc, mode, option,
                    "Original text:", srcChars,
                    "Round-tripped text:", destChars2,
                    "Intermediate  text:", destChars3,
                    "Reordering mode:", modes[mode].description,
                    "Reordering option:", options[option].description,
                    "Paragraph level:", level);
        }
        else {
            log_verbose("\nExpected round trip failure for case=%d mode=%d option=%d.\n"
                    "%20s %s\n%20s %s\n%20s %s\n%20s %s\n%20s %s"
                    "\n%20s %u\n", tc, mode, option,
                    "Original text:", srcChars,
                    "Round-tripped text:", destChars2,
                    "Intermediate  text:", destChars3,
                    "Reordering mode:", modes[mode].description,
                    "Reordering option:", options[option].description,
                    "Paragraph level:", level);
        }
        return FALSE;
    }
    if (!checkResultLength(pBiDi, destChars, destChars2, dest2, destLen2,
                           desc, "UBIDI_OPTION_REMOVE_CONTROLS", level)) {
        return FALSE;
    }
    if (outIndex > -1 && !testMaps(pBiDi, outIndex, srcChars, destChars,
                                      desc, "UBIDI_OPTION_REMOVE_CONTROLS",
                                      level, FALSE)) {
        return FALSE;
    }
    return TRUE;
}

static UBool
checkResultLength(UBiDi *pBiDi, const char *srcChars, const char *destChars,
                  const UChar *dest, int32_t destLen, const char* mode,
                  const char* option, UBiDiLevel level) {
    int32_t actualLen;
    if (strcmp(mode, "UBIDI_REORDER_INVERSE_NUMBERS_AS_L") == 0)
        actualLen = strlen(destChars);
    else  actualLen = ubidi_getResultLength(pBiDi);
    if (actualLen != destLen) {
        log_err("\nubidi_getResultLength failed.\n%20s %7d\n%20s %7d\n"
                "%20s %s\n%20s %s\n%20s %s\n%20s %s\n%20s %u\n",
                "Expected:", destLen, "Actual:", actualLen,
                "Input:", srcChars, "Output:", destChars,
                "Reordering mode:", mode, "Reordering option:", option,
                "Paragraph level:", level);
        return FALSE;
    }
    return TRUE;
}

static void
doReorderRunsTest(void) {
    static const struct {
        const char* textIn;
        const char* textOut[2][2];
        const char noroundtrip[2];
    } testCases[] = {
        {"abcGHI", {{"GHIabc", "GHIabc"}, {"GHIabc", "GHIabc"}}, {0, 0}},
        {"-=%$123/ *", {{"* /%$123=-", "* /%$123=-"},
                        {"* /%$123=-", "* /%$123=-"}}, {0, 0}},
        {"abc->12..>JKL", {{"JKL<..abc->12", "JKL<..abc->12"},
                           {"JKL<..abc->12", "JKL<..abc->12"}}, {0, 0}},
        {"JKL->12..>abc", {{"abc<..JKL->12", "abc<..JKL->12"},
                           {"abc<..JKL->12", "abc<..JKL->12"}}, {0, 0}},
        {"123->abc", {{"abc<-123", "abc<-123"},
                      {"abc&<-123", "abc<-123"}}, {1, 0}},
        {"123->JKL", {{"JKL<-123", "JKL<-123"},
                      {"JKL<-123", "JKL@<-123"}}, {0, 1}},
        {"*>12.>34->JKL", {{"JKL<-34<.12<*", "JKL<-34<.12<*"},
                           {"JKL<-34<.12<*", "JKL@<-34<.12<*"}}, {0, 1}},
        {"*>67.>89->JKL", {{"67.>89->JKL<*", "67.>89->JKL<*"},
                           {"67.>89->JKL<*", "67.>89->JKL<*"}}, {0, 0}},
        {"* /abc-=$%123", {{"abc-=$%123/ *", "abc-=$%123/ *"},
                           {"abc-=$%123/ *", "abc-=$%123/ *"}}, {0, 0}},
        {"* /$%def-=123", {{"def-=123%$/ *", "def-=123%$/ *"},
                           {"def-=123&%$/ *", "def-=123%$/ *"}}, {1, 0}},
        {"-=GHI* /123%$", {{"GHI* /123%$=-", "GHI* /123%$=-"},
                           {"GHI* /123%$=-", "GHI* /123%$=-"}}, {0, 0}},
        {"-=%$JKL* /123", {{"JKL* /123$%=-", "JKL* /123$%=-"},
                           {"JKL* /123&$%=-", "JKL* /123@$%=-"}}, {1, 1}},
        {"abc-=%$LMN* /123", {{"LMN* /123$%=-abc", "LMN* /123$%=-abc"},
                              {"LMN* /123&$%=-abc", "LMN* /123@$%=-abc"}}, {1, 1}}
    };
    UBiDi *pBiDi = getBiDiObject();
    UBiDi *pL2VBiDi = getBiDiObject();
    UChar src[MAXLEN], dest[MAXLEN], visual1[MAXLEN], visual2[MAXLEN];
    char destChars[MAXLEN], vis1Chars[MAXLEN], vis2Chars[MAXLEN];
    int32_t srcLen, destLen, vis1Len, vis2Len, option, i, j, nCases;
    UErrorCode rc = U_ZERO_ERROR;
    UBiDiLevel level;

    ubidi_setReorderingMode(pBiDi, UBIDI_REORDER_RUNS_ONLY);
    ubidi_setReorderingOptions(pL2VBiDi, UBIDI_OPTION_REMOVE_CONTROLS);

    for (option = 0; option < 2; option++) {
        ubidi_setReorderingOptions(pBiDi, option==0 ? UBIDI_OPTION_DEFAULT
                                                    : UBIDI_OPTION_INSERT_MARKS);
        for (i = 0, nCases = LENGTHOF(testCases); i < nCases; i++) {
            for(j = 0; j < 2; j++) {
                srcLen = strlen(testCases[i].textIn);
                pseudoToU16(srcLen, testCases[i].textIn, src);
                level = paraLevels[j];

                ubidi_setPara(pBiDi, src, srcLen, level, NULL, &rc);
                assertSuccessful("ubidi_setPara", &rc);
                *dest = 0;
                destLen = ubidi_writeReordered(pBiDi, dest, MAXLEN, UBIDI_DO_MIRRORING, &rc);
                assertSuccessful("ubidi_writeReordered", &rc);
                u16ToPseudo(destLen, dest, destChars);
                assertStringsEqual(testCases[i].textOut[option][level], destChars,
                        testCases[i].textIn, "UBIDI_REORDER_RUNS_ONLY",
                        option==0 ? "0" : "UBIDI_OPTION_INSERT_MARKS",
                        pBiDi);

                if((option==0) && testCases[i].noroundtrip[level]) {
                    continue;
                }
                ubidi_setPara(pL2VBiDi, src, srcLen, level, NULL, &rc);
                assertSuccessful("ubidi_setPara1", &rc);
                *visual1 = 0;
                vis1Len = ubidi_writeReordered(pL2VBiDi, visual1, MAXLEN, UBIDI_DO_MIRRORING, &rc);
                assertSuccessful("ubidi_writeReordered1", &rc);
                u16ToPseudo(vis1Len, visual1, vis1Chars);
                ubidi_setPara(pL2VBiDi, dest, destLen, level^1, NULL, &rc);
                assertSuccessful("ubidi_setPara2", &rc);
                *visual2 = 0;
                vis2Len = ubidi_writeReordered(pL2VBiDi, visual2, MAXLEN, UBIDI_DO_MIRRORING, &rc);
                assertSuccessful("ubidi_writeReordered2", &rc);
                u16ToPseudo(vis2Len, visual2, vis2Chars);
                assertStringsEqual(vis1Chars, vis2Chars,
                        testCases[i].textIn, "UBIDI_REORDER_RUNS_ONLY (2)",
                        option==0 ? "0" : "UBIDI_OPTION_INSERT_MARKS",
                        pBiDi);
            }
        }
    }
    ubidi_close(pBiDi);
    ubidi_close(pL2VBiDi);
}

static void
doReorderingModeBidiTest() {

    UChar src[MAXLEN], dest[MAXLEN];
    char destChars[MAXLEN];
    UBiDi *pBiDi = NULL, *pBiDi2 = NULL, *pBiDi3 = NULL;
    UErrorCode rc;
    int tc, mode, option, level;
    int32_t srcLen, destLen, index;
    const char *expectedChars;
    UBool testOK = TRUE;

    log_verbose("\n*** Bidi reordering mode test ***\n");

    pBiDi = getBiDiObject();
    pBiDi2 = getBiDiObject();
    pBiDi3 = getBiDiObject();

    ubidi_setInverse(pBiDi2, TRUE);

    for (tc = 0; tc < TC_COUNT; tc++) {
        const char* srcChars = textIn[tc];
        srcLen = strlen(srcChars);
        pseudoToU16(srcLen, srcChars, src);

        for (mode = 0; mode < MODES_COUNT; mode++) {
            ubidi_setReorderingMode(pBiDi, modes[mode].value);

            for (option = 0; option < OPTIONS_COUNT; option++) {
                ubidi_setReorderingOptions(pBiDi, options[option].value);

                for (level = 0; level < LEVELS_COUNT; level++) {
                    log_verbose("starting test %d mode=%d option=%d level=%d\n",
                                tc, modes[mode].value, options[option].value, level);
                    rc = U_ZERO_ERROR;
                    ubidi_setPara(pBiDi, src, srcLen, paraLevels[level], NULL, &rc);
                    assertSuccessful("ubidi_setPara", &rc);

                    *dest = 0;
                    destLen = ubidi_writeReordered(pBiDi, dest, MAXLEN,
                                                   UBIDI_DO_MIRRORING, &rc);
                    assertSuccessful("ubidi_writeReordered", &rc);
                    u16ToPseudo(destLen, dest, destChars);

                    if (modes[mode].value == UBIDI_REORDER_INVERSE_NUMBERS_AS_L) {
                        index = -1;
                        expectedChars = inverseBasic(pBiDi2, src, srcLen,
                                options[option].value, paraLevels[level], destChars);
                    }
                    else {
                        index = outIndices[tc][mode][option][level];
                        expectedChars = textOut[index];
                    }
                    if (!assertStringsEqual(expectedChars, destChars, srcChars,
                                modes[mode].description,
                                options[option].description,
                                pBiDi)) {
                        testOK = FALSE;
                    }
                    else if (options[option].value == UBIDI_OPTION_INSERT_MARKS &&
                             !assertRoundTrip(pBiDi3, tc, index, srcChars,
                                              destChars, dest, destLen,
                                              mode, option, paraLevels[level])) {
                        testOK = FALSE;
                    }
                    else if (!checkResultLength(pBiDi, srcChars, destChars,
                                dest, destLen, modes[mode].description,
                                options[option].description,
                                paraLevels[level])) {
                        testOK = FALSE;
                    }
                    else if (index > -1 && !testMaps(pBiDi, index, srcChars,
                            destChars, modes[mode].description,
                            options[option].description, paraLevels[level],
                            TRUE)) {
                        testOK = FALSE;
                    }
                }
            }
        }
    }
    if (testOK == TRUE) {
        log_verbose("\nReordering mode test OK\n");
    }
    ubidi_close(pBiDi3);
    ubidi_close(pBiDi2);
    ubidi_close(pBiDi);
}

static const char* inverseBasic(UBiDi *pBiDi, const UChar *src, int32_t srcLen,
                                uint32_t option, UBiDiLevel level, char *result) {
    UErrorCode rc = U_ZERO_ERROR;
    int32_t destLen;
    UChar dest2 [MAXLEN];

    if (pBiDi == NULL || src == NULL) {
        return NULL;
    }
    ubidi_setReorderingOptions(pBiDi, option);
    ubidi_setPara(pBiDi, src, srcLen, level, NULL, &rc);
    assertSuccessful("ubidi_setPara", &rc);

    *dest2 = 0;
    destLen = ubidi_writeReordered(pBiDi, dest2, MAXLEN,
                                   UBIDI_DO_MIRRORING, &rc);
    assertSuccessful("ubidi_writeReordered", &rc);
    u16ToPseudo(destLen, dest2, result);
    return result;
}

#define NULL_CHAR '\0'

static void
doBidiStreamingTest() {
#define MAXPORTIONS 10

    static const struct {
        const char* textIn;
        short int chunk;
        short int nPortions[2];
        char  portionLens[2][MAXPORTIONS];
        const char* message[2];
    } testData[] = {
        {   "123\\u000A"
            "abc45\\u000D"
            "67890\\u000A"
            "\\u000D"
            "02468\\u000D"
            "ghi",
            6, { 6, 6 }, {{ 6, 4, 6, 1, 6, 3}, { 4, 6, 6, 1, 6, 3 }},
            {"6, 4, 6, 1, 6, 3", "4, 6, 6, 1, 6, 3"}
        },
        {   "abcd\\u000Afgh\\u000D12345\\u000A456",
            6, { 4, 4 }, {{ 6, 3, 6, 3 }, { 5, 4, 6, 3 }},
            {"6, 3, 6, 3", "5, 4, 6, 3"}
        },
        {   "abcd\\u000Afgh\\u000D12345\\u000A45\\u000D",
            6, { 4, 4 }, {{ 6, 3, 6, 3 }, { 5, 4, 6, 3 }},
            {"6, 3, 6, 3", "5, 4, 6, 3"}
        },
        {   "abcde\\u000Afghi",
            10, { 1, 2 }, {{ 10 }, { 6, 4 }},
            {"10", "6, 4"}
        }
    };
    UChar src[MAXLEN];
    UBiDi *pBiDi = NULL;
    UChar *pSrc;
    UErrorCode rc = U_ZERO_ERROR;
    int32_t srcLen, processedLen, chunk, len, nPortions;
    int i, j, levelIndex;
    UBiDiLevel level;
    int nTests = LENGTHOF(testData), nLevels = LENGTHOF(paraLevels);
    UBool mismatch, testOK = TRUE;
    char processedLenStr[MAXPORTIONS * 5];

    log_verbose("\n*** Bidi streaming test ***\n");

    pBiDi = getBiDiObject();

    //ubidi_orderParagraphsLTR(pBiDi, TRUE);

    for (levelIndex = 0; levelIndex < nLevels; levelIndex++) {
        for (i = 0; i < nTests; i++) {
            srcLen = u_unescape(testData[i].textIn, src, MAXLEN);
            chunk = testData[i].chunk;
            nPortions = testData[i].nPortions[levelIndex];
            level = paraLevels[levelIndex];
            *processedLenStr = NULL_CHAR;

            mismatch = FALSE;

            ubidi_setReorderingOptions(pBiDi, UBIDI_OPTION_STREAMING);
            for (j = 0, pSrc = src; j < MAXPORTIONS && srcLen > 0; j++) {

                len = chunk < srcLen ? chunk : srcLen;
                ubidi_setPara(pBiDi, pSrc, len, level, NULL, &rc);
                assertSuccessful("ubidi_setPara", &rc);

                processedLen = ubidi_getProcessedLength(pBiDi);
                if (processedLen == 0) {
                    ubidi_setReorderingOptions(pBiDi, UBIDI_OPTION_DEFAULT);
                    j--;
                    continue;
                }
                ubidi_setReorderingOptions(pBiDi, UBIDI_OPTION_STREAMING);

                mismatch = j >= nPortions ||
                           processedLen != testData[i].portionLens[levelIndex][j];

                sprintf(processedLenStr + j * 4, "%4d", processedLen);
                srcLen -= processedLen, pSrc += processedLen;
            }

            if (mismatch || j != nPortions) {
                testOK = FALSE;
                log_err("\nProcessed lengths mismatch.\n"
                    "\tParagraph level: %u\n"
                    "\tInput string: %s\n"
                    "\tActually processed portion lengths: { %s }\n"
                    "\tExpected portion lengths          : { %s }\n",
                    paraLevels[levelIndex], testData[i].textIn,
                    processedLenStr, testData[i].message[levelIndex]);
            }
        }
    }
    ubidi_close(pBiDi);
    if (testOK == TRUE) {
        log_verbose("\nBiDi streaming test OK\n");
    }
}

U_CDECL_BEGIN

static UCharDirection U_CALLCONV
overrideBidiClass(const void *context, UChar32 c) {

#define DEF U_BIDI_CLASS_DEFAULT

    static const UCharDirection customClasses[] = {
       /* 0/8    1/9    2/A    3/B    4/C    5/D    6/E    7/F  */
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //00-07
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //08-0F
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //10-17
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //18-1F
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,     R,   DEF, //20-27
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //28-2F
           EN,    EN,    EN,    EN,    EN,    EN,    AN,    AN, //30-37
           AN,    AN,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //38-3F
            L,    AL,    AL,    AL,    AL,    AL,    AL,     R, //40-47
            R,     R,     R,     R,     R,     R,     R,     R, //48-4F
            R,     R,     R,     R,     R,     R,     R,     R, //50-57
            R,     R,     R,   LRE,   DEF,   RLE,   PDF,     S, //58-5F
          NSM,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //60-67
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //68-6F
          DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF,   DEF, //70-77
          DEF,   DEF,   DEF,   LRO,     B,   RLO,    BN,   DEF  //78-7F
    };
    static const int nEntries = LENGTHOF(customClasses);

    return c >= nEntries ? U_BIDI_CLASS_DEFAULT : customClasses[c];
}

U_CDECL_END

static void verifyCallbackParams(UBiDiClassCallback* fn, const void* context,
                                 UBiDiClassCallback* expectedFn,
                                 const void* expectedContext,
                                 int32_t sizeOfContext) {
    if (fn != expectedFn) {
        log_err("Class callback pointer is not set properly.\n");
    }
    if (context != expectedContext) {
        log_err("Class callback context is not set properly.\n");
    }
    else if (context != NULL &&
            uprv_memcmp(context, expectedContext, sizeOfContext)) {
        log_err("Callback context content doesn't match the expected one.\n");
    }
}

static void doBidiClassOverrideTest(void) {
    static const char* const textIn  = "JIH.>12->a \\u05D0\\u05D1 6 ABC78";
    static const char* const textOut = "12<.HIJ->a 78CBA 6 \\u05D1\\u05D0";

    UChar src[MAXLEN], dest[MAXLEN];
    UErrorCode rc = U_ZERO_ERROR;
    UBiDi *pBiDi = NULL;
    UBiDiClassCallback* oldFn = NULL;
    UBiDiClassCallback* newFn = overrideBidiClass;
    const void* oldContext = NULL;
    int32_t srcLen, destLen, textInSize = (int32_t)uprv_strlen(textIn);
    char* destChars = NULL;

    log_verbose("\n*** Bidi class override test ***\n");

    pBiDi = getBiDiObject();

    ubidi_getClassCallback(pBiDi, &oldFn, &oldContext);
    verifyCallbackParams(oldFn, oldContext, NULL, NULL, 0);

    ubidi_setClassCallback(pBiDi, newFn, textIn, &oldFn, &oldContext, &rc);
    if (!assertSuccessful("ubidi_setClassCallback", &rc)) {
        ubidi_close(pBiDi);
        return;
    }
    verifyCallbackParams(oldFn, oldContext, NULL, NULL, 0);

    ubidi_getClassCallback(pBiDi, &oldFn, &oldContext);
    verifyCallbackParams(oldFn, oldContext, newFn, textIn, textInSize);

    ubidi_setClassCallback(pBiDi, newFn, textIn, &oldFn, &oldContext, &rc);
    if (!assertSuccessful("ubidi_setClassCallback", &rc)) {
        ubidi_close(pBiDi);
        return;
    }
    verifyCallbackParams(oldFn, oldContext, newFn, textIn, textInSize);

    srcLen = u_unescape(textIn, src, MAXLEN);
    ubidi_setPara(pBiDi, src, srcLen, UBIDI_LTR, NULL, &rc);
    assertSuccessful("ubidi_setPara", &rc);

    destLen = ubidi_writeReordered(pBiDi, dest, MAXLEN,
                                   UBIDI_DO_MIRRORING, &rc);
    assertSuccessful("ubidi_writeReordered", &rc);

    destChars = aescstrdup(dest, destLen);
    if (uprv_strcmp(textOut, destChars)) {
        log_err("\nActual and expected output mismatch.\n"
            "%20s %s\n%20s %s\n%20s %s\n",
            "Input:", textIn, "Actual output:", destChars,
            "Expected output:", textOut);
    }
    else {
        log_verbose("\nClass override test OK\n");
    }
    ubidi_close(pBiDi);
}

static const char columns[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static char * formatMap(const int * map, int len, char * buffer)
{
    int i, k;
    char c;
    for (i = 0; i < len; i++) {
        k = map[i];
        if (k < 0)
            c = '-';
        else if (k >= sizeof columns)
            c = '+';
        else c = columns[k];
        buffer[i] = c;
    }
    buffer[len] = '\0';
    return buffer;
}

static UBool
testMaps(UBiDi *pBiDi, int32_t stringIndex, const char *src, const char *dest,
         const char *mode, const char* option, UBiDiLevel level, UBool forward)
{
    int32_t actualLogicalMap[MAX_MAP_LENGTH];
    int32_t actualVisualMap[MAX_MAP_LENGTH];
    int32_t getIndexMap[MAX_MAP_LENGTH];
    int32_t i, srcLen, resLen, index;
    const int32_t *expectedLogicalMap, *expectedVisualMap;
    UErrorCode rc = U_ZERO_ERROR;
    UBool testOK = TRUE;

    if (forward) {
        expectedLogicalMap = forwardMap[stringIndex];
        expectedVisualMap  = inverseMap[stringIndex];
    }
    else {
        expectedLogicalMap = inverseMap[stringIndex];
        expectedVisualMap  = forwardMap[stringIndex];
    }
    ubidi_getLogicalMap(pBiDi, actualLogicalMap, &rc);
    if (!assertSuccessful("ubidi_getLogicalMap", &rc)) {
        testOK = FALSE;
    }
    srcLen = ubidi_getProcessedLength(pBiDi);
    if (uprv_memcmp(expectedLogicalMap, actualLogicalMap, srcLen * sizeof(int32_t))) {
        char expChars[MAX_MAP_LENGTH];
        char actChars[MAX_MAP_LENGTH];
        log_err("\nubidi_getLogicalMap() returns unexpected map for output string "
                "index %d\n"
                "source: %s\n"
                "dest  : %s\n"
                "Scale : %s\n"
                "ExpMap: %s\n"
                "Actual: %s\n"
                "Paragraph level  : %d == %d\n"
                "Reordering mode  : %s == %d\n"
                "Reordering option: %s == %d\n"
                "Forward flag     : %d\n",
                stringIndex, src, dest, columns,
                formatMap(expectedLogicalMap, srcLen, expChars),
                formatMap(actualLogicalMap, srcLen, actChars),
                level, ubidi_getParaLevel(pBiDi),
                mode, ubidi_getReorderingMode(pBiDi),
                option, ubidi_getReorderingOptions(pBiDi),
                forward
                );
        testOK = FALSE;
    }
    resLen = ubidi_getResultLength(pBiDi);
    ubidi_getVisualMap(pBiDi, actualVisualMap, &rc);
    assertSuccessful("ubidi_getVisualMap", &rc);
    if (uprv_memcmp(expectedVisualMap, actualVisualMap, resLen * sizeof(int32_t))) {
        char expChars[MAX_MAP_LENGTH];
        char actChars[MAX_MAP_LENGTH];
        log_err("\nubidi_getVisualMap() returns unexpected map for output string "
                "index %d\n"
                "source: %s\n"
                "dest  : %s\n"
                "Scale : %s\n"
                "ExpMap: %s\n"
                "Actual: %s\n"
                "Paragraph level  : %d == %d\n"
                "Reordering mode  : %s == %d\n"
                "Reordering option: %s == %d\n"
                "Forward flag     : %d\n",
                stringIndex, src, dest, columns,
                formatMap(expectedVisualMap, resLen, expChars),
                formatMap(actualVisualMap, resLen, actChars),
                level, ubidi_getParaLevel(pBiDi),
                mode, ubidi_getReorderingMode(pBiDi),
                option, ubidi_getReorderingOptions(pBiDi),
                forward
                );
        testOK = FALSE;
    }
    for (i = 0; i < srcLen; i++) {
        index = ubidi_getVisualIndex(pBiDi, i, &rc);
        assertSuccessful("ubidi_getVisualIndex", &rc);
        getIndexMap[i] = index;
    }
    if (uprv_memcmp(actualLogicalMap, getIndexMap, srcLen * sizeof(int32_t))) {
        char actChars[MAX_MAP_LENGTH];
        char gotChars[MAX_MAP_LENGTH];
        log_err("\nMismatch between ubidi_getLogicalMap and ubidi_getVisualIndex for output string "
                "index %d\n"
                "source: %s\n"
                "dest  : %s\n"
                "Scale : %s\n"
                "ActMap: %s\n"
                "IdxMap: %s\n"
                "Paragraph level  : %d == %d\n"
                "Reordering mode  : %s == %d\n"
                "Reordering option: %s == %d\n"
                "Forward flag     : %d\n",
                stringIndex, src, dest, columns,
                formatMap(actualLogicalMap, srcLen, actChars),
                formatMap(getIndexMap, srcLen, gotChars),
                level, ubidi_getParaLevel(pBiDi),
                mode, ubidi_getReorderingMode(pBiDi),
                option, ubidi_getReorderingOptions(pBiDi),
                forward
                );
        testOK = FALSE;
    }
    for (i = 0; i < resLen; i++) {
        index = ubidi_getLogicalIndex(pBiDi, i, &rc);
        assertSuccessful("ubidi_getLogicalIndex", &rc);
        getIndexMap[i] = index;
    }
    if (uprv_memcmp(actualVisualMap, getIndexMap, resLen * sizeof(int32_t))) {
        char actChars[MAX_MAP_LENGTH];
        char gotChars[MAX_MAP_LENGTH];
        log_err("\nMismatch between ubidi_getVisualMap and ubidi_getLogicalIndex for output string "
                "index %d\n"
                "source: %s\n"
                "dest  : %s\n"
                "Scale : %s\n"
                "ActMap: %s\n"
                "IdxMap: %s\n"
                "Paragraph level  : %d == %d\n"
                "Reordering mode  : %s == %d\n"
                "Reordering option: %s == %d\n"
                "Forward flag     : %d\n",
                stringIndex, src, dest, columns,
                formatMap(actualVisualMap, resLen, actChars),
                formatMap(getIndexMap, resLen, gotChars),
                level, ubidi_getParaLevel(pBiDi),
                mode, ubidi_getReorderingMode(pBiDi),
                option, ubidi_getReorderingOptions(pBiDi),
                forward
                );
        testOK = FALSE;
    }
    return testOK;
}

