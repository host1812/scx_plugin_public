/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Definition of SCXProviderLib::SCXCallContext
 
    \date      07-07-27 08:14:38
 
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXPROVIDERCALLCTX_H
#define SCXPROVIDERCALLCTX_H


#include <scxproviderlib/scxprovidercapabilities.h>

namespace SCXProviderLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Defines the call context information for providers inheriting from BaseProvider
    
       Providers which require more detailed information regarding the context 
       of a call will find the information in this class, provided in the Do\<Method\>()
       calls. Especially when a provider supports more than one class, it need to 
       decide the exact reason why the provider was called by the BaseProvider 
       functionality. That information is present in an object of this type. 
    
    */
    class SCXCallContext
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Ctor, used by the ProviderBase only
    
           \param       objectPath  The object path reason why this call was invoked
           \param       supportType The type of support the BaseProvider has discovered the Provider has
        */
        SCXCallContext(SCXInstance objectPath, SCXProviderSupportType supportType)
            : m_ObjectPath(objectPath), m_ProviderSupportType(supportType) {}

        //! Return the Object Path supplied by the client 
        const SCXInstance&      GetObjectPath() const { return m_ObjectPath; } 

        //! Return the type of support the provider has for the supplied Object Path
        SCXProviderSupportType  GetSupportType() const { return m_ProviderSupportType; }
        
    private:
        //! Container for the ObjectPath
        SCXInstance             m_ObjectPath;

        //! Container for the Provider support type information 
        SCXProviderSupportType  m_ProviderSupportType;

    };
}

#endif /* SCXPROVIDERCALLCTX_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
