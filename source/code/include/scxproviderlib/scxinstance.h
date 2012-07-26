/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Definition of SCXProviderLib::SCXInstance
 
    \date      07-07-24 16:20:11
 
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXINSTANCE_H
#define SCXINSTANCE_H

#include <vector>

#include <scxproviderlib/scxproperty.h>

#include <Pegasus/Provider/CMPI/cmpidt.h>
#include <Pegasus/Provider/CMPI/cmpift.h>
#include <Pegasus/Provider/CMPI/cmpimacs.h>

namespace SCXProviderLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Representation of an CIM instance.
       
        An instance is a colletion of typed name/value pairs. An instance has two
        sets of properties; ordinary properties and key properties. Both are 
        represented in the same way, by SCXProperty instances. 
       
    */
    class SCXInstance
    {
    public:
        //! Default ctor
        SCXInstance() {};

        SCXInstance& operator=(const SCXInstance& other);

        /*----------------------------------------------------------------------------*/
        /**
            Set namespace of CIM instance.

            \param[in] cimNamespace Namespace to set.

        */
        void   SetCimNamespace(const std::wstring& cimNamespace) { m_cimNamespace = cimNamespace; }

        /*----------------------------------------------------------------------------*/
        /**
            Get namespace of CIM instance.

            \returns Namespace of CIM instance.

        */
        std::wstring GetCimNamespace() const { return m_cimNamespace; }

        /*----------------------------------------------------------------------------*/
        /**
            Set class name of CIM instance.

            \param[in] cimClassName Class name to set.

        */
        void   SetCimClassName(const std::wstring& cimClassName) { m_cimClassName = cimClassName; }

        /*----------------------------------------------------------------------------*/
        /**
            Get class name of CIM instance.

            \returns Class name of CIM instance.

        */
        std::wstring GetCimClassName() const { return m_cimClassName; }
        
        size_t AddProperty(const SCXProperty& prop);
        size_t AddKey(const SCXProperty& key);
        size_t NumberOfProperties() const;
        size_t NumberOfKeys() const;  
        
        const SCXProperty* GetProperty(size_t pos) const;
        const SCXProperty* GetProperty(const std::wstring& name) const;
        const SCXProperty* operator[](size_t pos) const;
        
        const SCXProperty* GetKey(size_t pos) const;
        const SCXProperty* GetKey(const std::wstring& name) const;

        /*----------------------------------------------------------------------------*/
        /**
         *  \fn void ApplyFilter(CMPIInstance* cInst) const
         *  \brief Apply filter to the CMPI instance.  Once a filter is applied further set property calls for the instance would not have an effect unless the property is in the filter.
         *
         *  \param[in] cInst: The CMPI instance to apply the filter to 
         */
        void ApplyFilter(CMPIInstance* cInst) const;

        /*----------------------------------------------------------------------------*/
        /**
         *  \fn size_t AddFilter(const std::wstring filter)
         *  \brief Add the filtering property to the WQL projection filter.
         *
         *  \param[in] filter: The filter to add 
         *  \returns the current number of filtering properties.
         */
        size_t AddFilter(const std::wstring filter);

        /*----------------------------------------------------------------------------*/
        /**
         *  \fn size_t DeleteFilter(size_t p)
         *  \brief Delete the filtering property to the WQL projection filter at position p.
         *
         *  \param[in] p: The position at which the property is to be deleted 
         *  \returns the current number of filtering properties.
         */
        size_t DeleteFilter(size_t p);

        /*----------------------------------------------------------------------------*/
        /**
         *  \fn size_t NumberOfFilters() const
         *  \brief Get the current number of filtering properties.
         *
         *  \returns the current number of filtering properties.
         */
        size_t NumberOfFilters() const;

        /*----------------------------------------------------------------------------*/
        /**
         *  \fn const std::wstring* GetFilter(size_t pos) const
         *  \brief Get the name of filtering property by position.
         *
         *  \param[in] pos: The position at which the property is to be returned 
         *  \returns the pointer to the name of filtering property at the specified position.
         */
        const std::wstring* GetFilter(size_t pos) const;

        std::wstring DumpString() const;
        
    private:
        //! The CIM namespace of this instance
        std::wstring             m_cimNamespace;
        //! The CIM Class name of this instance
        std::wstring             m_cimClassName;
        
        //! Vector containing all non-key properties
        std::vector<SCXProperty> m_properties;
        //! Vector containing all key properties
        std::vector<SCXProperty> m_keys;
        //! Vector containing the properties needed for WQL projection
        std::vector<std::wstring> m_filters;
    };
}
    
#endif /* SCXINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
