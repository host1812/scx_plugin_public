/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Specification of the network provider

    \date        08-03-14 09:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include "networkprovider.h"

#include "../meta_provider/startuplog.h"
#include <scxproviderlib/scxprovidercapabilities.h>

using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;

/*----------------------------------------------------------------------------*/

namespace {

//! Determine enabled state according to CIM
//! \param[in]   intf    Interface in question
unsigned short GetEnabledState(SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> intf) {
    enum {eUnknown = 0, eEnabled = 2, eDisabled = 3, eEnabledButOffline = 6} enabledState = eUnknown;
    bool up = false;
    bool running = false;
    bool knownIfUp = intf->GetUp(up);
    bool knownIfRunning = intf->GetRunning(running);
    if (knownIfUp && knownIfRunning) {
        if (up) {
            enabledState = eEnabled;
        } else {
            enabledState = running ? eEnabledButOffline : eDisabled;
        }
    }
    return enabledState;
}

}

/*----------------------------------------------------------------------------*/

namespace SCXCore {


/*----------------------------------------------------------------------------*/
/**
 * Initialize the interfaces
 */
void NetworkProviderDependencies::InitIntf() {
    m_interfaces = new NetworkInterfaceEnumeration();
    m_interfaces->Init();
}

/*----------------------------------------------------------------------------*/
/**
 * Clean up and release resources
 */
void NetworkProviderDependencies::CleanUpIntf() {
    m_interfaces->CleanUp();
    m_interfaces = 0;
}

/*----------------------------------------------------------------------------*/
/**
 * Update interfaces
 * \param[in]   updateInstances   Update existing instances only
 */
void NetworkProviderDependencies::UpdateIntf(bool updateInstances) {
    m_interfaces->Update(updateInstances);
}

/*----------------------------------------------------------------------------*/
/** Retrive the number of interfaces
  * \returns  Number of interfaces
  */
size_t NetworkProviderDependencies::IntfCount() const {
    return m_interfaces->Size();
}

/*----------------------------------------------------------------------------*/
/** Retrieve interface at index
 * \param[in]    pos     Index to retrieve interface at
 * \returns      Interface at the index
 */
SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> NetworkProviderDependencies::GetIntf(size_t pos) const {
    return m_interfaces->GetInstance(pos);
}

/*----------------------------------------------------------------------------*/
/** Class name for IPProtocolEndpoint class. */
const std::wstring sIPProtocolEndpointClassName = L"SCX_IPProtocolEndpoint";

/*----------------------------------------------------------------------------*/
/**
   Provide CMPI interface for this class

   The class implementation (concrete class) is NetworkProvider and the name of the
   provider in CIM registration terms is SCX_NetworkProvider.

*/
SCXProviderDef(NetworkProvider, SCX_NetworkProvider)

/*----------------------------------------------------------------------------*/
/**
   Constructs an instance relying on default PAL dependencies

*/
NetworkProvider::NetworkProvider() : super(L"scx.core.providers.networkprovider"),
        m_deps(new NetworkProviderDependencies()) {
    LogStartup();
    m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.providers.networkprovider");
    SCX_LOGTRACE(m_log, L"NetworkProvider default constructor");
}


/*----------------------------------------------------------------------------*/
/**
   Constructs an instance given specific dependencies.
    \param[in]   deps    Dependencies relied upon
*/
NetworkProvider::NetworkProvider(SCXCoreLib::SCXHandle<NetworkProviderDependencies> deps)
        : super(L"scx.core.providers.networkprovider"), m_deps(deps) {
    LogStartup();
    m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.providers.networkprovider");
    SCX_LOGTRACE(m_log, L"NetworkProvider default constructor");
}

/*----------------------------------------------------------------------------*/
/**
   Destructor
*/
NetworkProvider::~NetworkProvider() {
    // Do not log here since when this destructor is called the objects neccesary for logging might no longer be alive
}

/*----------------------------------------------------------------------------*/
/**
   Registration of supported capabilities

   Callback from the BaseProvider in which the provider registers the supported
   classes and methods. The registrations should match the contents of the
   MOF files exactly.

*/
void NetworkProvider::DoInit() {
    SCX_LOGTRACE(m_log, L"NetworkProvider DoInit");
    m_deps->InitIntf();
    m_ProviderCapabilities.RegisterCimClass(eSCX_IPProtocolEndpoint,
                                            L"SCX_IPProtocolEndpoint");
    m_ProviderCapabilities.RegisterCimClass(eSCX_EthernetPortStatistics,
                                            L"SCX_EthernetPortStatistics");
}

/*----------------------------------------------------------------------------*/
/**
    Provide a way for pal layer to do cleanup. Stop all threads etc.
*/
void NetworkProvider::DoCleanup() {
    SCX_LOGTRACE(m_log, L"NetworkProvider::DoCleanup");
    m_ProviderCapabilities.Clear();
    m_deps->CleanUpIntf();
}

/*----------------------------------------------------------------------------*/
/**
   Enumerate instance names

   \param[in]   callContext Details of original request
   \param[out]  names       Collection of instances with key properties

*/
void NetworkProvider::DoEnumInstanceNames(const SCXCallContext& callContext, SCXInstanceCollection &names) {
    SCX_LOGTRACE(m_log, L"NetworkProvider DoEnumInstanceNames");

    SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

    m_deps->UpdateIntf(false);

    SCX_LOGTRACE(m_log, StrAppend(L"Number of interfaces = ", m_deps->IntfCount()));

    //SCXCoreLib::SCXThreadLock lock(m_processes->GetLockHandle());
    for(size_t i = 0; i < m_deps->IntfCount(); i++) {
        SCXInstance inst;
        SCXCoreLib::SCXHandle<NetworkInterfaceInstance> intf = m_deps->GetIntf(i);
        AddKeys(intf, inst, cimtype);
        names.AddInstance(inst);
    }
}


/*----------------------------------------------------------------------------*/
/**
   Enumerate instances

   \param[in]   callContext The context of original client request
   \param[out]  instances   Collection of instances

*/
void NetworkProvider::DoEnumInstances(const SCXCallContext& callContext, SCXInstanceCollection &instances) {
    SCX_LOGTRACE(m_log, L"NetworkProvider DoEnumInstances");

    SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

    // Update network PAL instance. This is both update of number of interfaces and
    // current statistics for each interfaces.
    m_deps->UpdateIntf(false);

    SCX_LOGTRACE(m_log, StrAppend(L"Number of interfaces = ", m_deps->IntfCount()));

    //SCXCoreLib::SCXThreadLock lock(m_processes->GetLockHandle());
    for(size_t i = 0; i < m_deps->IntfCount(); i++)
    {
        SCXInstance inst;
        SCXCoreLib::SCXHandle<NetworkInterfaceInstance> intf = m_deps->GetIntf(i);
        AddKeys(intf, inst, cimtype);
        AddProperties(m_deps->GetIntf(i), inst, cimtype);
        instances.AddInstance(inst);
    }
}

/*----------------------------------------------------------------------------*/
/**
   Get an instance

   \param[in]   callContext Context of original request, indicating which instance to retrieve
   \param[out]  instance    The selected instance

   \throws      SCXInvalidArgumentException  If no Name property in keys
   \throws      SCXInternalErrorException    If instances in list are not NetworkInterfaceInstance
*/
void NetworkProvider::DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance) {
    SCX_LOGTRACE(m_log, L"NetworkProvider::DoGetInstance()");

    SupportedCimClasses cimtype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

    // Refresh the collection (both keys and current data)
    m_deps->UpdateIntf();

    //SCXCoreLib::SCXThreadLock lock(m_processes->GetLockHandle());
    SCXCoreLib::SCXHandle<NetworkInterfaceInstance> interfaceinst = FindInstance(callContext.GetObjectPath(), cimtype, m_deps);

    // If we get here whithout exception we got a match - set keys and properties,
    // the instance is returned as out value
    AddKeys(interfaceinst, instance, cimtype);
    AddProperties(interfaceinst, instance, cimtype);

}


/*----------------------------------------------------------------------------*/
/**
   Add a SCXInstance with the name property set from the NetworkInterfaceInstance to the collection

   \param[in]   intf           network interface instance to get data from
   \param[out]  instance       Instance to add keys to
   \param[in]   cimtype        Type of CIM Class to return

   \throws      SCXInvalidArgumentException - The instance can not be converted to a NetworkInterfaceInstance

   This method contains knowledge on which are the key fields for the class.
   The key properties are defined in the MOF file.

*/
void NetworkProvider::AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> intf, SCXInstance &instance, SupportedCimClasses cimtype) {
    SCX_LOGTRACE(m_log, L"NetworkProvider AddKeys()");

    if (intf == NULL) {
        throw SCXInvalidArgumentException(L"intf", L"Not a NetworkInterfaceInstance", SCXSRCLOCATION);
    }
    if (eSCX_IPProtocolEndpoint == cimtype) {
        instance.AddKey(SCXProperty(L"CreationClassName", sIPProtocolEndpointClassName));
        instance.AddKey(SCXProperty(L"Name", intf->GetName()));
        AddScopingSystemKeys(instance);

    } else if (eSCX_EthernetPortStatistics == cimtype) {
        SCXProperty prop(L"InstanceID", intf->GetName());
        instance.AddKey(prop);
    }

}

/*----------------------------------------------------------------------------*/
/**
   Set all properties from the NetworkInterfaceInstance in the SCXInstance

   \param[in]  intf          Network interface instance to get data from
   \param[in]  instance      Instance to populate
   \param[in]  cimtype       Type of CIM Class to return

   \throws      SCXInvalidArgumentException - If the instance can not be converted to a NetworkInterfaceInstance

   This method knows how to map the values of the Network interface PAL to the CMPI class
   definition.

*/
void NetworkProvider::AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> intf, SCXInstance &instance, SupportedCimClasses cimtype) {
    if (intf == NULL) {
        throw SCXInvalidArgumentException(L"intf", L"Not a NetworkInterfaceInstance", SCXSRCLOCATION);
    }

    SCX_LOGTRACE(m_log, L"NetworkProvider AddPropeties()");

    scxulong ulong = 0;
    wstring text;
    if (eSCX_IPProtocolEndpoint == cimtype) {
        instance.AddProperty(SCXProperty(L"ElementName", intf->GetName()));
        if (intf->GetIPAddress(text)) {
            SCXProperty prop(L"IPv4Address", text);
            instance.AddProperty(prop);
        }
        if (intf->GetBroadcastAddress(text)) {
            SCXProperty prop(L"IPv4BroadcastAddress", text);
            instance.AddProperty(prop);
        }
        if (intf->GetNetmask(text)) {
            SCXProperty prop(L"SubnetMask", text);
            instance.AddProperty(prop);
        }
        instance.AddProperty(SCXProperty(L"EnabledState", GetEnabledState(intf)));

    } else if (eSCX_EthernetPortStatistics == cimtype) {
        scxulong bytesReceived = intf->GetBytesReceived(ulong) ? ulong : 0;
        instance.AddProperty(SCXProperty(L"BytesReceived", bytesReceived));

        scxulong bytesTransmitted = intf->GetBytesSent(ulong) ? ulong : 0;
        instance.AddProperty(SCXProperty(L"BytesTransmitted", bytesTransmitted));

        instance.AddProperty(SCXProperty(L"BytesTotal", bytesReceived + bytesTransmitted));
        instance.AddProperty(SCXProperty(L"PacketsReceived", intf->GetPacketsReceived(ulong) ? ulong : 0));
        instance.AddProperty(SCXProperty(L"PacketsTransmitted", intf->GetPacketsSent(ulong) ? ulong : 0));
        instance.AddProperty(SCXProperty(L"TotalTxErrors", intf->GetErrorsSending(ulong) ? ulong : 0));
        instance.AddProperty(SCXProperty(L"TotalRxErrors", intf->GetErrorsReceiving(ulong) ? ulong : 0));
        instance.AddProperty(SCXProperty(L"TotalCollisions", intf->GetCollisions(ulong) ? ulong : 0));
    }
}


/*----------------------------------------------------------------------------*/
/**
  Lookup the instance representation, given keys provided from CIMOM

  \param[in]    keys      SCXInstance with property keys set
  \param[in]    cimtype   Kind of class
  \param[in]    deps      Dependencies to rely on
  \returns             Pointer to located instance

  \throws              SCXInvalidArgumentException
  \throws              SCXInternalErrorException
  \throws              SCXCIMInstanceNotFound   The instance with given keys cannot be found

  This method knows which the key properties of the entity are and returns
  pointer to that item if found.

*/
SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance>
NetworkProvider::FindInstance(const SCXInstance& keys, SupportedCimClasses cimtype,
        SCXCoreLib::SCXHandle<NetworkProviderDependencies> deps) const
{
    const SCXProperty* mainKeyProp = 0;
    if (eSCX_IPProtocolEndpoint == cimtype) {
        ValidateKeyValue(L"CreationClassName", keys, sIPProtocolEndpointClassName);
        ValidateScopingSystemKeys(keys);
        mainKeyProp = &GetKeyRef(L"Name", keys);
    } else if (eSCX_EthernetPortStatistics == cimtype) {
        mainKeyProp = &GetKeyRef(L"InstanceID", keys);
    }
    SCXASSERT(mainKeyProp != 0);
    for(size_t i = 0; i < deps->IntfCount(); i++) {
        SCXCoreLib::SCXHandle<NetworkInterfaceInstance> instance = deps->GetIntf(i);
        if (instance == 0) {
            throw SCXInternalErrorException(L"Instance from list not a NetworkInterfaceInstance", SCXSRCLOCATION);
        }
        // Compare key values of input args and the current instance
        if (instance->GetName() == mainKeyProp->GetStrValue()) {
            // Match
            return instance;
        }
    }

    throw SCXCIMInstanceNotFound(keys.DumpString(), SCXSRCLOCATION);
}

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

