/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       Header for OS Provider

   \date        08-03-07 13:46:00

*/
/*----------------------------------------------------------------------------*/

#ifndef OSPROVIDER_H
#define OSPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>

#include <scxsystemlib/osenumeration.h>
#include <scxsystemlib/osinstance.h>
#include <scxsystemlib/memoryenumeration.h>
#include <scxsystemlib/scxostypeinfo.h>

#include <scxcorelib/scxlog.h>

namespace SCXCore
{

    /*----------------------------------------------------------------------------*/
    /**
       Memory provider

       Concrete instance of the CMPI BaseProvider delivering CIM
       information about the Operating System on current host.

    */
    class OSProvider : public SCXProviderLib::BaseProvider
    {
    public:
        OSProvider();
        ~OSProvider();

        virtual const std::wstring DumpString() const;

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_OperatingSystem                //!< The only class supported by this provider
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
        void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::OSInstance> osinst,
                     SCXProviderLib::SCXInstance& inst) const;
        void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::OSInstance> osinst,
                           SCXCoreLib::SCXHandle<SCXSystemLib::MemoryInstance> meminst,
                           SCXProviderLib::SCXInstance& inst) const;

        //! PAL implementation representing os information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::OSEnumeration> m_osEnum;

        //! PAL implementation representing memory information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::MemoryEnumeration> m_memEnum;

		//! PAL for providing static OS information
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXOSTypeInfo> m_OSTypeInfo;
    };
}

#endif /* OSPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
