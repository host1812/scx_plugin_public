/*----------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Process Provider implementation

    \date      07-11-05 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>

#include <scxproviderlib/scxprovidercapabilities.h>

#include "processprovider.h"
#include "../meta_provider/startuplog.h"


#include <scxsystemlib/processenumeration.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>

#include <sstream>
#include <algorithm>
#include <vector>

using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;

namespace SCXCore {

    /*----------------------------------------------------------------------------*/
    /**
       Provide CMPI interface for this class

       The class implementation (concrete class) is ProcessProvider and the name of the
       provider in CIM registration terms is SCX_ProcessProvider.

    */
    SCXProviderDef(ProcessProvider, SCX_ProcessProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       The Singleton thread lock will be held during this call.

    */
    ProcessProvider::ProcessProvider() :
        BaseProvider(L"scx.core.providers.processprovider"), m_processes(NULL)
    {
        LogStartup();

        SCX_LOGTRACE(m_log, L"ProcessProvider constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    ProcessProvider::~ProcessProvider()
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
    void ProcessProvider::DoInit()
    {
        SCX_LOGTRACE(m_log, L"ProcessProvider::DoInit");

        if (m_processes != NULL)
        {
            SCXASSERTFAIL(L"DoInit() called multiple times without a call to DoCleanup() between");
            DoCleanup();
        }

        m_processes = new ProcessEnumeration();
        m_processes->Init();

        m_ProviderCapabilities.RegisterCimClass(eSCX_UnixProcess,
                                                L"SCX_UnixProcess");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_UnixProcess, eTopResourceConsumerMethod,
                                                 L"TopResourceConsumers");
        m_ProviderCapabilities.RegisterCimClass(eSCX_UnixProcessStatisticalInformation,
                                                L"SCX_UnixProcessStatisticalInformation");
    }

    /*----------------------------------------------------------------------------*/
    /**
        Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void ProcessProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"ProcessProvider::DoCleanup");

        m_ProviderCapabilities.Clear();

        if (m_processes != NULL)
        {
            m_processes->CleanUp();
            m_processes = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Add a SCXInstance with the name property set frmo the ProcessInstance to the collection

       \param[in]   processinst    Process instance to get data from
       \param[out]  inst           Instance to add keys to
       \param[in]   cimtype        Type of CIM Class to return

       \throws      SCXInvalidArgumentException - The instance can not be converted to a ProcessInstance

       This method contains knowledge on which are the key fields for the class.
       The key properties are defined in the MOF file.

    */
    void ProcessProvider::AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> processinst, SCXInstance &inst, SupportedCimClasses cimtype) // private
    {
        SCX_LOGTRACE(m_log, L"ProcessProvider AddKeys()");

        if (processinst == NULL)
        {
            throw SCXInvalidArgumentException(L"einst", L"Not a ProcessInstance", SCXSRCLOCATION);
        }

        scxulong pid;
        if (processinst->GetPID(pid))
        {
            SCXProperty pid_prop(L"Handle", StrFrom(pid));
            inst.AddKey(pid_prop);
        }

        AddScopingOperatingSystemKeys(inst);
        if (eSCX_UnixProcessStatisticalInformation == cimtype)
        {
            std::string name;
            if (processinst->GetName(name))
            {
                SCXProperty name_prop(L"Name", StrFromMultibyte(name));
                inst.AddKey(name_prop);
                SCXProperty creationClass_prop(L"ProcessCreationClassName", L"SCX_UnixProcessStatisticalInformation");
                inst.AddKey(creationClass_prop);
            }
        }
        else if (eSCX_UnixProcess == cimtype)
        {
            SCXProperty creationClass_prop(L"CreationClassName", L"SCX_UnixProcess");
            inst.AddKey(creationClass_prop);
        }

    }

    /*----------------------------------------------------------------------------*/
    /**
       Set all properties from the ProcessInstance in the SCXInstance

       \param[in]  processinst  - Process instance to get data from
       \param[in]  inst         - Instance to populate
       \param[in]  cimtype      - Type of CIM Class to return

       \throws      SCXInvalidArgumentException - If the instance can not be converted to a ProcessInstance

       This method knows how to map the values of the Process PAL to the CMPI class
       definition.

    */
    void ProcessProvider::AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> processinst, SCXInstance &inst, SupportedCimClasses cimtype) // private
    {
        if (processinst == NULL)
        {
            throw SCXInvalidArgumentException(L"einst", L"Not a ProcessInstance", SCXSRCLOCATION);
        }

        SCX_LOGTRACE(m_log, L"ProcessProvider AddPropeties()");

        if (eSCX_UnixProcessStatisticalInformation == cimtype)
        {
            unsigned int uint = 0;
            scxulong ulong = 0;

            if (processinst->GetRealData(ulong))
            {
                SCXProperty prop(L"RealData", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetRealStack(ulong))
            {
                SCXProperty prop(L"RealStack", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetVirtualText(ulong))
            {
                SCXProperty prop(L"VirtualText", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetVirtualData(ulong))
            {
                SCXProperty prop(L"VirtualData", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetVirtualStack(ulong))
            {
                SCXProperty prop(L"VirtualStack", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetVirtualMemoryMappedFileSize(ulong))
            {
                SCXProperty prop(L"VirtualMemoryMappedFileSize", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetVirtualSharedMemory(ulong))
            {
                SCXProperty prop(L"VirtualSharedMemory", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetCpuTimeDeadChildren(ulong))
            {
                SCXProperty prop(L"CpuTimeDeadChildren", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetSystemTimeDeadChildren(ulong))
            {
                SCXProperty prop(L"SystemTimeDeadChildren", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetRealText(ulong))
            {
                SCXProperty prop(L"RealText", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetCPUTime(uint))
            {
                SCXProperty prop(L"CPUTime", uint);
                inst.AddProperty(prop);
            }

            if (processinst->GetBlockWritesPerSecond(ulong))
            {
                SCXProperty prop(L"BlockWritesPerSecond", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetBlockReadsPerSecond(ulong))
            {
                SCXProperty prop(L"BlockReadsPerSecond", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetBlockTransfersPerSecond(ulong))
            {
                SCXProperty prop(L"BlockTransfersPerSecond", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetPercentUserTime(ulong))
            {
                SCXProperty prop(L"PercentUserTime", (unsigned char) ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetPercentPrivilegedTime(ulong))
            {
                SCXProperty prop(L"PercentPrivilegedTime", (unsigned char) ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetUsedMemory(ulong))
            {
                SCXProperty prop(L"UsedMemory", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetPercentUsedMemory(ulong))
            {
                SCXProperty prop(L"PercentUsedMemory", (unsigned char) ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetPagesReadPerSec(ulong))
            {
                SCXProperty prop(L"PagesReadPerSec", ulong);
                inst.AddProperty(prop);
            }
        }
        else if (eSCX_UnixProcess == cimtype)
        {
            std::string name("");
            std::vector<std::string> params;
            std::wstring str(L"");
            unsigned int uint = 0;
            unsigned short ushort = 0;
            scxulong ulong = 0;
            SCXCoreLib::SCXCalendarTime ctime;
            int pid = 0;

            if (processinst->GetOtherExecutionDescription(str))
            {
                SCXProperty prop(L"OtherExecutionDescription", str);
                inst.AddProperty(prop);
            }

            if (processinst->GetKernelModeTime(ulong))
            {
                SCXProperty prop(L"KernelModeTime", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetUserModeTime(ulong))
            {
                SCXProperty prop(L"UserModeTime", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetWorkingSetSize(ulong))
            {
                SCXProperty prop(L"WorkingSetSize", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetProcessSessionID(ulong))
            {
                SCXProperty prop(L"ProcessSessionID", ulong);
                inst.AddProperty(prop);
            }

            if (processinst->GetProcessTTY(name))
            {
                SCXProperty prop(L"ProcessTTY", StrFromMultibyte(name));
                inst.AddProperty(prop);
            }

            if (processinst->GetModulePath(name))
            {
                SCXProperty prop(L"ModulePath", StrFromMultibyte(name));
                inst.AddProperty(prop);
            }

            if (processinst->GetParameters(params))
            {
                std::vector<SCXProperty> props;
                for (std::vector<std::string>::const_iterator iter = params.begin();
                     iter != params.end(); ++iter)
                {
                    SCXProperty item(L"", StrFromMultibyte(*iter));
                    props.push_back(item);
                }
                SCXProperty prop(L"Parameters", props);
                inst.AddProperty(prop);
            }

            if (processinst->GetProcessWaitingForEvent(name))
            {
                SCXProperty prop(L"ProcessWaitingForEvent", StrFromMultibyte(name));
                inst.AddProperty(prop);
            }

            if (processinst->GetName(name))
            {
                SCXProperty name_prop(L"Name", StrFromMultibyte(name));
                inst.AddProperty(name_prop);
            }

            if (processinst->GetPriority(uint))
            {
                SCXProperty prio_prop(L"Priority", uint);
                inst.AddProperty(prio_prop);
            }

            if (processinst->GetExecutionState(ushort))
            {
                SCXProperty state_prop(L"ExecutionState", ushort);
                inst.AddProperty(state_prop);
            }

            if (processinst->GetCreationDate(ctime))
            {
                SCXProperty cdate_prop(L"CreationDate", ctime);
                inst.AddProperty(cdate_prop);
            }

            if (processinst->GetTerminationDate(ctime))
            {
                SCXProperty edate_prop(L"TerminationDate", ctime);
                inst.AddProperty(edate_prop);
            }

            if (processinst->GetParentProcessID(pid))
            {
                SCXProperty ppid_prop(L"ParentProcessID", StrFrom(pid));
                inst.AddProperty(ppid_prop);
            }

            if (processinst->GetRealUserID(ulong))
            {
                SCXProperty user_prop(L"RealUserID", ulong);
                inst.AddProperty(user_prop);
            }

            if (processinst->GetProcessGroupID(ulong))
            {
                SCXProperty group_prop(L"ProcessGroupID", ulong);
                inst.AddProperty(group_prop);
            }

            if (processinst->GetProcessNiceValue(uint))
            {
                SCXProperty nice_prop(L"ProcessNiceValue", uint);
                inst.AddProperty(nice_prop);
            }
        }

    }

    /*----------------------------------------------------------------------------*/
    /**
      Lookup the instance representation, given keys provided from CIMOM

      \param[in]    keys   SCXInstance with property keys set
      \returns             Pointer to located instance

      \throws              SCXInvalidArgumentException
      \throws              SCXInternalErrorException
      \throws              SCXCIMInstanceNotFound   The instance with given keys cannot be found

      This method knows which the key properties of the entity are and returns
      pointer to that item if found.

    */
    SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> ProcessProvider::FindInstance(const SCXInstance& keys) const // private
    {
        // Start by extracting all key properties
        ValidateScopingOperatingSystemKeys(keys);
        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(keys));
        if (eSCX_UnixProcessStatisticalInformation == cimtype) {
            ValidateKeyValue(L"ProcessCreationClassName", keys, L"SCX_UnixProcessStatisticalInformation");
            GetKeyRef(L"Name", keys);
        } else if (eSCX_UnixProcess == cimtype) {
            ValidateKeyValue(L"CreationClassName", keys, L"SCX_UnixProcess");
        } else {
            throw SCXInvalidStateException(L"Unknown cimtype value", SCXSRCLOCATION);
        }

        const SCXProperty &pidprop = GetKeyRef(L"Handle", keys);
        scxulong pid;

        for(size_t i=0; i<m_processes->Size(); i++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> testinst = m_processes->GetInstance(i);
            if (testinst == NULL)
            {
                throw SCXInternalErrorException(L"Instance from list not an ProcessInstance", SCXSRCLOCATION);
            }
            // Compare key values of input args and the current instance
            testinst->GetPID(pid);
            if (StrFrom(pid) == pidprop.GetStrValue())
            {
                // Match
                return testinst;
            }
        }

        // As last resort, check if we the request is for the _Total instance
        if (m_processes->GetTotalInstance() != 0)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> testinst = m_processes->GetTotalInstance();
            if (testinst == NULL)
            {
                throw SCXInternalErrorException(L"Total instance not a ProcessInstance", SCXSRCLOCATION);
            }
            testinst->GetPID(pid);
            if (StrFrom(pid) == pidprop.GetStrValue())
            {
                return testinst;
            }
        }


        throw SCXCIMInstanceNotFound(keys.DumpString(), SCXSRCLOCATION);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instance names

       \param[in]   callContext Details of original request
       \param[out]  names       Collection of instances with key properties

    */
    void ProcessProvider::DoEnumInstanceNames(const SCXCallContext& callContext, SCXInstanceCollection &/*names*/)
    {
        SCX_LOGTRACE(m_log, L"ProcessProvider DoEnumInstanceNames");

        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        SCXCoreLib::SCXThreadLock lock(m_processes->GetLockHandle());

        m_processes->UpdateNoLock(lock, false);

        SCX_LOGTRACE(m_log, StrAppend(L"Number of Processes = ", m_processes->Size()));

        for(size_t i=0; i<m_processes->Size(); i++)
        {
            SCXInstance inst;
            AddKeys(m_processes->GetInstance(i), inst, cimtype);

            // Fix for WI 17483:
            //
            // Use SendInstance() rather than building a vector of items
            SCXASSERT( SupportsSendInstance() );
            SendInstanceName(inst);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instances

       \param[in]   callContext The context of original client request
       \param[out]  instances   Collection of instances

    */
    void ProcessProvider::DoEnumInstances(const SCXCallContext& callContext, SCXInstanceCollection &/*instances*/)
    {
        SCX_LOGTRACE(m_log, L"ProcessProvider DoEnumInstances");

        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        SCXCoreLib::SCXThreadLock lock(m_processes->GetLockHandle());

        // Update Process PAL instance. This is both update of number of Processes and
        // current statistics for each Process.
        m_processes->UpdateNoLock(lock);

        SCX_LOGTRACE(m_log, StrAppend(L"Number of Processes = ", m_processes->Size()));

        for(size_t i=0; i<m_processes->Size(); i++)
        {
            SCXInstance inst;
            AddKeys(m_processes->GetInstance(i), inst, cimtype);
            AddProperties(m_processes->GetInstance(i), inst, cimtype);

            // Fix for WI 17483:
            //
            // Use SendInstance() rather than building a vector of items
            SCXASSERT( SupportsSendInstance() );
            SendInstance(inst);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get an instance

       \param[in]   callContext Context of original request, indicating which instance to retrieve
       \param[out]  instance    The selected instance

       \throws      SCXInvalidArgumentException  If no Name property in keys
       \throws      SCXInternalErrorException    If instances in list are not ProcessInstance
    */
    void ProcessProvider::DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"ProcessProvider::DoGetInstance()");

        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        SCXCoreLib::SCXThreadLock lock(m_processes->GetLockHandle());

        // Refresh the collection (both keys and current data)
        m_processes->UpdateNoLock(lock);

        SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> testinst = FindInstance(callContext.GetObjectPath());

        // If we get here without exception we got a match - set keys and properties,
        // the instance is returned as out value
        AddKeys(testinst, instance, cimtype);
        AddProperties(testinst, instance, cimtype);

        // All done, simply return
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the value for the spcified resource from a specified instance

        \param[in]     resource      Name of resource to get
        \param[in]     processinst   Instance to get resource from

        \returns       Value for specifed resource

        \throws        SCXInternalErrorException    If given resource not handled
    */
    scxulong ProcessProvider::GetResource(const std::wstring &resource, SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> processinst)
    {
        scxulong res = 0;
        bool gotResource = false;

        if (StrCompare(resource, L"CPUTime", true) == 0)
        {
            unsigned int cputime;
            gotResource = processinst->GetCPUTime(cputime);
            res = static_cast<scxulong>(cputime);
        }
        else if (StrCompare(resource, L"BlockReadsPerSecond", true) == 0)
        {
            gotResource = processinst->GetBlockReadsPerSecond(res);
        }
        else if (StrCompare(resource, L"BlockWritesPerSecond", true) == 0)
        {
            gotResource = processinst->GetBlockWritesPerSecond(res);
        }
        else if (StrCompare(resource, L"BlockTransfersPerSecond", true) == 0)
        {
            gotResource = processinst->GetBlockTransfersPerSecond(res);
        }
        else if (StrCompare(resource, L"PercentUserTime", true) == 0)
        {
            gotResource = processinst->GetPercentUserTime(res);
        }
        else if (StrCompare(resource, L"PercentPrivilegedTime", true) == 0)
        {
            gotResource = processinst->GetPercentPrivilegedTime(res);
        }
        else if (StrCompare(resource, L"UsedMemory", true) == 0)
        {
            gotResource = processinst->GetUsedMemory(res);
        }
        else if (StrCompare(resource, L"PercentUsedMemory", true) == 0)
        {
            gotResource = processinst->GetPercentUsedMemory(res);
        }
        else if (StrCompare(resource, L"PagesReadPerSec", true) == 0)
        {
            gotResource = processinst->GetPagesReadPerSec(res);
        }
        else
        {
            throw UnknownResourceException(resource, SCXSRCLOCATION);
        }

        if ( ! gotResource)
        {
            throw SCXInternalErrorException(StrAppend(L"GetResource: Failed to get resouce: ", resource), SCXSRCLOCATION);
        }

        return res;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Struct used for sorting on a specific value
    */
    struct ProcessInstanceSort {
        ProcessInstanceSort() : procinst(0), value(0){}

        //! Pointer to instance containing all values
        SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> procinst;
        //! Copy of value to sort on
        scxulong value;
    };

    /*----------------------------------------------------------------------------*/
    /**
        Compare two ProcessInstanceSort structs

        \param[in]     p1   First struct to comapare
        \param[in]     p2   Second struct to compare

        \returns       true if value in p1 is greater then in p2
    */
    static bool CompareProcSort(ProcessInstanceSort p1, ProcessInstanceSort p2)
    {
        return p1.value > p2.value;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the processes that are the top conumers of a specified resource

        \param[in]     resource    Name of resource to get top consumers for
        \param[in]     count       Numer of processes to return in top list
        \param[out]    result      Result string

    */
    void ProcessProvider::GetTopResourceConsumers(const std::wstring &resource, unsigned int count, std::wstring &result)
    {
        SCX_LOGTRACE(m_log, L"SCXProcessProvider GetTopResourceConsumers");

        std::wstringstream ss;
        std::vector<ProcessInstanceSort> procsort;

        SCXCoreLib::SCXThreadLock lock(m_processes->GetLockHandle());

        m_processes->UpdateNoLock(lock);

        // Build separate vector for sorting
        for(size_t i=0; i<m_processes->Size(); i++)
        {
            ProcessInstanceSort p;

            p.procinst = m_processes->GetInstance(i);
            p.value = GetResource(resource, p.procinst);
            procsort.push_back(p);
        }

        std::sort(procsort.begin(), procsort.end(), CompareProcSort);

        ss << std::endl << L"PID   Name                 " << resource << std::endl;
        ss << L"-------------------------------------------------------------" << std::endl;

        for(size_t i=0; i<procsort.size() && i<count; i++)
        {
            const ProcessInstanceSort* processinst = &procsort[i];

            scxulong pid;

            ss.width(5);
            if (processinst->procinst->GetPID(pid))
            {
                ss << pid;
            }
            else
            {
                ss << L"-----";
            }
            ss << L" ";

            std::string name;
            ss.setf(std::ios_base::left);
            ss.width(20);
            if (processinst->procinst->GetName(name))
            {
                ss << StrFromMultibyte(name);
            }
            else
            {
                ss << L"<unknown>";
            }
            ss.unsetf(std::ios_base::left);
            ss << L" ";

            ss.width(10);
            ss << processinst->value;

            ss << std::endl;
        }

        result = ss.str();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Invoke a method on an instance

        \param[in]     callContext Keys indicating instance to execute method on
        \param[in]     methodname  Name of method called
        \param[in]     args        Arguments provided for method call
        \param[out]    outargs     Output arguments - not used
        \param[out]    result      Result value

    */
    void ProcessProvider::DoInvokeMethod(
        const SCXCallContext& callContext,
        const std::wstring& methodname,
        const SCXArgs& args,
        SCXArgs& /*outargs*/,
        SCXProperty& result)
    {
        SCX_LOGTRACE(m_log, L"SCXProcessProvider DoInvokeMethod");

        SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        if (cimtype == eSCX_UnixProcess)
        {
            SupportedCimMethods cimmethod = static_cast<SupportedCimMethods>(m_ProviderCapabilities.GetCimMethodId(callContext.GetObjectPath(), methodname));

            if (cimmethod == eTopResourceConsumerMethod)
            {
                const SCXProperty* resource = args.GetProperty(L"resource");
                const SCXProperty* count = args.GetProperty(L"count");

                if (count == NULL || resource == NULL)
                {
                    throw SCXInternalErrorException(L"missing arguments to TopResourceConsumer method", SCXSRCLOCATION);
                }

                if (resource->GetType() != SCXProperty::SCXStringType || count->GetType() != SCXProperty::SCXUShortType)
                {
                    throw SCXInternalErrorException(L"Wrong type of arguments to TopResourceConsumer method", SCXSRCLOCATION);
                }

                std::wstring return_str;
                GetTopResourceConsumers(resource->GetStrValue(), count->GetUShortValue(), return_str);
                result.SetValue(return_str);
            }
            else
            {
                throw SCXInternalErrorException(StrAppend(L"Unhandled method name: ", methodname), SCXSRCLOCATION);
            }
        }
        else
        {
            throw SCXInternalErrorException(StrAppend(L"No methods on class: ", cimtype), SCXSRCLOCATION);
        }

    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

