/*
******************************************************************************
* Copyright (C) 1997-2001, International Business Machines Corporation and   *
* others. All Rights Reserved.                                               *
******************************************************************************
*
* File URESBUND.C
*
* Modification History:
*
*   Date        Name        Description
*   04/01/97    aliu        Creation.
*   06/14/99    stephen     Removed functions taking a filename suffix.
*   07/20/99    stephen     Changed for UResourceBundle typedef'd to void*
*   11/09/99    weiv            Added ures_getLocale()
*   March 2000  weiv        Total overhaul - using data in DLLs
*   06/20/2000  helena      OS/400 port changes; mostly typecast.
******************************************************************************
*/

#include "unicode/ustring.h"

#include "uresimp.h"
#include "cwchar.h"
#include "ucln_cmn.h"
#include "cmemory.h"
#include "cstring.h"
#include "uhash.h"
#include "umutex.h"

/* this is just for internal purposes. DO NOT USE! */
static void entryCloseInt(UResourceDataEntry *resB);


/*
Static cache for already opened resource bundles - mostly for keeping fallback info
TODO: This cache should probably be removed when the deprecated code is
      completely removed.
*/
static UHashtable *cache = NULL;

static UBool isMutexInited = FALSE;
static UMTX resbMutex = NULL;

/* INTERNAL: hashes an entry  */
static int32_t hashEntry(const UHashKey parm) {
    UResourceDataEntry *b = (UResourceDataEntry *)parm.pointer;
    UHashKey namekey, pathkey;
    namekey.pointer = b->fName;
    pathkey.pointer = b->fPath;
    return uhash_hashChars(namekey)+37*uhash_hashChars(pathkey);
}

/* INTERNAL: compares two entries */
static UBool compareEntries(const UHashKey p1, const UHashKey p2) {
    UResourceDataEntry *b1 = (UResourceDataEntry *)p1.pointer;
    UResourceDataEntry *b2 = (UResourceDataEntry *)p2.pointer;
    UHashKey name1, name2, path1, path2;
    name1.pointer = b1->fName;
    name2.pointer = b2->fName;
    path1.pointer = b1->fPath;
    path2.pointer = b2->fPath;
    return (UBool)(uhash_compareChars(name1, name2) & 
        uhash_compareChars(path1, path2));
}


/**
 *  Internal function, gets parts of locale name according 
 *  to the position of '_' character
 */
static UBool chopLocale(char *name) {
    char *i = uprv_strrchr(name, '_');

    if(i != NULL) {
        *i = '\0';
        return TRUE;
    }

    return FALSE;
}

/**
 *  Internal function
 */
static void entryIncrease(UResourceDataEntry *entry) {
    umtx_lock(&resbMutex);
    entry->fCountExisting++;
    while(entry->fParent != NULL) {
      entry = entry->fParent;
      entry->fCountExisting++;
    }
    umtx_unlock(&resbMutex);
}

/**
 *  Internal function. Tries to find a resource in given Resource 
 *  Bundle, as well as in its parents
 */
static const ResourceData *getFallbackData(const UResourceBundle* resBundle, const char* * resTag, UResourceDataEntry* *realData, Resource *res, UErrorCode *status) {
    UResourceDataEntry *resB = resBundle->fData;
    int32_t indexR = -1;
    int32_t i = 0;
    *res = RES_BOGUS;
    if(resB != NULL) {
        if(resB->fBogus == U_ZERO_ERROR) { /* if this resource is real, */
            *res = res_getTableItemByKey(&(resB->fData), resB->fData.rootRes, &indexR, resTag); /* try to get data from there */
            i++;
        }
        if(resBundle->fHasFallback == TRUE) {
            while(*res == RES_BOGUS && resB->fParent != NULL) { /* Otherwise, we'll look in parents */
                resB = resB->fParent;
                if(resB->fBogus == U_ZERO_ERROR) {
                    i++;
                    *res = res_getTableItemByKey(&(resB->fData), resB->fData.rootRes, &indexR, resTag);
                }
            }
        }

        if(*res != RES_BOGUS) { /* If the resource is found in parents, we need to adjust the error */
            if(i>1) {
                if(uprv_strcmp(resB->fName, uloc_getDefault())==0 || uprv_strcmp(resB->fName, kRootLocaleName)==0) {
                    *status = U_USING_DEFAULT_ERROR;
                } else {
                    *status = U_USING_FALLBACK_ERROR;
                }
            }
            *realData = resB;
            return (&(resB->fData));
        } else { /* If resource is not found, we need to give an error */
            *status = U_MISSING_RESOURCE_ERROR;
            return NULL;
        }
    } else {
            *status = U_MISSING_RESOURCE_ERROR;
            return NULL;
    }
}

/** INTERNAL: Initializes the cache for resources */
static void initCache(UErrorCode *status) {
    if(isMutexInited == FALSE) {
        umtx_lock(NULL);
        if(isMutexInited == FALSE) {
          umtx_init(&resbMutex);
          isMutexInited = TRUE;
        }
        umtx_unlock(NULL);
    }
    if(cache == NULL) {
        UHashtable *newCache = uhash_open(hashEntry, compareEntries, status);
        if (U_FAILURE(*status)) {
            return;
        }
        umtx_lock(&resbMutex);
        if(cache == NULL) {
            cache = newCache;
            newCache = NULL;
        }
        umtx_unlock(&resbMutex);
        if(newCache != NULL) {
            uhash_close(newCache);
        }
    }
}

/* Works just like ucnv_flushCache() */
static int32_t ures_flushCache()
{
    UResourceDataEntry *resB = NULL;
    int32_t pos = -1;
    int32_t rbDeletedNum = 0;
    const UHashElement *e;

    /*if shared data hasn't even been lazy evaluated yet
    * return 0
    */
    if (cache == NULL)
        return 0;

    /*creates an enumeration to iterate through every element in the table */
    umtx_lock(&resbMutex);
    while ((e = uhash_nextElement(cache, &pos)) != NULL)
    {
        resB = (UResourceDataEntry *) e->value;
        /* Deletes only if reference counter == 0
         * Don't worry about the children of this node.
         * Those will eventually get deleted too, if not already.
         * Don't worry about the parents of this node.
         * Those will eventually get deleted too, if not already.
         */
        if (resB->fCountExisting == 0)
        {
            rbDeletedNum++;
            uhash_removeElement(cache, e);
            if(resB->fBogus == U_ZERO_ERROR) {
                res_unload(&(resB->fData));
            }
            if(resB->fName != NULL) {
                uprv_free(resB->fName);
            }
            if(resB->fPath != NULL) {
                uprv_free(resB->fPath);
            }
            uprv_free(resB);
        }
    }
    umtx_unlock(&resbMutex);

    return rbDeletedNum;
}

UBool ures_cleanup(void)
{
    if (cache != NULL) {
        ures_flushCache();
        if (cache != NULL && uhash_count(cache) == 0) {
            uhash_close(cache);
            cache = NULL;
            umtx_destroy(&resbMutex);
        }
    }
    return (cache == NULL);
}


/** INTERNAL: sets the name (locale) of the resource bundle to given name */

static void setEntryName(UResourceDataEntry *res, char *name, UErrorCode *status) {
    if(res->fName != NULL) {
        uprv_free(res->fName);
    }
    res->fName = (char *)uprv_malloc(sizeof(char)*uprv_strlen(name)+1);
    if(res->fName == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        uprv_strcpy(res->fName, name);
    }
}

/**
 *  INTERNAL: Inits and opens an entry from a data DLL.
 */
static UResourceDataEntry *init_entry(const char *localeID, const char *path, UErrorCode *status) {
    UResourceDataEntry *r = NULL;
    UResourceDataEntry find;
    int32_t hashValue;
    char name[96];
    const char *myPath = NULL;
    char aliasName[100] = { 0 };
    int32_t aliasLen = 0;
    UBool isAlias = FALSE;
    UHashKey hashkey;

    if(U_FAILURE(*status)) {
        return NULL;
    }

    /* here we try to deduce the right locale name */
    if(localeID == NULL) { /* if localeID is NULL, we're trying to open default locale */
        uprv_strcpy(name, uloc_getDefault());
    } else if(uprv_strlen(localeID) == 0) { /* if localeID is "" then we try to open root locale */
        uprv_strcpy(name, kRootLocaleName);
    } else { /* otherwise, we'll open what we're given */
        uprv_strcpy(name, localeID);
    }

    if(path != NULL) { /* if we actually have path, we'll use it */
        if(uprv_strcmp(path, u_getDataDirectory()) != 0) { /* unless it is system default path */
            myPath = path;
        }
    }

    find.fName = name;
    find.fPath = (char *)myPath;

    /* calculate the hash value of the entry */
    hashkey.pointer = (void *)&find;
    hashValue = hashEntry(hashkey);

    /* check to see if we already have this entry */
    r = (UResourceDataEntry *)uhash_get(cache, &find);

    if(r != NULL) { /* if the entry is already in the hash table */
        r->fCountExisting++; /* we just increase it's reference count */
        *status = r->fBogus; /* and set returning status */
    } else { /* otherwise, we'll try to construct a new entry */
        UBool result = FALSE;

        r = (UResourceDataEntry *) uprv_malloc(sizeof(UResourceDataEntry));

        if(r == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        r->fCountExisting = 1;

        r->fName = NULL;
        setEntryName(r, name, status);

        r->fPath = NULL;
        if(myPath != NULL && !U_FAILURE(*status)) {
            r->fPath = (char *)uprv_malloc(sizeof(char)*uprv_strlen(myPath)+1);
            if(r->fPath == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
            } else {
                uprv_strcpy(r->fPath, myPath);
            }
        }

        r->fHashKey = hashValue;
        r->fParent = NULL;
        r->fData.data = NULL;
        r->fData.pRoot = NULL;
        r->fData.rootRes = 0;
        r->fBogus = U_ZERO_ERROR;
        
        /* this is the acutal loading - returns bool true/false */
        result = res_load(&(r->fData), r->fPath, r->fName, status);

        if (result == FALSE || U_FAILURE(*status)) { 
            /* we have no such entry in dll, so it will always use fallback */
            *status = U_USING_FALLBACK_ERROR;
            r->fBogus = U_USING_FALLBACK_ERROR;
        } else { /* if we have a regular entry */
            /* We might be able to do this a wee bit more efficiently (we could check whether the aliased data) */
            /* is already in the cache), but it's good the way it is */
            /* handle the alias by trying to get out the %%Alias tag.*/
            /* We'll try to get alias string from the bundle */
            Resource aliasres = res_getResource(&(r->fData), "%%ALIAS");
            const UChar *alias = res_getString(&(r->fData), aliasres, &aliasLen);
            if(alias != NULL && aliasLen > 0) { /* if there is actual alias - unload and load new data */
                u_UCharsToChars(alias, aliasName, u_strlen(alias)+1);
                isAlias = TRUE;
                res_unload(&(r->fData));
                result = res_load(&(r->fData), r->fPath, aliasName, status);
                if (result == FALSE || U_FAILURE(*status)) { 
                    /* we couldn't load aliased data - so we have no data */
                    *status = U_USING_FALLBACK_ERROR;
                    r->fBogus = U_USING_FALLBACK_ERROR;
                }
                setEntryName(r, aliasName, status);
            }
        }

        {
            UResourceDataEntry *oldR = NULL;
            if((oldR = (UResourceDataEntry *)uhash_get(cache, r)) == NULL) { /* if the data is not cached */
              /* just insert it in the cache */
                uhash_put(cache, (void *)r, r, status);
            } else {
              /* somebody have already inserted it while we were working, discard newly opened data */
              /* Also, we could get here IF we opened an alias */
                uprv_free(r->fName);
                if(r->fPath != NULL) {
                    uprv_free(r->fPath);
                }
                res_unload(&(r->fData));
                uprv_free(r);
                r = oldR;
                r->fCountExisting++;
            }
        }

    }
    return r;
}

/* INTERNAL: */
static UResourceDataEntry *findFirstExisting(const char* path, char* name, UBool *isRoot, UBool *hasChopped, UBool *isDefault, UErrorCode* status) {
  UResourceDataEntry *r = NULL;
  UBool hasRealData = FALSE;
  const char *defaultLoc = uloc_getDefault();
  UErrorCode intStatus = U_ZERO_ERROR;
  *hasChopped = TRUE; /* we're starting with a fresh name */

  while(*hasChopped && !hasRealData) {
    r = init_entry(name, path, &intStatus);
    *isDefault = (UBool)(uprv_strncmp(name, defaultLoc, uprv_strlen(name)) == 0);
    hasRealData = (UBool)(r->fBogus == U_ZERO_ERROR);
    if(!hasRealData) {
      entryCloseInt(r);
      r = NULL;
      *status = U_USING_FALLBACK_ERROR;
    } else {
      uprv_strcpy(name, r->fName); /* this is needed for supporting aliases */
    }

    *isRoot = (UBool)(uprv_strcmp(name, kRootLocaleName) == 0);

    /*Fallback data stuff*/
    *hasChopped = chopLocale(name);
  }
  return r;
}

static UResourceDataEntry *entryOpen(const char* path, const char* localeID, UErrorCode* status) {
    UErrorCode intStatus = U_ZERO_ERROR;
    UResourceDataEntry *r = NULL;
    UResourceDataEntry *t1 = NULL;
    UResourceDataEntry *t2 = NULL;
    UBool isDefault = FALSE;
    UBool isRoot = FALSE;
    UBool hasRealData = FALSE;
    UBool hasChopped = TRUE;
    char name[96];

    if(U_FAILURE(*status)) {
      return NULL;
    }

    initCache(status);

    uprv_strcpy(name, localeID);

    umtx_lock(&resbMutex);
    { /* umtx_lock */
      /* We're going to skip all the locales that do not have any data */
      r = findFirstExisting(path, name, &isRoot, &hasChopped, &isDefault, &intStatus);

      if(r != NULL) { /* if there is one real locale, we can look for parents. */
        t1 = r;
        hasRealData = TRUE;
        while (hasChopped && !isRoot && t1->fParent == NULL) {
            /* insert regular parents */
            t2 = init_entry(name, r->fPath, status);
            t1->fParent = t2;
            t1 = t2;
            hasChopped = chopLocale(name);
        }
      }

      /* we could have reached this point without having any real data */
      /* if that is the case, we need to chain in the default locale   */
      if(r==NULL && !isDefault && !isRoot /*&& t1->fParent == NULL*/) {
          /* insert default locale */
          uprv_strcpy(name, uloc_getDefault());
          r = findFirstExisting(path, name, &isRoot, &hasChopped, &isDefault, &intStatus);
          intStatus = U_USING_DEFAULT_ERROR;
          if(r != NULL) { /* the default locale exists */
            t1 = r;
            hasRealData = TRUE;
            isDefault = TRUE;
            while (hasChopped && t1->fParent == NULL) {
                /* insert chopped defaults */
                t2 = init_entry(name, r->fPath, status);
                t1->fParent = t2;
                t1 = t2;
                hasChopped = chopLocale(name);
            }
          } 
      }

      /* we could still have r == NULL at this point - maybe even default locale is not */
      /* present */
      if(r == NULL) {
        uprv_strcpy(name, kRootLocaleName);
        r = findFirstExisting(path, name, &isRoot, &hasChopped, &isDefault, &intStatus);
        if(r != NULL) {
          t1 = r;
          intStatus = U_USING_DEFAULT_ERROR;
          hasRealData = TRUE;
        } else { /* we don't even have the root locale */
          *status = U_MISSING_RESOURCE_ERROR;
        }
      } else if(!isRoot && uprv_strcmp(t1->fName, kRootLocaleName) != 0 && t1->fParent == NULL) {
          /* insert root locale */
          t2 = init_entry(kRootLocaleName, r->fPath, status);
          if(!hasRealData) {
            r->fBogus = U_USING_DEFAULT_ERROR;
          }
          hasRealData = (UBool)((t2->fBogus == U_ZERO_ERROR) | hasRealData);
          t1->fParent = t2;
          t1 = t2;
      }

      while(r != NULL && !isRoot && t1->fParent != NULL) {
          t1->fParent->fCountExisting++;
          t1 = t1->fParent;
          hasRealData = (UBool)((t1->fBogus == U_ZERO_ERROR) | hasRealData);
      }
    } /* umtx_lock */
    umtx_unlock(&resbMutex);

    if(U_SUCCESS(*status)) {
      *status = intStatus;
      return r;
    } else {
      return NULL;
    }
}


/**
 * Functions to create and destroy resource bundles.
 */

/* INTERNAL: */
static UResourceBundle *init_resb_result(const ResourceData *rdata, const Resource r, const char *key, UResourceDataEntry *realData, UResourceBundle *resB, UErrorCode *status) {
    if(status == NULL || U_FAILURE(*status)) {
        return resB;
    }
    if(resB == NULL) {
        resB = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
        ures_setIsStackObject(resB, FALSE);
    } else {
        if(ures_isStackObject(resB) != FALSE) {
            ures_setIsStackObject(resB, TRUE);
        }
    }
    resB->fData = realData;
    entryIncrease(resB->fData);
    resB->fHasFallback = FALSE;
    resB->fIsTopLevel = FALSE;
    resB->fIndex = -1;
    resB->fKey = key;
    resB->fVersion = NULL;
    resB->fRes = r;
    resB->fResData.data = rdata->data;
    resB->fResData.pRoot = rdata->pRoot;
    resB->fResData.rootRes = rdata->rootRes;
    resB->fSize = res_countArrayItems(&(resB->fResData), resB->fRes);
    return resB;
}

UResourceBundle *ures_copyResb(UResourceBundle *r, const UResourceBundle *original, UErrorCode *status) {
    UBool isStackObject;
    if(U_FAILURE(*status) || r == original) {
        return r;
    }
    if(original != NULL) {
        if(r == NULL) {
            isStackObject = FALSE;
            r = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
        } else {
            isStackObject = ures_isStackObject(r);
            if(U_FAILURE(*status)) {
                return r;
            }
            if(isStackObject == FALSE) {
                ures_close(r);
                r = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
            }
        }
        uprv_memcpy(r, original, sizeof(UResourceBundle));
        ures_setIsStackObject(r, isStackObject);
        if(r->fData != NULL) {
          entryIncrease(r->fData);
        }
        return r;
    } else {
        return r;
    }
}

/**
 * Functions to retrieve data from resource bundles.
 */

U_CAPI const UChar* U_EXPORT2 ures_getString(const UResourceBundle* resB, int32_t* len, UErrorCode* status) {

    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    switch(RES_GET_TYPE(resB->fRes)) {
        case RES_STRING:
            return res_getString(&(resB->fResData), resB->fRes, len);
        case RES_INT:
        case RES_INT_VECTOR:
        case RES_BINARY:
        case RES_ARRAY:
        case RES_TABLE:
        default:
            *status = U_RESOURCE_TYPE_MISMATCH;
    }

    return NULL;
}

U_CAPI const uint8_t* U_EXPORT2 ures_getBinary(const UResourceBundle* resB, int32_t* len, 
                                               UErrorCode*               status) {
  if (status==NULL || U_FAILURE(*status)) {
    return NULL;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }
  switch(RES_GET_TYPE(resB->fRes)) {
  case RES_BINARY:
    return res_getBinary(&(resB->fResData), resB->fRes, len);
  case RES_INT:
  case RES_STRING:
  case RES_INT_VECTOR:
  case RES_ARRAY:
  case RES_TABLE:
  default:
    *status = U_RESOURCE_TYPE_MISMATCH;
  }

  return NULL;
}

U_CAPI const int32_t* U_EXPORT2 ures_getIntVector(const UResourceBundle* resB, int32_t* len, 
                                                   UErrorCode*               status) {
  if (status==NULL || U_FAILURE(*status)) {
    return NULL;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }
  switch(RES_GET_TYPE(resB->fRes)) {
  case RES_INT_VECTOR:
    return res_getIntVector(&(resB->fResData), resB->fRes, len);
  case RES_INT:
  case RES_STRING:
  case RES_ARRAY:
  case RES_BINARY:
  case RES_TABLE:
  default:
    *status = U_RESOURCE_TYPE_MISMATCH;
  }

  return NULL;
}

/* this function returns a signed integer */ 
/* it performs sign extension */
U_CAPI int32_t U_EXPORT2 ures_getInt(const UResourceBundle* resB, UErrorCode *status) {
  if (status==NULL || U_FAILURE(*status)) {
    return 0xffffffff;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0xffffffff;
  }
  if(RES_GET_TYPE(resB->fRes) != RES_INT) {
    *status = U_RESOURCE_TYPE_MISMATCH;
    return 0xffffffff;
  }
  return RES_GET_INT(resB->fRes);
}

U_CAPI uint32_t U_EXPORT2 ures_getUInt(const UResourceBundle* resB, UErrorCode *status) {
  if (status==NULL || U_FAILURE(*status)) {
    return 0xffffffff;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0xffffffff;
  }
  if(RES_GET_TYPE(resB->fRes) != RES_INT) {
    *status = U_RESOURCE_TYPE_MISMATCH;
    return 0xffffffff;
  }
  return RES_GET_UINT(resB->fRes);
}


U_CAPI UResType U_EXPORT2 ures_getType(UResourceBundle *resB) {
  if(resB == NULL) {
    return RES_NONE;
  }
  return (UResType) (RES_GET_TYPE(resB->fRes));
}

U_CAPI const char * U_EXPORT2 ures_getKey(UResourceBundle *resB) {
  if(resB == NULL) {
    return NULL;
  }
  
  return(resB->fKey);
}

U_CAPI int32_t U_EXPORT2 ures_getSize(UResourceBundle *resB) {
  if(resB == NULL) {
    return 0;
  }
  
  return resB->fSize;
}

U_CAPI void U_EXPORT2 ures_resetIterator(UResourceBundle *resB){
  if(resB == NULL) {
    return;
  }
  resB->fIndex = -1;
}

U_CAPI UBool U_EXPORT2 ures_hasNext(UResourceBundle *resB) {
  if(resB == NULL) {
    return FALSE;
  }
  return (UBool)(resB->fIndex < resB->fSize-1);
}

U_CAPI const UChar* U_EXPORT2 ures_getNextString(UResourceBundle *resB, int32_t* len, const char ** key, UErrorCode *status) {
  Resource r = RES_BOGUS;
  
  if (status==NULL || U_FAILURE(*status)) {
    return NULL;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }
  
  if(resB->fIndex == resB->fSize-1) {
    *status = U_INDEX_OUTOFBOUNDS_ERROR;
  } else {
    resB->fIndex++;
    switch(RES_GET_TYPE(resB->fRes)) {
    case RES_INT:
    case RES_BINARY:
    case RES_STRING:
      return res_getString(&(resB->fResData), resB->fRes, len); 
    case RES_TABLE:
      r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, resB->fIndex, key);
      if(r == RES_BOGUS && resB->fHasFallback) {
        /* TODO: do the fallback */
      }
      return res_getString(&(resB->fResData), r, len); 
    case RES_ARRAY:
      r = res_getArrayItem(&(resB->fResData), resB->fRes, resB->fIndex);
      if(r == RES_BOGUS && resB->fHasFallback) {
        /* TODO: do the fallback */
      }
      return res_getString(&(resB->fResData), r, len);
    case RES_INT_VECTOR:
    default:
      return NULL;
      break;
    }
  }

  return NULL;
}

U_CAPI UResourceBundle* U_EXPORT2 ures_getNextResource(UResourceBundle *resB, UResourceBundle *fillIn, UErrorCode *status) {
    const char *key = NULL;
    Resource r = RES_BOGUS;

    if (status==NULL || U_FAILURE(*status)) {
            /*return NULL;*/
            return fillIn;
    }
    if(resB == NULL) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            /*return NULL;*/
            return fillIn;
    }

    if(resB->fIndex == resB->fSize-1) {
      *status = U_INDEX_OUTOFBOUNDS_ERROR;
      /*return NULL;*/
    } else {
        resB->fIndex++;
        switch(RES_GET_TYPE(resB->fRes)) {
        case RES_INT:
        case RES_BINARY:
        case RES_STRING:
            return ures_copyResb(fillIn, resB, status);
        case RES_TABLE:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, resB->fIndex, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fData, fillIn, status);
        case RES_ARRAY:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, resB->fIndex);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fData, fillIn, status);
        case RES_INT_VECTOR:
        default:
            /*return NULL;*/
            return fillIn;
        }
    }
    /*return NULL;*/
    return fillIn;
}

U_CAPI UResourceBundle* U_EXPORT2 ures_getByIndex(const UResourceBundle *resB, int32_t indexR, UResourceBundle *fillIn, UErrorCode *status) {
    const char* key = NULL;
    Resource r = RES_BOGUS;

    if (status==NULL || U_FAILURE(*status)) {
        /*return NULL;*/
        return fillIn;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        /*return NULL;*/
        return fillIn;
    }

    if(indexR >= 0 && resB->fSize > indexR) {
        switch(RES_GET_TYPE(resB->fRes)) {
        case RES_INT:
        case RES_BINARY:
        case RES_STRING:
            return ures_copyResb(fillIn, resB, status);
        case RES_TABLE:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, indexR, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fData, fillIn, status);
        case RES_ARRAY:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, indexR);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fData, fillIn, status);
        case RES_INT_VECTOR:
        default:
            /*return NULL;*/
            return fillIn;
        }
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
    }
    /*return NULL;*/
    return fillIn;
}

U_CAPI const UChar* U_EXPORT2 ures_getStringByIndex(const UResourceBundle *resB, int32_t indexS, int32_t* len, UErrorCode *status) {
    const char* key = NULL;
    Resource r = RES_BOGUS;

    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(indexS >= 0 && resB->fSize > indexS) {
        switch(RES_GET_TYPE(resB->fRes)) {
        case RES_INT:
        case RES_BINARY:
        case RES_STRING:
            return res_getString(&(resB->fResData), resB->fRes, len);
        case RES_TABLE:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, indexS, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return res_getString(&(resB->fResData), r, len);
        case RES_ARRAY:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, indexS);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return res_getString(&(resB->fResData), r, len);
        /*case RES_INT_VECTOR:*/
        /*default:*/
          /*return;*/
        }
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
    }
    return NULL;
}

U_CAPI UResourceBundle* U_EXPORT2 ures_getByKey(const UResourceBundle *resB, const char* inKey, UResourceBundle *fillIn, UErrorCode *status) {
    Resource res = RES_BOGUS;
    UResourceDataEntry *realData = NULL;
    const char *key = inKey;

    if (status==NULL || U_FAILURE(*status)) {
        return fillIn;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return fillIn;
    }

    if(RES_GET_TYPE(resB->fRes) == RES_TABLE) {
        int32_t t;
        res = res_getTableItemByKey(&(resB->fResData), resB->fRes, &t, &key);
        if(res == RES_BOGUS) {
            key = inKey;
            if(resB->fHasFallback == TRUE) {
                const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
                if(U_SUCCESS(*status)) {
                    return init_resb_result(rd, res, key, realData, fillIn, status);
                } else {
                    *status = U_MISSING_RESOURCE_ERROR;
                }
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            return init_resb_result(&(resB->fResData), res, key, resB->fData, fillIn, status);
        }
    } 
#if 0
    /* this is a kind of TODO item. If we have an array with an index table, we could do this. */
    /* not currently */
    else if(RES_GET_TYPE(resB->fRes) == RES_ARRAY && resB->fHasFallback == TRUE) {
        /* here should go a first attempt to locate the key using index table */
        const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
        if(U_SUCCESS(*status)) {
            return init_resb_result(rd, res, key, realData, fillIn, status);
        } else {
            *status = U_MISSING_RESOURCE_ERROR;
        }
    }
#endif    
    else {
        *status = U_RESOURCE_TYPE_MISMATCH;
    }
    return fillIn;
}

U_CAPI const UChar* U_EXPORT2 ures_getStringByKey(const UResourceBundle *resB, const char* inKey, int32_t* len, UErrorCode *status) {
    Resource res = RES_BOGUS;
    UResourceDataEntry *realData = NULL;
    const char* key = inKey;

    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(RES_GET_TYPE(resB->fRes) == RES_TABLE) {
        int32_t t=0;
        res = res_getTableItemByKey(&(resB->fResData), resB->fRes, &t, &key);
        if(res == RES_BOGUS) {
            key = inKey;
            if(resB->fHasFallback == TRUE) {
                const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
                if(U_SUCCESS(*status)) {
                    return res_getString(rd, res, len);
                } else {
                    *status = U_MISSING_RESOURCE_ERROR;
                }
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            return res_getString(&(resB->fResData), res, len);
        }
    } 
#if 0 
    /* this is a kind of TODO item. If we have an array with an index table, we could do this. */
    /* not currently */   
    else if(RES_GET_TYPE(resB->fRes) == RES_ARRAY && resB->fHasFallback == TRUE) {
        /* here should go a first attempt to locate the key using index table */
        const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
        if(U_SUCCESS(*status)) {
            return res_getString(rd, res, len);
        } else {
            *status = U_MISSING_RESOURCE_ERROR;
        }
    } 
#endif    
    else {
        *status = U_RESOURCE_TYPE_MISMATCH;
    }
    return NULL;
}


/* TODO: clean from here down */

/**
 *  INTERNAL: Get the name of the first real locale (not placeholder) 
 *  that has resource bundle data.
 */
U_CAPI const char* ures_getLocale(const UResourceBundle* resourceBundle, UErrorCode* status)
{
    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if (!resourceBundle) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
      return resourceBundle->fData->fName;
    }
}

static void entryCloseInt(UResourceDataEntry *resB) {
    UResourceDataEntry *p = resB;

    while(resB != NULL) {
        p = resB->fParent;
        resB->fCountExisting--;

        /* Entries are left in the cache. TODO: add ures_cacheFlush() to force a flush
         of the cache. */
/*
        if(resB->fCountExisting <= 0) {
            uhash_remove(cache, resB);
            if(resB->fBogus == U_ZERO_ERROR) {
                res_unload(&(resB->fData));
            }
            if(resB->fName != NULL) {
                uprv_free(resB->fName);
            }
            if(resB->fPath != NULL) {
                uprv_free(resB->fPath);
            }
            uprv_free(resB);
        }
*/

        resB = p;
    }
}

/** 
 *  API: closes a resource bundle and cleans up.
 */

static void entryClose(UResourceDataEntry *resB) {
  umtx_lock(&resbMutex);
  entryCloseInt(resB);
  umtx_unlock(&resbMutex);
}


U_CFUNC const char* ures_getName(const UResourceBundle* resB) {
  if(resB == NULL) {
    return NULL;
  }

  return resB->fData->fName;
}

U_CFUNC const char* ures_getPath(const UResourceBundle* resB) {
  if(resB == NULL) {
    return NULL;
  }

  return resB->fData->fPath;
}

/* OLD API implementation */

/**
 *  API: This function is used to open a resource bundle 
 *  proper fallback chaining is executed while initialization. 
 *  The result is stored in cache for later fallback search.
 */
U_CAPI void ures_openFillIn(UResourceBundle *r, const char* path,
                    const char* localeID, UErrorCode* status) {
    if(r == NULL) {
        *status = U_INTERNAL_PROGRAM_ERROR;
    } else {
        UResourceDataEntry *firstData;
        r->fHasFallback = TRUE;
        r->fIsTopLevel = TRUE;
        r->fKey = NULL;
        r->fVersion = NULL;
        r->fIndex = -1;
        r->fData = entryOpen(path, localeID, status);
        /* this is a quick fix to get regular data in bundle - until construction is cleaned up */
        firstData = r->fData;
        while(firstData->fBogus != U_ZERO_ERROR && firstData->fParent != NULL) {
            firstData = firstData->fParent;
        }
        r->fResData.data = firstData->fData.data;
        r->fResData.pRoot = firstData->fData.pRoot;
        r->fResData.rootRes = firstData->fData.rootRes;
        r->fRes = r->fResData.rootRes;
        r->fSize = res_countArrayItems(&(r->fResData), r->fRes);
    }
}
U_CAPI UResourceBundle* ures_open(const char* path,
                    const char* localeID,
                    UErrorCode* status)
{
    char canonLocaleID[100];
    UResourceDataEntry *hasData = NULL;
    UResourceBundle *r;
    int32_t length;

    if(status == NULL || U_FAILURE(*status)) {
        return NULL;
    }

    /* first "canonicalize" the locale ID */
    length = uloc_getName(localeID, canonLocaleID, sizeof(canonLocaleID), status);
    if(U_FAILURE(*status) || *status == U_STRING_NOT_TERMINATED_WARNING) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    r = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
    if(r == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    r->fHasFallback = TRUE;
    r->fIsTopLevel = TRUE;
    ures_setIsStackObject(r, FALSE);
    r->fKey = NULL;
    r->fVersion = NULL;
    r->fIndex = -1;
    r->fData = entryOpen(path, canonLocaleID, status);
    if(U_FAILURE(*status)) {
        uprv_free(r);
        return NULL;
    }

    hasData = r->fData;
    while(hasData->fBogus != U_ZERO_ERROR) {
        hasData = hasData->fParent;
        if(hasData == NULL) {
          /* This can happen only if fallback chain gets broken by an act of God */
          /* TODO: this unlikely to happen, consider removing it */
            entryClose(r->fData);
            uprv_free(r);
            *status = U_MISSING_RESOURCE_ERROR;
            return NULL;
        }
    }

    r->fResData.data = hasData->fData.data;
    r->fResData.pRoot = hasData->fData.pRoot;
    r->fResData.rootRes = hasData->fData.rootRes;
    r->fRes = r->fResData.rootRes;
    r->fSize = res_countArrayItems(&(r->fResData), r->fRes);

    return r;
}

U_CAPI UResourceBundle* ures_openW(const wchar_t* myPath,
                    const char* localeID,
                    UErrorCode* status)
{
    UResourceBundle *r;
    size_t pathSize = (uprv_wcslen(myPath) + 1) * sizeof(int32_t);
    char *path = (char *)uprv_malloc(pathSize);

    uprv_wcstombs(path, myPath, pathSize);

    /*u_UCharsToChars(myPath, path, uprv_wcslen(myPath)+1);*/

    r = ures_open(path, localeID, status);
    uprv_free(path);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    return r;
}


U_CAPI UResourceBundle* U_EXPORT2 ures_openU(const UChar* myPath, 
                  const char* localeID, 
                  UErrorCode* status)
{
    UResourceBundle *r;
    int32_t pathSize = u_strlen(myPath) + 1;
    char *path = (char *)uprv_malloc(pathSize);

    u_UCharsToChars(myPath, path, pathSize);

    r = ures_open(path, localeID, status);
    uprv_free(path);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    return r;
}

/**
 *  Opens a resource bundle without "canonicalizing" the locale name. No fallback will be performed 
 *  or sought. However, alias substitution will happen!
 */
U_CFUNC UResourceBundle* ures_openDirect(const char* path, const char* localeID, UErrorCode* status) {
    UResourceBundle *r;

    if(status == NULL || U_FAILURE(*status)) {
        return NULL;
    }

    r = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
    if(r == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    r->fHasFallback = FALSE;
    r->fIsTopLevel = TRUE;
    ures_setIsStackObject(r, FALSE);
    r->fIndex = -1;
    r->fData = entryOpen(path, localeID, status);
    if(U_FAILURE(*status)) {
        uprv_free(r);
        return NULL;
    }
    if(*status != U_ZERO_ERROR /*r->fData->fBogus != U_ZERO_ERROR*/) {
      /* we didn't find one we were looking for - so openDirect */
      /* should fail */
        entryClose(r->fData);
        uprv_free(r);
        *status = U_MISSING_RESOURCE_ERROR;
        return NULL;
    }

    r->fKey = NULL;
    r->fVersion = NULL;
    r->fResData.data = r->fData->fData.data;
    r->fResData.pRoot = r->fData->fData.pRoot;
    r->fResData.rootRes = r->fData->fData.rootRes;
    r->fRes = r->fResData.rootRes;
    r->fSize = res_countArrayItems(&(r->fResData), r->fRes);
    return r;
}

U_CFUNC void ures_setIsStackObject( UResourceBundle* resB, UBool state) {
    if(state) {
        resB->fMagic1 = 0;
        resB->fMagic2 = 0;
    } else {
        resB->fMagic1 = MAGIC1;
        resB->fMagic2 = MAGIC2;
    }
}

U_CFUNC UBool ures_isStackObject(UResourceBundle* resB) {
  return((resB->fMagic1 == MAGIC1 && resB->fMagic2 == MAGIC2)?FALSE:TRUE);
}

/**
 *  API: Counts members. For arrays and tables, returns number of resources.
 *  For strings, returns 1.
 */
U_CAPI int32_t ures_countArrayItems(const UResourceBundle* resourceBundle,
                  const char* resourceKey,
                  UErrorCode* status)
{
    UResourceBundle resData;
    ures_setIsStackObject(&resData, TRUE);
        if (status==NULL || U_FAILURE(*status)) {
                return 0;
        }
        if(resourceBundle == NULL) {
                *status = U_ILLEGAL_ARGUMENT_ERROR;
                return 0;
        }
    ures_getByKey(resourceBundle, resourceKey, &resData, status);

    if(resData.fResData.data != NULL) {
      int32_t result = res_countArrayItems(&resData.fResData, resData.fRes);
      ures_close(&resData);
      return result;
    } else {
      *status = U_MISSING_RESOURCE_ERROR;
      ures_close(&resData);
      return 0;
    }
}

U_CAPI void ures_close(UResourceBundle*    resB)
{
    if(resB != NULL) {
        if(resB->fData != NULL) {
            entryClose(resB->fData);
        }
        /*
        if(resB->fKey != NULL) {
            uprv_free(resB->fKey);
        }
        */
        if(resB->fVersion != NULL) {
            uprv_free(resB->fVersion);
        }

        if(ures_isStackObject(resB) == FALSE) {
            uprv_free(resB);
        }
    }
}

U_CAPI const char* ures_getVersionNumber(const UResourceBundle*   resourceBundle)
{
    if (!resourceBundle) return NULL;

    if(resourceBundle->fVersion == NULL) {

        /* If the version ID has not been built yet, then do so.  Retrieve */
        /* the minor version from the file. */
        UErrorCode status = U_ZERO_ERROR;
        int32_t minor_len = 0;
        int32_t len;

        const UChar* minor_version = ures_getStringByKey(resourceBundle, kVersionTag, &minor_len, &status);
    
        /* Determine the length of of the final version string.  This is */
        /* the length of the major part + the length of the separator */
        /* (==1) + the length of the minor part (+ 1 for the zero byte at */
        /* the end). */

        len = (minor_len > 0) ? minor_len : 1;
    
        /* Allocate the string, and build it up. */
        /* + 1 for zero byte */


        ((UResourceBundle *)resourceBundle)->fVersion = (char *)uprv_malloc(1 + len); 
    
        if(minor_len > 0) {
            u_UCharsToChars(minor_version, resourceBundle->fVersion , minor_len);
            resourceBundle->fVersion[len] =  '\0';
        }
        else {
          uprv_strcat(resourceBundle->fVersion, kDefaultMinorVersion);
        }
    }

    return resourceBundle->fVersion;
}

U_CAPI void U_EXPORT2 ures_getVersion(const UResourceBundle* resB, UVersionInfo versionInfo) {
    if (!resB) return;

    u_versionFromString(versionInfo, ures_getVersionNumber(resB));
}

/* eof */
