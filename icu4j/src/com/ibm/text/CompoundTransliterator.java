/*
 *******************************************************************************
 * Copyright (C) 1996-2000, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/text/Attic/CompoundTransliterator.java,v $ 
 * $Date: 2001/11/29 16:11:46 $ 
 * $Revision: 1.23 $
 *
 *****************************************************************************************
 */
package com.ibm.text;
import com.ibm.util.Utility;
import java.util.Enumeration;
import java.util.Vector;

/**
 * A transliterator that is composed of two or more other
 * transliterator objects linked together.  For example, if one
 * transliterator transliterates from script A to script B, and
 * another transliterates from script B to script C, the two may be
 * combined to form a new transliterator from A to C.
 *
 * <p>Composed transliterators may not behave as expected.  For
 * example, inverses may not combine to form the identity
 * transliterator.  See the class documentation for {@link
 * Transliterator} for details.
 *
 * <p>Copyright &copy; IBM Corporation 1999.  All rights reserved.
 *
 * @author Alan Liu
 * @version $RCSfile: CompoundTransliterator.java,v $ $Revision: 1.23 $ $Date: 2001/11/29 16:11:46 $
 */
public class CompoundTransliterator extends Transliterator {

    private Transliterator[] trans;

    /**
     * For compound RBTs (those with an ::id block before and/or after
     * the main rule block) we record the index of the RBT here.
     * Otherwise, this should have a value of -1.  We need this
     * information to implement toRules().
     */
    private int compoundRBTIndex;    

    private static final String COPYRIGHT =
        "\u00A9 IBM Corporation 1999-2001. All rights reserved.";

    /**
     * Constructs a new compound transliterator given an array of
     * transliterators.  The array of transliterators may be of any
     * length, including zero or one, however, useful compound
     * transliterators have at least two components.
     * @param transliterators array of <code>Transliterator</code>
     * objects
     * @param filter the filter.  Any character for which
     * <tt>filter.contains()</tt> returns <tt>false</tt> will not be
     * altered by this transliterator.  If <tt>filter</tt> is
     * <tt>null</tt> then no filtering is applied.
     */
    public CompoundTransliterator(Transliterator[] transliterators,
                                  UnicodeFilter filter) {
        super(joinIDs(transliterators), filter);
        trans = new Transliterator[transliterators.length];
        System.arraycopy(transliterators, 0, trans, 0, trans.length);
        computeMaximumContextLength();
    }

    /**
     * Constructs a new compound transliterator given an array of
     * transliterators.  The array of transliterators may be of any
     * length, including zero or one, however, useful compound
     * transliterators have at least two components.
     * @param transliterators array of <code>Transliterator</code>
     * objects
     */
    public CompoundTransliterator(Transliterator[] transliterators) {
        this(transliterators, null);
    }
    
    /**
     * Constructs a new compound transliterator.
     * @param ID compound ID
     * @param direction either Transliterator.FORWARD or Transliterator.REVERSE
     * @param filter a global filter for this compound transliterator
     * or null
     */
    public CompoundTransliterator(String ID, int direction,
                                  UnicodeFilter filter) {
        super(ID, filter);
        init(ID, direction, -1, null, true);
    }
    
    /**
     * Constructs a new compound transliterator with no filter.
     * @param ID compound ID
     * @param direction either Transliterator.FORWARD or Transliterator.REVERSE
     */
    public CompoundTransliterator(String ID, int direction) {
        this(ID, direction, null);
    }
    
    /**
     * Constructs a new forward compound transliterator with no filter.
     * @param ID compound ID
     */
    public CompoundTransliterator(String ID) {
        this(ID, FORWARD, null);
    }

    /**
     * Package private constructor for compound RBTs.  Construct a
     * compound transliterator using the given idBlock, with the
     * splitTrans inserted at the idSplitPoint.
     */
    CompoundTransliterator(String ID,
                           String idBlock,
                           int idSplitPoint,
                           Transliterator splitTrans) {
        super(ID, null);
        init(idBlock, FORWARD, idSplitPoint, splitTrans, false);
    }
    
    /**
     * Package private constructor for Transliterator from a vector of
     * transliterators.  The vector order is FORWARD, so if dir is
     * REVERSE then the vector order will be reversed.  The caller is
     * responsible for fixing up the ID.
     */
    CompoundTransliterator(int dir,
                           Vector list) {
        super("", null);
        trans = null;
        compoundRBTIndex = -1;
        init(list, dir, false);
        // assume caller will fixup ID
    }

    /**
     * Finish constructing a transliterator: only to be called by
     * constructors.  Before calling init(), set trans and filter to NULL.
     * @param id the id containing ';'-separated entries
     * @param direction either FORWARD or REVERSE
     * @param idSplitPoint the index into id at which the
     * splitTrans should be inserted, if there is one, or
     * -1 if there is none.
     * @param splitTrans a transliterator to be inserted
     * before the entry at offset idSplitPoint in the id string.  May be
     * NULL to insert no entry.
     * @param fixReverseID if TRUE, then reconstruct the ID of reverse
     * entries by calling getID() of component entries.  Some constructors
     * do not require this because they apply a facade ID anyway.
     */
    private void init(String id,
                      int direction,
                      int idSplitPoint,
                      Transliterator splitTrans,
                      boolean fixReverseID) {
        // assert(trans == 0);

        Vector list = new Vector();
        int[] splitTransIndex = new int[1];
        UnicodeSet[] compoundFilter = new UnicodeSet[1];
        StringBuffer regenID = new StringBuffer();
        Transliterator.parseCompoundID(id, regenID, direction,
                                       idSplitPoint, splitTrans,
                                       list, splitTransIndex, compoundFilter);
        compoundRBTIndex = splitTransIndex[0];

        init(list, direction, fixReverseID);

        if (compoundFilter[0] != null) {
            setFilter(compoundFilter[0]);
        }
    }

    /**
     * Finish constructing a transliterator: only to be called by
     * constructors.  Before calling init(), set trans and filter to NULL.
     * @param list a vector of transliterator objects to be adopted.  It
     * should NOT be empty.  The list should be in declared order.  That
     * is, it should be in the FORWARD order; if direction is REVERSE then
     * the list order will be reversed.
     * @param direction either FORWARD or REVERSE
     * @param fixReverseID if TRUE, then reconstruct the ID of reverse
     * entries by calling getID() of component entries.  Some constructors
     * do not require this because they apply a facade ID anyway.
     */
    private void init(Vector list,
                      int direction,
                      boolean fixReverseID) {
        // assert(trans == 0);

        // Allocate array
        int count = list.size();
        trans = new Transliterator[count];

        // Move the transliterators from the vector into an array.
        // Reverse the order if necessary.
        int i;
        for (i=0; i<count; ++i) {
            int j = (direction == FORWARD) ? i : count - 1 - i;
            trans[i] = (Transliterator) list.elementAt(j);
        }

        // Fix compoundRBTIndex for REVERSE transliterators
        if (compoundRBTIndex >= 0 && direction == REVERSE) {
            compoundRBTIndex = count - 1 - compoundRBTIndex;
        }

        // If the direction is UTRANS_REVERSE then we may need to fix the
        // ID.
        if (direction == REVERSE && fixReverseID) {
            StringBuffer newID = new StringBuffer();
            for (i=0; i<count; ++i) {
                if (i > 0) {
                    newID.append(ID_DELIM);
                }
                newID.append(trans[i].getID());
            }
            setID(newID.toString());
        }

        computeMaximumContextLength();
    }

    /**
     * Return the IDs of the given list of transliterators, concatenated
     * with ';' delimiting them.  Equivalent to the perlish expression
     * join(';', map($_.getID(), transliterators).
     */
    private static String joinIDs(Transliterator[] transliterators) {
        StringBuffer id = new StringBuffer();
        for (int i=0; i<transliterators.length; ++i) {
            if (i > 0) {
                id.append(';');
            }
            id.append(transliterators[i].getID());
        }
        return id.toString();
    }

    /**
     * Returns the number of transliterators in this chain.
     * @return number of transliterators in this chain.
     */
    public int getCount() {
        return trans.length;
    }

    /**
     * Returns the transliterator at the given index in this chain.
     * @param index index into chain, from 0 to <code>getCount() - 1</code>
     * @return transliterator at the given index
     */
    public Transliterator getTransliterator(int index) {
        return trans[index];
    }

    /**
     * Append c to buf, unless buf is empty or buf already ends in c.
     */
    private static void _smartAppend(StringBuffer buf, char c) {
        if (buf.length() != 0 &&
            buf.charAt(buf.length() - 1) != c) {
            buf.append(c);
        }
    }

    /**
     * Override Transliterator:
     * Create a rule string that can be passed to createFromRules()
     * to recreate this transliterator.
     * @param escapeUnprintable if TRUE then convert unprintable
     * character to their hex escape representations, \\uxxxx or
     * \\Uxxxxxxxx.  Unprintable characters are those other than
     * U+000A, U+0020..U+007E.
     * @return the rule string
     * @deprecated To be removed after 2002-sep-30.
     */
    public String toRules(boolean escapeUnprintable) {
        // We do NOT call toRules() on our component transliterators, in
        // general.  If we have several rule-based transliterators, this
        // yields a concatenation of the rules -- not what we want.  We do
        // handle compound RBT transliterators specially -- those for which
        // compoundRBTIndex >= 0.  For the transliterator at compoundRBTIndex,
        // we do call toRules() recursively.
        StringBuffer rulesSource = new StringBuffer();
        if (compoundRBTIndex >= 0 && getFilter() != null) {
            // If we are a compound RBT and if we have a global
            // filter, then emit it at the top.
            rulesSource.append("::").append(getFilter().toPattern(escapeUnprintable)).append(ID_DELIM);
        }
        for (int i=0; i<trans.length; ++i) {
            String rule;
            if (i == compoundRBTIndex) {
                rule = trans[i].toRules(escapeUnprintable);
            } else {
                rule = trans[i].baseToRules(escapeUnprintable);
            }
            _smartAppend(rulesSource, '\n');
            rulesSource.append(rule);
            _smartAppend(rulesSource, ID_DELIM);
        }
        return rulesSource.toString();
    }

    /**
     * Implements {@link Transliterator#handleTransliterate}.
     */
    protected void handleTransliterate(Replaceable text,
                                       Position index, boolean incremental) {
        /* Call each transliterator with the same start value and
         * initial cursor index, but with the limit index as modified
         * by preceding transliterators.  The cursor index must be
         * reset for each transliterator to give each a chance to
         * transliterate the text.  The initial cursor index is known
         * to still point to the same place after each transliterator
         * is called because each transliterator will not change the
         * text between start and the initial value of cursor.
         *
         * IMPORTANT: After the first transliterator, each subsequent
         * transliterator only gets to transliterate text committed by
         * preceding transliterators; that is, the cursor (output
         * value) of transliterator i becomes the limit (input value)
         * of transliterator i+1.  Finally, the overall limit is fixed
         * up before we return.
         *
         * Assumptions we make here:
         * (1) contextStart <= start <= limit <= contextLimit <= text.length()
         * (2) start <= start' <= limit'  ;cursor doesn't move back
         * (3) start <= limit'            ;text before cursor unchanged
         * - start' is the value of start after calling handleKT
         * - limit' is the value of limit after calling handleKT
         */

        /**
         * Example: 3 transliterators.  This example illustrates the
         * mechanics we need to implement.  C, S, and L are the contextStart,
         * start, and limit.  gl is the globalLimit.  contextLimit is
         * equal to limit throughout.
         *
         * 1. h-u, changes hex to Unicode
         *
         *    4  7  a  d  0      4  7  a
         *    abc/u0061/u    =>  abca/u    
         *    C  S       L       C   S L   gl=f->a
         *
         * 2. upup, changes "x" to "XX"
         *
         *    4  7  a       4  7  a
         *    abca/u    =>  abcAA/u    
         *    C  SL         C    S   
         *                       L    gl=a->b
         * 3. u-h, changes Unicode to hex
         *
         *    4  7  a        4  7  a  d  0  3
         *    abcAA/u    =>  abc/u0041/u0041/u    
         *    C  S L         C              S
         *                                  L   gl=b->15
         * 4. return
         *
         *    4  7  a  d  0  3
         *    abc/u0041/u0041/u    
         *    C S L
         */

        if (trans.length < 1) {
            index.start = index.limit;
            return; // Short circuit for empty compound transliterators
        }

        // compoundLimit is the limit value for the entire compound
        // operation.  We overwrite index.limit with the previous
        // index.start.  After each transliteration, we update
        // compoundLimit for insertions or deletions that have happened.
        int compoundLimit = index.limit;

        // compoundStart is the start for the entire compound
        // operation.
        int compoundStart = index.start;

        int delta = 0; // delta in length

        StringBuffer log = null;
        if (DEBUG) {
            log = new StringBuffer("CompoundTransliterator{" + getID() +
                                   (incremental ? "}i: IN=" : "}: IN="));
            Utility.formatInput(log, text, index);
            System.out.println(Utility.escape(log.toString()));
        }

        // Give each transliterator a crack at the run of characters.
        // See comments at the top of the method for more detail.
        for (int i=0; i<trans.length; ++i) {
            index.start = compoundStart; // Reset start
            int limit = index.limit;

            if (index.start == index.limit) {
                // Short circuit for empty range
                if (DEBUG) {
                    System.out.println("CompoundTransliterator[" + i +
                                       ".." + (trans.length-1) +
                                       (incremental ? "]i: " : "]: ") +
                                       Utility.formatInput(text, index) +
                                       " (NOTHING TO DO)");
                }
                break;
            }

            if (DEBUG) {
                log.setLength(0);
                log.append("CompoundTransliterator[" + i + "=" +
                           trans[i].getID() +
                           (incremental ? "]i: " : "]: "));
                Utility.formatInput(log, text, index);
            }

            trans[i].filteredTransliterate(text, index, incremental);

            // In a properly written transliterator, start == limit after
            // handleTransliterate() returns when incremental is false.
            // Catch cases where the subclass doesn't do this, and throw
            // an exception.  (Just pinning start to limit is a bad idea,
            // because what's probably happening is that the subclass
            // isn't transliterating all the way to the end, and it should
            // in non-incremental mode.)
            if (!incremental && index.start != index.limit) {
                System.out.println("ERROR: Incomplete non-incremental transliteration in " + trans[i].getID());
                index.start = index.limit;
            }

            if (DEBUG) {
                log.append(" => ");
                Utility.formatInput(log, text, index);
                System.out.println(Utility.escape(log.toString()));
            }

            // Cumulative delta for insertions/deletions
            delta += index.limit - limit;

            if (incremental) {
                // In the incremental case, only allow subsequent
                // transliterators to modify what has already been
                // completely processed by prior transliterators.  In the
                // non-incrmental case, allow each transliterator to
                // process the entire text.
                index.limit = index.start;
            }
        }

        compoundLimit += delta;

        // Start is good where it is -- where the last transliterator left
        // it.  Limit needs to be put back where it was, modulo
        // adjustments for deletions/insertions.
        index.limit = compoundLimit;

        if (DEBUG) {
            log.setLength(0);
            log.append("CompoundTransliterator{" + getID() +
                       (incremental ? "}i: OUT=" : "}: OUT="));
            Utility.formatInput(log, text, index);
            System.out.println(Utility.escape(log.toString()));
        }
    }

    /**
     * Compute and set the length of the longest context required by this transliterator.
     * This is <em>preceding</em> context.
     */
    private void computeMaximumContextLength() {
        int max = 0;
        for (int i=0; i<trans.length; ++i) {
            int len = trans[i].getMaximumContextLength();
            if (len > max) {
                max = len;
            }
        }
        setMaximumContextLength(max);
    }
}
