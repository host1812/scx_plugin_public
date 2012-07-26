/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     CPU Provider implementation

    \date      07-05-16 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>

#include <scxproviderlib/scxprovidercapabilities.h>

#include "cpuprovider.h"

#include <scxsystemlib/cpuenumeration.h>
#include <scxcorelib/scxlog.h>

#include "../meta_provider/startuplog.h"

using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;

namespace SCXCore {

    /*----------------------------------------------------------------------------*/
    /**
       Provide CMPI interface for this class

       The class implementation (concrete class) is CPUProvider and the name of the
       provider in CIM registration terms is SCX_CPUProvider.

    */
    SCXProviderDef(CPUProvider, SCX_CPUProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       The Singleton thread lock will be held during this call.

    */
    CPUProvider::CPUProvider() :
        BaseProvider(L"scx.core.providers.cpuprovider"), m_cpus(NULL)
    {
        LogStartup();
        SCX_LOGTRACE(m_log, L"CPUProvider constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    CPUProvider::~CPUProvider()
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
    void CPUProvider::DoInit()
    {
        if (m_cpus != NULL)
        {
            SCXASSERTFAIL(L"DoInit() called multiple times without a call to DoCleanup() between");
            DoCleanup();
        }

        m_ProviderCapabilities.RegisterCimClass(eSCX_ProcessorStatisticalInformation,
                                                L"SCX_ProcessorStatisticalInformation");

        m_cpus = new CPUEnumeration();
        m_cpus->Init();
    }


    /*----------------------------------------------------------------------------*/
    /**
        Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void CPUProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"CPUProvider::DoCleanup");
        m_ProviderCapabilities.Clear();
        if (m_cpus != NULL)
        {
            m_cpus->CleanUp();
            m_cpus = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Add a SCXInstance with the name property set frmo the CPUInstance to the collection

       \param[in]   cpuinst  CPU instance to get data from
       \param[out]  inst     Instance to add keys to

       \throws      SCXInvalidArgumentException - The instance can not be converted to a CPUInstance

       This method contains knowledge on which are the key fields for the class.
       The key properties are defined in the MOF file.

    */
    void CPUProvider::AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> cpuinst, SCXInstance &inst) // private
    {
        SCX_LOGTRACE(m_log, L"CPUProvider AddKeys()");

        if (cpuinst == NULL)
        {
            throw SCXInvalidArgumentException(L"einst", L"Not a CPUInstance", SCXSRCLOCATION);
        }

        SCXProperty name_prop(L"Name", cpuinst->GetProcName());
        inst.AddKey(name_prop);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set all properties from the CPUInstance in the SCXInstance

       \param[in]  cpuinst - CPU instance to get data from
       \param[in]  inst    - Instance to populate

       \throws      SCXInvalidArgumentException - If the instance can not be converted to a CPUInstance

       This method knows how to map the values of the CPU PAL to the CMPI class
       definition.

    */
    void CPUProvider::AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> cpuinst, SCXInstance &inst) // private
    {
        scxulong data;

        if (cpuinst == NULL)
        {
            throw SCXInvalidArgumentException(L"einst", L"Not a CPUInstance", SCXSRCLOCATION);
        }

        SCX_LOGTRACE(m_log, L"CPUProvider AddPropeties()");

        SCXProperty total_prop(L"IsAggregate", cpuinst->IsTotal());
        inst.AddProperty(total_prop);

        if (cpuinst->GetProcessorTime(data))
        {
            SCXProperty data_prop(L"PercentProcessorTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
        }

        if (cpuinst->GetIdleTime(data))
        {
            SCXProperty data_prop(L"PercentIdleTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
        }

        if (cpuinst->GetUserTime(data))
        {
            SCXProperty data_prop(L"PercentUserTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
        }

        if (cpuinst->GetNiceTime(data))
        {
            SCXProperty data_prop(L"PercentNiceTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
        }

        if (cpuinst->GetPrivilegedTime(data))
        {
            SCXProperty data_prop(L"PercentPrivilegedTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
        }

        if (cpuinst->GetIowaitTime(data))
        {
            SCXProperty data_prop(L"PercentIOWaitTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
        }

        if (cpuinst->GetInterruptTime(data))
        {
            SCXProperty data_prop(L"PercentInterruptTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
        }

        if (cpuinst->GetDpcTime(data))
        {
            SCXProperty data_prop(L"PercentDPCTime", static_cast<unsigned char> (data));
            inst.AddProperty(data_prop);
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
    SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> CPUProvider::FindInstance(const SCXInstance& keys) const // private
    {
        // Start by extracting all key properties
        const SCXProperty& nameprop = GetKeyRef(L"Name", keys);

        for(size_t i=0; i<m_cpus->Size(); i++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> testinst = m_cpus->GetInstance(i);

            // Compare key values of input args and the current instance
            if (testinst->GetProcName() == nameprop.GetStrValue())
            {
                // Match
                return testinst;
            }
        }

        // As last resort, check if we the request is for the _Total instance
        if (m_cpus->GetTotalInstance() != 0 )
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> testinst = m_cpus->GetTotalInstance();

            if (testinst->GetProcName() == nameprop.GetStrValue())
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
    void CPUProvider::DoEnumInstanceNames(const SCXCallContext& /* callContext */,
                                          SCXInstanceCollection &names)
    {
        SCX_LOGTRACE(m_log, L"CPUProvider DoEnumInstanceNames");

        m_cpus->Update(false);

        SCX_LOGTRACE(m_log, StrAppend(L"Number of CPUs = ", m_cpus->Size()));

        for(size_t i=0; i<m_cpus->Size(); i++)
        {
            // For each CPU instance, create an SCXInstance and add it to the
            // names collection.
            SCXInstance inst;
            AddKeys(m_cpus->GetInstance(i), inst);
            names.AddInstance(inst);
        }

        if (m_cpus->GetTotalInstance() != 0 )
        {
            // There will always be one total instance
            SCXInstance inst;
            AddKeys(m_cpus->GetTotalInstance(), inst);
            names.AddInstance(inst);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instances

       \param[in]   callContext The context of original client request
       \param[out]  instances   Collection of instances

    */
    void CPUProvider::DoEnumInstances(const SCXCallContext& /* callContext */,
                                      SCXInstanceCollection &instances)
    {
        SCX_LOGTRACE(m_log, L"CPUProvider DoEnumInstances");

        // Update CPU PAL instance. This is both update of number of CPUs and
        // current statistics for each CPU.
        m_cpus->Update();

        SCX_LOGTRACE(m_log, StrAppend(L"Number of CPUs = ", m_cpus->Size()));

        for(size_t i=0; i<m_cpus->Size(); i++)
        {
            SCXInstance inst;
            AddKeys(m_cpus->GetInstance(i), inst);
            AddProperties(m_cpus->GetInstance(i), inst);
            instances.AddInstance(inst);
        }

        if (m_cpus->GetTotalInstance() != 0 )
        {
            SCXInstance inst;
            AddKeys(m_cpus->GetTotalInstance(), inst);
            AddProperties(m_cpus->GetTotalInstance(), inst);
            instances.AddInstance(inst);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get an instance

       \param[in]   callContext Context of original request, indicating which instance to retrieve
       \param[out]  instance    The selected instance

       \throws      SCXInvalidArgumentException  If no Name property in keys
       \throws      SCXInternalErrorException    If instances in list are not CPUInstance
    */
    void CPUProvider::DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"CPUProvider::DoGetInstance()");

        // Refresh the collection (both keys and current data)
        m_cpus->Update();

        SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> testinst = FindInstance(callContext.GetObjectPath());

        // If we get here whithout exception we got a match - set keys and properties,
        // the instance is returned as out value
        AddKeys(testinst, instance);
        AddProperties(testinst, instance);

        // All done, simply return
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/


