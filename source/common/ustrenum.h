/*
**********************************************************************
* Copyright (c) 2002-2008, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: November 11 2002
* Since: ICU 2.4
**********************************************************************
*/
#ifndef _USTRENUM_H_
#define _USTRENUM_H_

#include "unicode/uenum.h"
#include "unicode/strenum.h"

/**
 * Given a StringEnumeration, wrap it in a UEnumeration.  The
 * StringEnumeration is adopted; after this call, the caller must not
 * delete it (regardless of error status).
 */
U_CAPI UEnumeration* U_EXPORT2
uenum_openStringEnumeration(U_NAMESPACE_QUALIFIER StringEnumeration* adopted, UErrorCode* ec);

/**
 * Given an array of const char* strings (invariant chars only),
 * return a UEnumeration.  Must have strings[i] != 0 for i in
 * 0..count-1.
 */
U_CAPI UEnumeration* U_EXPORT2
uenum_openCharStringsEnumeration(const char* const* strings, int32_t count,
                                 UErrorCode* ec);

//----------------------------------------------------------------------
U_NAMESPACE_BEGIN

/**
 * A wrapper to make a UEnumeration into a StringEnumeration.  The
 * wrapper adopts the UEnumeration is wraps.
 */
class U_COMMON_API UStringEnumeration : public StringEnumeration {

public:
    /**
     * Constructor.  This constructor adopts its UEnumeration
     * argument.
     * @param uenum a UEnumeration object.  This object takes
     * ownership of 'uenum' and will close it in its destructor.  The
     * caller must not call uenum_close on 'uenum' after calling this
     * constructor.
     */
    UStringEnumeration(UEnumeration* uenum);

    /**
     * Destructor.  This closes the UEnumeration passed in to the
     * constructor.
     */
    virtual ~UStringEnumeration();

    /**
     * Return the number of elements that the iterator traverses.
     * @param status the error code.
     * @return number of elements in the iterator.
     */
    virtual int32_t count(UErrorCode& status) const;

    /**
     * Returns the next element a UnicodeString*.  If there are no
     * more elements, returns NULL.
     * @param status the error code.
     * @return a pointer to the string, or NULL.
     */
    virtual const UnicodeString* snext(UErrorCode& status);

    /**
     * Resets the iterator.
     * @param status the error code.
     */
    virtual void reset(UErrorCode& status);

    /**
     * ICU4C "poor man's RTTI", returns a UClassID for the actual ICU class.
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU4C "poor man's RTTI", returns a UClassID for this ICU class.
     */
    static UClassID U_EXPORT2 getStaticClassID();

private:
    UEnumeration *uenum; // owned
};

U_NAMESPACE_END

#endif

