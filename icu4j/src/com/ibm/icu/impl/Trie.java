/*
******************************************************************************
* Copyright (C) 1996-2002, International Business Machines Corporation and   *
* others. All Rights Reserved.                                               *
******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/impl/Trie.java,v $
* $Date: 2002/04/25 22:19:25 $
* $Revision: 1.5 $
*
******************************************************************************
*/

package com.ibm.icu.impl;

import java.io.InputStream;
import java.io.DataInputStream;
import java.io.IOException;
import com.ibm.icu.text.UTF16;
import com.ibm.icu.lang.UCharacter;

/**
 * <p>A trie is a kind of compressed, serializable table of values 
 * associated with Unicode code points (0..0x10ffff).</p>
 * <p>This class defines the basic structure of a trie and provides methods 
 * to <b>retrieve the offsets to the actual data</b>.</p>
 * <p>Data will be the form of an array of basic types, char or int.</p>
 * <p>The actual data format will have to be specified by the user in the
 * inner static interface com.ibm.icu.impl.Trie.DataManipulate.</p>
 * <p>This trie implementation is optimized for getting offset while walking
 * forward through a UTF-16 string.
 * Therefore, the simplest and fastest access macros are the
 * fromLead() and fromOffsetTrail() methods.
 * The fromBMP() method are a little more complicated; they get offsets even
 * for lead surrogate codepoints, while the fromLead() method get special
 * "folded" offsets for lead surrogate code units if there is relevant data
 * associated with them.
 * From such a folded offsets, an offset needs to be extracted to supply
 * to the fromOffsetTrail() methods.
 * To handle such supplementary codepoints, some offset information are kept
 * in the data.</p>
 * <p>Methods in com.ibm.icu.impl.Trie.DataManipulate are called to retrieve 
 * that offset from the folded value for the lead surrogate unit.</p>
 * <p>For examples of use, see com.ibm.icu.impl.CharTrie or 
 * com.ibm.icu.impl.IntTrie.</p>
 * @author synwee
 * @see com.ibm.icu.impl.CharTrie
 * @see com.ibm.icu.impl.IntTrie
 * @since release 2.1, Jan 01 2002
 */
public abstract class Trie
{
    // public class declaration ----------------------------------------
    
    /**
    * Character data in com.ibm.impl.Trie have different user-specified format
    * for different purposes.
    * This interface specifies methods to be implemented in order for
    * com.ibm.impl.Trie, to surrogate offset information encapsulated within 
    * the data.
    * @draft 2.1
    */
    public static interface DataManipulate
    {
        /**
        * Called by com.ibm.icu.impl.Trie to extract from a lead surrogate's 
        * data
        * the index array offset of the indexes for that lead surrogate.
        * @param value data value for a surrogate from the trie, including the
        *        folding offset
        * @return data offset or 0 if there is no data for the lead surrogate
        * @draft 2.1
        */
        public int getFoldingOffset(int value);
    }
    
    // public methods --------------------------------------------------
    
    /**
     * Determines if this trie has a linear latin 1 array
     * @return true if this trie has a linear latin 1 array, false otherwise
     */
    public final boolean isLatin1Linear()
    {
    	return m_isLatin1Linear_;
    }

    // protected constructor -------------------------------------------

    /**
    * Trie constructor for CharTrie use.
    * @param inputStream ICU data file input stream which contains the
    *                        trie
    * @param datamanipulate object containing the information to parse the 
    *                       trie data
    * @exception IOException thrown when input stream does not have the
    *                        right header.
    * @draft 2.1
    */
    protected Trie(InputStream inputStream, 
                   DataManipulate  dataManipulate) throws IOException
    {
        DataInputStream input = new DataInputStream(inputStream);
        // Magic number to authenticate the data.
        int signature = input.readInt();
        m_options_    = input.readInt();
        
        if (!checkHeader(signature)) {
            throw new IllegalArgumentException("ICU data file error: Trie header authentication failed, please check if you have the most updated ICU data file");
        }
        
        m_dataManipulate_ = dataManipulate;
        m_isLatin1Linear_ = (m_options_ &
                             HEADER_OPTIONS_LATIN1_IS_LINEAR_MASK_) != 0;
        m_dataOffset_     = input.readInt();
        m_dataLength_     = input.readInt();
        unserialize(inputStream);
    }

    // protected data members ------------------------------------------

    /**
    * Shift size for shifting right the input index. 1..9
    * @draft 2.1
    */
    protected static final int INDEX_STAGE_1_SHIFT_ = 5;
    /**
    * Shift size for shifting left the index array values.
    * Increases possible data size with 16-bit index values at the cost
    * of compactability.
    * This requires blocks of stage 2 data to be aligned by
    * DATA_GRANULARITY.
    * 0..INDEX_STAGE_1_SHIFT
    * @draft 2.1
    */
    protected static final int INDEX_STAGE_2_SHIFT_ = 2;
    /**
    * Mask for getting the lower bits from the input index.
    * DATA_BLOCK_LENGTH_ - 1.
    * @draft 2.1
    */
    protected static final int INDEX_STAGE_3_MASK_ =
                                              (1 << INDEX_STAGE_1_SHIFT_) - 1;
    /**
    * Surrogate mask to use when shifting offset to retrieve supplementary
    * values
    * @draft 2.1
    */
    protected static final int SURROGATE_MASK_ = 0x3FF;                                              
    /**
    * Index or UTF16 characters
    * @draft 2.1
    */
    protected char m_index_[];
    /**
    * Internal TrieValue which handles the parsing of the data value.
    * This class is to be implemented by the user
    * @draft 2.1
    */
    protected DataManipulate m_dataManipulate_;
    /**
    * Start index of the data portion of the trie. CharTrie combines 
    * index and data into a char array, so this is used to indicate the 
    * initial offset to the data portion.
    * @draft 2.1
    */
    protected int m_dataOffset_;
    /**
    * Length of the data array 
    */
    protected int m_dataLength_;
    
    // protected methods -----------------------------------------------

    /**
    * Gets the offset to the data which the surrogate pair points to.
    * @param lead lead surrogate
    * @param trail trailing surrogate
    * @return offset to data
    * @draft 2.1
    */
    protected abstract int getSurrogateOffset(char lead, char trail);
    
    /**
    * Gets the value at the argument index
    * @param index value at index will be retrieved
    * @return 32 bit value 
    * @draft 2.1
    */
    protected abstract int getValue(int index);

    /**
    * Gets the default initial value
    * @return 32 bit value 
    * @draft 2.1
    */
    protected abstract int getInitialValue();
    
    /**
    * Gets the offset to the data which the index ch after variable offset
    * points to.
    * Note for locating a non-supplementary character data offset, calling
    * <p>
    * getRawOffset(0, ch);
    * </p>
    * will do. Otherwise if it is a supplementary character formed by
    * surrogates lead and trail. Then we would have to call getRawOffset()
    * with getFoldingIndexOffset(). See getSurrogateOffset().
    * @param offset index offset which ch is to start from
    * @param ch index to be used after offset
    * @return offset to the data
    * @draft 2.1
    */
    protected final int getRawOffset(int offset, char ch)
    {
        return (m_index_[offset + (ch >> INDEX_STAGE_1_SHIFT_)] <<
                INDEX_STAGE_2_SHIFT_) + (ch & INDEX_STAGE_3_MASK_);
    }

    /**
    * Gets the offset to data which the BMP character points to
    * Treats a lead surrogate as a normal code point.
    * @param ch BMP character
    * @return offset to data
    * @draft 2.1
    */
    protected final int getBMPOffset(char ch)
    {
        int offset = 0;
        if (UTF16.isLeadSurrogate(ch)) {
            offset = LEAD_INDEX_OFFSET_;
        }
        return getRawOffset(offset, ch);
    }

    /**
    * Gets the offset to the data which this lead surrogate character points
    * to.
    * Data at the returned offset may contain folding offset information for
    * the next trailing surrogate character.
    * @param ch lead surrogate character
    * @return offset to data
    * @draft 2.1
    */
    protected final int getLeadOffset(char ch)
    {
       return getRawOffset(0, ch);
    }

    /**
    * Internal trie getter from a code point.
    * Could be faster(?) but longer with
    *   if((c32)<=0xd7ff) { (result)=_TRIE_GET_RAW(trie, data, 0, c32); }
    * Gets the offset to data which the codepoint points to
    * @param ch codepoint
    * @return offset to data
    * @draft 2.1
    */
    protected final int getCodePointOffset(int ch)
    {
        if (UCharacter.isBMP(ch)) {
            // BMP codepoint
            return getBMPOffset((char)ch);
        }
        if (ch <= UCharacter.MAX_VALUE)
        {
            char lead = UTF16.getLeadSurrogate(ch);
            // look at the construction of supplementary characters
            // trail forms the ends of it.
            return getSurrogateOffset(lead, (char)(ch & SURROGATE_MASK_));
        }
        // return -1 if there is an error, in this case we return the default
        // value: m_initialValue_
        return -1;
    }

    /**
    * <p>Parses the inputstream and creates the trie index with it.</p>
    * <p>This is overwritten by the child classes.
    * @param inputStream input stream containing the trie information
    * @exception IOException thrown when data reading fails.
    * @draft 2.1
    */
    protected void unserialize(InputStream inputStream) throws IOException
    {
        //indexLength is a multiple of 1024 >> INDEX_STAGE_2_SHIFT_
        m_index_              = new char[m_dataOffset_];
        DataInputStream input = new DataInputStream(inputStream);
        for (int i = 0; i < m_dataOffset_; i ++) {
             m_index_[i] = input.readChar();
        }
    }

    /**
    * Determines if this is a 32 bit trie
    * @return true if options specifies this is a 32 bit trie
    * @draft 2.1
    */
    protected final boolean isIntTrie()
    {
        return (m_options_ & HEADER_OPTIONS_DATA_IS_32_BIT_) != 0;
    }

    /**
    * Determines if this is a 16 bit trie
    * @return true if this is a 16 bit trie
    * @draft 2.1
    */
    protected final boolean isCharTrie()
    {
        return (m_options_ & HEADER_OPTIONS_DATA_IS_32_BIT_) == 0;
    }

    // private data members --------------------------------------------

    /**
    * Lead surrogate code points' index displacement in the index array.
    * 0x10000-0xd800=0x2800
    * 0x2800 >> INDEX_STAGE_1_SHIFT_
    */
    private static final int LEAD_INDEX_OFFSET_ =
                                             0x2800 >> 5;
    /**
    * Signature index
    */
    private static final int HEADER_SIGNATURE_INDEX_ = 0;
    /**
    * Options index
    */
    private static final int HEADER_OPTIONS_INDEX_ = 1 << 1;
    /**
    * Index length index
    */
    private static final int HEADER_INDEX_LENGTH_INDEX_ = 2 << 1;
    /**
    * Data length index
    */
    private static final int HEADER_DATA_LENGTH_INDEX_ = 3 << 1;
    /**
    * Size of header
    */
    private static final int HEADER_LENGTH_ = 4 << 1;
    /**
    * Latin 1 option mask
    */
    private static final int HEADER_OPTIONS_LATIN1_IS_LINEAR_MASK_ = 0x200;
    /**
    * Constant number to authenticate the byte block
    */
    private static final int HEADER_SIGNATURE_ = 0x54726965;
    /**
    * Header option formatting
    */
    private static final int HEADER_OPTIONS_SHIFT_MASK_ = 0xF;
    private static final int HEADER_OPTIONS_INDEX_SHIFT_ = 4;
    private static final int HEADER_OPTIONS_DATA_IS_32_BIT_ = 0x100;
    
    /**
    * Flag indicator for Latin quick access data block
    */
    private boolean m_isLatin1Linear_;
    
    /**
    * <p>Trie options field.</p>
    * <p>options bit field:<br>
    * 9  1 = Latin-1 data is stored linearly at data + DATA_BLOCK_LENGTH<br>
    * 8  0 = 16-bit data, 1=32-bit data<br>
    * 7..4  INDEX_STAGE_1_SHIFT   // 0..INDEX_STAGE_2_SHIFT<br>
    * 3..0  INDEX_STAGE_2_SHIFT   // 1..9<br>
    */
    private int m_options_;
  
    // private methods ---------------------------------------------------
    
    /**
    * Authenticates raw data header.
    * Checking the header information, signature and options.
    * @param rawdata array of char data to be checked
    * @return true if the header is authenticated valid
    * @draft 2.1
    */
    private final boolean checkHeader(int signature)
    {
        // check the signature
        // Trie in big-endian US-ASCII (0x54726965).
        // Magic number to authenticate the data.
        if (signature != HEADER_SIGNATURE_) {
            return false;
        }

        if ((m_options_ & HEADER_OPTIONS_SHIFT_MASK_) != 
                                                    INDEX_STAGE_1_SHIFT_ ||
            ((m_options_ >> HEADER_OPTIONS_INDEX_SHIFT_) &
                                                HEADER_OPTIONS_SHIFT_MASK_)
                                                 != INDEX_STAGE_2_SHIFT_) {
            return false;
        }
        return true;
    }
}