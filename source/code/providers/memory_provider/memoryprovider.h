/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Memory provider header file
 
    \date      2007-07-04 09:31:50
 
*/
/*----------------------------------------------------------------------------*/
#ifndef MEMORYPROVIDER_H
#define MEMORYPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>
#include <scxsystemlib/memoryenumeration.h>
#include <scxsystemlib/memoryinstance.h>
#include <scxcorelib/scxlog.h>

namespace SCXCore
{
    
    /*----------------------------------------------------------------------------*/
    /**
       Memory provider
   
       Concrete instance of the CMPI BaseProvider delivering CIM
       information about Memory on current host. 

       A provider-specific thread lock will be held at each call to 
       the Do* methods, so this implementation class does not need 
       to worry about that. 

    */
    class MemoryProvider : public SCXProviderLib::BaseProvider
    {
    public:
        MemoryProvider();
        ~MemoryProvider();

        virtual const std::wstring DumpString() const;

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_MemoryStatisticalInformation
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
        void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::MemoryInstance> meminst, SCXProviderLib::SCXInstance& inst) const;
        void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::MemoryInstance> meminst, SCXProviderLib::SCXInstance& inst) const;

        //! PAL implementation retrieving memory information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::MemoryEnumeration> m_memEnum;
    };
}

#endif /* MEMORYPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
