/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief     Implementation of most of CMPI Template functionality.

   \date      07-05-14 12:45:51


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxnameresolver.h>
#include <scxsystemlib/scxostypeinfo.h>

#include <string>
#include <sstream>
#include <vector>
#include <assert.h>
#include <fstream>
#include <iostream>

#include <scxproviderlib/cmpibase.h>
#include <scxcorelib/stringaid.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXProviderLib
{
    //! Used for initializating rc codes to OK values. Maps 1:1 to CMPI definition.
    static const CMPIStatus CMPI_OK = { CMPI_RC_OK, NULL };

    /******************************************************************************
     *
     *   CMPIBaseProvider implementation
     *
     ******************************************************************************/


    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       Since the BaseProvider is pure abstract this ctor is only ever run
       when concrete subclass instances are created.

       \param   module  Module name string used for prefix in logfile

    */
    BaseProvider::BaseProvider(const std::wstring& module) :
        m_ProviderCapabilities(this), m_broker(NULL), m_allowUnload(false),
        m_initDone(false), m_cleanupDone(false), m_result(NULL), m_objectPath(NULL)
    {
        m_lock = ThreadLockHandleGet();
        m_log = SCXLogHandleFactory::GetLogHandle(module);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    BaseProvider::~BaseProvider()
    {
        // Do not nullify m_result or m_objectPath since they are owned by CMPI.
    }


    /*----------------------------------------------------------------------------*/
    /**
       Set broker object used in CMPI calls.

       \param       brkr    The broker object to save

       Normally used only by the provider factory macro.

    */
    void BaseProvider::SetBroker(const CMPIBroker* brkr)
    {
        m_broker = brkr;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Mark the provider as candidate for unloading on inactivity

       By default providers based on CMPI template will not unload.
    */
    void BaseProvider::EnableProviderUnloading()
    {
        m_allowUnload = true;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Mark the provider as candidate for unloading on inactivity

       By default providers based on CMPI template will not unload.

    */
    void BaseProvider::DisableProviderUnloading()
    {
        m_allowUnload = false;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Extract value from SCXProperty object to CMPIValue and CMPIType

       \param[in]  prop   The property to get data from
       \param[in]  value  CMPI value to populate from property
       \param[in]  type   CMPI type to populate from property

       \throws      SCXInvalidArgumentException if input type of a property is unknown
       \throws      SCXResourceExhaustedException if fails to create neccesary data
       \throws      SCXPropertyVectorError if the property contains an array with elements of mixed types
    */
    void BaseProvider::SCXPropertyToCMPIValue( // private
        const SCXProperty* prop,
        CMPIValue* value,
        CMPIType* type) const
    {
        CMPIStatus rc = CMPI_OK;

        SCXASSERT(NULL != prop);   // Asserts are sufficient as this is a private method
        SCXASSERT(NULL != value);
        SCXASSERT(NULL != type);

        SCX_LOGHYSTERICAL(m_log, wstring(L"SCXPropertyToCMPIValue - ").append(prop->DumpString()));
        switch (prop->GetType())
        {
        case SCXProperty::SCXStringType:
            value->string = CMNewString(m_broker, StrToUTF8(prop->GetStrValue()).c_str(), &rc);
            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXResourceExhaustedException(L"CMPI String",
                                                    StrAppend(L"CMNewString() Failed - ", rc.rc),
                                                    SCXSRCLOCATION);
            }
            SCX_LOGHYSTERICAL(m_log, wstring(L"SCXPropertyToCMPIValue - SCXStringType - ").
                              append(prop->GetStrValue()));
            *type = CMPI_string;
            break;

        case SCXProperty::SCXTimeType:
            value->dateTime = CMNewDateTimeFromChars(m_broker, StrToUTF8(prop->GetTimeValue().ToCIM()).c_str(), &rc);
            SCX_LOGHYSTERICAL(m_log, L"SCXPropertyToCMPIValue - SCXTimeType");
            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXResourceExhaustedException(L"CMPI DateTime",
                                                    StrAppend(L"CMNewDateTimeFromChars() Failed - ", rc.rc),
                                                    SCXSRCLOCATION);
            }
            *type = CMPI_dateTime;
            break;

        case SCXProperty::SCXFloatType:
            value->real32 = prop->GetFloatValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXFloatType - ",
                                               prop->GetFloatValue()));
            *type = CMPI_real32;
            break;

        case SCXProperty::SCXDoubleType:
            value->real64 = prop->GetDoubleValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXDoubleType - ",
                                               prop->GetDoubleValue()));
            *type = CMPI_real64;
            break;

        case SCXProperty::SCXIntType:
            value->sint32 = prop->GetIntValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXIntType - ",
                                               prop->GetIntValue()));
            *type = CMPI_sint32;
            break;

        case SCXProperty::SCXSShortType:
            value->sint16 = prop->GetSShortValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXSShortType - ",
                                               prop->GetSShortValue()));
            *type = CMPI_sint16;
            break;

        case SCXProperty::SCXUIntType:
            value->uint32 = prop->GetUIntValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXUIntType - ",
                                               prop->GetUIntValue()));
            *type = CMPI_uint32;
            break;

        case SCXProperty::SCXUShortType:
            value->uint16 = prop->GetUShortValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXUShortType - ",
                                               prop->GetUShortValue()));
            *type = CMPI_uint16;
            break;

        case SCXProperty::SCXUCharType:
            value->uint8 = prop->GetUCharValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXUCharType - ",
                                               prop->GetUCharValue()));
            *type = CMPI_uint8;
            break;

        case SCXProperty::SCXULongType:
            value->uint64 = prop->GetULongValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXULongType - ",
                                               prop->GetULongValue()));
            *type = CMPI_uint64;
            break;

        case SCXProperty::SCXBoolType:
            value->boolean = prop->GetBoolValue();
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"SCXPropertyToCMPIValue - SCXBoolType - ",
                                               prop->GetBoolValue()));
            *type = CMPI_boolean;
            break;

        case SCXProperty::SCXArrayType:
        {
            const std::vector<SCXProperty>& scxPropertyVector = prop->GetVectorValue();

            SCX_LOGHYSTERICAL(m_log, std::wstring(L"SCXPropertyToCMPIValue - SCXArrayType NrElements - ").append(prop->DumpString()));

            // Now we create a vector of CMPIValue
            std::vector<CMPIValue> cmpiValueVector;
            CMPIType containedElementType = CMPI_ARRAY; // This enables us to test that all elements are of the same type.
            for (std::vector<SCXProperty>::const_iterator iter = scxPropertyVector.begin();
                 iter != scxPropertyVector.end();
                 ++iter)
            {
                const SCXProperty &scxproperty = *iter;
                CMPIValue cmpiValue;
                CMPIType elementType;
                SCXPropertyToCMPIValue(&scxproperty, &cmpiValue, &elementType);
                if (CMPI_ARRAY == containedElementType)
                {
                    containedElementType = elementType;
                }
                if (elementType != containedElementType)
                {
                    throw SCXPropertyVectorError(*prop, L"Mixed property types in vector", SCXSRCLOCATION);
                }
                cmpiValueVector.push_back(cmpiValue);
            }

            // Ok. Now we know the CMPIType (in containedElementType)
            CMPIType arrayType = containedElementType | CMPI_ARRAY;
            value->array = CMNewArray(m_broker, static_cast<CMPICount>(cmpiValueVector.size()), arrayType, &rc);
            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXCoreLib::SCXResourceExhaustedException(L"CMPI Array",
                                                                SCXCoreLib::StrAppend(L"CMNewArray() Failed - ", rc.rc),
                                                                SCXSRCLOCATION);
            }

            int index = 0;
            for (std::vector<CMPIValue>::const_iterator iter = cmpiValueVector.begin();
                 iter != cmpiValueVector.end();
                 ++iter)
            {
                value->array->ft->setElementAt(value->array, index, &(*iter), containedElementType);
                ++index;
            }

            *type = arrayType;
            break;
        }
        default:
            throw SCXInvalidArgumentException(L"prop", L"Unhandled type", SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Populate SCXProperty object from CMPIValue and CMPIType

       \param[in]  value  CMPI value to get data from
       \param[in]  type   CMPI type to use
       \param[in]  prop   The property to populate

       \throws     SCXInvalidArgumentException if input type of a property is unknown

    */
    void BaseProvider::CMPIValueToSCXProperty(  // private
        const CMPIValue& value,
        CMPIType type,
        SCXProperty& prop) const
    {
        CMPIStatus rc = CMPI_OK;

        SCX_LOGHYSTERICAL(m_log, L"CMPIValueToSCXProperty");

        if (CMPI_ARRAY == (type & CMPI_ARRAY))
        {
            SCX_LOGHYSTERICAL(m_log, L"CMPIValueToSCXProperty - CMPI_ARRAY");
            std::vector<SCXProperty> propertyVector;
            for (unsigned int i = 0;
                 i < value.array->ft->getSize(value.array, NULL);
                 ++i)
            {
                SCXProperty vectorElement;
                CMPIData cmpiElement = value.array->ft->getElementAt(value.array, i, NULL);
                CMPIValueToSCXProperty(cmpiElement.value, cmpiElement.type, vectorElement);
                propertyVector.push_back(vectorElement);
            }
            prop.SetValue(propertyVector);

            return;
        }


        switch (type)
        {
        case CMPI_string:
            prop.SetValue(StrFromUTF8(CMGetCharPtr(value.string)));
            SCX_LOGHYSTERICAL(m_log, wstring(L"CMPIValueToSCXProperty - CMPI_string - ").
                              append(prop.GetStrValue()));
            break;

        case CMPI_dateTime:
        {
            std::wstring tmpString = StrFromUTF8(CMGetCharPtr(CMGetStringFormat(value.dateTime, &rc)));
            SCXCalendarTime tmpTime = SCXCalendarTime::FromCIM(tmpString);
            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXInvalidArgumentException(L"value", L"Failed to get string format from value.dateTime",
                                                  SCXSRCLOCATION);
            }
            prop.SetValue(tmpTime);
            SCX_LOGHYSTERICAL(m_log, StrAppend( StrAppend(L"CMPIValueToSCXProperty - CMPI_dateTime - ", tmpString),
                                                StrAppend(L" : ", tmpTime.DumpString())));
        }
        break;

        case CMPI_real32:
            prop.SetValue(value.real32);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_real32 - ",
                                               prop.GetFloatValue()));
            break;

        case CMPI_real64:
            prop.SetValue(value.real64);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_real64 - ",
                                               prop.GetDoubleValue()));
            break;

        case CMPI_sint32:
            prop.SetValue(value.sint32);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_sint32 - ",
                                               prop.GetIntValue()));
            break;

        case CMPI_sint16:
            prop.SetValue(value.sint16);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_sint16 - ",
                                               prop.GetSShortValue()));
            break;

        case CMPI_uint8:
            prop.SetValue(value.uint8);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_uint8 - ",
                                               prop.GetUCharValue()));
            break;

        case CMPI_uint16:
            prop.SetValue(value.uint16);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_uint16 - ",
                                               prop.GetUShortValue()));
            break;

        case CMPI_uint32:
            prop.SetValue(value.uint32);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_uint32 - ",
                                               prop.GetUIntValue()));
            break;

        case CMPI_uint64:
            prop.SetValue(static_cast<scxulong>(value.uint64));
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_uint64 - ",
                                               prop.GetULongValue()));
            break;

        case CMPI_boolean:
            prop.SetValue(static_cast<bool>(value.boolean));
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMPIValueToSCXProperty - CMPI_boolean - ",
                                               prop.GetBoolValue()));
            break;

        default:
            SCX_LOGERROR(m_log, StrAppend(L"No support for converting CMPI data type ", type));
            // throw SCXInvalidArgumentException(L"type", L"Unhandled type", SCXSRCLOCATION);
//            throw SCXInvalidArgumentException(L"type", L"Unhandled type", SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Populate SCXProperty object from CMPIData

       \param[in]  data  CMPI data object to get data from
       \param[in]  prop  The property to populate

       Convenience method to be able to convert CMPIData to SCXProperty directly.

    */
    void BaseProvider::CMPIDataToSCXProperty( // private
        const CMPIData& data,
        SCXProperty& prop) const
    {
        CMPIValueToSCXProperty(data.value, data.type, prop);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Get keys from SCX instance object and add to CMPI Object path

       \param[in]  sInst             SCX Instance object to read from
       \param[in]  pCmpiObjectPath   CMPI Object Path to write to

       \throws     SCXResourceExhaustedException if fails to create neccesary data
    */
    void BaseProvider::SCXInstanceGetKeys(const SCXInstance* sInst, CMPIObjectPath* pCmpiObjectPath) const // private
    {
        CMPIStatus rc = CMPI_OK;

        for (size_t j=0; j< sInst->NumberOfKeys(); j++)
        {
            const SCXProperty* prop = sInst->GetKey(j);
            CMPIValue value;
            CMPIType type;

            SCX_LOGHYSTERICAL(m_log, wstring(L"BaseProvider::SCXInstanceGetKeys() - Handle a key - ").
                              append(prop->GetName()));

            SCXPropertyToCMPIValue(prop, &value, &type);
            rc = CMAddKey(pCmpiObjectPath, StrToUTF8(prop->GetName()).c_str(), &value, type);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CMAddKey returns:", rc.rc));
            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXResourceExhaustedException(L"CMPI Object Path",
                                                    StrAppend(L"CMAddKey() Failed - ", rc.rc),
                                                    SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Populate CMPI Instance object from SCX INstance object

       \param[in]     sInst  SCX INstance object to read from
       \param[in,out] cInst  CMPI Instance to write to

    */
    void BaseProvider::SCXInstanceToCMPIInstance(const SCXInstance* sInst, CMPIInstance *cInst) const // private
    {
        CMPIStatus rc = CMPI_OK;

        SCXASSERT(sInst); // Sufficient to assert since this is a private method
        SCXASSERT(cInst);

        sInst->ApplyFilter(cInst);

        for (size_t j=0; j< sInst->NumberOfProperties(); j++)
        {
            const SCXProperty* prop = sInst->GetProperty(j);
            CMPIValue value;
            CMPIType type;

            SCX_LOGHYSTERICAL(m_log,
                              wstring(L"BaseProvider::SCXInstanceToCMPIInstance() - Handle a property - ").
                              append(prop->GetName()));
            SCXPropertyToCMPIValue(prop, &value, &type);
            // If we have an array which is empty we do not insert it.
            if ( 0 == (type & CMPI_ARRAY) ||
                 0 < value.array->ft->getSize(value.array, NULL))
            {
                rc = CMSetProperty(cInst, StrToUTF8(prop->GetName()).c_str(), &value, type);
                if (rc.rc != CMPI_RC_OK)
                {
                    // A non-OK rc from CMSetProperty() _can_ be caused by type mismatch between
                    // the type in MOF definition and actual type.
                    wostringstream buf;
                    buf << L"CMSetProperty() failed with code " << rc.rc
                        << L" for property " << prop->DumpString();
                    throw SCXInternalErrorException(buf.str(), SCXSRCLOCATION);
                }
            }
        }

        // The CMPI interface requires key properties to be set explicit properties even
        // though they are (most likely) already set as keys too
        for (size_t j=0; j< sInst->NumberOfKeys(); j++)
        {
            const SCXProperty* prop = sInst->GetKey(j);
            CMPIValue value;
            CMPIType type;

            SCX_LOGHYSTERICAL(m_log, wstring(L"BaseProvider::SCXInstanceToCMPIInstance() - Handle a key - ").
                              append(prop->GetName()));

            SCXPropertyToCMPIValue(prop, &value, &type);

            rc = CMSetProperty(cInst, StrToUTF8(prop->GetName()).c_str(), &value, type);
            if (rc.rc != CMPI_RC_OK)
            {
                wostringstream buf;
                buf << L"CMSetProperty() failed with code " << rc.rc
                    << L" for property " << prop->DumpString();
                throw SCXInternalErrorException(buf.str(), SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Extract key values, CIM namespace and CIM class name from CMPI object path

       \param[in]     pObjectPath   CMPI Object path to get keys from
       \param[in,out] scxObjectPath SCX Instance to populate with keys

    */
    void BaseProvider::CMPIObjectPathToScxObjectPath(const CMPIObjectPath* pObjectPath,
                                                     SCXInstance& scxObjectPath) const // private
    {
        CMPIStatus rc = CMPI_OK;

        SCXASSERT(pObjectPath); // Sufficient to assert since this is a private method

        // Set the CIM namespace in the target
        CMPIString* nameSpace = CMGetNameSpace(pObjectPath, &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetNameSpace() failed - ", rc.rc), SCXSRCLOCATION);
        }

        const char* nameSpaceStr = CMGetCharsPtr(nameSpace, &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetCharsPtr() failed - ", rc.rc), SCXSRCLOCATION);
        }

        wstring cimNamespace = StrFromUTF8(nameSpaceStr);
        scxObjectPath.SetCimNamespace(cimNamespace);

        // Set the CIM class name in the target
        CMPIString* className = CMGetClassName(pObjectPath, &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetClassName() failed - ", rc.rc), SCXSRCLOCATION);
        }

        const char* classNameStr = CMGetCharsPtr(className, &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetCharsPtr() failed - ", rc.rc), SCXSRCLOCATION);
        }

        wstring cimClassName = StrFromUTF8(classNameStr);
        scxObjectPath.SetCimClassName(cimClassName);

        // Take care of the keys
        unsigned int key_count = CMGetKeyCount(pObjectPath, &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetKeyCount() Failed - ", rc.rc), SCXSRCLOCATION);
        }

        for (unsigned int i=0; i<key_count; i++)
        {
            CMPIString *name = NULL;
            CMPIData data = CMGetKeyAt(pObjectPath, i, &name, &rc);
            SCXProperty prop;

            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXInternalErrorException(StrAppend(L"CMGetKeyAt() Failed - ", rc.rc), SCXSRCLOCATION);
            }
            CMPIDataToSCXProperty(data, prop);
            prop.SetName(StrFromUTF8(CMGetCharPtr(name)));

            SCX_LOGHYSTERICAL(m_log, wstring(L"BaseProvider::CMPIObjectPathToScxObjectPath() - Handle a key - ").
                              append(prop.GetName()));
            scxObjectPath.AddKey(prop);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Convert an CMPI Instance into an SCXInstance

       \param[in]     scxObjectPath  SCX Object path to get keys from
       \param[in]     pInstance      Pointer to the CMPI instance object
       \param[out]    scxInstance    Output as an SCX Instance

    */
    void BaseProvider::CMPIInstanceToScxInstance(const SCXInstance&  scxObjectPath,
                                                 const CMPIInstance* pInstance,
                                                 SCXInstance&        scxInstance) const // private
    {
        CMPIStatus rc = CMPI_OK;

        SCXASSERT(pInstance); // Sufficient to assert since this is a private method

        // Start by copying all info from the already set up object path
        scxInstance = scxObjectPath;

        // Then take care of the properties. In the SCX representation the properties which
        // are keys should not be duplicated as non-key properties so we check for that.

        unsigned int property_count = CMGetPropertyCount(pInstance, &rc);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetPropertyCount() Failed - ", rc.rc), SCXSRCLOCATION);
        }

        for (unsigned int i=0; i<property_count; i++)
        {
            CMPIString *name;
            CMPIData data = CMGetPropertyAt(pInstance, i, &name, &rc);
            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXInternalErrorException(StrAppend(L"CMGetKeyAt() Failed - ", rc.rc), SCXSRCLOCATION);
            }

            if (! CMIsNullValue(data))
            {
                SCXProperty scxProperty;

                // Set the property name
                scxProperty.SetName(StrFromUTF8(CMGetCharPtr(name)));

                // Set property data from CMPI instance to  the SCX instance by calling aux function
                CMPIDataToSCXProperty(data, scxProperty);

                SCX_LOGHYSTERICAL(m_log, wstring(L"BaseProvider::CMPIInstanceToScxInstance() - Handle a property - ").
                                  append(scxProperty.GetName()));

                if (!CMIsKeyValue(data))
                {
                    // Ordinary property, set it
                    scxInstance.AddProperty(scxProperty);
                }
                else
                {
#if defined(_DEBUG)
                    // If we already have the property as a key, make sure it's there already
                    const SCXProperty* pKey = scxObjectPath.GetKey(scxProperty.GetName());
                    SCXASSERT(pKey);
                    SCXASSERT(*pKey == scxProperty);
#endif
                }
            }
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Create a new CMPI Object Path object with information taken from another CMPi Object Path

       \param[in]   pObjectPath   The CMPI Object Path to get information from

       \returns     A new CMPI Object Path

       \throws      SCXInternalErrorException if failing to get information from input object path
       \throws      SCXResourceExhaustedException if failing to create new object path

    */
    CMPIObjectPath* BaseProvider::GetNewObjectPath(const CMPIObjectPath* pObjectPath) const // private
    {
        CMPIStatus rc = CMPI_OK;

        CMPIString* nameSpace = CMGetNameSpace(pObjectPath, &rc);

        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetNameSpace() failed - ", rc.rc), SCXSRCLOCATION);
        }

        const char* nameSpaceStr = CMGetCharsPtr(nameSpace, &rc);

        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetCharsPtr() failed - ", rc.rc), SCXSRCLOCATION);
        }

        CMPIString* className = CMGetClassName(pObjectPath, &rc);

        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetClassName() failed - ", rc.rc), SCXSRCLOCATION);
        }

        const char* classNameStr = CMGetCharsPtr(className, &rc);

        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMGetCharsPtr() failed - ", rc.rc), SCXSRCLOCATION);
        }

        CMPIObjectPath* pCmpiObjectPath = CMNewObjectPath(m_broker, nameSpaceStr, classNameStr, &rc);

        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXResourceExhaustedException(L"CMPI Object Path", StrAppend(L"CMNewObjectPath() failed - ", rc.rc),
                                                SCXSRCLOCATION);
        }

        return pCmpiObjectPath;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Init the provider instance

       Called by the singleton macro

    */
    void BaseProvider::Init()
    {
        try
        {
            SCXThreadLock lock(m_lock);

            // Only one call to cleanup regardless of which provider type this is
            // (instance, association...)
            // See also BaseProvider::MethodCleanup()
            if (!m_initDone)
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::Init - Calling DoInit()");
                // Call the init of the subclass, allowing it to run its capabilities
                // registration phase
                DoInit();
                m_initDone = true;
            }
            else
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::Init - NOT Calling DoInit()");
            }

            // Does not make sense to have a provider which do not support any classes
            SCXASSERT(0 != m_ProviderCapabilities.GetNumberRegisteredClasses());
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::Init() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::Init() - ").append(DumpString(e)));
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::Init() - Unknown exception"));
        }
    }


    /*----------------------------------------------------------------------------*/
    /*
      Implementation of CMPI interface. See CMPI documentation for reference:
      http://sharepointemea/sites/aurora/Shared%20Documents/OpenGroup/CMPI%202.0.pdf
    */
    /*----------------------------------------------------------------------------*/

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::Cleanup(
        CMPIInstanceMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        CMPIBoolean terminate)
    {
        try
        {
            SCXThreadLock lock(m_lock);

            if (!terminate && false == m_allowUnload)
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::Cleanup - Provider unloading disabled");
                CMReturn(CMPI_RC_DO_NOT_UNLOAD);
            }

            // Only one call to cleanup regardless of which provider type this is
            // (instance, association...)
            // See also BaseProvider::MethodCleanup()
            if (!m_cleanupDone)
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::Cleanup - Calling DoCleanup()");
                DoCleanup();
                m_cleanupDone = true;
            }
            else
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::Cleanup - NOT Calling DoCleanup()");
            }

            CMReturn(CMPI_RC_OK);
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::Cleanup() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::Cleanup() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::Cleanup() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::Cleanup() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::MethodCleanup(
        CMPIMethodMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        CMPIBoolean terminate)
    {
        try
        {
            SCXThreadLock lock(m_lock);

            if (!terminate && false == m_allowUnload)
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::MethodCleanup - Provider unloading disabled");
                CMReturn(CMPI_RC_DO_NOT_UNLOAD);
            }

            // Only one call to cleanup regardless of which provider type this is
            // (instance, association...)
            // See also BaseProvider::Cleanup()
            if (!m_cleanupDone)
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::MethodCleanup - Calling DoCleanup()");
                DoCleanup();
                m_cleanupDone = true;
            }
            else
            {
                SCX_LOGTRACE(m_log, L"BaseProvider::MethodCleanup - NOT Calling DoCleanup()");
            }

            CMReturn(CMPI_RC_OK);
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::MethodCleanup() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::MethodCleanup() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::MethodCleanup() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::MethodCleanup() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::EnumInstanceNames(
        CMPIInstanceMI*  /*cThis*/,
        const CMPIContext* /*pContext*/,
        const CMPIResult* resultHandle,
        const CMPIObjectPath* pObjectPath)
    {
        try
        {
            CMPIStatus rc = CMPI_OK;

            SCX_LOGTRACE(m_log, wstring(L"BaseProvider::EnumInstanceNames()"));
            SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                         append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));

            SCXInstance scxObjectPath;
            CMPIObjectPathToScxObjectPath(pObjectPath, scxObjectPath);

            SCXProviderSupportType providerSupport = m_ProviderCapabilities.CheckClassSupport(scxObjectPath);

            // Check if the class is supported by this provider
            if (eNoSupport != providerSupport)
            {
                SCXInstanceCollection instances;

                SCXCallContext callContext(scxObjectPath, providerSupport);

                // Setup data members needed by SendInstanceName() if supported by provider.
                SCXThreadLock lock(m_lock);

                if (SupportsSendInstance())
                {
                    m_result = resultHandle;
                    m_objectPath = pObjectPath;
                }

                // Call virtual method to do enumeration
                SCX_LOGTRACE(m_log, L"BaseProvider::EnumInstanceNames() - Calling DoEnumInstanceNames()");
                DoEnumInstanceNames(callContext, instances);

                // Clear data used
                m_result = NULL;
                m_objectPath = NULL;

                lock.Unlock();

                // If we sent one instance at a time, then nothing should be added to the vector
                if (SupportsSendInstance())
                {
                    SCXASSERT( instances.Size() == 0 );
                    SCX_LOGTRACE(m_log, L"BaseProvider::EnumInstanceNames() - DoEnumInstanceNames() returned - <One at a time>");
                }
                else
                {
                SCX_LOGTRACE(m_log, StrAppend(L"BaseProvider::EnumInstanceNames() - DoEnumInstanceNames() returnd - ", instances.Size()));
                }

                // Convert returned instance collection to CMPI types and return these
                for (size_t i=0; i<instances.Size(); i++)
                {
                    CMPIObjectPath* pCmpiObjectPath = GetNewObjectPath(pObjectPath);

                    SCXInstanceGetKeys(instances[i], pCmpiObjectPath);

                    rc = CMReturnObjectPath(resultHandle, pCmpiObjectPath);

                    if (rc.rc != CMPI_RC_OK)
                    {
                        throw SCXInternalErrorException(StrAppend(L"CMReturnObjectPath() Failed - ", rc.rc),
                                                        SCXSRCLOCATION);
                    }
                }

                rc = CMReturnDone(resultHandle);

                if (rc.rc != CMPI_RC_OK)
                {
                    throw SCXInternalErrorException(StrAppend(L"CMReturnDone() Failed - ", rc.rc), SCXSRCLOCATION);
                }

                CMReturn(CMPI_RC_OK);
            }
            else
            {
                SCX_LOGINFO(m_log, wstring(L"BaseProvider::EnumInstanceNames() - Invalid class - ").
                            append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
                CMReturn(CMPI_RC_ERR_INVALID_CLASS);
            }
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::EnumInstanceNames() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXAccessViolationException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::EnumInstanceNames() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_ACCESS_DENIED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::EnumInstanceNames() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::EnumInstanceNames() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::EnumInstanceNames() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::EnumInstances(
        CMPIInstanceMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        const CMPIResult* resultHandle,
        const CMPIObjectPath* pObjectPath,
        const char** /*properties*/)
    {
        try
        {
            CMPIStatus rc = CMPI_OK;

            SCX_LOGTRACE(m_log, wstring(L"BaseProvider::EnumInstances()"));
            SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                         append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));

            SCXInstance scxObjectPath;
            CMPIObjectPathToScxObjectPath(pObjectPath, scxObjectPath);

            SCXProviderSupportType providerSupport = m_ProviderCapabilities.CheckClassSupport(scxObjectPath);

            // Check if class is supported by this provider
            if (eNoSupport != providerSupport)
            {
                SCXInstanceCollection instances;

                SCXCallContext callContext(scxObjectPath, providerSupport);

                // Setup data members needed by SendInstance() if supported by provider.
                SCXThreadLock lock(m_lock);

                if (SupportsSendInstance())
                {
                    m_result = resultHandle;
                    m_objectPath = pObjectPath;
                }

                // Call virtual method to do enumeration
                SCX_LOGTRACE(m_log, L"BaseProvider::EnumInstances() - Calling DoEnumInstances()");
                DoEnumInstances(callContext, instances);

                // Clear data used
                m_result = NULL;
                m_objectPath = NULL;

                lock.Unlock();

                // If we sent one instance at a time, then nothing should be added to the vector
                if (SupportsSendInstance())
                {
                    SCXASSERT( instances.Size() == 0 );
                    SCX_LOGTRACE(m_log, L"BaseProvider::EnumInstances() - DoEnumInstances() returned - <One at a time>");
                }
                else
                {
                SCX_LOGTRACE(m_log, StrAppend(L"BaseProvider::EnumInstances() - DoEnumInstances() returned - ",
                                              instances.Size()));
                }

                // Convert returned instance collection to CMPI types and return these
                for (size_t i=0; i<instances.Size(); i++)
                {
                    CMPIObjectPath* pCmpiObjectPath = GetNewObjectPath(pObjectPath);

                    SCXInstanceGetKeys(instances[i], pCmpiObjectPath);

                    CMPIInstance *pInstance = CMNewInstance(m_broker, pCmpiObjectPath, &rc);

                    if (rc.rc != CMPI_RC_OK)
                    {
                        throw SCXResourceExhaustedException(L"CMPI Instance", StrAppend(L"CMNewInstance() failed - ", rc.rc),
                                                            SCXSRCLOCATION);
                    }

                    SCXInstanceToCMPIInstance(instances[i], pInstance);

                    rc = CMReturnInstance(resultHandle, pInstance);
                    if (rc.rc != CMPI_RC_OK)
                    {
                        throw SCXInternalErrorException(StrAppend(L"CMReturnInstance() Failed - ", rc.rc),
                                                        SCXSRCLOCATION);
                    }
                    SCX_LOGHYSTERICAL(m_log, L"BaseProvider::EnumInstances() - Add instance for returning");
                }
            }
            else
            {
                SCX_LOGINFO(m_log, wstring(L"BaseProvider::EnumInstances() - Invalid class - ").
                            append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
                CMReturn(CMPI_RC_ERR_INVALID_CLASS);
            }

            SCX_LOGTRACE(m_log, L"BaseProvider::EnumInstances() - Call ReturnDone");
            rc = CMReturnDone(resultHandle);

            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXInternalErrorException(StrAppend(L"CMReturnDone() Failed - ", rc.rc), SCXSRCLOCATION);
            }

            SCX_LOGTRACE(m_log, L"BaseProvider::EnumInstances() - return OK");
            CMReturn(CMPI_RC_OK);
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::EnumInstances() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXAccessViolationException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::EnumInstances() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_ACCESS_DENIED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::EnumInstances() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::EnumInstances() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::EnumInstances() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::GetInstance(
        CMPIInstanceMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        const CMPIResult* resultHandle,
        const CMPIObjectPath* pObjectPath,
        const char** /*properties*/)
    {
        try
        {
            CMPIStatus rc = CMPI_OK;

            SCX_LOGTRACE(m_log, wstring(L"BaseProvider::GetInstance()"));
            SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                         append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));

            SCXInstance scxObjectPath;
            CMPIObjectPathToScxObjectPath(pObjectPath, scxObjectPath);

            SCXProviderSupportType providerSupport = m_ProviderCapabilities.CheckClassSupport(scxObjectPath);

            // Check if correct class
            if (eNoSupport != providerSupport)
            {
                SCXInstance objectPath;
                SCXInstance inst;

                SCXCallContext callContext(scxObjectPath, providerSupport);

                SCXThreadLock lock(m_lock);
                SCX_LOGTRACE(m_log, L"BaseProvider::GetInstance() - Calling DoGetInstance()");
                DoGetInstance(callContext, inst);
                lock.Unlock();

                CMPIObjectPath* pCmpiObjectPath = GetNewObjectPath(pObjectPath);

                CMPIInstance *pInstance = CMNewInstance(m_broker, pCmpiObjectPath, &rc);

                if (rc.rc != CMPI_RC_OK)
                {
                    throw SCXResourceExhaustedException(L"CMPI Instance",
                                                        StrAppend(L"CMNewInstance() failed - ", rc.rc),
                                                        SCXSRCLOCATION);
                }

                SCXInstanceToCMPIInstance(&inst, pInstance);

                rc = CMReturnInstance(resultHandle, pInstance);
                if (rc.rc != CMPI_RC_OK)
                {
                    throw SCXInternalErrorException(StrAppend(L"CMReturnInstance() Failed - ", rc.rc),
                                                    SCXSRCLOCATION);

                }
                SCX_LOGTRACE(m_log, L"BaseProvider::GetInstance() - Add instance for returning");

            }
            else
            {
                SCX_LOGINFO(m_log, wstring(L"BaseProvider::GetInstance() - Invalid class - ").
                            append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
                CMReturn(CMPI_RC_ERR_INVALID_CLASS);
            }

            SCX_LOGTRACE(m_log, L"BaseProvider::GetInstance() - Call ReturnDone");
            rc = CMReturnDone(resultHandle);

            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXInternalErrorException(StrAppend(L"CMReturnDone() Failed - ", rc.rc), SCXSRCLOCATION);
            }

            SCX_LOGTRACE(m_log, L"BaseProvider::GetInstance() - return OK");
            CMReturn(CMPI_RC_OK);
        }
        catch (const SCXCIMInstanceNotFound& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::GetInstance() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_FOUND);
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::GetInstance() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXAccessViolationException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::GetInstance() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_ACCESS_DENIED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::GetInstance() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::GetInstance() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::GetInstance() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::CreateInstance(
        CMPIInstanceMI*      /* cThis */,
        const CMPIContext*   /* pContext */,
        const CMPIResult*    resultHandle ,
        const CMPIObjectPath* pObjectPath,
        const CMPIInstance*   pInstance)
    {
        try
        {
            CMPIStatus rc = CMPI_OK;

            SCX_LOGTRACE(m_log, wstring(L"BaseProvider::CreateInstance()"));
            SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                         append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));

            // For some reason the Object Path keys are not populated in pObjectPath for CreateInstance,
            // at least not when the call is made via CIMClient interface. We solve this "artificially"
            // by requesting the object path from the instance instead via a CIMOM query.
            CMPIObjectPath* pFullObjectPath = CMGetObjectPath(pInstance, &rc);
            SCX_LOGTRACE(m_log, wstring(L"Object Path from inst = ").
                         append(StrFromUTF8(CMGetCharPtr(pFullObjectPath->ft->toString(pFullObjectPath, NULL)))));

            SCXInstance scxObjectPath;
            CMPIObjectPathToScxObjectPath(pFullObjectPath, scxObjectPath);

            SCXProviderSupportType providerSupport = m_ProviderCapabilities.CheckClassSupport(scxObjectPath);

            // Check if correct class
            if (eNoSupport != providerSupport)
            {
                SCXInstance objectPath;
                SCXInstance newScxInstance;
                SCXInstance newObjectPath;

                SCXCallContext callContext(scxObjectPath, providerSupport);

                CMPIInstanceToScxInstance(scxObjectPath, pInstance, newScxInstance);

                SCXThreadLock lock(m_lock);
                SCX_LOGTRACE(m_log, L"BaseProvider::CreateInstance() - Calling DoCreateInstance()");
                DoCreateInstance(callContext, newScxInstance, newObjectPath);
                lock.Unlock();

                SCX_LOGTRACE(m_log, L"BaseProvider::CreateInstance() - Add instance for returning");

                CMPIObjectPath* pCmpiObjectPath = GetNewObjectPath(pObjectPath); // Use the OP without keys here
                SCXInstanceGetKeys(&newObjectPath, pCmpiObjectPath);
                rc = CMReturnObjectPath(resultHandle, pCmpiObjectPath);
                if (rc.rc != CMPI_RC_OK)
                {
                    throw SCXInternalErrorException(StrAppend(L"BaseProvider::CreateInstance() - CMReturnObjectPath() Failed - ", rc.rc),
                                                    SCXSRCLOCATION);
                }
            }
            else
            {
                SCX_LOGINFO(m_log, wstring(L"BaseProvider::CreateInstance() - Invalid class - ").
                            append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
                CMReturn(CMPI_RC_ERR_INVALID_CLASS);
            }

//            SCX_LOGTRACE(m_log, L"BaseProvider::CreateInstance() - Call ReturnDone");
//            rc = CMReturnDone(resultHandle);
//            if (rc.rc != CMPI_RC_OK)
//            {
//                throw SCXInternalErrorException(StrAppend(L"CMReturnDone() Failed - ", rc.rc), SCXSRCLOCATION);
//            }

            SCX_LOGTRACE(m_log, L"BaseProvider::CreateInstance() - return OK");
            CMReturn(CMPI_RC_OK);
        }
        catch (const SCXCIMInstanceAlreadyExists& e)
        {

            SCX_LOGINFO(m_log, wstring(L"BaseProvider::CreateInstance(): Obj already exists - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_ALREADY_EXISTS);
        }
        catch (const SCXCIMInstanceManipError& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::CreateInstance(): Failed to manipulate - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (const SCXAccessViolationException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::CreateInstance() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_ACCESS_DENIED);
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::CreateInstance() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::CreateInstance() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::CreateInstance() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::CreateInstance() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }


    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::ModifyInstance(
        CMPIInstanceMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        const CMPIResult* /*resultHandle*/,
        const CMPIObjectPath* pObjectPath,
        const CMPIInstance* /*pInstance*/,
        const char** /*properties*/)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::ModifyInstance - Returning not supported error");
        SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                     append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
        CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::DeleteInstance(

        CMPIInstanceMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        const CMPIResult* /*resultHandle*/,
        const CMPIObjectPath* pObjectPath)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::DeleteInstance - Returning not supported error");
        SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                     append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
        CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::ExecQuery(
        CMPIInstanceMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        const CMPIResult* resultHandle,
        const CMPIObjectPath* pObjectPath,
        const char* query, const char* language)
    {
        try
        {
            CMPIStatus rc = CMPI_OK;

            SCX_LOGTRACE(m_log, wstring(L"BaseProvider::ExecQuery()"));
            SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                         append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));

            SCXInstance scxObjectPath;
            CMPIObjectPathToScxObjectPath(pObjectPath, scxObjectPath);

            SCXProviderSupportType providerSupport = m_ProviderCapabilities.CheckClassSupport(scxObjectPath);

            // Check if class is supported by this provider
            if (eNoSupport != providerSupport)
            {
                SCXInstanceCollection instances;

                SCXCallContext callContext(scxObjectPath, providerSupport);

                // Setup data members needed by SendInstance() if supported by provider.
                SCXThreadLock lock(m_lock);

                if (SupportsSendInstance())
                {
                    m_result = resultHandle;
                    m_objectPath = pObjectPath;
                }

                // Call virtual method to run query
                SCX_LOGTRACE(m_log, L"BaseProvider::ExecQuery() - Calling DoExecQuery()");
                DoExecQuery(callContext, instances, StrFromUTF8(query), StrFromUTF8(language));

                // Clear data used
                m_result = NULL;
                m_objectPath = NULL;

                lock.Unlock();

                // If we sent one instance at a time, then nothing should be added to the vector
                if (SupportsSendInstance())
                {
                    SCXASSERT( instances.Size() == 0 );
                    SCX_LOGTRACE(m_log, L"BaseProvider::ExecQuery() - DoExecQuery() returned - <One at a time>");
                }
                else
                {
                SCX_LOGTRACE(m_log, StrAppend(L"BaseProvider::ExecQuery() - DoExecQuery() returned - ",
                                              instances.Size()));
                }

                // Convert returned instance collection to CMPI types and return these
                for (size_t i=0; i<instances.Size(); i++)
                {
                    CMPIObjectPath* pCmpiObjectPath = GetNewObjectPath(pObjectPath);

                    SCXInstanceGetKeys(instances[i], pCmpiObjectPath);

                    CMPIInstance *pInstance = CMNewInstance(m_broker, pCmpiObjectPath, &rc);

                    if (rc.rc != CMPI_RC_OK)
                    {
                        throw SCXResourceExhaustedException(L"CMPI Instance", StrAppend(L"CMNewInstance() failed - ", rc.rc),
                                                            SCXSRCLOCATION);
                    }

                    SCXInstanceToCMPIInstance(instances[i], pInstance);

                    rc = CMReturnInstance(resultHandle, pInstance);
                    if (rc.rc != CMPI_RC_OK)
                    {
                        throw SCXInternalErrorException(StrAppend(L"CMReturnInstance() Failed - ", rc.rc),
                                                        SCXSRCLOCATION);
                    }
                    SCX_LOGHYSTERICAL(m_log, L"BaseProvider::ExecQuery() - Add instance for returning");
                }
            }
            else
            {
                SCX_LOGINFO(m_log, wstring(L"BaseProvider::ExecQuery() - Invalid class - ").
                            append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
                CMReturn(CMPI_RC_ERR_INVALID_CLASS);
            }

            SCX_LOGTRACE(m_log, L"BaseProvider::ExecQuery() - Call ReturnDone");
            rc = CMReturnDone(resultHandle);

            if (rc.rc != CMPI_RC_OK)
            {
                throw SCXInternalErrorException(StrAppend(L"CMReturnDone() Failed - ", rc.rc), SCXSRCLOCATION);
            }

            SCX_LOGTRACE(m_log, L"BaseProvider::ExecQuery() - return OK");
            CMReturn(CMPI_RC_OK);
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::ExecQuery() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXCIMInvalidQuery& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::ExecQuery() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_INVALID_QUERY);
        }
        catch (const SCXCIMQueryLanguageNotSupported& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::ExecQuery() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_QUERY_LANGUAGE_NOT_SUPPORTED);
        }
        catch (const SCXAccessViolationException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::ExecQuery() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_ACCESS_DENIED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::ExecQuery() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::ExecQuery() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::ExecQuery() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }

    /**
       Implementation of the CMPI 2.0 standard function with the same name, for internal
       use only.
    */
    CMPIStatus BaseProvider::InvokeMethod(
        CMPIMethodMI* /*cThis*/,
        const CMPIContext* /*pContext*/,
        const CMPIResult* resultHandle,
        const CMPIObjectPath* pObjectPath,
        const char* method,
        const CMPIArgs* in,
        CMPIArgs* out)
    {
        try
        {
            CMPIStatus rc = CMPI_OK;
            SCXArgs args;
            SCXArgs outargs;
            SCXProperty result;
            unsigned int i=0;

            SCX_LOGTRACE(m_log, wstring(L"BaseProvider::InvokeMethod()"));
            SCX_LOGTRACE(m_log, wstring(L"Object Path = ").
                         append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));

            SCXProviderLib::SCXInstance scxObjectPath;
            CMPIObjectPathToScxObjectPath(pObjectPath, scxObjectPath);
            SCXProviderSupportType providerSupport = m_ProviderCapabilities.CheckClassSupport(scxObjectPath);

            // Check if correct class
            if (eNoSupport != providerSupport)
            {
                // Convert in argument structure from CMPI
                unsigned int arg_count = CMGetArgCount(in, &rc);
                if (rc.rc != CMPI_RC_OK)
                {
                    throw SCXInternalErrorException(StrAppend(L"CMGetArgCount(in) Failed - ", rc.rc),
                                                    SCXSRCLOCATION);
                }

                SCX_LOGTRACE(m_log, StrAppend(L"BaseProvider::InvokeMethod() - Extracting in arguments - ", arg_count));
                    for (i=0; i<arg_count; i++)
                    {
                        CMPIString *name;
                        CMPIData data = CMGetArgAt(in, i, &name, &rc);
                        SCXProperty prop;

                        if (rc.rc != CMPI_RC_OK)
                        {
                            throw SCXInternalErrorException(StrAppend(L"CMGetArgAt(in) Failed - ", rc.rc),
                                                            SCXSRCLOCATION);
                        }
                        // Using properties for arguments
                        CMPIDataToSCXProperty(data, prop);
                        prop.SetName(StrFromUTF8(CMGetCharPtr(name)));

                        args.AddProperty(prop);
                    }

                SCX_LOGTRACE(m_log, L"BaseProvider::InvokeMethod() - Extracting Object Path information");

                SCXCallContext callContext(scxObjectPath, providerSupport);

                // Call virtual method for actual method execution
                SCXThreadLock lock(m_lock);
                SCX_LOGTRACE(m_log, L"BaseProvider::InvokeMethod() - Calling DoInvokeMethod()");
                DoInvokeMethod(callContext, StrFromUTF8(method), args, outargs, result);
                lock.Unlock();

                for (size_t j=0; j<outargs.NumberOfProperties(); j++)
                {
                    const SCXProperty* prop = outargs.GetProperty(j);
                    CMPIValue value;
                    CMPIType type;

                    SCX_LOGHYSTERICAL(m_log,
                                      StrAppend(L"BaseProvider::InvokeMethod() - Handle a outarg - ", prop->GetName()));
                    SCXPropertyToCMPIValue(prop, &value, &type);
                    rc = CMAddArg(out, StrToUTF8(prop->GetName()).c_str(), &value, type);
                    if (rc.rc != CMPI_RC_OK)
                    {
                        throw SCXInternalErrorException(StrAppend(L"CMAddArg() Failed - ", rc.rc), SCXSRCLOCATION);
                    }
                }

                SCX_LOGTRACE(m_log, wstring(L"BaseProvider::InvokeMethod() - DoInvokeMethod() returns - ").
                             append(result.DumpString()));

                // Convert returned property to CMPI format and return
                CMPIValue cmpivalue;
                CMPIType cmpitype;

                SCXPropertyToCMPIValue(&result, &cmpivalue, &cmpitype);

                SCX_LOGTRACE(m_log, L"BaseProvider::InvokeMethod() - ReturnData");

                rc = CMReturnData(resultHandle, &cmpivalue, cmpitype);

                if (rc.rc != CMPI_RC_OK)
                {
                    throw SCXInternalErrorException(StrAppend(L"CMReturnData() Failed - ", rc.rc),
                                                    SCXSRCLOCATION);
                }

                SCX_LOGTRACE(m_log, L"BaseProvider::InvokeMethod() - ReturnDone")
                    rc = CMReturnDone(resultHandle);

                if (rc.rc != CMPI_RC_OK)
                {
                    throw SCXInternalErrorException(StrAppend(L"CMReturnDone() Failed - ", rc.rc), SCXSRCLOCATION);
                }

                SCX_LOGTRACE(m_log, L"BaseProvider - InvokeMethod - Return OK");
                CMReturn(CMPI_RC_OK);
            }
            else
            {
                SCX_LOGINFO(m_log, wstring(L"BaseProvider::InvokeMethod() - Invalid class - ").
                            append(StrFromUTF8(CMGetCharPtr(pObjectPath->ft->toString(pObjectPath, NULL)))));
                CMReturn(CMPI_RC_ERR_INVALID_CLASS);
            }
        }
        catch (const SCXCIMInstanceNotFound& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::InvokeMethod() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_FOUND);
        }
        catch (const SCXAccessViolationException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::InvokeMethod() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_ACCESS_DENIED);
        }
        catch (const SCXNotSupportedException& e)
        {
            SCX_LOGINFO(m_log, wstring(L"BaseProvider::InvokeMethod() - ").
                        append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, wstring(L"BaseProvider::InvokeMethod() - ").
                           append(e.What()).append(L" - ").append(e.Where()));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (std::exception &e) {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::InvokeMethod() - ").append(DumpString(e)));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
        catch (...)
        {
            SCX_LOGERROR(m_log, wstring(L"BaseProvider::InvokeMethod() - Unknown exception"));
            CMReturn(CMPI_RC_ERR_FAILED);
        }
    }
    /*----------------------------------------------------------------------------*/


    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for virtual method that do instance name enumeration

       \param[in]   callContext The context of the call - see SCXCallContext
       \param[out]  names       Collection of instances with key properties
    */
    void BaseProvider::DoEnumInstanceNames(const SCXCallContext& /* callContext */,
                                           SCXInstanceCollection& /*names*/)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::DoEnumInstanceNames - Default implementation returns not supported");
        throw SCXNotSupportedException(L"DoEnumInstanceNames", SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for virtual method that do instance enumeration

       \param[in]   callContext The context of the call - see SCXCallContext
       \param[out]  instances  RETURN collection of instances
    */
    void BaseProvider::DoEnumInstances(const SCXCallContext& /* callContext */,
                                       SCXInstanceCollection& /* instances */)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::DoEnumInstances - Default implementation returns not supported");
        throw SCXNotSupportedException(L"DoEnumInstances", SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for virtual method that retrieves an instance

       \param[in]  callContext    Context of the original request, indicating which instance to retrieve
       \param[out] instance       The selected instance

       If the provider implementation cannot find supplied instance it should throw
       SCXCIMInstanceNotFound.

    */
    void BaseProvider::DoGetInstance(const SCXCallContext& /* callContext */, SCXInstance& /* instance */)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::DoGetInstance - Default implementation returns not supported");
        throw SCXNotSupportedException(L"DoGetInstance", SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for virtual method for creating CIM instances

       \param[in]  callContext Context of the original request, indicating which instance to retrieve
       \param[in]  instance    The full instance to be created
       \param[in]  createdObjectPath An instance containing the object path of created instance.

       If the instance already exist it should throw SCXCIMInstanceAlreadyExists.
       Other problems can be reported via SCXCIMInstanceManipError.

    */
    void BaseProvider::DoCreateInstance(const SCXCallContext& /* callContext */, const SCXInstance& /* instance */, SCXInstance& /*createdObjectPath*/)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::DoGetInstance - Default implementation returns not supported");
        throw SCXNotSupportedException(L"DoGetInstance", SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for returning one instance name at a time

       \param[in]  instance    The full instance to be created

       Errors generally come from the CMPI layer, and are one of two flavors:
         SCXResourceExhaustedException: Out of resources creating instance
         SCXInternalErrorException: Error returning instance
    */
    void BaseProvider::SendInstanceName(const SCXInstance& instance)
    {
        SCXASSERT(m_objectPath != NULL);
        SCXASSERT(m_result != NULL);

        CMPIObjectPath* pCmpiObjectPath = GetNewObjectPath(m_objectPath);
        CMPIStatus rc;

        SCXInstanceGetKeys(&instance, pCmpiObjectPath);

        rc = CMReturnObjectPath(m_result, pCmpiObjectPath);

        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMReturnObjectPath() Failed - ", rc.rc),
                                            SCXSRCLOCATION);
        }

        SCX_LOGHYSTERICAL(m_log, L"BaseProvider::SendInstanceName() - Add instance for returning");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for returning one instance at a time

       \param[in]  instance    The full instance to be created

       Errors generally come from the CMPI layer, and are one of two flavors:
         SCXResourceExhaustedException: Out of resources creating instance
         SCXInternalErrorException: Error returning instance
    */
    void BaseProvider::SendInstance(const SCXInstance& instance)
    {
        SCXASSERT(m_objectPath != NULL);
        SCXASSERT(m_result != NULL);

        CMPIObjectPath* pCmpiObjectPath = GetNewObjectPath(m_objectPath);
        CMPIStatus rc;

        SCXInstanceGetKeys(&instance, pCmpiObjectPath);

        CMPIInstance *pInstance = CMNewInstance(m_broker, pCmpiObjectPath, &rc);

        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXResourceExhaustedException(L"CMPI Instance", StrAppend(L"CMNewInstance() failed - ", rc.rc),
                                                SCXSRCLOCATION);
        }

        SCXInstanceToCMPIInstance(&instance, pInstance);

        rc = CMReturnInstance(m_result, pInstance);
        if (rc.rc != CMPI_RC_OK)
        {
            throw SCXInternalErrorException(StrAppend(L"CMReturnInstance() Failed - ", rc.rc),
                                            SCXSRCLOCATION);
        }
        SCX_LOGHYSTERICAL(m_log, L"BaseProvider::SendInstance() - Add instance for returning");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for virtual method that do query execution
       Redefine this method in the provider to support queries. You also have
       to add "7" to the set of integers named  "ProviderType" in the .mof-file
    */
    void BaseProvider::DoExecQuery(const SCXProviderLib::SCXCallContext& /* callContext */,
                                   SCXInstanceCollection& /* instances */,
                                   std::wstring /* query */, std::wstring /* language */)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::DoExecQuery - Default implementation returns not supported");
        throw SCXNotSupportedException(L"DoExecQuery", SCXSRCLOCATION);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for virtual method that do cleanup
    */
    void BaseProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"BaseProvider::DoCleanup - Default implementation");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default implementation for virtual method that do method invocation

       \param[in]   callContext  Keys indicating instance to execute method on
       \param[in]   args         Arguments provided for method call
       \param[in]   outargs      Out arguments provided for method call
       \param[in]   methodname   Name of method called
       \param[in]   result       RETURN result property

       \throws      SCXNotSupportedException

       \note Currently only string results (i.e. a single string) are supported.
       Since CMPI allows a method to return other types as well this might
       be added later.

       If the provider implementation cannot find supplied instance it should throw
       SCXCIMInstanceNotFound.


    */
    void BaseProvider::DoInvokeMethod(
        const SCXCallContext& /* callContext */,
        const wstring&        /* methodname */,
        const SCXArgs&        /* args */,
        SCXArgs&              /* outargs */,
        SCXProperty&          /* result */)
    {
        SCX_LOGWARNING(m_log, L"BaseProvider::DoInvokeMethod - Default implementation returns not supported");
        throw SCXNotSupportedException(L"DoInvokeMethod", SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the keys that specify the SCX_OperatingSystem instance.

       \param[out]   csName Value of CSName key
       \param[out]   csCreationClassName Value of CSCreationClassName key
       \param[out]   name Value of name key.
       \param[out]   creationClassName Value of CreationClassName key.

    */
    void BaseProvider::GetKeysForSCX_OperatingSystem(std::wstring& csName,
                                                   std::wstring& csCreationClassName,
                                                   std::wstring& name,
                                                   std::wstring& creationClassName)
    {
        GetKeysForSCX_ComputerSystem(csName, csCreationClassName);
        creationClassName = s_cOSCreationClassName;
        try {
            static SCXSystemLib::SCXOSTypeInfo osinfo;
            name = osinfo.GetOSName(true);
        } catch (SCXException& e) {
            SCX_LOGWARNING(SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup_lib.baseprovider"),
                           StrAppend(
                               StrAppend(L"Can't read host/domainname because ", e.What()),
                               e.Where()));
        }

    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the keys that specify the SCX_ComputerSystem instance.

       \param[out]   name Value of Name key
       \param[out]   creationClassName Value of CreationClassName key

    */
    void BaseProvider::GetKeysForSCX_ComputerSystem(std::wstring& name,               // private
                                                  std::wstring& creationClassName)
    {
        creationClassName = s_cCSCreationClassName;

        try {
            SCXCoreLib::NameResolver mi;
            name = mi.GetHostDomainname();
        } catch (SCXException& e) {
            SCX_LOGWARNING(SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup_lib.baseprovider"),
                           StrAppend(
                               StrAppend(L"Can't read host/domain name because ", e.What()),
                               e.Where()));
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
     * Get (demand) a reference to a key of an instance.
     * A populated instance should always include its identity, that is, its keys.
     * The identity consists of all keys in combination, that is, all keys must be present.
     * GetKeyRef makes sure that the required key actually exists.
     * \note The requirement that
     * the key must exist, makes it possible to return a reference instead of a pointer.
     * \param[in]   key     Name of key
     * \param[in]   keys    Instance hosting all keys
     * \throws      SCXInvalidArgumentException     Key does not exist
     */
    const SCXProviderLib::SCXProperty &BaseProvider::GetKeyRef(const std::wstring &key, const SCXProviderLib::SCXInstance &keys) {
        const SCXProperty *prop = keys.GetKey(key);
        if (prop == 0) {
            std::wostringstream message;
            message << L"No " << key << L" key found";
            throw SCXInvalidArgumentException(L"keys", message.str(), SCXSRCLOCATION);
        }
        return *prop;
    }

    /*----------------------------------------------------------------------------*/
    /**
     *  Validate that a key of an instance exists, and have a certain value.
     * \param[in]   name     Name of key
     * \param[in]   instance Instance hosting all keys
     * \param[in]   value    Expected value
     * \throws      SCXCIMInstanceNotFound     No instance having the key value
     */
    void BaseProvider::ValidateKeyValue(const std::wstring &name, const SCXProviderLib::SCXInstance &instance, const std::wstring &value) {
        if (GetKeyRef(name, instance).GetStrValue() != value) {
            std::wostringstream wo;
            wo << L"Expected " << name << L"=" << value << L" found " << instance.DumpString();
            throw SCXCIMInstanceNotFound(wo.str(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
     * Validate that the keys of a scoping computer system is present
     * The fact that an instance is scoped (contained) by a compuyter system is reflected
     * by the instance including the identity (keys) of the scoping computer system. Make
     * sure they are present and have the correct value
     * \note We know the scoping computer system by definition.
     * \param[in]   keys    Instance hosting keys
     */
    void BaseProvider::ValidateScopingComputerSystemKeys(const SCXProviderLib::SCXInstance& keys) {
        std::wstring csName;
        std::wstring csCreationClassName;
        std::wstring osName;
        std::wstring osCreationClassName;
        GetKeysForSCX_ComputerSystem(csName, csCreationClassName);

        ValidateKeyValue(L"CSName", keys, csName);
        ValidateKeyValue(L"CSCreationClassName", keys, csCreationClassName);
    }

    /*----------------------------------------------------------------------------*/
    /**
     * Add keys of scoping computer system to instance
     * The fact that an instance is scoped (contained) by a computer system is reflected
     * by the instance including the identity (keys) of the scoping computer system. Add them
     * to the instance in question.
     * \note We know the scoping computer system by definition.
     * \param[in]   instance    To be added to
     */
    void BaseProvider::AddScopingComputerSystemKeys(SCXProviderLib::SCXInstance &instance) {
        std::wstring csName;
        std::wstring csCreationClassName;
        GetKeysForSCX_ComputerSystem(csName, csCreationClassName);
        SCXProperty csNameProp(L"CSName", csName);
        instance.AddKey(csNameProp);
        SCXProperty csCreationClassNameProp(L"CSCreationClassName", csCreationClassName);
        instance.AddKey(csCreationClassNameProp);
    }

    /*----------------------------------------------------------------------------*/
    /**
     * Validate that the keys of a scoping system is present.
     * The fact that an instance is scoped (contained) by a system is reflected
     * by the instance including the identity (keys) of the scoping system. Make
     * sure they are present and have the correct value
     * \note We know the scoping system by definition.
     * \param[in]   keys    Instance hosting keys
     */
    void BaseProvider::ValidateScopingSystemKeys(const SCXProviderLib::SCXInstance& keys) {
        std::wstring systemName;
        std::wstring systemCreationClassName;
        GetKeysForSCX_ComputerSystem(systemName, systemCreationClassName);

        ValidateKeyValue(L"SystemName", keys, systemName);
        ValidateKeyValue(L"SystemCreationClassName", keys, systemCreationClassName);
    }

    /*----------------------------------------------------------------------------*/
    /**
     * Add keys of scoping system to instance
     * The fact that an instance is scoped (contained) by a system is reflected
     * by the instance including the identity (keys) of the scoping system. Add them
     * to the instance in question.
     * \note We know the scoping system by definition.
     * \param[in]   instance    To be added to
     */
    void BaseProvider::AddScopingSystemKeys(SCXProviderLib::SCXInstance &instance) {
        std::wstring systemName;
        std::wstring systemCreationClassName;
        GetKeysForSCX_ComputerSystem(systemName, systemCreationClassName);
        SCXProperty systemNameProp(L"SystemName", systemName);
        instance.AddKey(systemNameProp);
        SCXProperty systemCreationClassNameProp(L"SystemCreationClassName", systemCreationClassName);
        instance.AddKey(systemCreationClassNameProp);

    }

    /*----------------------------------------------------------------------------*/
    /**
     * Validate that the keys of a scoping operating system is present
     * The fact that an instance is scoped (contained) by an operating system is reflected
     * by the instance including the identity (keys) of the scoping operating system. Make
     * sure they are present and have the correct value
     * \note We know the scoping operating system by definition.
     * \param[in]   keys    Instance hosting keys
     */
    void BaseProvider::ValidateScopingOperatingSystemKeys(const SCXProviderLib::SCXInstance& keys) {
        std::wstring csName;
        std::wstring csCreationClassName;
        std::wstring osName;
        std::wstring osCreationClassName;
        GetKeysForSCX_OperatingSystem(csName, csCreationClassName, osName, osCreationClassName);

        ValidateKeyValue(L"CSName", keys, csName);
        ValidateKeyValue(L"CSCreationClassName", keys, csCreationClassName);
        ValidateKeyValue(L"OSName", keys, osName);
        ValidateKeyValue(L"OSCreationClassName", keys, osCreationClassName);
    }

    /*----------------------------------------------------------------------------*/
    /**
     * Add keys of scoping operating system to instance
     * The fact that an instance is scoped (contained) by an operating system is reflected
     * by the instance including the identity (keys) of the scoping operating system. Add them
     * to the instance in question.
     * \note We know the scoping operating system by definition.
     * \param[in]   instance    To be added to
     */
    void BaseProvider::AddScopingOperatingSystemKeys(SCXProviderLib::SCXInstance &instance) {
        std::wstring csName;
        std::wstring csCreationClassName;
        std::wstring osName;
        std::wstring osCreationClassName;
        GetKeysForSCX_OperatingSystem(csName, csCreationClassName, osName, osCreationClassName);
        SCXProperty csNameProp(L"CSName", csName);
        instance.AddKey(csNameProp);
        SCXProperty csCreationClassNameProp(L"CSCreationClassName", csCreationClassName);
        instance.AddKey(csCreationClassNameProp);
        SCXProperty osNameProp(L"OSName", osName);
        instance.AddKey(osNameProp);
        SCXProperty osCreationClassNameProp(L"OSCreationClassName", osCreationClassName);
        instance.AddKey(osCreationClassNameProp);

    }

    //! Class name of scoping operating system
    const std::wstring BaseProvider::s_cOSCreationClassName(L"SCX_OperatingSystem");

    //! Class name of scoping computer system
    const std::wstring BaseProvider::s_cCSCreationClassName(L"SCX_ComputerSystem");


}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
