/*
******************************************************************************
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
******************************************************************************
*
* File ucoleitr.cpp
*
* Modification History:
*
* Date        Name        Description
* 02/15/2001  synwee      Modified all methods to process its own function 
*                         instead of calling the equivalent c++ api (coleitr.h)
******************************************************************************/

#include "unicode/ucoleitr.h"
#include "unicode/ustring.h"
#include "unicode/sortkey.h"
#include "ucolimp.h"
#include "cmemory.h"

#define BUFFER_LENGTH 100

typedef struct collIterate collIterator;

/* public methods ---------------------------------------------------- */

/**
* Since this is going to be deprecated, I'll leave it as it is
*/
U_CAPI int32_t
ucol_keyHashCode(const uint8_t *key, 
                       int32_t  length)
{
  CollationKey newKey(key, length);
  return newKey.hashCode();
}


UCollationElements*
ucol_openElements(const UCollator  *coll,
                  const UChar      *text,
                        int32_t    textLength,
                        UErrorCode *status)
{
  UCollationElements *result;

  if (U_FAILURE(*status))
    return NULL;

  result = (UCollationElements *)uprv_malloc(sizeof(UCollationElements));

  result->collator_ = coll;
  
  /* gets the correct length of the null-terminated string */
  if (textLength == -1)
    textLength = u_strlen(text);

  result->length_ = textLength;
  init_collIterate(text, textLength, &result->iteratordata_, FALSE);

  return result;
}

U_CAPI void
ucol_closeElements(UCollationElements *elems)
{
  collIterate *ci = &elems->iteratordata_;
  if (ci->writableBuffer != ci->stackWritableBuffer)
    uprv_free(ci->writableBuffer);
  if (elems->iteratordata_.isWritable && elems->iteratordata_.string != NULL)
    uprv_free(elems->iteratordata_.string);
  uprv_free(elems);
}

U_CAPI void
ucol_reset(UCollationElements *elems)
{
  collIterate *ci = &(elems->iteratordata_);
  ci->pos         = ci->string;
  ci->len         = ci->string + elems->length_;
  ci->CEpos       = ci->toReturn = ci->CEs;
  /*
  problem here, that means we'll have to keep calculating the new thai set
  whenever we reset. maybe getSpecialCE should just do up the whole string
  instead of only a substring of it.
  */
  ci->isThai      = TRUE;
  if (ci->stackWritableBuffer != ci->writableBuffer)
  {
    uprv_free(ci->writableBuffer);
    ci->writableBuffer = ci->stackWritableBuffer;
  }
}

U_CAPI int32_t
ucol_next(UCollationElements *elems,
          UErrorCode         *status)
{
  if (U_FAILURE(*status)) 
    return UCOL_NULLORDER;

  int32_t result;
  UCOL_GETNEXTCE(result, elems->collator_, elems->iteratordata_, status);
  /*
  if ((elems->iteratordata_).CEpos > (elems->iteratordata_).toReturn) 
  {                       
    result = *((elems->iteratordata_).toReturn++);                                      
    if ((elems->iteratordata_).CEpos == (elems->iteratordata_).toReturn)
      (elems->iteratordata_).CEpos = (elems->iteratordata_).toReturn = 
      (elems->iteratordata_).CEs; 
  } 
  else 
    if ((elems->iteratordata_).pos < (elems->iteratordata_).len) 
    {                        
      UChar ch = *(elems->iteratordata_).pos++;     
      if (ch <= 0xFF)
        (result) = (elems->collator_)->latinOneMapping[ch];                                          
      else
        (result) = ucmp32_get((elems->collator_)->mapping, ch);                                      
                                                                                    
      if((result) >= UCOL_NOT_FOUND) 
      {
        (result) = getSpecialCE((elems->collator_), (result), 
                                &(elems->iteratordata_), (status));        
        if ((result) == UCOL_NOT_FOUND)
          (result) = ucol_getNextUCA(ch, &(elems->iteratordata_), (status));                                                                            
      }                                                                               
    } 
    else
      (result) = UCOL_NO_MORE_CES;                                                     
  */
    
  if (result == UCOL_NO_MORE_CES)
    result = UCOL_NULLORDER;
  return result;
}

U_CAPI int32_t
ucol_previous(UCollationElements *elems,
              UErrorCode         *status)
{
  if(U_FAILURE(*status)) 
    return UCOL_NULLORDER;

  int32_t result;
  UCOL_GETPREVCE(result, elems->collator_, elems->iteratordata_, 
                 elems->length_, status);

  /* synwee : to be removed, only for testing 
  const UCollator   *coll  = elems->collator_;
        collIterate *data  = &(elems->iteratordata_);
        int32_t     length = elems->length_;

  if (data->CEpos > data->CEs) 
  {              
    data->toReturn --;
    (result) = *(data->toReturn);                                           
    if (data->CEs == data->toReturn)                                
      data->CEpos = data->toReturn = data->CEs; 
  }                                                                          
  else 
  {                    
    /* pointers are always at the next position to be retrieved for getnextce 
    for every first previous step after a next, value returned will the same 
    as the last next value
    */
    /*if (data->len - data->pos == length)
      (result) = UCOL_NO_MORE_CES;                                                                                                                    
    else 
    {                  
      if (data->pos != data->writableBuffer)
        data->pos --;                                 
      else 
      {                                                                 
        data->pos = data->string +                                             
                            (length - (data->len - data->writableBuffer));     
        data->len = data->string + length;                                     
        data->isThai = TRUE;                                                  
      }                

      UChar ch = *(data->pos);
      if (ch <= 0xFF)                                                
        (result) = (coll)->latinOneMapping[ch];                                                                                       
      else
        (result) = ucmp32_get((coll)->mapping, ch);                           
                                                                       
      if ((result) >= UCOL_NOT_FOUND) 
      {
        (result) = getSpecialPrevCE(coll, result, data, length, status);      
        if ((result) == UCOL_NOT_FOUND)
          (result) = ucol_getPrevUCA(ch, data, length, status);                                      
      }                                                                      
    }                                                                        
  }   */

  if (result == UCOL_NO_MORE_CES)
    result = UCOL_NULLORDER;

  return result;
}

U_CAPI int32_t
ucol_getMaxExpansion(const UCollationElements *elems,
                           int32_t            order)
{
  /* 
  synwee : requested this implementation from vladimir, need discussion. so 
  hang on.
  */
  /* return ((CollationElementIterator*)elems)->getMaxExpansion(order); */
  return -1;
}

U_CAPI void
ucol_setText(      UCollationElements *elems,
             const UChar              *text,
                   int32_t            textLength,
                   UErrorCode         *status)
{
  if (U_FAILURE(*status)) 
    return;
  
  /* gets the correct length of the null-terminated string */
  if (textLength == -1)
    textLength = u_strlen(text);

  elems->length_ = textLength;

  if (elems->iteratordata_.isWritable && elems->iteratordata_.string != NULL)
    uprv_free(elems->iteratordata_.string);
  init_collIterate(text, textLength, &elems->iteratordata_, FALSE);
}

U_CAPI UTextOffset
ucol_getOffset(const UCollationElements *elems)
{
  /* return ((CollationElementIterator*)elems)->getOffset(); */
  const collIterate *ci = &(elems->iteratordata_);
  if (ci->isThai == TRUE)
    return ci->pos - ci->string;

  /* 
  if it is a thai string with reversed elements, since getNextCE does not 
  store only a substring in writeablebuffer, we'll have to do some calculation
  to get the offset out.
  need discussion to see if it is a better idea to store the whole string 
  instead.
  */
  return elems->length_ - (ci->len - ci->pos);
}

U_CAPI void
ucol_setOffset(UCollationElements    *elems,
               UTextOffset           offset,
               UErrorCode            *status)
{
  if (U_FAILURE(*status)) 
    return;

  collIterate *ci = &(elems->iteratordata_);
  ci->pos         = ci->string + offset;
  ci->CEpos       = ci->toReturn = ci->CEs;
  /*
  problem here, that means we'll have to keep calculating the new thai set
  whenever we reset. maybe getSpecialCE should just do up the whole string
  instead of only a substring of it.
  */
  ci->isThai      = TRUE;
  if (ci->stackWritableBuffer != ci->writableBuffer)
  {
    uprv_free(ci->writableBuffer);
    ci->writableBuffer = ci->stackWritableBuffer;
  }
}





