/*
*******************************************************************************
*
*   Copyright (C) 1998-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File genrb.c
*
* Modification History:
*
*   Date        Name        Description
*   05/25/99    stephen     Creation.
*   5/10/01     Ram         removed ustdio dependency
*******************************************************************************
*/

#include "genrb.h"
#include "unicode/uclean.h"

/* Protos */
static void  processFile(const char *filename, const char* cp, const char *inputDir, const char *outputDir, const char *packageName, UErrorCode *status);
static char *make_res_filename(const char *filename, const char *outputDir,
                               const char *packageName, UErrorCode *status);

/* File suffixes */
#define RES_SUFFIX ".res"
#define COL_SUFFIX ".col"

static char theCurrentFileName[2048];
const char *gCurrentFileName = theCurrentFileName;
#ifdef XP_MAC_CONSOLE
#include <console.h>
#endif

enum
{
    HELP1,
    HELP2,
    VERBOSE,
    QUIET,
    VERSION,
    SOURCEDIR,
    DESTDIR,
    ENCODING,
    ICUDATADIR,
    WRITE_JAVA,
    COPYRIGHT,
    PACKAGE_NAME,
    BUNDLE_NAME,
    WRITE_XLIFF,
    STRICT,
    NO_BINARY_COLLATION,
    /*added by Jing*/
    LANGUAGE
};

UOption options[]={
                      UOPTION_HELP_H,
                      UOPTION_HELP_QUESTION_MARK,
                      UOPTION_VERBOSE,
                      UOPTION_QUIET,
                      UOPTION_VERSION,
                      UOPTION_SOURCEDIR,
                      UOPTION_DESTDIR,
                      UOPTION_ENCODING,
                      UOPTION_ICUDATADIR,
                      UOPTION_WRITE_JAVA,
                      UOPTION_COPYRIGHT,
                      UOPTION_PACKAGE_NAME,
                      UOPTION_BUNDLE_NAME,
                      UOPTION_DEF( "write-xliff", 'x', UOPT_OPTIONAL_ARG),
                      UOPTION_DEF( "strict",    'k', UOPT_NO_ARG), /* 14 */
                      UOPTION_DEF( "noBinaryCollation", 'C', UOPT_NO_ARG),/* 15 */
                      UOPTION_DEF( "language",  'l', UOPT_REQUIRES_ARG)
                  };

static     UBool       write_java = FALSE;
static     UBool       write_xliff = FALSE;
static     const char* outputEnc ="";
static     const char* gPackageName=NULL;
static     const char* bundleName=NULL;
/*added by Jing*/
static     const char* language = NULL;
static     const char* xliffOutputFileName = NULL;
int
main(int argc,
     char* argv[])
{
    UErrorCode  status    = U_ZERO_ERROR;
    const char *arg       = NULL;
    const char *outputDir = NULL; /* NULL = no output directory, use current */
    const char *inputDir  = NULL;
    const char *encoding  = "";
    int         i;

    U_MAIN_INIT_ARGS(argc, argv);

    argc = u_parseArgs(argc, argv, (int32_t)(sizeof(options)/sizeof(options[0])), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr, "%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
    } else if(argc<2) {
        argc = -1;
    }

    if(options[VERSION].doesOccur) {
        fprintf(stderr,
                "%s version %s (ICU version %s).\n"
                "%s\n",
                argv[0], GENRB_VERSION, U_ICU_VERSION, U_COPYRIGHT_STRING);
        return U_ZERO_ERROR;
    }

    if(argc<0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
        /*
         * Broken into chucks because the C89 standard says the minimum
         * required supported string length is 509 bytes.
         */
        fprintf(stderr,
                "Usage: %s [OPTIONS] [FILES]\n"
                "\tReads the list of resource bundle source files and creates\n"
                "\tbinary version of reosurce bundles (.res files)\n",
                argv[0]);
        fprintf(stderr,
                "Options:\n"
                "\t-h or -? or --help       this usage text\n"
                "\t-q or --quiet            do not display warnings\n"
                "\t-v or --verbose          print extra information when processing files\n"
                "\t-V or --version          prints out version number and exits\n"
                "\t-c or --copyright        include copyright notice\n");
        fprintf(stderr,
                "\t-e or --encoding         encoding of source files\n"
                "\t-d of --destdir          destination directory, followed by the path, defaults to %s\n"
                "\t-s or --sourcedir        source directory for files followed by path, defaults to %s\n"
                "\t-i or --icudatadir       directory for locating any needed intermediate data files,\n"
                "\t                         followed by path, defaults to %s\n",
                u_getDataDirectory(), u_getDataDirectory(), u_getDataDirectory());
        fprintf(stderr,
                "\t-j or --write-java       write a Java ListResourceBundle for ICU4J, followed by optional encoding\n"
                "\t                         defaults to ASCII and \\uXXXX format.\n"
                "\t-p or --package-name     For ICU4J: package name for writing the ListResourceBundle for ICU4J,\n"
                "\t                         defaults to com.ibm.icu.impl.data\n");
        fprintf(stderr,
                "\t-b or --bundle-name      bundle name for writing the ListResourceBundle for ICU4J,\n"
                "\t                         defaults to LocaleElements\n"
                "\t-x or --write-xliff      write a XLIFF file for the resource bundle. Followed by an optional output file name.\n"
                "\t-k or --strict           use pedantic parsing of syntax\n"
                /*added by Jing*/
                "\t-l or --language         For XLIFF: language code compliant with ISO 639.\n");

        return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    if(options[VERBOSE].doesOccur) {
        setVerbose(TRUE);
    }

    if(options[QUIET].doesOccur) {
        setShowWarning(FALSE);
    }
    if(options[STRICT].doesOccur) {
        setStrict(TRUE);
    }
    if(options[COPYRIGHT].doesOccur){
        setIncludeCopyright(TRUE);
    }

    if(options[SOURCEDIR].doesOccur) {
        inputDir = options[SOURCEDIR].value;
    }

    if(options[DESTDIR].doesOccur) {
        outputDir = options[DESTDIR].value;
    }
    if(options[PACKAGE_NAME].doesOccur) {
        gPackageName = options[PACKAGE_NAME].value;
        if(!strcmp(gPackageName, "ICUDATA"))
        {
            gPackageName = U_ICUDATA_NAME;
        }
        if(gPackageName[0] == 0)
        {
            gPackageName = NULL;
        }
    }

    if(options[ENCODING].doesOccur) {
        encoding = options[ENCODING].value;
    }

    if(options[ICUDATADIR].doesOccur) {
        u_setDataDirectory(options[ICUDATADIR].value);
    }
    /* Initialize ICU */
    u_init(&status);
    if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
        /* Note: u_init() will try to open ICU property data.
         *       failures here are expected when building ICU from scratch.
         *       ignore them.
        */
        fprintf(stderr, "%s: can not initialize ICU.  status = %s\n",
            argv[0], u_errorName(status));
        exit(1);
    }
    status = U_ZERO_ERROR;
    if(options[WRITE_JAVA].doesOccur) {
        write_java = TRUE;
        outputEnc = options[WRITE_JAVA].value;
    }

    if(options[BUNDLE_NAME].doesOccur) {
        bundleName = options[BUNDLE_NAME].value;
    }

    if(options[WRITE_XLIFF].doesOccur) {
        write_xliff = TRUE;
        if(options[WRITE_XLIFF].value != NULL){
            xliffOutputFileName = options[WRITE_XLIFF].value;
        }
    }

    if(options[NO_BINARY_COLLATION].doesOccur) {
      initParser(FALSE);
    } else {
      initParser(TRUE);
    }

    /*added by Jing*/
    if(options[LANGUAGE].doesOccur) {
        language = options[LANGUAGE].value;
    }

    /* generate the binary files */
    for(i = 1; i < argc; ++i) {
        status = U_ZERO_ERROR;
        arg    = getLongPathname(argv[i]);

        if (inputDir) {
            uprv_strcpy(theCurrentFileName, inputDir);
            uprv_strcat(theCurrentFileName, U_FILE_SEP_STRING);
        } else {
            *theCurrentFileName = 0;
        }
        uprv_strcat(theCurrentFileName, arg);

        if (isVerbose()) {
            printf("Processing file \"%s\"\n", theCurrentFileName);
        }
        processFile(arg, encoding, inputDir, outputDir, gPackageName, &status);
    }

    return status;
}

/* Process a file */
static void
processFile(const char *filename, const char *cp, const char *inputDir, const char *outputDir, const char *packageName, UErrorCode *status) {
    /*FileStream     *in           = NULL;*/
    struct SRBRoot *data         = NULL;
    UCHARBUF       *ucbuf        = NULL;
    char           *rbname       = NULL;
    char           *openFileName = NULL;
    char           *inputDirBuf  = NULL;

    char           outputFileName[256];

    int32_t dirlen  = 0;
    int32_t filelen = 0;

    if (status==NULL || U_FAILURE(*status)) {
        return;
    }
    if(filename==NULL){
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }else{
        filelen = (int32_t)uprv_strlen(filename);
    }
    if(inputDir == NULL) {
        const char *filenameBegin = uprv_strrchr(filename, U_FILE_SEP_CHAR);
        openFileName = (char *) uprv_malloc(dirlen + filelen + 2);
        openFileName[0] = '\0';
        if (filenameBegin != NULL) {
            /*
             * When a filename ../../../data/root.txt is specified,
             * we presume that the input directory is ../../../data
             * This is very important when the resource file includes
             * another file, like UCARules.txt or thaidict.brk.
             */
            int32_t filenameSize = (int32_t)(filenameBegin - filename + 1);
            inputDirBuf = uprv_strncpy((char *)uprv_malloc(filenameSize), filename, filenameSize);

            /* test for NULL */
            if(inputDirBuf == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto finish;
            }

            inputDirBuf[filenameSize - 1] = 0;
            inputDir = inputDirBuf;
            dirlen  = (int32_t)uprv_strlen(inputDir);
        }
    }else{
        dirlen  = (int32_t)uprv_strlen(inputDir);

        if(inputDir[dirlen-1] != U_FILE_SEP_CHAR) {
            openFileName = (char *) uprv_malloc(dirlen + filelen + 2);

            /* test for NULL */
            if(openFileName == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto finish;
            }

            openFileName[0] = '\0';
            /*
             * append the input dir to openFileName if the first char in
             * filename is not file seperation char and the last char input directory is  not '.'.
             * This is to support :
             * genrb -s. /home/icu/data
             * genrb -s. icu/data
             * The user cannot mix notations like
             * genrb -s. /icu/data --- the absolute path specified. -s redundant
             * user should use
             * genrb -s. icu/data  --- start from CWD and look in icu/data dir
             */
            if( (filename[0] != U_FILE_SEP_CHAR) && (inputDir[dirlen-1] !='.')){
                uprv_strcpy(openFileName, inputDir);
                openFileName[dirlen]     = U_FILE_SEP_CHAR;
            }
            openFileName[dirlen + 1] = '\0';
        } else {
            openFileName = (char *) uprv_malloc(dirlen + filelen + 1);

            /* test for NULL */
            if(openFileName == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto finish;
            }

            uprv_strcpy(openFileName, inputDir);

        }
    }

    uprv_strcat(openFileName, filename);

    ucbuf = ucbuf_open(openFileName, &cp,getShowWarning(),TRUE, status);

    if(*status == U_FILE_ACCESS_ERROR) {

        fprintf(stderr, "couldn't open file %s\n", openFileName == NULL ? filename : openFileName);
        goto finish;
    }
    if (ucbuf == NULL || U_FAILURE(*status)) {
        fprintf(stderr, "An error occured processing file %s. Error: %s\n", openFileName == NULL ? filename : openFileName,u_errorName(*status));
        goto finish;
    }
    /* auto detected popular encodings? */
    if (cp!=NULL && isVerbose()) {
        printf("autodetected encoding %s\n", cp);
    }
    /* Parse the data into an SRBRoot */
    data = parse(ucbuf, inputDir, status);

    if (data == NULL || U_FAILURE(*status)) {
        fprintf(stderr, "couldn't parse the file %s. Error:%s\n", filename,u_errorName(*status));
        goto finish;
    }

    /* Determine the target rb filename */
    rbname = make_res_filename(filename, outputDir, packageName, status);
    if(U_FAILURE(*status)) {
        fprintf(stderr, "couldn't make the res fileName for  bundle %s. Error:%s\n", filename,u_errorName(*status));
        goto finish;
    }
    if(write_java== TRUE){
        bundle_write_java(data,outputDir,outputEnc, outputFileName, sizeof(outputFileName),packageName,bundleName,status);
    }else if(write_xliff ==TRUE){
        bundle_write_xml(data,outputDir,outputEnc, filename, outputFileName, sizeof(outputFileName),language, xliffOutputFileName,status);
    }else{
        /* Write the data to the file */
        bundle_write(data, outputDir, packageName, outputFileName, sizeof(outputFileName), status);
    }
    if (U_FAILURE(*status)) {
        fprintf(stderr, "couldn't write bundle %s. Error:%s\n", outputFileName,u_errorName(*status));
    }
    bundle_close(data, status);

finish:

    if (inputDirBuf != NULL) {
        uprv_free(inputDirBuf);
    }

    if (openFileName != NULL) {
        uprv_free(openFileName);
    }

    if(ucbuf) {
        ucbuf_close(ucbuf);
    }

    if (rbname) {
        uprv_free(rbname);
    }
}

/* Generate the target .res file name from the input file name */
static char*
make_res_filename(const char *filename,
                  const char *outputDir,
                  const char *packageName,
                  UErrorCode *status) {
    char *basename;
    char *dirname;
    char *resName;

    int32_t pkgLen = 0; /* length of package prefix */

    if (U_FAILURE(*status)) {
        return 0;
    }

    if(packageName != NULL)
    {
        pkgLen = (int32_t)(1 + uprv_strlen(packageName));
    }

    /* setup */
    basename = dirname = resName = 0;

    /* determine basename, and compiled file names */
    basename = (char*) uprv_malloc(sizeof(char) * (uprv_strlen(filename) + 1));
    if(basename == 0) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        goto finish;
    }

    get_basename(basename, filename);

    dirname = (char*) uprv_malloc(sizeof(char) * (uprv_strlen(filename) + 1));
    if(dirname == 0) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        goto finish;
    }

    get_dirname(dirname, filename);

    if (outputDir == NULL) {
        /* output in same dir as .txt */
        resName = (char*) uprv_malloc(sizeof(char) * (uprv_strlen(dirname)
                                      + pkgLen
                                      + uprv_strlen(basename)
                                      + uprv_strlen(RES_SUFFIX) + 8));
        if(resName == 0) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto finish;
        }

        uprv_strcpy(resName, dirname);

        if(packageName != NULL)
        {
            uprv_strcat(resName, packageName);
            uprv_strcat(resName, "_");
        }

        uprv_strcat(resName, basename);

    } else {
        int32_t dirlen      = (int32_t)uprv_strlen(outputDir);
        int32_t basenamelen = (int32_t)uprv_strlen(basename);

        resName = (char*) uprv_malloc(sizeof(char) * (dirlen + pkgLen + basenamelen + 8));

        if (resName == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto finish;
        }

        uprv_strcpy(resName, outputDir);

        if(outputDir[dirlen] != U_FILE_SEP_CHAR) {
            resName[dirlen]     = U_FILE_SEP_CHAR;
            resName[dirlen + 1] = '\0';
        }

        if(packageName != NULL)
        {
            uprv_strcat(resName, packageName);
            uprv_strcat(resName, "_");
        }

        uprv_strcat(resName, basename);
    }

finish:
    uprv_free(basename);
    uprv_free(dirname);

    return resName;
}

/*
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 */
