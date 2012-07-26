/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       This file contains the class SCXOSTypeInfo
    
    \date        2009-05-01 11:39:54

    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXOSNAME_H
#define SCXOSNAME_H

#include <string>
#include <sys/utsname.h>

#if defined(hpux)
#include <sys/pstat.h>
#endif

#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
       Class to retreive common, static information about the current OS.

       This class can be used to find out identity of the current operating system.
       Expect it to be extended with bit width and system uptime etc., and 
       OS configuration parameters that do not change with less than system boot. 
       
    */
    class SCXOSTypeInfo
    {
    public:
        SCXOSTypeInfo();
        virtual ~SCXOSTypeInfo();

        std::wstring    GetCaption() const;  
        /** Returns the OS Version, typically from uname for Unix, /etc/xxx-release info for Linux */
        std::wstring    GetOSVersion() const { return m_osVersion; }
        std::wstring    GetOSName(bool bCompatMode = false) const;
        /** Returns a short alias for the OS in question, e.g. "Solaris" or "SLED" or "RHEL" */
        std::wstring    GetOSAlias()   const { return m_osAlias;   } 

        std::wstring    GetOSFamilyString() const;
        std::wstring    GetArchitectureString() const;

        std::wstring    GetUnameArchitectureString() const;

    private:
        void Init();

        std::wstring m_osVersion;               //!< Cached value for osVersion
        std::wstring m_osName;                  //!< Cached value for osName
        std::wstring m_osAlias;                 //!< Cached value for osAlias
        bool m_unameIsValid;                    //!< Is m_unameInfo valid now?
        struct utsname m_unameInfo;             //!< Saves the output from uname()
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.

#if defined(linux)
        std::wstring m_linuxDistroCaption;      //!< The current platform and version
#endif
    };
    
}

#endif /* SCXOSNAME_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
