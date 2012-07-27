/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 

*/
/**
    \file        

    \brief       PAL representation of a Test Instance

    \date        07-05-21 12:00:00

   
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <string>
#include <sstream>
#include <vector>

#include <scxcorelib/stringaid.h>

#include /*<scxsystemlib/ */<wkinstance.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Constructor
        
        \param[in] itemNumber Number of item, used as base for instance name
        \param[in] isTotal Whether the instance represents a Total value of a collection

    */
    TestInstance::TestInstance(unsigned int itemNumber, bool isTotal) : EntityInstance(isTotal)
        , m_valueA(0)
        , m_valueB(0)
        , m_valueC(SCXCoreLib::SCXCalendarTime::CurrentLocal())
        , m_valueD()
        , m_valueE()
        , m_valueF()
    {
        if (isTotal)
        {
            m_itemName = L"_Total";
        }
        else
        {
            // The name of an instance is the number
            m_itemName = StrFrom(itemNumber);
        }

        m_itemNumber = itemNumber;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Destructor

    */
    TestInstance::~TestInstance()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get item name
        
        \returns      name of processor instance

    */
    const wstring& TestInstance::GetItemName() const
    {
        return m_itemName;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get item number
        
        \returns      number of processor instance

    */
    unsigned int TestInstance::GetItemNumber() const
    {
        return m_itemNumber;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get fictive value A
        
        \param[out]  aValue   fictive value A

        \returns     true if A value is supported by the implementation 

    */
    bool TestInstance::GetAValue(unsigned int& aValue) const
    {
        aValue = m_valueA;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get fictive value B
        
        \param[out]  bValue   fictive value B

        \returns     true if B value is supported by this implementation

    */
    bool TestInstance::GetBValue(unsigned int& bValue) const
    {
        bValue = m_valueB;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get fictive value C
        
        \param[out]  cValue   fictive value C.

        \returns     true if C value is supported by this implementation

    */
    bool TestInstance::GetCValue(SCXCoreLib::SCXCalendarTime& cValue) const
    {
        cValue = m_valueC;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get fictive value D
        
        \param[out]  dValue   fictive value D

        \returns     true if D value is supported by this implementation

    */
    bool TestInstance::GetDValue(std::vector<unsigned int>& dValue) const
    {
        dValue = m_valueD;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get fictive value E
        
        \param[out]  eValue   fictive value E

        \returns     true if E value is supported by this implementation

    */
    bool TestInstance::GetEValue(std::vector<std::wstring>& eValue) const
    {
        eValue = m_valueE;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get fictive value F
        
        \param[out]  fValue   fictive value F

        \returns     true if F value is supported by this implementation

    */
    bool TestInstance::GetFValue(std::vector<SCXCoreLib::SCXCalendarTime>& fValue) const
    {
        fValue = m_valueF;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update a fictive value A
        
        \param[in]  aValue The value to set

    */
    void TestInstance::UpdateValueA(unsigned int aValue) // private
    {
        m_valueA = aValue;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update a fictive value B
        
        \param[in]  bValue The value to set

    */
    void TestInstance::UpdateValueB(unsigned int bValue) // private
    {
        m_valueB = bValue;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update a fictive value D
        
    */
    void TestInstance::UpdateValueD() // private
    {
        m_valueD.push_back(m_valueA);
        if (m_valueD.size() > 5)
        {
            m_valueD.clear();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update a fictive value E
        
    */
    void TestInstance::UpdateValueE() // private
    {
        m_valueE.push_back(SCXCoreLib::StrAppend(L"Value", m_valueA));
        if (m_valueE.size() > 5)
        {
            m_valueE.clear();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update a fictive value F
        
    */
    void TestInstance::UpdateValueF() // private
    {
        SCXCoreLib::SCXCalendarTime value(SCXCoreLib::SCXCalendarTime::CurrentLocal());
        m_valueF.push_back(value);
        if (m_valueF.size() > 5)
        {
            m_valueF.clear();
        }
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
