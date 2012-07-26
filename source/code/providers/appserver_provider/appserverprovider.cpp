/*----------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Main implementation file for Application Server Provider

    \date      2011-05-09


*/
/*----------------------------------------------------------------------------*/

#include <errno.h>

#include <scxcorelib/scxcmn.h>

#include <fstream>
#include <sstream>

#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>
#include "appserverprovider.h"

#include "buildversion.h"

using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;
using namespace std;


namespace SCXCore {

    /**
       Returns the appserver enumerator
    */
    SCXHandle<AppServerEnumeration> AppServerProviderPALDependencies::CreateEnum()
    {
        return SCXHandle<AppServerEnumeration>(new AppServerEnumeration());
    }

    /*----------------------------------------------------------------------------*/
    /**
       Provide CMPI interface for this class

       The class implementation (concrete class) is ASProvider and the name of the
       provider in CIM registration terms is SCX_ASProvider.

    */
    SCXProviderDef(ASProvider, SCX_ASProvider)

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor
    */
    ASProvider::ASProvider(SCXCoreLib::SCXHandle<AppServerProviderPALDependencies> deps) :
        BaseProvider(L"scx.core.providers.asprovider"), m_deps(deps), m_appservers(NULL)
    {
        SCX_LOGTRACE(m_log, L"ASProvider constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    ASProvider::~ASProvider()
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
    void ASProvider::DoInit()
    {
        SCX_LOGTRACE(m_log, L"ASProvider::DoInit");
        if (NULL != m_appservers)
        {
            SCXASSERTFAIL(L"DoInit() called multiple times without a call to DoCleanup() between");
            DoCleanup();
        }

        m_ProviderCapabilities.RegisterCimClass(eSCX_AS, L"SCX_Application_Server");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_AS, eSetDeepMonitoringMethod, L"SetDeepMonitoring");

        m_appservers = m_deps->CreateEnum();
        m_appservers->Init();
    }


    /*----------------------------------------------------------------------------*/
    /**
        Provide a way for pal layer to do cleanup. Stop all threads etc.
    */
    void ASProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"ASProvider::DoCleanup");

        m_ProviderCapabilities.Clear();

        if (NULL != m_appservers)
        {
            m_appservers->CleanUp();
            m_appservers = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set all keys in the SCXInstance

       \param[out]  inst  Instance to add keys to

    */
    void ASProvider::AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::AppServerInstance> asinst, SCXInstance &inst) // private
    {
        SCX_LOGTRACE(m_log, L"ASProvider AddKeys()");

        if (NULL == asinst)
        {
            throw SCXInvalidArgumentException(L"asinst", L"Not a AppServerInstance", SCXSRCLOCATION);
        }

        SCXProperty name_prop(L"Name", asinst->GetId());
        inst.AddKey(name_prop);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set all properties in the SCXInstance

       \param[in]  inst  Instance to populate

    */
    void ASProvider::AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::AppServerInstance> asinst, SCXInstance &inst) // private
    {
        if (NULL == asinst)
        {
            throw SCXInvalidArgumentException(L"asinst", L"Not a AppServerInstance", SCXSRCLOCATION);
        }

        SCX_LOGTRACE(m_log, L"ASProvider AddPropeties()");

        SCXProperty http_prop(L"HttpPort", asinst->GetHttpPort());
        inst.AddProperty(http_prop);

        SCXProperty https_prop(L"HttpsPort", asinst->GetHttpsPort());
        inst.AddProperty(https_prop);

        SCXProperty version_prop(L"Version", asinst->GetVersion());
        inst.AddProperty(version_prop);

        SCXProperty majorversion_prop(L"MajorVersion", asinst->GetMajorVersion());
        inst.AddProperty(majorversion_prop);

        SCXProperty port_prop(L"Port", asinst->GetPort());
        inst.AddProperty(port_prop);

        SCXProperty protocol_prop(L"Protocol", asinst->GetProtocol());
        inst.AddProperty(protocol_prop);

        SCXProperty diskpath_prop(L"DiskPath", asinst->GetDiskPath());
        inst.AddProperty(diskpath_prop);

        SCXProperty type_prop(L"Type", asinst->GetType());
        inst.AddProperty(type_prop);

        SCXProperty deep_prop(L"IsDeepMonitored", asinst->GetIsDeepMonitored());
        inst.AddProperty(deep_prop);

        SCXProperty run_prop(L"IsRunning", asinst->GetIsRunning());
        inst.AddProperty(run_prop);
        
        SCXProperty profile_prop(L"Profile", asinst->GetProfile());
        inst.AddProperty(profile_prop);

        SCXProperty cell_prop(L"Cell", asinst->GetCell());
        inst.AddProperty(cell_prop);

        SCXProperty node_prop(L"Node", asinst->GetNode());
        inst.AddProperty(node_prop);

        SCXProperty server_prop(L"Server", asinst->GetServer());
        inst.AddProperty(server_prop);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instance names

       \param[in]   callContext Context details of this request
       \param[out]  instances   Collection of instances with key properties

    */
    void ASProvider::DoEnumInstanceNames(const SCXCallContext& /*callContext*/,
                                           SCXInstanceCollection& instances)
    {
        SCX_LOGTRACE(m_log, L"ASProvider DoEnumInstanceNames");

        m_appservers->Update(false);

        SCX_LOGTRACE(m_log, StrAppend(L"Number of Application Servers = ", m_appservers->Size()));

        for(size_t i=0; i<m_appservers->Size(); i++)
        {
            // For each application server instance, create an SCXInstance and add it to the
            // names collection.
            SCXInstance inst;
            AddKeys(m_appservers->GetInstance(i), inst);
            instances.AddInstance(inst);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumerate instances

       \param[in]     callContext                 Details of the client request
       \param[out]    instances                   Collection of instances

    */
    void ASProvider::DoEnumInstances(const SCXCallContext& /*callContext*/,
                                       SCXInstanceCollection& instances)
    {
        SCX_LOGTRACE(m_log, L"ASProvider DoEnumInstances");

        m_appservers->Update();

        SCX_LOGTRACE(m_log, StrAppend(L"Number of application servers = ", m_appservers->Size()));

        for(size_t i=0; i<m_appservers->Size(); i++)
        {
            SCXInstance inst;
            AddKeys(m_appservers->GetInstance(i), inst);
            AddProperties(m_appservers->GetInstance(i), inst);
            instances.AddInstance(inst);
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
    SCXCoreLib::SCXHandle<SCXSystemLib::AppServerInstance> ASProvider::FindInstance(const SCXInstance& keys) const // private
    {
        // Start by extracting the key properties
        const SCXProperty& nameprop = GetKeyRef(L"Name", keys);

        SCXHandle<AppServerInstance> inst = m_appservers->GetInstance(nameprop.GetStrValue());

        if (NULL != inst)
        {
            return inst;
        }

        throw SCXCIMInstanceNotFound(keys.DumpString(), SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get an instance

       \param[in]   callContext Context of the original request, indicating which instance to retrieve.
       \param[out]  instance    The returned instance

    */
    void ASProvider::DoGetInstance(const SCXProviderLib::SCXCallContext& callContext, SCXProviderLib::SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"ASProvider DoGetInstance");

        // Refresh the collection (both keys and current data)
        m_appservers->Update();

        SCXCoreLib::SCXHandle<SCXSystemLib::AppServerInstance> testinst = FindInstance(callContext.GetObjectPath());

        // If we get here whithout exception we got a match - set keys and properties,
        // the instance is returned as out value
        AddKeys(testinst, instance);
        AddProperties(testinst, instance);

        // All done, simply return
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set the IsDeepMonitored property for an app server instance

        \param[in]     id    Id for the app server instance to change
        \param[in]     deep  value to change it to
        \returns             true if app server instance found, else false
    */
    bool ASProvider::SetDeepMonitoring(wstring id, bool deep, wstring protocol)
    {
        SCXHandle<AppServerInstance> inst = m_appservers->GetInstance(id);
        if (NULL != inst)
        {
            inst->SetIsDeepMonitored(deep, protocol);
            return true;
        }
        return false;
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
    void ASProvider::DoInvokeMethod(
        const SCXCallContext& callContext,
        const std::wstring& methodname,
        const SCXArgs& args,
        SCXArgs& /*outargs*/,
        SCXProperty& result)
    {
        SCX_LOGTRACE(m_log, L"ASProvider DoInvokeMethod");

        // If unregisterd cim method this will throw SCXProvCapNotRegistered exception
        SupportedCimMethods cimmethod = static_cast<SupportedCimMethods>(m_ProviderCapabilities.GetCimMethodId(callContext.GetObjectPath(), methodname));
        
        if (cimmethod == eSetDeepMonitoringMethod)
        {
            const SCXProperty* id = args.GetProperty(L"id");
            const SCXProperty* deep = args.GetProperty(L"deep");
            const SCXProperty* protocol = args.GetProperty(L"protocol");
            wstring prot = L"HTTP";

            // Treat the protocol parameter as not required and fall back to HTTP if not specified
            if (NULL != protocol && protocol->GetType() == SCXProperty::SCXStringType)
            {
                prot = protocol->GetStrValue();
            }

            if (NULL == id || NULL == deep)
            {
                throw SCXInternalErrorException(L"missing arguments to SetDeepMonitoring method", SCXSRCLOCATION);
            }

            if (id->GetType() != SCXProperty::SCXStringType || deep->GetType() != SCXProperty::SCXBoolType)
            {
                throw SCXInternalErrorException(L"Wrong type of arguments to SetDeepMonitoring method", SCXSRCLOCATION);
            }

            bool deepresult;
            deepresult = SetDeepMonitoring(id->GetStrValue(), deep->GetBoolValue(), prot);
            result.SetValue(deepresult);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns       The object represented as a string suitable for logging.

    */
    const std::wstring ASProvider::DumpString() const
    {
        return L"ASProvider";
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

