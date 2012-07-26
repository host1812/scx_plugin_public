/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Implementation for a file scxlog backend.

    \date        2008-07-23 15:15:04

*/
/*----------------------------------------------------------------------------*/

#if !defined(WIN32)
#include "buildversion.h"
#endif
#include "scxlogfilebackend.h"
#include "scxlogitem.h"
#include <stdexcept>
#include <scxcorelib/stringaid.h>
#if defined(SCX_UNIX)
#include <scxcorelib/scxuser.h>
#endif

namespace SCXCoreLib
{
    SCXProcessId SCXLogFileBackend::s_processID = SCXProcess::GetCurrentProcessID();

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.
    */
    SCXLogFileBackend::SCXLogFileBackend() :
        SCXLogBackend(),
        m_FilePath(),
        m_LogFileRunningNumber(1),
        m_procStartTimestamp(SCXCalendarTime::CurrentUTC())
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Constructor with filepath.
        \param[in] filePath Path to log file.
    */
    SCXLogFileBackend::SCXLogFileBackend(const SCXFilePath& filePath) :
        SCXLogBackend(),
        m_FilePath(filePath),
        m_FileStream(0),
        m_LogFileRunningNumber(1),
        m_procStartTimestamp(SCXCalendarTime::CurrentUTC())
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor.
    */
    SCXLogFileBackend::~SCXLogFileBackend()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add name of current user to file path.
    */
    void SCXLogFileBackend::AddUserNameToFilePath()
    {
#if defined(SCX_UNIX)
        SCXUser user;
        if (!user.IsRoot())
        {
            m_FilePath.AppendDirectory(user.GetName());
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        An SCXLogItem is submitted for output to this specific backend.
        When this method is called from LogThisItem, we are in the scope of a
        thread lock so there should be no need for one here.

        \param[in] item Log item to be submitted for output.
    */
    void SCXLogFileBackend::DoLogItem(const SCXLogItem& item)
    {
        if (m_FileStream == 0 || ! m_FileStream->is_open())
        {
            try {
                m_FileStream = SCXFile::OpenWFstream(m_FilePath, std::ios::out|std::ios::app);
                // Write a log file header
                std::wstringstream continuationLogMsg;
                if (m_LogFileRunningNumber > 1) 
                {
                    continuationLogMsg << L"* Log file number: " << StrFrom(m_LogFileRunningNumber) << std::endl;
                }
                (*m_FileStream) << L"*" << std::endl
                                << L"* Microsoft System Center Cross Platform Extensions (SCX)" << std::endl
#if !defined(WIN32)
                                << L"* Build number: " << SCX_BUILDVERSION_MAJOR << L"." << SCX_BUILDVERSION_MINOR << L"." << SCX_BUILDVERSION_PATCH << L"-" << SCX_BUILDVERSION_BUILDNR << L" " << SCX_BUILDVERSION_STATUS << std::endl
#endif
                                << L"* Process id: " << StrFrom(SCXProcess::GetCurrentProcessID()) << std::endl
                                << L"* Process started: " << m_procStartTimestamp.ToExtendedISO8601() << std::endl
                                << continuationLogMsg.str() 
                                << L"*" << std::endl
                                << L"* Log format: <date> <severity>     [<code module>:<process id>:<thread id>] <message>" << std::endl
                                << L"*" << std::endl;
            }
            catch (const SCXFilePathNotFoundException&)
            {
                // We get this if we don't have permissions to create or write to this file.
                // There's not much we can do about this.
                return;
            }
            catch (const SCXUnauthorizedFileSystemAccessException&)
            {
                // We get this if we don't have permissions to create or write to this file.
                // There's not much we can do about this.
                return;
            }
        }

        std::wstring msg = Format(item);
        (*m_FileStream) << msg << std::endl;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Handle log rotations that have occurred
     */
    void SCXLogFileBackend::HandleLogRotate()
    {
        m_LogFileRunningNumber++;
        m_FileStream->close();
        m_FileStream = 0;
        SCXLogItem item(L"scx.core.providers", eInfo, L"Log rotation complete", 
                        SCXSRCLOCATION, SCXThread::GetCurrentThreadID());
        DoLogItem(item);
    }

    /*----------------------------------------------------------------------------*/
    /**
        The backend can be configured using key - value pairs.

        \param[in] key Name of property to set.
        \param[in] value Value of property to set.
    */
    void SCXLogFileBackend::SetProperty(const std::wstring& key, const std::wstring& value)
    {
        if (L"PATH" == key)
        {
            m_FilePath.Set(value);
            AddUserNameToFilePath();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        This implementation is initialized once the file path is not empty.

        \returns true if m_FilePath is not empty
    */
    bool SCXLogFileBackend::IsInitialized() const
    {
        return m_FilePath.Get().length() != 0;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the path to the log file.

        \returns Current path to log file.
    */
    const SCXFilePath& SCXLogFileBackend::GetFilePath() const
    {
        return m_FilePath;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Log format method.

        \param[in]  item An SCXLogItem to format.
        \returns    A formatted message:
                    "<time> <SEVERITY> [<module>:<processid>:<threadid>] <message>"
    */
    const std::wstring SCXLogFileBackend::Format(const SCXLogItem& item) const
    {
        static const wchar_t* severityStrings[] = {
            L"NotSet    ",
            L"Hysterical",
            L"Trace     ",
            L"Info      ",
            L"Warning   ",
            L"Error     "
        };

        std::wstringstream ss;

        ss << item.GetTimestamp().ToExtendedISO8601() << L" ";

        if (item.GetSeverity() > eError)
        {
            ss << L"Unknown " << item.GetSeverity();
        }
        else
        {
            ss << severityStrings[item.GetSeverity()];
        }

        ss << L" [" << item.GetModule() << L":" << s_processID << L":" << item.GetThreadId() << L"] " << item.GetMessage();

        return ss.str();
    }
} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
