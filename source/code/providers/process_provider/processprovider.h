/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Process Provider

    \date      07-11-05 12:00:00


*/
/*----------------------------------------------------------------------------*/
#ifndef PROCESSPROVIDER_H
#define PROCESSPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>
#include <scxsystemlib/processenumeration.h>
#include <scxcorelib/scxlog.h>

namespace SCXCore
{

    /*----------------------------------------------------------------------------*/
    /**
       Process provider

       Concrete instance of the CMPI BaseProvider delivering CIM
       information about Processes on current host.

       A provider-specific thread lock will be held at each call to
       the Do* methods, so this implementation class does not need
       to worry about that.

    */
    class ProcessProvider : public SCXProviderLib::BaseProvider
    {
    public:

        /*----------------------------------------------------------------------------*/
        /**
            Exception thrown when an unknown resource is requested.
        */
        class UnknownResourceException : public SCXCoreLib::SCXException
        {
        public: 
            /*----------------------------------------------------------------------------*/
            /**
                Ctor 
                \param[in] resource  Name of the resource requested.
                \param[in] l         Source code location object
            */
            UnknownResourceException(std::wstring resource, 
                                     const SCXCoreLib::SCXCodeLocation& l) : SCXException(l), 
                                                                             m_resource(resource)
            {           
            }

            //! Exception description string.
            //! \returns String representation of the exception.
            std::wstring What() const
            {
                return L"Unknown resource: " + m_resource;
            }

        protected:
            //! Contains the requested resource string.
            std::wstring   m_resource;
        };

        ProcessProvider();
        ~ProcessProvider();

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_UnixProcess,
            eSCX_UnixProcessStatisticalInformation
        };

        //! The CIM methods this provider supports
        enum SupportedCimMethods {
            eTopResourceConsumerMethod
        };

        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        virtual void DoCleanup();
        virtual void DoEnumInstanceNames(const SCXProviderLib::SCXCallContext& callContext,
                                         SCXProviderLib::SCXInstanceCollection &names);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext,
                                     SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoGetInstance(const SCXProviderLib::SCXCallContext& callContext,
                                   SCXProviderLib::SCXInstance& instance);
        virtual void DoInvokeMethod(const SCXProviderLib::SCXCallContext& callContext,
                                    const std::wstring& methodname, const SCXProviderLib::SCXArgs& args,
                                    SCXProviderLib::SCXArgs& outargs, SCXProviderLib::SCXProperty& result);
        //! Determines if the provider supports SendInstanceName() and SendInstance() functions
        virtual bool SupportsSendInstance() const { return true; }

    private:
        void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> processinst, SCXProviderLib::SCXInstance& inst, SupportedCimClasses cimtype);
        void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> processinst, SCXProviderLib::SCXInstance& inst, SupportedCimClasses cimtype);
        SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> FindInstance(const SCXProviderLib::SCXInstance& keys) const;
        void GetTopResourceConsumers(const std::wstring &resource, unsigned int count, std::wstring &result);
        scxulong GetResource(const std::wstring &resource, SCXCoreLib::SCXHandle<SCXSystemLib::ProcessInstance> processinst);

    protected:
        //! PAL implementation retrieving CPU information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::ProcessEnumeration> m_processes;
    };
}

#endif /* CPUPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
