/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Implementation of network interface enumeration PAL
    
    \date        08-03-03 12:12:02

    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterfaceenumeration.h>
#include "networkinterface.h"

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib {

/*----------------------------------------------------------------------------*/
//! Constructs an enumeration dependent on the actual system
NetworkInterfaceEnumeration::NetworkInterfaceEnumeration()
        : m_log(SCXLogHandleFactory::GetLogHandle(
                L"scx.core.common.pal.system.networkinterface.networkinterfaceenumeration")),
          m_deps(new NetworkInterfaceDependencies())
{    
}

/*----------------------------------------------------------------------------*/
//! Constructs an enumeration dependent on injected dependencies rather than the actual system.
//! Useful for testing behaviour in a reproducable manner.
NetworkInterfaceEnumeration::NetworkInterfaceEnumeration(SCXHandle<NetworkInterfaceDependencies> deps)
        : m_log(SCXLogHandleFactory::GetLogHandle(
                L"scx.core.common.pal.system.networkinterface.networkinterfaceenumeration")),
          m_deps(deps)
{    
}

/*----------------------------------------------------------------------------*/
//! Destructor
NetworkInterfaceEnumeration::~NetworkInterfaceEnumeration() {
}

/*----------------------------------------------------------------------------*/
//! Implementation of the Init method of the entity framework.
void NetworkInterfaceEnumeration::Init() {
    UpdateEnumeration();
}

/*----------------------------------------------------------------------------*/
/**
   Implementation of the Update method of the entity framework.

   \param updateInstances - indicates whether only the existing instances shall be updated. 
   
   The method refreshes the set of known instances in the enumeration. 
   
   Any newly created instances must have a well-defined state after execution, 
   meaning that instances which update themselves have to init themselves upon 
   creation. 
*/
void NetworkInterfaceEnumeration::Update(bool updateInstances) {
    if (updateInstances) {
        UpdateInstances();        
    } else {
        UpdateEnumeration();
    }
    
}

/*----------------------------------------------------------------------------*/
//! Run the Update() method on all instances in the colletion, including the
//! Total instance if any.
//! \note Optimized implementation that recreates the same result as running update on
//!       each instance, but does not actually do so
void NetworkInterfaceEnumeration::UpdateInstances() {
    vector<NetworkInterfaceInfo> latestInterfaces = NetworkInterfaceInfo::FindAll(m_deps);
    typedef map<wstring, size_t> IndexByStrMap;
    IndexByStrMap latestInterfaceById;

    // Create an index of the latest instances by their id
    for (size_t nr = 0; nr < latestInterfaces.size(); nr++) {
        latestInterfaceById.insert(IndexByStrMap::value_type(NetworkInterfaceInstance(latestInterfaces[nr]).GetId(), nr));
    }    
    
    for (EntityIterator oldIter = Begin(); oldIter != End(); oldIter++) {
        IndexByStrMap::iterator stillExistingInterfacePos = latestInterfaceById.find((*oldIter)->GetId());        
        if (stillExistingInterfacePos != latestInterfaceById.end()) {                                    
            // Update instances that still exists
            (*oldIter)->Update(latestInterfaces[stillExistingInterfacePos->second]);
        }
    }
    
}

/*----------------------------------------------------------------------------*/
//! Make the enumeration correspond to the current state of the system
void NetworkInterfaceEnumeration::UpdateEnumeration() {
    vector<NetworkInterfaceInfo> latestInterfaces = NetworkInterfaceInfo::FindAll(m_deps);
    typedef map<wstring, size_t> IndexByStrMap;
    IndexByStrMap newInterfaceById;

    // Prepare an index of new instances by their id
    for (size_t nr = 0; nr < latestInterfaces.size(); nr++) {
        newInterfaceById.insert(IndexByStrMap::value_type(NetworkInterfaceInstance(latestInterfaces[nr]).GetId(), nr));
    }    
    
    for (size_t nr = Size();  nr > 0; nr--) {
        IndexByStrMap::iterator stillExistingInterfacePos = newInterfaceById.find(GetInstance(nr-1)->GetId());
        if (stillExistingInterfacePos != newInterfaceById.end()) {                                    
            // Update instances that still exists
            GetInstance(nr-1)->Update(latestInterfaces[stillExistingInterfacePos->second]);

            // Remove instances, that are not new, from index of new instances
            newInterfaceById.erase(stillExistingInterfacePos);
        } else { 
            // Remove instances, that no longer exists, from the enumeration
            RemoveInstance(Begin() + nr - 1);
        }
    }
    
    // Add new instances to the enumeration
    for (IndexByStrMap::iterator iter = newInterfaceById.begin(); iter != newInterfaceById.end(); iter++) {
        const NetworkInterfaceInfo &intf = latestInterfaces[iter->second];
        bool knownState = intf.IsKnownIfUp() && intf.IsKnownIfRunning();
        bool acceptedAddress = !intf.IsIPAddressKnown() || intf.GetIPAddress().find(L"127.0.0.") != 0; 
        // Fix according to WI5275, don't add uninteresting interfaces
        if (knownState && acceptedAddress) {
            AddInstance(
                SCXCoreLib::SCXHandle<NetworkInterfaceInstance>(new NetworkInterfaceInstance(latestInterfaces[iter->second])));
        }
    }
}

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
