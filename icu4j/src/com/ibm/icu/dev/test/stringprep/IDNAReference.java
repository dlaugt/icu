/*
 *******************************************************************************
 * Copyright (C) 2003, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/dev/test/stringprep/IDNAReference.java,v $
 * $Date: 2003/08/21 23:42:25 $
 * $Revision: 1.1 $
 *
 *******************************************************************************
*/
package com.ibm.icu.dev.test.stringprep;

import com.ibm.icu.text.UCharacterIterator;
import com.ibm.icu.stringprep.ParseException;

/**
 * @author ram
 *
 * To change the template for this generated type comment go to
 * Window>Preferences>Java>Code Generation>Code and Comments
 */
public class IDNAReference {
    
    private static char[] ACE_PREFIX = new char[]{ 0x0078,0x006E,0x002d,0x002d } ;
    private static final int ACE_PREFIX_LENGTH  = 4;

    private static final int MAX_LABEL_LENGTH   = 63;
    private static final int HYPHEN             = 0x002D;
    private static final int CAPITAL_A          = 0x0041;
    private static final int CAPITAL_Z          = 0x005A;
    private static final int LOWER_CASE_DELTA   = 0x0020;
    private static final int FULL_STOP          = 0x002E;


    public static final int DEFAULT             = 0x0000;
    public static final int ALLOW_UNASSIGNED    = 0x0001;
    public static final int USE_STD3_RULES      = 0x0002;
    public static final NamePrepTransform transform = NamePrepTransform.getInstance();
  
    private static boolean startsWithPrefix(StringBuffer src){
        boolean startsWithPrefix = true;

        if(src.length() < ACE_PREFIX_LENGTH){
            return false;
        }
        for(int i=0; i<ACE_PREFIX_LENGTH;i++){
            if(toASCIILower(src.charAt(i)) != ACE_PREFIX[i]){
                startsWithPrefix = false;
            }
        }
        return startsWithPrefix;
    }

    private static char toASCIILower(char ch){
        if(CAPITAL_A <= ch && ch <= CAPITAL_Z){
            return (char)(ch + LOWER_CASE_DELTA);
        }
        return ch;
    }

    private static StringBuffer toASCIILower(StringBuffer src){
        StringBuffer dest = new StringBuffer();
        for(int i=0; i<src.length();i++){
            dest.append(toASCIILower(src.charAt(i)));
        }
        return dest;
    }

    private static int compareCaseInsensitiveASCII(StringBuffer s1, StringBuffer s2){
        char c1,c2;
        int rc;
        for(int i =0;/* no condition */;i++) {
            /* If we reach the ends of both strings then they match */
            if(i == s1.length()) {
                return 0;
            }

            c1 = s1.charAt(i);
            c2 = s2.charAt(i);
        
            /* Case-insensitive comparison */
            if(c1!=c2) {
                rc=(int)toASCIILower(c1)-(int)toASCIILower(c2);
                if(rc!=0) {
                    return rc;
                }
            }
        }
    }
    
    private static int getSeparatorIndex(char[] src,int start, int limit){
        for(; start<limit;start++){
            if(NamePrepTransform.isLabelSeparator(src[start])){
                return start;
            }
        }
        // we have not found the separator just return length
        return start;
    }
    
    private static boolean isLDHChar(int ch){
        // high runner case
        if(ch>0x007A){
            return false;
        }
        //[\\u002D \\u0030-\\u0039 \\u0041-\\u005A \\u0061-\\u007A]
        if( (ch==0x002D) || 
            (0x0030 <= ch && ch <= 0x0039) ||
            (0x0041 <= ch && ch <= 0x005A) ||
            (0x0061 <= ch && ch <= 0x007A)
          ){
            return true;
        }
        return false;
    }
        
    public static StringBuffer convertToASCII(String src, int options)
        throws ParseException{
        UCharacterIterator iter = UCharacterIterator.getInstance(src);
        return convertToASCII(iter,options);
    }
    public static StringBuffer convertToASCII(StringBuffer src, int options)
        throws ParseException{
        UCharacterIterator iter = UCharacterIterator.getInstance(src);
        return convertToASCII(iter,options);
    }
    public static StringBuffer convertToASCII(UCharacterIterator srcIter, int options)
                throws ParseException{
    
        char[] caseFlags = null;
    
        // the source contains all ascii codepoints
        boolean srcIsASCII  = true;
        // assume the source contains all LDH codepoints
        boolean srcIsLDH = true; 

        //get the options
        boolean useSTD3ASCIIRules = (boolean)((options & USE_STD3_RULES) != 0);
    
        int failPos = -1;
        // step 2
        //StringPrep prep = StringPrep.getNameprepInstance();
        StringBuffer processOut = transform.prepare(srcIter,options);
        int poLen = processOut.length();
        StringBuffer dest = new StringBuffer();
        // step 3 & 4
        for(int j=0;j<poLen;j++ ){
            char ch=processOut.charAt(j);
            if(ch > 0x7F){
                srcIsASCII = false;
            }
            // here we do not assemble surrogates
            // since we know that LDH code points
            // are in the ASCII range only
            if(isLDHChar(ch)==false){
                srcIsLDH = false;
                failPos = j;
            }
        }
    
        if(useSTD3ASCIIRules == true){
            // verify 3a and 3b
            if( srcIsLDH == false /* source contains some non-LDH characters */
                || processOut.charAt(0) ==  HYPHEN 
                || processOut.charAt(processOut.length()-1) == HYPHEN){

                /* populate the parseError struct */
                if(srcIsLDH==false){
                     throw new ParseException( "The input does not conform to the STD 3 ASCII rules",
                                              ParseException.STD3_ASCII_RULES_ERROR,
                                              processOut.toString(),
                                             (failPos>0) ? (failPos-1) : failPos);
                }else if(processOut.charAt(0) == HYPHEN){
                    throw new ParseException("The input does not conform to the STD 3 ASCII rules",
                                              ParseException.STD3_ASCII_RULES_ERROR,processOut.toString(),0);
     
                }else{
                     throw new ParseException("The input does not conform to the STD 3 ASCII rules",
                                              ParseException.STD3_ASCII_RULES_ERROR,
                                              processOut.toString(),
                                              (poLen>0) ? poLen-1 : poLen);

                }
            }
        }
        if(srcIsASCII){
            dest =  processOut;
        }else{
            // step 5 : verify the sequence does not begin with ACE prefix
            if(!startsWithPrefix(processOut)){

                //step 6: encode the sequence with punycode
                StringBuffer punyout = PunycodeReference.encode(processOut,caseFlags);
                
                // convert all codepoints to lower case ASCII
                StringBuffer lowerOut = toASCIILower(punyout);

                //Step 7: prepend the ACE prefix
                dest.append(ACE_PREFIX,0,ACE_PREFIX_LENGTH);
                //Step 6: copy the contents in b2 into dest
                dest.append(lowerOut);
            }else{
                throw new ParseException("The input does not start with the ACE Prefix.",
                                   ParseException.ACE_PREFIX_ERROR,processOut.toString(),0);
            }
        }
        if(dest.length() > MAX_LABEL_LENGTH){
            throw new ParseException("The labels in the input are too long. Length > 64.", 
                                    ParseException.LABEL_TOO_LONG_ERROR,dest.toString(),0);
        }
        return dest;
    }
    
    public static StringBuffer convertIDNtoASCII(UCharacterIterator iter,int options)
            throws ParseException{
            return convertIDNToASCII(iter.getText(), options);          
    }
    public static StringBuffer convertIDNtoASCII(StringBuffer str,int options)
            throws ParseException{
            return convertIDNToASCII(str.toString(), options);          
    }
    public static StringBuffer convertIDNToASCII(String src,int options)
            throws ParseException{
        char[] srcArr = src.toCharArray();
        StringBuffer result = new StringBuffer();
        int sepIndex=0;
        int oldSepIndex = 0;
        for(;;){
            sepIndex = getSeparatorIndex(srcArr,sepIndex,srcArr.length);
            UCharacterIterator iter = UCharacterIterator.getInstance(new String(srcArr,oldSepIndex,sepIndex-oldSepIndex));
            result.append(convertToASCII(iter,options));
            if(sepIndex==srcArr.length){
                break;
            }
            // increment the sepIndex to skip past the separator
            sepIndex++;
            oldSepIndex = sepIndex;
            result.append((char)FULL_STOP);
        }
        return result;
    }

    public static StringBuffer convertToUnicode(String src, int options)
           throws ParseException{
        UCharacterIterator iter = UCharacterIterator.getInstance(src);
        return convertToUnicode(iter,options);
    }
    public static StringBuffer convertToUnicode(StringBuffer src, int options)
           throws ParseException{
        UCharacterIterator iter = UCharacterIterator.getInstance(src);
        return convertToUnicode(iter,options);
    }   
    public static StringBuffer convertToUnicode(UCharacterIterator iter, int options)
           throws ParseException{

        char[] caseFlags = null;
        
        //get the options
        boolean useSTD3ASCIIRules = (boolean)((options & USE_STD3_RULES) != 0);

        // the source contains all ascii codepoints
        boolean srcIsASCII  = true;
        // assume the source contains all LDH codepoints
        boolean srcIsLDH = true; 
               
        int failPos = -1;
        int ch;
        int saveIndex = iter.getIndex();
        // step 1: find out if all the codepoints in src are ASCII  
        while((ch=iter.next())!= UCharacterIterator.DONE){
            if(ch>0x7F){
                srcIsASCII = false;
            }
            if((srcIsLDH = isLDHChar(ch))==false){
                failPos = iter.getIndex();
            }
        }
        StringBuffer processOut;
        
        if(srcIsASCII == false){
            // step 2: process the string
            iter.setIndex(saveIndex);
            processOut = transform.prepare(iter,options);

        }else{
            //just point to source
            processOut = new StringBuffer(iter.getText());
        }
        // TODO:
        // The RFC states that 
        // <quote>
        // ToUnicode never fails. If any step fails, then the original input
        // is returned immediately in that step.
        // </quote>
        
        //step 3: verify ACE Prefix
        if(startsWithPrefix(processOut)){

           //step 4: Remove the ACE Prefix
           String temp = processOut.substring(ACE_PREFIX_LENGTH,processOut.length());

           //step 5: Decode using punycode
           StringBuffer decodeOut = PunycodeReference.decode(new StringBuffer(temp),caseFlags);
        
            //step 6:Apply toASCII
            StringBuffer toASCIIOut = convertToASCII(decodeOut, options);

            //step 7: verify
            if(compareCaseInsensitiveASCII(processOut, toASCIIOut) !=0){
                throw new ParseException("The verification step prescribed by the RFC 3491 failed",
                                          ParseException.VERIFICATION_ERROR); 
             }

            //step 8: return output of step 5
            return decodeOut;
            
        }else{
            // verify that STD3 ASCII rules are satisfied
            if(useSTD3ASCIIRules == true){
                if( srcIsLDH == false /* source contains some non-LDH characters */
                    || processOut.charAt(0) ==  HYPHEN 
                    || processOut.charAt(processOut.length()-1) == HYPHEN){
    
                        if(srcIsLDH==false){
                            throw new ParseException("The input does not conform to the STD 3 ASCII rules",
                                                     ParseException.STD3_ASCII_RULES_ERROR,processOut.toString(),
                                                     (failPos>0) ? (failPos-1) : failPos);
                        }else if(processOut.charAt(0) == HYPHEN){
                            throw new ParseException("The input does not conform to the STD 3 ASCII rules",
                                                     ParseException.STD3_ASCII_RULES_ERROR,
                                                     processOut.toString(),0);
         
                        }else{
                            throw new ParseException("The input does not conform to the STD 3 ASCII rules",
                                                     ParseException.STD3_ASCII_RULES_ERROR,
                                                     processOut.toString(),
                                                     processOut.length());
    
                        }
                  }
            }
            // just return the source
            return new StringBuffer(iter.getText());
        }  
    }
    public static StringBuffer convertIDNToUnicode(UCharacterIterator iter, int options)
        throws ParseException{
        return convertIDNToUnicode(iter.getText(), options);
    }
    public static StringBuffer convertIDNToUnicode(StringBuffer str, int options)
        throws ParseException{
        return convertIDNToUnicode(str.toString(), options);
    }
    public static StringBuffer convertIDNToUnicode(String src, int options)
        throws ParseException{
            
        char[] srcArr = src.toCharArray();
        StringBuffer result = new StringBuffer();
        int sepIndex=0;
        int oldSepIndex=0;
        for(;;){
            sepIndex = getSeparatorIndex(srcArr,sepIndex,srcArr.length);
            UCharacterIterator iter = UCharacterIterator.getInstance(new String(srcArr,oldSepIndex,sepIndex-oldSepIndex));
            result.append(convertToUnicode(iter,options));
            if(sepIndex==srcArr.length){
                break;
            }
            // increment the sepIndex to skip past the separator
            sepIndex++;
            oldSepIndex = sepIndex;
            result.append((char)FULL_STOP);
        }
        return result;
    }
    //  TODO: optimize
    public static int compare(StringBuffer s1, StringBuffer s2, int options)
        throws ParseException{
        if(s1==null || s2 == null){
            throw new IllegalArgumentException("One of the source buffers is null");
        }
        StringBuffer s1Out = convertIDNToASCII(s1.toString(), options);
        StringBuffer s2Out = convertIDNToASCII(s2.toString(), options);
        return compareCaseInsensitiveASCII(s1Out,s2Out);
    }
    //  TODO: optimize
    public static int compare(String s1, String s2, int options)
        throws ParseException{
        if(s1==null || s2 == null){
            throw new IllegalArgumentException("One of the source buffers is null");
        }
        StringBuffer s1Out = convertIDNToASCII(s1, options);
        StringBuffer s2Out = convertIDNToASCII(s2, options);
        return compareCaseInsensitiveASCII(s1Out,s2Out);
    }
    //  TODO: optimize
    public static int compare(UCharacterIterator i1, UCharacterIterator i2, int options)
        throws ParseException{
        if(i1==null || i2 == null){
            throw new IllegalArgumentException("One of the source buffers is null");
        }
        StringBuffer s1Out = convertIDNToASCII(i1.getText(), options);
        StringBuffer s2Out = convertIDNToASCII(i2.getText(), options);
        return compareCaseInsensitiveASCII(s1Out,s2Out);
    }

}
