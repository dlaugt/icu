/*
 **********************************************************************
 *   Copyright (C) 1997-1999, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
*
* File locid.cpp
*
* Created by: Richard Gillam
*
* Modification History:
*
*   Date        Name        Description
*   02/11/97    aliu        Changed gLocPath to fgDataDirectory and added 
*                           methods to get and set it.
*   04/02/97    aliu        Made operator!= inline; fixed return value 
*                           of getName().
*   04/15/97    aliu        Cleanup for AIX/Win32.
*   04/24/97    aliu        Numerous changes per code review.
*   08/18/98    stephen     Changed getDisplayName()
*                           Added SIMPLIFIED_CHINESE, TRADITIONAL_CHINESE
*                           Added getISOCountries(), getISOLanguages(),
*                           getLanguagesForCountry()
*   03/16/99    bertrand    rehaul.
*   07/21/99    stephen     Added U_CFUNC setDefault
*   11/09/99    weiv        Added const char * getName() const;
*   04/12/00    srl         removing unicodestring api's and cached hash code
*******************************************************************************
*/



#include "uhash.h"
#include "unicode/locid.h"
#include "unicode/uloc.h"
#include "unicode/resbund.h"
#include "uresimp.h"
#include "mutex.h"
#include "unicode/unicode.h"
#include "cmemory.h"
#include "cstring.h"

/**
 * static variables
 */
Locale Locale::fgDefaultLocale;

Locale*            Locale::localeList = NULL;
int32_t            Locale::localeListCount;
UnicodeString*    Locale::isoLanguages = NULL;
int32_t            Locale::isoLanguagesCount;
UnicodeString*    Locale::isoCountries = NULL;;
int32_t            Locale::isoCountriesCount;




/**
 * Constant definitions
 */
const Locale  Locale::ENGLISH("en");
const Locale  Locale::FRENCH("fr");
const Locale  Locale::GERMAN("de");
const Locale  Locale::ITALIAN("it");
const Locale  Locale::JAPANESE("ja");
const Locale  Locale::KOREAN("ko");
const Locale  Locale::CHINESE("zh");
const Locale  Locale::SIMPLIFIED_CHINESE("zh", "CN");
const Locale  Locale::TRADITIONAL_CHINESE("zh", "TW");

// Useful constant for country.

const Locale  Locale::FRANCE    ("fr", "FR");
const Locale  Locale::GERMANY   ("de", "DE"); 
const Locale  Locale::ITALY     ("it", "IT");
const Locale  Locale::JAPAN     ("ja", "JP");
const Locale  Locale::KOREA     ("ko", "KR");
const Locale  Locale::CHINA     ("zh", "CN");
const Locale  Locale::PRC       ("zh", "CN");
const Locale  Locale::TAIWAN    ("zh", "TW");
const Locale  Locale::UK        ("en", "GB");
const Locale  Locale::US        ("en", "US");
const Locale  Locale::CANADA    ("en", "CA");
const Locale  Locale::CANADA_FRENCH("fr", "CA");


/**
 * Table mapping ISO country codes to the ISO language codes of the languages spoken
 * in those countries.
 * (Because the Java VM specification for building arrays and hashtables causes
 * code that builds the tables element by element to be produces, we compress the data
 * into a single encoded String, and lazy evaluate the table from it.)
 */
UHashtable* Locale::ctry2LangMapping = 0;
const UnicodeString Locale::compressedCtry2LangMapping = UnicodeString(
    "ADfresAEarenAFpsAGenAIrnALsqAMhyruANnlenAOptAResASensmATdeAUenAWnlenAZazhyru"
    "BAsrshhrslmksqBBenBDbnhibhenBEfrnldeBFfrBGbgtrBHarenBIrnfrswBJfrBMenBNmsenzh"
    "BOesayquBRptBSenBTdzenneBVnoBWentnBYberuBZenesCAenfrCCenCFfrsgCGfrCHfrdeitrm"
    "CIfrCKmienCLesCMenfrCNzhboCOesCResCUesCVptCXenCYeltrenCZcsskDEdeDJarfrsoDKda"
    "DMenfrDOesDZarfrECesquEEetruEGarenfrEHarfritERamtiarenitESeseucaglETamaren"
    "FIfisvFJenfjhiFKenFMenFOfodaFRfreubrcoFXfrGAfrGBengdcyGDenfrGEkahyruGFfrGHen"
    "GIenesGLdaikklGMenwoGNfrGPfrenGQesGRelGTesGUenGWptGYenhiurHKzhenHNesHRhrHTfr"
    "HUhuIDinennlIEengaILiwarjiINhienguknksmlmrneorpasatateIOenIQarkutkIRfaarku"
    "ISisITitfrdeJMenJOarJPjaKEenswKGkyKHkmKIenKMfrarKNenKPkoKRkoKWarenKYenKZkkru"
    "LAlofrLBarenfrLCenfrLIdeLKtasienLRenLSstenLTltruplLUfrdeLVlvltruLYarenit"
    "MAarfresMCfrenitMDmorobgMGmgenfrMKmkshtrMLfrMMmyMNmnruMOzhptMQfrMRarfrMSen"
    "MTmtenitMUenfrhiMWenMXesMYmsenMZptNAenafdeNEfrhaNFenNGenhayoNIesNLnlfyNOno"
    "NPneNRnaenNUenNZenmiOMarenPAesenPEesquayPFfrPGenPHentlesPKurenpspasdPLplPMfren"
    "PNenPResenPTptPWenPYesgnQAarenREfrtaROrohuRUruRWenfrrwSAarSBenSCenfrSDarsu"
    "SEsvSGzhenmstaSHenSIslSJnoSKskhuplshSLenSMitSNfrSOarenitsoSRnleneshiSTptSVes"
    "SYarSZenssTCenTDfrarTFfrTGfrTHthTJtgruuzTKenmiTMtkruTNarTOentoTRtrkuTTenTVen"
    "TWzhTZenswUAukruUGenswUMenUSenesUYesUZuzruVAlaitVCenVEesVGenVIenVNvizhfr"
    "VUenfrbiWFfrWSensmYEarYTfrmgswYUsrshmkhuZAafenZMenZRfrswZWensn",
    "");

/*Used for stack allocation of temporary buffers
 *can be tweaked  for speed and likelihood of resorting to heap allocation*/ 
#define BUFFER_SIZE 50

/*Character separating the posix id fields*/
const UChar sep = 0x005F; // '_'
const char sepchar = '_'; // In the platform codepage.

Locale::~Locale()
{   
    /*if fullName is on the heap, we delete it*/
    if (fullName != fullNameBuffer) 
    {
        delete []fullName;
    }
}

Locale::Locale()
{
    init(uloc_getDefault());
}

Locale::Locale( const   char * newLanguage, 
                const   char * newCountry, 
                const   char * newVariant) 
{
    char togo_stack[ULOC_FULLNAME_CAPACITY];
    char *togo;
    char *togo_heap = NULL;
    int32_t size = 0;
    int32_t lsize = 0;
    int32_t csize = 0;
    int32_t vsize = 0;
    char    *p;

    if( (newLanguage==NULL) && (newCountry == NULL) && (newVariant == NULL) )
    {
        init(NULL); /* shortcut */
    }
    else
    {
        // Calculate the size of the resulting string.

        // Language
        if ( newLanguage != NULL )
        {
            lsize = (int32_t)uprv_strlen(newLanguage);
            size = lsize;
        }

        // _Country
        if ( newCountry != NULL )
        {
            csize = (int32_t)uprv_strlen(newCountry);
            size += csize;
        }

        // _Variant
        if ( newVariant != NULL )
        {
            // remove leading _'s
            while(newVariant[0] == sepchar)
            {
                newVariant++;
            }
            
            // remove trailing _'s
            vsize = (int32_t)uprv_strlen(newVariant);
            while( (vsize>1) && (newVariant[vsize-1] == sepchar) )
            {
                vsize--;
            }
        }

        if( vsize > 0 )
        {
            size += vsize;
        }

        // Separator rules:
        if ( vsize > 0 )
        {
            size += 2;  // at least: __v 
        }
        else if ( csize > 0 )
        {
            size += 1;  // at least: _v 
        }

        //  NOW we have the full locale string..

        /*if the whole string is longer than our internal limit, we need
        to go to the heap for temporary buffers*/
        if (size > ULOC_FULLNAME_CAPACITY)
        {
            togo_heap = new char[size+1];
            togo = togo_heap;
        }
        else
        {
            togo = togo_stack;
        }

        togo[0] = 0;

        // Now, copy it back.
        p = togo;
        if ( lsize != 0 )
        {
            uprv_strcpy(p, newLanguage);
            p += lsize;
        }

        if ( ( vsize != 0 ) || (csize != 0) )  // at least:  __v
        {                                      //            ^
            *p++ = sepchar;
        }

        if ( csize != 0 )
        { 
            
            uprv_strcpy(p, newCountry);
            p += csize;
        }

        if ( vsize != 0)
        {
            *p++ = sepchar; // at least: __v

            uprv_strncpy(p, newVariant, vsize);  // Must use strncpy because 
            p += vsize;                          // of trimming (above).
            *p = 0; // terminate
        }

        // Parse it, because for example 'language' might really be a complete
        // string.
        init(togo);

        delete [] togo_heap; /* If it was needed */
    }
}

Locale::Locale(const    Locale& other)

{
    int j;
    /*Copy the language and country fields*/
    uprv_strcpy(language, other.language);
    uprv_strcpy(country, other.country);

    /*make fullName point to the heap if necessary*/
    if ((j=(int)uprv_strlen(other.fullName)) > ULOC_FULLNAME_CAPACITY)
    {
        fullName = new char[j+1];
    }
    else
        fullName = fullNameBuffer;

    uprv_strcpy(fullName, other.fullName);

    /*Make the variant point to the same offset as the copied*/
    variant = fullName + (other.variant - other.fullName) ;
}

UBool
Locale::operator==( const   Locale& other) const
{
    if (uprv_strcmp(other.language, language) == 0
        && uprv_strcmp(other.country, country) == 0
        && uprv_strcmp(other.variant, variant) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*This function initializes a Locale from a C locale ID*/
Locale& Locale::init(const char* localeID)
{
    int k,l;
    UErrorCode err = U_ZERO_ERROR;

    if (localeID == NULL)
        localeID = uloc_getDefault();

    l = uloc_getLanguage(localeID, 
        this->language,
        ULOC_LANG_CAPACITY,
        &err);

    l += k = uloc_getCountry(localeID,
        this->country,
        ULOC_COUNTRY_CAPACITY,
        &err);

    l--; //adjust for the 2 zero terminators

    /*Go to heap for the fullName if necessary*/
    int j;
    if ((j=(int)uprv_strlen(localeID)) > ULOC_FULLNAME_CAPACITY)
    {
        this->fullName = new char[j+1];
    }
    else {
        this->fullName = this->fullNameBuffer;
    }

    uprv_strcpy(this->fullName, localeID);

    /*Setting up the variant:
    -point to the zero terminator of fullName if there is none
    -point to the first character of the variant if ther is one
    */
    if (k > 1)
    {
        if (this->fullName[l] == '\0')
        {
            this->variant = this->fullName + l;
        }
        else
        {
            int32_t varLength;
            UErrorCode intErr = U_ZERO_ERROR;
            varLength = uloc_getVariant(this->fullName, NULL, 0,  &intErr);

            if(U_FAILURE(intErr) && (intErr != U_BUFFER_OVERFLOW_ERROR))
            {
                this->variant = (char*)u_errorName(intErr);
            }
            else if(varLength <= 0)
            {   /* bail  - point at the null*/
                // this->variant = this->fullName + j;
                this->variant = "Len<=0";
            }
            else
            {
                /* variant is at the end. We just don't know where exactly it might be. */
                this->variant = this->fullName + j - varLength + 1 ;
            }
        }
    }
    else
        this->variant = this->fullName + l - 1;

    return *this;
}

Locale& Locale::operator=(const Locale& other)
{
    uprv_strcpy(language, other.language);
    uprv_strcpy(country, other.country);
    if (other.fullName == other.fullNameBuffer)
    {
        fullName = fullNameBuffer;
    }
    else 
    {
    /*In case the assigner has some of its data on the heap
        * we need to do the same*/
        if (fullName != fullNameBuffer) delete []fullName;
        fullName = new char[(uprv_strlen(other.fullName)+1)];
    }
    uprv_strcpy(fullName, other.fullName);
    /*Make the variant point to the same offset as the assigner*/
    variant = fullName + (other.variant - other.fullName) ;

    return *this;
}

int32_t
Locale::hashCode() const 
{
    UnicodeString fullNameUString(language, "");
    return fullNameUString.append(UnicodeString(country, "")).append(UnicodeString(variant, "")).hashCode();
}


Locale&
Locale::getDefault() 
{
    return fgDefaultLocale;
}


void locale_set_default_internal(const char *id)
{
    Locale::getDefault().init(id);
}

/* sfb 07/21/99 */
U_CFUNC void
locale_set_default(const char *id)
{
    locale_set_default_internal(id);
}
/* end */


void 
Locale::setDefault( const   Locale&     newLocale, 
                            UErrorCode&  status) 
{
    if (U_FAILURE(status))
        return;

    uloc_setDefault(newLocale.fullName, &status);

    fgDefaultLocale = newLocale;
}

Locale
Locale::createFromName (const char *name)
{
    char stack[128];
    char *heap = NULL;
    char *buf = stack;
    int32_t buflen = 128;
    UErrorCode status;

    status = U_ZERO_ERROR;

    /* for some reason */
    if(uprv_strlen(name) > buflen) {
        buflen = (int32_t)uprv_strlen(name)+1;
        heap = (char*)uprv_malloc(buflen);
        buf = heap;
    }

    uloc_getName(name, buf, buflen, &status);

    Locale l(buf);
    if(heap != NULL)
    {
        free(heap);
    }
    return l;
}

const char *
Locale::getCountry() const
{
    return country;
}

const char *
Locale::getLanguage() const
{
    return language;
}

const char *
Locale::getVariant() const
{
    return variant;
}

#ifdef ICU_LOCID_USE_DEPRECATES
UnicodeString& 
Locale::getLanguage(UnicodeString& lang) const
{
  lang = language;
  return lang;
}

UnicodeString& 
Locale::getCountry(UnicodeString& cntry) const
{
  cntry = country;
  return cntry;
}

UnicodeString& 
Locale::getVariant(UnicodeString& var) const
{
  var = variant;
  return var;
}

UnicodeString& 
Locale::getName(UnicodeString& name) const
{
    name = UnicodeString(fullName,"");
  return name;
}
#endif

const char * 
Locale::getName() const
{
    return fullName;
}

const char *
Locale::getISO3Language() const
{
    return uloc_getISO3Language(fullName);
}


const char *
Locale::getISO3Country() const
{
    return uloc_getISO3Country(fullName);
}

#ifdef ICU_LOCID_USE_DEPRECATES
UnicodeString& 
Locale::getISO3Language(UnicodeString& lang, UErrorCode& status) const
{
    if(U_FAILURE(status))
      return lang;

    lang = uloc_getISO3Language(fullName);
    if (lang.length() == 0)
      status = U_MISSING_RESOURCE_ERROR;
    
    return lang;
}

UnicodeString& 
Locale::getISO3Country(UnicodeString& cntry, UErrorCode& status) const
{
    if(U_FAILURE(status))
        return cntry;

    cntry = uloc_getISO3Country(fullName);
    if (cntry.length() == 0)
        status = U_MISSING_RESOURCE_ERROR;

    return cntry;
}
#endif

/**
 * Return the LCID value as specified in the "LocaleID" resource for this
 * locale.  The LocaleID must be expressed as a hexadecimal number, from
 * one to four digits.  If the LocaleID resource is not present, or is
 * in an incorrect format, 0 is returned.  The LocaleID is for use in
 * Windows (it is an LCID), but is available on all platforms.
 */
uint32_t 
Locale::getLCID() const
{
    return uloc_getLCID(fullName);
}

UnicodeString& 
Locale::getDisplayLanguage(UnicodeString& dispLang) const
{
    return this->getDisplayLanguage(getDefault(), dispLang);
}

/*We cannot make any assumptions on the size of the output display strings
* Yet, since we are calling through to a C API, we need to set limits on
* buffer size. For all the following getDisplay functions we first attempt
* to fill up a stack allocated buffer. If it is to small we heap allocated
* the exact buffer we need copy it to the UnicodeString and delete it*/

UnicodeString&
Locale::getDisplayLanguage( const   Locale&         inLocale,
                UnicodeString&  dispLang) const
{
    UErrorCode status = U_ZERO_ERROR;
    UChar bufBuffer[BUFFER_SIZE];
    UChar* buf = bufBuffer;
    
    //  dispLang = "result";
    //  return dispLang;
    int size = uloc_getDisplayLanguage(fullName,
        inLocale.fullName,
        buf,
        BUFFER_SIZE,
        &status);
    
    
    if (status == U_BUFFER_OVERFLOW_ERROR)
    {
        status = U_ZERO_ERROR;
        buf = new UChar[size];
        
        uloc_getDisplayLanguage(fullName,
            inLocale.fullName,
            buf,
            size,
            &status);
        
    }
    
    dispLang = buf;
    
    if (buf != bufBuffer)
        delete []buf;
    
    return dispLang;
}

UnicodeString& 
Locale::getDisplayCountry(UnicodeString& dispCntry) const
{
    return this->getDisplayCountry(getDefault(), dispCntry);
}

UnicodeString& 
Locale::getDisplayCountry(  const   Locale&         inLocale,
                                    UnicodeString&  dispCntry) const
{
    UErrorCode status = U_ZERO_ERROR;
    UChar bufBuffer[BUFFER_SIZE];
    UChar* buf = bufBuffer;

    int size = uloc_getDisplayCountry(fullName,
        inLocale.fullName,
        buf,
        BUFFER_SIZE,
        &status);

    if (status == U_BUFFER_OVERFLOW_ERROR)
    {
        status = U_ZERO_ERROR;
        buf = new UChar[size];
        uloc_getDisplayCountry(fullName,
            inLocale.fullName,
            buf,
            size,
            &status);
    }

    dispCntry = buf;

    if (buf != bufBuffer)
        delete []buf;

    return dispCntry;
}

UnicodeString& 
Locale::getDisplayVariant(UnicodeString& dispVar) const
{
    return this->getDisplayVariant(getDefault(), dispVar);
}

UnicodeString& Locale::getDisplayVariant(const Locale& inLocale,
                     UnicodeString& dispVar) const
{
    UErrorCode status = U_ZERO_ERROR;
    UChar bufBuffer[BUFFER_SIZE];
    UChar* buf = bufBuffer;

    int size = uloc_getDisplayVariant(fullName,
        inLocale.fullName,
        buf,
        BUFFER_SIZE,
        &status);

    if (status == U_BUFFER_OVERFLOW_ERROR)
    {
        status = U_ZERO_ERROR;
        buf = new UChar[size];
        uloc_getDisplayVariant(fullName,
            inLocale.fullName,
            buf,
            size,
            &status);
    }

    dispVar = buf;

    if (buf != bufBuffer)
        delete []buf;

    return dispVar;
}

UnicodeString& 
Locale::getDisplayName( UnicodeString& name ) const
{
    return this->getDisplayName(getDefault(), name);
}

UnicodeString& 
Locale::getDisplayName( const   Locale&     inLocale,
            UnicodeString& result) const
{
    UErrorCode status = U_ZERO_ERROR;
    UChar bufBuffer[BUFFER_SIZE];
    UChar* buf = bufBuffer;

    int size = uloc_getDisplayName(fullName,
        inLocale.fullName,
        buf,
        BUFFER_SIZE,
        &status);

    if (status == U_BUFFER_OVERFLOW_ERROR)
    {
        status = U_ZERO_ERROR;
        
        buf = new UChar[size];
        uloc_getDisplayName(fullName,
            inLocale.fullName,
            buf,
            size,
            &status);
    }

    result = buf;

    if (buf != bufBuffer) {
        delete []buf;
    }

    return result;
}

const Locale*
Locale::getAvailableLocales(int32_t& count) 
{
    // for now, there is a hardcoded list, so just walk through that list and set it up.
    if (localeList == 0) {
        UErrorCode status = U_ZERO_ERROR;
        ResourceBundle index(UnicodeString(""), Locale(kIndexLocaleName), status);
        ResourceBundle locales = index.get(kIndexTag, status);

        char name[96];
        locales.resetIterator();

        count = locales.getSize();

        Locale *newLocaleList = new Locale[count];

        int32_t i = 0;
        UnicodeString temp;
        while(locales.hasNext()) {
            temp = locales.getNextString(status);
            temp.extract(0, temp.length(), name);
            name[temp.length()] = '\0';
            newLocaleList[i++].setFromPOSIXID(name);
        }

        Mutex mutex;
        if(localeList != 0) {
            delete []newLocaleList;
        }
        else {
            localeListCount = count;
            localeList = newLocaleList;
        }
    }
    count = localeListCount;
    return localeList;
}

const char* const* Locale::getISOCountries()
{
    return uloc_getISOCountries();
}

const char* const* Locale::getISOLanguages()
{
    return uloc_getISOLanguages();
}


#ifdef ICU_LOCID_USE_DEPRECATES
/**
 * Returns a list of all 2-letter country codes defined in ISO 3166.
 * Can be used to create Locales.
 */
const UnicodeString*
Locale::getISOCountries(int32_t& count) 
{
  if(isoCountries == 0) {
    const char* const*    cResult = uloc_getISOCountries();
    int32_t tempCount;
    
    int i;
    tempCount = 0;
    for (i = 0; cResult[i] != NULL; i++)      ++tempCount;
    
    UnicodeString *temp = new UnicodeString [tempCount];
    
    for(i = 0; i < tempCount; ++i)
      temp[i] = cResult[i];
    
    Mutex mutex;        
    if(isoCountries != 0)
      delete [] temp;
    else {
      isoCountries = temp;
      isoCountriesCount = tempCount;
    }
  }
  
  count = isoCountriesCount;


  return isoCountries;
}

/**
 * Returns a list of all 2-letter language codes defined in ISO 639.
 * Can be used to create Locales.
 * [NOTE:  ISO 639 is not a stable standard-- some languages' codes have changed.
 * The list this function returns includes both the new and the old codes for the
 * languages whose codes have changed.]
 */
const UnicodeString* 
Locale::getISOLanguages(int32_t& count) 
{
  if(isoLanguages == 0) {
    const char* const* cResult = uloc_getISOLanguages();
    int32_t tempCount;
    
    int i;
    tempCount = 0;
    for (i = 0; cResult[i] != NULL; i++)   ++tempCount;
    
    UnicodeString *temp = new UnicodeString [tempCount];
    
    for(i = 0; i < tempCount; ++i)
      temp[i] = cResult[i];
    
    Mutex mutex;        
    if(isoLanguages != 0)
      delete [] temp;
    else {
      isoLanguages = temp;
      isoLanguagesCount = tempCount;
    }
  }
  
  count = isoLanguagesCount;
  return isoLanguages;
}
#endif

/**
 * Given an ISO country code, returns an array of Strings containing the ISO
 * codes of the languages spoken in that country.  Official languages are listed
 * in the returned table before unofficial languages, but other than that, the
 * order of the returned list is indeterminate.  If the value the user passes in
 * for "country" is not a valid ISO 316 country code, or if we don't have language
 * information for the specified country, this function returns an empty array.
 *
 * [This function is not currently part of Locale's API, but is needed in the
 * implementation.  We hope to add it to the API in a future release.]
 */
const UnicodeString* 
Locale::getLanguagesForCountry(const UnicodeString& country, int32_t& count) 
{
    // To save on the size of a static array in the .class file, we keep the
    // data around encoded into a String.  The first time this function is called,
    // the String s parsed to produce a Hashtable, which is then used for all
    // lookups.
    if(ctry2LangMapping == 0) {
        UErrorCode err = U_ZERO_ERROR;
        UHashtable *temp = uhash_open(uhash_hashUnicodeString, uhash_compareUnicodeString, &err);
        if (U_FAILURE(err)) 
        {
            count = 0;
            return NULL;
        }

        uhash_setKeyDeleter(temp, uhash_deleteUnicodeString);

        int32_t i = 0;
        int32_t j;
        int32_t count = (int32_t)(sizeof(compressedCtry2LangMapping) / sizeof(compressedCtry2LangMapping[0]));
        while (i < count) {
            UnicodeString key;
            compressedCtry2LangMapping.extractBetween(i, i + 2, key);
            i += 2;
            for(j = i; j < count; j += 2)
                if(Unicode::isUpperCase(compressedCtry2LangMapping[j]))
                    break;
                UnicodeString compressedValues;
                compressedCtry2LangMapping.extractBetween(i, j, compressedValues);
                UnicodeString *values = new UnicodeString[compressedValues.length() / 2];
                int32_t valLen = (int32_t)(sizeof(values) / sizeof(values[0]));
                for (int32_t k = 0; k < valLen; ++k)
                    compressedValues.extractBetween(k * 2, (k * 2) + 2, values[k]);
                uhash_put(temp, new UnicodeString(key), values, &err);
                i = j;
        }

        Mutex mutex;
        if(ctry2LangMapping != 0)
            uhash_close(temp);
        else
            ctry2LangMapping = temp;
    }

    const UnicodeString *result = (const UnicodeString*)uhash_get(ctry2LangMapping, &country);
    if(result == 0)
        count = 0;
    else
        count = (int32_t)(sizeof(result) / sizeof(result[0]));

    return result;
}


// ================= privates =====================================




// Set the locale's data based on a posix id. 
void Locale::setFromPOSIXID(const char *posixID)
{
    init(posixID);
}

#ifdef ICU_LOCID_USE_DEPRECATES
// Deprecated APIs
Locale::Locale( const   UnicodeString&  newLanguage)
{
  char myLocaleID[ULOC_FULLNAME_CAPACITY];

  myLocaleID[newLanguage.extract(0, 0x7fffffff, myLocaleID, "")] = '\0';
  init(myLocaleID);
}

Locale::Locale( const   UnicodeString&  newLanguage, 
                const   UnicodeString&  newCountry)
{
    UnicodeString togo(newLanguage);
    char myLocaleID[ULOC_FULLNAME_CAPACITY];
  
    if(newCountry.length()>0) {
        togo += sep;
        togo += newCountry;
    }

    myLocaleID[togo.extract(0, 0x7fffffff, myLocaleID, "")] = '\0';
    init(myLocaleID);
}


Locale::Locale( const   UnicodeString&  newLanguage, 
                const   UnicodeString&  newCountry, 
                const   UnicodeString&  newVariant) 
{
  UnicodeString togo(newLanguage);
  
  char myLocaleID[ULOC_FULLNAME_CAPACITY];
  UnicodeString newVariantCopy(newVariant);
  
  
  if (newCountry.length() > 0 ||
      newVariantCopy.length() > 0 )
    {
      togo += sep;
      togo += newCountry;
    }
  
  int vsize = newVariantCopy.length();
    if (vsize > 0)
      {
    int i = 0;
    //We need to trim variant codes : (_*)$var(_*) --> $var 
    while ((i<vsize) && newVariantCopy[i] == sep) newVariantCopy.remove(i++, 1);
    i = newVariantCopy.length() - 1;
    while (i && (newVariantCopy[i] == sep)) newVariantCopy.remove(i--, 1);
    
    togo += sep ;
    togo += newVariantCopy ;
      }
  
  int size = togo.length();

  /*if the variant is longer than our internal limit, we need
  to go to the heap for temporary buffers*/
  if (size > ULOC_FULLNAME_CAPACITY)
    {
      char *togo_heap = new char[size+1];
      togo.extract(0,size, togo_heap, "");
      togo_heap[size] = '\0';
      init(togo_heap);
      delete []togo_heap;
    }
  else 
    {
      togo.extract(0,size, myLocaleID, "");
      myLocaleID[size] = '\0';
      init(myLocaleID);
    }
  
}
#endif

//eof




