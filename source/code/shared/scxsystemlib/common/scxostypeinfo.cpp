/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Retrieve basic OS information

    \date        09-05-01 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <fstream>
#include <vector>
#include <errno.h>
#include <assert.h>

#if defined(macos)
#include <Gestalt.h>
#include <sys/sysctl.h>
#include <iostream>
#endif

#if defined(sun)
#include <sys/systeminfo.h>
#endif

#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>

#include <scxsystemlib/scxostypeinfo.h>
#include <scxsystemlib/scxsysteminfo.h>

using namespace std;
using namespace SCXCoreLib;

/* This is an anonymous namespace that holds utility classes that are local. */

namespace {

#if defined(linux)
    /**
       Extracts the distribution specific OS name.

       \param[in]  platformString String to extract info from
       \returns    The name of the operating system

       This is a Linux-specific function that picks the name without version
       and os info from a distribution specific file.
    */
    wstring ExtractOSName(const wstring& platformString)
    {
        wstring caption(platformString);

        // We assume caption somewhere in the end has a release number containing
        // a digit. The OSName is then probably all "words" before then "release
        // number word".
        wstring::size_type n = caption.find_first_of(L"0123456789");
        if (n != wstring::npos)
        {
            n = caption.substr(0,n).find_last_of(L" \t\n\t");
            caption = caption.substr(0,n);
        }
        // On redhat the word "release" may also be part of the caption.
        // Remove that part also on all platforms to be sure.
        n = caption.find(L"release");
        if (n != wstring::npos)
          {
            n = caption.substr(0,n).find_last_of(L" \t\n\t");
            caption = caption.substr(0,n);
          }

        return StrTrim(caption);
    }

    /*----------------------------------------------------------------------------*/
#endif /* linux */

}


namespace SCXSystemLib
{

    SCXOSTypeInfo::SCXOSTypeInfo() : 
        m_osVersion(L""), m_osName(L""), m_osAlias(L""),
        m_unameIsValid(false)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.common.scxostypeinfo");
#if defined(hpux)
        /* Get information the system static variables (guaranteed constant until reboot) */
        m_unameIsValid = !((uname(&m_unameInfo) < 0) && (errno != EOVERFLOW));
        // Meaning: Ok if no error, or if errno is EOVERFLOW
#else
        m_unameIsValid = !(uname(&m_unameInfo) < 0);
#endif
        if (!m_unameIsValid)
        {
            throw SCXCoreLib::SCXErrnoException(L"uname", errno, SCXSRCLOCATION);
        }

        // Populate value caches
        Init();

    }


    /*----------------------------------------------------------------------------*/
    /**
       Dtor
    */
    SCXOSTypeInfo::~SCXOSTypeInfo()
    {
    }


    /*----------------------------------------------------------------------------*/
    /**
       Return the "human readable" name of the Operating System
    
       \param       bCompatMode - If true, for RHEL and SLES, return compatible string, see details
       \returns     String with the name (in some sense) of the OS
       
       For most of the cases, this function only returns information been set up
       by GetOSNameAndVersion(). However, if bCompatMode is true, for historical
       reasons we need to give special treatment to SuSE and RedHat for this 
       property since the Library MPs for these look for the explicit string :-(
    
    */
    std::wstring SCXOSTypeInfo::GetOSName(bool bCompatMode) const
    {
        if (bCompatMode)
        {
#if defined(linux) && defined(PF_DISTRO_SUSE)
            return L"SuSE Distribution";
# elif defined(linux) && defined(PF_DISTRO_REDHAT)
            return L"Red Hat Distribution";
#endif
            // For all other, fall through since compat mode is irrelevant
        }

        return m_osName;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Return the Caption for the OS, typically a longer string 
    
       \returns     Caption for OS, intended for display/human reader
       \throws      SCXCoreLib::SCXNotSupportedException - if not implemented for the platform
    
    */
    std::wstring SCXOSTypeInfo::GetCaption() const
    {
#if defined(linux)
        return m_linuxDistroCaption;
#elif defined(sun) || defined(hpux)
        // append in caption the system name and release.
        string capt(m_unameInfo.sysname);
        capt.append(" ");
        capt.append(m_unameInfo.release);
        return SCXCoreLib::StrFromMultibyte(capt);

#elif defined(aix)
        // append in caption the system name and release.
        string capt(m_unameInfo.sysname);
        capt.append(" ");
        capt.append(m_unameInfo.version);
        capt.append(".");
        capt.append(m_unameInfo.release);
        return SCXCoreLib::StrFromMultibyte(capt);
#elif defined(macos)
        // append in caption the system name and release.
        wstring capt(m_osName);
        capt.append(L" ");
        capt.append(m_osVersion);
        return capt;
#else
        throw SCXCoreLib::SCXNotSupportedException(L"GetCaption()",  SCXSRCLOCATION);
#endif
    }


    /*----------------------------------------------------------------------------*/
    /**
       Find out operating system name and version

       This method caches system information in m_osName, m_osVersion and m_osAlias.
       It requires m_unameInfo to be set prior to call. 

    */
    void SCXOSTypeInfo::Init()  // private
    {
        m_osVersion = L"";
        m_osName = L"Unknown";

        assert(m_unameIsValid);

#if defined(hpux) || defined(sun)

        if (m_unameIsValid)
        {
            m_osName = StrFromMultibyte(m_unameInfo.sysname);
            m_osVersion = StrFromMultibyte(m_unameInfo.release);
        }

#if defined(hpux)
        m_osAlias = L"HPUX";
#elif defined(sun)
        m_osAlias = L"Solaris";
#endif

#elif defined(aix)

        if (m_unameIsValid)
        {
            m_osName = StrFromMultibyte(m_unameInfo.sysname);

            // To get "5.3" we must read "5" and "3" from different fields.
            string ver(m_unameInfo.version);
            ver.append(".");
            ver.append(m_unameInfo.release);
            m_osVersion = StrFromMultibyte(ver);
        }
        m_osAlias = L"AIX";

#elif defined(linux)

        vector<wstring> lines;
        SCXStream::NLFs nlfs;

#if defined(PF_DISTRO_SUSE)

        static const string relFileName = "/etc/SuSE-release";
        wifstream relfile(relFileName.c_str());
        wstring version(L"");
        wstring patchlevel(L"");

        SCXStream::ReadAllLines(relfile, lines, nlfs);

        if (!lines.empty()) {
            m_osName = ExtractOSName(lines[0]);
        }

        // Set the Linux Caption (get first line of the /etc/SuSE-release file)
        m_linuxDistroCaption = lines[0];
        if (0 == m_linuxDistroCaption.length())
        {
            // Fallback - should not normally happen
            m_linuxDistroCaption = L"SuSE";
        }

        // File contains one or more lines looking like this:
        // SUSE Linux Enterprise Server 10 (i586)
        // VERSION = 10
        // PATCHLEVEL = 1
        for (size_t i = 0; i<lines.size(); i++)
        {
            if (StrIsPrefix(StrTrim(lines[i]), L"VERSION", true))
            {
                wstring::size_type n = lines[i].find_first_of(L"=");
                if (n != wstring::npos)
                {
                    version = StrTrim(lines[i].substr(n+1));
                }
            }
            else if (StrIsPrefix(StrTrim(lines[i]), L"PATCHLEVEL", true))
            {
                wstring::size_type n = lines[i].find_first_of(L"=");
                if (n != wstring::npos)
                {
                    patchlevel = StrTrim(lines[i].substr(n+1));
                }
            }
        }

        if (version.length() > 0)
        {
            m_osVersion = version;

            if (patchlevel.length() > 0)
            {
                m_osVersion = version.append(L".").append(patchlevel);
            }
        }
        
        if (std::wstring::npos != m_osName.find(L"Desktop"))
        { 
            m_osAlias = L"SLED";
        }
        else
        { // Assume server.
            m_osAlias = L"SLES";
        }
#elif defined(PF_DISTRO_REDHAT)

        static const string relFileName = "/etc/redhat-release";
        wifstream relfile(relFileName.c_str());

        SCXStream::ReadAllLines(relfile, lines, nlfs);

        if (!lines.empty()) {
            m_osName = ExtractOSName(lines[0]);
        }

        // Set the Linux Caption (get first line of the /etc/redhat-release file)
        m_linuxDistroCaption = lines[0];
        if (0 == m_linuxDistroCaption.length())
        {
            // Fallback - should not normally happen
            m_linuxDistroCaption = L"Red Hat";
        }

        // File should contain one line that looks like this:
        // Red Hat Enterprise Linux Server release 5.1 (Tikanga)
        if (lines.size() > 0)
        {
            wstring::size_type n = lines[0].find_first_of(L"0123456789");
            if (n != wstring::npos)
            {
                wstring::size_type n2 = lines[0].substr(n).find_first_of(L" \t\n\t");
                m_osVersion = StrTrim(lines[0].substr(n,n2));
            }
        }
        
        if ((std::wstring::npos != m_osName.find(L"Client")) // RHED5
            || (std::wstring::npos != m_osName.find(L"Desktop"))) // RHED4
        { 
            m_osAlias = L"RHED";
        }
        else
        { // Assume server.
            m_osAlias = L"RHEL";
        }
#elif defined(PF_DISTRO_UBUNTU)

        static const string relFileName = "/etc/lsb-release";
        wifstream relfile(relFileName.c_str());
        wstring version(L"");
        wstring patchlevel(L"");

        SCXStream::ReadAllLines(relfile, lines, nlfs);

        // File contains one or more lines looking like this:
        // DISTRIB_ID=Ubuntu
        // DISTRIB_RELEASE=6.06
        // DISTRIB_CODENAME=dapper
        // DISTRIB_DESCRIPTION="Ubuntu 6.06.2 LTS"

        for (size_t i = 0; i<lines.size(); i++)
        {
            if (StrIsPrefix(StrTrim(lines[i]), L"DISTRIB_DESCRIPTION", true))
            {
                wstring::size_type n = lines[i].find_first_of(L"\"");
                if (n != wstring::npos)
                {
                    // Strip the quote (") characters ...
                    m_osName = StrTrim(lines[i].substr(n+1));
                    m_osName = m_osName.erase(m_osName.length()-1);
                    // This is an alias on Ubuntu (and not used for SCX_STACK_ONLY)
                    m_linuxDistroCaption = m_osName;
                }
            }
        }

        if (! m_osName.empty())
        {
            wstring::size_type n = m_osName.find_first_of(L"0123456789");
            if (n != wstring::npos)
            {
                wstring::size_type n2 = m_osName.substr(n).find_first_of(L" \t\n\t");
                m_osVersion = StrTrim(m_osName.substr(n,n2));
            }

            m_osName = ExtractOSName(m_osName);
        }
        m_osAlias = L"Ubuntu";

#else
#error "Linux Platform not supported"
#endif

#elif defined(macos)
        m_osAlias = L"MacOS";

        if (m_unameIsValid)
        {
            // MacOS is called "Darwin" in uname info, so we hard-code here
            m_osName = L"Mac OS";

            // This value we could read dynamically from the xml file
            // /System/Library/CoreServices/SystemVersion.plist, but that
            // file may be named differently based on client/server, and
            // reading the plist file would require framework stuff.
            //
            // Rather than using the plist, we'll use Gestalt, which is an
            // API designed to figure out versions of anything and everything.
            // Note that use of Gestalt requires the use of framework stuff
            // as well, so the Makefiles for MacOS are modified for that.

            SInt32 major, minor, bugfix;
            if (0 != Gestalt(gestaltSystemVersionMajor, &major)
                || 0 != Gestalt(gestaltSystemVersionMinor, &minor)
                || 0 != Gestalt(gestaltSystemVersionBugFix, &bugfix))
            {
                throw SCXCoreLib::SCXErrnoException(L"Gestalt", errno, SCXSRCLOCATION);
            }

            wostringstream sout;
            sout << major << L"." << minor << L"." << bugfix;
            m_osVersion = sout.str();
        }
#else
#error "Platform not supported"
#endif

    }

    /*----------------------------------------------------------------------------*/
    /**
       Returns the "family name" of the OS, e.g. "Linux" for Linux
    
       \returns     String with name of the OS family
       
    */
    std::wstring SCXOSTypeInfo::GetOSFamilyString() const
    {
#if defined(hpux)
        return L"HPUX";
#elif defined(linux)
        return L"Linux";
#elif defined(sun)
        return L"Solaris";
#elif defined(aix)
        return L"AIX";
#elif defined(macos)
        return L"MacOS";
#else 
#error "Not defined for this platform"
#endif
    }


    /*----------------------------------------------------------------------------*/
    /**
       Returns the architecture of the platform, e.g. PA-Risc, x86 or SPARC
    
       \returns   String with the architecture of the platform
    
    */
    std::wstring SCXOSTypeInfo::GetArchitectureString() const
    {
#if defined(hpux)
#if defined(hppa)
        return L"PA-Risc";
#else
        return L"IA64";
#endif // defined(hpux)

#elif defined(linux)

        unsigned short bitSize = 0;
        try
        {
            SystemInfo sysInfo;
            sysInfo.GetNativeBitSize(bitSize);
        }
        catch (SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, StrAppend( StrAppend(L"Failure in SystemInstance::GetNativeBitSize: ", e.What()), e.Where()));
        }

        if (32 == bitSize)
        {
            return L"x86";
        }
        else if (64 == bitSize) 
        {
            return L"x64";
        }
        else 
        {
            assert(!"Unknown architecture");
            return L"Unknown";
        }

#elif defined(sun)
            
#if defined(sparc)
        return L"SPARC";
#else
        return L"x86";
#endif
        
#elif defined(aix)
        return L"powerpc";     // This is what uname -p says
        
#elif defined(macos)

        // On MacOS Intel platforms, the architecture appears to always be i386,
        // regardless of 32-bit vs. 64-bit capabilities.  However, since we may
        // (some day) run on something else, we'll implement this properly.
        //
        // Intel (i386) on Mac is a special case: we return 'x86' or 'x64' based
        // on 64-bit capabilities of the CPU.

        // First get the machine architecture dynamically

        int mib[2];
        char hwMachine[64];
        size_t len_hwMachine = sizeof(hwMachine);

        mib[0] = CTL_HW;
        mib[1] = HW_MACHINE;

        if (0 != sysctl(mib, 2, hwMachine, &len_hwMachine, NULL, 0))
        {
            wostringstream sout;
            sout << L"Failure calling sysctl(): Errno=" << errno;
            SCX_LOGERROR(m_log, sout.str());

            return L"";
        }

        // Now figure out our bit size (if Intel, handle the special case)

        if (0 == strncmp("i386", hwMachine, sizeof(hwMachine)))
        {
            unsigned short bitSize = 0;
            try
            {
                SystemInfo sysInfo;
                sysInfo.GetNativeBitSize(bitSize);
            }
            catch (SCXCoreLib::SCXException &e)
            {
                SCX_LOGERROR(m_log, StrAppend( StrAppend(L"Failure in SystemInstance::GetNativeBitSize: ", e.What()), e.Where()));
            }

            if (32 == bitSize)
            {
                return L"x86";
            }
            else if (64 == bitSize) 
            {
                return L"x64";
            }
        }

        // Return the actual architecture, whatever it is

        return StrFromMultibyte(hwMachine);

#else
#error "Platform not supported"
#endif
    }
    

    /*----------------------------------------------------------------------------*/
    /**
       Returns the architecture of the current platform, as uname(3) reports it
    
       \returns     String with uname result
       \throws      < One for each exception from this function >
    
    */
    std::wstring SCXOSTypeInfo::GetUnameArchitectureString() const
    {
        assert(m_unameIsValid);
        
#if defined(linux) || defined(hpux) || defined(macos)
        if (m_unameIsValid)
        {
            return StrFromMultibyte(m_unameInfo.machine);
        }
#elif defined(sun)
        char buf[256];
        if (0 < sysinfo(SI_ARCHITECTURE, buf, sizeof(buf)))
        {
            return StrFromMultibyte(buf);
        }
#elif defined(aix)
        // Can't find a generic way to get this property on AIX. This is however the
        // only one supported right now.
        return L"powerpc";
#else
#error "Platform not supported"
#endif
        return L"Platform not supported";
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
