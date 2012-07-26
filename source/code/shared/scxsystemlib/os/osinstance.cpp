/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       PAL representation of the operating system

   \date        08-03-04 15:22:00

*/
/*----------------------------------------------------------------------------*/

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

// For stat()
#include <sys/stat.h>

// For getutxent
#include <utmpx.h>

#if defined(linux) || defined(sun)
// Regexp includes
#include <regex.h>
// For getrlimit
#include <sys/resource.h>
// Dirent includes
#include <dirent.h>
#endif

#if defined(hpux)
// For gettune()
#include <sys/dyntune.h>
#include <sys/time.h>
#endif

#if defined(sun)
//#include <sys/swap.h>
//#include <sys/sysinfo.h>
#endif

#include <fstream>
#include <sstream>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>

#include <scxsystemlib/osenumeration.h>
#include <scxsystemlib/osinstance.h>

#include <limits.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{
    const wchar_t *OSInstance::moduleIdentifier = L"scx.core.common.pal.system.os.osinstance";

    /*----------------------------------------------------------------------------*/
    /**
       Constructor
    */
    OSInstance::OSInstance() :
        EntityInstance(true)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGTRACE(m_log, L"OSInstance constructor");

        // Do various initiation that can be done once and for all
#if defined(linux)
        PrecomputeMaxProcesses();
#elif defined(sun)
        /* Nothing yet */
#elif defined(hpux)
        /* Get information the system static variables (guaranteed constant until reboot) */
        m_psts_isValid = true;
        if (pstat_getstatic(&m_psts, sizeof(m_psts), 1, 0) < 0) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not do pstat_getstatic(). errno = ", errno));
            m_psts_isValid = false;
        }

        // Compute the boot time once and for all
        SetBootTime();
#elif defined(aix)
        /* Nothing yet */
#else
        #error "Not implemented for this platform."
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    OSInstance::~OSInstance()
    {
        SCX_LOGTRACE(m_log, L"OSInstance destructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Updates instance with latest data in preparation for read of individual 
       properties.
    */
    void OSInstance::Update()
    {
        SCX_LOGTRACE(m_log, L"OSInstance Update()");

        // Get current time
        m_now = SCXCalendarTime::CurrentLocal();

#if defined(hpux)
        m_unameIsValid = !((uname(&m_unameInfo) < 0) && (errno != EOVERFLOW));
        // Meaning: Ok if no error, or if errno is EOVERFLOW
#else
        m_unameIsValid = !(uname(&m_unameInfo) < 0);
#endif
        if (!m_unameIsValid) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not do uname(). errno = ", errno));
        }

#if defined(linux)

        SetBootTime();

#elif defined(hpux)

        /* Get information about the system dynamic variables */
        m_pstd_isValid = true;
        if (pstat_getdynamic(&m_pstd, sizeof(m_pstd), 1, 0) != 1) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not do pstat_getdynamic(). errno = ",errno));
            m_pstd_isValid = false;
        }

#elif defined (aix) || defined(macos) || defined(sun)
        // Nothing extra for these platforms
#else
        #error Platform not supported
#endif
    }


    /*----------------------------------------------------------------------------*/
    /**
       Clean up the instance. Closes the thread.
    */
    void OSInstance::CleanUp()
    {
        SCX_LOGTRACE(m_log, L"OSInstance CleanUp()");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Dump object as string (for logging).

       \returns     The object represented as a string suitable for logging.
    */
    const std::wstring OSInstance::DumpString() const
    {
        std::wstringstream ss;
        ss << L"OSInstance";
        return ss.str();
    }

    /*----------------------------------------------------------------------------*/
    /* Platform specific utility functions */

#if defined(linux)

    /**
       Sets the time related member variables.

       Information is read from the file /proc/uptime that contains the
       number of seconds since the system was last rebooted.

       This code is duplicated from processinstance.cpp.
    */
    void OSInstance::SetBootTime(void)
    {
        m_system_boot_isValid = false;
        //m_system_boot.IsInitialized();
        // First read seconds since boot
        FILE *f = fopen("/proc/uptime", "r");
        if (!f) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not open /proc/uptime. errno = ", errno));
        }
        int s = fscanf(f, "%lf", &m_upsec);
        fclose(f);
        if (s != 1) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not read /proc/uptime. errno = ", errno));
        }

        SCXAmountOfTime delta;
        delta.SetSeconds(m_upsec);
        m_system_boot = m_now - delta;
        m_system_boot_isValid = true;
    }

    /**
       Computes the kernel-configured maximum number
       of processes.

       Since this is not likely to change until reboot we
       compute this number once and for all.
    */
    void OSInstance::PrecomputeMaxProcesses(void)
    {
        // VERY inspired by the Pegasus code. 

        //-- prior to 2.4.* kernels, this will not work.  also, this is
        //   technically the maximum number of threads allowed; since
        //   linux has no notion of kernel-level threads, this is the
        //   same as the total number of processes allowed.  should
        //   this change, the algorithm will need to change.
        const char proc_file[] = "/proc/sys/kernel/threads-max";
        const size_t MAXPATHLEN = 80;
        char buffer[MAXPATHLEN];
        
        m_MaxProcesses = 0;
        FILE* vf = fopen(proc_file, "r");
        if (vf)
        {
            if (fgets(buffer, MAXPATHLEN, vf) != NULL)
                sscanf(buffer, "%u", &m_MaxProcesses);
            fclose(vf);
        }

    }

#endif /* linux */

#if defined(hpux)
    /**
     * Determines system boot up time.
     *
     * Call this once only, after pstat_getstatic was called.
     */
    void OSInstance::SetBootTime(void)
    {
        if ((m_system_boot_isValid = m_psts_isValid)) {
            m_system_boot = SCXCalendarTime::FromPosixTime(m_psts.boot_time);
            m_system_boot.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
        }
    }

#endif

    /*====================================================================================*/
    /* Properties of CIM_OperatingSystem                                                  */
    /*====================================================================================*/

    /**
       Gets the OSType
       \param[out]  ost
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       A integer indicating the type of OperatingSystem.
    */
    bool OSInstance::GetOSType(unsigned short& ost) const
    {
#if defined(linux)
        ost = LINUX;
        return true;
#elif defined(aix)
        ost = AIX;
        return true;
#elif defined(hpux)
        ost = HP_UX;
        return true;
#elif defined(macos)
        ost = MACOS;
        return true;
#elif defined(sun)
        ost = SunOS;
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the OtherTypeDescription
       \param[out]  otd
       \returns     true if this value is supported by the implementation

       \note Linux: Note that the implementation is just plain wrong with
       regard to how this property is defined by the CIM model.

       According to the CIM model:
       A string describing the manufacturer and OperatingSystem
       type - used when the OperatingSystem property, OSType, is
       set to 1 or 59 (\"Other\" or \"Dedicated\"). The format of
       the string inserted in OtherTypeDescription should be
       similar in format to the Values strings defined for OSType.
       OtherTypeDescription should be set to NULL when OSType is
       any value other than 1 or 59.
    */
    bool OSInstance::GetOtherTypeDescription(std::wstring& otd) const
    {
#if defined(SCX_UNIX)
        if (!m_unameIsValid)
        {
            return false;
        }

        string tmp(m_unameInfo.release);
        tmp.append(" ");
        tmp.append(m_unameInfo.version);
        otd.assign(StrFromMultibyte(tmp));
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the Version
       \param[out]  ver
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       A string describing the Operating System's version
       number. The format of the version information is as follows:
       [Major Number].[Minor Number].[Revision] or
       [Major Number].[Minor Number].[Revision Letter].
    */
    bool OSInstance::GetVersion(std::wstring& ver) const
    {
#if defined(SCX_UNIX)
        if (!m_unameIsValid)
        {
            return false;
        }
        ver.assign(StrFromMultibyte(m_unameInfo.release));
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the LastBootUpTime
       \param[out]  lbut
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Time when the OperatingSystem was last booted.
    */
    bool OSInstance::GetLastBootUpTime(SCXCalendarTime& lbut) const
    {
#if defined(linux)
        // Same basic solution as Pegasus, but we have a time PAL to use instead
        // The m_system_boot is set once by Update().
        if (!m_system_boot_isValid) { return false; }
        lbut = m_system_boot;
        return true;
#elif defined(hpux)
        if (!m_system_boot_isValid) { return false; }
        lbut = m_system_boot;
        return true;
#elif defined(aix) || defined(sun)
        // Not supported in the Pegasus implementation for AIX or Solaris
        (void) lbut;
        return false;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the LocalDateTime
       \param[out]  ldt
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       OperatingSystem's notion of the local date and time of day.
    */
    bool OSInstance::GetLocalDateTime(SCXCalendarTime& ldt) const
    {
#if defined(SCX_UNIX)
        ldt = m_now;
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the CurrentTimeZone
       \param[out]  ctz
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       CurrentTimeZone indicates the number of minutes the
       OperatingSystem is offset from Greenwich Mean Time.
       Either the number is positive, negative or zero.
    */
    bool OSInstance::GetCurrentTimeZone(signed short& ctz) const
    {
#if defined(SCX_UNIX)
        ctz = static_cast<signed short>(m_now.GetOffsetFromUTC().GetMinutes());
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the NumberOfLicensedUsers
       \param[out]  nolu
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Number of user licenses for the OperatingSystem.
       If unlimited, enter 0.
    */
    bool OSInstance::GetNumberOfLicensedUsers(unsigned int& nolu) const
    {
#if defined(hpux)
        // Taken from the Pegasus implementation
        if (!m_unameIsValid) { return false; }
        // For HP-UX, the number of licensed users is returned in the version
        // field of uname result.
        switch (m_unameInfo.version[0]) {
        case 'A' : { nolu = 2; break; }
        case 'B' : { nolu = 16; break; }
        case 'C' : { nolu = 32; break; }
        case 'D' : { nolu = 64; break; }
        case 'E' : { nolu = 8; break; }
        case 'U' : {
            // U could be 128, 256, or unlimited
            // need to find test system with 128 or 256 user license
            // to determine if uname -l has correct value
            // for now, return 0 = unlimited
            nolu = 0;
            break;
        }
        default : return false;
        }
        return true;
#elif defined(sun)
        // According to the Pegasus Solaris implementation they don't know how 
        // to determine this number, but they still return 0 for unlimited.
        nolu = 0;
        return true;
#else
        // Thanks to the glory of free software there is no limit on the number of users!
        nolu = 0;
        return true;
#endif
    }

    /**
       Gets the NumberOfUsers
       \param[out]  numberOfUsers
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Number of user sessions for which the OperatingSystem is
       currently storing state information.
    */
    bool OSInstance::GetNumberOfUsers(unsigned int& numberOfUsers) const
    {
#if defined(SCX_UNIX)
        // This is the Pegasus code, straight up. 
        // Note that getutxent() isn't thread safe, but that's no problem 
        // since access here is protected.
        struct utmpx * utmpp;
        
        numberOfUsers = 0;
        
        while ((utmpp = getutxent()) != NULL)
        {
            if (utmpp->ut_type == USER_PROCESS)
            {
                numberOfUsers++;
            }
        }

        endutxent();
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the MaxNumberOfProcesses
       \param[out]  mp
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Maximum number of process contexts the OperatingSystem can
       support. If there is no fixed maximum, the value should be 0.
       On systems that have a fixed maximum, this object can help
       diagnose failures that occur when the maximum is reached.
    */
    bool OSInstance::GetMaxNumberOfProcesses(unsigned int& mp) const
    {
#if defined(linux)
        mp = m_MaxProcesses;
        return m_MaxProcesses != 0;
#elif defined(hpux)
        mp = m_psts.max_proc;
        return m_psts_isValid;
#elif defined(sun) | defined(aix)
        // Not supported by the Pegasus Solaris or AIX implementation
        (void) mp;
        return false;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the MaxProcessMemorySize
       \param[out]  mpms
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Maximum number of Kbytes of memory that can be allocated
       to a Process. For Operating Systems with no virtual memory,
       this value is typically equal to the total amount of
       physical Memory minus memory used by the BIOS and OS. For
       some Operating Systems, this value may be infinity - in
       which case, 0 should be entered. In other cases, this value
       could be a constant - for example, 2G or 4G.
    */
    bool OSInstance::GetMaxProcessMemorySize(scxulong& mpms) const
    {
#if defined(linux) || defined(sun)
        struct rlimit rls;

        int res = getrlimit(RLIMIT_AS, &rls);
        if (0 == res)
        {
            if (RLIM_INFINITY == rls.rlim_max)
            {
                mpms = 0;
            }
            else
            {
                mpms = rls.rlim_max / 1024;
            }
        }
        return res == 0;
#elif defined(hpux)
        // Pegasus implements a very elaborate scheme that supports many
        // different versions of HP/UX through various mechanisms to get 
        // kernel configuration data. Since we only support 11v3 and on 
        // we can cut out all the older stuff and just reuse what we need.
        // But corrected to return the output in Kilobytes.

        const static char *maxsiz[3][2] = { { "maxdsiz", "maxdsiz_64bit" },
                                            { "maxssiz", "maxssiz_64bit" },
                                            { "maxtsiz", "maxtsiz_64bit" }};
        int is64 = sysconf(_SC_KERNEL_BITS) == 64 ? 1 : 0;
        uint64_t data = 0, sum = 0;
        for (int i = 0; i < 3; i++) {
            if (gettune(maxsiz[i][is64], &data) != 0) { return false; }
            sum += data;
        }
        mpms = sum / 1024;
        return true;
#elif defined(aix)
        // Not supported in the Pegasus implementation for AIX.
        (void) mpms;
        return false;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the MaxProcessesPerUser
       \param[out]  mppu
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       A value that indicates the maximum processes that a user
       can have associate with it.
    */
    bool OSInstance::GetMaxProcessesPerUser(unsigned int& mppu) const
    {
#if defined(linux) || defined(sun)
        // Not supported on Solaris, for some reason. We reuse the Linux code. 
        // Here's the Pegasus implementation for Linux. Spot the problem?
        // return sysconf(_SC_CHILD_MAX);
        long res = sysconf(_SC_CHILD_MAX);

        if (res == -1 && errno == 0)
        {
            res = INT_MAX;
        }

        mppu = static_cast<unsigned int>(res);

        return (res >= 0);
#elif defined(hpux)
        // We could use the same sysconf() call as on Linux,

        // but Pegasus uses gettune(), and so do we.
        uint64_t maxuprc = 0;
        if (gettune("maxuprc", &maxuprc) != 0)
        {
            return false;
        }
        mppu = maxuprc;
        return true;
#elif defined(aix)
        // Not supported in the Pegasus implementation for AIX.
        (void) mppu;
        return false;
#else
        #error Platform not supported
#endif
    }

    /*====================================================================================*/
    /* Properties of SCX_OperatingSystem (These comes from PG_OperatingSystem)            */
    /*====================================================================================*/

    /**
       Gets the SystemUpTime
       \param[out]  sut
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       The elapsed time, in seconds, since the OS was booted.
       A convenience property, versus having to calculate
       the time delta from LastBootUpTime to LocalDateTime.
    */
    bool OSInstance::GetSystemUpTime(scxulong& sut) const
    {
#if defined(linux)
        sut = static_cast<scxulong>(m_upsec);
        return true;
#elif defined(sun)
        // Not supported by the Pegasus Solaris implementation
        // The uptime command reads this info from /var/adm/utmpx
        // See the utmpx man page
        (void) sut;
        return false;
#elif defined(hpux)
        // It's simpler to reuse the Pegasus implementation here, than to use our time PAL
        time_t timeval = time(0);
        sut = timeval - m_psts.boot_time;
        return (timeval > 0) && m_psts_isValid;
#elif defined(aix)
        // Not supported in the Pegasus implementation for AIX.
        (void) sut;
        return false;
#else
        #error Platform not supported
#endif
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
