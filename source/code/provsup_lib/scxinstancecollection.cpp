/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     SCXInstanceCollection implementation 
 
    \date      07-07-25 09:24:13
 
    < Optional detailed description of file purpose >  
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxproviderlib/scxinstancecollection.h>
#include <scxproviderlib/scxcmpibaseexceptions.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXProviderLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Add instance to collection
        
        \param[in]  inst  Instance to add

        \returns          Number of instances in the collection after adding this one
    */
    size_t SCXInstanceCollection::AddInstance(const SCXInstance& inst) 
    { 
        m_instances.push_back(inst); 
        return m_instances.size(); 
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get an instance at the specified position
        
        \param[in]  pos  Position of instance to get

        \returns         Retrieved instance
        \throws          SCXIllegalIndexExceptionUInt  Illegal pos

    */
    const SCXInstance* SCXInstanceCollection::GetInstance(size_t pos) const
    { 
        if (pos < m_instances.size()) 
        {
            return &m_instances[pos]; 
        }
        else 
        {
            throw SCXIllegalIndexException<size_t>(L"pos", pos, 0, true, m_instances.size(), true, SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get an instance at the specified position
        
        \param[in]  pos  Position of instance to get

        \returns         Retrieved instance
        \throws          SCXIllegalIndexExceptionUInt - Illegal pos

    */
    const SCXInstance* SCXInstanceCollection::operator[](size_t pos) const
    { 
        return GetInstance(pos); 
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of instances in this collection
        
        \returns       Number of instances in the collection
    */
    size_t SCXInstanceCollection::Size() const
    { 
        return m_instances.size(); 
    }

    /*----------------------------------------------------------------------------*/
    /**
        Clear all instances in this collection
    */
    void SCXInstanceCollection::clear()
    { 
        m_instances.clear();
    }
}


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
