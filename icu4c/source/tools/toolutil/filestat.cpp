/******************************************************************************
 *   Copyright (C) 2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 */

#include "filestat.h"
#include "cstring.h"
#include "unicode/putil.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#if U_HAVE_DIRENT_H
#include <dirent.h>
typedef struct dirent DIRENT;

#define MAX_PATH_SIZE PATH_MAX /* Set the limit for the size of the path. */
#define MAX_READ_SIZE 512

#define SKIP1 "."
#define SKIP2 ".."
#endif

typedef struct stat STAT;

static int32_t whichFileModTimeIsLater(const char *file1, const char *file2);

/*
 * Goes through the given directory recursive to compare each file's modification time with that of the file given.
 */
U_CAPI UBool U_EXPORT2
isFileModTimeLater(const char *filePath, const char *dirToCheckAgainst) {
    UBool isLatest = TRUE;

    if (filePath == NULL || dirToCheckAgainst == NULL) {
        return FALSE;
    }

#if U_HAVE_DIRENT_H
    DIR *pDir = NULL;
    if ((pDir= opendir(dirToCheckAgainst)) != NULL) {
        DIR *subDirp = NULL;
        DIRENT *dirEntry = NULL;

        while ((dirEntry = readdir(pDir)) != NULL) {
            if (uprv_strcmp(dirEntry->d_name, SKIP1) != 0 && uprv_strcmp(dirEntry->d_name, SKIP2) != 0) {
                char newpath[MAX_PATH_SIZE] = "";
                uprv_strcpy(newpath, dirToCheckAgainst);
                uprv_strcat(newpath, U_FILE_SEP_STRING);
                uprv_strcat(newpath, dirEntry->d_name);

                if ((subDirp = opendir(newpath)) != NULL) {
                    /* If this new path is a directory, make a recursive call with the newpath. */
                    closedir(subDirp);
                    isLatest = isFileModTimeLater(filePath, newpath);
                    if (!isLatest) {
                        break;
                    }
                } else {
                    int32_t latest = whichFileModTimeIsLater(filePath, newpath);
                    if (latest < 0 || latest == 2) {
                        isLatest = FALSE;
                        break;
                    }
                }

            }
        }
        closedir(pDir);
    } else {
        fprintf(stderr, "Unable to open directory: %s\n", dirToCheckAgainst);
        return FALSE;
    }
#endif
    return isLatest;
}

/* Compares the mod time of both files returning a number indicating which one is later. -1 if error ocurs. */
static int32_t whichFileModTimeIsLater(const char *file1, const char *file2) {
    int32_t result = 0;
    STAT stbuf1, stbuf2;

    if (stat(file1, &stbuf1) == 0 && stat(file2, &stbuf2) == 0) {
        time_t modtime1, modtime2;
        double diff;

        modtime1 = stbuf1.st_mtim.tv_sec;
        modtime2 = stbuf2.st_mtim.tv_sec;

        diff = difftime(modtime1, modtime2);
        if (diff < 0.0) {
            result = 2;
        } else if (diff > 0.0) {
            result = 1;
        }

    } else {
        fprintf(stderr, "Unable to get stats from file: %s or %s\n", file1, file2);
        result = -1;
    }

    return result;
}
