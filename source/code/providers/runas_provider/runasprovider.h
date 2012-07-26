/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     RunAs Provider

    \date      07-11-29 12:00:00
*/
/*----------------------------------------------------------------------------*/
#ifndef RUNASPROVIDER_H
#define RUNASPROVIDER_H

#include <string>

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxfilepath.h>
#include <scxsystemlib/scxsysteminfo.h>
#include <scxproviderlib/cmpibase.h>

#include "scxrunasconfigurator.h"

using namespace SCXCoreLib;

namespace SCXCore
{
    /*----------------------------------------------------------------------------*/
    /**
       RunAs provider

       Concrete instance of the CMPI BaseProvider implementing a method
       for executing commands

       A provider-specific thread lock will be held at each call to
       the Do* methods, so this implementation class does not need
       to worry about that.
    */
    class RunAsProvider : public SCXProviderLib::BaseProvider
    {
    public:
        /**
           Default constructor
           \param[in] configurator RunAs provider configurator 
           The Singleton thread lock will be held during this call.
        */
        RunAsProvider(SCXCoreLib::SCXHandle<RunAsConfigurator> configurator
                      = SCXCoreLib::SCXHandle<RunAsConfigurator>(new RunAsConfigurator()));
        ~RunAsProvider();

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_OperatingSystem
        };

        //! The CIM methods this provider supports
        enum SupportedCimMethods {
            eExecuteCommandMethod,
            eExecuteShellCommandMethod,
            eExecuteScriptMethod
        };

        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        virtual void DoCleanup();

        virtual void DoInvokeMethod(const SCXProviderLib::SCXCallContext& callContext,
                                    const std::wstring& methodname, const SCXProviderLib::SCXArgs& args,
                                    SCXProviderLib::SCXArgs& outargs, SCXProviderLib::SCXProperty& result);

    private:
        bool ExecuteCommand(const std::wstring &command, std::wstring &resultOut,
                            std::wstring &resultErr, int& returncode, unsigned timeout = 0, const std::wstring &elevationtype = L"");

        bool ExecuteShellCommand(const std::wstring &command, std::wstring &resultOut,
                                 std::wstring &resultErr, int& returncode, unsigned timeout = 0, const std::wstring &elevationtype = L"");

        bool ExecuteScript(const std::wstring &script, const std::wstring &arguments,
                           std::wstring &resultOut, std::wstring &resultErr,
                           int& returncode, unsigned timeout = 0, const std::wstring &elevationtype = L"");

        void ParseConfiguration();

        // Construct the command by considering the elevation type.
        // Noted that SystemInfo GetElevatedCommand function will return a shell command
        // when the elevation type is sudo ( it simply returns the command when the current user is already elevated).
        inline std::wstring ConstructCommandWithElevation(const std::wstring &command, const std::wstring &elevationtype) {
            SCXSystemLib::SystemInfo si;
            if (elevationtype == L"sudo")
            {
                return si.GetElevatedCommand(command);
            }

            return command;
        }

        // Construct a shell command for the given command and the elevation type.  
        inline std::wstring ConstructShellCommandWithElevation(const std::wstring &command, const std::wstring &elevationtype) {
            SCXSystemLib::SystemInfo si;

            std::wstring newCommand(si.GetShellCommand(command));

            // Only when current user is not priviledged and elevation type is sudo
            // the command need to be elevated.
            // Force a shell command so we get a shell (even if already elevated)
            if (elevationtype == L"sudo")
            {
                newCommand = si.GetElevatedCommand(newCommand);
            }

            return newCommand;
        }
        
        //! Configurator.
        SCXCoreLib::SCXHandle<RunAsConfigurator> m_Configurator;
    };
}

#endif /* RUNASPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
