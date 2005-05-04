/**
*******************************************************************************
* Copyright (C) 2005, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
package com.ibm.icu.text;

import java.io.InputStream;
import java.io.Reader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Collections;
import java.util.Arrays;


/**
 *
 * <code>CharsetDetector</code> provides a facility for detecting the
 * charset or encoding of character data in an unknown format.
 * The input data can either be from an input stream or an array of bytes.
 * The result of the detection operation is a list of possibly matching
 * charsets, or, for simple use, you can just ask for a Java Reader that
 * will will work over the input data.
 * <p/>
 * Character set detection is at best an imprecise operation.  The detection
 * process will attempt to identify the charset that best matches the characteristics
 * of the byte data, but the process is partly statistical in nature, and
 * the results can not be guaranteed to always be correct.
 * <p/>
 * For best accuracy in charset detection, the input data should be primarily
 * in a single language, and a minimum of a few hundred bytes worth of plain text
 * in the language are needed.  The detection process will attempt to
 * ignore html or xml style markup that could otherwise obscure the content.
 * <p/>
 * <b>Question:</b>Should we have getters corresponding to the setters for inut text
 * and declared encoding?
 * <p/>
 * <b>A thought:</b>  If we were to create our own type of Java Reader, we could defer
 * figuring out an actual charset for data that starts out with too much English
 *  only ASCII until the user actually read through to something that didn't look
 * like 7 bit English.  If  nothing else ever appeared, we would never need to
 *  actually choose the "real" charset.  All assuming that the application just
 *   wants the data, and doesn't care about a char set name.
 *
 *
 */
public class CharsetDetector {


    /**
     *   Constructor
     */
    public CharsetDetector() {
    }

    /**
     * Set the declared encoding for charset detection.
    *  The declared encoding of an input text is an encoding obtained
    *  from an http header or xml declaration or similar source that
    *  can be provided as additional information to the charset detector.  
    *  A match between a declared encoding and a possible detected encoding
    *  will raise the quality of that detected encoding by a small delta,
    *  and will also appear as a "reason" for the match.
    * <p/>
    * A declared encoding that is incompatible with the input data being
    * analyzed will not be added to the list of possible encodings.
    * 
    *  @param encoding The declared encoding 
    */
    public CharsetDetector setDeclaredEncoding(String encoding) {
        fDeclaredEncoding = encoding;
        return this;
    }
    
    /**
     * Set the input text (byte) data whose charset is to be detected.
     * @param in the input text of unknown encoding
     * @return This CharsetDetector
     */
    public CharsetDetector setText(byte [] in) {
        fRawInput  = in;
        fRawLength = in.length;      
        return this;
    }
    
    /**
     * Set the input text (byte) data whose charset is to be detected.
     *  <p/>
     *   The input stream that supplies the character data must have markSupported()
     *   == true; the charset detection process will read a small amount of data,
     *   then return the stream to its original position via
     *   the InputStream.reset() operation.  The exact amount that will
     *   be read depends on the characteristics of the data itself.

     * @param in the input text of unknown encoding
     * @return This CharsetDetector
     */
    public CharsetDetector setText(InputStream in) throws IOException {
        fInputStream = in;
        fInputStream.mark(4000);
        fRawInput = new byte[4000];       // Always make a new buffer because the
                                          //   previous one may have come from the caller,
                                          //   in which case we can't touch it.
        fRawLength = fInputStream.read(fRawInput);
        fInputStream.reset();
        return this;
    }

  
    /**
     * Return the charset that best matches the supplied input data.
     * 
     * Note though, that because the detection 
     * only looks at the start of the input data,
     * there is a possibility that the returned charset will fail to handle
     * the full set of input data.
     * <p/>
     * Raise an exception if 
     *  <ul>
     *    <li>no charset appears to match the data.</li>
     *    <li>no input text has been provided</li>
     *  </ul>
     *
     * @return a CharsetMatch object representing the best matching charset.
     * *
     * TODO:  A better implementation would be to copy the detect loop from
     *        detectAll(), and cut it short as soon as a match with a high confidence
     *        is found.  This is something to be done later, after things are otherwise
     *        working.
     */
    public CharsetMatch detect() {
        return detectAll()[0];
     }
    
    /**
     *  Return an array of all charsets that appear to be plausible
     *  matches with the input data.  The array is ordered with the
     *  best quality match first.
     * <p/>
     * Raise an exception if 
     *  <ul>
     *    <li>no charsets appear to match the input data.</li>
     *    <li>no input text has been provided</li>
     *  </ul>
      * 
     * @return An array of CharsetMatch objects representing possibly matching charsets.
     */
    public CharsetMatch[] detectAll() {
        CharsetRecognizer csr;
        int               i;
        int               detectResults;
        int               confidence;
        ArrayList         matches = new ArrayList();
        
        //  Iterate over all possible charsets, remember all that
        //    give a match quality > 0.
        for (i=0; i<fCSRecognizers.size(); i++) {
            csr = (CharsetRecognizer)fCSRecognizers.get(i);
            detectResults = csr.match(this);
            confidence = detectResults & 0x000000ff;
            if (confidence > 0) {
                CharsetMatch  m = new CharsetMatch(this, csr, confidence);
                matches.add(m);
            }
        }
        Collections.sort(matches);      // CharsetMatch compares on confidence
        Collections.reverse(matches);   //  Put best match first.
        CharsetMatch [] resultArray = new CharsetMatch[matches.size()];
        resultArray = (CharsetMatch[]) matches.toArray(resultArray);
        return resultArray;
    }

    
    /**
     * Autodetect the charset of an inputStream, and return a Java Reader
     * to access the converted input data.
     * <p/>
     * This is a convenience method that is equivalent to
     *   <code>this.setDeclaredEncoding(declaredEncoding).setText(in).detect().getReader();</code>
     * <p/>
     *   For the input stream that supplies the character data, markSupported()
     *   must be true; the  charset detection will read a small amount of data,
     *   then return the stream to its original position via
     *   the InputStream.reset() operation.  The exact amount that will
     *    be read depends on the characteristics of the data itself.
     *<p/>
     * Raise an exception if no charsets appear to match the input data.
     * 
     * @param in The source of the byte data in the unknown charset.
     *
     * @param declaredEncoding  A declared encoding for the data, if available,
     *           or null or an empty string if none is available.
     */
    public Reader getReader(InputStream in, String declaredEncoding) {
        return null;
    }

    /**
     * Autodetect the charset of an inputStream, and return a String
     * containing the converted input data.
     * <p/>
     * This is a convenience method that is equivalent to
     *   <code>this.setDeclaredEncoding(declaredEncoding).setText(in).detect().getString();</code>
     *<p/>
     * Raise an exception if no charsets appear to match the input data.
     * 
     * @param in The source of the byte data in the unknown charset.
     *
     * @param declaredEncoding  A declared encoding for the data, if available,
     *           or null or an empty string if none is available.
     */
    public String getString(byte[] in, String declaredEncoding) {
        return null;
    }

 
    /**
     * Get the names of all char sets that can be recognized by the char set detector.
     *
     * @return an array of the names of all charsets that can be recognized
     * by the charset detector.
     */
    public static String[] getAllDetectableCharsets() {
        return fCharsetNames;
    }
    

    /**
     *  MungeInput - after getting a set of raw input data to be analyzed, preprocess
     *               it by removing what appears to be html markup.
     */
    private void MungeInput() {
        int srci = 0;
        int dsti = 0;
        byte b;
        boolean  inMarkup = false;
        int      openTags = 0;
        int      badTags  = 0;
        
        //
        //  html / xml markup stripping.
        //     quick and dirty, not 100% accurate, but hopefully good enough, statistically.
        //     discard everything within < brackets >
        //     Count how many total '<' and illegal (nested) '<' occur, so we can make some
        //     guess as to whether the input was actually marked up at all.
        for (srci=0; srci<fRawLength; srci++) {
            b = fRawInput[srci];
            if (b == (byte)'<') {
                if (inMarkup) {
                    badTags++;
                }
                inMarkup = true;
                openTags++;
            }
            if (inMarkup == false) {
                fInputBytes[dsti++] = b;
            }
            
            if (b == (byte)'>') {
                inMarkup = false;
            }        
        }
        fInputLen = dsti;
        
        //
        //  If it looks like this input wasn't marked up, or if it looks like it's
        //    essentially nothing but markup abandon the markup stripping.
        //    Detection will have to work on the unstripped input.
        //
        if (openTags<5 || openTags/5 < badTags || 
                (fInputLen < 100 && fRawLength>600)) {
            for (srci=0; srci<fRawLength; srci++) {
                fInputBytes[srci] = fRawInput[srci];
            }
            fInputLen = srci;
        }
        
        //
        // Tally up the byte occurence statistics.
        //   These are available for use by the various detectors.
        //
        Arrays.fill(fByteStats, (short)0);
        for (srci=0; srci<fInputLen; srci++) {
            int val = fInputBytes[srci] & 0x00ff;
            fByteStats[val]++;
        }        
     }

    /**
     *  The following items are accessed by individual CharsetRecongizers during
     *     the recognition process
     */
    byte[]      fInputBytes =     // The text to be checked.  Markup will have been
                   new byte[4000];//   removed if appropriate.
    
    int         fInputLen;        // Length of the byte data in fInputText.
    
    short       fByteStats[];     // byte frequency statistics for the input text.
                                  //   Value is percent, not absolute.
                                  //   Value is rounded up, so zero really means zero occurences.
    
    String      fDeclaredEncoding;
    
    

    //
    //  Stuff private to CharsetDetector
    //
    byte[]               fRawInput;     // Original, untouched input bytes.
                                        //  If user gave us a byte array, this is it.
                                        //  If user gave us a stream, it's read to a 
                                        //   buffer here.
    int                  fRawLength;    // Length of data in fRawInput array.
    
     InputStream         fInputStream;  // User's input stream, or null if the user
                                        //   gave us a byte array.
    
    
    /**
     *  List of recognizers for all charsets known to the implementation.
     *
     */
    private static ArrayList fCSRecognizers = createRecognizers();
    private static String [] fCharsetNames;
    
   /**
     * Create the singleton instances of the CharsetRecognizer classes
     */
    private static ArrayList createRecognizers() {
        ArrayList recognizers = new ArrayList();
        recognizers.add(new CharsetRecog_UTF8());
        recognizers.add(new CharsetRecog_mbcs.CharsetRecog_sjis());
        recognizers.add(new CharsetRecog_2022.CharsetRecog_2022JP());
        recognizers.add(new CharsetRecog_2022.CharsetRecog_2022CN());
        recognizers.add(new CharsetRecog_2022.CharsetRecog_2022KR());
        recognizers.add(new CharsetRecog_mbcs.CharsetRecog_euc.CharsetRecog_euc_cn());
        recognizers.add(new CharsetRecog_mbcs.CharsetRecog_euc.CharsetRecog_euc_jp());
        recognizers.add(new CharsetRecog_mbcs.CharsetRecog_euc.CharsetRecog_euc_kr());
        
        // Create an array of all charset names, as a side effect.
        // Needed for the getAllDetectableCharsets() API.
        fCharsetNames = new String [recognizers.size()];
        for (int i=0; i<recognizers.size(); i++) {
            fCharsetNames[i] = ((CharsetRecognizer)recognizers.get(i)).getName();          
        }
        return recognizers;
    }
}
