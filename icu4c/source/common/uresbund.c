/*
******************************************************************************
* Copyright (C) 1997-2005, International Business Machines Corporation and   *
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
*   06/24/02    weiv        Added support for resource sharing
******************************************************************************
*/

#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "uresimp.h"
#include "ustr_imp.h"
#include "cwchar.h"
#include "ucln_cmn.h"
#include "cmemory.h"
#include "cstring.h"
#include "uhash.h"
#include "unicode/uenum.h"
#include "uenumimp.h"
#include "ulocimp.h"
#include "umutex.h"
#include "putilimp.h"


/*
Static cache for already opened resource bundles - mostly for keeping fallback info
TODO: This cache should probably be removed when the deprecated code is
      completely removed.
*/
static UHashtable *cache = NULL;

static UMTX resbMutex = NULL;

/* INTERNAL: hashes an entry  */
static int32_t U_EXPORT2 U_CALLCONV hashEntry(const UHashTok parm) {
    UResourceDataEntry *b = (UResourceDataEntry *)parm.pointer;
    UHashTok namekey, pathkey;
    namekey.pointer = b->fName;
    pathkey.pointer = b->fPath;
    return uhash_hashChars(namekey)+37*uhash_hashChars(pathkey);
}

/* INTERNAL: compares two entries */
static UBool U_EXPORT2 U_CALLCONV compareEntries(const UHashTok p1, const UHashTok p2) {
    UResourceDataEntry *b1 = (UResourceDataEntry *)p1.pointer;
    UResourceDataEntry *b2 = (UResourceDataEntry *)p2.pointer;
    UHashTok name1, name2, path1, path2;
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
                    *status = U_USING_DEFAULT_WARNING;
                } else {
                    *status = U_USING_FALLBACK_WARNING;
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

/* Works just like ucnv_flushCache() */
/* TODO: figure out why fCountExisting may not go to zero. Do not make this function public yet. */
static int32_t ures_flushCache()
{
    UResourceDataEntry *resB = NULL;
    int32_t pos = -1;
    int32_t rbDeletedNum = 0;
    const UHashElement *e;

    /*if shared data hasn't even been lazy evaluated yet
    * return 0
    */
    umtx_lock(&resbMutex);
    if (cache == NULL) {
        umtx_unlock(&resbMutex);
        return 0;
    }

    /*creates an enumeration to iterate through every element in the table */
    while ((e = uhash_nextElement(cache, &pos)) != NULL)
    {
        resB = (UResourceDataEntry *) e->value.pointer;
        /* Deletes only if reference counter == 0
         * Don't worry about the children of this node.
         * Those will eventually get deleted too, if not already.
         * Don't worry about the parents of this node.
         * Those will eventually get deleted too, if not already.
         */
        /* DONE: figure out why fCountExisting may not go to zero. Do not make this function public yet. */
        /* 04/05/2002 [weiv] fCountExisting should now be accurate. If it's not zero, that means that    */
        /* some resource bundles are still open somewhere. */

        /*U_ASSERT(resB->fCountExisting == 0);*/
        if (resB->fCountExisting == 0) {
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

static UBool U_CALLCONV ures_cleanup(void)
{
    if (cache != NULL) {
        ures_flushCache();
        if (cache != NULL && uhash_count(cache) == 0) {
            uhash_close(cache);
            cache = NULL;
        }
    }
    if (cache == NULL && resbMutex != NULL) {
        umtx_destroy(&resbMutex);
    }
    return (cache == NULL);
}

/** INTERNAL: Initializes the cache for resources */
static void initCache(UErrorCode *status) {
    UBool makeCache = FALSE;
    umtx_lock(&resbMutex);
    makeCache = (cache ==  NULL);
    umtx_unlock(&resbMutex);
    if(makeCache) {
        UHashtable *newCache = uhash_open(hashEntry, compareEntries, status);
        if (U_FAILURE(*status)) {
            return;
        }
        umtx_lock(&resbMutex);
        if(cache == NULL) {
            cache = newCache;
            newCache = NULL;
            ucln_common_registerCleanup(UCLN_COMMON_URES, ures_cleanup);
        }
        umtx_unlock(&resbMutex);
        if(newCache != NULL) {
            uhash_close(newCache);
        }
    }
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
 *    CAUTION:  resbMutex must be locked when calling this function.
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
    UHashTok hashkey;

    if(U_FAILURE(*status)) {
        return NULL;
    }

    /* here we try to deduce the right locale name */
    if(localeID == NULL) { /* if localeID is NULL, we're trying to open default locale */
        uprv_strcpy(name, uloc_getDefault());
    } else if(*localeID == 0) { /* if localeID is "" then we try to open root locale */
        uprv_strcpy(name, kRootLocaleName);
    } else { /* otherwise, we'll open what we're given */
        uprv_strcpy(name, localeID);
    }

    if(path != NULL) { /* if we actually have path, we'll use it */
      myPath = path;
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
        /* if the resource has a warning */
        /* we don't want to overwrite a status with no error */
        if(r->fBogus != U_ZERO_ERROR) {
          *status = r->fBogus; /* set the returning status */
        } 
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
            *status = U_USING_FALLBACK_WARNING;
            r->fBogus = U_USING_FALLBACK_WARNING;
        } else { /* if we have a regular entry */
            /* We might be able to do this a wee bit more efficiently (we could check whether the aliased data) */
            /* is already in the cache), but it's good the way it is */
            /* handle the alias by trying to get out the %%Alias tag.*/
            /* We'll try to get alias string from the bundle */
            Resource aliasres = res_getResource(&(r->fData), "%%ALIAS");
            if (aliasres != RES_BOGUS) {
                const UChar *alias = res_getString(&(r->fData), aliasres, &aliasLen);
                if(alias != NULL && aliasLen > 0) { /* if there is actual alias - unload and load new data */
                    u_UCharsToChars(alias, aliasName, aliasLen+1);
                    isAlias = TRUE;
                    res_unload(&(r->fData));
                    result = res_load(&(r->fData), r->fPath, aliasName, status);
                    if (result == FALSE || U_FAILURE(*status)) { 
                        /* we couldn't load aliased data - so we have no data */
                        *status = U_USING_FALLBACK_WARNING;
                        r->fBogus = U_USING_FALLBACK_WARNING;
                    }
                    setEntryName(r, aliasName, status);
                }
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
/*   CAUTION:  resbMutex must be locked when calling this function! */
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
      /* this entry is not real. We will discard it. */
      /* However, the parent line for this entry is  */
      /* not to be used - as there might be parent   */
      /* lines in cache from previous openings that  */
      /* are not updated yet. */
      r->fCountExisting--;
      /*entryCloseInt(r);*/
      r = NULL;
      *status = U_USING_FALLBACK_WARNING;
    } else {
      uprv_strcpy(name, r->fName); /* this is needed for supporting aliases */
    }

    *isRoot = (UBool)(uprv_strcmp(name, kRootLocaleName) == 0);

    /*Fallback data stuff*/
    *hasChopped = chopLocale(name);
  }
  return r;
}

static void ures_setIsStackObject( UResourceBundle* resB, UBool state) {
    if(state) {
        resB->fMagic1 = 0;
        resB->fMagic2 = 0;
    } else {
        resB->fMagic1 = MAGIC1;
        resB->fMagic2 = MAGIC2;
    }
}

static UBool ures_isStackObject(const UResourceBundle* resB) {
  return((resB->fMagic1 == MAGIC1 && resB->fMagic2 == MAGIC2)?FALSE:TRUE);
}


U_CFUNC void ures_initStackObject(UResourceBundle* resB) {
  uprv_memset(resB, 0, sizeof(UResourceBundle));
  ures_setIsStackObject(resB, TRUE);
}

static UResourceDataEntry *entryOpen(const char* path, const char* localeID, UErrorCode* status) {
    UErrorCode intStatus = U_ZERO_ERROR;
    UErrorCode parentStatus = U_ZERO_ERROR;
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
            t2 = init_entry(name, r->fPath, &parentStatus);
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
          intStatus = U_USING_DEFAULT_WARNING;
          if(r != NULL) { /* the default locale exists */
            t1 = r;
            hasRealData = TRUE;
            isDefault = TRUE;
            while (hasChopped && t1->fParent == NULL) {
                /* insert chopped defaults */
                t2 = init_entry(name, r->fPath, &parentStatus);
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
          intStatus = U_USING_DEFAULT_WARNING;
          hasRealData = TRUE;
        } else { /* we don't even have the root locale */
          *status = U_MISSING_RESOURCE_ERROR;
        }
      } else if(!isRoot && uprv_strcmp(t1->fName, kRootLocaleName) != 0 && t1->fParent == NULL) {
          /* insert root locale */
          t2 = init_entry(kRootLocaleName, r->fPath, &parentStatus);
          if(!hasRealData) {
            r->fBogus = U_USING_DEFAULT_WARNING;
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
      if(U_SUCCESS(parentStatus)) {
        if(intStatus != U_ZERO_ERROR) {
          *status = intStatus;  
        }
        return r;
      } else {
        *status = parentStatus;
        return NULL;
      }
    } else {
      return NULL;
    }
}


/**
 * Functions to create and destroy resource bundles.
 *     CAUTION:  resbMutex must be locked when calling this function.
 */
/* INTERNAL: */
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

static UResourceBundle *init_resb_result(const ResourceData *rdata, Resource r, 
                                         const char *key, int32_t index, UResourceDataEntry *realData, 
                                         const UResourceBundle *parent, int32_t noAlias,
                                         UResourceBundle *resB, UErrorCode *status) 
{
    if(status == NULL || U_FAILURE(*status)) {
        return resB;
    }
    if(RES_GET_TYPE(r) == URES_ALIAS) { /* This is an alias, need to exchange with real data */
      if(noAlias < URES_MAX_ALIAS_LEVEL) { 
        int32_t len = 0;
        const UChar *alias = res_getAlias(rdata, r, &len);   
        if(len > 0) {
          /* we have an alias, now let's cut it up */
          char stackAlias[200];
          char *chAlias = NULL, *path = NULL, *locale = NULL, *keyPath = NULL;
          int32_t capacity;

          /*
           * Allocate enough space for both the char * version
           * of the alias and parent->fResPath.
           *
           * We do this so that res_findResource() can modify the path,
           * which allows us to remove redundant _res_findResource() variants
           * in uresdata.c.
           * res_findResource() now NUL-terminates each segment so that table keys
           * can always be compared with strcmp() instead of strncmp().
           * Saves code there and simplifies testing and code coverage.
           *
           * markus 2003oct17
           */
          ++len; /* count the terminating NUL */
          if(parent != NULL && parent->fResPath != NULL) {
            capacity = (int32_t)uprv_strlen(parent->fResPath) + 1;
          } else {
            capacity = 0;
          }
          if(capacity < len) {
            capacity = len;
          }
          if(capacity <= sizeof(stackAlias)) {
            capacity = sizeof(stackAlias);
            chAlias = stackAlias;
          } else {
            chAlias = (char *)uprv_malloc(capacity);
            /* test for NULL */
            if(chAlias == NULL) {
              *status = U_MEMORY_ALLOCATION_ERROR;
              return NULL;
            }
          }
          u_UCharsToChars(alias, chAlias, len);

          if(*chAlias == RES_PATH_SEPARATOR) {
              if(*(chAlias+1) == RES_PATH_SEPARATOR) {
                /* this is an XPath alias, starting with "//" */
                /* it contains the path to a resource which should be looked up */
                /* starting in parent */
                locale = parent->fData->fName; /* this is the parent's name */
                path = realData->fPath; /* we will be looking in the same package */
                keyPath = chAlias+2; 
              } else {
                /* there is a path included */
                locale = uprv_strchr(chAlias+1, RES_PATH_SEPARATOR);
                if(locale == NULL) {
                  locale = uprv_strchr(chAlias, 0); /* avoid locale == NULL to make code below work */
                } else {
                  *locale = 0;
                  locale++;
                }
                path = chAlias+1;
                if(uprv_strcmp(path, "ICUDATA") == 0) { /* want ICU data */
                  path = NULL;
                }
                keyPath = uprv_strchr(locale, RES_PATH_SEPARATOR);
                if(keyPath) {
                  *keyPath = 0;
                  keyPath++;
                }
              }
          } else {
            /* no path, start with a locale */
            locale = chAlias;
            keyPath = uprv_strchr(locale, RES_PATH_SEPARATOR);
            if(keyPath) {
              *keyPath = 0;
              keyPath++;
            }
            path = realData->fPath;
          }


          {
            /* got almost everything, let's try to open */
            /* first, open the bundle with real data */
            UResourceBundle *result = resB;
            const char* temp = NULL;
            UErrorCode intStatus = U_ZERO_ERROR;
            UResourceBundle *mainRes = ures_openDirect(path, locale, &intStatus); 
            if(U_SUCCESS(intStatus)) {
              if(keyPath == NULL) {
                /* no key path. This means that we are going to 
                 * to use the corresponding resource from
                 * another bundle
                 */
                /* first, we are going to get a corresponding parent 
                 * resource to the one we are searching.
                 */
                char *aKey = parent->fResPath;
                if(aKey) {
                  uprv_strcpy(chAlias, aKey); /* allocated large enough above */
                  aKey = chAlias;
                  r = res_findResource(&(mainRes->fResData), mainRes->fRes, &aKey, &temp);
                } else {
                  r = mainRes->fRes;
                }
                if(key) {
                  /* we need to make keyPath from parent's fResPath and
                   * current key, if there is a key associated
                   */
                  len = (int32_t)(uprv_strlen(key) + 1);
                  if(len > capacity) {
                    capacity = len;
                    if(chAlias == stackAlias) {
                      chAlias = (char *)uprv_malloc(capacity);
                    } else {
                      chAlias = (char *)uprv_realloc(chAlias, capacity);
                    }
                    if(chAlias == NULL) {
                      ures_close(mainRes);
                      *status = U_MEMORY_ALLOCATION_ERROR;
                      return NULL;
                    }
                  }
                  uprv_memcpy(chAlias, key, len);
                  aKey = chAlias;
                  r = res_findResource(&(mainRes->fResData), r, &aKey, &temp);
                } else if(index != -1) {
                /* if there is no key, but there is an index, try to get by the index */
                /* here we have either a table or an array, so get the element */
                  if(RES_GET_TYPE(r) == URES_TABLE || RES_GET_TYPE(r) == URES_TABLE32) {
                    r = res_getTableItemByIndex(&(mainRes->fResData), r, index, (const char **)&aKey);
                  } else { /* array */
                    r = res_getArrayItem(&(mainRes->fResData), r, index);
                  }
                }
                if(r != RES_BOGUS) {
                  result = init_resb_result(&(mainRes->fResData), r, key, -1, mainRes->fData, parent, noAlias+1, resB, status);
                } else {
                  *status = U_MISSING_RESOURCE_ERROR;
                  result = resB;
                }
              } else {
                /* this one is a bit trickier. 
                 * we start finding keys, but after we resolve one alias, the path might continue.
                 * Consider: 
                 *     aliastest:alias { "testtypes/anotheralias/Sequence" }
                 *     anotheralias:alias { "/ICUDATA/sh/CollationElements" }
                 * aliastest resource should finally have the sequence, not collation elements.
                 */
                result = mainRes;
                while(*keyPath && U_SUCCESS(*status)) {
                  r = res_findResource(&(result->fResData), result->fRes, &keyPath, &temp);
                  if(r == RES_BOGUS) {
                    *status = U_MISSING_RESOURCE_ERROR;
                    result = resB;
                    break;
                  }
                  resB = init_resb_result(&(result->fResData), r, key, -1, result->fData, parent, noAlias+1, resB, status);
                  result = resB;
                }
              }
            } else { /* we failed to open the resource we're aliasing to */
              *status = intStatus;
            }
            if(chAlias != stackAlias) {
              uprv_free(chAlias);
            }
            if(mainRes != result) {
              ures_close(mainRes);
            }
            return result;
          }
        } else {
          /* bad alias, should be an error */ 
          *status = U_ILLEGAL_ARGUMENT_ERROR;
          return resB;
        }
      } else {
        *status = U_TOO_MANY_ALIASES_ERROR;
        return resB;
      }
    }
    if(resB == NULL) {
        resB = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
        /* test for NULL */
        if (resB == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        ures_setIsStackObject(resB, FALSE);
        resB->fResPath = NULL;
        resB->fResPathLen = 0;
    } else {
        if(resB->fData != NULL) {
            entryClose(resB->fData);
        }
        if(resB->fVersion != NULL) {
            uprv_free(resB->fVersion);
        }
        if(ures_isStackObject(resB) != FALSE) {
            ures_initStackObject(resB);
        }
        if(parent != resB) {
            ures_freeResPath(resB);
        }
    }
    resB->fData = realData;
    entryIncrease(resB->fData);
    resB->fHasFallback = FALSE;
    resB->fIsTopLevel = FALSE;
    resB->fIndex = -1;
    resB->fKey = key;
    resB->fParentRes = parent;
    resB->fTopLevelData = parent->fTopLevelData;
    if(parent->fResPath && parent != resB) {
        ures_appendResPath(resB, parent->fResPath, parent->fResPathLen);
    }
    if(key != NULL) {
        ures_appendResPath(resB, key, (int32_t)uprv_strlen(key));
        ures_appendResPath(resB, RES_PATH_SEPARATOR_S, 1);
    } else {
        char buf[256];
        int32_t len = T_CString_integerToString(buf, index, 10);
        ures_appendResPath(resB, buf, len);
        ures_appendResPath(resB, RES_PATH_SEPARATOR_S, 1);
    }
    /* Make sure that Purify doesn't complain about uninitialized memory copies. */
    {
        int32_t unusedLen = ((resB->fResBuf == resB->fResPath) ? resB->fResPathLen : 0);
        uprv_memset(resB->fResBuf + unusedLen, 0, sizeof(resB->fResBuf) - unusedLen);
    }

    resB->fVersion = NULL;
    resB->fRes = r;
    /*resB->fParent = parent->fRes;*/
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
            /* test for NULL */
            if (r == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return NULL;
            }
        } else {
            isStackObject = ures_isStackObject(r);
            if(U_FAILURE(*status)) {
                return r;
            }
            ures_close(r);
            if(isStackObject == FALSE) {
                r = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
                /* test for NULL */
                if (r == NULL) {
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    return NULL;
                }
            }
        }
        uprv_memcpy(r, original, sizeof(UResourceBundle));
        r->fResPath = NULL;
        r->fResPathLen = 0;
        if(original->fResPath) {
          ures_appendResPath(r, original->fResPath, original->fResPathLen);
        }
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
        case URES_STRING:
            return res_getString(&(resB->fResData), resB->fRes, len);
        case URES_INT:
        case URES_INT_VECTOR:
        case URES_BINARY:
        case URES_ARRAY:
        case URES_TABLE:
        case URES_TABLE32:
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
  case URES_BINARY:
    return res_getBinary(&(resB->fResData), resB->fRes, len);
  case URES_INT:
  case URES_STRING:
  case URES_INT_VECTOR:
  case URES_ARRAY:
  case URES_TABLE:
  case URES_TABLE32:
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
  case URES_INT_VECTOR:
    return res_getIntVector(&(resB->fResData), resB->fRes, len);
  case URES_INT:
  case URES_STRING:
  case URES_ARRAY:
  case URES_BINARY:
  case URES_TABLE:
  case URES_TABLE32:
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
  if(RES_GET_TYPE(resB->fRes) != URES_INT) {
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
  if(RES_GET_TYPE(resB->fRes) != URES_INT) {
    *status = U_RESOURCE_TYPE_MISMATCH;
    return 0xffffffff;
  }
  return RES_GET_UINT(resB->fRes);
}


U_CAPI UResType U_EXPORT2 ures_getType(const UResourceBundle *resB) {
  UResType type;

  if(resB == NULL) {
    return URES_NONE;
  }
  type = (UResType) RES_GET_TYPE(resB->fRes);
  return type == URES_TABLE32 ? URES_TABLE : type;
}

U_CAPI const char * U_EXPORT2 ures_getKey(const UResourceBundle *resB) {
  if(resB == NULL) {
    return NULL;
  }
  
  return(resB->fKey);
}

U_CAPI int32_t U_EXPORT2 ures_getSize(const UResourceBundle *resB) {
  if(resB == NULL) {
    return 0;
  }
  
  return resB->fSize;
}

static const UChar* ures_getStringWithAlias(const UResourceBundle *resB, Resource r, int32_t sIndex, int32_t *len, UErrorCode *status) {
  if(RES_GET_TYPE(r) == URES_ALIAS) {
    const UChar* result = 0;
    UResourceBundle *tempRes = ures_getByIndex(resB, sIndex, NULL, status);
    result = ures_getString(tempRes, len, status);
    ures_close(tempRes);
    return result;
  } else {
    return res_getString(&(resB->fResData), r, len); 
  }
}

U_CAPI void U_EXPORT2 ures_resetIterator(UResourceBundle *resB){
  if(resB == NULL) {
    return;
  }
  resB->fIndex = -1;
}

U_CAPI UBool U_EXPORT2 ures_hasNext(const UResourceBundle *resB) {
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
    case URES_INT:
    case URES_BINARY:
    case URES_STRING:
      return res_getString(&(resB->fResData), resB->fRes, len); 
    case URES_TABLE:
    case URES_TABLE32:
      r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, resB->fIndex, key);
      if(r == RES_BOGUS && resB->fHasFallback) {
        /* TODO: do the fallback */
      }
      return ures_getStringWithAlias(resB, r, resB->fIndex, len, status);
    case URES_ARRAY:
      r = res_getArrayItem(&(resB->fResData), resB->fRes, resB->fIndex);
      if(r == RES_BOGUS && resB->fHasFallback) {
        /* TODO: do the fallback */
      }
      return ures_getStringWithAlias(resB, r, resB->fIndex, len, status);
    case URES_ALIAS:
      return ures_getStringWithAlias(resB, resB->fRes, resB->fIndex, len, status);
    case URES_INT_VECTOR:
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
        case URES_INT:
        case URES_BINARY:
        case URES_STRING:
            return ures_copyResb(fillIn, resB, status);
        case URES_TABLE:
        case URES_TABLE32:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, resB->fIndex, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fIndex, resB->fData, resB, 0, fillIn, status);
        case URES_ARRAY:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, resB->fIndex);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fIndex, resB->fData, resB, 0, fillIn, status);
        case URES_INT_VECTOR:
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
        case URES_INT:
        case URES_BINARY:
        case URES_STRING:
            return ures_copyResb(fillIn, resB, status);
        case URES_TABLE:
        case URES_TABLE32:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, indexR, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, indexR, resB->fData, resB, 0, fillIn, status);
        case URES_ARRAY:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, indexR);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return init_resb_result(&(resB->fResData), r, key, indexR, resB->fData, resB, 0, fillIn, status);
        case URES_INT_VECTOR:
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
        case URES_INT:
        case URES_BINARY:
        case URES_STRING:
            return res_getString(&(resB->fResData), resB->fRes, len);
        case URES_TABLE:
        case URES_TABLE32:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, indexS, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return ures_getStringWithAlias(resB, r, indexS, len, status);
        case URES_ARRAY:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, indexS);
            if(r == RES_BOGUS && resB->fHasFallback) {
                /* TODO: do the fallback */
            }
            return ures_getStringWithAlias(resB, r, indexS, len, status);
        case URES_ALIAS:
          return ures_getStringWithAlias(resB, resB->fRes, indexS, len, status);

        /*case URES_INT_VECTOR:*/
        /*default:*/
          /*return;*/
        }
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
    }
    return NULL;
}

/*U_CAPI const char *ures_getResPath(UResourceBundle *resB) {
  return resB->fResPath;
}*/

U_CAPI UResourceBundle* U_EXPORT2
ures_findResource(const char* path, UResourceBundle *fillIn, UErrorCode *status) 
{
  UResourceBundle *first = NULL; 
  UResourceBundle *result = fillIn;
  char *packageName = NULL;
  char *pathToResource = NULL;
  char *locale = NULL, *localeEnd = NULL;
  int32_t length;

  if(status == NULL || U_FAILURE(*status)) {
    return result;
  }

  length = (int32_t)(uprv_strlen(path)+1);
  pathToResource = (char *)uprv_malloc(length*sizeof(char));
  /* test for NULL */
  if(pathToResource == NULL) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return result;
  }
  uprv_memcpy(pathToResource, path, length);

  locale = pathToResource;
  if(*pathToResource == RES_PATH_SEPARATOR) { /* there is a path specification */
    pathToResource++;
    packageName = pathToResource;
    pathToResource = uprv_strchr(pathToResource, RES_PATH_SEPARATOR);
    if(pathToResource == NULL) {
      *status = U_ILLEGAL_ARGUMENT_ERROR;
    } else {
      *pathToResource = 0;
      locale = pathToResource+1;
    }
  }

  localeEnd = uprv_strchr(locale, RES_PATH_SEPARATOR);
  if(localeEnd != NULL) {
    *localeEnd = 0;
  }

  first = ures_open(packageName, locale, status);

  if(U_SUCCESS(*status)) {
    if(localeEnd) {
      result = ures_findSubResource(first, localeEnd+1, fillIn, status);
    } else {
      result = ures_copyResb(fillIn, first, status);
    }
    ures_close(first);
  }
  uprv_free(pathToResource);
  return result;
}

U_CAPI UResourceBundle* U_EXPORT2
ures_findSubResource(const UResourceBundle *resB, char* path, UResourceBundle *fillIn, UErrorCode *status) 
{
  Resource res = RES_BOGUS;
  UResourceBundle *result = fillIn;
  const char *key;

  if(status == NULL || U_FAILURE(*status)) {
    return result;
  }

  /* here we do looping and circular alias checking */
  /* this loop is here because aliasing is resolved on this level, not on res level */
  /* so, when we encounter an alias, it is not an aggregate resource, so we return */
  do {
    res = res_findResource(&(resB->fResData), resB->fRes, &path, &key); 
    if(res != RES_BOGUS) {
        result = init_resb_result(&(resB->fResData), res, key, -1, resB->fData, resB, 0, fillIn, status);
        resB = result;
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
        break;
    }
  } while(*path); /* there is more stuff in the path */

  return result;
}

U_CAPI UResourceBundle* U_EXPORT2 
ures_getByKeyWithFallback(const UResourceBundle *resB, 
                          const char* inKey, 
                          UResourceBundle *fillIn, 
                          UErrorCode *status) {
    Resource res = RES_BOGUS;
    /*UResourceDataEntry *realData = NULL;*/
    const char *key = inKey;
    UResourceBundle *helper = NULL;

    if (status==NULL || U_FAILURE(*status)) {
        return fillIn;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return fillIn;
    }

    if(RES_GET_TYPE(resB->fRes) == URES_TABLE || RES_GET_TYPE(resB->fRes) == URES_TABLE32) {
        int32_t t;
        res = res_getTableItemByKey(&(resB->fResData), resB->fRes, &t, &key);
        if(res == RES_BOGUS) {
            UResourceDataEntry *dataEntry = resB->fData;
            char path[256];
            char* myPath = path;

            while(res == RES_BOGUS && dataEntry->fParent != NULL) { /* Otherwise, we'll look in parents */
                dataEntry = dataEntry->fParent;
                if(dataEntry->fBogus == U_ZERO_ERROR) {
                    uprv_strncpy(path, resB->fResPath, resB->fResPathLen);
                    uprv_strcpy(path+resB->fResPathLen, inKey);
                    myPath = path;
                    key = inKey;
                    do {
                        res = res_findResource(&(dataEntry->fData), dataEntry->fData.rootRes, &myPath, &key);
                        if (RES_GET_TYPE(res) == URES_ALIAS && *myPath) {
                            /* We hit an alias, but we didn't finish following the path. */
                            helper = init_resb_result(&(dataEntry->fData), res, inKey, -1, dataEntry, resB, 0, helper, status);
                            dataEntry = helper->fData;
                        }
                    } while(*myPath); /* Continue until the whole path is consumed */
                }
            }
            /*const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);*/
            if(res != RES_BOGUS) {
              /* check if resB->fResPath gives the right name here */
                fillIn = init_resb_result(&(dataEntry->fData), res, inKey, -1, dataEntry, resB, 0, fillIn, status);
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            fillIn = init_resb_result(&(resB->fResData), res, key, -1, resB->fData, resB, 0, fillIn, status);
        }
    } 
    else {
        *status = U_RESOURCE_TYPE_MISMATCH;
    }
    ures_close(helper);
    return fillIn;
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

    if(RES_GET_TYPE(resB->fRes) == URES_TABLE || RES_GET_TYPE(resB->fRes) == URES_TABLE32) {
        int32_t t;
        res = res_getTableItemByKey(&(resB->fResData), resB->fRes, &t, &key);
        if(res == RES_BOGUS) {
            key = inKey;
            if(resB->fHasFallback == TRUE) {
                const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
                if(U_SUCCESS(*status)) {
                  /* check if resB->fResPath gives the right name here */
                    return init_resb_result(rd, res, key, -1, realData, resB, 0, fillIn, status);
                } else {
                    *status = U_MISSING_RESOURCE_ERROR;
                }
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            return init_resb_result(&(resB->fResData), res, key, -1, resB->fData, resB, 0, fillIn, status);
        }
    } 
#if 0
    /* this is a kind of TODO item. If we have an array with an index table, we could do this. */
    /* not currently */
    else if(RES_GET_TYPE(resB->fRes) == URES_ARRAY && resB->fHasFallback == TRUE) {
        /* here should go a first attempt to locate the key using index table */
        const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
        if(U_SUCCESS(*status)) {
            return init_resb_result(rd, res, key, realData, resB, fillIn, status);
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

    if(RES_GET_TYPE(resB->fRes) == URES_TABLE || RES_GET_TYPE(resB->fRes) == URES_TABLE32) {
        int32_t t=0;

        res = res_getTableItemByKey(&(resB->fResData), resB->fRes, &t, &key);

        if(res == RES_BOGUS) {
            key = inKey;
            if(resB->fHasFallback == TRUE) {
                const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
                if(U_SUCCESS(*status)) {
                    switch (RES_GET_TYPE(res)) {
                    case URES_STRING:
                    case URES_TABLE:
                    case URES_TABLE32:
                    case URES_ARRAY:
                        return res_getString(rd, res, len);
                    case URES_ALIAS:
                      {
                        const UChar* result = 0;
                        UResourceBundle *tempRes = ures_getByKey(resB, inKey, NULL, status);
                        result = ures_getString(tempRes, len, status);
                        ures_close(tempRes);
                        return result;
                      }
                    default:
                        *status = U_RESOURCE_TYPE_MISMATCH;
                    }
                } else {
                    *status = U_MISSING_RESOURCE_ERROR;
                }
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            switch (RES_GET_TYPE(res)) {
            case URES_STRING:
            case URES_TABLE:
            case URES_TABLE32:
            case URES_ARRAY:
                return res_getString(&(resB->fResData), res, len);
            case URES_ALIAS:
              {
                const UChar* result = 0;
                UResourceBundle *tempRes = ures_getByKey(resB, inKey, NULL, status);
                result = ures_getString(tempRes, len, status);
                ures_close(tempRes);
                return result;
              }
            default:
                *status = U_RESOURCE_TYPE_MISMATCH;
            }
        }
    } 
#if 0 
    /* this is a kind of TODO item. If we have an array with an index table, we could do this. */
    /* not currently */   
    else if(RES_GET_TYPE(resB->fRes) == URES_ARRAY && resB->fHasFallback == TRUE) {
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
U_CAPI const char*  U_EXPORT2
ures_getLocale(const UResourceBundle* resourceBundle, UErrorCode* status)
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

U_CAPI const char* U_EXPORT2 
ures_getLocaleByType(const UResourceBundle* resourceBundle, 
                     ULocDataLocaleType type, 
                     UErrorCode* status) {
    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if (!resourceBundle) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
        switch(type) {
        case ULOC_ACTUAL_LOCALE:
            return resourceBundle->fData->fName;
            break;
        case ULOC_VALID_LOCALE:
            return resourceBundle->fTopLevelData->fName;
            break;
        case ULOC_REQUESTED_LOCALE:
            return NULL;
            break;
        default:
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return NULL;
        }
    }
}


/*
U_CFUNC void ures_setResPath(UResourceBundle *resB, const char* toAdd) {
  if(resB->fResPath == NULL) {
    resB->fResPath = resB->fResBuf;
    *(resB->fResPath) = 0;
  } 
  resB->fResPathLen = uprv_strlen(toAdd);
  if(RES_BUFSIZE <= resB->fResPathLen+1) {
    if(resB->fResPath == resB->fResBuf) {
      resB->fResPath = (char *)uprv_malloc((resB->fResPathLen+1)*sizeof(char));
    } else {
      resB->fResPath = (char *)uprv_realloc(resB->fResPath, (resB->fResPathLen+1)*sizeof(char));
    }
  }
  uprv_strcpy(resB->fResPath, toAdd);
}
*/
U_CFUNC void ures_appendResPath(UResourceBundle *resB, const char* toAdd, int32_t lenToAdd) {
  int32_t resPathLenOrig = resB->fResPathLen;
  if(resB->fResPath == NULL) {
    resB->fResPath = resB->fResBuf;
    *(resB->fResPath) = 0;
    resB->fResPathLen = 0;
  } 
  resB->fResPathLen += lenToAdd;
  if(RES_BUFSIZE <= resB->fResPathLen+1) {
    if(resB->fResPath == resB->fResBuf) {
      resB->fResPath = (char *)uprv_malloc((resB->fResPathLen+1)*sizeof(char));
      uprv_strcpy(resB->fResPath, resB->fResBuf);
    } else {
      resB->fResPath = (char *)uprv_realloc(resB->fResPath, (resB->fResPathLen+1)*sizeof(char));
    }
  }
  uprv_strcpy(resB->fResPath + resPathLenOrig, toAdd);
}

U_CFUNC void ures_freeResPath(UResourceBundle *resB) {
  if (resB->fResPath && resB->fResPath != resB->fResBuf) {
    uprv_free(resB->fResPath);
  }
  resB->fResPath = NULL;
  resB->fResPathLen = 0;
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
U_CAPI void  U_EXPORT2
ures_openFillIn(UResourceBundle *r, const char* path,
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
        if(r->fData != NULL) {
            entryClose(r->fData);
        }
        if(r->fVersion != NULL) {
            uprv_free(r->fVersion);
        }
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
        /*r->fParent = RES_BOGUS;*/
        /*r->fResPath = NULL;*/
        r->fParentRes = NULL;
        r->fTopLevelData = r->fData;

        ures_freeResPath(r);
    }
}

U_CAPI UResourceBundle*  U_EXPORT2
ures_open(const char* path,
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
    length = uloc_getBaseName(localeID, canonLocaleID, sizeof(canonLocaleID), status);
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
    r->fParentRes = NULL;
    r->fTopLevelData = r->fData;

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
    /*r->fParent = RES_BOGUS;*/
    r->fSize = res_countArrayItems(&(r->fResData), r->fRes);
    r->fResPath = NULL;
    r->fResPathLen = 0;
    /*
    if(r->fData->fPath != NULL) {
      ures_setResPath(r, r->fData->fPath);
      ures_appendResPath(r, RES_PATH_PACKAGE_S);
      ures_appendResPath(r, r->fData->fName);
    } else {
      ures_setResPath(r, r->fData->fName);
    }
    */


    return r;
}

/**
 *  Opens a resource bundle without "canonicalizing" the locale name. No fallback will be performed 
 *  or sought. However, alias substitution will happen!
 */
U_CAPI UResourceBundle*  U_EXPORT2
ures_openDirect(const char* path, const char* localeID, UErrorCode* status) {
    UResourceBundle *r;
    UErrorCode subStatus = U_ZERO_ERROR;

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
    r->fData = entryOpen(path, localeID, &subStatus);
    if(U_FAILURE(subStatus)) {
        *status = subStatus;
        uprv_free(r);
        return NULL;
    }
    if(subStatus != U_ZERO_ERROR /*r->fData->fBogus != U_ZERO_ERROR*/) {
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
    /*r->fParent = RES_BOGUS;*/
    r->fSize = res_countArrayItems(&(r->fResData), r->fRes);
    r->fResPath = NULL;
    r->fResPathLen = 0;
    r->fParentRes = NULL;
    r->fTopLevelData = r->fData;

    return r;
}

/**
 *  API: Counts members. For arrays and tables, returns number of resources.
 *  For strings, returns 1.
 */
U_CAPI int32_t  U_EXPORT2
ures_countArrayItems(const UResourceBundle* resourceBundle,
                  const char* resourceKey,
                  UErrorCode* status)
{
    UResourceBundle resData;
    ures_initStackObject(&resData);
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

U_CAPI void  U_EXPORT2
ures_close(UResourceBundle*    resB)
{
    if(resB != NULL) {
        if(resB->fData != NULL) {
            entryClose(resB->fData);
        }
        if(resB->fVersion != NULL) {
            uprv_free(resB->fVersion);
        }
        ures_freeResPath(resB);

        if(ures_isStackObject(resB) == FALSE) {
            uprv_free(resB);
        }
        else {
#if 0 /*U_DEBUG*/
            /* poison the data */
            uprv_memset(resB, -1, sizeof(UResourceBundle));
#endif
        }
    }
}

U_CAPI const char*  U_EXPORT2
ures_getVersionNumber(const UResourceBundle*   resourceBundle)
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
          uprv_strcpy(resourceBundle->fVersion, kDefaultMinorVersion);
        }
    }

    return resourceBundle->fVersion;
}

U_CAPI void U_EXPORT2 ures_getVersion(const UResourceBundle* resB, UVersionInfo versionInfo) {
    if (!resB) return;

    u_versionFromString(versionInfo, ures_getVersionNumber(resB));
}

/** Tree support functions *******************************/
#define INDEX_LOCALE_NAME "res_index"
#define INDEX_TAG         "InstalledLocales"
#define DEFAULT_TAG       "default"

#if defined(URES_TREE_DEBUG)
#include <stdio.h>
#endif

typedef struct ULocalesContext {
    UResourceBundle installed;
    UResourceBundle curr;
} ULocalesContext;

static void U_CALLCONV
ures_loc_closeLocales(UEnumeration *enumerator) {
    ULocalesContext *ctx = (ULocalesContext *)enumerator->context;
    ures_close(&ctx->curr);
    ures_close(&ctx->installed);
    uprv_free(ctx);
    uprv_free(enumerator);
}

static int32_t U_CALLCONV
ures_loc_countLocales(UEnumeration *en, UErrorCode *status) {
    ULocalesContext *ctx = (ULocalesContext *)en->context;
    return ures_getSize(&ctx->installed);
}

static const char* U_CALLCONV 
ures_loc_nextLocale(UEnumeration* en,
                    int32_t* resultLength,
                    UErrorCode* status) {
    ULocalesContext *ctx = (ULocalesContext *)en->context;
    UResourceBundle *res = &(ctx->installed);
    UResourceBundle *k = NULL;
    const char *result = NULL;
    int32_t len = 0;
    if(ures_hasNext(res) && (k = ures_getNextResource(res, &ctx->curr, status))) {
        result = ures_getKey(k);
        len = (int32_t)uprv_strlen(result);
    }
    if (resultLength) {
        *resultLength = len;
    }
    return result;
}

static void U_CALLCONV 
ures_loc_resetLocales(UEnumeration* en, 
                      UErrorCode* status) {
    UResourceBundle *res = &((ULocalesContext *)en->context)->installed;
    ures_resetIterator(res);
}


static const UEnumeration gLocalesEnum = {
    NULL,
        NULL,
        ures_loc_closeLocales,
        ures_loc_countLocales,
        uenum_unextDefault,
        ures_loc_nextLocale,
        ures_loc_resetLocales
};


U_CAPI UEnumeration* U_EXPORT2
ures_openAvailableLocales(const char *path, UErrorCode *status)
{
    UResourceBundle *index = NULL;
    UEnumeration *en = NULL;
    ULocalesContext *myContext = NULL;
    
    if(U_FAILURE(*status)) {
        return NULL;
    }
    myContext = uprv_malloc(sizeof(ULocalesContext));
    en =  (UEnumeration *)uprv_malloc(sizeof(UEnumeration));
    if(!en || !myContext) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(en);
        uprv_free(myContext);
        return NULL;
    }
    uprv_memcpy(en, &gLocalesEnum, sizeof(UEnumeration));
    
    ures_initStackObject(&myContext->installed);
    ures_initStackObject(&myContext->curr);
    index = ures_openDirect(path, INDEX_LOCALE_NAME, status);
    ures_getByKey(index, INDEX_TAG, &myContext->installed, status);
    if(U_SUCCESS(*status)) {
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "Got %s::%s::[%s] : %s\n", 
            path, INDEX_LOCALE_NAME, INDEX_TAG, ures_getKey(&myContext->installed));
#endif
        en->context = myContext;
    } else {
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "%s open failed - %s\n", path, u_errorName(*status));
#endif
        ures_close(&myContext->installed);
        uprv_free(myContext);
        uprv_free(en);
        en = NULL;
    }
    
    ures_close(index);
    
    return en;
}

U_CAPI int32_t U_EXPORT2
ures_getFunctionalEquivalent(char *result, int32_t resultCapacity,
                             const char *path, const char *resName, const char *keyword, const char *locid,
                             UBool *isAvailable, UBool omitDefault, UErrorCode *status)
{
    char kwVal[1024] = ""; /* value of keyword 'keyword' */
    char defVal[1024] = ""; /* default value for given locale */
    char defLoc[1024] = ""; /* default value for given locale */
    char base[1024] = ""; /* base locale */
    char found[1024];
    char parent[1024];
    char full[1024] = "";
    UResourceBundle bund1, bund2;
    UResourceBundle *res = NULL;
    UErrorCode subStatus = U_ZERO_ERROR;
    int32_t length = 0;
    if(U_FAILURE(*status)) return 0;
    if(isAvailable) { 
        *isAvailable = TRUE;
    }
    uloc_getKeywordValue(locid, keyword, kwVal, 1024-1,&subStatus);
    if(!uprv_strcmp(kwVal, DEFAULT_TAG)) {
        kwVal[0]=0;
    }
    uloc_getBaseName(locid, base, 1024-1,&subStatus);
#if defined(URES_TREE_DEBUG)
    fprintf(stderr, "getFunctionalEquivalent: \"%s\" [%s=%s] in %s - %s\n", 
            locid, keyword, kwVal, base, u_errorName(subStatus));
#endif
    ures_initStackObject(&bund1);
    ures_initStackObject(&bund2);
    
    
    uprv_strcpy(parent, base);
    uprv_strcpy(found, base);
    
    if(U_FAILURE(subStatus)) {
        *status = subStatus;
        return 0;
    }
    
    do {
        subStatus = U_ZERO_ERROR;
        res = ures_open(path, parent, &subStatus);
        if(((subStatus == U_USING_FALLBACK_WARNING) ||
            (subStatus == U_USING_DEFAULT_WARNING)) && isAvailable) {
            *isAvailable = FALSE;
        }
        isAvailable = NULL; /* only want to set this the first time around */
        
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "%s;%s -> %s [%s]\n", path?path:"ICUDATA", parent, u_errorName(subStatus), ures_getLocale(res, &subStatus));
#endif
        if(U_FAILURE(subStatus)) {
            *status = subStatus;
        } else if(subStatus == U_ZERO_ERROR) {
            ures_getByKey(res,resName,&bund1, &subStatus);
            if(subStatus == U_ZERO_ERROR) {
                const UChar *defUstr;
                int32_t defLen;
                /* look for default item */
#if defined(URES_TREE_DEBUG)
                fprintf(stderr, "%s;%s : loaded default -> %s\n",
                    path?path:"ICUDATA", parent, u_errorName(subStatus));
#endif
                defUstr = ures_getStringByKey(&bund1, DEFAULT_TAG, &defLen, &subStatus);
                if(U_SUCCESS(subStatus) && defLen) {
                    u_UCharsToChars(defUstr, defVal, u_strlen(defUstr));
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "%s;%s -> default %s=%s,  %s\n", 
                        path?path:"ICUDATA", parent, keyword, defVal, u_errorName(subStatus));
#endif
                    uprv_strcpy(defLoc, parent);
                    if(kwVal[0]==0) {
                        uprv_strcpy(kwVal, defVal);
#if defined(URES_TREE_DEBUG)
                        fprintf(stderr, "%s;%s -> kwVal =  %s\n", 
                            path?path:"ICUDATA", parent, keyword, kwVal);
#endif
                    }
                }
            }
        }
        
        subStatus = U_ZERO_ERROR;
        
        uprv_strcpy(found, parent);
        uloc_getParent(found,parent,1023,&subStatus);
        ures_close(res);
    } while(!defVal[0] && *found && U_SUCCESS(*status));
    
    /* Now, see if we can find the kwVal collator.. start the search over.. */
    uprv_strcpy(parent, base);
    uprv_strcpy(found, base);
    
    do {
        subStatus = U_ZERO_ERROR;
        res = ures_open(path, parent, &subStatus);
        if((subStatus == U_USING_FALLBACK_WARNING) && isAvailable) {
            *isAvailable = FALSE;
        }
        isAvailable = NULL; /* only want to set this the first time around */
        
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "%s;%s -> %s (looking for %s)\n", 
            path?path:"ICUDATA", parent, u_errorName(subStatus), kwVal);
#endif
        if(U_FAILURE(subStatus)) {
            *status = subStatus;
        } else if(subStatus == U_ZERO_ERROR) {
            ures_getByKey(res,resName,&bund1, &subStatus);
#if defined(URES_TREE_DEBUG)
/**/ fprintf(stderr,"@%d [%s] %s\n", __LINE__, resName, u_errorName(subStatus));
#endif
            if(subStatus == U_ZERO_ERROR) {
                ures_getByKey(&bund1, kwVal, &bund2, &subStatus);
#if defined(URES_TREE_DEBUG)
/**/ fprintf(stderr,"@%d [%s] %s\n", __LINE__, kwVal, u_errorName(subStatus));
#endif
                if(subStatus == U_ZERO_ERROR) {
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "%s;%s -> full0 %s=%s,  %s\n", 
                        path?path:"ICUDATA", parent, keyword, kwVal, u_errorName(subStatus));
#endif
                    uprv_strcpy(full, parent);
                    if(*full == 0) {
                        uprv_strcpy(full, "root");
                    }
                        /* now, recalculate default kw if need be */
                        if(uprv_strlen(defLoc) > uprv_strlen(full)) {
                          const UChar *defUstr;
                          int32_t defLen;
                          /* look for default item */
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> recalculating Default0\n", 
                                    path?path:"ICUDATA", full);
#endif
                          defUstr = ures_getStringByKey(&bund1, DEFAULT_TAG, &defLen, &subStatus);
                          if(U_SUCCESS(subStatus) && defLen) {
                            u_UCharsToChars(defUstr, defVal, u_strlen(defUstr));
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> default0 %s=%s,  %s\n", 
                                    path?path:"ICUDATA", full, keyword, defVal, u_errorName(subStatus));
#endif
                            uprv_strcpy(defLoc, full);
                          }
                        } /* end of recalculate default KW */
#if defined(URES_TREE_DEBUG)
                        else {
                          fprintf(stderr, "No trim0,  %s <= %s\n", defLoc, full);
                        }
#endif
                } else {
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "err=%s in %s looking for %s\n", 
                        u_errorName(subStatus), parent, kwVal);
#endif
                }
            }
        }
        
        subStatus = U_ZERO_ERROR;
        
        uprv_strcpy(found, parent);
        uloc_getParent(found,parent,1023,&subStatus);
        ures_close(res);
    } while(!full[0] && *found && U_SUCCESS(*status));
    
    if((full[0]==0) && uprv_strcmp(kwVal, defVal)) {
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "Failed to locate kw %s - try default %s\n", kwVal, defVal);
#endif
        uprv_strcpy(kwVal, defVal);
        uprv_strcpy(parent, base);
        uprv_strcpy(found, base);
        
        do { /* search for 'default' named item */
            subStatus = U_ZERO_ERROR;
            res = ures_open(path, parent, &subStatus);
            if((subStatus == U_USING_FALLBACK_WARNING) && isAvailable) {
                *isAvailable = FALSE;
            }
            isAvailable = NULL; /* only want to set this the first time around */
            
#if defined(URES_TREE_DEBUG)
            fprintf(stderr, "%s;%s -> %s (looking for default %s)\n",
                path?path:"ICUDATA", parent, u_errorName(subStatus), kwVal);
#endif
            if(U_FAILURE(subStatus)) {
                *status = subStatus;
            } else if(subStatus == U_ZERO_ERROR) {
                ures_getByKey(res,resName,&bund1, &subStatus);
                if(subStatus == U_ZERO_ERROR) {
                    ures_getByKey(&bund1, kwVal, &bund2, &subStatus);
                    if(subStatus == U_ZERO_ERROR) {
#if defined(URES_TREE_DEBUG)
                        fprintf(stderr, "%s;%s -> full1 %s=%s,  %s\n", path?path:"ICUDATA",
                            parent, keyword, kwVal, u_errorName(subStatus));
#endif
                        uprv_strcpy(full, parent);
                        if(*full == 0) {
                            uprv_strcpy(full, "root");
                        }
                        
                        /* now, recalculate default kw if need be */
                        if(uprv_strlen(defLoc) > uprv_strlen(full)) {
                          const UChar *defUstr;
                          int32_t defLen;
                          /* look for default item */
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> recalculating Default1\n", 
                                    path?path:"ICUDATA", full);
#endif
                          defUstr = ures_getStringByKey(&bund1, DEFAULT_TAG, &defLen, &subStatus);
                          if(U_SUCCESS(subStatus) && defLen) {
                            u_UCharsToChars(defUstr, defVal, u_strlen(defUstr));
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> default %s=%s,  %s\n", 
                                    path?path:"ICUDATA", full, keyword, defVal, u_errorName(subStatus));
#endif
                            uprv_strcpy(defLoc, full);
                          }
                        } /* end of recalculate default KW */
#if defined(URES_TREE_DEBUG)
                        else {
                          fprintf(stderr, "No trim1,  %s <= %s\n", defLoc, full);
                        }
#endif
                    }
                }
            }
            subStatus = U_ZERO_ERROR;
            
            uprv_strcpy(found, parent);
            uloc_getParent(found,parent,1023,&subStatus);
            ures_close(res);
        } while(!full[0] && *found && U_SUCCESS(*status));
    }
    
    if(U_SUCCESS(*status)) {
        if(!full[0]) {
#if defined(URES_TREE_DEBUG)
          fprintf(stderr, "Still could not load keyword %s=%s\n", keyword, kwVal);
#endif
          *status = U_MISSING_RESOURCE_ERROR;
        } else if(omitDefault) {
#if defined(URES_TREE_DEBUG)
          fprintf(stderr,"Trim? full=%s, defLoc=%s, found=%s\n", full, defLoc, found);
#endif        
          if(uprv_strlen(defLoc) <= uprv_strlen(full)) {
            /* found the keyword in a *child* of where the default tag was present. */
            if(!uprv_strcmp(kwVal, defVal)) { /* if the requested kw is default, */
              /* and the default is in or in an ancestor of the current locale */
#if defined(URES_TREE_DEBUG)
              fprintf(stderr, "Removing unneeded var %s=%s\n", keyword, kwVal);
#endif
              kwVal[0]=0;
            }
          }
        }
        uprv_strcpy(found, full);
        if(kwVal[0]) {
            uprv_strcat(found, "@");
            uprv_strcat(found, keyword);
            uprv_strcat(found, "=");
            uprv_strcat(found, kwVal);
        } else if(!omitDefault) {
            uprv_strcat(found, "@");
            uprv_strcat(found, keyword);
            uprv_strcat(found, "=");
            uprv_strcat(found, defVal);
        }
    }
    /* we found the default locale - no need to repeat it.*/
    
    ures_close(&bund1);
    ures_close(&bund2);
    
    length = (int32_t)uprv_strlen(found);

    if(U_SUCCESS(*status)) {
        int32_t copyLength = uprv_min(length, resultCapacity);
        if(copyLength>0) {
            uprv_strncpy(result, found, copyLength);
        }
        if(length == 0) {
          *status = U_MISSING_RESOURCE_ERROR; 
        }
    } else {
        length = 0;
        result[0]=0;
    }
    return u_terminateChars(result, resultCapacity, length, status);
}

U_CAPI UEnumeration* U_EXPORT2
ures_getKeywordValues(const char *path, const char *keyword, UErrorCode *status)
{
#define VALUES_BUF_SIZE 2048
#define VALUES_LIST_SIZE 512
    
    char       valuesBuf[VALUES_BUF_SIZE];
    int32_t    valuesIndex = 0;
    const char *valuesList[VALUES_LIST_SIZE];
    int32_t    valuesCount = 0;
    
    const char *locale;
    int32_t     locLen;
    
    UEnumeration *locs = NULL;
    
    UResourceBundle    item;
    UResourceBundle    subItem;
    
    ures_initStackObject(&item);
    ures_initStackObject(&subItem);
    locs = ures_openAvailableLocales(path, status);
    
    if(U_FAILURE(*status)) {
      ures_close(&item);
      ures_close(&subItem);
        return NULL;
    }
    
    valuesBuf[0]=0;
    valuesBuf[1]=0;
    
    while((locale = uenum_next(locs, &locLen, status))) {
        UResourceBundle   *bund = NULL;
        UResourceBundle   *subPtr = NULL;
        UErrorCode subStatus = U_ZERO_ERROR; /* don't fail if a bundle is unopenable */
        bund = ures_openDirect(path, locale, &subStatus);
        
#if defined(URES_TREE_DEBUG)
        if(!bund || U_FAILURE(subStatus)) {
            fprintf(stderr, "%s-%s values: Can't open %s locale - skipping. (%s)\n", 
                path?path:"<ICUDATA>", keyword, locale, u_errorName(subStatus));
        }
#endif
        
        ures_getByKey(bund, keyword, &item, &subStatus);
        
        if(!bund || U_FAILURE(subStatus)) {
#if defined(URES_TREE_DEBUG)
            fprintf(stderr, "%s-%s values: Can't find in %s - skipping. (%s)\n", 
                path?path:"<ICUDATA>", keyword, locale, u_errorName(subStatus));
#endif
            ures_close(bund);
            bund = NULL;
            continue;
        }
        
        while((subPtr = ures_getNextResource(&item,&subItem,&subStatus))
            && U_SUCCESS(subStatus)) {
            const char *k;
            int32_t i;
            k = ures_getKey(&subItem);
            
#if defined(URES_TREE_DEBUG)
            /* fprintf(stderr, "%s | %s | %s | %s\n", path?path:"<ICUDATA>", keyword, locale, k); */
#endif
            for(i=0;k&&i<valuesCount;i++) {
                if(!uprv_strcmp(valuesList[i],k)) {
                    k = NULL; /* found duplicate */
                }
            }
            if(k && *k) {
                int32_t kLen = (int32_t)uprv_strlen(k);
                if(!uprv_strcmp(k,DEFAULT_TAG)) {
                    continue; /* don't need 'default'. */
                }
                if((valuesCount >= (VALUES_LIST_SIZE-1)) ||       /* no more space in list .. */
                    ((valuesIndex+kLen+1+1) >= VALUES_BUF_SIZE)) { /* no more space in buffer (string + 2 nulls) */
                    *status = U_ILLEGAL_ARGUMENT_ERROR; /* out of space.. */
                } else {
                    uprv_strcpy(valuesBuf+valuesIndex, k);
                    valuesList[valuesCount++] = valuesBuf+valuesIndex;
                    valuesIndex += kLen;
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "%s | %s | %s | [%s]   (UNIQUE)\n",
                        path?path:"<ICUDATA>", keyword, locale, k);
#endif
                    valuesBuf[valuesIndex++] = 0; /* terminate */
                }
            }
        }
        ures_close(bund);
    }
    valuesBuf[valuesIndex++] = 0; /* terminate */
    
    ures_close(&item);
    ures_close(&subItem);
    uenum_close(locs);
#if defined(URES_TREE_DEBUG)
    fprintf(stderr, "%s:  size %d, #%d\n", u_errorName(*status), 
        valuesIndex, valuesCount);
#endif
    return uloc_openKeywordList(valuesBuf, valuesIndex, status);
}

/* eof */
