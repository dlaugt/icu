/*
 *******************************************************************************
 *
 *   Copyright (C) 2003, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  usprep.cpp
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003jul2
 *   created by: Ram Viswanadha
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/usprep.h"

#include "unicode/unorm.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/uversion.h"
#include "umutex.h"
#include "cmemory.h"
#include "sprpimpl.h"
#include "ustr_imp.h"
#include "uhash.h"
#include "cstring.h"

U_CDECL_BEGIN

/*
Static cache for already opened StringPrep profiles
*/
static UHashtable *SHARED_DATA_HASHTABLE = NULL;

static UMTX usprepMutex = NULL;


static UBool U_CALLCONV
isAcceptable(void * /* context */,
             const char * /* type */, 
             const char * /* name */,
             const UDataInfo *pInfo) {
    if(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x53 &&   /* dataFormat="SPRP" */
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x52 &&
        pInfo->dataFormat[3]==0x50 &&
        pInfo->formatVersion[0]==3 &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static int32_t U_CALLCONV
getFoldingOffset(uint32_t data) {

    return (int32_t)data;

}

/* hashes an entry  */
static int32_t U_EXPORT2 U_CALLCONV 
hashEntry(const UHashTok parm) {
    UStringPrepKey *b = (UStringPrepKey *)parm.pointer;
    UHashTok namekey, pathkey;
    namekey.pointer = b->name;
    pathkey.pointer = b->path;
    return uhash_hashChars(namekey)+37*uhash_hashChars(pathkey);
}

/* compares two entries */
static UBool U_EXPORT2 U_CALLCONV 
compareEntries(const UHashTok p1, const UHashTok p2) {
    UStringPrepKey *b1 = (UStringPrepKey *)p1.pointer;
    UStringPrepKey *b2 = (UStringPrepKey *)p2.pointer;
    UHashTok name1, name2, path1, path2;
    name1.pointer = b1->name;
    name2.pointer = b2->name;
    path1.pointer = b1->path;
    path2.pointer = b2->path;
    return ((UBool)(uhash_compareChars(name1, name2) & 
        uhash_compareChars(path1, path2)));
}

U_CDECL_END

U_CFUNC void 
usprep_init(UErrorCode *status) {
    umtx_init(&usprepMutex);
}

/** Initializes the cache for resources */
static void 
initCache(UErrorCode *status) {
  UBool makeCache = FALSE;
  umtx_lock(&usprepMutex);
  makeCache = (SHARED_DATA_HASHTABLE ==  NULL);
  umtx_unlock(&usprepMutex);
  if(makeCache) {
      UHashtable *newCache = uhash_open(hashEntry, compareEntries, status);
      if (U_FAILURE(*status)) {
          return;
      }
      umtx_lock(&usprepMutex);
      if(SHARED_DATA_HASHTABLE == NULL) {
          SHARED_DATA_HASHTABLE = newCache;
          newCache = NULL;
      }
      umtx_unlock(&usprepMutex);
      if(newCache != NULL) {
          uhash_close(newCache);
      }
  }
}

static UBool U_CALLCONV
loadData(UStringPrepProfile* profile, 
         const char* path, 
         const char* name, 
         const char* type, 
         UErrorCode* errorCode) {
    /* load Unicode SPREP data from file */    
    UTrie _sprepTrie={ 0,0,0,0,0,0,0 };
    UDataMemory *dataMemory;
    const int32_t *p=NULL;
    const uint8_t *pb;
    UVersionInfo unicodeVersion;
    int32_t normVer, uniVer;

    if(errorCode==NULL || U_FAILURE(*errorCode)) {
        return 0;
    }

    /* open the data outside the mutex block */
    //TODO: change the path
    dataMemory=udata_openChoice(path, type, name, isAcceptable, NULL, errorCode);
    if(U_FAILURE(*errorCode)) {
        return FALSE;
    }

    p=(const int32_t *)udata_getMemory(dataMemory);
    pb=(const uint8_t *)(p+_SPREP_INDEX_TOP);
    utrie_unserialize(&_sprepTrie, pb, p[_SPREP_INDEX_TRIE_SIZE], errorCode);
    _sprepTrie.getFoldingOffset=getFoldingOffset;


    if(U_FAILURE(*errorCode)) {
        udata_close(dataMemory);
        return FALSE;
    }

    /* in the mutex block, set the data for this process */
    umtx_lock(&usprepMutex);
    if(profile->sprepData==NULL) {
        profile->sprepData=dataMemory;
        dataMemory=NULL;
        uprv_memcpy(&profile->indexes, p, sizeof(profile->indexes));
        uprv_memcpy(&profile->sprepTrie, &_sprepTrie, sizeof(UTrie));
    } else {
        p=(const int32_t *)udata_getMemory(profile->sprepData);
    }
    umtx_unlock(&usprepMutex);
    /* initialize some variables */
    profile->mappingData=(uint16_t *)((uint8_t *)(p+_SPREP_INDEX_TOP)+profile->indexes[_SPREP_INDEX_TRIE_SIZE]);
    
    /* 
     * check the normalization corrections version and the current Unicode version 
     * supported by ICU 
     */
    u_versionFromString(unicodeVersion, U_UNICODE_VERSION);
    normVer = profile->indexes[_SPREP_NORM_CORRECTNS_LAST_UNI_VERSION];
    uniVer  = (unicodeVersion[0] << 24) + (unicodeVersion[1] << 16) + 
              (unicodeVersion[2] << 8 ) + (unicodeVersion[3]);

    if( normVer < uniVer &&
        ((profile->indexes[_SPREP_OPTIONS] & _SPREP_NORMALIZATION_ON) > 0) /* normalization turned on*/
      ){
        *errorCode = U_INVALID_FORMAT_ERROR;
        udata_close(dataMemory);
        return FALSE;
    }
    profile->isDataLoaded = TRUE;

    /* if a different thread set it first, then close the extra data */
    if(dataMemory!=NULL) {
        udata_close(dataMemory); /* NULL if it was set correctly */
    }


    return profile->isDataLoaded;
}

static UStringPrepProfile* 
usprep_getProfile(const char* path, 
                  const char* name,
                  UErrorCode *status){

    UStringPrepProfile* profile = NULL;

    initCache(status);

    if(U_FAILURE(*status)){
        return NULL;
    }

    UStringPrepKey stackKey;
    /* 
     * const is cast way to save malloc, strcpy and free calls 
     * we use the passed in pointers for fetching the data from the 
     * hash table which is safe
     */
    stackKey.name = (char*) name;
    stackKey.path = (char*) path;

    /* fetch the data from the cache */
    profile = (UStringPrepProfile*) (uhash_get(SHARED_DATA_HASHTABLE,&stackKey));
    
    if(profile == NULL){
        UStringPrepKey* key   = (UStringPrepKey*) uprv_malloc(sizeof(UStringPrepKey));
        if(key == NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        /* else load the data and put the data in the cache */
        profile = (UStringPrepProfile*) uprv_malloc(sizeof(UStringPrepProfile));
        if(profile == NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(key);
            return NULL;
        }

        /* initialize the data struct members */
        uprv_memset(profile->indexes,0,sizeof(profile->indexes));
        profile->mappingData = NULL;
        profile->sprepData   = NULL;
        profile->refCount    = 0;
    
        /* initialize the  key memebers */
        key->name  = (char*) uprv_malloc(strlen(name)+1);
        if(key->name == NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(key);
            uprv_free(profile);
            return NULL;
        }

        uprv_strcpy(key->name, name);
        
        key->path=NULL;

        if(path != NULL){
            key->path      = (char*) uprv_malloc(strlen(path)+1);
            if(key->path == NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                uprv_free(key->path);
                uprv_free(key);
                uprv_free(profile);
                return NULL;
            }
            uprv_strcpy(key->path, path);
        }        

        /* load the data */
        if(!loadData(profile, path, name, _SPREP_DATA_TYPE, status) || U_FAILURE(*status) ){
            return NULL;
        }
        umtx_lock(&usprepMutex);
        /* add the data object to the cache */
        uhash_put(SHARED_DATA_HASHTABLE, key, profile, status);
        umtx_unlock(&usprepMutex);
    }
    umtx_lock(&usprepMutex);
    /* increment the refcount */
    profile->refCount++;
    umtx_unlock(&usprepMutex);

    return profile;
}

U_CAPI UStringPrepProfile* U_EXPORT2
usprep_open(const char* path, 
            const char* name,
            UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return NULL;
    }

    usprep_init(status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
       
    /* initialize the profile struct members */
    return usprep_getProfile(path,name,status);;
}

U_CAPI void U_EXPORT2
usprep_close(UStringPrepProfile* profile){
    if(profile==NULL){
        return;
    }

    umtx_lock(&usprepMutex);
    /* decrement the ref count*/
    if(profile->refCount > 0){
        profile->refCount--;
    }
    umtx_unlock(&usprepMutex);
    
}

static void 
usprep_unload(UStringPrepProfile* data){
    udata_close(data->sprepData);
}


static int32_t 
usprep_internal_flushCache(UBool noRefCount){
    UStringPrepProfile *profile = NULL;
    UStringPrepKey  *key  = NULL;
    int32_t pos = -1;
    int32_t deletedNum = 0;
    const UHashElement *e;

    /*
     * if shared data hasn't even been lazy evaluated yet
     * return 0
     */
    umtx_lock(&usprepMutex);
    if (SHARED_DATA_HASHTABLE == NULL) {
        umtx_unlock(&usprepMutex);
        return 0;
    }

    /*creates an enumeration to iterate through every element in the table */
    while ((e = uhash_nextElement(SHARED_DATA_HASHTABLE, &pos)) != NULL)
    {
        profile = (UStringPrepProfile *) e->value.pointer;
        key  = (UStringPrepKey *) e->key.pointer;

        if ((noRefCount== FALSE && profile->refCount == 0) || 
             noRefCount== TRUE) {
            deletedNum++;
            uhash_removeElement(SHARED_DATA_HASHTABLE, e);

            /* unload the data */
            usprep_unload(profile);

            if(key->name != NULL) {
                uprv_free(key->name);
                key->name=NULL;
            }
            if(key->path != NULL) {
                uprv_free(key->path);
                key->path=NULL;
            }
            uprv_free(profile);
            uprv_free(key);
        }
       
    }
    umtx_unlock(&usprepMutex);

    return deletedNum;
}

/* Works just like ucnv_flushCache() */
static int32_t 
usprep_flushCache(){
    return usprep_internal_flushCache(FALSE);
}

U_CFUNC UBool 
usprep_cleanup(void){
    if (SHARED_DATA_HASHTABLE != NULL) {
        usprep_internal_flushCache(TRUE);
        if (SHARED_DATA_HASHTABLE != NULL && uhash_count(SHARED_DATA_HASHTABLE) == 0) {
            uhash_close(SHARED_DATA_HASHTABLE);
            SHARED_DATA_HASHTABLE = NULL;
        }
    }

    umtx_destroy(&usprepMutex);             /* Don't worry about destroying the mutex even  */
                                            /*  if the hash table still exists.  The mutex  */
                                            /*  will lazily re-init  itself if needed.      */
    return (SHARED_DATA_HASHTABLE == NULL);
}

U_CFUNC void 
uprv_syntaxError(const UChar* rules, 
                 int32_t pos,
                 int32_t rulesLen,
                 UParseError* parseError){
    if(parseError == NULL){
        return;
    }
    if(pos == rulesLen && rulesLen >0){
        pos--;
    }
    parseError->offset = pos;
    parseError->line = 0 ; // we are not using line numbers 
    
    // for pre-context
    int32_t start = (pos <=U_PARSE_CONTEXT_LEN)? 0 : (pos - (U_PARSE_CONTEXT_LEN-1));
    int32_t stop  = pos;
    
    u_memcpy(parseError->preContext,rules+start,stop-start);
    //null terminate the buffer
    parseError->preContext[stop-start] = 0;
    
    //for post-context
    start = pos;
    if(start<rulesLen) {
        U16_FWD_1(rules, start, rulesLen);
    }

    stop  = ((pos+U_PARSE_CONTEXT_LEN)<= rulesLen )? (pos+(U_PARSE_CONTEXT_LEN)) : 
                                                            rulesLen;
    if(start < stop){
        u_memcpy(parseError->postContext,rules+start,stop-start);
        //null terminate the buffer
        parseError->postContext[stop-start]= 0;
    }
    
}


static inline UStringPrepType
getValues(uint16_t trieWord, int16_t& value, UBool& isIndex){

    UStringPrepType type;
    if(trieWord == 0){
        /* 
         * Initial value stored in the mapping table 
         * just return USPREP_TYPE_LIMIT .. so that
         * the source codepoint is copied to the destination
         */
        type = USPREP_TYPE_LIMIT;
    }else if(trieWord >= _SPREP_TYPE_THRESHOLD){
        type = (UStringPrepType) (trieWord - _SPREP_TYPE_THRESHOLD);
    }else{
        /* get the type */
        type = USPREP_MAP;
        /* ascertain if the value is index or delta */
        if(trieWord & 0x02){
            isIndex = TRUE;
            value = trieWord  >> 2; //mask off the lower 2 bits and shift

        }else{
            isIndex = FALSE;
            value = (int16_t)trieWord;
            value =  (value >> 2);

        }
 
        if((trieWord>>2) == _SPREP_MAX_INDEX_VALUE){
            type = USPREP_DELETE;
            isIndex =FALSE;
            value = 0;
        }
    }
    return type;
}



static int32_t 
usprep_map(  const UStringPrepProfile* profile, 
             const UChar* src, int32_t srcLength, 
             UChar* dest, int32_t destCapacity,
             int32_t options,
             UParseError* parseError,
             UErrorCode* status ){
    
    uint16_t result;
    int32_t destIndex=0;
    int32_t srcIndex;
    UBool allowUnassigned = (UBool) ((options & USPREP_ALLOW_UNASSIGNED)>0);
    UStringPrepType type;
    int16_t value;
    UBool isIndex;
    int32_t* indexes = (int32_t*)profile->indexes;

    // no error checking the caller check for error and arguments
    // no string length check the caller finds out the string length

    for(srcIndex=0;srcIndex<srcLength;){
        UChar32 ch;

        U16_NEXT(src,srcIndex,srcLength,ch);
        
        result=0;

        UTRIE_GET16(&profile->sprepTrie,ch,result);
        
        type = getValues(result, value, isIndex);

        // check if the source codepoint is unassigned
        if(type == USPREP_UNASSIGNED && allowUnassigned == FALSE){

            uprv_syntaxError(src,srcIndex-U16_LENGTH(ch), srcLength,parseError);
            *status = U_STRINGPREP_UNASSIGNED_ERROR;
            return 0;
            
        }else if(type == USPREP_MAP){
            
            int32_t index, length;

            if(isIndex){
                index = value;
                if(index >= indexes[_SPREP_ONE_UCHAR_MAPPING_INDEX_START] &&
                         index < indexes[_SPREP_TWO_UCHARS_MAPPING_INDEX_START]){
                    length = 1;
                }else if(index >= indexes[_SPREP_TWO_UCHARS_MAPPING_INDEX_START] &&
                         index < indexes[_SPREP_THREE_UCHARS_MAPPING_INDEX_START]){
                    length = 2;
                }else if(index >= indexes[_SPREP_THREE_UCHARS_MAPPING_INDEX_START] &&
                         index < indexes[_SPREP_FOUR_UCHARS_MAPPING_INDEX_START]){
                    length = 3;
                }else{
                    length = profile->mappingData[index++];
         
                }

                /* copy mapping to destination */
                for(int32_t i=0; i< length; i++){
                    if(destIndex < destCapacity  ){
                        dest[destIndex] = profile->mappingData[index+i];
                    }
                    destIndex++; /* for pre-flighting */
                }  
                continue;
            }else{
                // subtract the delta to arrive at the code point
                ch -= value;
            }

        }else if(type==USPREP_DELETE){
             // just consume the codepoint and contine
            continue;
        }
        //copy the code point into destination
        if(ch <= 0xFFFF){
            if(destIndex < destCapacity ){
                dest[destIndex] = (UChar)ch;
            }
            destIndex++;
        }else{
            if(destIndex+1 < destCapacity ){
                dest[destIndex]   = U16_LEAD(ch);
                dest[destIndex+1] = U16_TRAIL(ch);
            }
            destIndex +=2;
        }
       
    }
        
    return u_terminateUChars(dest, destCapacity, destIndex, status);
}


static int32_t 
usprep_normalize(   const UChar* src, int32_t srcLength, 
                    UChar* dest, int32_t destCapacity,
                    UErrorCode* status ){

    return unorm_normalize(src,srcLength,UNORM_NFKC,UNORM_UNICODE_3_2,dest,destCapacity,status);
}


 /*
   1) Map -- For each character in the input, check if it has a mapping
      and, if so, replace it with its mapping.  

   2) Normalize -- Possibly normalize the result of step 1 using Unicode
      normalization. 

   3) Prohibit -- Check for any characters that are not allowed in the
      output.  If any are found, return an error.  

   4) Check bidi -- Possibly check for right-to-left characters, and if
      any are found, make sure that the whole string satisfies the
      requirements for bidirectional strings.  If the string does not
      satisfy the requirements for bidirectional strings, return an
      error.  
      [Unicode3.2] defines several bidirectional categories; each character
       has one bidirectional category assigned to it.  For the purposes of
       the requirements below, an "RandALCat character" is a character that
       has Unicode bidirectional categories "R" or "AL"; an "LCat character"
       is a character that has Unicode bidirectional category "L".  Note


       that there are many characters which fall in neither of the above
       definitions; Latin digits (<U+0030> through <U+0039>) are examples of
       this because they have bidirectional category "EN".

       In any profile that specifies bidirectional character handling, all
       three of the following requirements MUST be met:

       1) The characters in section 5.8 MUST be prohibited.

       2) If a string contains any RandALCat character, the string MUST NOT
          contain any LCat character.

       3) If a string contains any RandALCat character, a RandALCat
          character MUST be the first character of the string, and a
          RandALCat character MUST be the last character of the string.
*/

#define MAX_STACK_BUFFER_SIZE 300


U_CAPI int32_t U_EXPORT2
usprep_prepare(   const UStringPrepProfile* profile,
                  const UChar* src, int32_t srcLength, 
                  UChar* dest, int32_t destCapacity,
                  int32_t options,
                  UParseError* parseError,
                  UErrorCode* status ){

    // check error status
    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    
    //check arguments
    if(profile==NULL || src==NULL || srcLength<-1 || (dest==NULL && destCapacity!=0)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    UChar b1Stack[MAX_STACK_BUFFER_SIZE], b2Stack[MAX_STACK_BUFFER_SIZE];
    UChar *b1 = b1Stack, *b2 = b2Stack;
    int32_t b1Len, b2Len=0,
            b1Capacity = MAX_STACK_BUFFER_SIZE , 
            b2Capacity = MAX_STACK_BUFFER_SIZE;
    uint16_t result;
    int32_t b2Index = 0;
    UCharDirection direction=U_CHAR_DIRECTION_COUNT, firstCharDir=U_CHAR_DIRECTION_COUNT;
    UBool leftToRight=FALSE, rightToLeft=FALSE;
    int32_t rtlPos =-1, ltrPos =-1;
    const int32_t *indexes = profile->indexes;

    // get the options
    UBool doNFKC            = (UBool)((indexes[_SPREP_OPTIONS] & _SPREP_NORMALIZATION_ON) > 0);
    UBool checkBiDi         = (UBool)((indexes[_SPREP_OPTIONS] & _SPREP_CHECK_BIDI_ON) > 0);

    //get the string length
    if(srcLength == -1){
        srcLength = u_strlen(src);
    }
    // map
    b1Len = usprep_map(profile, src, srcLength, b1, b1Capacity, options, parseError, status);

    if(*status == U_BUFFER_OVERFLOW_ERROR){
        // redo processing of string
        /* we do not have enough room so grow the buffer*/
        b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
        if(b1==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        *status = U_ZERO_ERROR; // reset error
        
        b1Len = usprep_map(profile, src, srcLength, b1, b1Len, options, parseError, status);
        
    }

    // normalize
    if(doNFKC == TRUE){
        b2Len = usprep_normalize(b1,b1Len, b2,b2Capacity,status);
        
        if(*status == U_BUFFER_OVERFLOW_ERROR){
            // redo processing of string
            /* we do not have enough room so grow the buffer*/
            b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR);
            if(b2==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; // reset error
        
            b2Len = usprep_normalize(b1,b1Len, b2,b2Len,status);
        
        }

    }else{
        b2 = b1;
        b2Len = b1Len;
    }
    

    if(U_FAILURE(*status)){
        goto CLEANUP;
    }

    UChar32 ch;
    UStringPrepType type;
    int16_t value;
    UBool isIndex;
    
    // Prohibit and checkBiDi in one pass
    for(b2Index=0; b2Index<b2Len;){
        
        ch = 0;

        U16_NEXT(b2, b2Index, b2Len, ch);

        UTRIE_GET16(&profile->sprepTrie,ch,result);
        
        type = getValues(result, value, isIndex);

        if( type == USPREP_PROHIBITED || 
            ((result < _SPREP_TYPE_THRESHOLD) && (result&0x01))){
            *status = U_STRINGPREP_PROHIBITED_ERROR;
            uprv_syntaxError(b1, b2Index-U16_LENGTH(ch), b2Len, parseError);
            goto CLEANUP;
        }

        direction = u_charDirection(ch);
        if(firstCharDir == U_CHAR_DIRECTION_COUNT){
            firstCharDir = direction;
        }
        if(direction == U_LEFT_TO_RIGHT){
            leftToRight = TRUE;
            ltrPos = b2Index-1;
        }
        if(direction == U_RIGHT_TO_LEFT || direction == U_RIGHT_TO_LEFT_ARABIC){
            rightToLeft = TRUE;
            rtlPos = b2Index-1;
        }
    }           
    if(checkBiDi == TRUE){
        // satisfy 2
        if( leftToRight == TRUE && rightToLeft == TRUE){
            *status = U_STRINGPREP_CHECK_BIDI_ERROR;
            uprv_syntaxError(b2,(rtlPos>ltrPos) ? rtlPos : ltrPos, b2Len, parseError);
            goto CLEANUP;
        }

        //satisfy 3
        if( rightToLeft == TRUE && 
            !((firstCharDir == U_RIGHT_TO_LEFT || firstCharDir == U_RIGHT_TO_LEFT_ARABIC) &&
              (direction == U_RIGHT_TO_LEFT || direction == U_RIGHT_TO_LEFT_ARABIC))
           ){
            *status = U_STRINGPREP_CHECK_BIDI_ERROR;
            uprv_syntaxError(b2, rtlPos, b2Len, parseError);
            return FALSE;
        }
    }
    if(b2Len <= destCapacity){
        uprv_memmove(dest,b2, b2Len*U_SIZEOF_UCHAR);
    }

CLEANUP:
    if(b1!=b1Stack){
        uprv_free(b1);
    }
    if(b1!=b1Stack && b2!=b2Stack){
        uprv_free(b2);
    }
    return u_terminateUChars(dest, destCapacity, b2Len, status);
}


U_CFUNC UBool 
usprep_isLabelSeparator(UStringPrepProfile* profile, 
                        UChar32 ch, UErrorCode* status){
    // check error status
    if(status==NULL || U_FAILURE(*status)){
        return FALSE;
    }
    //check the arguments
    if(profile==NULL){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }

    uint16_t result;
    UStringPrepType type;
    int16_t value;
    UBool isIndex;

    UTRIE_GET16(&profile->sprepTrie,ch, result);
    
    type = getValues(result,value,isIndex);

    if( type  == USPREP_LABEL_SEPARATOR){
        return TRUE;
    }
    
    return FALSE;
}

#endif /* #if !UCONFIG_NO_IDNA */
