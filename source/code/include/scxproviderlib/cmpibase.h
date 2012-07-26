/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Declarations of CMPITemplate-related classes

    \date      07-05-14 11:51:22


*/
/*----------------------------------------------------------------------------*/
#ifndef CMPIBASE_H
#define CMPIBASE_H

#include <string>
#include <vector>

#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scx_widen_string.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlocale.h>

#include <scxproviderlib/scxproperty.h>
#include <scxproviderlib/scxargs.h>
#include <scxproviderlib/scxinstance.h>
#include <scxproviderlib/scxinstancecollection.h>
#include <scxproviderlib/scxcmpibaseexceptions.h>
#include <scxproviderlib/scxprovidercapabilities.h>
#include <scxproviderlib/scxprovidercallctx.h>

#include <Pegasus/Provider/CMPI/cmpidt.h>
#include <Pegasus/Provider/CMPI/cmpift.h>
#include <Pegasus/Provider/CMPI/cmpimacs.h>


/**
   Contains all SCX framework support for writing CMPI providers.

   The namespace is of interest only to actual providers.

*/
namespace SCXProviderLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Base class for SCX CMPI providers.

       Provider writers should not be exposed to CMPI, which this class
       encapsulates. All CMPI stuff should be contained in this class.

       To implement a provider, subclass this class and use the SCXProviderDef() macro
       to set up neccesary structures.
    */
    class BaseProvider
    {
    public:

        virtual ~BaseProvider();

        void SetBroker(const CMPIBroker* brkr);

        void Init();

        // These definitions map 1:1 to the CMPI definition, callbacks from CIMOM
        CMPIStatus Cleanup(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                           CMPIBoolean terminate);
        CMPIStatus MethodCleanup(CMPIMethodMI* cThis, const CMPIContext* pContext,
                                 CMPIBoolean terminate);
        CMPIStatus EnumInstanceNames(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                     const CMPIResult* resultHandle,
                                     const CMPIObjectPath* pObjectPath);
        CMPIStatus EnumInstances(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                 const CMPIResult* resultHandle,
                                 const CMPIObjectPath* pObjectPath, const char** properties);
        CMPIStatus GetInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                               const CMPIResult* resultHandle,
                               const CMPIObjectPath* pObjectPath, const char** properties);
        CMPIStatus CreateInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                  const CMPIResult* resultHandle,
                                  const CMPIObjectPath* pCmpiObjectPath,
                                  const CMPIInstance* pInstance);
        CMPIStatus ModifyInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                  const CMPIResult* resultHandle,
                                  const CMPIObjectPath* pCmpiObjectPath,
                                  const CMPIInstance* pInstance, const char** properties);
        CMPIStatus DeleteInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                  const CMPIResult* resultHandle,
                                  const CMPIObjectPath* pCmpiObjectPath);
        CMPIStatus ExecQuery(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                             const CMPIResult* resultHandle,
                             const CMPIObjectPath* pCmpiObjectPath, const char* query,
                             const char* language);
        CMPIStatus InvokeMethod(CMPIMethodMI* cThis, const CMPIContext* pContext,
                                const CMPIResult* resultHandle,
                                const CMPIObjectPath* pObjectPath, const char* method,
                                const CMPIArgs* in, CMPIArgs* out);

        //! Get a handle to the CIMOM for callbacks
        //! \returns Handle to the CIMOM (CMPI broker)
        const CMPIBroker*  GetBrokerHandle() const { return m_broker; }


        static void ValidateScopingComputerSystemKeys(const SCXProviderLib::SCXInstance& keys);
        static void AddScopingComputerSystemKeys(SCXProviderLib::SCXInstance &instance);

        static void ValidateScopingSystemKeys(const SCXProviderLib::SCXInstance& keys);
        static void AddScopingSystemKeys(SCXProviderLib::SCXInstance &instance);

        static void ValidateScopingOperatingSystemKeys(const SCXProviderLib::SCXInstance& keys);
        static void AddScopingOperatingSystemKeys(SCXProviderLib::SCXInstance &instance);

        static const SCXProviderLib::SCXProperty &GetKeyRef(const std::wstring &key, const SCXProviderLib::SCXInstance &keys);
        static void ValidateKeyValue(const std::wstring &name, const SCXProviderLib::SCXInstance &instance, const std::wstring &value);

    protected:
        // Used only by subclasses
        BaseProvider(const std::wstring& module);

        void EnableProviderUnloading();
        void DisableProviderUnloading();

        //! Object handling dynamic capabilities of the provider. Set up by DoInit()
        // This is the set of methods a provider implementation subclass should implement to
        // provide its functionality
        //! This method should be used to register classes and methods etc. using the SCXProviderCapabilities class
        virtual void DoInit() = 0;
        virtual void DoEnumInstanceNames(const SCXCallContext& callContext, SCXInstanceCollection& names);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext, SCXInstanceCollection& instances);
        virtual void DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance);
        virtual void DoCreateInstance(const SCXCallContext& callContext, const SCXInstance& instance, SCXInstance& createdObjectPath);
        virtual void DoExecQuery(const SCXProviderLib::SCXCallContext& callContext, SCXInstanceCollection& instances,
                std::wstring query, std::wstring language);
        virtual void DoCleanup();
        virtual void DoInvokeMethod(const SCXCallContext& callContext, const std::wstring& methodname,
                                    const SCXArgs& args, SCXArgs& outargs, SCXProperty& result);
        //! Determines if the provider supports SendInstanceName() and SendInstance() functions
        virtual bool SupportsSendInstance() const { return false; }

        //! This function sends one instance to the server through CMPI (by calling CMReturnObjectPath).
        virtual void SendInstanceName(const SCXInstance& instance);

        //! This function sends one instance to the server through CMPI (by calling CMReturnInstance).
        virtual void SendInstance(const SCXInstance& instance);

        SCXProviderCapabilities         m_ProviderCapabilities;  //!< provider capabilities

        //! Handle to the log functionality. Also used by subclass.
        SCXCoreLib::SCXLogHandle m_log;

        const static std::wstring s_cOSCreationClassName; //!< "SCX_OperatingSystem"
        const static std::wstring s_cCSCreationClassName; //!< "SCX_ComputerSystem"

    private:

        static void GetKeysForSCX_OperatingSystem(std::wstring& CSName,
                                                  std::wstring& CSCreationClassName,
                                                  std::wstring& Name,
                                                  std::wstring& CreationClassName);
        static void GetKeysForSCX_ComputerSystem(std::wstring& Name,
                                                 std::wstring& CreationClassName);

        void SCXInstanceToCMPIInstance(const SCXInstance* inst, CMPIInstance *pInstance) const;
        void SCXInstanceGetKeys(const SCXInstance* sInst, CMPIObjectPath* pCmpiObjectPath) const;
        void SCXPropertyToCMPIValue(const SCXProperty* prop, CMPIValue* value, CMPIType* type) const;
        void CMPIValueToSCXProperty(const CMPIValue& value, CMPIType type, SCXProperty& prop) const ;
        void CMPIDataToSCXProperty(const CMPIData& data, SCXProperty& prop) const;
        void CMPIObjectPathToScxObjectPath(const CMPIObjectPath* pObjectPath, SCXInstance& objectPath) const;
        void CMPIInstanceToScxInstance(const SCXInstance&  scxObjectPath, const CMPIInstance* pInstance,
                                       SCXInstance& scxInstance) const;
        CMPIObjectPath* GetNewObjectPath(const CMPIObjectPath* pObjectPath) const;

        //! Pointer back to the CIMOM (MB) set up during provider init call from the MB
        const CMPIBroker*               m_broker;

        //! Anonymous lock for this instance
        SCXCoreLib::SCXThreadLockHandle m_lock;

        //! Flag indicating if provider allows unloading or not. By default, it is not.
        bool                            m_allowUnload;

        //! Flag to ensure init mehtod is invoked only once
        bool                            m_initDone;

        //! Flag to ensure cleanup mehtod is invoked only once
        bool                            m_cleanupDone;

        // State data used by SendInstance(). These must be set before dispatching to provider method and cleared afterwards.
        const CMPIResult* m_result;
        const CMPIObjectPath* m_objectPath;
    };
}

namespace SCXCore {

    /*
       Note: SingleProvider must be in same namespace as the providers or else all non-gcc compilers won't let us
       do an explicit instantiation of s_logModule from the SCXProviderDef macro.
    */

    /*----------------------------------------------------------------------------*/
    /**
        Template for CMPI interface for a provider.

        Used internally by the registration macro SCXProviderDef() and fills the
        gap between the C CMPI inteface to the C++ provider instance.

        CMPI is a C interface and need class methods for the interface.
        This template implements the neccesary class methods needed to interface with CMPI.
        By using this template you will get a specific class that passes the calls on to a
        singleton instance of your provider.

    */
    template <class T>
    class SingleProvider
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Implementation of CMPI interface. Only passthrough to instance methods.
            See CMPI documentation for reference:
            http://sharepointemea/sites/aurora/Shared%20Documents/OpenGroup/CMPI%202.0.pdf
        */
        /*----------------------------------------------------------------------------*/
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus Cleanup(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                  CMPIBoolean terminate)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::Cleanup() - terminate = ", terminate));
            // WI 10487: delete singleton after cleanup
            CMPIStatus ret;  
            memset( &ret, 0, sizeof(ret) );
            ret.rc = CMPI_RC_OK;
            if ( inst ) { 
                // only call cleanup if instance is loaded
                ret = inst->Cleanup(cThis, pContext, terminate);
                if ( CMPI_RC_OK == ret.rc )
                    RemoveSingleInstance();
            }
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::Cleanup() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus MethodCleanup(CMPIMethodMI* cThis, const CMPIContext* pContext,
                                        CMPIBoolean terminate)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::MethodCleanup() - terminate = ", terminate));
            CMPIStatus ret;  
            memset( &ret, 0, sizeof(ret) );
            ret.rc = CMPI_RC_OK;
            if ( inst ) {
                // only call cleanup if instance is loaded
                ret = GetSingleInstance()->MethodCleanup(cThis, pContext, terminate);
                if ( CMPI_RC_OK == ret.rc )
                    RemoveSingleInstance();
            }
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::MethodCleanup() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus EnumInstanceNames(CMPIInstanceMI*  cThis, const CMPIContext* pContext,
                                            const CMPIResult* resultHandle,
                                            const CMPIObjectPath* pObjectPath)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::EnumInstanceNames()");
            CMPIStatus ret = GetSingleInstance()->EnumInstanceNames(cThis, pContext, resultHandle, pObjectPath);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::EnumInstanceNames() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus EnumInstances(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                        const CMPIResult* resultHandle,
                                        const CMPIObjectPath* pObjectPath,
                                        const char** properties)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::EnumInstances()");
            CMPIStatus ret = GetSingleInstance()->EnumInstances(cThis, pContext, resultHandle, pObjectPath, properties);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::EnumInstances() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus GetInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                      const CMPIResult* resultHandle,
                                      const CMPIObjectPath* pCmpiObjectPath,
                                      const char** properties)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::GetInstance()");
            CMPIStatus ret = GetSingleInstance()->GetInstance(cThis, pContext, resultHandle, pCmpiObjectPath, properties);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::GetInstance() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus CreateInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                         const CMPIResult* resultHandle,
                                         const CMPIObjectPath* pCmpiObjectPath,
                                         const CMPIInstance* pInstance)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::CreateInstance()");
            CMPIStatus ret = GetSingleInstance()->CreateInstance(cThis, pContext, resultHandle, pCmpiObjectPath, pInstance);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::CreateInstance() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus ModifyInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                         const CMPIResult* resultHandle,
                                         const CMPIObjectPath* pCmpiObjectPath,
                                         const CMPIInstance* pInstance, const char** properties)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::ModifyInstance()");
            CMPIStatus ret = GetSingleInstance()->ModifyInstance(cThis, pContext, resultHandle, pCmpiObjectPath,
                                                                 pInstance, properties);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::ModifyInstance() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus DeleteInstance(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                         const CMPIResult* resultHandle,
                                         const CMPIObjectPath* pCmpiObjectPath)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::DeleteInstance()");
            CMPIStatus ret = GetSingleInstance()->DeleteInstance(cThis, pContext, resultHandle, pCmpiObjectPath);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::DeletInstance() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
           \note The order of the parameters "language" and "query" corresponds to
           actual CMPI implementation, the CMPI documentation states the opposite order
         */
        static CMPIStatus ExecQuery(CMPIInstanceMI* cThis, const CMPIContext* pContext,
                                    const CMPIResult* resultHandle,
                                    const CMPIObjectPath* pCmpiObjectPath,
                                    const char* language, const char* query)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::ExecQuery()");
            CMPIStatus ret = GetSingleInstance()->ExecQuery(cThis, pContext, resultHandle, pCmpiObjectPath, query, language);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::ExecQuery() - Returning - ", ret.rc));
            return ret;
        };
        /**
           C entry point for CMPI function for this provider.
         */
        static CMPIStatus InvokeMethod(CMPIMethodMI* cThis, const CMPIContext* pContext,
                                       const CMPIResult* resultHandle,
                                       const CMPIObjectPath* pCmpiObjectPath,
                                       const char* method, const CMPIArgs* in, CMPIArgs* out)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            SCX_LOGTRACE(log, L"SingleProvider::InvokeMethod()");
            CMPIStatus ret = GetSingleInstance()->InvokeMethod(cThis, pContext, resultHandle, pCmpiObjectPath,
                                                               method, in, out);
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"SingleProvider::InvokeMethod() - Returning - ", ret.rc));
            return ret;
        };
        /*----------------------------------------------------------------------------*/

        /*----------------------------------------------------------------------------*/
        /**
            Build call structure needed to interface instance part of CMPI

            \param[in] miName   Provider name

            \returns   CMPIInstanceMI structure containing pointers to class methods

            Exposed via the FT table to the MB.

        */
        static CMPIInstanceMI* InitInstance(const char* miName)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            static CMPIInstanceMIFT instMIFT =
            {
                CMPICurrentVersion,
                CMPICurrentVersion,
                miName,
                Cleanup,
                EnumInstanceNames,
                EnumInstances,
                GetInstance,
                CreateInstance,
                ModifyInstance,
                DeleteInstance,
                ExecQuery
            };
            static CMPIInstanceMI mi = { 0, &instMIFT };

            SCX_LOGTRACE(log, L"SingleProvider::InitInstance()");

            /* The seems like a suitable place to report the active locale */
            SCX_LOGTRACE(log, SCXCoreLib::StrAppend(L"Active locale setting is ",
                         SCXCoreLib::SCXLocaleContext::GetActiveLocale()));

            return &mi;
        };

        /*----------------------------------------------------------------------------*/
        /**
            Build call structure needed to interface method part of CMPI

            \param[in]   miName  Provider name

            \returns     CMPIMethodMI structure containing pointers to class methods

            Exposed via the FT table to the MB.

        */
        static CMPIMethodMI* InitMethod(const char* miName)
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);
            static CMPIMethodMIFT methMIFT =
            {
                CMPICurrentVersion,
                CMPICurrentVersion,
                miName,
                MethodCleanup,
                InvokeMethod
            };
            static CMPIMethodMI mi = { 0, &methMIFT };
            SCX_LOGTRACE(log, L"SingleProvider::InitMethod()");
            return &mi;
        };

        /*----------------------------------------------------------------------------*/
        /**
            Retrieve a pointer to the singleton instance.

            \returns       Pointer to the singleton instance

            Used by internal CMPI Template handling only.

        */
        static T* GetSingleInstance()
        {
            SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(s_logModule);

            SCXCoreLib::SCXThreadLock lock(m_lock);

            SCX_LOGTRACE(log, L"SingleProvider::GetSingleInstance()");

            if (inst == NULL)
            {
                inst = new T;
                SCX_LOGTRACE(log, L"SingleProvider::GetSingleInstance() - Created singleton instance");
            }
            return inst;
        };

        /*----------------------------------------------------------------------------*/
        /**
            Delete the singleton instance.

            Used by internal CMPI Template handling only.

        */
        static void RemoveSingleInstance()
        {
            // Do not log here since when this is called the objects neccesary for logging might no longer be alive

            if (inst != NULL)
            {
                delete inst;
                inst = NULL;
            }
        };

        /**
            Constructor.
        */
        SingleProvider() {};

        /**
            Destructor.
        */
        ~SingleProvider() { RemoveSingleInstance(); };

    private:
        //! Handle to the only instance of this provider
        static T* inst;
        //! Handle to the thread lock of the provider
        static SCXCoreLib::SCXThreadLockHandle m_lock;
        //! Log functionality handle
        static const std::wstring s_logModule; // Note: Explicitly instantiated in provider macro
    };

    //! Definition of the singleton instance
    template <class T>
    T* SingleProvider<T>::inst = NULL;

    //! Definition of the thread lock used for the singleton itself (not to be
    //! confused with the provider lock in the CMPI base class).
    template <class T>
    SCXCoreLib::SCXThreadLockHandle SingleProvider<T>::m_lock = SCXCoreLib::ThreadLockHandleGet();

    /*----------------------------------------------------------------------------*/
    /**
        Macro instantiating the provider

        \param[in]   cl    Class name of the provider, used for name of singleton
                           instance. The name should match the name of the concrete
                           subclass implementing the provider.
        \param[in]   pn    Provider name (must match registration information)

        Each provider must have one single definition of this macro. It will
        make sure a singleton with the provider is accessible from ouside the
        provider module using correct (C) linking.

        The class name provided in cl will be used to create a singleton instance
        of the class. The cl instance will be created at library load time, with
        the thread lock of the singleton held.

        SCXProviderDef() wraps both Instance Provider functionality and Method
        Provider functionality into the same provider implementation class. The
        ctor of the implementation class will be run upon first access of either
        the Instance FT or the Method FT, as the Instance() method of the
        singleton is invoked.

        The top-level descritpion on how to write a CMPI provider based on the
        CMPI Template is found at \ref Using_CMPI_Template_Overview.


        \note When the SCXProviderDef() macro is called to instantiate a provider,
              the line can not be terminated by a semicolon as would be natural.

    */

#if (defined(hpux) && (PF_MAJOR==11) && (PF_MINOR < 31))
    /*
       This version must be used for aCC version < 03.80, will be removed when compiler is in place.
           WI7939
    */

    #define SCXProviderDef(cl, pn)                                              \
        typedef SingleProvider<cl> cl##Single;                          \
        static cl##Single cl##Remover;                                  \
        template<> const std::wstring cl##Single::s_logModule = L"scx.core.provsup.cmpibase.singleprovider"; \
        extern "C" CMPIInstanceMI* pn##_Create_InstanceMI(const CMPIBroker* brkr, \
                                                          const CMPIContext* /*ctx*/, \
                                                          CMPIStatus* /*rc*/) \
        {                                                               \
            SCX_LOGTRACE(SCXCoreLib::SCXLogHandleFactory::Instance().GetLogHandle(L"scx.core.provsup.cmpibase.scxproviderdef"), std::wstring(L"Create_InstanceMI")); \
            cl##Single::GetSingleInstance()->SetBroker(brkr);           \
            cl##Single::GetSingleInstance()->Init();                    \
            return cl##Single::InitInstance("instance" #pn);            \
        }                                                               \
        extern "C" CMPIMethodMI* pn##_Create_MethodMI(const CMPIBroker* brkr, \
                                                      const CMPIContext* /*ctx*/, \
                                                      CMPIStatus* /*rc*/) \
        {                                                               \
            SCX_LOGTRACE(SCXCoreLib::SCXLogHandleFactory::Instance().GetLogHandle(L"scx.core.provsup.cmpibase.scxproviderdef"), std::wstring(L"Create_MethodMI")); \
            cl##Single::GetSingleInstance()->SetBroker(brkr);           \
            cl##Single::GetSingleInstance()->Init();                    \
            return cl##Single::InitMethod("method" #pn);                \
        }

#else

    /* New improved version for all other plaforms. */
    #define SCXProviderDef(cl, pn) \
        typedef SingleProvider<cl> cl##Single; \
        static cl##Single cl##Remover; \
        template<> const std::wstring cl##Single::s_logModule = L"scx.core.provsup.cmpibase.singleprovider." L ## #cl; \
        extern "C" CMPIInstanceMI* pn##_Create_InstanceMI(const CMPIBroker* brkr, \
                                                          const CMPIContext* /*ctx*/, \
                                                          CMPIStatus* /*rc*/) \
        { \
            SCX_LOGTRACE(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.cmpibase.scxproviderdef"), std::wstring(L"Create_InstanceMI " L ## #pn)); \
            cl##Single::GetSingleInstance()->SetBroker(brkr); \
            cl##Single::GetSingleInstance()->Init(); \
            return cl##Single::InitInstance("instance" #pn); \
        } \
        extern "C" CMPIMethodMI* pn##_Create_MethodMI(const CMPIBroker* brkr, \
                                                      const CMPIContext* /*ctx*/, \
                                                      CMPIStatus* /*rc*/) \
        { \
            SCX_LOGTRACE(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.cmpibase.scxproviderdef"), std::wstring(L"Create_MethodMI " L ## #pn)); \
            cl##Single::GetSingleInstance()->SetBroker(brkr); \
            cl##Single::GetSingleInstance()->Init(); \
            return cl##Single::InitMethod("method" #pn); \
        }
#endif
}

#endif /* CMPIBASE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
