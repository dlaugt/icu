/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-1999, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CDATTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda               Creation
*********************************************************************************
*/

/* C API TEST FOR DATE FORMAT */

#include "unicode/uloc.h"
#include "unicode/utypes.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"
#include "unicode/unum.h"
#include "unicode/ustring.h"
#include "cintltst.h"
#include "cdattst.h"
#include<stdio.h>
#include<string.h>
#include "cformtst.h"


void addDateForTest(TestNode** root)
{
    addTest(root, &TestDateFormat, "tsformat/cdattst/TestDateFormat");
    addTest(root, &TestSymbols, "tsformat/cdattst/TestSymbols");
}
/* Testing the DateFormat API */
void TestDateFormat()
{
    UDateFormat *def, *fr, *it, *de, *def1, *fr_pat;
    UDateFormat *copy;
    UErrorCode status = U_ZERO_ERROR;
    UChar* result;
    const UCalendar *cal;
    const UNumberFormat *numformat1, *numformat2;
    UChar temp[30];
    int32_t numlocales;
    UDate d1;
    int32_t resultlength;
    int32_t resultlengthneeded;
    int32_t parsepos;
    UFieldPosition pos;
    UDate d = 837039928046.0;
    double num = -10456.37;
    const char* str="yyyy.MM.dd G 'at' hh:mm:ss z";
    const char t[]="2/3/76 2:50 AM";
    /*Testing udat_open() to open a dateformat */
    log_verbose("\nTesting udat_open() with various parameters\n");
    fr = udat_open(UDAT_FULL, UDAT_DEFAULT, "fr_FR", NULL,0, &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in creating the dateformat using full time style with french locale\n %s\n", 
            myErrorName(status) );
    }
    /* this is supposed to open default date format, but later on it treats it like it is "en_US" 
       - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
    /* def = udat_open(UDAT_SHORT, UDAT_SHORT, NULL, NULL, 0, &status); */
    def = udat_open(UDAT_SHORT, UDAT_SHORT, "en_US", NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in creating the dateformat using short date and time style\n %s\n", 
            myErrorName(status) );
    }
    it = udat_open(UDAT_DEFAULT, UDAT_MEDIUM, "it_IT", NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in creating the dateformat using medium date style with italian locale\n %s\n", 
            myErrorName(status) );
    }
    de = udat_open(UDAT_LONG, UDAT_LONG, "de_DE", NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in creating the dateformat using long time and date styles with german locale\n %s\n",
            myErrorName(status));
    }
    /*creating a default dateformat */
    def1 = udat_open(UDAT_SHORT, UDAT_SHORT, NULL, NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in creating the dateformat using short date and time style\n %s\n", 
            myErrorName(status) );
    }


    /*Testing udat_getAvailable() and udat_countAvailable()*/ 
    log_verbose("\nTesting getAvailableLocales and countAvailable()\n");
    numlocales=udat_countAvailable();
    /* use something sensible w/o hardcoding the count */
    if(numlocales < 0)
        log_err("FAIL: error in countAvailable\n");
    log_verbose("The number of locales for which date/time formatting patterns are available is %d\n", numlocales);
    /*for(i=0;i<numlocales;i++)
        log_verbose("%s\n", uloc_getName(udat_getAvailable(i))); */
    
    /*Testing udat_clone()*/
    log_verbose("\nTesting the udat_clone() function of date format\n");
    copy=udat_clone(def, &status);
    if(U_FAILURE(status)){
        log_err("Error in creating the clone using udat_clone: %s\n", myErrorName(status) );
    }
    /*if(def != copy)
        log_err("Error in udat_clone");*//*how should i check for equality????*/
    
    /*Testing udat_format()*/
    log_verbose("\nTesting the udat_format() function of date format\n");
    u_uastrcpy(temp, "7/10/96 4:05 PM");
    /*format using def */
    resultlength=0;
    resultlengthneeded=udat_format(def, d, NULL, resultlength, &pos, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_format(def, d, result, resultlength, &pos, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in formatting using udat_format(.....) %s\n", myErrorName(status) );
    }
    else
        log_verbose("PASS: formatting successful\n");
    if(u_strcmp(result, temp)==0)
        log_verbose("PASS: Date Format for US locale successful uisng udat_format()\n");
    else
        log_err("FAIL: Date Format for US locale failed using udat_format()\n");
    /*format using fr */
    
    u_uastrcpy(temp, "10 juil. 96 16 h 05 GMT-07:00");
    result=myDateFormat(fr, d);
    if(u_strcmp(result, temp)==0)
        log_verbose("PASS: Date Format for french locale successful uisng udat_format()\n");
    else
        log_err("FAIL: Date Format for french locale failed using udat_format()\n");
    /*foramt using it */
    u_uastrcpy(temp, "10/lug/96 16:05:28");
    
    if(u_strcmp(myDateFormat(it,d), temp)==0)
        log_verbose("PASS: Date Format for italian locale successful uisng udat_format()\n");
    else
        log_err("FAIL: Date Format for italian locale failed using udat_format()\n");

free(result);
    
    /*Testing parsing using udat_parse()*/
    log_verbose("\nTesting parsing using udat_parse()\n");
    u_uastrcpy(temp,"2/3/76 2:50 AM");
    parsepos=0;
    
    d1=udat_parse(def, temp, u_strlen(temp), &parsepos, &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in parsing using udat_parse(.....) %s\n", myErrorName(status) );
    }
    else
        log_verbose("PASS: parsing succesful\n");
    /*format it back and check for equality */
    
    
    if(u_strcmp(myDateFormat(def, d1),temp)!=0)
        log_err("FAIL: error in parsing\n");

        
        
    
    /*Testing udat_openPattern()  */
    status=U_ZERO_ERROR;
    log_verbose("\nTesting the udat_openPattern with a specified pattern\n");
    /*for french locale */
    fr_pat=udat_openPattern(temp, u_strlen(temp), "fr_FR", &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in creating a date format using udat_openPattern \n %s\n", 
            myErrorName(status) );
    }
    else
        log_verbose("PASS: creating dateformat using udat_openPattern() succesful\n");

    
        /*Testing applyPattern and toPattern */
    log_verbose("\nTesting applyPattern and toPattern()\n");
    udat_applyPattern(def1, FALSE, temp, u_strlen(temp));
    log_verbose("Extracting the pattern\n");

    resultlength=0;
    resultlengthneeded=udat_toPattern(def1, FALSE, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded + 1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_toPattern(def1, FALSE, result, resultlength, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in extracting the pattern from UNumberFormat\n %s\n", 
            myErrorName(status) );
    }
    if(u_strcmp(result, temp)!=0)
        log_err("FAIL: Error in extracting the pattern\n");
    else
        log_verbose("PASS: applyPattern and toPattern work fine\n");
    

free(result);    
    
    
    /*Testing getter and setter functions*/
    /*isLenient and setLenient()*/
    log_verbose("\nTesting the isLenient and setLenient properties\n");
    udat_setLenient(fr, udat_isLenient(it));
    if(udat_isLenient(fr) != udat_isLenient(it)) 
        log_err("ERROR: setLenient() failed\n");
    else
        log_verbose("PASS: setLenient() successful\n");


    /*Test get2DigitYearStart set2DigitYearStart */
    log_verbose("\nTesting the get and set 2DigitYearStart properties\n");
    d1= udat_get2DigitYearStart(fr_pat,&status);
    if(U_FAILURE(status)) {
            log_err("ERROR: udat_get2DigitYearStart failed %s\n", myErrorName(status) );
    }
    status = U_ZERO_ERROR;
    udat_set2DigitYearStart(def1 ,d1, &status);
    if(U_FAILURE(status)) {
        log_err("ERROR: udat_set2DigitYearStart failed %s\n", myErrorName(status) );
    }
    if(udat_get2DigitYearStart(fr_pat, &status) != udat_get2DigitYearStart(def1, &status))
        log_err("FAIL: error in set2DigitYearStart\n");
    else
        log_verbose("PASS: set2DigitYearStart successful\n");
    /*try setting it to another value */
    udat_set2DigitYearStart(de, 2000.0, &status);
    if(U_FAILURE(status)){
        log_verbose("ERROR: udat_set2DigitYearStart failed %s\n", myErrorName(status) );
    }
    if(udat_get2DigitYearStart(de, &status) != 2000)
        log_err("FAIL: error in set2DigitYearStart\n");
    else
        log_verbose("PASS: set2DigitYearStart successful\n");

    

    /*Test getNumberFormat() and setNumberFormat() */
    log_verbose("\nTesting the get and set NumberFormat properties of date format\n");
    numformat1=udat_getNumberFormat(fr_pat);
    udat_setNumberFormat(def1, numformat1);
    numformat2=udat_getNumberFormat(def1);
    if(u_strcmp(myNumformat(numformat1, num), myNumformat(numformat2, num)) !=0)
        log_err("FAIL: error in setNumberFormat or getNumberFormat()\n");
    else
        log_verbose("PASS:setNumberFormat and getNumberFormat succesful\n");
    /*try setting the number format to another format */
    numformat1=udat_getNumberFormat(def);
    udat_setNumberFormat(def1, numformat1);
    numformat2=udat_getNumberFormat(def1);
    if(u_strcmp(myNumformat(numformat1, num), myNumformat(numformat2, num)) !=0)
        log_err("FAIL: error in setNumberFormat or getNumberFormat()\n");
    else
        log_verbose("PASS: setNumberFormat and getNumberFormat succesful\n");



    /*Test getCalendar and setCalendar*/
    log_verbose("\nTesting the udat_getCalendar() and udat_setCalendar() properties\n");
    cal=udat_getCalendar(fr_pat);
    
    
    udat_setCalendar(def1, cal);
    if(!ucal_equivalentTo(udat_getCalendar(fr_pat), udat_getCalendar(def1)))
        log_err("FAIL: Error in setting and getting the calendar\n");
    else
        log_verbose("PASS: getting and setting calendar successful\n");
        
        
    
    /*Closing the UDateForamt */
    udat_close(def);
    udat_close(fr);
    udat_close(it);
    udat_close(de);
    udat_close(def1);
    udat_close(fr_pat);
    
}

/*Testing udat_getSymbols() and udat_setSymbols() and udat_countSymbols()*/
void TestSymbols()
{
    UDateFormat *def, *fr;
    UErrorCode status = U_ZERO_ERROR;
    UChar *value, *result;
    int32_t resultlength;
    int32_t resultlengthout;
    UChar *pattern;
        

    /*creating a dateformat with french locale */
    log_verbose("\ncreating a date format with french locale\n");
    fr = udat_open(UDAT_FULL, UDAT_DEFAULT, "fr_FR", NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_err("error in creating the dateformat using full time style with french locale\n %s\n", 
            myErrorName(status) );
    }
    /*creating a default dateformat */
    log_verbose("\ncreating a date format with default locale\n");
    /* this is supposed to open default date format, but later on it treats it like it is "en_US" 
       - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
    /* def = udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,NULL, NULL, 0, &status); */
    def = udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,"en_US", NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_err("error in creating the dateformat using short date and time style\n %s\n", 
            myErrorName(status) );
    }
    
    
    /*Testing countSymbols, getSymbols and setSymbols*/
    log_verbose("\nTesting countSymbols\n");
    /*since the month names has the last string empty and week names are 1 based 1.e first string in the weeknames array is empty */
    if(udat_countSymbols(def, UDAT_ERAS)!=2 || udat_countSymbols(def, UDAT_MONTHS)!=13 || 
        udat_countSymbols(def, UDAT_SHORT_MONTHS)!=13 || udat_countSymbols(def, UDAT_WEEKDAYS)!=8 ||
        udat_countSymbols(def, UDAT_SHORT_WEEKDAYS)!=8 || udat_countSymbols(def, UDAT_AM_PMS)!=2 ||
        udat_countSymbols(def, UDAT_LOCALIZED_CHARS)!=1)
      log_err("FAIL: error in udat_countSymbols\n");
    else
        log_verbose("PASS: udat_countSymbols() successful\n");

    /*testing getSymbols*/
    log_verbose("\nTesting getSymbols\n");
    pattern=(UChar*)malloc(sizeof(UChar) * 10);
    u_uastrcpy(pattern, "jeudi");
    resultlength=0;
    resultlengthout=udat_getSymbols(fr, UDAT_WEEKDAYS, 5 , NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthout+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_getSymbols(fr, UDAT_WEEKDAYS, 5, result, resultlength, &status);
        
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in udat_getSymbols().... %s\n", myErrorName(status) );
    }
    else
        log_verbose("PASS: getSymbols succesful\n");

    if(u_strcmp(result, pattern)==0)
        log_verbose("PASS: getSymbols retrieved the right value\n");
    else
        log_err("FAIL: getSymbols retrieved the wrong value\n");

    /*run series of tests to test getsymbols regressively*/
    log_verbose("\nTesting getSymbols() regressively\n");
    VerifygetSymbols(fr, UDAT_WEEKDAYS, 1, "dimanche");
    VerifygetSymbols(def, UDAT_WEEKDAYS, 1, "Sunday");
    VerifygetSymbols(fr, UDAT_SHORT_WEEKDAYS, 7, "sam.");
    VerifygetSymbols(def, UDAT_SHORT_WEEKDAYS, 7, "Sat");
    VerifygetSymbols(def, UDAT_MONTHS, 11, "December");
    VerifygetSymbols(def, UDAT_MONTHS, 0, "January");
    VerifygetSymbols(fr, UDAT_ERAS, 0, "av. J.-C.");
    VerifygetSymbols(def, UDAT_AM_PMS, 0, "AM");
    VerifygetSymbols(def, UDAT_AM_PMS, 1, "PM");
    VerifygetSymbols(fr, UDAT_SHORT_MONTHS, 0, "janv.");
    VerifygetSymbols(def, UDAT_SHORT_MONTHS, 11, "Dec");
    VerifygetSymbols(def,UDAT_LOCALIZED_CHARS, 0, "GyMdkHmsSEDFwWahKzYe");


free(result);
free(pattern);    
    
    log_verbose("\nTesting setSymbols\n");
    /*applying the pattern so that setSymbolss works */
    resultlength=0;
    resultlengthout=udat_toPattern(fr, FALSE, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthout + 1;
        pattern=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_toPattern(fr, FALSE, pattern, resultlength, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in extracting the pattern from UNumberFormat\n %s\n", 
            myErrorName(status) );
    }
    
    udat_applyPattern(def, FALSE, pattern, u_strlen(pattern));
    resultlength=0;
    resultlengthout=udat_toPattern(def, FALSE, NULL, resultlength,&status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthout + 1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_toPattern(fr, FALSE,result, resultlength, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in extracting the pattern from UNumberFormat\n %s\n", 
            myErrorName(status) );
    }
    if(u_strcmp(result, pattern)==0)
        log_verbose("Pattern applied properly\n");
    else
        log_err("pattern could not be applied properly\n");

free(result);
free(pattern);
    /*testing set symbols */
    resultlength=0;
    resultlengthout=udat_getSymbols(fr, UDAT_MONTHS, 11 , NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR){
        status=U_ZERO_ERROR;
        resultlength=resultlengthout+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_getSymbols(fr, UDAT_MONTHS, 11, result, resultlength, &status);
        
    }
    if(U_FAILURE(status))
        log_err("FAIL: error in getSymbols() %s\n", myErrorName(status) );
    resultlength=resultlengthout+1;
    
    udat_setSymbols(def, UDAT_MONTHS, 11, result, resultlength, &status);
    if(U_FAILURE(status))
        {
            log_err("FAIL: Error in udat_setSymbols() : %s\n", myErrorName(status) );
        }
    else
        log_verbose("PASS: SetSymbols successful\n");
    
    resultlength=0;
    resultlengthout=udat_getSymbols(def, UDAT_MONTHS, 11, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR){
        status=U_ZERO_ERROR;
        resultlength=resultlengthout+1;
        value=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_getSymbols(def, UDAT_MONTHS, 11, value, resultlength, &status);
    }
    if(U_FAILURE(status))
        log_err("FAIL: error in retrieving the value using getSymbols i.e roundtrip\n");
    
    if(u_strcmp(result, value)!=0)
        log_err("FAIL: Error in settting and getting symbols\n");
    else
        log_verbose("PASS: setSymbols successful\n");
    
    
    /*run series of tests to test setSymbols regressively*/
    log_verbose("\nTesting setSymbols regressively\n");
    VerifysetSymbols(def, UDAT_WEEKDAYS, 1, "Sundayweek");
    VerifysetSymbols(def, UDAT_ERAS, 0, "BeforeChrist");
    VerifysetSymbols(def, UDAT_SHORT_WEEKDAYS, 7, "Satweek");
    VerifysetSymbols(fr, UDAT_MONTHS, 11, "december");
    VerifysetSymbols(fr, UDAT_SHORT_MONTHS, 0, "Jan");

    
    /*run series of tests to test get and setSymbols regressively*/
    log_verbose("\nTesting get and set symbols regressively\n");
    VerifygetsetSymbols(fr, def, UDAT_WEEKDAYS, 1);
    VerifygetsetSymbols(fr, def, UDAT_WEEKDAYS, 7);
    VerifygetsetSymbols(fr, def, UDAT_SHORT_WEEKDAYS, 1);
    VerifygetsetSymbols(fr, def, UDAT_SHORT_WEEKDAYS, 7);
    VerifygetsetSymbols(fr, def, UDAT_MONTHS, 0);
    VerifygetsetSymbols(fr, def, UDAT_SHORT_MONTHS, 0);
    VerifygetsetSymbols(fr, def, UDAT_ERAS,1);
    VerifygetsetSymbols(fr, def, UDAT_LOCALIZED_CHARS, 0);
    VerifygetsetSymbols(fr, def, UDAT_AM_PMS, 1);


    /*closing*/
    
    udat_close(fr);
    udat_close(def);
    free(result);
    free(value);
    
}

/*INTERNAL FUNCTIONS USED*/
void VerifygetSymbols(UDateFormat* datfor, UDateFormatSymbolType type, int32_t index, const char* expected)
{
    UChar *pattern;
    UErrorCode status = U_ZERO_ERROR;
    UChar *result;
    int32_t resultlength, resultlengthout;

    
    pattern=(UChar*)malloc(sizeof(UChar) * (strlen(expected)+1));
    u_uastrcpy(pattern, expected);
    resultlength=0;
    resultlengthout=udat_getSymbols(datfor, type, index , NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthout+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_getSymbols(datfor, type, index, result, resultlength, &status);
        
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in udat_getSymbols()... %s\n", myErrorName(status) );
        return;
    }
    if(u_strcmp(result, pattern)==0)
        log_verbose("PASS: getSymbols retrieved the right value\n");
    else{
        log_err("FAlL: getSymbols retrieved the wrong value\n Expected %s Got %s\n", austrdup(pattern), 
            austrdup(result) );
    }
    free(result);
    free(pattern);
}

void VerifysetSymbols(UDateFormat* datfor, UDateFormatSymbolType type, int32_t index, const char* expected)
{
    UChar *result;
    UChar *value;
    int32_t resultlength, resultlengthout;
    UErrorCode status = U_ZERO_ERROR;

    value=(UChar*)malloc(sizeof(UChar) * (strlen(expected) + 1));
    u_uastrcpy(value, expected);
    udat_setSymbols(datfor, type, index, value, u_strlen(value), &status);
    if(U_FAILURE(status))
        {
            log_err("FAIL: Error in udat_setSymbols()  %s\n", myErrorName(status) );
            return;
        }

    resultlength=0;
    resultlengthout=udat_getSymbols(datfor, type, index, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR){
        status=U_ZERO_ERROR;
        resultlength=resultlengthout+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_getSymbols(datfor, type, index, result, resultlength, &status);
    }
    if(U_FAILURE(status)){
        log_err("FAIL: error in retrieving the value using getSymbols after setting it previously\n %s\n", 
            myErrorName(status) );
        return;
    }
    
    if(u_strcmp(result, value)!=0){
        log_err("FAIL:Error in setting and then getting symbols\n Expected %s Got %s\n", austrdup(value),
            austrdup(result) );
    }
    else
        log_verbose("PASS: setSymbols successful\n");

    free(value);
    free(result);
}


void VerifygetsetSymbols(UDateFormat* from, UDateFormat* to, UDateFormatSymbolType type, int32_t index)
{
    UChar *result;
    UChar *value;
    int32_t resultlength, resultlengthout;
    UErrorCode status = U_ZERO_ERROR;
    
    resultlength=0;
    resultlengthout=udat_getSymbols(from, type, index , NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR){
        status=U_ZERO_ERROR;
        resultlength=resultlengthout+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_getSymbols(from, type, index, result, resultlength, &status);
    }
    if(U_FAILURE(status)){
        log_err("FAIL: error in getSymbols() %s\n", myErrorName(status) );
        return;
    }
    
    resultlength=resultlengthout+1;
    udat_setSymbols(to, type, index, result, resultlength, &status);
    if(U_FAILURE(status))
        {
            log_err("FAIL: Error in udat_setSymbols() : %s\n", myErrorName(status) );
            return;
        }

    resultlength=0;
    resultlengthout=udat_getSymbols(to, type, index, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR){
        status=U_ZERO_ERROR;
        resultlength=resultlengthout+1;
        value=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_getSymbols(to, type, index, value, resultlength, &status);
    }
    if(U_FAILURE(status)){
        log_err("FAIL: error in retrieving the value using getSymbols i.e roundtrip\n %s\n", 
            myErrorName(status) );
        return;
    }
    
    if(u_strcmp(result, value)!=0){
        log_err("FAIL:Error in setting and then getting symbols\n Expected %s Got %s\n", austrdup(result),
            austrdup(value) );
    }
    else
        log_verbose("PASS: setSymbols successful\n");

    free(value);
    free(result);
}



UChar* myNumformat(const UNumberFormat* numfor, double d)
{
    UChar *result2;
    int32_t resultlength, resultlengthneeded;
    UFieldPosition pos;
    UErrorCode status = U_ZERO_ERROR;
    
    resultlength=0;
    resultlengthneeded=unum_formatDouble(numfor, d, NULL, resultlength, &pos, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result2=(UChar*)malloc(sizeof(UChar) * resultlength);
        unum_formatDouble(numfor, d, result2, resultlength, &pos, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in formatting using unum_format(.....) %s\n", myErrorName(status) );
        return 0;
    }
    
    return result2;
}
