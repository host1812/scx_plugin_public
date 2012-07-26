/*----------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Main implementation file for Meta Provider

    \date      2008-02-01 09:35:36


*/
/*----------------------------------------------------------------------------*/

#include <errno.h>

#include <scxcorelib/scxcmn.h>

#include <fstream>
#include <sstream>

#include <scxcorelib/scxnameresolver.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>
#include <scxsystemlib/cpuenumeration.h>
#include <scxsystemlib/scxsysteminfo.h>
#include "metaprovider.h"

#include "startuplog.h"

#include "buildversion.h"

#if defined(sun)
#include <sys/systeminfo.h>
#include <scxcorelib/scxlocale.h>
#endif

using namespace SCXProviderLib;
using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;


namespace SCXCore {

    /** Installation information file name */
#if defined (macos)
    static const string installInfoFileName = "/private/etc/opt/microsoft/scx/conf/installinfo.txt";
#else
    static const string installInfoFileName = "/etc/opt/microsoft/scx/conf/installinfo.txt";
#endif

    /*----------------------------------------------------------------------------*/
    /**
       Provide CMPI interface for this class

       The class implementation (concrete class) is MetaProvider and the name of the
       provider in CIM registration terms is SCX_MetaProvider.

    */
    SCXProviderDef(MetaProvider, SCX_MetaProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       The Singleton thread lock will be held during this call.
       Reading from install time insformation file and parsing release date since this
       is information that should not change.

    */
    MetaProvider::MetaProvider() :
            BaseProvider(L"scx.core.providers.metaprovider"), m_buildTimeOK(false),
            m_readInstallInfoFile(false)
    {
        LogStartup();
        SCX_LOGTRACE(m_log, L"MetaProvider constructor");

        ReadInstallInfoFile();
        GetReleaseDate();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    MetaProvider::~MetaProvider()
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
    void MetaProvider::DoInit()
    {
        SCX_LOGTRACE(m_log, L"MetaProvider::DoInit");
        m_ProviderCapabilities.RegisterCimClass(eSCX_Agent, L"SCX_Agent");
    }


    /*----------------------------------------------------------------------------*/
    /**
        Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void MetaProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"MetaProvider::DoCleanup");

        m_ProviderCapabilities.Clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Read installation information from file

       This method reads some proprietary information the SCX installer saves 
       in well-known location. Called from constructor since this reads in 
       information that should not change.

    */
    void MetaProvider::ReadInstallInfoFile()
    {
        wifstream infofile(installInfoFileName.c_str());

        m_readInstallInfoFile = false;

        if (SCXStream::IsGood(infofile))
        {
            vector<wstring> lines;
            SCXStream::NLFs nlfs;

            // Read all lines from install info file
            // First line should be install date on ISO8601 format
            // Second line should be install version string
            // Example:
            // 2008-03-17T17:28:32.0Z
            // 1.0.1-70
            SCXStream::ReadAllLines(infofile, lines, nlfs);
            if (lines.size() == 2)
            {
                SCX_LOGTRACE(m_log, StrAppend(L"Read time from installinfo file: ", lines[0]));
                SCX_LOGTRACE(m_log, StrAppend(L"Read install version from installinfo file: ", lines[1]));

                m_installVersion = lines[1];
                try
                {
                    m_installTime = SCXCalendarTime::FromISO8601(lines[0]);
                    m_readInstallInfoFile = true;
                }
                catch (SCXCoreLib::SCXException &e)
                {
                    SCX_LOGERROR(m_log, StrAppend(StrAppend(StrAppend(L"Failed to convert install time string to SCXCalenderTime: ", lines[0]), L" - "), e.What()));
                }
            }
            else
            {
                SCX_LOGERROR(m_log, StrAppend(L"Wrong number of rows in install info file. Expected 2, got: ", lines.size()));
            }
        }
        else
        {
            std::wstring errStr = L"Failed to open installinfo file " + StrFromMultibyte(installInfoFileName);
            SCX_LOGERROR(m_log, errStr);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Convert build date string to SCXCalendarTime

       At compile time the build timestamp is provided by preprocessor in 
       SCX_BUILDVERSION_DATE. This method converts time format to SCXCalendarTime.

    */
    void MetaProvider::GetReleaseDate()
    {
        m_buildTimeOK = false;

        wstring buildDate(SCX_BUILDVERSION_DATE);

        if (buildDate.length() == 8)
        {
            wstring buildYear = buildDate.substr(0, 4);
            wstring buildMonth = buildDate.substr(4, 2);
            wstring buildDay = buildDate.substr(6, 2);

            try
            {
                m_buildTime = SCXCalendarTime(StrToUInt(buildYear),StrToUInt(buildMonth),StrToUInt(buildDay));
                m_buildTimeOK = true;
                SCX_LOGTRACE(m_log, StrAppend(L"Build time: ", buildDate));
            }
            catch (SCXCoreLib::SCXException& e)
            {
                SCX_LOGERROR(m_log, StrAppend(L"Failed to convert build time string to SCXCalenderTime: ", buildDate));
            }
        }
        else
        {
            SCX_LOGWARNING(m_log, StrAppend(L"Build time string is not correct length: ", buildDate));
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Set all keys in the SCXInstance

       \param[out]  inst  Instance to add keys to

    */
    void MetaProvider::AddKeys(SCXInstance &inst) // private
    {
        SCXProperty name_prop(L"Name", L"scx");
        inst.AddKey(name_prop);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set all properties in the SCXInstance

       \param[in]  inst  Instance to populate

    */
    void MetaProvider::AddProperties(SCXInstance &inst) // private
    {

        //
        // Populate properties regarding the agent's build number
        // 
        wstringstream ss;

        ss << SCX_BUILDVERSION_MAJOR << L"." << SCX_BUILDVERSION_MINOR << L"." << SCX_BUILDVERSION_PATCH << L"-" << SCX_BUILDVERSION_BUILDNR;

        SCXProperty version_prop(L"VersionString", ss.str());
        inst.AddProperty(version_prop);

        SCXProperty major_prop(L"MajorVersion", static_cast<unsigned short>(SCX_BUILDVERSION_MAJOR));
        inst.AddProperty(major_prop);

        SCXProperty minor_prop(L"MinorVersion", static_cast<unsigned short>(SCX_BUILDVERSION_MINOR));
        inst.AddProperty(minor_prop);

        SCXProperty patch_prop(L"RevisionNumber", static_cast<unsigned short>(SCX_BUILDVERSION_PATCH));
        inst.AddProperty(patch_prop);

        SCXProperty build_prop(L"BuildNumber", static_cast<unsigned short>(SCX_BUILDVERSION_BUILDNR));
        inst.AddProperty(build_prop);

        SCXProperty status_prop(L"Description", StrAppend(StrAppend(SCX_BUILDVERSION_STATUS, " - "), SCX_BUILDVERSION_DATE));
        inst.AddProperty(status_prop);

        if (m_readInstallInfoFile)
        {
            SCXProperty installversion_prop(L"KitVersionString", m_installVersion);
            inst.AddProperty(installversion_prop);

            // provide standard property as "date-time"
            SCXProperty installdate_prop(L"InstallDate", m_installTime);
            inst.AddProperty(installdate_prop);
        }

        // 
        // Populate the build date - the value is looked up by ctor
        //
        if (m_buildTimeOK)
        {
            SCXProperty builddate_prop(L"BuildDate", m_buildTime.ToExtendedISO8601());
            inst.AddProperty(builddate_prop);
        }

        // 
        // Populate the hostname date - the value is cached internally in the MachnieInfo code.
        //
        try {
            NameResolver mi;
            SCXProperty hostname_prop(L"Hostname", mi.GetHostDomainname());
            inst.AddProperty(hostname_prop);
        } catch (SCXException& e) {
            SCX_LOGWARNING(m_log, StrAppend(
                               StrAppend(L"Can't read host/domainname because ", e.What()),
                               e.Where()));
        }


        // 
        // Populate name, version and alias for the OS 
        // 
        SCXProperty os_prop(L"OSName", m_osTypeInfo.GetOSName());
        inst.AddProperty(os_prop);

        SCXProperty osver_prop(L"OSVersion", m_osTypeInfo.GetOSVersion());
        inst.AddProperty(osver_prop);

        SCXProperty alias_prop(L"OSAlias", m_osTypeInfo.GetOSAlias());
        inst.AddProperty(alias_prop);

        SCXProperty ostype_prop(L"OSType", m_osTypeInfo.GetOSFamilyString());
        inst.AddProperty(ostype_prop);

        SCXProperty arch_prop(L"Architecture", m_osTypeInfo.GetArchitectureString());
        inst.AddProperty(arch_prop);

        // 
        // This property contains the architecture as uname reports it
        // 
        SCXProperty uname_prop(L"UnameArchitecture", m_osTypeInfo.GetUnameArchitectureString());
        inst.AddProperty(uname_prop);

        // 
        // Set property indicating what the lowest log level currently in effect for 
        // the agent is
        // 
        SCXProperty loglevel_prop(L"MinActiveLogSeverityThreshold",
                                  SCXCoreLib::SCXLogHandleFactory::GetLogConfigurator()->GetMinActiveSeverityThreshold());
        inst.AddProperty(loglevel_prop);

        //
        // Populate the type of machine this is (Physical, Virtual, or Unknown)
        //
        try {
            SystemInfo sysInfo;

            eVmType vmType;
            sysInfo.GetVirtualMachineState(vmType);

            wstring vmText;
            switch (vmType)
            {
                case eVmDetected:
                    vmText = L"Virtual";
                    break;

                case eVmNotDetected:
                    vmText = L"Physical";
                    break;

                case eVmUnknown:
                default:
                    vmText = L"Unknown";
                    break;
            }

            SCXProperty virtual_prop(L"MachineType", vmText);
            inst.AddProperty(virtual_prop);
        } catch (SCXException& e) {
            SCX_LOGWARNING(m_log, StrAppend(
                               StrAppend(L"Can't read virtual machine state because ", e.What()),
                               e.Where()));
        }

        //
        // Populate the number of physical and logical processors
        //
        try {
            scxulong count;
            if (SCXSystemLib::CPUEnumeration::GetProcessorCountPhysical(count, m_log))
            {
                SCXProperty physical_prop(L"PhysicalProcessors", count);
                inst.AddProperty(physical_prop);
            }
        } catch (SCXException& e) {
            SCX_LOGWARNING(m_log, StrAppend(
                               StrAppend(L"Can't read physical processor count because ", e.What()),
                               e.Where()));
        }

        try {
            scxulong count;
            if (SCXSystemLib::CPUEnumeration::GetProcessorCountLogical(count))
            {
                SCXProperty logical_prop(L"LogicalProcessors", count);
                inst.AddProperty(logical_prop);
            }
        } catch (SCXException& e) {
            SCX_LOGWARNING(m_log, StrAppend(
                               StrAppend(L"Can't read logical processor count because ", e.What()),
                               e.Where()));
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instance names

       \param[in]   callContext Context details of this request
       \param[out]  instances   Collection of instances with key properties

    */
    void MetaProvider::DoEnumInstanceNames(const SCXCallContext& /*callContext*/,
                                           SCXInstanceCollection& instances)
    {
        SCX_LOGTRACE(m_log, L"MetaProvider DoEnumInstanceNames");

        SCXInstance inst;

        AddKeys(inst);

        instances.AddInstance(inst);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instances

       \param[in]     callContext                 Details of the client request
       \param[out]    instances                   Collection of instances

    */
    void MetaProvider::DoEnumInstances(const SCXCallContext& /*callContext*/,
                                       SCXInstanceCollection& instances)
    {
        SCX_LOGTRACE(m_log, L"MetaProvider DoEnumInstances");

        SCXInstance inst;

        AddKeys(inst);
        AddProperties(inst);

        instances.AddInstance(inst);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get an instance

       \param[in]   callContext Context of the original request, indicating which instance to retrieve.
       \param[out]  instance    The returned instance

    */
    void MetaProvider::DoGetInstance(const SCXProviderLib::SCXCallContext& callContext, SCXProviderLib::SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"MetaProvider DoGetInstance");
        const SCXInstance& keys = callContext.GetObjectPath();
        ValidateKeyValue(L"Name", keys, L"scx");
        AddKeys(instance);
        AddProperties(instance);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns       The object represented as a string suitable for logging.

    */
    const std::wstring MetaProvider::DumpString() const
    {
        return L"MetaProvider";
    }



    /*----------------------------------------------------------------------------*/
    /**
       Log the startup message once regardless of how many times called
       
       This function will be called from all provider ctors except RunAs to 
       provide one initial log, regardless of how many times called.
       
    */
    void LogStartup(void) 
    {
        static bool bLoggedInitMessage = false;
        if (!bLoggedInitMessage) 
        {
            SCXCoreLib::SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.providers");
            SCX_LOGINFO(log, L"SCX Provider Module loaded");
            
#if defined(sun)
            
            // Log any errors encountered during SCXLocaleContext initialization.
            for (size_t i = 0; i < SCXLocaleContext::GetErrors().size(); i++)
            {
                SCX_LOGWARNING(log, SCXLocaleContext::GetErrors()[i]);
            }
            
#endif /* defined(sun) */
            
            bLoggedInitMessage = true;
        }
    }
    
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

