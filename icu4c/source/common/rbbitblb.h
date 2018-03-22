// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
//
//  rbbitblb.h
//

/*
**********************************************************************
*   Copyright (c) 2002-2016, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*/

#ifndef RBBITBLB_H
#define RBBITBLB_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/rbbi.h"
#include "rbbinode.h"


U_NAMESPACE_BEGIN

class RBBIRuleScanner;
class RBBIRuleBuilder;
class UVector32;

//
//  class RBBITableBuilder is part of the RBBI rule compiler.
//                         It builds the state transition table used by the RBBI runtime
//                         from the expression syntax tree generated by the rule scanner.
//
//                         This class is part of the RBBI implementation only.
//                         There is no user-visible public API here.
//

class RBBITableBuilder : public UMemory {
public:
    RBBITableBuilder(RBBIRuleBuilder *rb, RBBINode **rootNode, UErrorCode &status);
    ~RBBITableBuilder();

    void     build();

    /** Return the runtime size in bytes of the built state table.  */
    int32_t  getTableSize() const;

    /** Fill in the runtime state table. Sufficient memory must exist at the specified location.
     */
    void     exportTable(void *where);

    /** Find duplicate (redundant) character classes, beginning after the specifed
     *  pair, within this state table. This is an iterator-like function, used to
     *  identify char classes (state table columns) that can be eliminated.
     */
    bool     findDuplCharClassFrom(int &baseClass, int &duplClass);

    /** Remove a column from the state table. Used when two character categories
     *  have been found equivalent, and merged together, to eliminate the uneeded table column.
     */
    void     removeColumn(int32_t column);

    /** Check for, and remove dupicate states (table rows). */
    void     removeDuplicateStates();

    void     buildSafe(UErrorCode &status);

    /** Return the runtime size in bytes of the built safe reverse state table. */
    int32_t  getSafeTableSize() const;

    /** Fill in the runtime safe state table. Sufficient memory must exist at the specified location.
     */
    void     exportSafeTable(void *where);


private:
    void     calcNullable(RBBINode *n);
    void     calcFirstPos(RBBINode *n);
    void     calcLastPos(RBBINode  *n);
    void     calcFollowPos(RBBINode *n);
    void     calcChainedFollowPos(RBBINode *n);
    void     bofFixup();
    void     buildStateTable();
    void     flagAcceptingStates();
    void     flagLookAheadStates();
    void     flagTaggedStates();
    void     mergeRuleStatusVals();

    /**
     * Merge redundant state table columns, eliminating character classes with identical behavior.
     * Done after the state tables are generated, just before converting to their run-time format.
     */
    int32_t  mergeColumns();

    void     addRuleRootNodes(UVector *dest, RBBINode *node);

    /** Find the next duplicate state. An iterator function.
     * @param firstState (in/out) begin looking at this state, return the first of the
     *                   pair of duplicates.
     * @param duplicateState returns the duplicate state of fistState
     * @return true if a duplicate pair of states was found.
     */
    bool findDuplicateState(int32_t &firstState, int32_t &duplicateState);

    /** Remove a duplicate state/
     * @param keepState First of the duplicate pair. Keep it.
     * @param duplState Duplicate state. Remove it. Redirect all references to the duplicate state
     *                  to refer to keepState instead.
     */
    void removeState(int32_t keepState, int32_t duplState);

    // Set functions for UVector.
    //   TODO:  make a USet subclass of UVector

    void     setAdd(UVector *dest, UVector *source);
    UBool    setEquals(UVector *a, UVector *b);

    void     sortedAdd(UVector **dest, int32_t val);

public:
#ifdef RBBI_DEBUG
    void     printSet(UVector *s);
    void     printPosSets(RBBINode *n /* = NULL*/);
    void     printStates();
    void     printRuleStatusTable();
    void     printSafeTable();
#else
    #define  printSet(s)
    #define  printPosSets(n)
    #define  printStates()
    #define  printRuleStatusTable()
    #define printSafeTable()
#endif

private:
    RBBIRuleBuilder  *fRB;
    RBBINode         *&fTree;              // The root node of the parse tree to build a
                                           //   table for.
    UErrorCode       *fStatus;

    /** State Descriptors, UVector<RBBIStateDescriptor> */
    UVector          *fDStates;            //  D states (Aho's terminology)
                                           //  Index is state number
                                           //  Contents are RBBIStateDescriptor pointers.

    /** Synthesized safe table, UVector of UnicodeString, one string per table row.   */
    UVector          *fSafeTable;


    RBBITableBuilder(const RBBITableBuilder &other); // forbid copying of this class
    RBBITableBuilder &operator=(const RBBITableBuilder &other); // forbid copying of this class
};

//
//  RBBIStateDescriptor - The DFA is constructed as a set of these descriptors,
//                        one for each state.
class RBBIStateDescriptor : public UMemory {
public:
    UBool            fMarked;
    int32_t          fAccepting;
    int32_t          fLookAhead;
    UVector          *fTagVals;
    int32_t          fTagsIdx;
    UVector          *fPositions;          // Set of parse tree positions associated
                                           //   with this state.  Unordered (it's a set).
                                           //   UVector contents are RBBINode *

    UVector32        *fDtran;              // Transitions out of this state.
                                           //   indexed by input character
                                           //   contents is int index of dest state
                                           //   in RBBITableBuilder.fDStates

    RBBIStateDescriptor(int maxInputSymbol,  UErrorCode *fStatus);
    ~RBBIStateDescriptor();

private:
    RBBIStateDescriptor(const RBBIStateDescriptor &other); // forbid copying of this class
    RBBIStateDescriptor &operator=(const RBBIStateDescriptor &other); // forbid copying of this class
};



U_NAMESPACE_END
#endif
