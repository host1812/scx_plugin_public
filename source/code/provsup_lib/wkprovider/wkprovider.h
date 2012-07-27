/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file
 
    \brief     Definition of the test and demo provider 
 
    \date      07-05-14 12:00:00
 

*/
/*----------------------------------------------------------------------------*/
#ifndef TESTPROVIDER_H
#define TESTPROVIDER_H

#include <string>

#include <scxcorelib/scxlog.h>

#include <scxproviderlib/cmpibase.h>

#include /*scxsystemlib/ */<wkenumeration.h>
namespace SCXCore
{
    /*----------------------------------------------------------------------------*/
    /**
       A test provider.
    
       This is a test provider with two main purposes: 
       \li To have as starting point when creating a new provider
       \li To test all concepts and features of the CMPI Template 

       These two purposes are sometimes a bit contradictinoary; to test all 
       features the provider has to be complete, but to show the concepts the 
       provider should be as trivial as possible. Nevertheless, that is what 
       this provider does. 

       Most information on how to create an actual provider can be found at 
       \ref Using_CMPI_Template_Overview and the individual methods for overriding
       of the ProviderBase class. 

    */
    class SCXTestProvider : public SCXProviderLib::BaseProvider
    {
    public:
        // Ctor
        SCXTestProvider();
        // Dtor if resources allocated to this instance
        virtual ~SCXTestProvider();
        
    protected:
        //! The CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_Test1,
            eSCX_Test2
        };

        //! The CIM methods this provider supports
        enum SupportedCimMethods {
            eSomeMethod, 
            eOtherMethod,
            eThirdMethod
        };


        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        
        virtual void DoCleanup();
        virtual void DoEnumInstanceNames(const SCXProviderLib::SCXCallContext& callContext, 
                                         SCXProviderLib::SCXInstanceCollection& names);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext, 
                                     SCXProviderLib::SCXInstanceCollection& instances);
        virtual void DoExecQuery(const SCXProviderLib::SCXCallContext& callContext, 
                                     SCXProviderLib::SCXInstanceCollection& instances,
                                     std::wstring query, std::wstring language);
        virtual void DoGetInstance(const SCXProviderLib::SCXCallContext& callContext, 
                                   SCXProviderLib::SCXInstance& instance);
        virtual void DoCreateInstance(const SCXProviderLib::SCXCallContext& callContext, 
                                      const SCXProviderLib::SCXInstance& instance, 
                                      SCXProviderLib::SCXInstance& objectPath);
        virtual void DoInvokeMethod(const SCXProviderLib::SCXCallContext& callContext,
                                    const std::wstring& methodname, const SCXProviderLib::SCXArgs& args, 
                                    SCXProviderLib::SCXArgs& outargs, SCXProviderLib::SCXProperty& result);
        
    private:
        void AddKeys(const SCXCoreLib::SCXHandle<SCXSystemLib::TestInstance> einst, SCXProviderLib::SCXInstance& names) const;
        void AddProperties(const SCXCoreLib::SCXHandle<SCXSystemLib::TestInstance> einst, SCXProviderLib::SCXInstance& inst) const;
        SCXCoreLib::SCXHandle<SCXSystemLib::TestInstance> FindInstance(const SCXProviderLib::SCXInstance& keys) const;

        //! PAL implementation retrieving CPU information for local host
        SCXSystemLib::TestEnumeration* m_TestItems;
    };
}

#endif /* TESTPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

