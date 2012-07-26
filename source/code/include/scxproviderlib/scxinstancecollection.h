/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Definition of SCXProviderLib::SCXInstanceCollection
 
    \date      07-07-25 09:16:28
 

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXINSTANCECOLLECTION_H
#define SCXINSTANCECOLLECTION_H

#include <string>

#include <scxproviderlib/scxinstance.h>


namespace SCXProviderLib
{
    
    /*----------------------------------------------------------------------------*/
    /**
       Representation of a collection of instances.
       
       This class is used for enumeration when a colletion of instances is returned
       
    */
    class SCXInstanceCollection
    {
    public:
        //! Default Ctor
        SCXInstanceCollection() {};
        
        size_t AddInstance(const SCXInstance& inst);
        size_t Size() const;
        const SCXInstance* GetInstance(size_t pos) const;
        const SCXInstance* operator[](size_t pos) const;
        void clear();
        
    private:
        //! A vector of SCXInstance instances
        std::vector<SCXInstance> m_instances;
    };
}

#endif /* SCXINSTANCECOLLECTION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
