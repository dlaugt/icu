/*
******************************************************************************
*
*   Copyright (C) 1997-2012, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
*  FILE NAME : platform.h
*
*   Date        Name        Description
*   05/13/98    nos         Creation (content moved here from ptypes.h).
*   03/02/99    stephen     Added AS400 support.
*   03/30/99    stephen     Added Linux support.
*   04/13/99    stephen     Reworked for autoconf.
******************************************************************************
*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "unicode/uconfig.h"
#include "unicode/uvernum.h"

/**
 * \file
 * \brief Basic types for the platform.
 *
 * This file used to be generated by autoconf/configure.
 * Starting with ICU 49, platform.h is a normal source file,
 * to simplify cross-compiling and working with non-autoconf/make build systems.
 *
 * When a value in this file does not work on a platform, then please
 * try to derive it from the U_PLATFORM value
 * (for which we might need a new value constant in rare cases)
 * and/or from other macros that are predefined by the compiler
 * or defined in standard (POSIX or platform or compiler) headers.
 *
 * As a temporary workaround, you can add an explicit #define for some macros
 * before it is first tested, or add an equivalent -D macro definition
 * to the compiler's command line.
 *
 * Note: Some compilers provide ways to show the predefined macros.
 * For example, with gcc you can compile an empty .c file and have the compiler
 * print the predefined macros with
 * \code
 * gcc -E -dM -x c /dev/null | sort
 * \endcode
 * (You can provide an actual empty .c file rather than /dev/null.
 * <code>-x c++</code> is for C++.)
 */

/**
 * \def U_PLATFORM
 * The U_PLATFORM macro defines the platform we're on.
 *
 * We used to define one different, value-less macro per platform.
 * That made it hard to know the set of relevant platforms and macros,
 * and hard to deal with variants of platforms.
 *
 * Starting with ICU 49, we define platforms as numeric macros,
 * with ranges of values for related platforms and their variants.
 * The U_PLATFORM macro is set to one of these values.
 *
 * Historical note from the Solaris Wikipedia article:
 * AT&T and Sun collaborated on a project to merge the most popular Unix variants
 * on the market at that time: BSD, System V, and Xenix.
 * This became Unix System V Release 4 (SVR4).
 *
 * @internal
 */

/** Unknown platform. @internal */
#define U_PF_UNKNOWN 0
/** Windows @internal */
#define U_PF_WINDOWS 1000
/** MinGW. Windows, calls to Win32 API, but using GNU gcc and binutils. @internal */
#define U_PF_MINGW 1800
/**
 * Cygwin. Windows, calls to cygwin1.dll for Posix functions,
 * using MSVC or GNU gcc and binutils.
 * @internal
 */
#define U_PF_CYGWIN 1900
/* Reserve 2000 for U_PF_UNIX? */
/** HP-UX is based on UNIX System V. @internal */
#define U_PF_HPUX 2100
/** Solaris is a Unix operating system based on SVR4. @internal */
#define U_PF_SOLARIS 2600
/** BSD is a UNIX operating system derivative. @internal */
#define U_PF_BSD 3000
/** AIX is based on UNIX System V Releases and 4.3 BSD. @internal */
#define U_PF_AIX 3100
/** IRIX is based on UNIX System V with BSD extensions. @internal */
#define U_PF_IRIX 3200
/**
 * Darwin is a POSIX-compliant operating system, composed of code developed by Apple,
 * as well as code derived from NeXTSTEP, BSD, and other projects,
 * built around the Mach kernel.
 * Darwin forms the core set of components upon which Mac OS X, Apple TV, and iOS are based.
 * (Original description modified from WikiPedia.)
 * @internal
 */
#define U_PF_DARWIN 3500
/** iPhone OS (iOS) is a derivative of Mac OS X. @internal */
#define U_PF_IPHONE 3550
/** QNX is a commercial Unix-like real-time operating system related to BSD. @internal */
#define U_PF_QNX 3700
/** Linux is a Unix-like operating system. @internal */
#define U_PF_LINUX 4000
/** Android is based on Linux. @internal */
#define U_PF_ANDROID 4050
/** "Classic" Mac OS (1984-2001) @internal */
#define U_PF_CLASSIC_MACOS 8000
/** z/OS is the successor to OS/390 which was the successor to MVS. @internal */
#define U_PF_OS390 9000
/** "IBM i" is the current name of what used to be i5/OS and earlier OS/400. @internal */
#define U_PF_OS400 9400

#ifdef U_PLATFORM
    /* Use the predefined value. */
#elif defined(__MINGW32__)
#   define U_PLATFORM U_PF_MINGW
#elif defined(__CYGWIN__)
#   define U_PLATFORM U_PF_CYGWIN
#elif defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#   define U_PLATFORM U_PF_WINDOWS
#elif defined(__ANDROID__)
#   define U_PLATFORM U_PF_ANDROID
    /* Android wchar_t support depends on the API level. */
#   include <android/api-level.h>
#elif defined(linux) || defined(__linux__) || defined(__linux)
#   define U_PLATFORM U_PF_LINUX
#elif defined(BSD) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
#   define U_PLATFORM U_PF_BSD
#elif defined(sun) || defined(__sun)
    /* Check defined(__SVR4) || defined(__svr4__) to distinguish Solaris from SunOS? */
#   define U_PLATFORM U_PF_SOLARIS
#   if defined(__GNUC__)
        /* Solaris/GCC needs this header file to get the proper endianness. Normally, this
         * header file is included with stddef.h but on Solairs/GCC, the GCC version of stddef.h
         *  is included which does not include this header file.
         */
#       include <sys/isa_defs.h>
#   endif
#elif defined(_AIX) || defined(__TOS_AIX__)
#   define U_PLATFORM U_PF_AIX
#elif defined(_hpux) || defined(hpux) || defined(__hpux)
#   define U_PLATFORM U_PF_HPUX
#elif defined(sgi) || defined(__sgi)
#   define U_PLATFORM U_PF_IRIX
#elif defined(__APPLE__) && defined(__MACH__)
#   include <TargetConditionals.h>
#   if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE  /* variant of TARGET_OS_MAC */
#       define U_PLATFORM U_PF_IPHONE
#   else
#       define U_PLATFORM U_PF_DARWIN
#   endif
#elif defined(macintosh)
#   define U_PLATFORM U_PF_CLASSIC_MACOS
#elif defined(__QNX__) || defined(__QNXNTO__)
#   define U_PLATFORM U_PF_QNX
#elif defined(__TOS_MVS__)
#   define U_PLATFORM U_PF_OS390
#elif defined(__OS400__) || defined(__TOS_OS400__)
#   define U_PLATFORM U_PF_OS400
#else
#   define U_PLATFORM U_PF_UNKNOWN
#endif

/**
 * \def CYGWINMSVC
 * Defined if this is Windows with Cygwin, but using MSVC rather than gcc.
 * Otherwise undefined.
 * @internal
 */
/* Commented out because this is already set in mh-cygwin-msvc
#if U_PLATFORM == U_PF_CYGWIN && defined(_MSC_VER)
#   define CYGWINMSVC
#endif
*/

/**
 * \def U_PLATFORM_USES_ONLY_WIN32_API
 * Defines whether the platform uses only the Win32 API.
 * Set to 1 for Windows/MSVC and MinGW but not Cygwin.
 * @internal
 */
#ifdef U_PLATFORM_USES_ONLY_WIN32_API
    /* Use the predefined value. */
#elif (U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_MINGW) || defined(CYGWINMSVC)
#   define U_PLATFORM_USES_ONLY_WIN32_API 1
#else
    /* Cygwin implements POSIX. */
#   define U_PLATFORM_USES_ONLY_WIN32_API 0
#endif

/**
 * \def U_PLATFORM_HAS_WIN32_API
 * Defines whether the Win32 API is available on the platform.
 * Set to 1 for Windows/MSVC, MinGW and Cygwin.
 * @internal
 */
#ifdef U_PLATFORM_HAS_WIN32_API
    /* Use the predefined value. */
#elif U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#   define U_PLATFORM_HAS_WIN32_API 1
#else
#   define U_PLATFORM_HAS_WIN32_API 0
#endif

/**
 * \def U_PLATFORM_IMPLEMENTS_POSIX
 * Defines whether the platform implements (most of) the POSIX API.
 * Set to 1 for Cygwin and most other platforms.
 * @internal
 */
#ifdef U_PLATFORM_IMPLEMENTS_POSIX
    /* Use the predefined value. */
#elif U_PLATFORM_USES_ONLY_WIN32_API || U_PLATFORM == U_PF_CLASSIC_MACOS
#   define U_PLATFORM_IMPLEMENTS_POSIX 0
#else
#   define U_PLATFORM_IMPLEMENTS_POSIX 1
#endif

/**
 * \def U_PLATFORM_IS_LINUX_BASED
 * Defines whether the platform is Linux or one of its derivatives.
 * @internal
 */
#ifdef U_PLATFORM_IS_LINUX_BASED
    /* Use the predefined value. */
#elif U_PF_LINUX <= U_PLATFORM && U_PLATFORM <= U_PF_ANDROID
#   define U_PLATFORM_IS_LINUX_BASED 1
#else
#   define U_PLATFORM_IS_LINUX_BASED 0
#endif

/**
 * \def U_PLATFORM_IS_DARWIN_BASED
 * Defines whether the platform is Darwin or one of its derivatives.
 * @internal
 */
#ifdef U_PLATFORM_IS_DARWIN_BASED
    /* Use the predefined value. */
#elif U_PF_DARWIN <= U_PLATFORM && U_PLATFORM <= U_PF_IPHONE
#   define U_PLATFORM_IS_DARWIN_BASED 1
#else
#   define U_PLATFORM_IS_DARWIN_BASED 0
#endif

/**
 * \def U_HAVE_STDINT_H
 * Defines whether stdint.h is available. It is a C99 standard header.
 * We used to include inttypes.h which includes stdint.h but we usually do not need
 * the additional definitions from inttypes.h.
 * @internal
 */
#ifdef U_HAVE_STDINT_H
    /* Use the predefined value. */
#elif U_PLATFORM_USES_ONLY_WIN32_API
#   if defined(__BORLANDC__) || (defined(_MSC_VER) && _MSC_VER>=1600)
        /* Windows Visual Studio 9 and below do not have stdint.h & inttypes.h, but VS 2010 adds them. */
#       define U_HAVE_STDINT_H 1
#   else
#       define U_HAVE_STDINT_H 0
#   endif
#elif U_PLATFORM == U_PF_SOLARIS
    /* Solaris has inttypes.h but not stdint.h. */
#   define U_HAVE_STDINT_H 0
#else
#   define U_HAVE_STDINT_H 1
#endif

/**
 * \def U_HAVE_INTTYPES_H
 * Defines whether inttypes.h is available. It is a C99 standard header.
 * We include inttypes.h where it is available but stdint.h is not.
 * @internal
 */
#ifdef U_HAVE_INTTYPES_H
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_SOLARIS
    /* Solaris has inttypes.h but not stdint.h. */
#   define U_HAVE_INTTYPES_H 1
#else
    /* Most platforms have both inttypes.h and stdint.h, or neither. */
#   define U_HAVE_INTTYPES_H U_HAVE_STDINT_H
#endif

/**
 * \def U_IOSTREAM_SOURCE
 * Defines what support for C++ streams is available.
 *
 * If U_IOSTREAM_SOURCE is set to 199711, then &lt;iostream&gt; is available
 * (the ISO/IEC C++ FDIS was published in November 1997), and then
 * one should qualify streams using the std namespace in ICU header
 * files.
 * Starting with ICU 49, this is the only supported version.
 *
 * If U_IOSTREAM_SOURCE is set to 198506, then &lt;iostream.h&gt; is
 * available instead (in June 1985 Stroustrup published
 * "An Extensible I/O Facility for C++" at the summer USENIX conference).
 * Starting with ICU 49, this version is not supported any more.
 *
 * If U_IOSTREAM_SOURCE is 0 (or any value less than 199711),
 * then C++ streams are not available and
 * support for them will be silently suppressed in ICU.
 *
 * @internal
 */
#ifndef U_IOSTREAM_SOURCE
#define U_IOSTREAM_SOURCE 199711
#endif

/**
 * \def U_HAVE_STD_STRING
 * Defines whether the standard C++ (STL) &lt;string&gt; header is available.
 * @internal
 */
#ifdef U_HAVE_STD_STRING
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_ANDROID
#   define U_HAVE_STD_STRING 0
#else
#   define U_HAVE_STD_STRING 1
#endif

/*===========================================================================*/
/** @{ Compiler and environment features                                     */
/*===========================================================================*/

/**
 * \def U_IS_BIG_ENDIAN
 * Determines the endianness of the platform.
 * @internal
 */
#ifdef U_IS_BIG_ENDIAN
    /* Use the predefined value. */
#elif defined(BYTE_ORDER) && defined(BIG_ENDIAN)
#   define U_IS_BIG_ENDIAN (BYTE_ORDER == BIG_ENDIAN)
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
    /* gcc */
#   define U_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#elif defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)
#   define U_IS_BIG_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__) || defined(_LITTLE_ENDIAN)
#   define U_IS_BIG_ENDIAN 0
#elif U_PLATFORM == U_PF_OS390 || U_PLATFORM == U_PF_OS400 || defined(__s390__) || defined(__s390x__)
    /* These platforms do not appear to predefine any endianness macros. */
#   define U_IS_BIG_ENDIAN 1
#elif defined(_PA_RISC1_0) || defined(_PA_RISC1_1) || defined(_PA_RISC2_0)
    /* HPPA do not appear to predefine any endianness macros. */
#   define U_IS_BIG_ENDIAN 1
#elif defined(sparc) || defined(__sparc) || defined(__sparc__)
    /* Some sparc based systems (e.g. Linux) do not predefine any endianness macros. */
#   define U_IS_BIG_ENDIAN 1
#else
#   define U_IS_BIG_ENDIAN 0
#endif

/**
 * \def U_HAVE_PLACEMENT_NEW
 * Determines whether to override placement new and delete for STL.
 * @stable ICU 2.6
 */
#ifdef U_HAVE_PLACEMENT_NEW
    /* Use the predefined value. */
#elif defined(__BORLANDC__)
#   define U_HAVE_PLACEMENT_NEW 0
#else
#   define U_HAVE_PLACEMENT_NEW 1
#endif

/**
 * \def U_HAVE_DEBUG_LOCATION_NEW 
 * Define this to define the MFC debug version of the operator new.
 *
 * @stable ICU 3.4
 */
#ifdef U_HAVE_DEBUG_LOCATION_NEW
    /* Use the predefined value. */
#elif defined(_MSC_VER)
#   define U_HAVE_DEBUG_LOCATION_NEW 1
#else
#   define U_HAVE_DEBUG_LOCATION_NEW 0
#endif

/** @} */

/*===========================================================================*/
/** @{ Character data types                                                  */
/*===========================================================================*/

/**
 * U_CHARSET_FAMILY is equal to this value when the platform is an ASCII based platform.
 * @stable ICU 2.0
 */
#define U_ASCII_FAMILY 0

/**
 * U_CHARSET_FAMILY is equal to this value when the platform is an EBCDIC based platform.
 * @stable ICU 2.0
 */
#define U_EBCDIC_FAMILY 1

/**
 * \def U_CHARSET_FAMILY
 *
 * <p>These definitions allow to specify the encoding of text
 * in the char data type as defined by the platform and the compiler.
 * It is enough to determine the code point values of "invariant characters",
 * which are the ones shared by all encodings that are in use
 * on a given platform.</p>
 *
 * <p>Those "invariant characters" should be all the uppercase and lowercase
 * latin letters, the digits, the space, and "basic punctuation".
 * Also, '\\n', '\\r', '\\t' should be available.</p>
 *
 * <p>The list of "invariant characters" is:<br>
 * \code
 *    A-Z  a-z  0-9  SPACE  "  %  &amp;  '  (  )  *  +  ,  -  .  /  :  ;  <  =  >  ?  _
 * \endcode
 * <br>
 * (52 letters + 10 numbers + 20 punc/sym/space = 82 total)</p>
 *
 * <p>This matches the IBM Syntactic Character Set (CS 640).</p>
 *
 * <p>In other words, all the graphic characters in 7-bit ASCII should
 * be safely accessible except the following:</p>
 *
 * \code
 *    '\' <backslash>
 *    '[' <left bracket>
 *    ']' <right bracket>
 *    '{' <left brace>
 *    '}' <right brace>
 *    '^' <circumflex>
 *    '~' <tilde>
 *    '!' <exclamation mark>
 *    '#' <number sign>
 *    '|' <vertical line>
 *    '$' <dollar sign>
 *    '@' <commercial at>
 *    '`' <grave accent>
 * \endcode
 * @stable ICU 2.0
 */
#ifdef U_CHARSET_FAMILY
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_OS390 && (!defined(__CHARSET_LIB) || !__CHARSET_LIB)
#   define U_CHARSET_FAMILY U_EBCDIC_FAMILY
#elif U_PLATFORM == U_PF_OS400 && !defined(__UTF32__)
#   define U_CHARSET_FAMILY U_EBCDIC_FAMILY
#else
#   define U_CHARSET_FAMILY U_ASCII_FAMILY
#endif

/**
 * \def U_CHARSET_IS_UTF8
 *
 * Hardcode the default charset to UTF-8.
 *
 * If this is set to 1, then
 * - ICU will assume that all non-invariant char*, StringPiece, std::string etc.
 *   contain UTF-8 text, regardless of what the system API uses
 * - some ICU code will use fast functions like u_strFromUTF8()
 *   rather than the more general and more heavy-weight conversion API (ucnv.h)
 * - ucnv_getDefaultName() always returns "UTF-8"
 * - ucnv_setDefaultName() is disabled and will not change the default charset
 * - static builds of ICU are smaller
 * - more functionality is available with the UCONFIG_NO_CONVERSION build-time
 *   configuration option (see unicode/uconfig.h)
 * - the UCONFIG_NO_CONVERSION build option in uconfig.h is more usable
 *
 * @stable ICU 4.2
 * @see UCONFIG_NO_CONVERSION
 */
#ifdef U_CHARSET_IS_UTF8
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_ANDROID || U_PLATFORM_IS_DARWIN_BASED
#   define U_CHARSET_IS_UTF8 1
#else
#   define U_CHARSET_IS_UTF8 0
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about wchar support                                       */
/*===========================================================================*/

/**
 * \def U_HAVE_WCHAR_H
 * Indicates whether <wchar.h> is available (1) or not (0). Set to 1 by default.
 *
 * @stable ICU 2.0
 */
#ifdef U_HAVE_WCHAR_H
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_ANDROID && __ANDROID_API__ < 9
    /*
     * Android before Gingerbread (Android 2.3, API level 9) did not support wchar_t.
     * The type and header existed, but the library functions did not work as expected.
     * The size of wchar_t was 1 but L"xyz" string literals had 32-bit units anyway.
     */
#   define U_HAVE_WCHAR_H 0
#else
#   define U_HAVE_WCHAR_H 1
#endif

/**
 * \def U_SIZEOF_WCHAR_T
 * U_SIZEOF_WCHAR_T==sizeof(wchar_t)
 *
 * @stable ICU 2.0
 */
#ifdef U_SIZEOF_WCHAR_T
    /* Use the predefined value. */
#elif (U_PLATFORM == U_PF_ANDROID && __ANDROID_API__ < 9) || U_PLATFORM == U_PF_CLASSIC_MACOS
    /*
     * Classic Mac OS and Mac OS X before 10.3 (Panther) did not support wchar_t or wstring.
     * Newer Mac OS X has size 4.
     */
#   define U_SIZEOF_WCHAR_T 1
#elif U_PLATFORM_HAS_WIN32_API || U_PLATFORM == U_PF_CYGWIN
#   define U_SIZEOF_WCHAR_T 2
#elif U_PLATFORM == U_PF_AIX
    /*
     * AIX 6.1 information, section "Wide character data representation":
     * "... the wchar_t datatype is 32-bit in the 64-bit environment and
     * 16-bit in the 32-bit environment."
     * and
     * "All locales use Unicode for their wide character code values (process code),
     * except the IBM-eucTW codeset."
     */
#   ifdef __64BIT__
#       define U_SIZEOF_WCHAR_T 4
#   else
#       define U_SIZEOF_WCHAR_T 2
#   endif
#elif U_PLATFORM == U_PF_OS390
    /*
     * z/OS V1R11 information center, section "LP64 | ILP32":
     * "In 31-bit mode, the size of long and pointers is 4 bytes and the size of wchar_t is 2 bytes.
     * Under LP64, the size of long and pointer is 8 bytes and the size of wchar_t is 4 bytes."
     */
#   ifdef _LP64
#       define U_SIZEOF_WCHAR_T 4
#   else
#       define U_SIZEOF_WCHAR_T 2
#   endif
#elif U_PLATFORM == U_PF_OS400
#   if defined(__UTF32__)
        /*
         * LOCALETYPE(*LOCALEUTF) is specified.
         * Wide-character strings are in UTF-32,
         * narrow-character strings are in UTF-8.
         */
#       define U_SIZEOF_WCHAR_T 4
#   elif defined(__UCS2__)
        /*
         * LOCALETYPE(*LOCALEUCS2) is specified.
         * Wide-character strings are in UCS-2,
         * narrow-character strings are in EBCDIC.
         */
#       define U_SIZEOF_WCHAR_T 2
#else
        /*
         * LOCALETYPE(*CLD) or LOCALETYPE(*LOCALE) is specified.
         * Wide-character strings are in 16-bit EBCDIC,
         * narrow-character strings are in EBCDIC.
         */
#       define U_SIZEOF_WCHAR_T 2
#   endif
#else
#   define U_SIZEOF_WCHAR_T 4
#endif

#ifndef U_HAVE_WCSCPY
#define U_HAVE_WCSCPY U_HAVE_WCHAR_H
#endif

/** @} */

/**
 * \def U_HAVE_CHAR16_T
 * Defines whether the char16_t type is available for UTF-16
 * and u"abc" UTF-16 string literals are supported.
 * This is a new standard type and standard string literal syntax in C++0x
 * but has been available in some compilers before.
 * @internal
 */
#ifdef U_HAVE_CHAR16_T
    /* Use the predefined value. */
#else
    /*
     * Notes:
     * Visual Studio 10 (_MSC_VER>=1600) defines char16_t but
     * does not support u"abc" string literals.
     * gcc 4.4 defines the __CHAR16_TYPE__ macro to a usable type but
     * does not support u"abc" string literals.
     */
#   define U_HAVE_CHAR16_T 0
#endif

/**
 * @{
 * \def U_DECLARE_UTF16
 * Do not use this macro because it is not defined on all platforms.
 * Use the UNICODE_STRING or U_STRING_DECL macros instead.
 * @internal
 */
#ifdef U_DECLARE_UTF16
    /* Use the predefined value. */
#elif (defined(__xlC__) && defined(__IBM_UTF_LITERAL) && U_SIZEOF_WCHAR_T != 2) \
    || (defined(__HP_aCC) && __HP_aCC >= 035000) \
    || (defined(__HP_cc) && __HP_cc >= 111106) \
    || U_HAVE_CHAR16_T
#   define U_DECLARE_UTF16(string) u ## string
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550)
/* || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x580) */
/* Sun's C compiler has issues with this notation, and it's unreliable. */
#   define U_DECLARE_UTF16(string) U ## string
#elif U_SIZEOF_WCHAR_T == 2 \
    && (U_CHARSET_FAMILY == 0 || (U_PF_OS390 <= U_PLATFORM && U_PLATFORM <= U_PF_OS400 && defined(__UCS2__)))
#   define U_DECLARE_UTF16(string) L ## string
#else
    /* Leave U_DECLARE_UTF16 undefined. See unistr.h. */
#endif

/** @} */

/*===========================================================================*/
/** @{ Symbol import-export control                                          */
/*===========================================================================*/

#ifdef U_EXPORT
    /* Use the predefined value. */
#elif defined(U_STATIC_IMPLEMENTATION)
#   define U_EXPORT
#elif defined(__GNUC__)
#   define U_EXPORT __attribute__((visibility("default")))
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550) \
   || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x550) 
#   define U_EXPORT __global
/*#elif defined(__HP_aCC) || defined(__HP_cc)
#   define U_EXPORT __declspec(dllexport)*/
#elif defined(_MSC_VER)
#   define U_EXPORT __declspec(dllexport)
#else
#   define U_EXPORT
#endif

/* U_CALLCONV is releated to U_EXPORT2 */
#ifdef U_EXPORT2
    /* Use the predefined value. */
#elif defined(_MSC_VER)
#   define U_EXPORT2 __cdecl
#else
#   define U_EXPORT2
#endif

#ifdef U_IMPORT
    /* Use the predefined value. */
#elif defined(_MSC_VER)
    /* Windows needs to export/import data. */
#   define U_IMPORT __declspec(dllimport)
#else
#   define U_IMPORT 
#endif

/**
 * \def U_CALLCONV
 * Similar to U_CDECL_BEGIN/U_CDECL_END, this qualifier is necessary
 * in callback function typedefs to make sure that the calling convention
 * is compatible.
 *
 * This is only used for non-ICU-API functions.
 * When a function is a public ICU API,
 * you must use the U_CAPI and U_EXPORT2 qualifiers.
 * @stable ICU 2.0
 */
#if U_PLATFORM == U_PF_OS390 && defined(__cplusplus)
#    define U_CALLCONV __cdecl
#else
#    define U_CALLCONV U_EXPORT2
#endif

/* @} */

#endif
