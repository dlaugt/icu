/*
*******************************************************************************
*   Copyright (C) 2000-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*
*   file name:  uversion.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   Created by: Vladimir Weinstein
*
*  Gets included by utypes.h and Windows .rc files
*/

/**
 * \file
 * \brief C API: API for accessing ICU version numbers. 
 */
/*===========================================================================*/
/* Main ICU version information                                              */
/*===========================================================================*/

#ifndef UVERSION_H
#define UVERSION_H

/**
 * IMPORTANT: When updating version, the following things need to be done:
 * source/common/unicode/uversion.h - this file: update major, minor,
 *        patchlevel, suffix, version, short version constants, namespace,
 *                                                             and copyright
 * source/common/common.vcproj - update 'Output file name' on the link tab so
 *                   that it contains the new major/minor combination
 * source/i18n/i18n.vcproj - same as for the common.vcproj
 * source/layout/layout.vcproj - same as for the common.vcproj
 * source/layoutex/layoutex.vcproj - same
 * source/stubdata/stubdata.vcproj - same as for the common.vcproj
 * source/io/io.vcproj - same as for the common.vcproj
 * source/data/makedata.mak - change U_ICUDATA_NAME so that it contains
 *                            the new major/minor combination
 * source/tools/genren/genren.pl - use this script according to the README
 *                    in that folder                                         
 */

#include "unicode/umachine.h"

/* Actual version info lives in uvernum.h */
#include "unicode/uvernum.h"

/** Maximum length of the copyright string.
 *  @stable ICU 2.4
 */
#define U_COPYRIGHT_STRING_LENGTH  128

/** An ICU version consists of up to 4 numbers from 0..255.
 *  @stable ICU 2.4
 */
#define U_MAX_VERSION_LENGTH 4

/** In a string, ICU version fields are delimited by dots.
 *  @stable ICU 2.4
 */
#define U_VERSION_DELIMITER '.'

/** The maximum length of an ICU version string.
 *  @stable ICU 2.4
 */
#define U_MAX_VERSION_STRING_LENGTH 20

/** The binary form of a version on ICU APIs is an array of 4 uint8_t.
 *  To compare two versions, use memcmp(v1,v2,sizeof(UVersionInfo)).
 *  @stable ICU 2.4
 */
typedef uint8_t UVersionInfo[U_MAX_VERSION_LENGTH];

/*===========================================================================*/
/* C++ namespace if supported. Versioned unless versioning is disabled.      */
/*===========================================================================*/

/**
 * \def U_NAMESPACE_BEGIN
 * This is used to begin a declaration of a public ICU C++ API.
 * If the compiler doesn't support namespaces, this does nothing.
 * @stable ICU 2.4
 */

/**
 * \def U_NAMESPACE_END
 * This is used to end a declaration of a public ICU C++ API
 * If the compiler doesn't support namespaces, this does nothing.
 * @stable ICU 2.4
 */

/**
 * \def U_NAMESPACE_USE
 * This is used to specify that the rest of the code uses the
 * public ICU C++ API namespace.
 * If the compiler doesn't support namespaces, this does nothing.
 * @stable ICU 2.4
 */

/**
 * \def U_NAMESPACE_QUALIFIER
 * This is used to qualify that a function or class is part of
 * the public ICU C++ API namespace.
 * If the compiler doesn't support namespaces, this does nothing.
 * @stable ICU 2.4
 */

/* Define namespace symbols if the compiler supports it. */
#if U_HAVE_NAMESPACE && defined(XP_CPLUSPLUS)
#   if U_DISABLE_RENAMING
#       define U_ICU_NAMESPACE icu
        namespace U_ICU_NAMESPACE { }
#   else
#       define U_ICU_NAMESPACE U_ICU_ENTRY_POINT_RENAME(icu)
        namespace U_ICU_NAMESPACE { }
        namespace icu = U_ICU_NAMESPACE;
#   endif

#   define U_NAMESPACE_BEGIN namespace U_ICU_NAMESPACE {
#   define U_NAMESPACE_END  }
#   define U_NAMESPACE_USE using namespace U_ICU_NAMESPACE;
#   define U_NAMESPACE_QUALIFIER U_ICU_NAMESPACE::

#   ifndef U_USING_ICU_NAMESPACE
#       define U_USING_ICU_NAMESPACE 1
#   endif
#   if U_USING_ICU_NAMESPACE
        U_NAMESPACE_USE
#   endif
#else
#   define U_NAMESPACE_BEGIN
#   define U_NAMESPACE_END
#   define U_NAMESPACE_USE
#   define U_NAMESPACE_QUALIFIER
#endif

/*===========================================================================*/
/* General version helper functions. Definitions in putil.c                  */
/*===========================================================================*/

/**
 * Parse a string with dotted-decimal version information and
 * fill in a UVersionInfo structure with the result.
 * Definition of this function lives in putil.c
 *
 * @param versionArray The destination structure for the version information.
 * @param versionString A string with dotted-decimal version information,
 *                      with up to four non-negative number fields with
 *                      values of up to 255 each.
 * @stable ICU 2.4
 */
U_STABLE void U_EXPORT2
u_versionFromString(UVersionInfo versionArray, const char *versionString);

/**
 * Parse a Unicode string with dotted-decimal version information and
 * fill in a UVersionInfo structure with the result.
 * Definition of this function lives in putil.c
 *
 * @param versionArray The destination structure for the version information.
 * @param versionString A Unicode string with dotted-decimal version
 *                      information, with up to four non-negative number
 *                      fields with values of up to 255 each.
 * @draft ICU 4.2
 */
U_STABLE void U_EXPORT2
u_versionFromUString(UVersionInfo versionArray, const UChar *versionString);


/**
 * Write a string with dotted-decimal version information according
 * to the input UVersionInfo.
 * Definition of this function lives in putil.c
 *
 * @param versionArray The version information to be written as a string.
 * @param versionString A string buffer that will be filled in with
 *                      a string corresponding to the numeric version
 *                      information in versionArray.
 *                      The buffer size must be at least U_MAX_VERSION_STRING_LENGTH.
 * @stable ICU 2.4
 */
U_STABLE void U_EXPORT2
u_versionToString(UVersionInfo versionArray, char *versionString);

/**
 * Gets the ICU release version.  The version array stores the version information
 * for ICU.  For example, release "1.3.31.2" is then represented as 0x01031F02.
 * Definition of this function lives in putil.c
 *
 * @param versionArray the version # information, the result will be filled in
 * @stable ICU 2.0
 */
U_STABLE void U_EXPORT2
u_getVersion(UVersionInfo versionArray);
#endif
