/*
**********************************************************************
* Copyright (c) 2002, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
**********************************************************************
*/
#ifndef _UPERF_H
#define _UPERF_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "utimer.h"
#include "uoptions.h"
#include "ucbuf.h"

U_NAMESPACE_USE
// Use the TESTCASE macro in subclasses of IntlTest.  Define the
// runIndexedTest method in this fashion:
//
//| void MyTest::runIndexedTest(int32_t index, UBool exec,
//|                             const char* &name, char* /*par*/) {
//|     switch (index) {
//|         TESTCASE(0,TestSomething);
//|         TESTCASE(1,TestSomethingElse);
//|         TESTCASE(2,TestAnotherThing);
//|         default: 
//|             name = ""; 
//|             return NULL;
//|     }
//| }
#define TESTCASE(id,test)                       \
    case id:                                    \
        name = #test;                           \
        if (exec) {                             \
            fprintf(stdout,#test "---");        \
            fprintf(stdout,"\n");               \
            return test();                      \
        }                                       \
        break

/**
 * Subclasses of PerfTest will need to create subclasses of
 * Function that define a call() method which contains the code to
 * be timed.  They then call setTestFunction() in their "Test..."
 * method to establish this as the current test functor.
 */
class U_EXPORT UPerfFunction {
public:
    /**
     * Subclasses must implement this method to do the action to be
     * measured.
     */
    virtual void call()=0;

    /**
     * Subclasses must implement this method to return positive
     * integer indicating the number of operations in a single
     * call to this object's call() method.
     */
    virtual long getOperationsPerIteration()=0;
    /**
     * Subclasses must implement this method to return either positive
     * or negative integer indicating the number of events in a single
     * call to this object's call() method. 
     * e.g: Number of breaks / iterations for break iterator
     */
    virtual long getEventsPerIteration()=0;

    /**
     * Call call() n times in a tight loop and return the elapsed
     * milliseconds.  If n is small and call() is fast the return
     * result may be zero.  Small return values have limited
     * meaningfulness, depending on the underlying CPU and OS.
     */
     double time(double n) {
        UTimer start, stop;
        utimer_getTime(&start); 
        while (n-- > 0) {
            call();
        }
        utimer_getTime(&stop);
        return utimer_getDeltaSeconds(&start,&stop); // ms
    }
	/**
	 * Subclasses must implement this method to return any
	 * errors that may have occured during performing an
	 * operation
	 */
	virtual UErrorCode getStatus()=0;

};


class U_EXPORT UPerfTest {
public:
    UBool run();
    virtual UBool runTest( char* name = NULL, char* par = NULL ); // not to be overidden
        
    virtual void usage( void ) ;
    
    virtual ~UPerfTest();

    void setCaller( UPerfTest* callingTest ); // for internal use only
    
    void setPath( char* path ); // for internal use only
    
    ULine* getLines(UErrorCode& status);

    const UChar* getBuffer(int32_t& len,UErrorCode& status);

protected:
    UPerfTest(int32_t argc, const char* argv[], UErrorCode& status);

    virtual UPerfFunction* runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL ); // overide !

    virtual UBool runTestLoop( char* testname, char* par );

    virtual UBool callTest( UPerfTest& testToBeCalled, char* par );

    UBool        verbose;
    const char*  sourceDir;
    const char*  fileName;
    char*        resolvedFileName;
    const char*  encoding;
    UBool        uselen;
    int32_t      iterations;
    int32_t      passes;
    int32_t      time;
    const char** _argv;
    int32_t      _argc;
    int32_t      _remainingArgc;
    ULine*       lines;
    int32_t      numLines;
    UCHARBUF*    ucharBuf;
    UBool        line_mode;
    UBool        bulk_mode;
    const UChar* buffer;
    int32_t      bufferLen;
private:
    UPerfTest*   caller;
    char*        path;           // specifies subtests

// static members
public:
    static UPerfTest* gTest;
    static const char* gUsageString;
};

const char* UPerfTest::gUsageString =
                "Usage: %s [OPTIONS] [FILES]\n"
                "\tReads the input file and prints out time taken in seconds\n"
                "Options:\n"
                "\t-h or -? or --help       this usage text\n"
                "\t-v or --verbose          print extra information when processing files\n"
                "\t-s or --sourcedir        source directory for files followed by path\n"
                "\t                         followed by path\n"
                "\t-e or --encoding         encoding of source files\n"
                "\t-u or --uselen           perform timing analysis on non-null terminated buffer using length\n"
                "\t-f or --file-name        file to be used as input data\n"
                "\t-p or --passes           Number of passes to be performed. Requires Numeric argument. Cannot be used with --time\n"
                "\t-i or --iterations       Number of iterations to be performed. Requires Numeric argument\n"
                "\t-t or --time             Threshold time for looping until in seconds. Requires Numeric argument.Cannot be used with --iterations\n"
                "\t-l or --line-mode        The data file should be processed in line mode\n"
                "\t-b or --bulk-mode        The data file should be processed in file based. Cannot be used with --line-mode\n";

#endif

