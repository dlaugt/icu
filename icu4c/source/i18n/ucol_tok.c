/*
*******************************************************************************
*
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_tok.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
* This module reads a tailoring rule string and produces a list of 
* tokens that will be turned into collation elements
* 
*/

#include "unicode/ustring.h"
#include "unicode/uchar.h"
 
#include "cmemory.h"
#include "ucol_tok.h"
#include "uhash.h"
#include "ucmp32.h"

static UHashtable *uchars2tokens = 0;

static const UChar *rulesToParse = 0;

/* will use a small structure, tokHash */

int32_t
uhash_hashTokens(const void *k) {
  int32_t hash = 0;
  if (k != NULL) {
      const UColToken *key = (const UColToken *)k;
      int32_t len = (key->source & 0xFF000000)>>24;
      int32_t inc = ((len - 32) / 32) + 1;

      const UChar *p = (key->source & 0x00FFFFFF) + rulesToParse;
      const UChar *limit = p + len;    

      while (p<limit) {
          hash = (hash * 37) + *p;
          p += inc;
      }

      if((len = ((key->expansion & 0xFF000000)>>24)) != 0) {
        p = (key->expansion & 0x00FFFFFF) + rulesToParse;
        limit = p + len;    
        while (p<limit) {
            hash = (hash * 37) + *p;
            p += inc;
        }
      }
  }
  return hash;
}

UBool uhash_compareTokens(const void *key1, const void *key2) {
    const UColToken *p1 = (const UColToken*) key1;
    const UColToken *p2 = (const UColToken*) key2;
    const UChar *s1 = (p1->source & 0x00FFFFFF) + rulesToParse;
    const UChar *s2 = (p2->source & 0x00FFFFFF) + rulesToParse;
    uint32_t s1L = ((p1->source & 0xFF000000) >> 24);
    uint32_t s2L = ((p2->source & 0xFF000000) >> 24);

    if (p1 == p2) {
        return TRUE;
    }
    if (p1 == NULL || p2 == NULL) {
        return FALSE;
    }
    if(p1->source == p2->source && p1->expansion == p2->expansion) {
      return TRUE;
    }
    if(s1L != s2L) {
      return FALSE;
    }
    while(s1 < s1+s1L-1 && *s1 == *s2) {
      ++s1;
      ++s2;
    }
    if(*s1 == *s2) {
      s1 = (p1->expansion & 0x00FFFFFF) + rulesToParse;
      s2 = (p2->expansion & 0x00FFFFFF) + rulesToParse;
      s1L = ((p1->expansion & 0xFF000000) >> 24);
      s2L = ((p2->expansion & 0xFF000000) >> 24);
      if(s1L != s2L) {
        return FALSE;
      }
      if(s1L != 0) {
        while(s1 < s1+s1L-1 && *s1 == *s2) {
          ++s1;
          ++s2;
        }
        return (UBool)(*s1 == *s2);
      } else {
        return TRUE;
      }
    } else {
      return FALSE;
    }
}

void deleteToken(void *token) {
    UColToken *tok = (UColToken *)token;
    uprv_free(tok);
}

void ucol_tok_initTokenList(UColTokenParser *src, UErrorCode *status) {
  if(U_FAILURE(*status)) {
    return;
  }
  rulesToParse = src->source;
  src->lh = (UColTokListHeader *)uprv_malloc(256*sizeof(UColTokListHeader));
  src->resultLen = 0;
  uchars2tokens = uhash_open(uhash_hashTokens, uhash_compareTokens, status);
  uhash_setValueDeleter(uchars2tokens, deleteToken);
}

/* -1 off, 1 on, 0 neither */
int32_t ucol_uprv_tok_isOnorOf(const UChar* onoff) {
  if(onoff) {
     if(u_tolower(*onoff) == 0x006F /*'o'*/) {
       onoff++;
       if(u_tolower(*onoff) == 0x006e /*'n'*/) {
         return 1;
       } else if(u_tolower(*onoff) == 0x0066 /*'f'*/) {
         onoff++;
         if(u_tolower(*onoff) == 0x0066 /*'f'*/) {
           return -1;
         }
       }
     }
  }
  return 0;
}

void ucol_uprv_tok_setOptionInImage(UCATableHeader *image, UColAttribute attrib, UColAttributeValue value) {
  switch(attrib) {
  case UCOL_FRENCH_COLLATION:
    image->frenchCollation = value;
    break;
  case UCOL_ALTERNATE_HANDLING:
    image->alternateHandling = value;
    break;
  case UCOL_CASE_FIRST:
    image->caseFirst = value;
    break;
  case UCOL_CASE_LEVEL:
    image->caseLevel = value;
    break;
  case UCOL_NORMALIZATION_MODE:
    image->normalizationMode = value;
    break;
  case UCOL_STRENGTH:
    image->strength = value;
    break;
  case UCOL_ATTRIBUTE_COUNT:
  default:
    break;
  }
}

#define UTOK_OPTION_COUNT 12

static didInit = FALSE;
/* we can be strict, or we can be lenient */
/* I'd surely be lenient with the option arguments */
/* maybe even with options */
U_STRING_DECL(suboption_00, "non-ignorable", 13);
U_STRING_DECL(suboption_01, "shifted",        7);

U_STRING_DECL(suboption_02, "lower",          5);
U_STRING_DECL(suboption_03, "upper",          5);
U_STRING_DECL(suboption_04, "off",            3);
U_STRING_DECL(suboption_05, "on",             2);
U_STRING_DECL(suboption_06, "2",              1);



U_STRING_DECL(option_00,    "undefined",      9);
U_STRING_DECL(option_01,    "rearrange",      9);  
U_STRING_DECL(option_02,    "alternate",      9);
U_STRING_DECL(option_03,    "backwards",      9);  
U_STRING_DECL(option_04,    "variable top",  12); 
U_STRING_DECL(option_05,    "top",            3);  
U_STRING_DECL(option_06,    "normalization", 13); 
U_STRING_DECL(option_07,    "caseLevel",      9);  
U_STRING_DECL(option_08,    "caseFirst",      9); 
U_STRING_DECL(option_09,    "scriptOrder",   11);  
U_STRING_DECL(option_10,    "charsetname",   11); 
U_STRING_DECL(option_11,    "charset",        7);  

ucolTokSuboption alternateSub[2] = {
  {suboption_00, 13, UCOL_NON_IGNORABLE},
  {suboption_01,  7, UCOL_SHIFTED}
};

ucolTokSuboption caseFirstSub[3] = {
  {suboption_02, 13, UCOL_LOWER_FIRST},
  {suboption_03,  7, UCOL_UPPER_FIRST},
  {suboption_04,  2, UCOL_OFF},
};

ucolTokSuboption onOffSub[2] = {
  {suboption_04, 3, UCOL_OFF},
  {suboption_05, 2, UCOL_ON}
};

ucolTokSuboption frenchSub[1] = {
  {suboption_06, 1, UCOL_ON}
};



ucolTokOption rulesOptions[UTOK_OPTION_COUNT] = {
 {option_02,  9, alternateSub, 2, UCOL_ALTERNATE_HANDLING}, /*"alternate"      */   
 {option_03,  9, frenchSub, 1, UCOL_FRENCH_COLLATION}, /*"backwards"      */   
 {option_07,  9, onOffSub, 2, UCOL_CASE_LEVEL}, /*"caseLevel"      */   
 {option_08,  9, caseFirstSub, 3, UCOL_CASE_FIRST}, /*"caseFirst"      */   
 {option_06, 13, onOffSub, 2, UCOL_NORMALIZATION_MODE}, /*"normalization"  */
 {option_04, 12, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"variable top"   */
 {option_01,  9, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"rearrange"      */   
 {option_05,  3, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"top"            */         
 {option_00,  9, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"undefined"      */
 {option_09, 11, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"scriptOrder"    */ 
 {option_10, 11, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"charsetname"    */ 
 {option_11,  7, NULL, 0, UCOL_ATTRIBUTE_COUNT}  /*"charset"        */     
};

UBool ucol_uprv_tok_readAndSetOption(UCATableHeader *image, const UChar* start, const UChar *end, UBool *variableTop, UBool *top, UErrorCode *status) {
  uint32_t i = 0;
  int32_t j=0;
  int32_t onOff = 0;
  UBool foundOption = FALSE;
  const UChar *optionArg = NULL;
  if(!didInit) {
    U_STRING_INIT(suboption_00, "non-ignorable", 13);
    U_STRING_INIT(suboption_01, "shifted",        7);

    U_STRING_INIT(suboption_02, "lower",          5);
    U_STRING_INIT(suboption_03, "upper",          5);
    U_STRING_INIT(suboption_04, "off",            3);
    U_STRING_INIT(suboption_05, "on",             2);

    U_STRING_INIT(suboption_06, "2",              1);


    U_STRING_INIT(option_00, "undefined",      9);
    U_STRING_INIT(option_01, "rearrange",      9);  
    U_STRING_INIT(option_02, "alternate",      9);
    U_STRING_INIT(option_03, "backwards",      9);  
    U_STRING_INIT(option_04, "variable top",  12); 
    U_STRING_INIT(option_05, "top",            3);  
    U_STRING_INIT(option_06, "normalization", 13); 
    U_STRING_INIT(option_07, "caseLevel",      9);  
    U_STRING_INIT(option_08, "caseFirst",      9); 
    U_STRING_INIT(option_09, "scriptOrder",   11);  
    U_STRING_INIT(option_10, "charsetname",   11); 
    U_STRING_INIT(option_11, "charset",        7);  
  }
  start++; /*skip opening '['*/
  while(i < UTOK_OPTION_COUNT) {
    if(u_strncmp(start, rulesOptions[i].optionName, rulesOptions[i].optionLen) == 0) {
      foundOption = TRUE;
      if(end - start > rulesOptions[i].optionLen) {
        optionArg = start+rulesOptions[i].optionLen+1; /* start of the options, skip space */
        while(u_isWhitespace(*optionArg)) { /* eat whitespace */
          optionArg++;
        }
      }     
      break;
    }
    i++;
  }

  if(!foundOption) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return FALSE;
  }

  if(i<5) {
    if(optionArg) {
      for(j = 0; j<rulesOptions[i].subSize; j++) {
        if(u_strncmp(optionArg, rulesOptions[i].subopts[j].subName, rulesOptions[i].subopts[j].subLen) == 0) {
          ucol_uprv_tok_setOptionInImage(image, rulesOptions[i].attr, rulesOptions[i].subopts[j].attrVal);
          return TRUE;
        }
      }
    }
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return FALSE;
  } else if(i == 5) { /* variable top */
    *variableTop = TRUE;
    return TRUE;
  } else if(i == 6) {  /*rearange */
    return TRUE;
  } else if(i == 7) {  /*top */
    *top = TRUE;
    return TRUE;
  } else {
    *status = U_UNSUPPORTED_ERROR;
    return FALSE;
  }
}

#define UCOL_TOK_UNSET 0xFFFFFFFF
#define UCOL_TOK_RESET 0xDEADBEEF

const UChar *ucol_tok_parseNextToken(UColTokenParser *src, 
                        uint32_t *strength, 
                        uint32_t *chOffset, uint32_t *chLen, 
                        uint32_t *exOffset, uint32_t *exLen,
                        UBool *varT, UBool *top_,
                        UBool startOfRules,
                        UErrorCode *status) { 
/* parsing part */

  UBool variableTop = FALSE;
  UBool top = FALSE;
  UBool inChars = TRUE;
  UBool inQuote = FALSE;
  UBool wasInQuote = FALSE;
  UChar *optionEnd = NULL;

  uint32_t newCharsLen = 0, newExtensionLen = 0;
  uint32_t charsOffset = 0, extensionOffset = 0;
  uint32_t newStrength = UCOL_TOK_UNSET; 

  while (src->current < src->end) {
      UChar ch = *(src->current);

    if (inQuote) {
      if (ch == 0x0027/*'\''*/) {
          inQuote = FALSE;
      } else {
        if ((newCharsLen == 0) || inChars) {
          if(newCharsLen == 0) {
            charsOffset = src->extraCurrent - src->source;
          }
          newCharsLen++;
        } else {
          if(newExtensionLen == 0) {
            extensionOffset = src->extraCurrent - src->source;
          }
          newExtensionLen++;
        }
      }
    } else {
      /* Sets the strength for this entry */
      switch (ch) {
        case 0x003D/*'='*/ : 
          if (newStrength != UCOL_TOK_UNSET) {
            goto EndOfLoop;
          }

          /* if we start with strength, we'll reset to top */
          if(startOfRules == TRUE) {
            top = TRUE;
            newStrength = UCOL_TOK_RESET;
            goto EndOfLoop;
          }
          newStrength = UCOL_IDENTICAL;
          break;

        case 0x002C/*','*/:  
          if (newStrength != UCOL_TOK_UNSET) {
            goto EndOfLoop;
          }

          /* if we start with strength, we'll reset to top */
          if(startOfRules == TRUE) {
            top = TRUE;
            newStrength = UCOL_TOK_RESET;
            goto EndOfLoop;
          }
          newStrength = UCOL_TERTIARY;
          break;

        case  0x003B/*';'*/:
          if (newStrength != UCOL_TOK_UNSET) {
            goto EndOfLoop;
          }

          /* if we start with strength, we'll reset to top */
          if(startOfRules == TRUE) {
            top = TRUE;
            newStrength = UCOL_TOK_RESET;
            goto EndOfLoop;
          }
          newStrength = UCOL_SECONDARY;
          break;

        case 0x003C/*'<'*/:  
          if (newStrength != UCOL_TOK_UNSET) {
            goto EndOfLoop;
          }

          /* if we start with strength, we'll reset to top */
          if(startOfRules == TRUE) {
            top = TRUE;
            newStrength = UCOL_TOK_RESET;
            goto EndOfLoop;
          }
          /* before this, do a scan to verify whether this is */
          /* another strength */
          if(*(src->current+1) == 0x003C) {
            src->current++;
            if(*(src->current+1) == 0x003C) {
              src->current++; /* three in a row! */
              newStrength = UCOL_TERTIARY;
            } else { /* two in a row */
              newStrength = UCOL_SECONDARY;
            }
          } else { /* just one */
            newStrength = UCOL_PRIMARY;
          }
          break;

        case 0x0026/*'&'*/:  
          if (newStrength != UCOL_TOK_UNSET) {
            goto EndOfLoop;
          }

          newStrength = UCOL_TOK_RESET; /* PatternEntry::RESET = 0 */
          break;

        case 0x005b/*'['*/:
          /* options - read an option, analyze it */
          if((optionEnd = u_strchr(src->current, 0x005d /*']'*/)) != NULL) {
            ucol_uprv_tok_readAndSetOption(src->image, src->current, optionEnd, &variableTop, &top, status);
            src->current = optionEnd;
            if(top == TRUE) {
              if(newStrength == UCOL_TOK_RESET) { 
                src->current++;
                goto EndOfLoop;
              } else {
                *status = U_INVALID_FORMAT_ERROR;
              }
            }
            if(U_FAILURE(*status)) {
              return NULL;
            }
          }
          break;

        /* Ignore the white spaces */
        case 0x0009/*'\t'*/:
        case 0x000C/*'\f'*/:
        case 0x000D/*'\r'*/:
        case 0x000A/*'\n'*/:
        case 0x0020/*' '*/:  
          break; /* skip whitespace TODO use Unicode */

        case 0x002F/*'/'*/:
                /* This entry has an extension. */
          inChars = FALSE;
          break;

        /* found a quote, we're gonna start copying */
        case 0x0027/*'\''*/:
          inQuote = TRUE;
          wasInQuote = TRUE;

          if (newCharsLen == 0) {
            charsOffset = src->extraCurrent - src->source;
            newCharsLen++;
          } else if (inChars) { /* we're reading some chars */
            charsOffset = src->extraCurrent - src->source;
            if(newCharsLen != 0) {
              uprv_memcpy(src->extraCurrent, src->current - newCharsLen, newCharsLen*sizeof(UChar));
              src->extraCurrent += newCharsLen;
            }
            newCharsLen++;
          } else {
            if(newExtensionLen != 0) {
              uprv_memcpy(src->extraCurrent, src->current - newExtensionLen, newExtensionLen*sizeof(UChar));
              src->extraCurrent += newExtensionLen;
            }
            newExtensionLen++;
          }

          ch = *(++(src->current)); /*pattern[++index]; */
          break;

        /* '@' is french only if the strength is not currently set */
        /* if it is, it's just a regular character in collation rules */
        case 0x0040/*'@'*/:
          if (newStrength == UCOL_TOK_UNSET) {
            src->image->frenchCollation = UCOL_ON;
            break;
          }

        default:
          if (newStrength == UCOL_TOK_UNSET) {
            *status = U_INVALID_FORMAT_ERROR;
            return NULL;
          }

          if (ucol_tok_isSpecialChar(ch) && (inQuote == FALSE)) {
            *status = U_INVALID_FORMAT_ERROR;
            return NULL;
          }



          if (inChars) {
            if(newCharsLen == 0) {
              charsOffset = src->current - src->source;
            }
            newCharsLen++;
          } else {
            if(newExtensionLen == 0) {
              extensionOffset = src->current - src->source;
            }
            newExtensionLen++;
          }

          break;
        }
    }

    if(wasInQuote) {
      if(ch != 0x27 || newCharsLen == 1) {
        *src->extraCurrent++ = ch;
      }
      if(src->extraCurrent == src->extraEnd) {
        /* reallocate */
      }
    }

      src->current++;
    }

 EndOfLoop:
    wasInQuote = FALSE;
  if (newStrength == UCOL_TOK_UNSET) {
    return NULL;
  }

  if (newCharsLen == 0 && top == FALSE) {
    *status = U_INVALID_FORMAT_ERROR;
    return NULL;
  }

  *strength = newStrength; 

  *chOffset = charsOffset;
  *chLen = newCharsLen;
  *exOffset = extensionOffset;
  *exLen = newExtensionLen;
  *varT = variableTop;
  *top_ = top;

  return src->current;
}

/*
Processing Description
  1 Build a ListList. Each list has a header, which contains two lists (positive 
  and negative), a reset token, a baseCE, nextCE, and previousCE. The lists and 
  reset may be null. 
  2 As you process, you keep a LAST pointer that points to the last token you 
  handled. 
*/

uint32_t ucol_uprv_tok_assembleTokenList(UColTokenParser *src, UErrorCode *status) {
  UColToken *lastToken = NULL;
  const UChar *parseEnd = NULL;
  uint32_t expandNext = 0;
  UBool variableTop = FALSE;
  UBool top = FALSE;

  UColTokListHeader *ListList = NULL;

  uint32_t newCharsLen = 0, newExtensionsLen = 0;
  uint32_t charsOffset = 0, extensionOffset = 0;
  uint32_t newStrength = UCOL_TOK_UNSET; 

  ucol_tok_initTokenList(src, status);

  ListList = src->lh;

  src->image->variableTopValue = 0;

  while(src->current < src->end) {
  
  parseEnd = ucol_tok_parseNextToken(src, 
                      &newStrength, 
                      &charsOffset, &newCharsLen, 
                      &extensionOffset, &newExtensionsLen,
                      &variableTop, &top,
                      (UBool)(lastToken == NULL),
                      status);

    if(U_SUCCESS(*status) && parseEnd != NULL) {
      UColToken *sourceToken = NULL;
      UColToken key;

      /* if we had a variable top, we're gonna put it in */
      if(variableTop == TRUE && src->image->variableTopValue == 0) {
        variableTop = FALSE;
        src->image->variableTopValue = *(src->source + charsOffset);
      }

      key.source = newCharsLen << 24 | charsOffset;
      key.expansion = newExtensionsLen << 24 | extensionOffset;

      /*  4 Lookup each [source,  expansion] in the CharsToToken map, and find a sourceToken */
      sourceToken = (UColToken *)uhash_get(uchars2tokens, &key);

      if(newStrength != UCOL_TOK_RESET) {
        if(lastToken == NULL) { /* this means that rules haven't started properly */
          *status = U_INVALID_FORMAT_ERROR;
          return 0;
        }
      /*  6 Otherwise (when relation != reset) */
        if(sourceToken == NULL) {
          /* If sourceToken is null, create new one, */
          sourceToken = (UColToken *)uprv_malloc(sizeof(UColToken));
          sourceToken->source = newCharsLen << 24 | charsOffset;
          sourceToken->expansion = newExtensionsLen << 24 | extensionOffset;

          sourceToken->debugSource = *(src->source + charsOffset);
          if(newExtensionsLen > 0) {
            sourceToken->debugExpansion = *(src->source + extensionOffset);
          } else {
            sourceToken->debugExpansion = 0;
          }


          sourceToken->polarity = UCOL_TOK_POLARITY_POSITIVE; /* TODO: this should also handle reverse */
          sourceToken->next = NULL;
          sourceToken->previous = NULL;
          sourceToken->noOfCEs = 0;
          sourceToken->noOfExpCEs = 0;
          uhash_put(uchars2tokens, sourceToken, sourceToken, status);
        } else {
          /* we could have fished out a reset here */
          if(sourceToken->strength != UCOL_TOK_RESET) {
            /* otherwise remove sourceToken from where it was. */
            if(sourceToken->next != NULL) {
              sourceToken->next->previous = sourceToken->previous;
            } else {
              sourceToken->listHeader->last[sourceToken->polarity] = sourceToken->previous;
            }

            if(sourceToken->previous != NULL) {
              sourceToken->previous->next = sourceToken->next;
            } else {
              sourceToken->listHeader->first[sourceToken->polarity] = sourceToken->next;
            }
          }
        }

        sourceToken->strength = newStrength;
        sourceToken->listHeader = lastToken->listHeader;
        /*
        1.	Find the strongest strength in each list, and set strongestP and strongestN 
        accordingly in the headers. 
        */

        if(lastToken->strength == UCOL_TOK_RESET) {
        /* If LAST is a reset 
              insert sourceToken at the head of either the positive list or the negative 
              list, depending on the polarity of relation. 
              set the polarity of sourceToken to be the same as the list you put it in. */
          if(sourceToken->listHeader->first[sourceToken->polarity] == 0) {
            sourceToken->listHeader->first[sourceToken->polarity] = sourceToken;
            sourceToken->listHeader->last[sourceToken->polarity] = sourceToken;
          } else {
            sourceToken->listHeader->first[sourceToken->polarity]->previous = sourceToken;
            sourceToken->next = sourceToken->listHeader->first[sourceToken->polarity];
            sourceToken->listHeader->first[sourceToken->polarity] = sourceToken;
          }

          /*
            If "xy" doesn't occur earlier in the list or in the UCA, convert &xy * c * 
            d * ... into &x * c/y * d * ... 
          */
          if(expandNext != 0 && sourceToken->expansion == 0) {
            sourceToken->expansion = expandNext;
            sourceToken->debugExpansion = *(src->source + (expandNext & 0xFFFFFF));
            expandNext = 0;
          }

        } else {
        /* Otherwise (when LAST is not a reset) 
              if polarity (LAST) == polarity(relation), insert sourceToken after LAST, 
              otherwise insert before. 
              when inserting after or before, search to the next position with the same 
              strength in that direction. (This is called postpone insertion).         */
          if(lastToken->polarity == sourceToken->polarity) {
            while(lastToken->next != NULL && lastToken->next->strength > sourceToken->strength) {
              lastToken = lastToken->next;
            }
            sourceToken->previous = lastToken;
            if(lastToken->next != NULL) {
              lastToken->next->previous = sourceToken;
            } else {
              sourceToken->listHeader->last[sourceToken->polarity] = sourceToken;
            }

            sourceToken->next = lastToken->next;
            lastToken->next = sourceToken;
          } else {
            while(lastToken->previous != NULL && lastToken->previous->strength > sourceToken->strength) {
              lastToken = lastToken->previous;
            }
            sourceToken->next = lastToken;
            if(lastToken->previous != NULL) {
              lastToken->previous->next = sourceToken;
            } else {
              sourceToken->listHeader->first[sourceToken->polarity] = sourceToken;
            }
            sourceToken->previous = lastToken->previous;
            lastToken->previous = sourceToken;
          }
        }
      } else {
        uint32_t CE = UCOL_NOT_FOUND, SecondCE = UCOL_NOT_FOUND;
        collIterate s;

        if(newCharsLen > 1) {
          expandNext = ((newCharsLen-1)<<24) | (charsOffset + 1);
        } else {
          expandNext = 0;
        }

      /*  5 If the relation is a reset: 
          If sourceToken is null 
            Create new list, create new sourceToken, make the baseCE from source, put 
            the sourceToken in ListHeader of the new list */
        if(sourceToken == NULL) {

          /*
              3. The rule for "& abcdefg < xyz" is a bit tricky. What it turns into is:

              a. Find the longest sequence in "abcdefg" that is in UCA *OR* in the
              tailoring so far. Suppose that is "abcd".
              b. Then treat this rule as equivalent to:
              "& abcd < xyz / efg"
          */
          if(newCharsLen > 1) {
            key.source = 0x01000000 | charsOffset;
            sourceToken = (UColToken *)uhash_get(uchars2tokens, &key);
            if(sourceToken != NULL) {
              lastToken = sourceToken;
              continue;
            }
          }
          /* do the reset thing */
          sourceToken = (UColToken *)uprv_malloc(sizeof(UColToken));
          sourceToken->source = newCharsLen << 24 | charsOffset;
          sourceToken->expansion = newExtensionsLen << 24 | extensionOffset;
          
          sourceToken->debugSource = *(src->source + charsOffset);
          sourceToken->debugExpansion = *(src->source + extensionOffset);


          sourceToken->polarity = UCOL_TOK_POLARITY_POSITIVE; /* TODO: this should also handle reverse */
          sourceToken->strength = UCOL_TOK_RESET;
          sourceToken->next = NULL;
          sourceToken->previous = NULL;
          sourceToken->listHeader = &ListList[src->resultLen];
          /*
            3 Consider each item: relation, source, and expansion: e.g. ...< x / y ... 
              First convert all expansions into normal form. Examples: 
                If "xy" doesn't occur earlier in the list or in the UCA, convert &xy * c * 
                d * ... into &x * c/y * d * ... 
                Note: reset values can never have expansions, although they can cause the 
                very next item to have one. They may be contractions, if they are found 
                earlier in the list. 
          */
          if(top == FALSE) {
            if(newCharsLen > 1) {
              sourceToken->source = 0x01000000 | charsOffset;
            } 

 
            init_collIterate(src->UCA, src->source+charsOffset, 1, &s, FALSE); /* or newCharsLen instead of 1??? */

            CE = ucol_getNextCE(src->UCA, &s, status);
            /*UCOL_GETNEXTCE(CE, src->UCA, s, &status);*/

            SecondCE = ucol_getNextCE(src->UCA, &s, status);
            /*UCOL_GETNEXTCE(SecondCE, src->UCA, s, &status);*/
    
            ListList[src->resultLen].baseCE = CE;
            if(isContinuation(SecondCE)) {
              ListList[src->resultLen].baseContCE = SecondCE;
            } else {
              ListList[src->resultLen].baseContCE = 0;
            }
          } else { /* top == TRUE */
            top = FALSE;
            ListList[src->resultLen].baseCE = UCOL_RESET_TOP_VALUE;
            ListList[src->resultLen].baseContCE = 0;
          }


          ListList[src->resultLen].first[UCOL_TOK_POLARITY_NEGATIVE] = NULL;
          ListList[src->resultLen].last[UCOL_TOK_POLARITY_NEGATIVE] = NULL;
          ListList[src->resultLen].first[UCOL_TOK_POLARITY_POSITIVE] = NULL;
          ListList[src->resultLen].last[UCOL_TOK_POLARITY_POSITIVE] = NULL;

          ListList[src->resultLen].reset = sourceToken;

          src->resultLen++;
          uhash_put(uchars2tokens, sourceToken, sourceToken, status);
        } else { /* reset to something already in rules */
          top = FALSE;
        }
      }
      /*  7 After all this, set LAST to point to sourceToken, and goto step 3. */  
      lastToken = sourceToken;
    } else {
      return 0;
    }
  }

  return src->resultLen;
}

uint32_t ucol_tok_assembleTokenList(UColTokenParser *src, UErrorCode *status) {
  uint32_t res = ucol_uprv_tok_assembleTokenList(src, status);
  return res;
}

void ucol_tok_closeTokenList(UColTokenParser *src) {
  uhash_close(uchars2tokens);
  uprv_free(src->lh);
  uprv_free(src->source);
}

