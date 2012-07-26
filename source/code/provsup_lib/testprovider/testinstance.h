/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       < Brief description of purpose of the file > 
    
    \date        07-05-21 12:00:00

    PAL representation of a Test instance
    
*/
/*----------------------------------------------------------------------------*/
#ifndef TESTINSTANCE_H
#define TESTINSTANCE_H

#include <string>

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxtime.h>

namespace SCXSystemLib  
{
    /*----------------------------------------------------------------------------*/
    /**
        Class that represents a colletion of instances.
        
        Concrete implementation of an instance of a Test entity

    */
    class TestInstance : public EntityInstance
    {
        friend class TestEnumeration;

    public:

        TestInstance(unsigned int itemNumber, bool isTotal = false);
        virtual ~TestInstance();

        const std::wstring& GetItemName() const;
        unsigned int GetItemNumber() const;

        // Return values indicate whether the implementation for this platform 
        // supports the value or not. 
        bool GetAValue(unsigned int& aValue) const;
        bool GetBValue(unsigned int& bValue) const;
        bool GetCValue(SCXCoreLib::SCXCalendarTime& cValue) const;
        bool GetDValue(std::vector<unsigned int>& dValue) const;
        bool GetEValue(std::vector<std::wstring>& eValue) const;
        bool GetFValue(std::vector<SCXCoreLib::SCXCalendarTime>& fValue) const;

    private:
        // These are called by concrete subclass of EntityEnumeration through 
        // friendship, i.e. TestEnumeration
        void UpdateValueA(unsigned int aValue);
        void UpdateValueB(unsigned int bValue);
        void UpdateValueD();
        void UpdateValueE();
        void UpdateValueF();

    private:

        std::wstring m_itemName;   //!< Name of this test item
        unsigned int m_itemNumber; //!< Number of this test item

        unsigned int m_valueA;     //!< Test value
        unsigned int m_valueB;     //!< Test value
        SCXCoreLib::SCXCalendarTime m_valueC; //!< Test value for time.
        std::vector<unsigned int> m_valueD;  //!< Test value for arrays.
        std::vector<std::wstring> m_valueE;  //!< Test value for arrays.
        std::vector<SCXCoreLib::SCXCalendarTime> m_valueF;  //!< Test value for arrays.
    };

}

#endif /* TESTINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
