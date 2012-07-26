/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       Implementation of OS Provider

   \date        08-03-07 10:55:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxnameresolver.h>
#include <scxcorelib/scxmath.h>

#include <scxproviderlib/scxprovidercapabilities.h>

#include "osprovider.h"

#include "../meta_provider/startuplog.h"

#include <scxsystemlib/osenumeration.h>
#include <scxsystemlib/osinstance.h>
#include <scxsystemlib/processenumeration.h>
#include <scxsystemlib/scxsysteminfo.h>

using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;

namespace SCXCore {

    /*----------------------------------------------------------------------------*/
    /**
       Provide CMPI interface for this class

       The class implementation (concrete class) is OSProvider and the name of the
       provider in CIM registration terms is SCX_OSProvider.

    */
    SCXProviderDef(OSProvider, SCX_OSProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       The Singleton thread lock will be held during this call.

    */
    OSProvider::OSProvider() :
        BaseProvider(L"scx.core.providers.osprovider"),
        m_osEnum(NULL),
        m_memEnum(NULL),
        m_OSTypeInfo(NULL)
    {
        LogStartup();

        SCX_LOGTRACE(m_log, L"OSProvider constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    OSProvider::~OSProvider()
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
    void OSProvider::DoInit()
    {
        SCX_LOGTRACE(m_log, L"OSProvider::DoInit");

        if (m_osEnum != NULL)
        {
            SCXASSERTFAIL(L"DoInit() called multiple times without a call to DoCleanup() between");
            DoCleanup();
        }

        SCXASSERT(NULL == m_memEnum);

        m_ProviderCapabilities.RegisterCimClass(eSCX_OperatingSystem,
                                                s_cOSCreationClassName);
        m_osEnum = new OSEnumeration();
        m_osEnum->Init();

        // We need the memory provider for some stuff as well
        m_memEnum = new MemoryEnumeration();
        m_memEnum->Init();


        SCXASSERT(NULL == m_OSTypeInfo);
        try {
            m_OSTypeInfo = new SCXSystemLib::SCXOSTypeInfo();
        }
        catch (SCXException& e) {
            SCX_LOGWARNING(m_log, StrAppend(
                               StrAppend(L"Failed to look up statick OS information", e.What()),
                               e.Where()));
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void OSProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"OSProvider::DoCleanup");

        m_ProviderCapabilities.Clear();

        if (m_osEnum != NULL)
        {
            m_osEnum->CleanUp();
            m_osEnum = NULL;
        }

        if (m_memEnum != NULL)
        {
            m_memEnum->CleanUp();
            m_memEnum = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set the keys of the SCXInstance object from the information in the entity instance

       \param[in]       osinst  Internal instance representation to get data from
       \param[out]      inst   Collection to add instance to

       \throws          SCXInvalidArgumentException  The instance can not be converted to a os instance

       This method contains knowledge on which are the key fields for the class.
       The key properties are defined in the MOF file.

    */
    void OSProvider::AddKeys(SCXCoreLib::SCXHandle<OSInstance> osinst, SCXInstance &inst) const
    {
        SCX_LOGTRACE(m_log, L"OSProvider AddKeys()");
                SCXASSERT(NULL != m_OSTypeInfo); 

        if (osinst == NULL)
        {
            throw SCXInvalidArgumentException(L"einst", L"Not a OSInstance", SCXSRCLOCATION);
        }

        // Set all the properties which are key properties to the SCXInstance

                SCXProperty osname_prop(L"Name", m_OSTypeInfo->GetOSName(true));
                inst.AddKey(osname_prop);

        SCXProperty csccn_prop(L"CSCreationClassName", s_cCSCreationClassName);
        inst.AddKey(csccn_prop);

        try {
            NameResolver mi;
            SCXProperty prop(L"CSName", mi.GetHostDomainname());
            inst.AddKey(prop);
        } catch (SCXException& e) {
            SCX_LOGWARNING(m_log, StrAppend(
                               StrAppend(L"Can't read host/domainname because ", e.What()),
                               e.Where()));
        }

        SCXProperty ccn_prop(L"CreationClassName", s_cOSCreationClassName);
        inst.AddKey(ccn_prop);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set all properties of the SCXInstance from the information in the EntityInstance

       \param[in]       osinst  Internal instance representation to get data from
       \param[in]       meminst Internal memory instance representation to get data from
       \param[out]      inst    Instance to populate

       throws           SCXInvalidArgumentException The instance can not be converted to a os instance

       This method knows how to map the values of the internal instance representation
       to the CMPI class definition.

    */
    void OSProvider::AddProperties(
        SCXCoreLib::SCXHandle<OSInstance> osinst,
        SCXCoreLib::SCXHandle<MemoryInstance> meminst,
        SCXInstance &inst) const
    {
        if (osinst == NULL) {
            throw SCXInvalidArgumentException(L"osinst", L"Not an OSInstance", SCXSRCLOCATION);
        }

        SCX_LOGTRACE(m_log, L"OSProvider::AddPropeties()");

        SCXCalendarTime ASCXCalendarTime;
        scxulong Ascxulong, Ascxulong1;
        unsigned short Aunsignedshort;
        vector<string> Avector;
        vector<unsigned short> Aushortvector;
        wstring Awstring;
        signed short Ashort;
        unsigned int Auint;

        SCXASSERT(NULL != m_OSTypeInfo);

        /*===================================================================================*/
        /* Properties of CIM_ManagedElement                                                  */
        /*===================================================================================*/
                
        wstring caption = m_OSTypeInfo->GetCaption();
        SCXProperty captionProp(L"Caption", caption);
        inst.AddProperty(captionProp);

        // Description is a synonym to Caption
        SCXProperty descProp(L"Description", caption);
        inst.AddProperty(descProp);

        /*===================================================================================*/
        /* Properties of CIM_ManagedSystemElement                                            */
        /*===================================================================================*/

        // We don't support the following because there's no way to retrieve on any platforms:
        //      InstallDate
        //      Status
        //      OperationalStatus
        //      StatusDescriptions
        //      HealthState

        /*===================================================================================*/
        /* Properties of CIM_OperatingSystem                                                 */
        /*===================================================================================*/

        // We don't support the following because there's no way to retrieve on any platforms:
        //      EnabledState
        //      OtherEnabledState
        //      RequestedState
        //      EnabledDefault
        //      TimeOfLastStateChange
        //      OverwritePolicy
        //      Distributed

        /* CSCreationClassName is a key property and thus set in AddKeys */
        /* CSName is a key property and thus set in AddKeys */
        /* CreationClassName is a key property and thus set in AddKeys */

        if (osinst->GetOSType(Aunsignedshort)){
            SCXProperty prop(L"OSType", Aunsignedshort);
            inst.AddProperty(prop);
        }
        if (osinst->GetOtherTypeDescription(Awstring)){
            SCXProperty prop(L"OtherTypeDescription", Awstring);
            inst.AddProperty(prop);
        }
        if (osinst->GetVersion(Awstring)){
            SCXProperty prop(L"Version", Awstring);
            inst.AddProperty(prop);
        }
        if (osinst->GetLastBootUpTime(ASCXCalendarTime)){
            SCXProperty prop(L"LastBootUpTime", ASCXCalendarTime);
            inst.AddProperty(prop);
        }
        if (osinst->GetLocalDateTime(ASCXCalendarTime)){
            SCXProperty prop(L"LocalDateTime", ASCXCalendarTime);
            inst.AddProperty(prop);
        }
        if (osinst->GetCurrentTimeZone(Ashort)){
            SCXProperty prop(L"CurrentTimeZone", Ashort);
            inst.AddProperty(prop);
        }
        if (osinst->GetNumberOfLicensedUsers(Auint)){
            SCXProperty prop(L"NumberOfLicensedUsers", Auint);
            inst.AddProperty(prop);
        }
        if (osinst->GetNumberOfUsers(Auint)){
            SCXProperty prop(L"NumberOfUsers", Auint);
            inst.AddProperty(prop);
        }
        if (ProcessEnumeration::GetNumberOfProcesses(Auint)){
            SCXProperty prop(L"NumberOfProcesses", Auint);
            inst.AddProperty(prop);
        }
        if (osinst->GetMaxNumberOfProcesses(Auint)){
            SCXProperty prop(L"MaxNumberOfProcesses", Auint);
            inst.AddProperty(prop);
        }
        if (meminst->GetTotalSwap(Ascxulong)) {
            // TotalSwapSpaceSize comes in MB - we need KBytes
            SCXProperty prop(L"TotalSwapSpaceSize", Ascxulong * 1024);
            inst.AddProperty(prop);
        }
        if (meminst->GetTotalPhysicalMemory(Ascxulong) && meminst->GetTotalSwap(Ascxulong1)) {
            // Both TotalPhysicalMemory and TotalSwap come in MB - we need KBytes
            SCXProperty prop(L"TotalVirtualMemorySize", Ascxulong*1024 + Ascxulong1*1024);
            inst.AddProperty(prop);
        }
        if (meminst->GetAvailableMemory(Ascxulong)) {
            Ascxulong *= 1024; // Convert GetAvailableMemory from MB to KBytes

            if (meminst->GetAvailableSwap(Ascxulong1)) {
                // GetAvailableSwap comes in MB - we need KBytes
                SCXProperty prop(L"FreeVirtualMemory", Ascxulong + Ascxulong1*1024);
                inst.AddProperty(prop);
            }

            SCXProperty prop(L"FreePhysicalMemory", Ascxulong);
            inst.AddProperty(prop);
        }
        if (meminst->GetTotalPhysicalMemory(Ascxulong)) {
            // GetTotalPhysicalMemory comes in MB - we need KBytes
            SCXProperty prop(L"TotalVisibleMemorySize", Ascxulong * 1024);
            inst.AddProperty(prop);
        }
        if (meminst->GetTotalSwap(Ascxulong)) {
            // GetTotalSwap comes in MB - we need KBytes
            SCXProperty prop(L"SizeStoredInPagingFiles", Ascxulong * 1024);
            inst.AddProperty(prop);
        }
        if (meminst->GetAvailableSwap(Ascxulong)) {
            // GetAvailableSwap comes in MB - we need KBytes
            SCXProperty prop(L"FreeSpaceInPagingFiles", Ascxulong * 1024);
            inst.AddProperty(prop);
        }
        if (osinst->GetMaxProcessMemorySize(Ascxulong)){
            SCXProperty prop(L"MaxProcessMemorySize", Ascxulong);
            inst.AddProperty(prop);
        }
        if (osinst->GetMaxProcessesPerUser(Auint)){
            SCXProperty prop(L"MaxProcessesPerUser", Auint);
            inst.AddProperty(prop);
        }

        /*===================================================================================*/
        /* Properties of SCX_OperatingSystem (Taken from PG_OperatingSystem)                 */
        /*===================================================================================*/

        SystemInfo sysInfo;
        if (sysInfo.GetNativeBitSize(Aunsignedshort)) {
            std::wostringstream bitText;
            bitText << Aunsignedshort << L" bit";

            SCXProperty prop(L"OperatingSystemCapability", bitText.str());
            inst.AddProperty(prop);
        }
        if (osinst->GetSystemUpTime(Ascxulong)){
            SCXProperty prop(L"SystemUpTime", Ascxulong);
            inst.AddProperty(prop);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instance names

       \param[in]   callContext Context details of this request
       \param[out]  names       Collection of instances with key properties

    */
    void OSProvider::DoEnumInstanceNames(const SCXCallContext& /* callContext */,
                                         SCXInstanceCollection &names)
    {
        SCX_LOGTRACE(m_log, L"OSProvider DoEnumInstanceNames");

        // There is only one instance.
        if (m_osEnum->GetTotalInstance() != 0)
        {
            SCXInstance inst;
            AddKeys(m_osEnum->GetTotalInstance(), inst);
            names.AddInstance(inst);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instances

       \param[in]     callContext                 Details of the client request
       \param[out]    instances                   Collection of instances

       \throws        SCXInternalErrorException   If instances in list are not CPUInstance

    */
    void OSProvider::DoEnumInstances(const SCXCallContext& /* callContext */,
                                     SCXInstanceCollection &instances)
    {
        SCX_LOGTRACE(m_log, L"OSProvider DoEnumInstances");

        // Refresh the collection
        m_osEnum->Update();
        m_memEnum->Update();

        // There should be only one instance.
        if (m_osEnum->GetTotalInstance() != 0)
        {
            SCXInstance inst;
            AddKeys(m_osEnum->GetTotalInstance(), inst);
            AddProperties(m_osEnum->GetTotalInstance(),
                          m_memEnum->GetTotalInstance(),
                          inst);
            instances.AddInstance(inst);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get an instance

       \param[in]   callContext Context of the original request, indicating which instance to retrieve.
       \param[out]  instance    The returned instance

       \throws     SCXInvalidArgumentException  If no Name property in keys
       \throws     SCXInternalErrorException    If instances in list are not CPUInstance
    */
    void OSProvider::DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"OSProvider::DoGetInstance()");

        // Refresh the collection
        m_osEnum->Update();
        m_memEnum->Update();

        const SCXProperty& nameprop = GetKeyRef(L"Name", callContext.GetObjectPath());

        if (0 == nameprop.GetStrValue().size())
        {
            throw SCXInvalidArgumentException(L"callContext", L"Empty Name property found", SCXSRCLOCATION);
        }

        // There should be only one instance.
        if (m_osEnum->GetTotalInstance() != 0)
        {
            AddKeys(m_osEnum->GetTotalInstance(), instance);
            AddProperties(m_osEnum->GetTotalInstance(),
                          m_memEnum->GetTotalInstance(),
                          instance);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Dump object as string (for logging).

       \returns       The object represented as a string suitable for logging.

    */
    const std::wstring OSProvider::DumpString() const
    {
        return L"OSProvider";
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

