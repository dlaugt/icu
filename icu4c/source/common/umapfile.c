/*
******************************************************************************
*
*   Copyright (C) 1999-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************/


/*----------------------------------------------------------------------------
 *                           
 *       Memory mapped file wrappers for use by the ICU Data Implementation
 *       All of the platform-specific implementation for mapping data files
 *         is here.  The rest of the ICU Data implementation uses only the
 *         wrapper functions.
 *
 *----------------------------------------------------------------------------*/
#include "unicode/utypes.h"


#include "udatamem.h"
#include "umapfile.h"


/* memory-mapping base definitions ------------------------------------------ */


#define MAP_WIN32       1
#define MAP_POSIX       2
#define MAP_FILE_STREAM 3
#define MAP_390DLL      4

#ifdef WIN32
#   define WIN32_LEAN_AND_MEAN
#   define NOGDI
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>

    typedef HANDLE MemoryMap;

#   define IS_MAP(map) ((map)!=NULL)

#   define MAP_IMPLEMENTATION MAP_WIN32

/* ### Todo: properly auto detect mmap(). Until then, just add your platform here. */
#elif U_HAVE_MMAP || defined(AIX) || defined(HPUX) || defined(OS390) || defined(PTX)
    typedef size_t MemoryMap;

#   define IS_MAP(map) ((map)!=0)

#   include <unistd.h>
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <fcntl.h>

#   ifndef MAP_FAILED
#       define MAP_FAILED ((void*)-1)
#   endif

#   if defined(OS390) && defined (OS390_STUBDATA)
        /*   No memory mapping for 390 batch mode.  Fake it using dll loading.  */
#       include <dll.h>
#       include "cstring.h"
#       include "cmemory.h"
#       include "unicode/udata.h"
#       define LIB_PREFIX "lib"
#       define LIB_SUFFIX ".dll"
#       define MAP_IMPLEMENTATION MAP_390DLL

/* This is inconvienient until we figure out what to do with U_ICUDATA_NAME in utypes.h */
#       define U_ICUDATA_ENTRY_NAME "icudt" U_ICU_VERSION_SHORT U_LIB_SUFFIX_C_NAME_STRING "_dat"
#   else
#       define MAP_IMPLEMENTATION MAP_POSIX
#   endif

#else /* unknown platform, no memory map implementation: use FileStream/uprv_malloc() instead */

#   include "filestrm.h"

    typedef void *MemoryMap;

#   define IS_MAP(map) ((map)!=NULL)

#   define MAP_IMPLEMENTATION MAP_FILE_STREAM

#endif




/*----------------------------------------------------------------------------*
 *                                                                            *
 *   Memory Mapped File support.  Platform dependent implementation of        *
 *                           functions used by the rest of the implementation.*
 *                                                                            *
 *----------------------------------------------------------------------------*/
#if MAP_IMPLEMENTATION==MAP_WIN32
    UBool
    uprv_mapFile(
         UDataMemory *pData,    /* Fill in with info on the result doing the mapping. */
                                /*   Output only; any original contents are cleared.  */
         const char *path       /* File path to be opened/mapped                      */
         )
    {
        HANDLE map;
        HANDLE file;

        UDataMemory_init(pData); /* Clear the output struct.        */

        /* open the input file */
        file=CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS, NULL);
        if(file==INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        /* create an unnamed Windows file-mapping object for the specified file */
        map=CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
        CloseHandle(file);
        if(map==NULL) {
            return FALSE;
        }

        /* map a view of the file into our address space */
        pData->pHeader=(const DataHeader *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
        if(pData->pHeader==NULL) {
            CloseHandle(map);
            return FALSE;
        }
        pData->map=map;
        return TRUE;
    }


    void
    uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            UnmapViewOfFile(pData->pHeader);
            CloseHandle(pData->map);
            pData->pHeader=NULL;
            pData->map=NULL;
        }
    }



#elif MAP_IMPLEMENTATION==MAP_POSIX
    UBool
    uprv_mapFile(UDataMemory *pData, const char *path) {
        int fd;
        int length;
        struct stat mystat;
        void *data;

        UDataMemory_init(pData); /* Clear the output struct.        */

        /* determine the length of the file */
        if(stat(path, &mystat)!=0 || mystat.st_size<=0) {
            return FALSE;
        }
        length=mystat.st_size;

        /* open the file */
        fd=open(path, O_RDONLY);
        if(fd==-1) {
            return FALSE;
        }

        /* get a view of the mapping */
#ifndef HPUX
        data=mmap(0, length, PROT_READ, MAP_SHARED,  fd, 0);
#else
        data=mmap(0, length, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
        close(fd); /* no longer needed */
        if(data==MAP_FAILED) {
            return FALSE;
        }

        pData->map = (char *)data + length;
        pData->pHeader=(const DataHeader *)data;
        pData->mapAddr = data;
        return TRUE;
    }

    
    
    void
    uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            size_t dataLen = (char *)pData->map - (char *)pData->mapAddr;
            if(munmap(pData->mapAddr, dataLen)==-1) {
            }
            pData->pHeader=NULL;
            pData->map=0;
            pData->mapAddr=NULL;
        }
    }



#elif MAP_IMPLEMENTATION==MAP_FILE_STREAM
    static UBool
    uprv_mapFile(UDataMemory *pData, const char *path) {
        FileStream *file;
        int32_t fileLength;
        void *p;

        UDataMemory_init(pData); /* Clear the output struct.        */
        /* open the input file */
        file=T_FileStream_open(path, "rb");
        if(file==NULL) {
            return FALSE;
        }

        /* get the file length */
        fileLength=T_FileStream_size(file);
        if(T_FileStream_error(file) || fileLength<=20) {
            T_FileStream_close(file);
            return FALSE;
        }

        /* allocate the memory to hold the file data */
        p=uprv_malloc(fileLength);
        if(p==NULL) {
            T_FileStream_close(file);
            return FALSE;
        }

        /* read the file */
        if(fileLength!=T_FileStream_read(file, p, fileLength)) {
            uprv_free(p);
            T_FileStream_close(file);
            return FALSE;
        }

        T_FileStream_close(file);
        pData->map=p;
        pData->pHeader=(const DataHeader *)p;
        pData->mapAddr=p;
        return TRUE;
    }

    static void
    uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            uprv_free(pData->map);
            pData->map     = NULL;
            pData->mapAddr = NULL;
            pData->pHeader = NULL;
        }
    }


#elif MAP_IMPLEMENTATION==MAP_390DLL
    /*  390 specific Library Loading.
     *  This is the only platform left that dynamically loads an ICU Data Library.
     *  All other platforms use .data files when dynamic loading is required, but
     *  this turn out to be awkward to support in 390 batch mode.
     *
     *  The idea here is to hide the fact that 390 is using dll loading from the
     *   rest of ICU, and make it look like there is file loading happening.
     *
     */

#   define DATA_TYPE "dat"

    UBool uprv_mapFile(UDataMemory *pData, const char *path) {
        const char *inBasename;
        char *basename;
        char pathBuffer[1024];
        const DataHeader *pHeader;
        dllhandle *handle;
        void *val=0;

        inBasename=uprv_strrchr(path, U_FILE_SEP_CHAR);
        if(inBasename==NULL) {
            inBasename = path;
        } else {
            inBasename++;
        }
        basename=uprv_computeDirPath(path, pathBuffer);
        if(uprv_strcmp(inBasename, U_ICUDATA_NAME".dat") != 0) {
            /* must mmap file... for build */
            int fd;
            int length;
            struct stat mystat;
            void *data;
            UDataMemory_init(pData); /* Clear the output struct. */

            /* determine the length of the file */
            if(stat(path, &mystat)!=0 || mystat.st_size<=0) {
                return FALSE;
            }
            length=mystat.st_size;

            /* open the file */
            fd=open(path, O_RDONLY);
            if(fd==-1) {
                return FALSE;
            }

            /* get a view of the mapping */
            data=mmap(0, length, PROT_READ, MAP_PRIVATE, fd, 0);
            close(fd); /* no longer needed */
            if(data==MAP_FAILED) {
                return FALSE;
            }
            pData->map = (char *)data + length;
            pData->pHeader=(const DataHeader *)data;
            pData->mapAddr = data;
            return TRUE;
        }

#       ifdef OS390BATCH
        /* ### hack: we still need to get u_getDataDirectory() fixed
        for OS/390 (batch mode - always return "//"? )
        and this here straightened out with LIB_PREFIX and LIB_SUFFIX (both empty?!)
        This is probably due to the strange file system on OS/390.  It's more like
        a database with short entry names than a typical file system. */
            /* U_ICUDATA_NAME should always have the correct name */
            /* 390port: BUT FOR BATCH MODE IT IS AN EXCEPTION ... */
            /* 390port: THE NEXT LINE OF CODE WILL NOT WORK !!!!! */
            /*lib=LOAD_LIBRARY("//" U_ICUDATA_NAME, "//" U_ICUDATA_NAME);*/
            uprv_strcpy(pathBuffer, "//IXMI" U_ICU_VERSION_SHORT "DA");
#       else
            /* set up the library name */
            uprv_strcpy(basename, LIB_PREFIX U_LIBICUDATA_NAME U_ICU_VERSION_SHORT LIB_SUFFIX);
#       endif

#       ifdef UDATA_DEBUG
             fprintf(stderr, "dllload: %s ", pathBuffer);
#       endif

        handle=dllload(pathBuffer);

#       ifdef UDATA_DEBUG
               fprintf(stderr, " -> %08X\n", handle );
#       endif

        if(handle != NULL) {
               /* we have a data DLL - what kind of lookup do we need here? */
               /* try to find the Table of Contents */
               UDataMemory_init(pData); /* Clear the output struct.        */
               val=dllqueryvar((dllhandle*)handle, U_ICUDATA_ENTRY_NAME);
               if(val == 0) {
                    /* failed... so keep looking */
                    return FALSE;
               }
#              ifdef UDATA_DEBUG
                    fprintf(stderr, "dllqueryvar(%08X, %s) -> %08X\n", handle, U_ICUDATA_ENTRY_NAME, val);
#              endif

               pData->pHeader=(const DataHeader *)val;
               return TRUE;
         } else {
               return FALSE; /* no handle */
         }
    }



    void uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            uprv_free(pData->map);
            pData->map     = NULL;
            pData->mapAddr = NULL;
            pData->pHeader = NULL;
        }   
    }

#else
#   error MAP_IMPLEMENTATION is set incorrectly
#endif


