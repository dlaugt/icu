/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/* Created by weiv 05/09/2002 */

#include "datamap.h"
#include "unicode/resbund.h"
#include <stdlib.h>

int32_t 
DataMap::utoi(const UnicodeString &s) const
{
  char ch[256];
  const UChar *u = s.getBuffer();
  int32_t len = s.length();
  u_UCharsToChars(u, ch, len);
  ch[len] = 0; /* include terminating \0 */
  return atoi(ch);
}


void deleteResBund(void *obj) {
  delete (ResourceBundle *)obj;
}


RBDataMap::~RBDataMap()
{
  delete fData;
}

RBDataMap::RBDataMap()
{
  UErrorCode status = U_ZERO_ERROR;
  fData = new Hashtable(TRUE, status);
  fData->setValueDeleter(deleteResBund);
}

// init from table resource
// will put stuff in hashtable according to 
// keys.
RBDataMap::RBDataMap(UResourceBundle *data, UErrorCode &status)
{
  fData = new Hashtable(TRUE, status);
  fData->setValueDeleter(deleteResBund);
  init(data, status);
}

// init from headers and resource
// with checking the whether the size of resource matches 
// header size
RBDataMap::RBDataMap(UResourceBundle *headers, UResourceBundle *data, UErrorCode &status) 
{
  fData = new Hashtable(TRUE, status);
  fData->setValueDeleter(deleteResBund);
  init(headers, data, status);
}


void RBDataMap::init(UResourceBundle *data, UErrorCode &status) {
  int32_t i = 0;
  fData->removeAll();
  UResourceBundle *t = NULL;
  for(i = 0; i < ures_getSize(data); i++) {
    t = ures_getByIndex(data, i, t, &status);
    fData->put(UnicodeString(ures_getKey(t), ""), new ResourceBundle(t, status), status);
  }
  ures_close(t);
}

void RBDataMap::init(UResourceBundle *headers, UResourceBundle *data, UErrorCode &status)
{
  int32_t i = 0, j = 0;
  fData->removeAll();
  UResourceBundle *t = NULL;
  const UChar *key = NULL;
  int32_t keyLen = 0;
  if(ures_getSize(headers) == ures_getSize(data)) {
    for(i = 0; i < ures_getSize(data); i++) {
      t = ures_getByIndex(data, i, t, &status);
      key = ures_getStringByIndex(headers, i, &keyLen, &status);
      fData->put(UnicodeString(key, keyLen), new ResourceBundle(t, status), status);
    }
  } else {
    // error
    status = U_INVALID_FORMAT_ERROR;
  }
  ures_close(t);
}


const UnicodeString RBDataMap::getString(const char* key, UErrorCode &status) const
{
  UnicodeString hashKey(key, "");
  ResourceBundle *r = (ResourceBundle *)fData->get(hashKey);
  if(r != NULL) {
    return r->getString(status);
  } else {
    status = U_MISSING_RESOURCE_ERROR;
    return UnicodeString("", "");
  }
}


const int32_t RBDataMap::getInt(const char* key, UErrorCode &status) const
{
  int32_t result = 0;
  UnicodeString r = this->getString(key, status);
  if(U_SUCCESS(status)) {
    result = utoi(r);
  }
  return result;
}

const UnicodeString* RBDataMap::getStringArray(int32_t& count, const char* key, UErrorCode &status) const 
{
  UnicodeString hashKey(key, "");
  ResourceBundle *r = (ResourceBundle *)fData->get(hashKey);
  if(r != NULL) {
    int32_t size = r->getSize();
    int32_t i = 0;
    UnicodeString *result = new UnicodeString[size];
    for(i = 0; i<size; i++) {
      result[i] = r->getStringEx(i, status);
    }
    return result;
  } else {
    status = U_MISSING_RESOURCE_ERROR;
    return NULL;
  }
}

const int32_t* RBDataMap::getIntArray(int32_t& count, const char* key, UErrorCode &status) const
{
  UnicodeString hashKey(key, "");
  ResourceBundle *r = (ResourceBundle *)fData->get(hashKey);
  if(r != NULL) {
    int32_t size = r->getSize();
    int32_t *result = new int32_t[size];
    int32_t i = 0;
    UnicodeString stringRes;
    for(i = 0; i<size; i++) {
      stringRes = r->getStringEx(i, status);
      result[i] = utoi(stringRes);
    }
    return result;
  } else {
    status = U_MISSING_RESOURCE_ERROR;
    return NULL;
  }
}


