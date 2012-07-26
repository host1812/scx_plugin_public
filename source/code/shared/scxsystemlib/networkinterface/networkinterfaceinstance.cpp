/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Implementation of network interface instance PAL
    
    \date        08-03-03 12:12:02

    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterfaceinstance.h>
#include "networkinterface.h"
#include <vector>

using namespace std;

namespace SCXSystemLib {

/*----------------------------------------------------------------------------*/
//! Constructor
//! \param[in]   info     Initial information
NetworkInterfaceInstance::NetworkInterfaceInstance(const NetworkInterfaceInfo &info) 
        : EntityInstance(info.GetName()), m_info(new NetworkInterfaceInfo(info)) {
    
}

/*----------------------------------------------------------------------------*/
//! Destructor
NetworkInterfaceInstance::~NetworkInterfaceInstance() {
}

/*----------------------------------------------------------------------------*/
//! Name of interface
//! \returns Name
std::wstring NetworkInterfaceInstance::GetName() const {
    return m_info->GetName();
}

/*----------------------------------------------------------------------------*/
//!  \copydoc SCXSystemLib::EntityInstance::Update()
void NetworkInterfaceInstance::Update() {
    m_info->Refresh();
}

/*----------------------------------------------------------------------------*/
//! Make the content correspond to given information
//! \param[in]   info    New content
void NetworkInterfaceInstance::Update(const NetworkInterfaceInfo &info) {
    (*m_info) = info; 
}

/*----------------------------------------------------------------------------*/
//! IP Address assigned to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetIPAddress(std::wstring &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eIPAddress)) {
        value = m_info->GetIPAddress();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Netmask assigned to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetNetmask(std::wstring &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eNetmask)) {
        value = m_info->GetNetmask();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Broadcast address assigned to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetBroadcastAddress(std::wstring &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eBroadcastAddress)) {
        value = m_info->GetBroadcastAddress();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of bytes received from interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetBytesReceived(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eBytesReceived)) {
        value = m_info->GetBytesReceived();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of bytes sent to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetBytesSent(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eBytesSent)) {
        value = m_info->GetBytesSent();
        supported = true;
    }
    return supported;
}
        
/*----------------------------------------------------------------------------*/
//! Number of packets received from interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetPacketsReceived(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::ePacketsReceived)) {
        value = m_info->GetPacketsReceived();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of packets sent to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetPacketsSent(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::ePacketsSent)) {
        value = m_info->GetPacketsSent();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of errors that have occurred when receiving from interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetErrorsReceiving(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eErrorsReceiving)) {
        value = m_info->GetErrorsReceiving();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of errors that have occurred when sending to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetErrorsSending(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eErrorsSending)) {
        value = m_info->GetErrorsSending();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of collisons that have occurred on interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetCollisions(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eCollisions)) {
        value = m_info->GetCollisions();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Is the interface up
//! \returns true iff if up
bool NetworkInterfaceInstance::GetUp(bool &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eUp)) {
        value =  m_info->IsUp();
        supported = true;
    }
    return supported;
}


/*----------------------------------------------------------------------------*/
//! Is the interface running
//! \returns true iff if running
bool NetworkInterfaceInstance::GetRunning(bool &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eRunning)) {
        value = m_info->IsRunning();
        supported = true;
    }
    return supported;
}

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
