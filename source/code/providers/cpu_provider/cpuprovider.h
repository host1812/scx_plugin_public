/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     CPU Provider
 
    \date      07-05-16 12:00:00
 
    
*/
/*----------------------------------------------------------------------------*/
#ifndef CPUPROVIDER_H
#define CPUPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>
#include <scxsystemlib/cpuenumeration.h>
#include <scxcorelib/scxlog.h>

namespace SCXCore
{
    
    /*----------------------------------------------------------------------------*/
    /**
       CPU provider
   
       Concrete instance of the CMPI BaseProvider delivering CIM
       information about CPUs on current host. 

       A provider-specific thread lock will be held at each call to 
       the Do* methods, so this implementation class does not need 
       to worry about that. 

    */
    class CPUProvider : public SCXProviderLib::BaseProvider
    {
    public:
        CPUProvider();
        ~CPUProvider();

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_ProcessorStatisticalInformation
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
        void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> cpuinst, SCXProviderLib::SCXInstance& inst);
        void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> cpuinst, SCXProviderLib::SCXInstance& inst);
        SCXCoreLib::SCXHandle<SCXSystemLib::CPUInstance> FindInstance(const SCXProviderLib::SCXInstance& keys) const;
        
        //! PAL implementation retrieving CPU information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::CPUEnumeration> m_cpus;
    };
}

#endif /* CPUPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
