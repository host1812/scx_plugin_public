/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Implementation of the network provider

    \date        08-03-14 09:00

*/
/*----------------------------------------------------------------------------*/
#ifndef NETWORKPROVIDER_H
#define NETWORKPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>
#include <scxsystemlib/networkinterfaceenumeration.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXCore
{
    /*----------------------------------------------------------------------------*/
    //! Encapsulates the dependencies of the network provider
    //!
    class NetworkProviderDependencies  {
    public:
        NetworkProviderDependencies() : m_interfaces(0) {}
        virtual void InitIntf();
        virtual void CleanUpIntf();
        virtual void UpdateIntf(bool updateInstances=true);
        virtual size_t IntfCount() const;
        virtual SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> GetIntf(size_t pos) const;

        //! Virtual destructor preparing for subclasses
        virtual ~NetworkProviderDependencies() { }
    private:
        //! PAL implementation retrieving network information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceEnumeration> m_interfaces;

    };

    /*----------------------------------------------------------------------------*/
    /**
       Network provider provider

       Concrete instance of the CMPI BaseProvider delivering CIM
       information about CPUs on current host.

       A provider-specific thread lock will be held at each call to
       the Do* methods, so this implementation class does not need
       to worry about that.

    */
    class NetworkProvider : public SCXProviderLib::BaseProvider
    {
    public:
        NetworkProvider();
        NetworkProvider(SCXCoreLib::SCXHandle<NetworkProviderDependencies> deps);
        ~NetworkProvider();

    protected:
        typedef SCXProviderLib::BaseProvider super; //!< Convenience type to refer to the superclass of this class.

        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_IPProtocolEndpoint,
            eSCX_EthernetPortStatistics
        };

        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        virtual void DoEnumInstanceNames(const SCXProviderLib::SCXCallContext& callContext,
                                         SCXProviderLib::SCXInstanceCollection &names);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext,
                                     SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoGetInstance(const SCXProviderLib::SCXCallContext& callContext,
                                   SCXProviderLib::SCXInstance& instance);
        virtual void DoCleanup();

    private:

        void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> intf, SCXProviderLib::SCXInstance& instance,
                     SupportedCimClasses cimtype);
        void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> intf, SCXProviderLib::SCXInstance& instance,
                           SupportedCimClasses cimtype);
        SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceInstance> FindInstance(const SCXProviderLib::SCXInstance& keys,
                                                             SupportedCimClasses cimtype,
                                                             SCXCoreLib::SCXHandle<NetworkProviderDependencies> deps) const;
        SCXCoreLib::SCXHandle<NetworkProviderDependencies> m_deps;  //!< External functionality that the provider is dependent upon
        SCXCoreLib::SCXLogHandle m_log;                 //!< Handle to log file
    };
}



#endif /* NETWORKPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
