/*
 **********************************************************************
 *   Copyright (C) 1997-2002, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
*/

#include "mutex.h"
#include "ucln_cmn.h"

/* Initialize the global mutex only when we can use it. */
#if (ICU_USE_THREADS == 1)

static int GlobalMutexInitialize()
{
    UErrorCode status = U_ZERO_ERROR;

    umtx_init(NULL);
    ucnv_init(&status);
    ures_init(&status);
    return 0;
}

static int initializesGlobalMutex = GlobalMutexInitialize();

#endif /* ICU_USE_THREADS==1 */

