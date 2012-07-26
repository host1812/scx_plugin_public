/*-------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Application Server provider header file

    \date      2011-05-09

*/
/*----------------------------------------------------------------------------*/
#ifndef ASPROVIDER_H
#define ASPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>
#include "appserverenumeration.h"
#include <scxcorelib/scxlog.h>

namespace SCXCore
{

    /*----------------------------------------------------------------------------*/
    /**
       Class representing all external dependencies from the AppServer PAL.

    */
    class AppServerProviderPALDependencies
    {
    public:
        virtual ~AppServerProviderPALDependencies() {};

        virtual SCXCoreLib::SCXHandle<SCXSystemLib::AppServerEnumeration> CreateEnum();
    };


    /*----------------------------------------------------------------------------*/
    /**
       Application Server provider

       Concrete instance of the CMPI BaseProvider delivering CIM
       information about the application servers running on current host.

       A provider-specific thread lock will be held at each call to
       the Do* methods, so this implementation class does not need
       to worry about that.

    */
    class ASProvider : public SCXProviderLib::BaseProvider
    {
    public:
        ASProvider(SCXCoreLib::SCXHandle<AppServerProviderPALDependencies> deps = SCXCoreLib::SCXHandle<AppServerProviderPALDependencies>(new AppServerProviderPALDependencies()));
        ~ASProvider();

        virtual const std::wstring DumpString() const;

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_AS   //!< Application Server
        };
        //! The CIM methods this provider supports
        enum SupportedCimMethods {
            eSetDeepMonitoringMethod
        };


        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        virtual void DoCleanup();
        virtual void DoEnumInstanceNames(const SCXProviderLib::SCXCallContext& callContext,
                                         SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext,
                                     SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoGetInstance(const SCXProviderLib::SCXCallContext& callContext,
                                   SCXProviderLib::SCXInstance& instance);
        virtual void DoInvokeMethod(const SCXProviderLib::SCXCallContext& callContext,
                                    const std::wstring& methodname, const SCXProviderLib::SCXArgs& args,
                                    SCXProviderLib::SCXArgs& outargs, SCXProviderLib::SCXProperty& result);

    private:
        void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::AppServerInstance> asinst, SCXProviderLib::SCXInstance& inst);
        void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::AppServerInstance> asinst, SCXProviderLib::SCXInstance& inst);
        SCXCoreLib::SCXHandle<SCXSystemLib::AppServerInstance> FindInstance(const SCXProviderLib::SCXInstance& keys) const;

        bool SetDeepMonitoring(std::wstring id, bool deep, std::wstring protocol);

        SCXCoreLib::SCXHandle<AppServerProviderPALDependencies> m_deps;

        //! PAL implementation retrieving appserver information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::AppServerEnumeration> m_appservers;
    };
}

#endif /* ASPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
