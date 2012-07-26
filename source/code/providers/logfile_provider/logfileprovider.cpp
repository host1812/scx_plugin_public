/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Main implementation file for Log File Provider
 
    \date      2008-0-08 09:35:36
 

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxmarshal.h>
#include <scxcorelib/scxprocess.h>
#include <scxsystemlib/scxsysteminfo.h>
#include <scxproviderlib/scxprovidercapabilities.h>

#include <errno.h>
#include <stdlib.h>

#include "logfileprovider.h"
#include "logfileutils.h"
#include "../meta_provider/startuplog.h"

using namespace SCXProviderLib;
using namespace SCXCoreLib;
using namespace std;

namespace SCXCore {
    /*----------------------------------------------------------------------------*/
    /**
        Provide CMPI interface for this class
       
        The class implementation (concrete class) is LogFileProvider and the name of the 
        provider in CIM registration terms is SCX_LogFileProvider.
    */
    SCXProviderDef(LogFileProvider, SCX_LogFileProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor
       
       The Singleton thread lock will be held during this call.
       
    */
    LogFileProvider::LogFileProvider() :
        BaseProvider(L"scx.core.providers.logfileprovider"),
        m_pLogFileReader(new LogFileReader())
    {
        InitializeObject();
    }

    LogFileProvider::LogFileProvider(SCXCoreLib::SCXHandle<LogFileReader> pLogFileReader)
        : BaseProvider(L"scx.core.providers.logfileprovider"),
          m_pLogFileReader(pLogFileReader)
    {
        InitializeObject();
    }

    void LogFileProvider::InitializeObject()
    {
        LogStartup();
        SCX_LOGTRACE(m_log, L"LogFileProvider constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    LogFileProvider::~LogFileProvider()
    {
        // Do not log here since when this destructor is called the objects neccesary for logging might no longer be alive
    }

    /*----------------------------------------------------------------------------*/
    /**
       Registration of supported capabilities

       Callback from the BaseProvider in which the provider registers the supported
       classes and methods. The registrations should match the contents of the 
       MOF files exactly.

    */
    void LogFileProvider::DoInit()
    {
        SCX_LOGTRACE(m_log, L"LogFileProvider::DoInit()");

        m_ProviderCapabilities.RegisterCimClass(eSCX_LogFile, L"SCX_LogFile");
        m_ProviderCapabilities.RegisterCimClass(eSCX_LogFileRecord, L"SCX_LogFileRecord");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_LogFile, eGetMatchedRowsMethod, L"GetMatchedRows");
    }
    

    /*----------------------------------------------------------------------------*/
    /**
        Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void LogFileProvider::DoCleanup() 
    {
        SCX_LOGTRACE(m_log, L"LogFileProvider::DoCleanup");

        m_ProviderCapabilities.Clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instance names

       \param[in]   callContext Context details of this request
       \param[out]  instances   Collection of instances with key properties

    */
    void LogFileProvider::DoEnumInstanceNames(
        const SCXCallContext& callContext,
        SCXInstanceCollection& /* instances */)
    {
        SCX_LOGTRACE(m_log, L"LogFileProvider DoEnumInstanceNames");

        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));
        
        if (cimtype != eSCX_LogFile)
        {
            throw SCXNotSupportedException(L"LogFileProvider Enumerate not for LogFile class", SCXSRCLOCATION);
        }

        throw SCXNotSupportedException(L"LogFileProvider DoEnumInstanceNames not implemented", SCXSRCLOCATION);
    }
    
    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instances

       \param[in]     callContext                 Details of the client request
       \param[out]    instances                   Collection of instances

       \throws        SCXInternalErrorException   If instances in list are not CPUInstance

    */
    void LogFileProvider::DoEnumInstances(const SCXCallContext& callContext, 
                                          SCXInstanceCollection& instances)  
    { 
        SCX_LOGTRACE(m_log, L"LogFileProvider DoEnumInstances");

        DoEnumInstanceNames(callContext, instances);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Execute Query

       \param[in]     callContext                 Details of the client request
       \param[out]    instances                   Collection of instances
       \param[in]     query                       The WQL/CQL query to execute
       \param[in]     language                    Type of query (CQL/WQL)

       \throws        SCXNotSupportedException    If unhandled class

    */
    void LogFileProvider::DoExecQuery(
        const SCXProviderLib::SCXCallContext& callContext,
        SCXInstanceCollection& /*instances*/,
        std::wstring query,
        std::wstring language)
    {
        SCX_LOGTRACE(m_log, wstring(L"LogFileProvider DoExecQuery - ").append(query).append(L" - ").append(language));

        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));
        
        if (cimtype != eSCX_LogFileRecord)
        {
            throw SCXNotSupportedException(L"LogFileProvider Query not for LogFileRecord class", SCXSRCLOCATION);
        }

        throw SCXNotSupportedException(L"LogFileProvider DoEnumInstanceNames not implemented", SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Invoke a method on an instance

        \param[in]     callContext Keys indicating instance to execute method on
        \param[in]     methodname  Name of method called
        \param[in]     args        Arguments provided for method call
        \param[out]    outargs     Output arguments
        \param[out]    result      Result value
    */
    void LogFileProvider::DoInvokeMethod(
        const SCXCallContext& callContext,
        const std::wstring& methodname,
        const SCXArgs& args,
        SCXArgs& outargs,
        SCXProperty& result)
    {
        SCX_LOGTRACE(m_log, L"SCXLogFileProvider DoInvokeMethod");

        // Check that call is made on correct class
        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        if (cimtype != eSCX_LogFile)
        {
            throw SCXNotSupportedException(L"LogFileProvider method invoke not for LogFile class", SCXSRCLOCATION);
        }
        
        // Check that call is made to correct method
        SupportedCimMethods cimmethod = static_cast<SupportedCimMethods>(m_ProviderCapabilities.GetCimMethodId(callContext.GetObjectPath(), methodname));
        if (cimmethod == eGetMatchedRowsMethod)
        {
            // Get arguments, and check for correct type
            const SCXProperty* filename_p = args.GetProperty(L"filename");
            const SCXProperty* regexps_p = args.GetProperty(L"regexps");
            const SCXProperty* qid_p = args.GetProperty(L"qid");
            // Note: elevationType is an optional parameter ...
            const SCXProperty* elevationType_p = args.GetProperty(L"elevationType");

            if (filename_p == NULL || regexps_p == NULL || qid_p == NULL)
            {
                throw SCXInternalErrorException(L"missing argument(s) to GetMatchedRows method", SCXSRCLOCATION);
            }
            
            if (filename_p->GetType() != SCXProperty::SCXStringType || 
                regexps_p->GetType() != SCXProperty::SCXArrayType || 
                qid_p->GetType() != SCXProperty::SCXStringType)
            {
                throw SCXInternalErrorException(L"Wrong type of arguments to GetMatchedRows method", SCXSRCLOCATION);
            }
            
            std::wstring filename = filename_p->GetStrValue();
            const std::vector<SCXProperty>& regexps_pv = regexps_p->GetVectorValue();
            std::wstring qid = qid_p->GetStrValue();

            // Support for sudo elevation
            bool fPerformElevation = false;
            if (NULL != elevationType_p)
            {
                if (elevationType_p->GetType() != SCXProperty::SCXStringType)
                {
                    throw SCXInternalErrorException(L"Wrong type of arguments to GetMatchedRows method (argument elevationType)", SCXSRCLOCATION);
                }

                std::wstring elevationType = StrToLower(elevationType_p->GetStrValue());

                if (elevationType != L"sudo" && elevationType != L"")
                {
                    throw SCXInvalidArgumentException(L"elevationType", L"Wrong elevation type " + elevationType, SCXSRCLOCATION);                     
                }

                if (L"sudo" == elevationType)
                {
                    fPerformElevation = true;
                }
            }

            SCX_LOGTRACE(m_log, StrAppend(L"SCXLogFileProvider DoInvokeMethod - filename = ", filename));
            SCX_LOGTRACE(m_log, StrAppend(L"SCXLogFileProvider DoInvokeMethod - qid = ", qid));
            SCX_LOGTRACE(m_log, StrAppend(L"SCXLogFileProvider DoInvokeMethod - regexp count = ", regexps_pv.size()));
            SCX_LOGTRACE(m_log, StrAppend(L"SCXLogFileProvider DoInvokeMethod - elevate = ", fPerformElevation));
            
            // Extract and parse the regular expressions
            std::vector<SCXRegexWithIndex> regexps;
            std::wstring invalid_regex(L"");
            
            for (size_t i=0; i<regexps_pv.size(); i++)
            {
                if (regexps_pv[i].GetType() != SCXProperty::SCXStringType)
                {
                    throw SCXInternalErrorException(L"Wrong type of members in regsxps array argument to GetMatchedRows method", SCXSRCLOCATION);
                }
                
                std::wstring regexp = regexps_pv[i].GetStrValue();
                
                SCX_LOGTRACE(m_log, StrAppend(L"SCXLogFileProvider DoInvokeMethod - regexp = ", regexp));
                
                try
                {
                    SCXRegexWithIndex regind;
                    regind.regex = new SCXRegex(regexp);
                    regind.index = i;
                    regexps.push_back(regind);
                }
                catch (SCXInvalidRegexException& e)
                {
                    SCX_LOGWARNING(m_log, StrAppend(L"SCXLogFileProvider DoInvokeMethod - invalid regexp : ", regexp));
                    invalid_regex = StrAppend(StrAppend(invalid_regex, invalid_regex.length()>0?L" ":L""), i);
                }
            }

            std::vector<SCXProperty> ans;
            // If any regular expressions with invalid syntax, add special row to result
            if (invalid_regex.length() > 0)
            {
                SCXProperty rowprop(L"row", StrAppend(L"InvalidRegexp;", invalid_regex));
                ans.push_back(rowprop);
            }

            try
            {
                // Call helper function to get the data
                std::vector<std::wstring> matchedLines;
                bool bWasPartialRead = InvokeLogFileReader(
                    filename, qid, regexps, fPerformElevation, matchedLines);

                // Add each match to the result property set
                for (std::vector<std::wstring>::iterator it = matchedLines.begin();
                     it != matchedLines.end();
                     it++)
                {
                    SCXProperty rowprop(L"row", *it);
                    ans.push_back(rowprop);
                }

                // Set "MoreRowsAvailable" if we terminated early
                if (bWasPartialRead)
                {
                    SCXProperty rowprop(L"row", L"MoreRowsAvailable;true");
                    ans.insert(ans.begin(), rowprop);
                }
                
                SCXProperty rowsprop(L"rows", ans);
                outargs.AddProperty(rowsprop);
            }
            catch (SCXFilePathNotFoundException& e)
            {
                SCX_LOGWARNING(m_log, StrAppend(L"LogFileProvider DoInvokeMethod - File not found: ", filename).append(e.What()));
            }
            result.SetValue((unsigned int)ans.size());
        }
        else
        {
            throw SCXNotSupportedException(StrAppend(L"Unhandled method name: ", methodname), SCXSRCLOCATION);
        }
    }

    /**
        Dump object as string (for logging).
    
        \returns       The object represented as a string suitable for logging.
    */
    const std::wstring LogFileProvider::DumpString() const
    {
        return L"LogFileProvider";
    }

    /*----------------------------------------------------------------------------*/
    /**
        Invoke the logfileread CLI (command line) program, with elevation if needed

        \param[in]     filename      Filename to scan for matches
        \param[in]     qid           QID used for state file handling
        \param[in]     regexps       List of regular expressions to look for
        \param[out]    matchedLines  Resulting matched lines, if any, from log file

        \returns       Boolean flag to indicate if partial matches were returned
    */
    bool LogFileProvider::InvokeLogFileReader(
        const std::wstring& filename,
        const std::wstring& qid,
        const std::vector<SCXRegexWithIndex>& regexps,
        bool fPerformElevation,
        std::vector<std::wstring>& matchedLines)
    {
        SCX_LOGTRACE(m_log, L"SCXLogFileProvider InvokeLogFileReader");

        // Process of log file was called by something like:
        //
        // bPartial = m_pLFR->ReadLogFile(filename, qid, regexps, matchedLines);
        //
        // Marshal our data to send along to the subprocess
        // (Note that matchedLines is returned, along with partial flag)

        std::stringstream processInput;
        std::stringstream processOutput;
        std::stringstream processError;

        SCX_LOGTRACE(m_log, L"SCXLogFileProvider InvokeLogFileReader - Marshaling");

        Marshal send(processInput);
        send.Write(filename);
        send.Write(qid);
        send.Write(regexps);
        send.Flush();

        // Test to see if we're running under testrunner.  This makes it easy
        // to know where to launch our test program, allowing unit tests to
        // test all the way through to the CLI.

        wstring programName;
        char *testrunFlag = getenv("SCX_TESTRUN_ACTIVE");
        if (NULL != testrunFlag)
        {
            programName = L"scxlogfilereader -t -p";
        }
        else
        {
            programName = L"/opt/microsoft/scx/bin/scxlogfilereader -p";
        }

        // Elevate the command if that's called for
        SCXSystemLib::SystemInfo si;

        if (fPerformElevation)
        {
            programName = si.GetElevatedCommand(programName);
        }

        // Call the log file reader (CLI) program

        SCX_LOGTRACE(m_log,
                     StrAppend(L"SCXLogFileProvider InvokeLogFileReader - Running ",
                               programName));

        try
        {
            int returnCode = SCXProcess::Run(
                programName,
                processInput, processOutput, processError);

            SCX_LOGTRACE(m_log,
                         StrAppend(L"SCXLogFileProvider InvokeLogFileReader - Result ", returnCode));

            switch (returnCode)
            {
                case 0:
                    // Normal exit code
                    break;
                case ENOENT:
                    // Log file didn't exist - scxlogfilereader logged message about it
                    // Nothing to unmarshal at this point ...
                    return false;
                default:
                    wstringstream errorMsg;
                    errorMsg << L"Unexpected return code running '"
                             << programName
                             << L"': "
                             << returnCode;

                    throw SCXInternalErrorException(errorMsg.str(), SCXSRCLOCATION);
            }
        }
        catch (SCXCoreLib::SCXException& e)
        {
            SCX_LOGWARNING(m_log, StrAppend(L"LogFileProvider InvokeLogFileReader - Exception: ", e.What()));
            throw;
        }

        // Unmarshall matchedLines and partialRead flag
        //
        // Note that we can't marshal/unmarshal a bool, so we treat as int

        int wasPartialRead;

        SCX_LOGTRACE(m_log, L"SCXLogFileProvider InvokeLogFileReader - UnMarshaling");

        UnMarshal receive(processOutput);
        receive.Read(wasPartialRead);
        receive.Read(matchedLines);

        SCX_LOGTRACE(m_log, StrAppend(L"SCXLogFileProvider InvokeLogFileReader - Returning: ", (0 != wasPartialRead)));

        return (0 != wasPartialRead);
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
