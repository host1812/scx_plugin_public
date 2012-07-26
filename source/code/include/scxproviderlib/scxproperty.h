/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief     Definition of SCXProviderLib::SCXProperty

   \date      07-07-24 16:18:48


*/
/*----------------------------------------------------------------------------*/
#ifndef SCXPROPERTY_H
#define SCXPROPERTY_H

#include <string>
#include <vector>
#include <scxcorelib/scxtime.h>

namespace SCXProviderLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Representation of a property.

       Keep name/value pair with type for value. The class behaves like an union
       in that instances can have one data item only at any time.

    */
    class SCXProperty
    {
    public:
        //! Internal enum indicating the type of current value.
        enum SCXType
        {
            SCXStringType,   //!< Carrying wstring data
            SCXIntType,      //!< Carrying integer
            SCXUIntType,     //!< Carrying unsigned int
            SCXULongType,    //!< Carrying scxulong
            SCXBoolType,     //!< Carrying bool
            SCXTimeType,     //!< Carrying scxtime
            SCXFloatType,    //!< Carrying float
            SCXDoubleType,   //!< Carrying double
            SCXUShortType,   //!< Carrying unsigned short
            SCXUCharType,    //!< Carrying unsigned char
            SCXSShortType,   //!< Carrying signed short
            SCXArrayType     //!< Carrying array data
        };

        // Subclasses use these to set properties
        SCXProperty(std::wstring name, const std::wstring& value);
        SCXProperty(std::wstring name, const wchar_t *value);
        SCXProperty(std::wstring name, int value);
        SCXProperty(std::wstring name, signed short value);
        SCXProperty(std::wstring name, unsigned int value);
        SCXProperty(std::wstring name, unsigned short value);
        SCXProperty(std::wstring name, unsigned char value);
        SCXProperty(std::wstring name, scxulong value);
        SCXProperty(std::wstring name, bool value);
        SCXProperty(std::wstring name, const SCXCoreLib::SCXCalendarTime& value);
        SCXProperty(std::wstring name, float value);
        SCXProperty(std::wstring name, double value);
        SCXProperty(std::wstring name, const std::vector<SCXProperty>& value);
        SCXProperty();

        // The CMPI base class uses these to retrieve property information
        const std::wstring& GetName() const;
        void SetName(std::wstring name);

        const std::wstring& GetStrValue() const;
        int             GetIntValue() const;
        unsigned int    GetUIntValue() const;
        signed short    GetSShortValue() const;
        unsigned short  GetUShortValue() const;
        unsigned char   GetUCharValue() const;
        scxulong        GetULongValue() const;
        bool            GetBoolValue() const;
        const SCXCoreLib::SCXCalendarTime& GetTimeValue() const;
        float           GetFloatValue() const;
        double          GetDoubleValue() const;
        const std::vector<SCXProperty>&    GetVectorValue() const;

        // The CMPI base class uses these to set properties before handing over to
        // subclass implementation
        void SetValue(const std::wstring& value);
        void SetValue(const wchar_t *value);
        void SetValue(int value);
        void SetValue(signed short value);
        void SetValue(unsigned int value);
        void SetValue(unsigned short value);
        void SetValue(unsigned char value);
        void SetValue(scxulong value);
        void SetValue(bool value);
        void SetValue(const SCXCoreLib::SCXCalendarTime& value);
        void SetValue(float value);
        void SetValue(double value);
        void SetValue(const std::vector<SCXProperty>& value);

        SCXType GetType() const;

        bool operator==(const SCXProperty& other) const;

        // For tracing etc.
        static std::wstring DumpTypeString(SCXType t);
        std::wstring DumpString() const;

    private:
        void Init(std::wstring name, SCXType type);

    private:
        //! Property name
        std::wstring m_name;

        // Depending on type exactly one of the following is set
        // When adding a type, be sure to remember to update the Init method.
        std::wstring   m_svalue;   //!< wstring
        int            m_ivalue;   //!< int
        signed short   m_ssvalue;  //!< sshort
        unsigned int   m_uivalue;  //!< uint
        unsigned short m_usvalue;  //!< ushort
        unsigned char  m_ucvalue;  //!< uchar
        scxulong       m_ulvalue;  //!< scxulong
        bool           m_bvalue;   //!< bool
        SCXCoreLib::SCXCalendarTime m_tvalue;   //!< scxcalendartime
        float          m_fvalue;   //!< float
        double         m_dvalue;   //!< double
        std::vector<SCXProperty>   m_vvalue;   //!< array

        //! Indicates which of the data fields that have a valid data
        SCXType      m_type;
    };

}

#endif /* SCXPROPERTY_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
