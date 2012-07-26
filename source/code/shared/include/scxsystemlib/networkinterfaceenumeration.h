/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Specification of the network interface enumeration PAL
    
    \date        08-03-03 11:23:02   
    
*/
/*----------------------------------------------------------------------------*/
#ifndef NETWORKINTERFACEENUMERATION_H
#define NETWORKINTERFACEENUMERATION_H

#include <scxsystemlib/networkinterfaceinstance.h>
#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/cpuinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib
{

    class NetworkInterfaceDependencies;
    
    /*----------------------------------------------------------------------------*/
    /**
        Represents a collection of network interfaces
    */
    class NetworkInterfaceEnumeration : public EntityEnumeration<NetworkInterfaceInstance>
    {
    public:
        NetworkInterfaceEnumeration();
        NetworkInterfaceEnumeration(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies>);
        virtual ~NetworkInterfaceEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances=true);
    protected:
        virtual void UpdateInstances();
        virtual void UpdateEnumeration();
    private:
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
        SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> m_deps; //!< Dependencies to rely on
        
    };

}


#endif /* NETWORKINTERFACEENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
