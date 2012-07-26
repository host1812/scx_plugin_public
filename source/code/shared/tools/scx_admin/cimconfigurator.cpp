/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**
   \file

   \brief      scx cim configuration tool for SCX.  Configurations options in OpenPegasus.

   \date       8/27/2008

*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/stringaid.h>

#include <iostream>

#include "cimconfigurator.h"

using namespace SCXCoreLib;
using namespace std;


// Oh where, oh where, does cimconfig live?  Oh where, oh where can it be?

/**
   Define CIMCONFIG_TOOL (location of where scxcimconfig lives on the platform)
 */
#if !defined(macos)
#define CIMCONFIG_TOOL  L"/opt/microsoft/scx/bin/tools/scxcimconfig"
#else
#define CIMCONFIG_TOOL  L"/usr/libexec/microsoft/scx/bin/tools/scxcimconfig"
#endif


/*
  Total list of trace components supported by OpenPegasus v2.9.0
  (defined in file pegasus/src/Pegasus/Common/Tracer.cpp):

    Authentication
    Authorization
    CIMExportRequestDispatcher
    CIMOMHandle
    CMPIProvider
    CMPIProviderInterface
    Config
    ControlProvider
    CQL
    DiscardedData
    Dispatcher
    ExportClient
    Http
    IndicationFormatter
    IndicationGeneration
    IndicationHandler
    IndicationReceipt
    IndicationService
    L10N
    Listener
    LogMessages
    MessageQueueService
    ObjectResolution
    OsAbstraction
    ProviderAgent
    ProviderManager
    Repository
    Server
    Shutdown
    SSL
    StatisticalData
    Thread
    UserManager
    WQL
    WsmServer
    Xml
    XmlIO

  Trace levels are defined as follows:
    Level 1: Function entry/exit.
    Level 2: Basic logic flow trace messages, minimal data detail
    Level 3: Intra function logic flow and moderate data detail
    Level 4: High data detail


  Configuration settings of interest in Pegasus:
    traceComponents:  Comma-separated list of trace components available
    traceLevel:       Level of tracing (global for all enabled trace components)
    logLevel:         Level of events (INFORMATION, etc) to be logged to syslog


  In general, we don't mess with syslog, and we primarily just deal with tracing
  (and the Pegasus log file).
 */

/**
   Define Pegasus trace components for no logging
 */
#define COMPONENTS_DETAIL_NONE L""
/**
  Define Pegasus trace components for partial logging - Everything except:
    Authentication, Authorization, L10N, Thread, and XmlIO.
*/
#define COMPONENTS_DETAIL_PARTIAL L"CIMExportRequestDispatcher,CIMOMHandle,CMPIProvider,CMPIProviderInterface,Config,ControlProvider,CQL,DiscardedData,Dispatcher,ExportClient,Http,IndicationFormatter,IndicationGeneration,IndicationHandler,IndicationReceipt,IndicationService,Listener,LogMessages,MessageQueueService,ObjectResolution,OsAbstraction,ProviderAgent,ProviderManager,Repository,Server,Shutdown,SSL,StatisticalData,UserManager,WQL,WsmServer,Xml"

/**
   Define Pegasus trace components for all logging
*/
#define COMPONENTS_DETAIL_FULL L"ALL"

/**
   Define Pegasus trace level for no logging
 */
#define TRACE_DETAIL_NONE    L"1"
/**
   Define Pegasus trace level for partial logging
 */
#define TRACE_DETAIL_PARTIAL L"4"
/**
   Define Pegasus trace level for full logging
 */
#define TRACE_DETAIL_FULL    L"4"


namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Constructor for SCX_CimConfigurator class
    */
    SCX_CimConfigurator::SCX_CimConfigurator()
    {
    }


    /*----------------------------------------------------------------------------*/
    /**
       Destructor for SCX_CimConfigurator class
    */
    SCX_CimConfigurator::~SCX_CimConfigurator()
    {
    }


    /*----------------------------------------------------------------------------*/
    /**
       Performs log rotation for the CIM server

       \returns           TRUE if supported (and suceeded), FALSE if unsupported

       \throws            Exception if supported, but error occurred
    */
    bool SCX_CimConfigurator::LogRotate()
    {
        // OpenPegasus doesn't support log rotation at this time
        // (to the best of my knowledge) without server restart

        return false;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Prints the current state of the CIM server

       \param[out]  buf   Output of current log settings
       \returns           TRUE if supported (and suceeded), FALSE if unsupported

       \throws            Exception if supported, but error occurred
    */
    bool SCX_CimConfigurator::Print(std::wostringstream& buf) const
    {
        std::ostringstream processOutput;
        std::ostringstream processError;

        return PrintInternal(buf, processOutput, processError);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Resets the logging level for the CIM server

       \returns           TRUE if supported (and suceeded), FALSE if unsupported

       \throws            Exception if supported, but error occurred
    */
    bool SCX_CimConfigurator::Reset()
    {
        std::ostringstream processOutput;
        std::ostringstream processError;

        return ResetInternal(processOutput, processError);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Sets the current logging configuration for CIM server

       \param       level   Current logging level (0->None, 1->Some, 2->All)
       \returns             TRUE if supported (and suceeded), FALSE if unsupported

       \throws              Exception if supported, but error occurred
    */
    bool SCX_CimConfigurator::Set(LogLevelEnum level)
    {
        std::ostringstream processOutput;
        std::ostringstream processError;

        return SetInternal(level, processOutput, processError);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Prints the current state of the CIM server
       (Internal interface for testing purposes)

       \param[out]  buf        Output of current log settings
       \param[out]  mystdout   Stdout output from the commands
       \param[out]  mystderr   Stderr output from the commands
       \returns                TRUE if supported (and suceeded), FALSE if unsupported

       \throws                 Exception if supported, but error occurred
    */
    bool SCX_CimConfigurator::PrintInternal(
        std::wostringstream& buf,
        std::ostringstream &mystdout,
        std::ostringstream &mystderr) const
    {
        std::wstring traceLevelP, traceLevelC, traceComponentP, traceComponentC;
        std::ostringstream tlCOut, tlCErr, tlPOut, tlPErr, tcCOut, tcCErr, tcPOut, tcPErr;

        // Get current traceLevel
        Execute(CIMCONFIG_TOOL L" -g traceLevel", tlCOut, tlCErr, true);
        if ( !ParseGetConfig(traceLevelC, tlCOut, tlCErr) )
        {
            traceLevelC = L"<Not Available>";
        }

        // Get planned traceLevel
        Execute(CIMCONFIG_TOOL L" -g traceLevel -p", tlPOut, tlPErr);
        if ( !ParseGetConfig(traceLevelP, tlPOut, tlPErr) )
        {
            throw SCXInternalErrorException(L"Planned traceLevel not available from scxcimconfig", SCXSRCLOCATION);
        }

        // Get current traceComponents
        Execute(CIMCONFIG_TOOL L" -g traceComponents", tcCOut, tcCErr, true);
        if ( !ParseGetConfig(traceComponentC, tcCOut, tcCErr) )
        {
            traceComponentC = L"<Not Available>";
        }

        // Get planned traceComponents
        Execute(CIMCONFIG_TOOL L" -g traceComponents -p", tcPOut, tcPErr);
        if ( !ParseGetConfig(traceComponentP, tcPOut, tcPErr) )
        {
            throw SCXInternalErrorException(L"Planned traceComponents not available from scxcimconfig", SCXSRCLOCATION);
        }

        // Save all the standand outputs and standard errors for debug purposes

        mystdout << tlCOut.str() << tlPOut.str() << tcCOut.str() << tcPOut.str();
        mystderr << tlCErr.str() << tlPErr.str() << tcCErr.str() << tcPErr.str();

        /*
         * Format the buffer to send back to caller
         */

        buf << L"CIMSERVER: Current traceLevel=" << traceLevelC << endl;
        buf << L"CIMSERVER: Planned traceLevel=" << traceLevelP << endl;
        buf << endl;
        buf << L"CIMSERVER: Current traceComponents=\"" << traceComponentC << "\"" << endl;
        buf << L"CIMSERVER: Planned traceComponents=\"" << traceComponentP << "\"" << endl;

        return true;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Resets the logging level for the CIM server.
       (Internal interface for testing purposes)

       \param[out]  mystdout   Stdout output from the commands
       \param[out]  mystderr   Stderr output from the commands
       \returns                TRUE if supported (and suceeded), FALSE if unsupported

       \throws                 Exception if supported, but error occurred
    */
    bool SCX_CimConfigurator::ResetInternal(std::ostringstream &mystdout, std::ostringstream &mystderr)
    {
        return SetInternal(eLogLevel_Errors, mystdout, mystderr);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Sets the current logging configuration for CIM server
       (Internal interface for testing purposes)

       \param       level      Current logging level (0->None, 1->Some, 2->All)
       \param[out]  mystdout   Stdout output from the commands
       \param[out]  mystderr   Stderr output from the commands
       \returns                TRUE if supported (and suceeded), FALSE if unsupported

       \throws                 Exception if supported, but error occurred
    */
    bool SCX_CimConfigurator::SetInternal(
        LogLevelEnum level,
        std::ostringstream &mystdout,
        std::ostringstream &mystderr)
    {
        wstring traceComponents, traceLevel;

        switch (level) {
            case eLogLevel_Errors:
                traceComponents = COMPONENTS_DETAIL_NONE;
                traceLevel = TRACE_DETAIL_NONE;
                break;

            case eLogLevel_Intermediate:
                traceComponents = COMPONENTS_DETAIL_PARTIAL;
                traceLevel = TRACE_DETAIL_PARTIAL;
                break;

            case eLogLevel_Verbose:
                traceComponents = COMPONENTS_DETAIL_FULL;
                traceLevel = TRACE_DETAIL_FULL;
                break;

            default:
                throw SCXInvalidArgumentException(L"level", L"Invalid value", SCXSRCLOCATION);
        }

        wstring command = CIMCONFIG_TOOL, persist = L" -p";
        command += L" -s traceLevel=";
        command += traceLevel;

        Execute(command, mystdout, mystderr, true);
        Execute(command + persist, mystdout, mystderr);

        command = CIMCONFIG_TOOL;
        command += L" -s traceComponents=\"";
        command += traceComponents;
        command += L"\"";

        Execute(command, mystdout, mystderr, true);
        Execute(command + persist, mystdout, mystderr);

        return true;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Execute a command (generally using scxcimcli).  Internal helper method.

       \param       command    Command to execute
       \param[out]  mystdout   Standard output from the command
       \param[out]  mystderr   Standard error from the command
       \param       fErrorOK   Allow error if CIM server is not running

       \throws                 Exception if an error occurred
    */
    void SCX_CimConfigurator::Execute(
        const std::wstring &command,
        std::ostringstream &mystdout,
        std::ostringstream &mystderr,
        bool fErrorOK /* = false */) const
    {
        int returncode;
        std::istringstream processInput;
        std::ostringstream processOutput;
        std::ostringstream processError;

        try {
            returncode = SCXProcess::Run(command, processInput, processOutput, processError);

            if ((returncode != 0)
                && !(fErrorOK
                     && returncode == 1
                     && processOutput.str().rfind("CIM server is not running") != string::npos))
            {
                throw SCXAdminException(StrFromMultibyte(processError.str()), SCXSRCLOCATION);
            }

            mystdout << processOutput.str();
            mystderr << processError.str();
        }
        catch (SCXCoreLib::SCXException& e)
        {
            throw SCXAdminException(e.What(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Parse output from scxcimconfig command (to retrieve current or planned configuration value)

       \param[out]  value      Value to return from parsed output
       \param       myOutput   Standard output from the command
       \param       myError    Standard error from the command
       \returns                TRUE if value could be determined, FALSE if value could not be determined

       \throws                 Exception if an error occurred
    */
    bool SCX_CimConfigurator::ParseGetConfig(
        std::wstring &value,
        std::ostringstream &myOutput,
        std::ostringstream &myError) const
    {
        std::wstring outputAsStr = StrFromMultibyte( myOutput.str() );
        (void) myError;

        // If the server is down, we can't determine the value
        if (outputAsStr.rfind(L"CIM server is not running") != string::npos)
        {
            return false;
        }

        if (StrIsPrefix(outputAsStr, L"Current value: ") || StrIsPrefix(outputAsStr, L"Planned value: "))
        {
            value = StrTrimR( outputAsStr.erase(0,15) );
            if (value.compare(COMPONENTS_DETAIL_PARTIAL) == 0)
            {
                value = L"<Intermediate Logging>";
            }
        }
        else
        {
            std::wstring badValue = L"Invalid value: \"";
            badValue += outputAsStr;
            badValue += L"\"";

            throw SCXInvalidArgumentException(L"myOutput", badValue, SCXSRCLOCATION);
        }

        return true;
    }
}
