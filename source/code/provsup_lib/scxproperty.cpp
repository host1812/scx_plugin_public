/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief     SCXProviderLib::SCXProperty implementation

   \date      07-07-25 09:22:38


*/
/*----------------------------------------------------------------------------*/


#include <scxcorelib/scxcmn.h>

#include <string>
#include <sstream>

#include <scxcorelib/scxdumpstring.h>
#include <scxproviderlib/scxproperty.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXProviderLib
{

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for string property

       \param[in]  name   Property name
       \param[in]  value  Property value
    */
    SCXProperty::SCXProperty(wstring name, const wstring& value)
    {
        Init(name, SCXStringType);
        m_svalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for string property (wchar_t variant)

       \param[in]  name   Property name
       \param[in]  value  Property value
    */
    SCXProperty::SCXProperty(wstring name, const wchar_t *value)
    {
        Init(name, SCXStringType);
        m_svalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for integer property

       \param[in]   name   Property name
       \param[in]   value  Property value
    */
    SCXProperty::SCXProperty(wstring name, int value)
    {
        Init(name, SCXIntType);
        m_ivalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for signed short property

       \param[in]   name   Property name
       \param[in]   value  Property value
    */
    SCXProperty::SCXProperty(wstring name, signed short value)
    {
        Init(name, SCXSShortType);
        m_ssvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for unsigned integer property

       \param[in]   name   Property name
       \param[in]   value  Property value
    */
    SCXProperty::SCXProperty(wstring name, unsigned int value)
    {
        Init(name, SCXUIntType);
        m_uivalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for unsigned short property

       \param[in]   name   Property name
       \param[in]   value  Property value
    */
    SCXProperty::SCXProperty(wstring name, unsigned short value)
    {
        Init(name, SCXUShortType);
        m_usvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for unsigned char property

       \param[in]   name   Property name
       \param[in]   value  Property value
    */
    SCXProperty::SCXProperty(wstring name, unsigned char value)
    {
        Init(name, SCXUCharType);
        m_ucvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for unsigned long property

       \param[in]   name   Property name
       \param[in]   value  Property value
    */
    SCXProperty::SCXProperty(wstring name, scxulong value)
    {
        Init(name, SCXULongType);
        m_ulvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for bool property

       \param[in]   name   Property name
       \param[in]   value  Property value
    */
    SCXProperty::SCXProperty(wstring name, bool value)
    {
        Init(name, SCXBoolType);
        m_bvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for time property

       \param[in]  name   Property name
       \param[in]  value Property value
    */
    SCXProperty::SCXProperty(wstring name, const SCXCalendarTime& value)
    {
        Init(name, SCXTimeType);
        m_tvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for float property

       \param[in]  name   Property name
       \param[in]  value Property value
    */
    SCXProperty::SCXProperty(wstring name, float value)
    {
        Init(name, SCXFloatType);
        m_fvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for double property

       \param[in]  name   Property name
       \param[in]  value Property value
    */
    SCXProperty::SCXProperty(wstring name, double value)
    {
        Init(name, SCXDoubleType);
        m_dvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor for array property

       \param[in]  name   Property name
       \param[in]  value  Property value
    */
    SCXProperty::SCXProperty(wstring name, const vector<SCXProperty>& value)
    {
        Init(name, SCXArrayType);
        m_vvalue = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default Constructor
    */
    SCXProperty::SCXProperty()
    {
        Init(L"", SCXStringType);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set all member variables to default values

       \param[in]  name   Property name
       \param[in]  type   Property type
    */
    void SCXProperty::Init(std::wstring name, SCXType type)
    {
        m_name = name;
        m_svalue = L"";
        m_ivalue = 0;
        m_ssvalue = 0;
        m_uivalue = 0;
        m_usvalue = 0;
        m_ulvalue = 0;
        m_bvalue = false;
        m_fvalue = 0;
        m_dvalue = 0;
        m_ucvalue = 0;
        m_type = type;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve property name

       \returns property name
    */
    const wstring& SCXProperty::GetName() const
    {
        SCXASSERT(m_name.length());
        return m_name;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property name

       \param[in]   name  Property name
    */
    void SCXProperty::SetName(wstring name)
    {
        m_name = name;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Get property string value

       \returns  Property value
    */
    const wstring& SCXProperty::GetStrValue() const
    {
        SCXASSERT( SCXStringType == m_type );

        return m_svalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property integer value

       \returns     Property value
    */
    int SCXProperty::GetIntValue() const
    {
        SCXASSERT( SCXIntType == m_type );
        return m_ivalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property signed short value

       \returns      Property value

    */
    signed short SCXProperty::GetSShortValue() const
    {
        SCXASSERT(SCXSShortType == m_type);
        return m_ssvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property unsigned integer value

       \returns      Property value

    */
    unsigned int SCXProperty::GetUIntValue() const
    {
        SCXASSERT(SCXUIntType == m_type);
        return m_uivalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property unsigned short value

       \returns      Property value

    */
    unsigned short SCXProperty::GetUShortValue() const
    {
        SCXASSERT(SCXUShortType == m_type);
        return m_usvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property unsigned char value

       \returns      Property value

    */
    unsigned char SCXProperty::GetUCharValue() const
    {
        SCXASSERT(SCXUCharType == m_type);
        return m_ucvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property unsigned long value

       \returns      Property value
    */
    scxulong SCXProperty::GetULongValue() const
    {
        SCXASSERT(SCXULongType == m_type);
        return m_ulvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property bool value

       \returns     Property value
    */
    bool SCXProperty::GetBoolValue() const
    {
        SCXASSERT(SCXBoolType == m_type);
        return m_bvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property time value

       \returns     Property value
    */
    const SCXCalendarTime& SCXProperty::GetTimeValue() const
    {
        SCXASSERT(SCXTimeType == m_type);
        return m_tvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property float value

       \returns     Property value
    */
    float SCXProperty::GetFloatValue() const
    {
        SCXASSERT(SCXFloatType == m_type);
        return m_fvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property double value

       \returns     Property value
    */
    double SCXProperty::GetDoubleValue() const
    {
        SCXASSERT(SCXDoubleType == m_type);
        return m_dvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property vector value

       \returns  Property value
    */
    const vector<SCXProperty>& SCXProperty::GetVectorValue() const
    {
        SCXASSERT( SCXArrayType == m_type );

        return m_vvalue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property string value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(const wstring& value)
    {
        m_svalue = value;
        m_type = SCXStringType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property string value (wchar_t variant)

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(const wchar_t *value)
    {
        m_svalue = std::wstring(value);
        m_type = SCXStringType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property integer value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(int value)
    {
        m_ivalue = value;
        m_type = SCXIntType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property signed short value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(signed short value)
    {
        m_ssvalue = value;
        m_type = SCXSShortType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property unsigned integer value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(unsigned int value)
    {
        m_uivalue = value;
        m_type = SCXUIntType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property unsigned short value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(unsigned short value)
    {
        m_usvalue = value;
        m_type = SCXUShortType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property unsigned char value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(unsigned char value)
    {
        m_ucvalue = value;
        m_type = SCXUCharType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property unsigned long value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(scxulong value)
    {
        m_ulvalue = value;
        m_type = SCXULongType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property bool value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(bool value)
    {
        m_bvalue = value;
        m_type = SCXBoolType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property time value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(const SCXCalendarTime& value)
    {
        m_tvalue = value;
        m_type = SCXTimeType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property float value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(float value)
    {
        m_fvalue = value;
        m_type = SCXFloatType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property double value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(double value)
    {
        m_dvalue = value;
        m_type = SCXDoubleType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set property vector value

       \param[in]  value  Property value to use
    */
    void SCXProperty::SetValue(const vector<SCXProperty>& value)
    {
        m_vvalue = value;
        m_type = SCXArrayType;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get property type

       \returns      Property type
    */
    SCXProperty::SCXType SCXProperty::GetType() const
    {
        return m_type;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Equality test operator

       \param       other RHS operand
       \returns     Boolean

    */
    bool SCXProperty::operator==(const SCXProperty& other) const
    {
        if (m_type == SCXTimeType)
        {
            return 
                (m_name == other.m_name) &&
                (m_type == other.m_type) &&
                (m_tvalue  == other.m_tvalue);
        }
        return
            (m_name    == other.m_name) &&
            (m_type    == other.m_type) &&
            (m_svalue  == other.m_svalue) &&
            (m_ivalue  == other.m_ivalue) &&
            (m_ssvalue == other.m_ssvalue) &&
            (m_uivalue == other.m_uivalue) &&
            (m_usvalue == other.m_usvalue) &&
            (m_ulvalue == other.m_ulvalue) &&
            (m_bvalue  == other.m_bvalue) &&
            (m_vvalue  == other.m_vvalue);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Dumps a string representation of the SCXProperty::SCXType type

       \param       t  The enum representation to convert
       \returns     A string with human-readable representation

       \date        07-07-23 10:45:56

    */
    std::wstring SCXProperty::DumpTypeString(SCXType t)    // static
    {
        switch (t)
        {
        case SCXStringType: return L"SCXString";
        case SCXIntType:    return L"SCXInt";
        case SCXSShortType: return L"SCXSShort";
        case SCXUIntType:   return L"SCXUInt";
        case SCXUShortType: return L"SCXUShort";
        case SCXUCharType:  return L"SCXUChar";
        case SCXULongType:  return L"SCXULong";
        case SCXBoolType:   return L"SCXBool";
        case SCXTimeType:   return L"SCXTime";
        case SCXFloatType:  return L"SCXFloat";
        case SCXDoubleType: return L"SCXDouble";
        case SCXArrayType:  return L"SCXArray";
        default:
            SCXASSERT( ! "Unknown property type" );
            return L"propertytype UNKNOWN";
        }

    }


    /*----------------------------------------------------------------------------*/
    /**
       Get a string that shows the content of the property

       \returns      String showing content of property

       Returns a string with content dump on format
       \verbatim
       \<name\>=\<value\> [type \<type\>]
       e.g.
       key=32 [type SCXULong]
       \endverbatim

    */
    wstring SCXProperty::DumpString() const
    {
        SCXDumpStringBuilder dsb("SCXProperty");

        dsb.Scalar("name", m_name);
            
        switch (m_type)
        {
        case SCXStringType: dsb.Scalar("value", m_svalue); break;
        case SCXIntType:    dsb.Scalar("value", m_ivalue); break;
        case SCXSShortType: dsb.Scalar("value", m_ssvalue); break;
        case SCXUIntType:   dsb.Scalar("value", m_uivalue); break;
        case SCXUShortType: dsb.Scalar("value", m_usvalue); break;
        case SCXUCharType:  dsb.Scalar("value", m_ucvalue); break;
        case SCXULongType:  dsb.Scalar("value", m_ulvalue); break;
        case SCXBoolType:   dsb.Scalar("value", m_bvalue); break;
        case SCXTimeType:   dsb.Instance("value", m_tvalue); break;
        case SCXFloatType:  dsb.Scalar("value", m_fvalue); break;
        case SCXDoubleType: dsb.Scalar("value", m_dvalue); break;
        case SCXArrayType:  dsb.Instances("value", m_vvalue); break;
        default: break;
        }

        dsb.Scalar("type", DumpTypeString(m_type));

        return dsb;
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
