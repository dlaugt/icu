#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "unicode/uversion.h"


U_EXPORT const struct {
    uint16_t headerSize;
    uint8_t magic1, magic2;
    UDataInfo info;
    char padding[8];
    uint32_t count, reserved;
    struct {
        const char *name; 
        const void *data;
    } toc[1];

} U_EXPORT2 U_ICUDATA_ENTRY_POINT = {
    32,          /* headerSize */
    0xda,        /* magic1,    */
    0x27,        /* magic2     */
    {            /*UDataInfo   */
       20,           /* size        */
        0,           /* reserved    */
        0,           /* isBigEndian */
        0,           /* charsetFamily */
        2,           /* sizeof UChar  */
        0,           /* reserved      */
       {             /* data format identifier */
           0x54, 0x6f, 0x43, 0x50}, /* "ToCP" */
           {1, 0, 0, 0},   /* format version major, minor, milli, micro */
           {0, 0, 0, 0}    /* dataVersion   */
    },
    {0,0,0,0,0,0,0,0},  /* Padding[8]   */ 
    0,                  /* count        */
    0,                  /* Reserved     */
    {
        { "dummyName", 0 }
    }
};


