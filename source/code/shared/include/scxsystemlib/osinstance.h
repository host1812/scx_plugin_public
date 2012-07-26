/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       PAL representation of Operating System
    \date        08-03-04 14:36:00

*/
/*----------------------------------------------------------------------------*/
#ifndef OSINSTANCE_H
#define OSINSTANCE_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxtime.h>
#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/scxostypeinfo.h>
#include <string>

// Definitions for uname()
#include <sys/utsname.h>

#if defined(hpux)
#include <sys/pstat.h>
#endif


namespace SCXSystemLib
{
    using SCXCoreLib::SCXCalendarTime;

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents the common set of OS parameters.

       This class only implements the total instance and has no
       collection threrad.

       These are the type mappings used in the property methods.
       typedef unsigned short uint16;
       typedef unsigned int uint32;
       typedef scxulong uint64;
       typedef signed short sint16;
       typedef bool boolean;
       typedef SCXCalendarTime datetime;
    */
    class OSInstance : public EntityInstance
    {
        friend class ProcessEnumeration;

        static const wchar_t *moduleIdentifier;         //!< Shared module string

    public:
        OSInstance();
        virtual ~OSInstance();

        virtual void Update();
        virtual void CleanUp();

        virtual const std::wstring DumpString() const;

        /* Properties of CIM_OperatingSystem */
        bool GetOSType(unsigned short& ost) const;
        bool GetOtherTypeDescription(std::wstring& otd) const;
        bool GetVersion(std::wstring& ver) const;
        bool GetLastBootUpTime(SCXCalendarTime& lbut) const;
        bool GetLocalDateTime(SCXCalendarTime& ldt) const;
        bool GetCurrentTimeZone(signed short& ctz) const;
        bool GetNumberOfLicensedUsers(unsigned int& nolu) const;
        bool GetNumberOfUsers(unsigned int& numberOfUsers) const;
        bool GetMaxNumberOfProcesses(unsigned int& mp) const;
        bool GetMaxProcessMemorySize(scxulong& mpms) const;
        bool GetMaxProcessesPerUser(unsigned int& mppu) const;

        /* Properties of PG_OperatingSystem */
        bool GetSystemUpTime(scxulong& sut) const;

    private:
        SCXOSTypeInfo m_osInfo;                 //!< Static OS Information
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.

        struct utsname m_unameInfo;             //!< Saves the output from uname()
        bool m_unameIsValid;                    //!< Is m_unameInfo valid now?

#if defined(linux)
        void SetBootTime(void);
        void PrecomputeMaxProcesses(void);

        unsigned int m_MaxProcesses;            //!< Maximum number of process contexts.
        double m_upsec;                         //!< Uptime in seconds
        std::wstring m_platform;                //!< The current platform and version
#endif

        SCXCalendarTime m_system_boot;          //!< Time of system boot
        bool m_system_boot_isValid;             //!< Is m_system_boot valid now?
        SCXCalendarTime m_now;                  //!< Current time on system

#if defined(hpux)
        void SetBootTime(void);

        struct pst_static m_psts;               //!< Holds output from pstat_getstatic
        struct pst_dynamic m_pstd;              //!< Holds output from pstat_getdynamic
        bool m_psts_isValid;                    //!< Is m_psts valid now?
        bool m_pstd_isValid;                    //!< Is m_pstd valid now?
#endif

        /**
           A constant returned by the GetOSType call.
           This comes from the Pegasus code. Source of the original definition
           is unknown.
        */
        enum OSTYPE {
            /* Useless documentation to make doxygen happy */
            Unknown,            //!< Unknown
            Other,              //!< Other
            MACOS,              //!< MACOS
            ATTUNIX,            //!< ATTUNIX
            DGUX,               //!< DGUX
            DECNT,              //!< DECNT
            Digital_Unix,       //!< Digital_Unix
            OpenVMS,            //!< OpenVMS
            HP_UX,              //!< UX
            AIX,                //!< AIX
            MVS,                //!< MVS
            OS400,              //!< OS400
            OS2,                //!< OS2
            JavaVM,             //!< JavaVM
            MSDOS,              //!< MSDOS
            WIN3x,              //!< WIN3x
            WIN95,              //!< WIN95
            WIN98,              //!< WIN98
            WINNT,              //!< WINNT
            WINCE,              //!< WINCE
            NCR3000,            //!< NCR3000
            NetWare,            //!< NetWare
            OSF,                //!< OSF
            DCOS,               //!< DCOS
            Reliant_UNIX,       //!< Reliant UNIX
            SCO_UnixWare,       //!< SCO UnixWare
            SCO_OpenServer,     //!< SCO OpenServer
            Sequent,            //!< Sequent
            IRIX,               //!< IRIX
            Solaris,            //!< Solaris
            SunOS,              //!< SunOS
            U6000,              //!< U6000
            ASERIES,            //!< ASERIES
            TandemNSK,          //!< TandemNSK
            TandemNT,           //!< TandemNT
            BS2000,             //!< BS2000
            LINUX,              //!< LINUX
            Lynx,               //!< Lynx
            XENIX,              //!< XENIX
            VM_ESA,             //!< VM/ESA
            Interactive_UNIX,   //!< Interactive UNIX
            BSDUNIX,            //!< BSDUNIX
            FreeBSD,            //!< FreeBSD
            NetBSD,             //!< NetBSD
            GNU_Hurd,           //!< Hurd
            OS9,                //!< OS9
            MACH_Kernel,        //!< Mach Kernel
            Inferno,            //!< Inferno
            QNX,                //!< QNX
            EPOC,               //!< EPOC
            IxWorks,            //!< IxWorks
            VxWorks,            //!< VxWorks
            MiNT,               //!< MiNT
            BeOS,               //!< BeOS
            HP_MPE,             //!< HP MPE
            NextStep,           //!< NextStep
            PalmPilot,          //!< PalmPilot
            Rhapsody,           //!< Rhapsody
            Windows_2000,       //!< Windows 2000
            Dedicated,          //!< Dedicated
            OS_390,             //!< OS 390
            VSE,                //!< VSE
            TPF,                //!< TPF
            Windows_Me,         //!< Me
            Open_UNIX,          //!< Open UNIX
            OpenBDS,            //!< OpenBDS
            NotApplicable,      //!< NotApplicable
            Windows_XP,         //!< Windows XP
            zOS,                //!< zOS
            Windows_2003,       //!< Windows 2003
            Windows_2003_64     //!< Windows_2003_64
        };

    };
}
#endif /* OSINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
