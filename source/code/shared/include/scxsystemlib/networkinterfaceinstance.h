/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Specification of network interface instance PAL 
    
    \date        08-03-03 11:19:02
    
*/
/*----------------------------------------------------------------------------*/
#ifndef NETWORKINTERFACEINSTANCE_H
#define NETWORKINTERFACEINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib  
{

    class NetworkInterfaceInfo;
    
    /*----------------------------------------------------------------------------*/
    /**
        Represent a network interface
    */
    class NetworkInterfaceInstance : public EntityInstance
    {
    public:
        NetworkInterfaceInstance(const NetworkInterfaceInfo &info);
        virtual ~NetworkInterfaceInstance();

        std::wstring GetName() const;

        virtual void Update();

        void Update(const NetworkInterfaceInfo &info);
        
        bool GetIPAddress(std::wstring &value) const;
        bool GetNetmask(std::wstring &value) const;
        bool GetBroadcastAddress(std::wstring &value) const;
        bool GetBytesReceived(scxulong &value) const;
        bool GetBytesSent(scxulong &value) const;
        bool GetPacketsReceived(scxulong &value) const;
        bool GetPacketsSent(scxulong &value) const;
        bool GetErrorsReceiving(scxulong &value) const;        
        bool GetErrorsSending(scxulong &value) const;
        bool GetCollisions(scxulong &value) const;
        bool GetUp(bool &value) const;
        bool GetRunning(bool &value) const;
        
    private:
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle

        SCXCoreLib::SCXHandle<NetworkInterfaceInfo> m_info; //!< Source of data
    };

}


#endif /* NETWORKINTERFACE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
