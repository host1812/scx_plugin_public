/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Definition of SCXProviderLib::SCXProviderCapabilities
 
    \date      07-07-25 13:25:23
 

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXPROVIDERCAPABILITIES_H
#define SCXPROVIDERCAPABILITIES_H

#include <scxcorelib/scxlog.h>

#include <scxproviderlib/scxinstance.h>

#include <Pegasus/Provider/CMPI/cmpidt.h>

#include <vector>
#include <map>

namespace SCXProviderLib
{
    //! The support type the provider has for the requested class
    enum SCXProviderSupportType {
        eNoSupport = 0,    //!< This provider does not support the objectPath
        eDirectSupport,    //!< This provider has registered this class
        eInheritedSupport  //!< This provider supports at least one subclass of the requested objectPath
    };

    // Fwd
    class BaseProvider;

    /*----------------------------------------------------------------------------*/
    /**
       Class used to communicate capabilities of a provider to the CMPI Template class
    
       \date         07-07-25 13:53:21
    
       A provider inheriting from SCXProviderLib::BaseProvider must implement 
       DoInit(). In that init method, should register register the classes it 
       supports, and which methods on these, by calling this instance. The BaseProvider, 
       will then use it to lookup any individual capabilities of the provider. The 
       provider will also make calls to this class rather than parsing any 
       received information.

       All access of this class will be from the provider, with thread locks so 
       this class require no extra locking.

    
    */
    class SCXProviderCapabilities {
    public:
        SCXProviderCapabilities(BaseProvider* pProvider);

        // Called from the DoInit() method of the provider to register 
        // capabilities of the provider.
        void RegisterCimClass(unsigned int cimClassId, std::wstring cimClassName);
        void RegisterCimMethod(unsigned int cimClassId, unsigned int cimMethodId, std::wstring cimMethodName);

        // Remove all capabilties previously registred
        void Clear();

        // Queries for registered enum associated with class or method
        unsigned int GetCimClassId(const SCXInstance& objectPath) const;
        unsigned int GetCimMethodId(const SCXInstance& objectPath, std::wstring cimMethodName) const;
        unsigned int GetCimMethodId(unsigned int cimClassId, std::wstring cimMethodName) const;

        // Predicate if this provider supports given Object Path
        SCXProviderSupportType CheckClassSupport(const SCXInstance& objectPath) const;
        SCXProviderSupportType CheckClassSupport(const SCXInstance& objectPath, unsigned int registeredCimClassID) const;
        
        bool ClassPathIsA(const SCXInstance& objectPath, unsigned int cimClassId) const;
        bool ClassPathIsExact(const SCXInstance& objectPath, unsigned int cimClassId) const;

        size_t GetNumberRegisteredClasses() const;

        // Debug conversion
        std::wstring DumpCimClassName(unsigned int cimClassId) const;
        std::wstring DumpCimMethodName(unsigned int cimMethodId) const;

    private:

        //! Handle to the owning BaseProvider
        const BaseProvider* m_pProvider;
        
        /** Data container for keeping track of registered methods */
        struct MethodInfo 
        {
            //! Ctor 
            MethodInfo(unsigned int cimMethodId, std::wstring cimMethodName) :
                m_cimMethodId(cimMethodId), m_cimMethodName(cimMethodName) {}
            
            unsigned int m_cimMethodId;    //!< Enum value representing the CIM method
            std::wstring m_cimMethodName;  //!< The name of the method
        };


        /** Data container for keeping track of registered classes */
        struct ClassInfo 
        {
            //! Ctor 
            ClassInfo(unsigned int cimClassId, std::wstring cimClassName) :
                m_cimClassId(cimClassId), m_cimClassName(cimClassName) {}
            
            unsigned int m_cimClassId;     //!< Enum value representing CIM class
            std::wstring m_cimClassName;   //!< The name of the CIM class, original caseing

            //! Vector of methods supported by the class
            std::vector<MethodInfo> m_MethodInfo;

        };

        //! Convenience shorthand for ClassInfo iterator
        typedef std::map<std::wstring, ClassInfo>::iterator ClassInfoIterator;
        //! Convenience shorthand for const ClassInfo iterator
        typedef std::map<std::wstring, ClassInfo>::const_iterator ConstClassInfoIterator;
        //! Convenience shorthand for MethodInfo iterator
        typedef std::vector<MethodInfo>::iterator MethodInfoIterator;
        //! Convenience shorthand for const MethodInfo iterator
        typedef std::vector<MethodInfo>::const_iterator ConstMethodInfoIterator;

        unsigned int           GetCimMethodId(ConstClassInfoIterator classIter, std::wstring cimMethodName) const;
        ClassInfoIterator      FindClassById(unsigned int cimClassId); 
        ConstClassInfoIterator FindClassById(unsigned int cimClassId) const;

        //! Store for registered classes, key is class name in lowercase 
        std::map<std::wstring, ClassInfo> m_RegisteredClasses;

        //! Log file handle
        SCXCoreLib::SCXLogHandle m_log;
    };
    
}

#endif /* SCXPROVIDERCAPABILITIES_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
