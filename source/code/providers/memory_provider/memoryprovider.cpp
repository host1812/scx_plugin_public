/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Main implementation file for Memory Provider

    \date      2007-07-04 09:35:36


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxmath.h>

#include <scxproviderlib/scxprovidercapabilities.h>

#include "../meta_provider/startuplog.h"

#include "memoryprovider.h"

#include <scxsystemlib/memoryenumeration.h>
#include <scxsystemlib/memoryinstance.h>

using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;

namespace SCXCore {

    /*----------------------------------------------------------------------------*/
    /**
       Provide CMPI interface for this class

       The class implementation (concrete class) is MemoryProvider and the name of the
       provider in CIM registration terms is SCX_MemoryProvider.

    */
    SCXProviderDef(MemoryProvider, SCX_MemoryProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       The Singleton thread lock will be held during this call.

    */
    MemoryProvider::MemoryProvider() :
        BaseProvider(L"scx.core.providers.memoryprovider"), m_memEnum(NULL)
    {
        LogStartup();
        SCX_LOGTRACE(m_log, L"MemoryProvider constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    MemoryProvider::~MemoryProvider()
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
    void MemoryProvider::DoInit()
    {
        SCX_LOGTRACE(m_log, L"MemoryProvider::DoInit");

        if (m_memEnum != NULL)
        {
            SCXASSERTFAIL(L"DoInit() called multiple times without a call to DoCleanup() between");
            DoCleanup();
        }

        m_ProviderCapabilities.RegisterCimClass(eSCX_MemoryStatisticalInformation,
                                                L"SCX_MemoryStatisticalInformation");

        m_memEnum = new MemoryEnumeration();
        m_memEnum->Init();
    }


    /*----------------------------------------------------------------------------*/
    /**
        Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void MemoryProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"MemoryProvider::DoCleanup");

        m_ProviderCapabilities.Clear();

        if (m_memEnum != NULL)
        {
            m_memEnum->CleanUp();
            m_memEnum = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set the keys of the SCXInstance object from the information in the entity instance

       \param[in]       meminst   Internal instance representation to get data from
       \param[out]      inst      Collection to add instance to

       \throws          SCXInvalidArgumentException  The instance can not be converted to a memory instance

       This method contains knowledge on which are the key fields for the class.
       The key properties are defined in the MOF file.

    */
    void MemoryProvider::AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::MemoryInstance> meminst, SCXInstance &inst) const // private
    {
        SCX_LOGTRACE(m_log, L"MemoryProvider::AddKeys()");

        if (meminst == NULL)
        {
            throw SCXInvalidArgumentException(L"einst", L"Not a MemoryInstance", SCXSRCLOCATION);
        }

        // Set all the properties which are key properties to the SCXInstance
        SCXProperty name_prop(L"Name", L"Memory");
        inst.AddKey(name_prop);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set all properties of the SCXInstance from the information in the EntityInstance

        \param[in]       meminst  Internal instance representation to get data from
        \param[out]      inst     Instance to populate

        throws           SCXInvalidArgumentException The instance can not be converted to a memory instance

        This method knows how to map the values of the internal instance representation
        to the CMPI class definition.

    */
    void MemoryProvider::AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::MemoryInstance> meminst, SCXInstance &inst) const // private
    {
        scxulong physmem = 0;
        scxulong resmem = 0;
        scxulong usermem = 0;
        scxulong data = 0;
        scxulong data2 = 0;

        SCX_LOGTRACE(m_log, L"MemoryProvider::AddPropeties()");

        if (meminst == NULL)
        {
            throw SCXInvalidArgumentException(L"einst", L"Not an MemoryInstance", SCXSRCLOCATION);
        }

        SCXProperty total_prop(L"IsAggregate", meminst->IsTotal());
        inst.AddProperty(total_prop);

        if (meminst->GetTotalPhysicalMemory(physmem))
        {
            // If availble, get memory unavailable for user processes and remove it from physical memory
            usermem = physmem - (meminst->GetReservedMemory(resmem) ? resmem : 0);
        }

        if (meminst->GetAvailableMemory(data))
        {
            SCXProperty data_prop(L"AvailableMemory", data);
            inst.AddProperty(data_prop);

            // If we have a number for physical memory use it to compute a percentage
            if (usermem > 0)
            {
                // Need an unsigned char to send to the SCXProperty since this is what
                // is required by the MOF.
                unsigned char percent = static_cast<unsigned char> (GetPercentage(0, data, 0, usermem));
                SCXProperty data_prop2(L"PercentAvailableMemory", percent);
                inst.AddProperty(data_prop2);
            }
        }

        if (meminst->GetUsedMemory(data))
        {
            SCXProperty data_prop(L"UsedMemory", data);
            inst.AddProperty(data_prop);

            // If we have a number for physical memory use it to compute a percentage
            if (usermem > 0)
            {
                unsigned char percent = static_cast<unsigned char> (GetPercentage(0, data, 0, usermem));
                SCXProperty data_prop2(L"PercentUsedMemory", percent);
                inst.AddProperty(data_prop2);
            }
        }

        bool pageReadsAvailable = meminst->GetPageReads(data);
        bool pageWritesAvailable = meminst->GetPageWrites(data2);
        if (pageReadsAvailable && pageWritesAvailable)
        {
            SCXProperty data_prop(L"PagesPerSec", data + data2);
            inst.AddProperty(data_prop);
        }
        if (pageReadsAvailable)
        {
            SCXProperty data_prop(L"PagesReadPerSec", data);
            inst.AddProperty(data_prop);
        }
        if (pageWritesAvailable)
        {
            SCXProperty data_prop(L"PagesWrittenPerSec", data2);
            inst.AddProperty(data_prop);
        }
        if (meminst->GetAvailableSwap(data))
        {
            SCXProperty data_prop(L"AvailableSwap", data);
            inst.AddProperty(data_prop);

            if (meminst->GetTotalSwap(data2))
            {
                unsigned char percent = static_cast<unsigned char> (GetPercentage(0, data, 0, data2));
                SCXProperty data_prop2(L"PercentAvailableSwap", percent);
                inst.AddProperty(data_prop2);
            }
        }
        if (meminst->GetUsedSwap(data))
        {
            SCXProperty data_prop(L"UsedSwap", data);
            inst.AddProperty(data_prop);

            if (meminst->GetTotalSwap(data2))
            {
                unsigned char percent = static_cast<unsigned char> (GetPercentage(0, data, 0, data2));
                SCXProperty data_prop2(L"PercentUsedSwap", percent);
                inst.AddProperty(data_prop2);
            }
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instance names

       \param[in]   callContext Context details of this request
       \param[out]  names       Collection of instances with key properties

    */
    void MemoryProvider::DoEnumInstanceNames(const SCXCallContext& /* callContext */,
                                             SCXInstanceCollection &names)
    {
        SCX_LOGTRACE(m_log, L"MemoryProvider DoEnumInstanceNames");

        // There should be only one instance.
        if (m_memEnum->GetTotalInstance() != 0)
        {
            SCXInstance inst;
            AddKeys(m_memEnum->GetTotalInstance(), inst);
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
    void MemoryProvider::DoEnumInstances(const SCXCallContext& /* callContext */,
                                         SCXInstanceCollection &instances)
    {
        SCX_LOGTRACE(m_log, L"MemoryProvider DoEnumInstances");

        // Update memory PAL instance.
        m_memEnum->Update();

        // There should be only one instance.
        if (m_memEnum->GetTotalInstance()!=0)
        {
            SCXInstance inst;
            AddKeys(m_memEnum->GetTotalInstance(), inst);
            AddProperties(m_memEnum->GetTotalInstance(), inst);
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
    void MemoryProvider::DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"MemoryProvider::DoGetInstance()");

        // Refresh the collection
        m_memEnum->Update();

        const SCXInstance& keys = callContext.GetObjectPath();
        ValidateKeyValue(L"Name", keys, L"Memory");

        // There should be only one instance.
        if (m_memEnum->GetTotalInstance()!=0)
        {
            AddKeys(m_memEnum->GetTotalInstance(), instance);
            AddProperties(m_memEnum->GetTotalInstance(), instance);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns       The object represented as a string suitable for logging.

    */
    const std::wstring MemoryProvider::DumpString() const
    {
        return L"MemoryProvider";
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

