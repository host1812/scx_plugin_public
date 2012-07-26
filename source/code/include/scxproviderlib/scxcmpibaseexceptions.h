/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Exceptions thrown from the Provider Template
 
    \date      07-07-25 09:19:12
 

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXCMPIBASEEXCEPTIONS_H
#define SCXCMPIBASEEXCEPTIONS_H

#include <string>

#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>

#include <scxproviderlib/scxinstance.h>

namespace SCXProviderLib
{

    /*----------------------------------------------------------------------------*/
    /**
       SCXCMPIException is a base exception for all CMPI-related exceptions
      
       \date          2007-06-20 15:40:00
      
       The standard SCXExceptions are also used in the framework.

    */
    class SCXCMPIException : public SCXCoreLib::SCXException {
    public: 
        //! Ctor
        SCXCMPIException(const SCXCoreLib::SCXCodeLocation& l) 
            : SCXException(l) {};
        
    protected:
    }; 

 
    /*----------------------------------------------------------------------------*/
    /**
      Exception indicating that a query is invalid
      
      \date          2007-11-21 13:16:00
      
    */
    class SCXCIMInvalidQuery : public SCXCMPIException 
    {
    public:
        //! Ctor
        SCXCIMInvalidQuery(const std::wstring& query,
                                   const std::wstring& language,
                           const SCXCoreLib::SCXCodeLocation& location)
            : SCXCMPIException(location), m_Query(query), m_Language(language) {};

        //! Format message
        std::wstring What() const { return L"Invalid query " + m_Query + L", language " + m_Language; }
    private:
        //! The invalid query
        std::wstring m_Query; 
        //! Stated, that is, expected language for query
        std::wstring m_Language;
    };            

    
    /*----------------------------------------------------------------------------*/
    /**
      Exception indicating that a provider does not support a requested language
      
      \date          2007-11-21 13:16:00
      
    */
    class SCXCIMQueryLanguageNotSupported : public SCXCMPIException 
    {
    public:
        //! Ctor
        SCXCIMQueryLanguageNotSupported(const std::wstring& language,
                                                                const SCXCoreLib::SCXCodeLocation& location)
            : SCXCMPIException(location), m_Language(language) {};

        //! Format message
        std::wstring What() const { return L"Language " + m_Language + L" not supported in requested context";}
    private:
        //! Stated, that is, expected language for query
        std::wstring m_Language;
    };            

    
    /*----------------------------------------------------------------------------*/
    /**
      Exception indicating that CIM instance with given keys cannot be found
      
      \date          2007-06-20 15:42:00
      
    */
    class SCXCIMInstanceNotFound : public SCXCMPIException 
    {
    public:
        //! Ctor
        SCXCIMInstanceNotFound(const std::wstring& keyInfo,
                               const SCXCoreLib::SCXCodeLocation& l)
            : SCXCMPIException(l), m_KeyInfo(keyInfo) {};

        //! Format message
        std::wstring What() const { return L"Cannot find instance with keys " + m_KeyInfo; }
    private:
        //! String with readable information about requested key details. Must be a string
        //! to be generic (should have been a key structure).
        std::wstring m_KeyInfo;
    };            


    /*----------------------------------------------------------------------------*/
    /**
      Exeception when attempting to create an already existing instance
      
      \date          2007-06-20 15:42:00

      This might be thrown by DoCreateInstance(). 
      
    */
    class SCXCIMInstanceAlreadyExists : public SCXCMPIException 
    {
    public:
        //! Ctor
        SCXCIMInstanceAlreadyExists(const SCXProviderLib::SCXInstance& instance,
                                    const SCXCoreLib::SCXCodeLocation& l)
            : SCXCMPIException(l), m_ViolatingInstance(instance) {};

        //! Format message
        std::wstring What() const { return L"CIM Instance already exists: " + m_ViolatingInstance.DumpString(); }
    private:
        //! The instance that could not be created
        SCXProviderLib::SCXInstance m_ViolatingInstance;
    };            

    /*----------------------------------------------------------------------------*/
    /**
      Exeception when creating or manipulating an instance fails 
      
      \date          2007-06-20 15:42:00

      This might be thrown by DoCreateInstance() or DoModifyInstance(). 
      
    */
    class SCXCIMInstanceManipError : public SCXCMPIException 
    {
    public:
        //! Ctor
        SCXCIMInstanceManipError(const SCXProviderLib::SCXInstance& instance,
                                 const std::wstring& reason,
                                 const SCXCoreLib::SCXCodeLocation& l)
            : SCXCMPIException(l), m_ViolatingInstance(instance), m_Reason(reason) {};

        //! Format message
        std::wstring What() const { return L"Instance operation failed: " + m_Reason + 
                                        L". Instance information: " + m_ViolatingInstance.DumpString(); }
    private:
        //! The instance that could not be created
        SCXProviderLib::SCXInstance m_ViolatingInstance;

        //! Details on the problem
        std::wstring                m_Reason;
    };            

    /*----------------------------------------------------------------------------*/
    /**
       Exeception when there are problems with the SCXProperty concerning vectors
      
       \date          2008-03-27 14:07:23

       This might be thrown by SCXPropertyToCMPIValue()
      
    */
    class SCXPropertyVectorError : public SCXCMPIException 
    {
    public:
        //! Ctor
        SCXPropertyVectorError(const SCXProviderLib::SCXProperty& property,
                               const std::wstring& reason,
                               const SCXCoreLib::SCXCodeLocation& l)
            : SCXCMPIException(l), m_ViolatingProperty(property), m_Reason(reason)
        { SCXASSERTFAIL((l.Where() + What()).c_str()); };

        //! Format message
        std::wstring What() const { return L"SCXProperty vector error: " + m_Reason + 
                                        L". Property information: " + m_ViolatingProperty.DumpString(); }
    private:
        //! The instance that could not be created
        SCXProviderLib::SCXProperty m_ViolatingProperty;

        //! Details on the problem
        std::wstring                m_Reason;
    };            


    /******************************************************************************
     *  
     *   Exceptions belonging to SCXProviderCapabilities lookup etc.
     *  
     ******************************************************************************/

    /*----------------------------------------------------------------------------*/
    /**
       Base exception for all exceptions related to SCXProviderCapabilities
    
    */
    class SCXProviderCapabilitiesException : public SCXCoreLib::SCXException {
    public: 
        //! Ctor
        SCXProviderCapabilitiesException(const SCXCoreLib::SCXCodeLocation& l) 
            : SCXException(l) {};
        
    };

    /*----------------------------------------------------------------------------*/
    /**
      Exception indicating that request for not registered entity has been made
      
    */
    class SCXProvCapNotRegistered : public SCXProviderCapabilitiesException 
    {
    public:
        //! Ctor
        SCXProvCapNotRegistered(const std::wstring& capability,
                                const std::wstring& type,
                                const SCXCoreLib::SCXCodeLocation& l)
            : SCXProviderCapabilitiesException(l), m_Capability(capability), m_Type(type) {};

        //! Ctor for numeric representation of the capability (class or method)
        SCXProvCapNotRegistered(unsigned int        id,
                                const std::wstring& type,
                                const SCXCoreLib::SCXCodeLocation& l)
            : SCXProviderCapabilitiesException(l), m_Type(type) 
        {
            m_Capability = SCXCoreLib::StrAppend(L"id ", id);
        };

        //! Format message
        std::wstring What() const { return L"Provider has no " + m_Type + L" registration for " + m_Capability; }
    private:
        //! Requested capability
        std::wstring m_Capability;
        
        //! Type of requested capability (CIM class or method)
        std::wstring m_Type;
    };            

}


#endif /* SCXCMPIBASEEXCEPTIONS_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
