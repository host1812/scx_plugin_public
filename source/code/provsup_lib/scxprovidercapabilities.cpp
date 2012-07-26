/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Implementation of SCXProviderLib::SCXProviderCapabilities
 
    \date      07-07-24 14:21:38
 
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/stringaid.h>

#include <scxproviderlib/cmpibase.h>
#include <scxproviderlib/scxcmpibaseexceptions.h>
#include <scxproviderlib/scxprovidercapabilities.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXProviderLib
{
    
    /*----------------------------------------------------------------------------*/
    /**
        Constructor

        \param[in] pProvider Pointer to BaseProvider.

    */
    SCXProviderCapabilities::SCXProviderCapabilities(BaseProvider* pProvider) 
        : m_pProvider(pProvider) 
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.providercap");    
    }

    /*----------------------------------------------------------------------------*/
    /**
        Register a CIM class as supported by current provider
    
        \param[in]     cimClassId   Enum value with unique key for this class
        \param[in]     cimClassName Name of the class 

        \pre The cimClassName has not been added already
       
        A provider must assign a unique ID (from an enum) with a CIM name, 
        corresponding to the CIM name in the registration MOF. This name will 
        then be used by both the BaseProvider and the actual provider to see
        if an incoming request can be serviced. 
    
    */
    void SCXProviderCapabilities::RegisterCimClass(unsigned int cimClassId, std::wstring cimClassName)
    {
        wstring key = StrToLower(cimClassName);

        // No duplicates allowed
        SCXASSERT(m_RegisteredClasses.find(key) == m_RegisteredClasses.end());

        m_RegisteredClasses.insert(pair<wstring, ClassInfo>(key, ClassInfo(cimClassId, cimClassName)));
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove all registered CIM classes

    */
    void SCXProviderCapabilities::Clear()
    {
        m_RegisteredClasses.clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Register supported methods for each supported class
    
        \param[in]   cimClassId  Enum value registered in RegisterCimClass()
        \param[in]   cimMethodId Enum value uniquely identifying a method 
        \param[in]   cimMethodName Name of the medhod as declared in the MOF declaration

        \pre The \a cimClassId should be registered 

        This method should be called for each method on each supported class to 
        register it with the framework. A method name registered with one Id on 
        one class should not be registered with another class under another 
        name (this will assert()).
       
    */    
    void SCXProviderCapabilities::RegisterCimMethod(unsigned int cimClassId, 
                                                    unsigned int cimMethodId, 
                                                    std::wstring cimMethodName)
    {
        ClassInfoIterator iter = FindClassById(cimClassId);

        if (iter == m_RegisteredClasses.end())
        {
            SCXASSERT(!"Attempt to register method on unregistered class");
            throw SCXProvCapNotRegistered(cimClassId, L"class", SCXSRCLOCATION);
        }

        // Got the proper class, store the method info in this class 
        iter->second.m_MethodInfo.push_back(MethodInfo(cimMethodId, cimMethodName));
    }
        

    /*----------------------------------------------------------------------------*/
    /**
        Return the id of the class in the key
    
        \param[in]   objectPath  Object Path to lookup
        \returns     Integer (enum value) which was used to register the class in question
       
        \throws      SCXProvCapNotRegistered The requested \a objectPath is not registered
    
        Lookup if the class is known to this provider, and return the enumeration value
        in that case. 
    
    */
    unsigned int SCXProviderCapabilities::GetCimClassId(const SCXInstance& objectPath) const
    {
        SCXASSERT(objectPath.GetCimClassName().length());

        ConstClassInfoIterator classInfo = m_RegisteredClasses.find(StrToLower(objectPath.GetCimClassName()));
        if (classInfo == m_RegisteredClasses.end())
        {
            throw SCXProvCapNotRegistered(objectPath.GetCimClassName(), L"class", SCXSRCLOCATION);
        }

        return classInfo->second.m_cimClassId;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Return the id of the method in the key for the object path
    
        \param[in]   objectPath  Object Path to lookup
        \param[in]   cimMethodName  Name of method to lookup
        \returns     Integer (enum value) which was used to register the class in question
       
        \throws      SCXProvCapNotRegistered The requested \a objectPath is not registered, or not registered for this class
    
        Lookup if the class is known to this provider, and return the enumeration value
        in that case. 
    
    */
    unsigned int SCXProviderCapabilities::GetCimMethodId(const SCXInstance& objectPath, 
                                                         std::wstring cimMethodName) const
    {
        SCXASSERT(objectPath.GetCimClassName().length());

        ConstClassInfoIterator classInfoIter = m_RegisteredClasses.find(StrToLower(objectPath.GetCimClassName()));
        if (classInfoIter == m_RegisteredClasses.end())
        {
            throw SCXProvCapNotRegistered(objectPath.GetCimClassName(), L"class", SCXSRCLOCATION);
        }
        // Got the class ID OK, now find method
        return GetCimMethodId(classInfoIter, cimMethodName);
    }

    /**
        \overload 
     */
    unsigned int SCXProviderCapabilities::GetCimMethodId(unsigned int cimClassId, 
                                                         std::wstring cimMethodName) const
    {
        ConstClassInfoIterator classInfoIter = FindClassById(cimClassId);
        if (classInfoIter == m_RegisteredClasses.end())
        {
            throw SCXProvCapNotRegistered(cimClassId, L"class", SCXSRCLOCATION);
        }

        return GetCimMethodId(classInfoIter, cimMethodName);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Internal method for finding Method Id once there is a Class iterator 
    
        \param[in]     classInfoIter  Iterator pointing at the ClassInfo we are interested in 
        \param[in]     cimMethodName Name of method to lookup
       
        \returns       Corresponding Method Id 
       
        \throws      SCXProvCapNotRegistered The requested \a objectPath is not registered, or not registered for this class
    
    */
    unsigned int SCXProviderCapabilities::GetCimMethodId(ConstClassInfoIterator classInfoIter, 
                                                         std::wstring cimMethodName) const // private 
    {
        // Got the class ID OK, now find method
        vector<MethodInfo>::const_iterator methodInfo = classInfoIter->second.m_MethodInfo.begin();
        while (methodInfo != classInfoIter->second.m_MethodInfo.end())
        {
            if (StrToLower(methodInfo->m_cimMethodName) == StrToLower(cimMethodName))
            {
                return methodInfo->m_cimMethodId;
            }
            methodInfo++;
        }
        // Not found
        wstring logmsg = L"Failed to find registration for method " + cimMethodName;
        SCX_LOGWARNING(m_log, logmsg);

        throw SCXProvCapNotRegistered(cimMethodName, L"method", SCXSRCLOCATION);
    }



    /*----------------------------------------------------------------------------*/
    /**
        Predicate telling wether the Object Path represents an instance or subclass of supported class
    
        \param[in]   objectPath Instance representing an Object Path
        \param[in]   cimClassId Enumeration value representing 

        \returns     true if \a cimClassId is an instance of cimClassId or a subclass, false otherwise

        \throws      SCXProvCapNotRegistered The cimClassId not registered 

    */
    bool SCXProviderCapabilities::ClassPathIsA(const SCXInstance& objectPath, unsigned int cimClassId) const
    {
        CMPIStatus rc = { CMPI_RC_OK, NULL };

        // This will throw if cimClassId is unknown
        wstring classNameFromId = DumpCimClassName(cimClassId);

        // Create an object path with CMPI datatype
        CMPIObjectPath* pCmpiObjectPath = CMNewObjectPath(m_pProvider->GetBrokerHandle(),
                                                          StrToUTF8(objectPath.GetCimNamespace()).c_str(),
                                                          StrToUTF8(objectPath.GetCimClassName()).c_str(),
                                                          &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXResourceExhaustedException(L"CMPI Object Path", StrAppend(L"CMNewObjectPath() failed - ", rc.rc), 
                                                SCXSRCLOCATION);
        }

        bool classok = CMClassPathIsA(m_pProvider->GetBrokerHandle(), 
                                      pCmpiObjectPath, 
                                      StrToUTF8(classNameFromId).c_str(), &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMClassPathIsA() Failed - ", rc.rc), SCXSRCLOCATION);
        }
        
        return classok;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Predicate whether an Object Path exactly matches a registered CIM class
    
        \param[in]    objectPath Object Path to check 
        \param[in]    cimClassId The enumeration value to compare with

        \returns      Boolean if the instance exactly represents the cimClassId

        \throws      SCXProvCapNotRegistered The cimClassId not registered 
       
        Predicate. 

    */
    bool SCXProviderCapabilities::ClassPathIsExact(const SCXInstance& objectPath, unsigned int cimClassId) const
    {
        unsigned int lookupCimClassId = GetCimClassId(objectPath);
        
        return cimClassId == lookupCimClassId;
    }
    

    /*----------------------------------------------------------------------------*/
    /**
        Predicate whether the supplied Object Path is supported by the provider
    
        \param[in]     objectPath Object Path to check
        \returns       A value of type SCXProviderCapabilities::ClassSupportType
    
        Check if the Object Path in \a instance represents a supported, i.e. registered
        CIM class. 
    
    */
    SCXProviderSupportType SCXProviderCapabilities::CheckClassSupport(const SCXInstance& objectPath) const
    {
        try
        {
            // Just check for direct support (if class is registered)
            GetCimClassId(objectPath);
            // Got a match
            return eDirectSupport;
        }
        catch (SCXProvCapNotRegistered)
        {
            // OK, check if this is a superclass of any of the supported
            // NOT YET IMPLEMENTED: WI 941
            return eNoSupport;
        }

        return eNoSupport;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Predicate whether the supplied Object Path is supported by the provider
    
        \param[in]     objectPath Object Path to check
        \param[in]     registeredCimClassID Class ID to check
        \returns       A value of type SCXProviderCapabilities::ClassSupportType
    
        Check if the Object Path in \a instance represents a supported, i.e. registered
        CIM class. 
    
    */
    SCXProviderSupportType SCXProviderCapabilities::CheckClassSupport(const SCXInstance& /* objectPath */, 
                                                                      unsigned int /* registeredCimClassID */) const
    {
        // NOT YET IMPLEMENTED: WI 941
        SCXASSERT( !"SCXProviderCapabilities::CheckClassSupport() for specific classID not implemented, WI 941");
        return eNoSupport;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Return the number of registered classes
    
        \returns     Number of registered classes

    */
    size_t SCXProviderCapabilities::GetNumberRegisteredClasses() const 
    {
        return m_RegisteredClasses.size();
    }


    /*----------------------------------------------------------------------------*/
    /**
        Dump the class name originally registered for this enumeration value
    
        \param[in]     cimClassId  Id to lookup
        \returns       String with registered name

        \throws        SCXProvCapNotRegistered The cimClassId not registered 
    
        Returns the name used at registration of the \a cimClassId. 
    
    */
    std::wstring SCXProviderCapabilities::DumpCimClassName(unsigned int cimClassId) const
    {
        ConstClassInfoIterator iter = FindClassById(cimClassId);
        if (iter == m_RegisteredClasses.end())
        {
            throw SCXProvCapNotRegistered(cimClassId, L"class", SCXSRCLOCATION);
        }
        return iter->second.m_cimClassName;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Dump the method name originally registered for this enumeration value
    
        \param[in]     cimMethodId  Id to lookup
        \returns       String with registered name

        \throws        SCXProvCapNotRegistered The cimMethodId not registered 
    
        The method is currently only intended for tracing and is not very efficient. 
    
    */
    std::wstring SCXProviderCapabilities::DumpCimMethodName(unsigned int cimMethodId) const
    {
        ConstClassInfoIterator constClassIter = m_RegisteredClasses.begin();
        while (constClassIter != m_RegisteredClasses.end())
        {
            ConstMethodInfoIterator constMethodIter = constClassIter->second.m_MethodInfo.begin();
            while (constMethodIter != constClassIter->second.m_MethodInfo.end())
            {
                if (constMethodIter->m_cimMethodId == cimMethodId)
                {
                    return constMethodIter->m_cimMethodName;
                }
                constMethodIter++;
            }
            constClassIter++;
        }
        // Out of luck
        throw SCXProvCapNotRegistered(cimMethodId, L"method", SCXSRCLOCATION);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Return an iterator pointing at the ClassInfo with given Id
    
        \param       cimClassId  The Id to look for 
        \returns     Iterator pointing at the ClassInfo, or m_RegisteredClasses.end()

        This method comes in two variants, one returning a const iterator and 
        one non-const. 

    */
    SCXProviderCapabilities::ClassInfoIterator SCXProviderCapabilities::FindClassById(unsigned int cimClassId) // private
    {
        ClassInfoIterator iter = m_RegisteredClasses.begin();
        while (iter != m_RegisteredClasses.end())
        {
            if (cimClassId == iter->second.m_cimClassId)
            {
                return iter;
            }
            iter++;
        }
        return iter;
    }

    /** 
        \overload
    */
    SCXProviderCapabilities::ConstClassInfoIterator SCXProviderCapabilities::FindClassById(unsigned int cimClassId)  const // private
    {
        ConstClassInfoIterator iter = m_RegisteredClasses.begin();
        while (iter != m_RegisteredClasses.end())
        {
            if (cimClassId == iter->second.m_cimClassId)
            {
                return iter;
            }
            iter++;
        }
        return iter;
    }
}
    
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
    
