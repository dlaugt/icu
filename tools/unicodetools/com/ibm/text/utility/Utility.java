/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/unicodetools/com/ibm/text/utility/Utility.java,v $
* $Date: 2005/11/01 00:10:53 $
* $Revision: 1.51 $
*
*******************************************************************************
*/

package com.ibm.text.utility;

import java.util.*;
import java.text.*;
import java.io.*;

import com.ibm.icu.text.Transliterator;
import com.ibm.icu.text.UnicodeSet;
import com.ibm.icu.text.UTF16;
import com.ibm.icu.text.Replaceable;
import com.ibm.icu.text.ReplaceableString;
import com.ibm.icu.text.UnicodeMatcher;
import com.ibm.icu.dev.test.util.UnicodeMap;
import com.ibm.icu.dev.test.util.UnicodeProperty;

import com.ibm.text.UCD.*;

public final class Utility implements UCD_Types {    // COMMON UTILITIES

    // static final boolean UTF8 = true; // TODO -- make argument
    public static final char BOM = '\uFEFF';
    
    public static String[] append(String[] array1, String[] array2) {
        String[] temp = new String[array1.length + array2.length];
        System.arraycopy(array1, 0, temp, 0, array1.length);
        System.arraycopy(array2, 0, temp, array1.length, array2.length);
        return temp;
    }

    public static String[] subarray(String[] array1, int start, int limit) {
        String[] temp = new String[limit - start];
        System.arraycopy(array1, start, temp, 0, limit - start);
        return temp;
    }

    public static String[] subarray(String[] array1, int start) {
        return subarray(array1, start, array1.length);
    }

    public static String getName(int i, String[] names) {
        try {
            return names[i];
        } catch (Exception e) {
            return "UNKNOWN";
        }
    }

    private static boolean needCRLF = false;
    
    public static int DOTMASK = 0x7FF;

    public static void dot(int i) {
        if ((i % DOTMASK) == 0) {
            needCRLF = true;
            System.out.print('.');
        }
    }

    public static void fixDot() {
        if (needCRLF) {
            System.out.println();
            needCRLF = false;
        }
    }

    public static long setBits(long source, int start, int end) {
        if (start < end) {
            int temp = start;
            start = end;
            end = temp;
        }
        long bmstart = (1L << (start+1)) - 1;
        long bmend = (1L << end) - 1;
        bmstart &= ~bmend;
        return source |= bmstart;
    }

    public static long setBit(long source, int start) {
        return setBits(source, start, start);
    }

    public static long clearBits(long source, int start, int end) {
        if (start < end) {
            int temp = start;
            start = end;
            end = temp;
        }
        int bmstart = (1 << (start+1)) - 1;
        int bmend = (1 << end) - 1;
        bmstart &= ~bmend;
        return source &= ~bmstart;
    }

    public static long clearBit(long source, int start) {
        return clearBits(source, start, start);
    }

    public static int find(String source, String[] target, boolean skeletonize) {
        if (skeletonize) source = getSkeleton(source);
        for (int i = 0; i < target.length; ++i) {
            if (source.equals(getSkeleton(target[i]))) return i;
        }
        return -1;
    }
    
    /**
     * These routines use the Java functions, because they only need to act on ASCII.
     * Removes space, _, and lowercases.
     */
    
    public static String getSkeleton(String source) {
        return UnicodeProperty.toSkeleton(source);
        /*
        skeletonBuffer.setLength(0);
        boolean gotOne = false;
        // remove spaces, '_', '-'
        // we can do this with char, since no surrogates are involved
        for (int i = 0; i < source.length(); ++i) {
            char ch = source.charAt(i);
            if (ch == '_' || ch == ' ' || ch == '-') {
                gotOne = true;
            } else {
                char ch2 = Character.toLowerCase(ch);
                if (ch2 != ch) {
                    gotOne = true;
                    skeletonBuffer.append(ch2);
                } else {
                    skeletonBuffer.append(ch);
                }
            }
        }
        if (!gotOne) return source; // avoid string creation
        return skeletonBuffer.toString();
        */
    }
    
    private static StringBuffer skeletonBuffer = new StringBuffer();
    
    /**
     * These routines use the Java functions, because they only need to act on ASCII
     * Changes space, - into _, inserts _ between lower and UPPER.
     */
    
    public static String getUnskeleton(String source, boolean titlecaseStart) {
        return UnicodeProperty.regularize(source, titlecaseStart);
        /*
        if (source == null) return source;
        if (source.equals("noBreak")) return source; // HACK
        StringBuffer result = new StringBuffer();
        int lastCat = -1;
        boolean haveFirstCased = true;
        for (int i = 0; i < source.length(); ++i) {
            char c = source.charAt(i);
            if (c == ' ' || c == '-' || c == '_') {
                c = '_';
                haveFirstCased = true;
            }
            if (c == '=') haveFirstCased = true;
            int cat = Character.getType(c);
            if (lastCat == Character.LOWERCASE_LETTER && cat == Character.UPPERCASE_LETTER) {
                result.append('_');
            }
            if (haveFirstCased && (cat == Character.LOWERCASE_LETTER 
                    || cat == Character.TITLECASE_LETTER || cat == Character.UPPERCASE_LETTER)) {
                if (titlecaseStart) {
                    c = Character.toUpperCase(c);
                }
                haveFirstCased = false;
            }
            result.append(c);
            lastCat = cat;
        }
        return result.toString();
        */
    }
    
    public static String findSubstring(String source, Set target, boolean invert) {
        Iterator it = target.iterator();
        while (it.hasNext()) {
            String other = it.next().toString();
            if ((other.indexOf(source) >= 0) == invert) return other;
        }
        return null;
    }

    public static byte lookup(String source, String[] target, boolean skeletonize) {
        int result = Utility.find(source, target, skeletonize);
        if (result != -1) return (byte)result;
        throw new ChainException("Could not find \"{0}\" in table [{1}]", new Object [] {source, target});
    }

    /**
     * Supplies a zero-padded hex representation of an integer (without 0x)
     */
    static public String hex(long i, int places) {
        if (i == Long.MIN_VALUE) return "-8000000000000000";
        boolean negative = i < 0;
        if (negative) {
            i = -i;
        }
        String result = Long.toString(i, 16).toUpperCase();
        if (result.length() < places) {
            result = "0000000000000000".substring(result.length(),places) + result;
        }
        if (negative) {
            return '-' + result;
        }
        return result;
    }

	public static String hex(long ch) {
	    return hex(ch,4);
	}

	public static String hex(byte ch) {
	    return hex(ch & 0xFF,2);
	}

	public static String hex(char ch) {
	    return hex(ch & 0xFFFF,4);
	}

	public static String hex(Object s) {
	    return hex(s, 4, " ");
	}

	public static String hex(Object s, int places) {
	    return hex(s, places, " ");
	}

	public static String hex(Object s, String separator) {
	    return hex(s, 4, separator);
	}

	public static String hex(Object o, int places, String separator) {
	    if (o == null) return "";
	    if (o instanceof Number) return hex(((Number)o).longValue(), places);

	    String s = o.toString();
	    StringBuffer result = new StringBuffer();
	    int ch;
	    for (int i = 0; i < s.length(); i += UTF32.count16(ch)) {
	        if (i != 0) result.append(separator);
	        ch = UTF32.char32At(s, i);
	        result.append(hex(ch));
	    }
	    return result.toString();
	}

	public static String hex(byte[] o, int start, int end, String separator) {
	    StringBuffer result = new StringBuffer();
	    //int ch;
	    for (int i = start; i < end; ++i) {
	        if (i != 0) result.append(separator);
	        result.append(hex(o[i]));
	    }
	    return result.toString();
	}

	public static String hex(char[] o, int start, int end, String separator) {
	    StringBuffer result = new StringBuffer();
	    for (int i = start; i < end; ++i) {
	        if (i != 0) result.append(separator);
	        result.append(hex(o[i]));
	    }
	    return result.toString();
	}

    /**
     * Returns a string containing count copies of s.
     * If count <= 0, returns "".
     */
	public static String repeat(String s, int count) {
	    if (count <= 0) return "";
	    if (count == 1) return s;
	    StringBuffer result = new StringBuffer(count*s.length());
	    for (int i = 0; i < count; ++i) {
	        result.append(s);
	    }
	    return result.toString();
	}

    public static int intFrom(String p) {
        if (p.length() == 0) return Integer.MIN_VALUE;
        return Integer.parseInt(p);
    }

    public static long longFrom(String p) {
        if (p.length() == 0) return Long.MIN_VALUE;
        return Long.parseLong(p);
    }

    public static float floatFrom(String p) {
        if (p.length() == 0) return Float.NaN;
        int fract = p.indexOf('/');
        if (fract == -1) return Float.valueOf(p).floatValue();
        String q = p.substring(0,fract);
        float num = 0;
        if (q.length() != 0) num = Integer.parseInt(q);
        p = p.substring(fract+1,p.length());
        float den = 0;
        if (p.length() != 0) den = Integer.parseInt(p);
        return num/den;
    }

    public static double doubleFrom(String p) {
        if (p.length() == 0) return Double.NaN;
        int fract = p.indexOf('/');
        if (fract == -1) return Double.valueOf(p).doubleValue();
        String q = p.substring(0,fract);
        double num = 0;
        if (q.length() != 0) num = Integer.parseInt(q);
        p = p.substring(fract+1,p.length());
        double den = 0;
        if (p.length() != 0) den = Integer.parseInt(p);
        return num/den;
    }

    public static int codePointFromHex(String p) {
        String temp = Utility.fromHex(p);
        if (UTF32.length32(temp) != 1) throw new ChainException("String is not single (UTF32) character: " + p, null);
        return UTF32.char32At(temp, 0);
    }

    public static String fromHex(String p) {
    	return fromHex(p, false);
    }
    
    public static String fromHex(String p, boolean acceptChars) {
        StringBuffer output = new StringBuffer();
        int value = 0;
        int count = 0;
        main:
        for (int i = 0; i < p.length(); ++i) {
            char ch = p.charAt(i);
            int digit = 0;
            switch (ch) {
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                    digit = ch - 'a' + 10;
                    break;
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                    digit = ch - 'A' + 10;
                    break;
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
                case '8': case '9':
                    digit = ch - '0';
                    break;
                default:
                    int type = Character.getType(ch);
                    if (type != Character.SPACE_SEPARATOR) {
                    	if (acceptChars) {
                            if (count >= 4 && count <= 6) {
                                UTF32.append32(output, value);
                                count = 0;
                                value = 0;
                            } else if (count != 0) {
                            	output.append(p.substring(i-count, i)); // TODO fix supplementary characters
                            }
                            UTF32.append32(output, ch);
                            continue main;
                   		
                    	}
                        throw new ChainException("bad hex value: '{0}' at position {1} in \"{2}\"",
                            new Object[] {String.valueOf(ch), new Integer(i), p});
                    }
                    // fall through!!
                case ' ': case ',': case ';': // do SPACE here, just for speed
                    if (count != 0) {
                    	if (count < 4 || count > 6) {
                    		if (acceptChars) output.append(p.substring(i-count, i));
                    		else throw new ChainException("bad hex value: '{0}' at position {1} in \"{2}\"",
                                    new Object[] {String.valueOf(ch), new Integer(i), p});
                    	} else {
                    		UTF32.append32(output, value);
                    	}
                    }
                    count = 0;
                    value = 0;
                    continue main;
            }
            value <<= 4;
            value += digit;
            if (value > 0x10FFFF) {
                throw new ChainException("Character code too large: '{0}' at position {1} in \"{2}\"",
                    new Object[] {String.valueOf(ch), new Integer(i), p});
            }
            count++;
        }
        if (count != 0) {
           	if (count < 4 || count > 6) {
           		if (acceptChars) output.append(p.substring(p.length()-count, p.length()));
        		else throw new ChainException("bad hex value: '{0}' at position {1} in \"{2}\"",
                        new Object[] {"EOS", new Integer(p.length()), p});
        	} else {
        		UTF32.append32(output, value);
        	}
        }
        return output.toString();
    }
    
    
    public static final class Position {
        public int start, limit;
    }
    
    /**
     * Finds the next position in the text that matches.
     * @param divider A UnicodeMatcher, such as a UnicodeSet.
     * @text obvious
     * @offset starting offset
     * @output start and limit of the piece found. If the return is false, then start,limit = length
     * @return true iff match found
     */
    public static boolean next(UnicodeMatcher matcher, Replaceable text, int offset,
      Position output) {
        int[] io = new int[1]; // TODO replace later; extra object creation
        int limit = text.length();
        // don't worry about surrogates; matcher will handle
        for (int i = offset; i <= limit; ++i) {
            io[0] = i;
            if (matcher.matches(text, io, limit, false) == UnicodeMatcher.U_MATCH) {
                // a hit, return
                output.start = i;
                output.limit = io[0];
                return true;
            }
        }
        output.start = output.limit = limit;
        return false;
    }

    /**
     * Finds the next position in the text that matches.
     * @param divider A UnicodeMatcher, such as a UnicodeSet.
     * @text obvious
     * @offset starting offset
     * @output start and limit of the piece found. If the return is false, then start,limit = 0
     * @return true iff match found
     */
    public static boolean previous(UnicodeMatcher matcher, Replaceable text, int offset,
      Position output) {
        int[] io = new int[1]; // TODO replace later; extra object creation
        int limit = 0;
        // don't worry about surrogates; matcher will handle
        for (int i = offset; i >= limit; --i) {
            io[0] = i;
            if (matcher.matches(text, io, offset, false) == UnicodeMatcher.U_MATCH) {
                // a hit, return
                output.start = i;
                output.limit = io[0];
                return true;
            }
        }
        output.start = output.limit = limit;
        return false;
    }

    /**
     * Splits a string containing divider into pieces, storing in output
     * and returns the number of pieces. The string does not have to be terminated:
     * the segment after the last divider is returned in the last output element.
     * Thus if the string has no dividers, then the whole string is returned in output[0]
     * with a return value of 1.
     * @param divider A UnicodeMatcher, such as a UnicodeSet.
     * @param s the text to be divided
     * @param output where the resulting pieces go
     * @return the number of items put into output
     */
	public static int split(UnicodeMatcher divider, Replaceable text, Position[] output) {
	    int index = 0;
	    for (int offset = 0;; offset = output[index-1].limit) {
	        if (output[index] == null) output[index] = new Position();
	        boolean matches = next(divider, text, offset, output[index++]);
	        if (!matches) return index;
	    }
	}

    /**
     * Splits a string containing divider into pieces, storing in output
     * and returns the number of pieces.
     */
	public static int split(String s, char divider, String[] output, boolean trim) {
	    try {
            int last = 0;
            int current = 0;
            int i;
            for (i = 0; i < s.length(); ++i) {
                if (s.charAt(i) == divider) {
                    String temp = s.substring(last,i);
                    if (trim) temp = temp.trim();
                    output[current++] = temp;
                    last = i+1;
                }
            }
            String temp = s.substring(last,i);
            if (trim) temp = temp.trim();
            output[current++] = temp;
            int result = current;
            while (current < output.length) {
                output[current++] = "";
            }
            return result;
        } catch (RuntimeException e) {
            throw new RuntimeException("Failure at line: " + s, e);
        }
	}

    public static String[] split(String s, char divider) {
        return split(s,divider,false);
    }
    public static int split(String s, char divider, String[] output) {
        return split(s,divider,output,false);
    }
    
	public static String[] split(String s, char divider, boolean trim) {
	    String[] result = new String[100]; // HACK
	    int count = split(s, divider, result, trim);
	    return extract(result, 0, count);
	}

	public static String[] extract(String[] source, int start, int limit) {
	    String[] result = new String[limit-start];
	    System.arraycopy(source, start, result, 0, limit - start);
	    return result;
	}

	/*
	public static String quoteJava(String s) {
	    StringBuffer result = new StringBuffer();
	    for (int i = 0; i < s.length(); ++i) {
	        result.append(quoteJava(s.charAt(i)));
	    }
	    return result.toString();
	}
	*/
	public static String quoteJavaString(String s) {
	    if (s == null) return "null";
	    StringBuffer result = new StringBuffer();
	    result.append('"');
	    for (int i = 0; i < s.length(); ++i) {
	        result.append(quoteJava(s.charAt(i)));
	    }
	    result.append('"');
	    return result.toString();
	}

	public static String quoteJava(int c) {
	    switch (c) {
	      case '\\':
	        return "\\\\";
	      case '"':
	        return "\\\"";
	      case '\r':
	        return "\\r";
	      case '\n':
	        return "\\n";
	      default:
            if (c >= 0x20 && c <= 0x7E) {
                return String.valueOf((char)c);
            } else if (UTF32.isSupplementary(c)) {
                return "\\u" + hex((char)UTF32.getLead(c),4) + "\\u" + hex((char)UTF32.getTrail(c),4);
            } else {
                return "\\u" + hex((char)c,4);
            }
        }
	}

    public static String quoteXML(int c, boolean HTML) {
        switch (c) {
            case '<': return "&lt;";
            case '>': return "&gt;";
            case '&': return "&amp;";
            case '\'': if (!HTML) return "&apos;";
            	break;
            case '"': return "&quot;";

            // fix controls, since XML can't handle

            // also do this for 09, 0A, and 0D, so we can see them.
            case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
            case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
            case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
            case 0x7F:

             // fix noncharacters, since XML can't handle
            case 0xFFFE: case 0xFFFF:

                return "<codepoint hex=\"" + hex(c,1) + "\"/>";
        }

        // fix surrogates, since XML can't handle
        if (UTF32.isSurrogate(c)) {
            return "<codepoint hex=\"" + hex(c,1) + "\"/>";
        }

        if (c <= 0x7E) {
            return UTF32.valueOf32(c);
        }

        // fix supplementaries & high characters, because of IE bug
        /*if (UTF32.isSupplementary(c) || 0xFFF9 <= c && c <= 0xFFFD) {
            return "#x" + hex(c,1) + ";";
        }
        */

        return "&#x" + hex(c,1) + ";";
    }

    public static String quoteXML(String source, boolean HTML) {
        if (source == null) return "null";
        StringBuffer result = new StringBuffer();
        for (int i = 0; i < source.length(); ++i) {
            int c = UTF32.char32At(source, i);
            if (UTF32.isSupplementary(c)) ++i;
            result.append(quoteXML(c, HTML));
        }
        return result.toString();
    }
    
    public static String quoteXML(String source) {
    	return quoteXML(source, false);
    }
    
    public static String quoteXML(int source) {
    	return quoteXML(source, false);
    }
    
    private static UCDProperty defaultIgnorable = null;
    
    public static String getDisplay(int cp) {
        String result = UTF16.valueOf(cp);
        byte cat = Default.ucd().getCategory(cp);
        if (cat == Mn || cat == Me) {
            result = String.valueOf(DOTTED_CIRCLE) + result;
        } else if (cat == Cf || cat == Cc || cp == 0x034F || cp == 0x00AD || cp == 0x1806) {
            result = "\u25A1";
        } else {
            if (defaultIgnorable == null) defaultIgnorable = DerivedProperty.make(DefaultIgnorable);
            if (defaultIgnorable.hasValue(cp)) result = "\u25A1";
        }
        return result;
    }
    
    public static int compare(char[] a, int aStart, int aEnd, char[] b, int bStart, int bEnd) {
        while (aStart < aEnd && bStart < bEnd) {
            int diff = a[aStart++] - b[bStart++];
            if (diff != 0) return diff;
        }
        return (aEnd - aStart) - (bEnd - bStart);
    }

    public static int compare(byte[] a, int aStart, int aEnd, byte[] b, int bStart, int bEnd) {
        while (aStart < aEnd && bStart < bEnd) {
            int diff = a[aStart++] - b[bStart++];
            if (diff != 0) return diff;
        }
        return (aEnd - aStart) - (bEnd - bStart);
    }

    public static int compareUnsigned(byte[] a, int aStart, int aEnd, byte[] b, int bStart, int bEnd) {
        while (aStart < aEnd && bStart < bEnd) {
            int diff = (a[aStart++] & 0xFF) - (b[bStart++] & 0xFF);
            if (diff != 0) return diff;
        }
        return (aEnd - aStart) - (bEnd - bStart);
    }

    /**
     * Joins an array together, using divider between the pieces
     */
    public static String join(int[] array, String divider) {
        String result = "{";
        for (int i = 0; i < array.length; ++i) {
            if (i != 0) result += divider;
            result += array[i];
        }
        return result + "}";
    }

    public static String join(long[] array, String divider) {
        String result = "{";
        for (int i = 0; i < array.length; ++i) {
            if (i != 0) result += divider;
            result += array[i];
        }
        return result + "}";
    }

    public static final String[] searchPath = {
        "EXTRAS",
        "5.0.0",
        "4.1.0",
        "4.0.1",
        "4.0.0",
        "3.2.0",
        "3.1.1",
        "3.1.0",
        "3.0.1",
        "3.0.0",
        "2.1.9",
        "2.1.8",
        "2.1.5",
        "2.1.2",
        "2.0.0",
        "1.1.0",
    };

    /*public static PrintWriter openPrintWriter(String filename) throws IOException {
        return openPrintWriter(filename, LATIN1_UNIX);
    }
    */
    
    public static final class Encoding extends PoorMansEnum {
        private static PoorMansEnum.EnumStore store = new PoorMansEnum.EnumStore();
        
        /* Boilerplate */
        public Encoding next() { return (Encoding) next; }
        public void getAliases(Collection output) { store.getAliases(this, output); }
        public static Encoding get(String s) { return (Encoding) store.get(s); }
        public static Encoding get(int v) { return (Encoding) store.get(v); }
        public static int getMax() { return store.getMax(); }
        
        private Encoding() {}
        private static Encoding add(String name) { return (Encoding) store.add(new Encoding(), name);}
    }
    
    public static final Encoding
        LATIN1_UNIX = Encoding.add("LATIN1_UNIX"),
        LATIN1_WINDOWS = Encoding.add("LATIN1_WINDOWS"),
        UTF8_UNIX = Encoding.add("UTF8_UNIX"),
        UTF8_WINDOWS = Encoding.add("UTF8_WINDOWS"),
        
        //UTF8 = Encoding.add("UTF8"), // for read-only
        //LATIN1 = Encoding.add("LATIN1"), // for read-only
        
        // read-only (platform doesn't matter, since it is only line-end)
        
        UTF8 = UTF8_WINDOWS,
        LATIN1 = LATIN1_WINDOWS,
        
        FIRST = LATIN1_UNIX;
        
    
   /*
    public static final Encoding 
        LATIN1_UNIX = Encoding.LATIN1_UNIX,
        LATIN1_WINDOWS = Encoding.LATIN1_WINDOWS, 
        UTF8_UNIX = Encoding.UTF8_UNIX, 
        UTF8_WINDOWS = Encoding.UTF8_WINDOWS;
   */
   
    public static PrintWriter openPrintWriter(String filename, Encoding options) throws IOException {
        return openPrintWriter(UCD_Types.GEN_DIR, filename, options);
    }
    // Normally use false, false.
    // But for UCD files use true, true
    // Or if they are UTF8, use true, false
    public static PrintWriter openPrintWriter(String directory, String filename, Encoding options) throws IOException {
        File file = new File(directory + filename);
        Utility.fixDot();
        System.out.println("Creating File: " + file.getCanonicalPath());
        File parent = new File(file.getParent());
        //System.out.println("Creating File: "+ parent);
        parent.mkdirs();
        return new PrintWriter(
                    new UTF8StreamWriter(
                        new FileOutputStream(file),
                        32*1024,
                        options == LATIN1_UNIX || options == UTF8_UNIX,
                        options == LATIN1_UNIX || options == LATIN1_WINDOWS));
    }
    
    public static String getOutputName(String filename) {
        return UCD_Types.GEN_DIR + filename;
    }
    
    public static void print(PrintWriter pw, Collection c, String separator) {
        print(pw, c, separator, null);
    }
    
    public interface Breaker {
        public String get(Object current, Object old);
        public boolean filter(Object current); // true is keep
    }
    
    public static void printMapOfCollection(PrintWriter pw, Map c, String mainSeparator, String itemSeparator, String subseparator) {
        Iterator it = c.keySet().iterator();
        boolean first = true;
        Object last = null;
        while (it.hasNext()) {
            Object key = it.next();
            Collection value = (Collection) c.get(key);
            if (first) {
                first = false;
            } else {
                pw.print(mainSeparator);
            }
            pw.print(key);
            pw.print(itemSeparator);
            print(pw, value, subseparator);
        }
    }
    
    public static int print(PrintWriter pw, Collection c, String separator, Breaker b) {
        Iterator it = c.iterator();
        int count = 0;
        boolean first = true;
        Object last = null;
        while (it.hasNext()) {
            Object obj = it.next();
            if (b != null && !b.filter(obj)) continue;
            if (first) {
                first = false;
            } else {
                pw.print(separator);
            }
            if (b != null) {
                pw.print(b.get(obj, last));
            } else {
                pw.print(obj);
            }
            count++;
            last = obj;
        }
        return count;
    }
    
    public static void print(PrintWriter pw, Map c, String pairSeparator, String separator, Breaker b) {
        Iterator it = c.keySet().iterator();
        boolean first = true;
        Object last = null;
        while (it.hasNext()) {
            Object obj = it.next();
            Object result = c.get(obj);
            if (b != null && !b.filter(obj)) continue;
            if (first) {
                first = false;
            } else {
                pw.print(separator);
            }
            if (b != null) {
                pw.print(b.get(obj, last) + pairSeparator + result);
            } else {
                pw.print(obj + pairSeparator + result);
            }
            last = obj;
        }
    }
    
    public static BufferedReader openReadFile(String filename, Encoding encoding) throws FileNotFoundException, UnsupportedEncodingException {
        FileInputStream fis = new FileInputStream(filename);
        InputStreamReader isr;
        if (encoding == UTF8_UNIX || encoding == UTF8_WINDOWS) {
            isr = new InputStreamReader(fis, "UTF8");
        } else {
            isr = new InputStreamReader(fis);
        }
        BufferedReader br = new BufferedReader(isr, 32*1024);
        return br;
    }
    
    public static void addCount(Map m, Object key, int count) {
        Integer oldCount = (Integer) m.get(key);
        if (oldCount == null) {
            m.put(key, new Integer(count));
            return;
        }
        m.put(key, new Integer(oldCount.intValue() + count));
    }
    
    public static void addToSet(Map m, Object key, Object value) {
        Collection set = (Collection) m.get(key);
        if (set == null) {
            set = new TreeSet();
            m.put(key, set);
        }
        set.add(value);
    }
        
    public static void addToList(Map m, Object key, Object value, boolean unique) {
        Collection set = (Collection) m.get(key);
        if (set == null) {
            set = new ArrayList();
            m.put(key, set);
        }
        if (!unique || !set.contains(value)) set.add(value);
    }

    public static String readDataLine(BufferedReader br) throws IOException {
    	return readDataLine(br, null);
    }
    
    public static String readDataLine(BufferedReader br, int[] count) throws IOException {
        String originalLine = "";
        String line = "";
        
        try {
            line = originalLine = br.readLine();
            if (line == null) return null;
            if (count != null) ++count[0];
            if (line.length() > 0 && line.charAt(0) == 0xFEFF) line = line.substring(1);
            int commentPos = line.indexOf('#');
            if (commentPos >= 0) line = line.substring(0, commentPos);
            line = line.trim();
        } catch (Exception e) {
            throw new ChainException("Line \"{0}\",  \"{1}\"", new String[] {originalLine, line}, e);
        }
        return line;
    }

    public static void appendFile(String filename, Encoding encoding, PrintWriter output) throws IOException {
    	appendFile(filename, encoding, output, null);
    }
    
    public static void appendFile(String filename, Encoding encoding, PrintWriter output, String[] replacementList) throws IOException {
        BufferedReader br = openReadFile(filename, encoding);
        /*
        FileInputStream fis = new FileInputStream(filename);
        InputStreamReader isr = (encoding == UTF8_UNIX || encoding == UTF8_WINDOWS) ? new InputStreamReader(fis, "UTF8") :  new InputStreamReader(fis);
        BufferedReader br = new BufferedReader(isr, 32*1024);
        */
        while (true) {
            String line = br.readLine();
            if (line == null) break;
            if (replacementList != null) {
            	for (int i = 0; i < replacementList.length; i += 2) {
            		line = replace(line, replacementList[i], replacementList[i+1]);
            	}
            }
            output.println(line);
        }
    }
    
    public static boolean renameIdentical(String file1, String file2, String batFile) throws IOException {
        if (file1 == null) {
            System.out.println("Null file");
            return false;
        }
        String lines[] = new String[2];
        boolean identical = filesAreIdentical(file1, file2, lines);
        if (identical) {
            renameIdentical(file2);
            if (batFile != null) renameIdentical(batFile);
            return true;
        } else {
            fixDot();
            System.out.println("Found difference in : " + file1 + ", " + file2);
            int diff = compare(lines[0], lines[1]);
            System.out.println(" File1: '" + lines[0].substring(0,diff) + "', '" + lines[0].substring(diff) + "'");
            System.out.println(" File2: '" + lines[1].substring(0,diff) + "', '" + lines[1].substring(diff) + "'");
            return false;
        }
    }
    
    public static boolean filesAreIdentical(String file1, String file2, String[] lines) throws IOException {
        BufferedReader br1 = new BufferedReader(new FileReader(file1), 32*1024);
        BufferedReader br2 = new BufferedReader(new FileReader(file2), 32*1024);
        String line1 = "";
        String line2 = "";
        try {
            for (int lineCount = 0; ; ++lineCount) {
                line1 = getLineWithoutFluff(br1, lineCount == 0);
                line2 = getLineWithoutFluff(br2, lineCount == 0);
                if (line1 == null) {
                    if (line2 == null) return true;
                    break;
                }
                if (!line1.equals(line2)) {
                    break;
                }
            }
            lines[0] = line1;
            lines[1] = line2;
            if (lines[0] == null) lines[0] = "<end of file>";
            if (lines[1] == null) lines[1] = "<end of file>";
            return false;
        } finally {
            br1.close();
            br2.close();
        }
    }
    
    static void renameIdentical(String file2) {
        File foo = new File(file2);
        File newName = new File(foo.getParent(), "UNCHANGED-" + foo.getName());
        if (newName.exists()) {
            for (int i = 1; newName.exists(); ++i) {
                newName = new File(foo.getParent(), "UNCHANGED" + i + "-" + foo.getName());
            }
        }
        System.out.println("IDENTICAL TO PREVIOUS, RENAMING : " + foo);
        System.out.println("TO : " + newName);
        boolean renameResult = foo.renameTo(newName);
        if (!renameResult) System.out.println("Couldn't rename!");
    }
    
    static String getLineWithoutFluff(BufferedReader br1, boolean first) throws IOException {
        while (true) {
            String line1 = br1.readLine();
            if (line1 == null) return line1;
            line1 = line1.trim();
            if (line1.length() == 0) continue;
            if (line1.equals("#")) continue;
            if (line1.startsWith("# Generated")) continue;
            if (line1.startsWith("# Date")) continue;
            if (line1.equals("# ================================================")) continue;
            if (first && line1.startsWith("#")) {
                first = false;
                continue;
            }
            return line1;
        }
    }    
    
    /** Returns -1 if strings are equal; otherwise the position they are different at
    */
    public static int compare(String a, String b) {
        int len = a.length();
        if (len > b.length()) len = b.length();
        for (int i = 0; i < len; ++i) {
            if (a.charAt(i) != b.charAt(i)) return i;
        }
        if (a.length() != b.length()) return len;
        return -1;
    }
    
    public static void copyTextFile(String filename, Encoding encoding, String newName, String[] replacementList) throws IOException {
        PrintWriter out = Utility.openPrintWriter(newName, UTF8_WINDOWS);
        appendFile(filename, encoding, out, replacementList);
        out.close();
    }

    public static void copyTextFile(String filename, Encoding encoding, String newName) throws IOException {
        copyTextFile(filename, encoding, newName, null);
    }

    public static BufferedReader openUnicodeFile(String filename, String version, boolean show, Encoding encoding) throws IOException {
        String name = getMostRecentUnicodeDataFile(filename, version, true, show);
        if (name == null) return null;
        return openReadFile(name, encoding); // new BufferedReader(new FileReader(name),32*1024);
    }

    public static String getMostRecentUnicodeDataFile(String filename, String version, 
    	      boolean acceptLatest, boolean show) throws IOException {
    	return getMostRecentUnicodeDataFile(filename, version, acceptLatest, show, ".txt");
    }
    
    public static String getMostRecentUnicodeDataFile(String filename, String version, 
      boolean acceptLatest, boolean show, String fileType) throws IOException {
        // get all the files in the directory

        int compValue = acceptLatest ? 0 : 1;
        for (int i = 0; i < searchPath.length; ++i) {
            if (version.length() != 0 && version.compareTo(searchPath[i]) < compValue) continue;

            String directoryName = UCD_Types.UCD_DIR + File.separator + searchPath[i] + "-Update" + File.separator;
            if (show) System.out.println("Trying: '" + directoryName + "', '" + filename + "'");
            String result = searchDirectory(new File(directoryName), filename, show, fileType);
            if (result != null) return result;
            
        }
        return null;
    }
    
    public static Set getDirectoryContentsLastFirst(File directory) {
        Set result = new TreeSet(new Comparator() {
                public int compare(Object a, Object b) {
                    return ((Comparable) b).compareTo(a);
                }
            });
        result.addAll(java.util.Arrays.asList(directory.list()));
        return result;
    }
    
    public static String searchDirectory(File directory, String filename, boolean show) throws IOException {
    	return searchDirectory(directory, filename, show, ".txt");
    }
    
    public static String searchDirectory(File directory, String filename, boolean show, String fileType) throws IOException {
        Iterator it = getDirectoryContentsLastFirst(directory).iterator();
        while (it.hasNext()) {
            String fn = (String) it.next();
            File foo = new File(directory + File.separator + fn);
            // System.out.println("\tChecking: '" + foo.getCanonicalPath() + "'");
            if (foo.isDirectory()) {
                String attempt = searchDirectory(foo, filename, show, fileType);
                if (attempt != null) return attempt;
            }
            if (fn.endsWith(fileType) && fn.startsWith(filename)) {
                if (show) System.out.println("\tFound: '" + fn + "'");
                return foo.getCanonicalPath();
            }
        }
        return null;
    }

    public static void writeHtmlHeader(PrintWriter log, String title) {
        log.println("<html><head>");
        log.println("<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>");
        log.println("<title>" + title + "</title>");
        log.println("<style><!--");
        log.println("table        { border-collapse: collapse; border: 1 solid blue }");
        log.println("td           { border: 1 solid blue; padding: 2 }");
        log.println("th           { border: 1 solid blue; padding: 2 }");
        log.println("--></style>");
        log.println("</head><body>");
    }
    
    /**
     * Replaces all occurances of piece with replacement, and returns new String
     */
    public static String replace(String source, String piece, String replacement) {
        if (source == null || source.length() < piece.length()) return source;
    	int pos = 0;
        while (true) {
            pos = source.indexOf(piece, pos);
            if (pos < 0) return source;
            source = source.substring(0,pos) + replacement + source.substring(pos + piece.length());
            pos += replacement.length();
        }
    }
    
    public static String replace(String source, String[][] replacements) {
        return replace(source, replacements, replacements.length);
    }    
    
    public static String replace(String source, String[][] replacements, int count) {
        for (int i = 0; i < count; ++i) {
            source = replace(source, replacements[i][0], replacements[i][1]);
        }
        return source;
    }    
    
    public static String replace(String source, String[][] replacements, boolean reverse) {
        if (!reverse) return replace(source, replacements);
        for (int i = 0; i < replacements.length; ++i) {
            source = replace(source, replacements[i][1], replacements[i][0]);
        }
        return source;
    }    
    
    public static String getStack() {
        Exception e = new Exception();
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        e.printStackTrace(pw);
        pw.flush();
        return "Showing Stack with fake " + sw.getBuffer().toString();
    }
    
    public static String getUnicodeImage(int cp) {
        String code = hex(cp, 4);
        return "<img alt='U+" + code + "' src='http://www.unicode.org/cgi-bin/refglyph?24-" + code + "' style='vertical-align:middle'>";
    }
    
    static PrintWriter showSetNamesPw;
    
    public static void showSetDifferences(String name1, UnicodeSet set1, String name2, UnicodeSet set2, boolean separateLines, UCD ucd) {
        if (showSetNamesPw == null) showSetNamesPw = new PrintWriter(System.out);
        showSetDifferences(showSetNamesPw, name1, set1, name2, set2, separateLines, false, null, ucd);
    }
    
    public static void showSetDifferences(PrintWriter pw, String name1, UnicodeSet set1, String name2, UnicodeSet set2, 
      boolean separateLines, boolean withChar, UnicodeMap names, UCD ucd) {
        
        UnicodeSet temp = new UnicodeSet(set1).removeAll(set2);
        pw.println();
        pw.println("In " + name1 + ", but not in " + name2 + ": ");
        showSetNames(pw, "\t",  temp,  separateLines,  false,  withChar, names, ucd);
        
        temp = new UnicodeSet(set2).removeAll(set1);
        pw.println();
        pw.println("Not in " + name1 + ", but in " + name2 + ": ");
        showSetNames(pw, "\t",  temp,  separateLines,  false,  withChar, names, ucd);
        
        temp = new UnicodeSet(set2).retainAll(set1);
        pw.println();
        pw.println("In both " + name1 + " and " + name2 + ": ");
        pw.println(temp.size() == 0 ? "<none>" : ""+ temp);
        pw.flush();
        // showSetNames(pw, "\t",  temp,  false,  false,  withChar, names, ucd);
    }
    
    public static void showSetNames(String prefix, UnicodeSet set, boolean separateLines, UCD ucd) {
        showSetNames(prefix,  set,  separateLines,  false,  false, ucd);
    }
    
    public static void showSetNames(String prefix, UnicodeSet set, boolean separateLines, boolean IDN, UCD ucd) {
        showSetNames(prefix,  set,  separateLines,  IDN,  false, ucd);
    }
    
    public static void showSetNames(PrintWriter pw, String prefix, UnicodeSet set, boolean separateLines, boolean IDN, UCD ucd) {
        showSetNames( pw,  prefix,  set,  separateLines,  IDN,  false, null, ucd);
    }
    
    public static void showSetNames(String prefix, UnicodeSet set, boolean separateLines, boolean IDN, boolean withChar, UCD ucd) {
        if (showSetNamesPw == null) showSetNamesPw = new PrintWriter(System.out);
        showSetNames(showSetNamesPw, prefix, set, separateLines, IDN, withChar, null, ucd);
    }
    
    static java.text.NumberFormat nf = java.text.NumberFormat.getInstance();
    
    public static void showSetNames(PrintWriter pw, String prefix, UnicodeSet set, boolean separateLines, boolean IDN, 
            boolean withChar, UnicodeMap names, UCD ucd) {
        if (set.size() == 0) {
            pw.println(prefix + "<none>");
            pw.flush();
            return;
        }
        boolean useHTML = false;
        int count = set.getRangeCount();
        for (int i = 0; i < count; ++i) {
            int start = set.getRangeStart(i);
            int end = set.getRangeEnd(i);
            if (separateLines || (IDN && isSeparateLineIDN(start,end,ucd))) {
                for (int cp = start; cp <= end; ++cp) {
                    if (!IDN) pw.println(prefix + ucd.getCode(cp)
                        + "\t# " 
                        + (useHTML ? "(" + getUnicodeImage(cp) + ") " : "")
                        + (withChar && (cp >= 0x20) ? "(" + UTF16.valueOf(cp) + ") " : "")
                        + (names != null ? names.getValue(cp) + " " : "")
                        + ucd.getName(cp)
                        + (useHTML ? "<br>" : ""));
                    else {
                        pw.println(prefix + Utility.hex(cp,4) + "; " + ucd.getName(cp));
                    }
                }
            } else {
                if (!IDN) {
                    pw.println(prefix + ucd.getCode(start)
                        + ((start != end) ? (".." + ucd.getCode(end)) : "")
                        + "\t# "
                        + (withChar && (start >= 0x20) ? " (" + UTF16.valueOf(start)
                            + ((start != end) ? (".." + UTF16.valueOf(end)) : "") + ") " : "")
                        + ucd.getName(start) + ((start != end) ? (".." + ucd.getName(end)) : "")
                    );
                } else {
                    
                    pw.println(prefix + Utility.hex(start,4)
                        + ((start != end) ? ("-" + Utility.hex(end,4)) : "")
                        + (ucd.isAssigned(start)
                           ? "; " + ucd.getName(start) + ((start != end) 
                                ? ("-" + ucd.getName(end)) 
                                : "") 
                           : "")
                    );
                }
            }
        }
        pw.println("Total: " + nf.format(set.size()));
        pw.flush();
    }
    
    private static boolean isSeparateLineIDN(int cp, UCD ucd) {
        if (ucd.hasComputableName(cp)) return false;
        int cat = ucd.getCategory(cp);
        if (cat == UCD_Types.Cn) return false;
        if (ucd.getCategory(cp) == UCD_Types.Cc && !ucd.getBinaryProperty(cp, UCD_Types.White_space)) return false;
        return true;
    }
    
    private static boolean isSeparateLineIDN(int start, int end, UCD ucd) {
        return (isSeparateLineIDN(start, ucd) || isSeparateLineIDN(end, ucd));
    }

    public static Transliterator createFromFile(String fileName, int direction, Transliterator pretrans) throws IOException {
        StringBuffer buffer = new StringBuffer();
        FileLineIterator fli = new FileLineIterator();
        fli.open(fileName, Utility.UTF8);
        fli.commentChar = FileLineIterator.NOTCHAR; // disable comments
        while (true) {
            String line = fli.read();
            if (line == null) break;
            if (line.startsWith("\uFEFF")) line = line.substring(1);
            if (pretrans != null) line = pretrans.transliterate(line);
            buffer.append(line);
            buffer.append("\r\n"); // separate with whitespace
        }
        fli.close();
        
        /*
        
        // read and concatenate all the lines
        FileInputStream fis = new FileInputStream(fileName);
        InputStreamReader isr = new InputStreamReader(fis, "UTF8");
        BufferedReader br = new BufferedReader(isr, 32*1024);
        while (true) {
            String line = br.readLine();
            if (line == null) break;
            if (line.length() > 0 && line.charAt(0) == '\uFEFF') line = line.substring(1); // strip BOM
            if (pretrans != null) line = pretrans.transliterate(line);
            buffer.append(line);
            buffer.append("\r\n"); // separate with whitespace
        }
        br.close();
        //System.out.println(buffer.toString());
        */
        
        // Transform file name into id
        String id = fileName;
        int pos = id.lastIndexOf('.');
        if (pos >= 0) id = id.substring(0, pos);
        //System.out.println(buffer);
        return Transliterator.createFromRules(id, buffer.toString(), direction);
    }

}