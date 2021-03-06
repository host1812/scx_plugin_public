/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Main routine for LogFileReader command line program.
 
    \date      2011-04-14 17:10:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxdefaultlogpolicyfactory.h> // Using the default log policy.
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxmarshal.h>
#include <scxcorelib/stringaid.h>
#include "source/code/shared/scxcorelib/util/persist/scxfilepersistmedia.h"

#include <errno.h>
#include <iostream>
#include <unistd.h>

#include "buildversion.h"
#include "logfileutils.h"

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif

using namespace SCXCore;
using namespace SCXCoreLib;
using namespace std;

static void usage(const char * name, bool fBrief, int exitValue);
static void show_version();
static void PerformMarshalTest();
static int ReadLogFile_Interactive();
static int ReadLogFile_Provider();
static void ReadLogFile_TestSetup();

SCXHandle<SCXPersistMedia> s_pmedia;
SCXCoreLib::SCXHandle<LogFileReader> s_pReader;

static bool s_fTestMode = false;
extern int optopt, opterr;

// This is the main entry point for the logfilereader command line program.
//
// It will, by default, parse arguments, unmarshall input (from Logfile
// Provider), perform processing, and marshall up the results.

/*----------------------------------------------------------------------------*/
/**
   logfilereader (main) function.

   \param argc size of \a argv[]
   \param argv array of string pointers from the command line.
   \returns 0 on success, otherwise, 1 on error.

   Usage: 
   Result Code
   \n  0  success
   \n >1  an error occured while executing the command.
*/

int main(int argc, char * const argv[])
{
    int exitStatus = 0;
    bool fRunProvider = false;
    int c;

    // If no arguments, show brief usage help.

    if (1 == argc)
    {
        usage(argv[0], true, 0);
    }

    // Parse the arguments
    //
    // Illegal arguments are displayed slightly different across our platforms.
    // To help allow for consistent output, override error output and handle it
    // ourselves via the opterr variable.

    opterr = 0;                 // Disable printing errors for bad options
    while ((c = getopt(argc, argv, "hi?mptv")) != -1) {
        switch(c) {
            case 'h':                   /* Show extended help information */
                usage(argv[0], false, 0);
                /*NOTREACHED*/
                break;
            case 'i':                   /* Interactive (prompted) use */
                ReadLogFile_Interactive();
                fRunProvider = false;
                break;
            case 'm':
                PerformMarshalTest();
                fRunProvider = false;
                break;
            case 'p':                   /* Provider entry (read log file) */
                fRunProvider = true;
                break;
            case 't':                   /* Test mode requested (check for testrunner) */
                s_fTestMode = true;
                break;
            case 'v':                   /* Show version info */
                show_version();
                fRunProvider = false;
                break;
            case '?':                   /* Show basic help information */
                // If invalid parameter, return error result
                if (0 != optopt && '\?' != optopt)
                {
                    cerr << argv[0]
                         << ": invalid option -- '"
                         << (char) optopt 
                         << "'" << endl;
                    usage(argv[0], true, 64 /* Random exit code that is not ENOENT */);
                }
                /*FALL THRU*/
            default:
                usage(argv[0], true, 0);
                /*NOTREACHED*/
                break;
        }
    }

    if (fRunProvider) {
        exitStatus = ReadLogFile_Provider();
    }

    return exitStatus;
}


/*----------------------------------------------------------------------------*/
/**
   Output a usage message.
   
   \param name Application name (derived from argv[0]).
   \param exitValue Value to return after writing the usage message.
   \return Does not return.
*/
static void usage(char const * const name, bool fBrief, int exitValue)
{
    if (fBrief)
    {
        wcout << name << L": Try '" << name << L" -h' for more information." << endl;
    }
    else
    {
        wcout << L"Usage: " << name << endl
              << endl
              << L"Options:" << endl
              << L"  -h:\tDisplay detailed help information" << endl
              << L"  -i:\tInteractive use (for debugging purposes only)" << endl
              << L"  -m:\tRun marshal unit tests (debugging purposes only)" << endl
              << L"  -p:\tProvider interface (for internal use only)" << endl
              << L"  -t:\tProvide hooks for testrunner environmental setup" << endl
              << L"  -v:\tDisplay version information" << endl;
    }

    exit(exitValue);
} 


/*----------------------------------------------------------------------------*/
/**
   Output the version string
*/
static void show_version()
{
    wcout << L"Version: " << SCX_BUILDVERSION_MAJOR << L"." << SCX_BUILDVERSION_MINOR
          << L"." << SCX_BUILDVERSION_PATCH << L"-" << SCX_BUILDVERSION_BUILDNR
          << L" (" << SCX_BUILDVERSION_STATUS << L" - " << SCX_BUILDVERSION_DATE << "L)\n";

    return;
}

/*----------------------------------------------------------------------------*/
/**
   Perform low-level marshal tests for unit test purposes.
*/
void PerformMarshalTest()
{
    // Unmarshal the parameters from the caller (passed via standard input)

    wstring filename;
    wstring qid;
    vector<SCXRegexWithIndex> regexps;
    int matchedLinesCount;

    UnMarshal receive(cin);
    receive.Read(filename);
    receive.Read(qid);
    receive.Read(regexps);
    receive.Read(matchedLinesCount);

    // Now marshal the results - everything we got plus a vector
    //
    // Populate the vector with some random data

    vector<wstring> matchedLines;

    for (int i = 0; i < matchedLinesCount; i++)
    {
        wstringstream wss;
        wss << L"This is entry number " << i << L" in the vector";
        matchedLines.push_back(wss.str());
    }

    // Marshal the results

    int wasPartialRead = 65536;

    Marshal send(cout);
    send.Write(filename);
    send.Write(qid);
    send.Write(regexps);
    send.Write(matchedLinesCount);
    send.Write(wasPartialRead);
    send.Write(matchedLines);
    send.Flush();

    return;
}

/*----------------------------------------------------------------------------*/
/**
   Implementation for interactive interface to read log files.

   Parameters are prompted for interactively.

   Input parameters are (as required by LogFileReader::ReadLogFile):
     filename:  Filename to be read
     qid:       ID (from property)
     regexps:   Regular expressions to search for

   Output result (sent to stdout):
     matchedLines:  Resulting lines that match the regular expressions

   \return 0 (exit status for scxlogfilereader executable)
*/
int ReadLogFile_Interactive()
{
    wstring filename;
    wstring qid;
    vector<SCXRegexWithIndex> regexps;

    // Get the filename and qid
    wcout << L"Enter filename: ";
    getline(wcin, filename);
    if ( wcin.eof() ) return 0;
    wcout << L"  Filename: '" << filename << L"'" << endl;

    wcout << L"Enter QID: ";
    getline(wcin, qid);
    if ( wcin.eof() ) return 0;
    wcout << L"  QID: '" << qid << L"'" << endl;

    // Read the list of regular expressions
    int count = 0;

    while (true)
    {
        wstring expression;

        wcout << L"Enter regular expression #" << count + 1
              << L" (^D to end): ";
        getline(wcin, expression);
        if ( wcin.eof() ) break;

        try
        {
            SCXRegexWithIndex regind;
            regind.regex = new SCXRegex(expression);
            regind.index = ++count;
            regexps.push_back(regind);

            wcout << L"  Expression #" << count << L": '" << expression
                  << L"'" << endl;
        }
        catch (SCXInvalidRegexException& e)
        {
            wcout << L"  Regular expression '" << expression
                  << L"' invalid; will be ignored" << endl;
        }
    }

    wcout << endl;

    if (count == 0)
    {
        wcout << L"  No valid regular expressions entered - exiting" << endl;
        return 0;
    }

    wcout << L"  Processing with " << count << L" expressions" << endl << endl;

    // Finally, input is gathered - do the processing

    try
    {
        vector<wstring> matchedLines;

        s_pReader = new LogFileReader();
        bool bPartial = s_pReader->ReadLogFile(filename, qid, regexps,
                                               matchedLines);

        // Display the output interactively

        wcout << L"Partial flag: " << (bPartial ? L"True" : L"False") << endl;

        vector<wstring>::iterator it;
        for (it=matchedLines.begin(); it < matchedLines.end(); it++)
        {
            wcout << *it << endl;
        }
    }
    catch (SCXFilePathNotFoundException& e)
    {
        wcerr << L"scxlogfilereader - File '" << filename << "' not found: "
              << e.What() << endl;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
   Implementation for provider interface to read log files.

   Ultimately, this is what the provider calls to process reading of log files.

   Parameters come in from STDIN (marshalled by the log file provider), and the
   results are returned (via STDOUT, marshalled).

   Input parameters are (as required by LogFileReader::ReadLogFile):
     filename:  Filename to be read
     qid:       ID (from property)
     regexps:   Regular expressions to search for

   Output parameters are:
     matchedLines:  Resulting lines that match the regular expressions

   \return Resulting status (exit status for scxlogfilereader executable)
*/
int ReadLogFile_Provider()
{
    SCXHandle<LogFileReader> logFileReader(new LogFileReader());
    if (s_fTestMode)
    {
        ReadLogFile_TestSetup();
        logFileReader->SetPersistMedia(s_pmedia);
    }

    SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(L"scx.logfilereader.ReadLogFile");

    // Unmarshal the parameters from the caller (passed via standard input)

    wstring filename;
    wstring qid;
    vector<SCXRegexWithIndex> regexps;

    UnMarshal receive(cin);
    receive.Read(filename);
    receive.Read(qid);
    receive.Read(regexps);

    try
    {
        vector<wstring> matchedLines;
        bool bWasPartialRead = logFileReader->ReadLogFile(filename, qid, regexps,
                                                          matchedLines);

        // Marshal the results

        int wasPartialRead = bWasPartialRead;
        Marshal send(cout);
        send.Write(wasPartialRead);
        send.Write(matchedLines);
        send.Flush();
    }
    catch (SCXFilePathNotFoundException& e)
    {
        SCX_LOGWARNING(logH, StrAppend(L"scxlogfilereader - File not found: ", filename).append(e.What()));

        // Return a special exit code so we know that the log file wasn't found
        return ENOENT;
    }
    catch (SCXException &e)
    {
        SCX_LOGWARNING(logH, StrAppend(L"scxlogfilereader - Unexpected exception: ", e.What()));

        return EINTR;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
   Setup the environment for test purposes (for running under testrunner).

   This works in concert with the unit tests themselves, but some of the
   setup must be done in the context of scxlogfilereader itself.  This
   will perform that setup.

   Note that the logfileprovider will only pass the -t flag (which results
   in this routine being called) if running unit tests at the time.  Otherwise,
   this function is never called.
*/

void ReadLogFile_TestSetup()
{
    const std::wstring testlogfilename = L"./logfileproviderTest.log";
    const std::wstring testQID = L"TestQID";
    const std::wstring testQID2 = L"AnotherTestQID";

    s_pmedia = GetPersistMedia();
    SCXFilePersistMedia* m = dynamic_cast<SCXFilePersistMedia*> (s_pmedia.GetData());
    SCXASSERT(m != 0);
    m->SetBasePath(L"./");
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
