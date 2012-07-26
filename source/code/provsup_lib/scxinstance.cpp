/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Implementation of SCXProviderLib::SCXInstance
 
    \date      07-07-25 09:25:33
 

*/
/*----------------------------------------------------------------------------*/


#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>

#include <string>

#include <scxproviderlib/scxcmpibaseexceptions.h>
#include <scxproviderlib/scxinstance.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXProviderLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Assignment operator
    
       \param       other  The source object

    */
    SCXInstance& SCXInstance::operator=(const SCXInstance& other) 
    {
        m_cimNamespace = other.m_cimNamespace;
        m_cimClassName = other.m_cimClassName;

        // Thank you for the existence of STL
        m_properties = other.m_properties;
        m_keys = other.m_keys;
        m_filters = other.m_filters;

        return *this;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Get an instance property at the specified position
        
        \param[in]  pos  Position of property to get
        
        \returns         Retrieved property
    */
    const SCXProperty* SCXInstance::GetProperty(size_t pos) const
    { 
        if (pos < m_properties.size()) 
        {
            return &m_properties[pos]; 
        }
        else 
        {
            throw SCXIllegalIndexException<size_t>(L"pos", pos, 0, true, m_properties.size(), true, SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get an instance key at the specified position
        
        \param[in]  pos  Position of key to get

        \returns         Retrieved key
    */
    const SCXProperty* SCXInstance::GetKey(size_t pos) const
    { 
        if (pos < m_keys.size()) 
        {
            return &m_keys[pos]; 
        }
        else 
        {
            throw SCXIllegalIndexException<size_t>(L"pos", pos, 0, true, m_keys.size(), true, SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get an instance property at the specified position
        
        \param[in] pos  Position of property to get

        \return         Retrieved property

        \throws         SCXIllegalIndexExceptionUInt - pos outside num of properties
    */
    const SCXProperty* SCXInstance::operator[](size_t pos) const 
    { 
        return GetProperty(pos); 
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get first instance property with specified name
        
        \param[in]   name  name of property to find

        \returns           Retrieved property or NULL if not found
    */
    const SCXProperty* SCXInstance::GetProperty(const wstring& name) const
    { 
        for (unsigned int i=0; i<m_properties.size(); i++)
        {
            if (m_properties[i].GetName() == name)
            {
                return &m_properties[i];
            }
        }

        return NULL;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get first instance key with specified name
        
        \param[in]  name  name of key to find

        \returns          Retrieved key or NULL if not found
    */
    const SCXProperty* SCXInstance::GetKey(const wstring& name) const
    { 
        for (unsigned int i=0; i<m_keys.size(); i++)
        {
            if (m_keys[i].GetName() == name)
            {
                return &m_keys[i];
            }
        }

        return NULL;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add a property to the instance
        
        \param[in]  prop  Property to add

        \returns          Number of properties in the instance after adding this one
    */
    size_t SCXInstance::AddProperty(const SCXProperty& prop) 
    { 
        m_properties.push_back(prop); 
        return m_properties.size(); 
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add a key to the instance
        
        \param[in]  key  Key to add

        \returns         Number of keys in the instance after adding this one
    */
    size_t SCXInstance::AddKey(const SCXProperty& key) 
    { 
        m_keys.push_back(key); 
        return m_keys.size(); 
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of properties in this instance
        
        \returns      Number of properties in the instance
    */
    size_t SCXInstance::NumberOfProperties() const 
    { 
        return m_properties.size(); 
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of keys in this instance
        
        \returns      Number of keys in the instance
    */
    size_t SCXInstance::NumberOfKeys() const 
    { 
        return m_keys.size(); 
    }


    /*----------------------------------------------------------------------------*/
    /**
        Get a string that shows the content of the instance
        
        \returns      String showing content of instance
    */
    wstring SCXInstance::DumpString() const
    { 
        wstringstream ss;

        ss << "SCXInstance: " ;
        
        if (m_cimNamespace.length()) 
        {
            ss << "Namespace[" << m_cimNamespace << "] CIMClass[" << m_cimClassName << "]";
        }

        if (m_keys.size() > 0)
        {
            ss << "Keys: ";
        }
        for (unsigned int i=0; i<m_keys.size(); i++)
        {
            ss << m_keys[i].DumpString() << " ";
        }

        if (m_properties.size() > 0)
        {
            ss << "Properties: ";
        }
        for (unsigned int i=0; i<m_properties.size(); i++)
        {
            ss << m_properties[i].DumpString() << " ";
        }

        return ss.str();
    }


    size_t SCXInstance::NumberOfFilters() const 
    { 
        return m_filters.size(); 
    }
    
    size_t SCXInstance::AddFilter(const std::wstring filter) 
    { 
        m_filters.push_back(filter); 
        return m_filters.size(); 
    }
    
    const std::wstring* SCXInstance::GetFilter(size_t pos) const
    { 
        if (pos < m_filters.size()) 
        {
            return &m_filters[pos]; 
        }
        else 
        {
            throw SCXIllegalIndexException<size_t>(L"pos", pos, 0, true, m_filters.size(), true, SCXSRCLOCATION);
        }
    }
    
    size_t SCXInstance::DeleteFilter(size_t pos) 
    { 
        if (pos < m_filters.size()) 
        {
            m_filters.erase(m_filters.begin() + pos);
        }
        else 
        {
            throw SCXIllegalIndexException<size_t>(L"pos", pos, 0, true, m_filters.size(), true, SCXSRCLOCATION);
        }
        return m_filters.size(); 
    }
    
    void SCXInstance::ApplyFilter(CMPIInstance* cInst) const
    {
        if(0 == m_filters.size() || NULL == cInst)
            return;
        
        vector<string> temp;
        vector<const char *> cfilter;

        for (vector<wstring>::const_iterator i = m_filters.begin();
             i != m_filters.end();
             ++i)
        {
            temp.push_back(StrToUTF8(*i));
        }
        for (vector<string>::const_iterator i = temp.begin();
             i != temp.end();
             ++i)
        {
            cfilter.push_back(i->c_str());
        }

        cfilter.push_back(NULL);
        CMPIStatus rc = cInst->ft->setPropertyFilter(cInst, const_cast<const char **>(&(cfilter[0])), const_cast<const char **>(&(cfilter[0])));
        
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"setPropertyFilter() Failed - ", rc.rc), SCXSRCLOCATION);
        }
        
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
