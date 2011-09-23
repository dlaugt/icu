/*
*******************************************************************************
*   Copyright (C) 2000-2011, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*
*   file name:  uvernum.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   Created by: Vladimir Weinstein
*   Updated by: Steven R. Loomis
*
*  Gets included by uversion.h and other files.
*
* IMPORTANT: When updating version, the following things need to be done:
* source/common/unicode/uvernum.h - this file: update major, minor,
*        patchlevel, suffix, version, short version constants, namespace,
*                    renaming macro, and copyright
*
* The following files need to be updated as well, which can be done
*  by running the UNIX makefile target 'update-windows-makefiles' in icu/source.
*
*
* source/common/common.vcproj - update 'Output file name' on the link tab so
*                   that it contains the new major/minor combination
* source/i18n/i18n.vcproj - same as for the common.vcproj
* source/layout/layout.vcproj - same as for the common.vcproj
* source/layoutex/layoutex.vcproj - same
* source/stubdata/stubdata.vcproj - same as for the common.vcproj
* source/io/io.vcproj - same as for the common.vcproj
* source/data/makedata.mak - change U_ICUDATA_NAME so that it contains
*                            the new major/minor combination and the Unicode version.
*/

#ifndef UVERNUM_H
#define UVERNUM_H

/** The standard copyright notice that gets compiled into each library. 
 *  This value will change in the subsequent releases of ICU
 *  @stable ICU 2.4
 */
#define U_COPYRIGHT_STRING \
  " Copyright (C) 2011, International Business Machines Corporation and others. All Rights Reserved. "

/** The current ICU major version as an integer. 
 *  This value will change in the subsequent releases of ICU
 *  @stable ICU 2.4
 */
#define U_ICU_VERSION_MAJOR_NUM 4

/** The current ICU minor version as an integer. 
 *  This value will change in the subsequent releases of ICU
 *  @stable ICU 2.6
 */
#define U_ICU_VERSION_MINOR_NUM 8

/** The current ICU patchlevel version as an integer.  
 *  This value will change in the subsequent releases of ICU
 *  @stable ICU 2.4
 */
#define U_ICU_VERSION_PATCHLEVEL_NUM 1

/** The current ICU build level version as an integer.  
 *  This value is for use by ICU clients. It defaults to 0.
 *  @stable ICU 4.0
 */
#ifndef U_ICU_VERSION_BUILDLEVEL_NUM
#define U_ICU_VERSION_BUILDLEVEL_NUM 1
#endif

/** Glued version suffix for renamers 
 *  This value will change in the subsequent releases of ICU
 *  @stable ICU 2.6
 */
#define U_ICU_VERSION_SUFFIX _48

/** Glued version suffix function for renamers 
 *  This value will change in the subsequent releases of ICU.
 *  If a custom suffix (such as matching library suffixes) is desired, this can be modified.
 *  Note that if present, platform.h may contain an earlier definition of this macro.
 *  @stable ICU 4.2
 */
#ifndef U_ICU_ENTRY_POINT_RENAME
#define U_ICU_ENTRY_POINT_RENAME(x)    x ## _48
#endif

/** The current ICU library version as a dotted-decimal string. The patchlevel
 *  only appears in this string if it non-zero. 
 *  This value will change in the subsequent releases of ICU
 *  @stable ICU 2.4
 */
#define U_ICU_VERSION "4.8.1.1"

/** The current ICU library major/minor version as a string without dots, for library name suffixes. 
 *  This value will change in the subsequent releases of ICU
 *  @stable ICU 2.6
 */
#define U_ICU_VERSION_SHORT "48"

/** Data version in ICU4C.
 * @internal ICU 4.4 Internal Use Only
 **/
#define U_ICU_DATA_VERSION "4.8.1"

/*===========================================================================
 * ICU collation framework version information
 * Version info that can be obtained from a collator is affected by these
 * numbers in a secret and magic way. Please use collator version as whole
 *===========================================================================
 */

/**
 * Collation runtime version (sort key generator, strcoll).
 * If the version is different, sort keys for the same string could be different.
 * This value may change in subsequent releases of ICU.
 * @stable ICU 2.4
 */
#define UCOL_RUNTIME_VERSION 7

/**
 * Collation builder code version.
 * When this is different, the same tailoring might result
 * in assigning different collation elements to code points.
 * This value may change in subsequent releases of ICU.
 * @stable ICU 2.4
 */
#define UCOL_BUILDER_VERSION 8

/**
 * This is the version of collation tailorings.
 * This value may change in subsequent releases of ICU.
 * @stable ICU 2.4
 */
#define UCOL_TAILORINGS_VERSION 1

#endif
