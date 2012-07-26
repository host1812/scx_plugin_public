/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     RunAs Provider implementation

    \date      07-11-29 12:00:00
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>

#include <scxproviderlib/scxprovidercapabilities.h>

#include "runasprovider.h"

#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxuser.h>

#include <sstream>
#include <algorithm>

using namespace SCXProviderLib;
using namespace SCXCoreLib;

namespace SCXCore {

   /*----------------------------------------------------------------------------*/
    /**
       Provide CMPI interface for this class

       The class implementation (concrete class) is RunAsProvider and the name of the
       provider in CIM registration terms is SCX_RunAsProvider.
    */
    SCXProviderDef(RunAsProvider, SCX_RunAsProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       \param[in] configurator RunAs provider configurator 

       The Singleton thread lock will be held during this call.
    */
    RunAsProvider::RunAsProvider(SCXCoreLib::SCXHandle<RunAsConfigurator> configurator
                                 /* = new RunAsConfigurator() */) : 
        BaseProvider(L"scx.core.providers.runasprovider"),
        m_Configurator(configurator)
    {
        SCX_LOGTRACE(m_log, L"RunAsProvider constructor");

        EnableProviderUnloading();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    RunAsProvider::~RunAsProvider()
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
    void RunAsProvider::DoInit()
    {
        SCX_LOGTRACE(m_log, L"RunAsProvider::DoInit");

        m_ProviderCapabilities.RegisterCimClass(eSCX_OperatingSystem,
                                                L"SCX_OperatingSystem");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_OperatingSystem, eExecuteCommandMethod,
                                                 L"ExecuteCommand");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_OperatingSystem, eExecuteShellCommandMethod,
                                                 L"ExecuteShellCommand");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_OperatingSystem, eExecuteScriptMethod,
                                                 L"ExecuteScript");
        ParseConfiguration();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void RunAsProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"RunAsProvider::DoCleanup");

        m_ProviderCapabilities.Clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Execute a command

        \param[in]     command          Command to execute
        \param[out]    resultOut        Result string from stdout
        \param[out]    resultErr        Result string from stderr
        \param[out]    returncode       Return code from command
        \param[in]     timeout          Accepted number of seconds to wait
        \param[in]     elevationtype    Elevation type 
        \returns       true if command succeeded, else false
        \throws SCXAccessViolationException If execution is prohibited by configuration
    */
    bool RunAsProvider::ExecuteCommand(const std::wstring &command, std::wstring &resultOut, std::wstring &resultErr, int& returncode,
                                       unsigned timeout, const std::wstring &elevationtype)
    {
        SCX_LOGTRACE(m_log, L"SCXRunAsProvider ExecuteCommand");

        if ( ! m_Configurator->GetAllowRoot())
        {
            SCXUser currentUser;
            if (currentUser.IsRoot())
            {
                throw SCXAccessViolationException(L"Configuration prohibits execution with user: root", SCXSRCLOCATION);
            }
        }

        std::istringstream processInput;
        std::ostringstream processOutput;
        std::ostringstream processError;
        
        // Construct the command by considering the elevation type. It simply returns the command
        // when elevation type is not empty or the current user is already privilege.
        // The elevated command will become a shell command by the design.        
        std::wstring elecommand = ConstructCommandWithElevation(command, elevationtype);


        try
        {
            returncode = SCXCoreLib::SCXProcess::Run(elecommand, processInput, processOutput, processError, timeout * 1000, m_Configurator->GetCWD(), m_Configurator->GetChRootPath());
            SCX_LOGHYSTERICAL(m_log, L"\"" + elecommand + L"\" returned " + StrFrom(returncode));
            resultOut = StrFromMultibyte(processOutput.str());
            SCX_LOGHYSTERICAL(m_log, L"stdout: " + resultOut);
            resultErr = StrFromMultibyte(processError.str());
            SCX_LOGHYSTERICAL(m_log, L"stderr: " + resultErr);
        }
        catch (SCXCoreLib::SCXException& e)
        {
            resultOut = L"";
            resultErr = e.What();
            returncode = -1;
        }

        return (returncode == 0);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Execute a command in the default shell.

        \param[in]     command     Command to execute
        \param[out]    resultOut        Result string from stdout
        \param[out]    resultErr        Result string from stderr
        \param[out]    returncode       Return code from command
        \param[in]     timeout          Accepted number of seconds to wait
        \param[in]     elevationtype    Elevation type
        \returns       true if command succeeded, else false
        \throws SCXAccessViolationException If execution is prohibited by configuration
    */
    bool RunAsProvider::ExecuteShellCommand(const std::wstring &command, std::wstring &resultOut, std::wstring &resultErr, int& returncode,
                                            unsigned timeout, const std::wstring &elevationtype)
    {
        SCX_LOGTRACE(m_log, L"SCXRunAsProvider ExecuteShellCommand");

        if ( ! m_Configurator->GetAllowRoot())
        {
            SCXUser currentUser;
            if (currentUser.IsRoot())
            {
                throw SCXAccessViolationException(L"Configuration prohibits execution with user: root", SCXSRCLOCATION);
            }
        }

        std::istringstream processInput;
        std::ostringstream processOutput;
        std::ostringstream processError;
       
        // Construct the shell command with the given command and elevation type.
        // Please be noted that the constructed shell command use the single quotes. Hence,
        // the current limitation is that the shell command fails if the given command has 
        // single quote. 
        std::wstring shellcommand = ConstructShellCommandWithElevation(command, elevationtype);

        try
        {
            returncode = SCXCoreLib::SCXProcess::Run(shellcommand, processInput, processOutput, processError, timeout * 1000, m_Configurator->GetCWD(), m_Configurator->GetChRootPath());
            SCX_LOGHYSTERICAL(m_log, L"\"" + shellcommand + L"\" returned " + StrFrom(returncode));
            resultOut = StrFromMultibyte(processOutput.str());
            SCX_LOGHYSTERICAL(m_log, L"stdout: " + resultOut);
            resultErr = StrFromMultibyte(processError.str());
            SCX_LOGHYSTERICAL(m_log, L"stderr: " + resultErr);
        }
        catch (SCXCoreLib::SCXException& e)
        {
            resultOut = L"";
            resultErr = e.What();
            returncode = -1;
        }

        return (returncode == 0);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Execute a script

        \param[in]     script           Script to execute
        \param[in]     arguments        Command line arguments to script
        \param[out]    resultOut        Result string from stdout
        \param[out]    resultErr        Result string from stderr
        \param[out]    returncode       Return code from command
        \param[in]     timeout          Accepted number of seconds to wait
        \param[in]     elevationtype    Elevation type

        \returns       true if script succeeded, else false
        \throws SCXAccessViolationException If execution is prohibited by configuration    */
    bool RunAsProvider::ExecuteScript(const std::wstring &script, const std::wstring &arguments, std::wstring &resultOut, std::wstring &resultErr, int& returncode,
                                      unsigned timeout, const std::wstring &elevationtype)
    {
        SCX_LOGTRACE(m_log, L"SCXRunAsProvider ExecuteScript");

        if ( ! m_Configurator->GetAllowRoot())
        {
            SCXUser currentUser;
            if (currentUser.IsRoot())
            {
                throw SCXAccessViolationException(L"Configuration prohibits execution with user: root", SCXSRCLOCATION);
            }
        }

        std::istringstream processInput;
        std::ostringstream processOutput;
        std::ostringstream processError;

        try
        {
            SCXFilePath scriptfile = SCXFile::CreateTempFile(script);
            SCXFileSystem::Attributes attribs = SCXFileSystem::GetAttributes(scriptfile);
            attribs.insert(SCXFileSystem::eUserExecute);
            SCXFile::SetAttributes(scriptfile, attribs);

            std::wstring command(scriptfile.Get());
            command.append(L" ").append(arguments);

            // Construct the command with the given elevation type.
            command = ConstructCommandWithElevation(command, elevationtype);

            returncode = SCXCoreLib::SCXProcess::Run(command, processInput, processOutput, processError, timeout * 1000, m_Configurator->GetCWD(), m_Configurator->GetChRootPath());
            SCXFile::Delete(scriptfile);

            SCX_LOGHYSTERICAL(m_log, L"\"" + command + L"\" returned " + StrFrom(returncode));
            resultOut = StrFromMultibyte(processOutput.str());
            SCX_LOGHYSTERICAL(m_log, L"stdout: " + resultOut);
            resultErr = StrFromMultibyte(processError.str());
            SCX_LOGHYSTERICAL(m_log, L"stderr: " + resultErr);
        }
        catch (SCXCoreLib::SCXException& e)
        {
            resultOut = L"";
            resultErr = e.What();
            returncode = -1;
        }

        return (returncode == 0);
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
    void RunAsProvider::DoInvokeMethod(
        const SCXCallContext& callContext,
        const std::wstring& methodname,
        const SCXArgs& args,
        SCXArgs& outargs,
        SCXProperty& result)
    {
        SCX_LOGTRACE(m_log, L"SCXRunAsProvider DoInvokeMethod");

        SupportedCimMethods cimmethod = static_cast<SupportedCimMethods>(m_ProviderCapabilities.GetCimMethodId(callContext.GetObjectPath(), methodname));
        
        if (cimmethod == eExecuteCommandMethod || cimmethod == eExecuteShellCommandMethod)
        {
            const SCXProperty* command = args.GetProperty(L"Command");
            const SCXProperty* timeout = args.GetProperty(L"timeout");
            const SCXProperty* elevationtype = args.GetProperty(L"ElevationType");
            int returncode = 0;

            if (command == NULL || timeout == NULL)
            {
                throw SCXInternalErrorException(L"missing arguments to ExecuteCommand method", SCXSRCLOCATION);
            }

            if (command->GetType() != SCXProperty::SCXStringType ||
                timeout->GetType() != SCXProperty::SCXUIntType)
            {
                throw SCXInternalErrorException(L"Wrong type of arguments to ExecuteCommand method", SCXSRCLOCATION);
            }

            std::wstring return_out, return_err;
            bool cmdok;

            std::wstring elevation = L"";
            if (elevationtype != NULL)
            {
                elevation = StrToLower(elevationtype->GetStrValue());

                if (elevation != L"sudo" && elevation != L"")
                {
                     throw SCXInternalErrorException(L"Wrong elevation type " + elevation, SCXSRCLOCATION);                     
                }
            }
            if (cimmethod == eExecuteCommandMethod)
            {
                 cmdok = ExecuteCommand(command->GetStrValue(), return_out, return_err, returncode, timeout->GetUIntValue(), elevation);
            }
            else
            {
                cmdok = ExecuteShellCommand(command->GetStrValue(), return_out, return_err, returncode, timeout->GetUIntValue(), elevation);
            }

            result.SetValue(cmdok);

            SCXProperty ret_prop(L"ReturnCode", returncode);
            outargs.AddProperty(ret_prop);
            SCXProperty stdout_prop(L"StdOut", return_out);
            outargs.AddProperty(stdout_prop);
            SCXProperty stderr_prop(L"StdErr", return_err);
            outargs.AddProperty(stderr_prop);
        }
        else if (cimmethod == eExecuteScriptMethod)
        {
            const SCXProperty* script = args.GetProperty(L"Script");
            const SCXProperty* arguments = args.GetProperty(L"Arguments");
            const SCXProperty* timeout = args.GetProperty(L"timeout");
            const SCXProperty* elevationtype = args.GetProperty(L"ElevationType");
 
            int returncode = 0;

            if (script == NULL)
            {
                throw SCXInternalErrorException(L"missing arguments to ExecuteScript method", SCXSRCLOCATION);
            }

            if (script->GetType() != SCXProperty::SCXStringType)
            {
                throw SCXInternalErrorException(L"Wrong type of arguments to ExecuteScript method", SCXSRCLOCATION);
            }

            if (arguments == NULL || timeout == NULL)
            {
                throw SCXInternalErrorException(L"missing arguments to ExecuteScript method (arguments, timeout)", SCXSRCLOCATION);
            }

            if (arguments->GetType() != SCXProperty::SCXStringType ||
                timeout->GetType() != SCXProperty::SCXUIntType)
            {
                throw SCXInternalErrorException(L"Wrong type of arguments to ExecuteScript method", SCXSRCLOCATION);
            }

            std::wstring elevation = L"";
            if (elevationtype != NULL)
            {
                elevation = StrToLower(elevationtype->GetStrValue());
                
                if (elevation != L"sudo" && elevation != L"")
                {
                     throw SCXInternalErrorException(L"Wrong elevation type " + elevation, SCXSRCLOCATION);
                }
            }

            std::wstring return_out, return_err;
            std::wstring strScript = script->GetStrValue();
            // before Peg 2.9, open wsman removed '\r' characters
            std::wstring::size_type pos_slash_r = strScript.find( '\r' );

            while ( std::wstring::npos != pos_slash_r ){
                strScript.erase( pos_slash_r, 1 );
                pos_slash_r = strScript.find( '\r' );
            }
            
            bool cmdok = ExecuteScript(strScript, arguments->GetStrValue(), return_out, return_err, returncode, timeout->GetUIntValue(), elevation);

            result.SetValue(cmdok);

            SCXProperty ret_prop(L"ReturnCode", returncode);
            outargs.AddProperty(ret_prop);
            SCXProperty stdout_prop(L"StdOut", return_out);
            outargs.AddProperty(stdout_prop);
            SCXProperty stderr_prop(L"StdErr", return_err);
            outargs.AddProperty(stderr_prop);
        }
        else
        {
            throw SCXInternalErrorException(StrAppend(L"Unhandled method name: ", methodname), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Parses the configuration file.

       All parse errors are logged. If configuration cannot be parsed, default
       configuration is used.

    */
    void RunAsProvider::ParseConfiguration()
    {
        m_Configurator->Parse();
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
