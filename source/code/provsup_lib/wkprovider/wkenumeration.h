/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 

*/
/**
    \file

    \brief          Enumeration of Test Items
    \date           07-05-21 12:00:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef TESTENUMERATION_H
#define TESTENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include /*<scxsystemlib/ */<wkinstance.h>
#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Class that represents a colletion of Test:s.
        
        PAL Holding collection of Test:s.
    */
    class TestEnumeration : public EntityEnumeration<TestInstance>
    {
    public:

        TestEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances=true);

        SCXCoreLib::SCXHandle<TestInstance> Create(unsigned int instanceNumber);

    private:
        //! Handle to log file 
        SCXCoreLib::SCXLogHandle m_log;
    };

}

#endif /* TESTENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
