/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Enumeration of Test Items
    
    \date        07-05-21 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>

#include <scxcorelib/stringaid.h>

#include /*<scxsystemlib/ */<testenumeration.h>
#include /*<scxsystemlib/ */<testinstance.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor
    */
    TestEnumeration::TestEnumeration() : EntityEnumeration<TestInstance>()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.test.testenumeration");    

        SCX_LOGTRACE(m_log, L"TestEnumeration default constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
        Create Test instances
    */
    void TestEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"TestEnumeration Init()");

        SetTotalInstance(SCXHandle<TestInstance>(new TestInstance(0, true)));

        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update all Test data

        Throws: SCXInternalErrorException - If any instance is not a TestInstance

    */
    void TestEnumeration::Update(bool updateInstances)
    {
        size_t count = 5;

        SCX_LOGTRACE(m_log, StrAppend(StrAppend(L"TestEnumeration Update() - ", updateInstances).append(L" - "), count));

        // add items if needed (i.e. currently empty)
        if (0 == Size())
        {
            for (size_t i=Size(); i<count; i++)
            {
                SCX_LOGTRACE(m_log, StrAppend(L"TestEnumeration Update() - Adding Test ", i));
                AddInstance(SCXHandle<TestInstance>(new TestInstance(i)));
            }
        }

        if (updateInstances)
        {
            for (size_t i=0; i<Size(); i++)
            {
                SCXHandle<TestInstance> inst = GetInstance(i);
                
                unsigned int valA = 0; 
                unsigned int valB = 0;  
                
                inst->GetAValue(valA);
                inst->GetBValue(valB);
                inst->UpdateValueA(valA + 1);
                inst->UpdateValueB(valB + 2);
                inst->UpdateValueD();
                inst->UpdateValueE();
                inst->UpdateValueF();
            }
        }
    }

    /**
        Creates a new instance.
        returns the created instance.
    */
    SCXHandle<TestInstance> TestEnumeration::Create(unsigned int instanceNumber)
    {
        SCXHandle<TestInstance> inst(new TestInstance(instanceNumber));
        AddInstance(inst);
        return inst;
    }
}


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

