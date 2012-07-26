//%LICENSE////////////////////////////////////////////////////////////////
//
// Licensed to The Open Group (TOG) under one or more contributor license
// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
// this work for additional information regarding copyright ownership.
// Each contributor licenses this file to you under the OpenPegasus Open
// Source License; you may not use this file except in compliance with the
// License.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////
//
//%/////////////////////////////////////////////////////////////////////////////

#include <Pegasus/Common/Tracer.h>

#include "IndicationConstants.h"
#include "IndicationServiceConfiguration.h"

PEGASUS_NAMESPACE_BEGIN

IndicationServiceConfiguration::IndicationServiceConfiguration(
    CIMRepository * repository)
    : _cimRepository (repository)
{
}

IndicationServiceConfiguration::~IndicationServiceConfiguration()
{
}


CIMInstance IndicationServiceConfiguration::getInstance(
    const CIMNamespaceName & nameSpace,
    const CIMObjectPath & instanceName,
    Boolean localOnly,
    Boolean includeQualifiers,
    Boolean includeClassOrigin,
    const CIMPropertyList & propertyList)
{
    CIMInstance instance;

    if (instanceName.getClassName().equal(
        PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE))
    {
        instance = _getIndicationServiceInstance(
            includeQualifiers,
            includeClassOrigin,
            propertyList);
    }
    else
    {
        PEGASUS_ASSERT(instanceName.getClassName().equal(
            PEGASUS_CLASSNAME_CIM_INDICATIONSERVICECAPABILITIES));
        instance = _getIndicationServiceCapabilitiesInstance(
            includeQualifiers,
            includeClassOrigin,
            propertyList);
    }
    CIMObjectPath path = instanceName;
    path.setNameSpace(nameSpace);

    if (!path.identical(instance.getPath()))
    {
        throw PEGASUS_CIM_EXCEPTION(
            CIM_ERR_NOT_FOUND,
            instanceName.toString());
    }

    return instance;
}

Array<CIMInstance> IndicationServiceConfiguration::
    enumerateInstancesForClass(
        const CIMNamespaceName & nameSpace,
        const CIMName & className,
        Boolean localOnly,
        Boolean includeQualifiers,
        Boolean includeClassOrigin,
        const CIMPropertyList & propertyList)
{
    Array<CIMInstance> instances;
    if (className.equal(PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE))
    {
        instances.append(_getIndicationServiceInstance(
            includeQualifiers,
            includeClassOrigin,
            propertyList));
    }
    else
    {
        PEGASUS_ASSERT(className.equal(
            PEGASUS_CLASSNAME_CIM_INDICATIONSERVICECAPABILITIES));
        instances.append(_getIndicationServiceCapabilitiesInstance(
            includeQualifiers,
            includeClassOrigin,
            propertyList));
    }

    return instances;
}

Array <CIMObjectPath> IndicationServiceConfiguration::
    enumerateInstanceNamesForClass(
        const CIMNamespaceName & nameSpace,
        const CIMName & className)
{
    Array<CIMObjectPath> instanceNames;
    if (className.equal(PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE))
    {
        instanceNames.append(_getIndicationServiceInstance(
            false,
            false,
            CIMPropertyList()).getPath());
    }
    else
    {
        PEGASUS_ASSERT(className.equal(
            PEGASUS_CLASSNAME_CIM_INDICATIONSERVICECAPABILITIES));
        instanceNames.append(_getIndicationServiceCapabilitiesInstance(
            false,
            false,
            CIMPropertyList()).getPath());
    }

    return instanceNames;
}

CIMInstance IndicationServiceConfiguration::_buildInstanceSkeleton(
    const CIMNamespaceName & nameSpace,
    const CIMName& className,
    CIMClass &returnedClass)
{
    returnedClass = _cimRepository->getClass(
        nameSpace,
        className,
        false,
        true,
        true);

    CIMInstance skeleton = returnedClass.buildInstance(
        true,
        true,
        CIMPropertyList());

    return skeleton;
}

void IndicationServiceConfiguration::_setPropertyValue(
    CIMInstance& instance,
    const CIMName& propertyName,
    const CIMValue & value)
{
    Uint32 pos;

    if((pos = instance.findProperty(propertyName)) != PEG_NOT_FOUND)
    {
        instance.getProperty(pos).setValue(value);
    }
}

CIMInstance IndicationServiceConfiguration::_getIndicationServiceInstance(
    Boolean includeQualifiers,
    Boolean includeClassOrigin,
    const CIMPropertyList &propertyList)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationServiceConfiguration::"
            "_getIndicationServiceInstance");

    CIMInstance instance;
    CIMClass returnedClass;

    instance = _buildInstanceSkeleton(
        PEGASUS_NAMESPACENAME_INTEROP,
        PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE,
        returnedClass);

    _setPropertyValue(
        instance,
        _PROPERTY_SYSTEMCREATIONCLASSNAME,
        PEGASUS_CLASSNAME_PG_COMPUTERSYSTEM.getString());

    _setPropertyValue(
        instance,
        _PROPERTY_SYSTEMNAME,
        System::getFullyQualifiedHostName());

     _setPropertyValue(
        instance,
        _PROPERTY_CREATIONCLASSNAME,
        PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE.getString());

    _setPropertyValue(
        instance,
        _PROPERTY_NAME,
        String(PEGASUS_INSTANCEID_GLOBAL_PREFIX) + ":" + "IndicationService");

    _setPropertyValue(
        instance,
        _PROPERTY_ELEMENTNAME,
        String("IndicationService"));

    Array<Uint16> operationalStatus;
    operationalStatus.append(2);
    _setPropertyValue(
        instance,
        _PROPERTY_OPERATIONALSTATUS,
        operationalStatus);

    _setPropertyValue(
        instance,
        _PROPERTY_STARTED,
        CIMValue(Boolean(true)));

    _setPropertyValue(
        instance,
        _PROPERTY_DESCRIPTION,
        String("Pegasus Indication Service"));

    _setPropertyValue(
        instance,
        _PROPERTY_FILTERCREATIONENABLED,
        CIMValue(_PROPERTY_FILTERCREATIONENABLED_VALUE));

    _setPropertyValue(
        instance,
        _PROPERTY_SUBSCRIPTIONREMOVALACTION,
        CIMValue(_PROPERTY_SUBSCRIPTIONREMOVALACTION_VALUE));

    _setPropertyValue(
        instance,
        _PROPERTY_SUBSCRIPTIONREMOVALTIMEINTERVAL,
        CIMValue(_PROPERTY_SUBSCRIPTIONREMOVALTIMEINTERVAL_VALUE));

    _setPropertyValue(
        instance,
        _PROPERTY_DELIVERYRETRYATTEMPTS,
        CIMValue(_PROPERTY_DELIVERYRETRYATTEMPTS_VALUE));

    _setPropertyValue(
        instance,
        _PROPERTY_DELIVERYRETRYINTERVAL,
        CIMValue(_PROPERTY_DELIVERYRETRYINTERVAL_VALUE));

    CIMObjectPath path = instance.buildPath(returnedClass);
    path.setNameSpace(PEGASUS_NAMESPACENAME_INTEROP);
    instance.setPath(path);
    instance.filter(includeQualifiers, includeClassOrigin, propertyList);

    PEG_METHOD_EXIT();

    return instance;
}

CIMInstance IndicationServiceConfiguration::
    _getIndicationServiceCapabilitiesInstance(
        Boolean includeQualifiers,
        Boolean includeClassOrigin,
        const CIMPropertyList &propertyList)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationServiceConfiguration::"
            "_getIndicationServiceCapabilitiesInstance");

    CIMInstance instance;
    CIMClass returnedClass;

    instance = _buildInstanceSkeleton(
        PEGASUS_NAMESPACENAME_INTEROP,
        PEGASUS_CLASSNAME_CIM_INDICATIONSERVICECAPABILITIES,
        returnedClass);

    _setPropertyValue(
        instance,
        _PROPERTY_INSTANCEID,
        String("CIM:PegasusIndicationServiceCapabilities"));

    _setPropertyValue(
        instance,
        _PROPERTY_CAPTION,
        String("IndicationService Capabilities"));

    _setPropertyValue(
        instance,
        _PROPERTY_DESCRIPTION,
        String("Pegasus Indication Service Capabilities"));

    _setPropertyValue(
        instance,
        _PROPERTY_ELEMENTNAME,
        String("Capabilities for IndicationService"));

    _setPropertyValue(
        instance,
        _PROPERTY_FILTERCREATIONENABLEDISSETTABLE,
        CIMValue(Boolean(false)));

    _setPropertyValue(
        instance,
        _PROPERTY_DELIVERYRETRYATTEMPTSISSETTABLE,
        CIMValue(Boolean(false)));

    _setPropertyValue(
        instance,
        _PROPERTY_DELIVERYRETRYINTERVALISSETTABLE,
        CIMValue(Boolean(false)));

    _setPropertyValue(
        instance,
        _PROPERTY_SUBSCRIPTIONREMOVALACTIONISSETTABLE,
        CIMValue(Boolean(false)));

    _setPropertyValue(
        instance,
        _PROPERTY_SUBSCRIPTIONREMOVALTIMEINTERVALISSETTABLE,
        CIMValue(Boolean(false)));

    CIMValue value(CIMTYPE_UINT32, false);

    _setPropertyValue(
        instance,
        _PROPERTY_MAXLISTENERDESTINATIONS,
        value); 

    _setPropertyValue(
        instance,
        _PROPERTY_MAXACTIVESUBSCRIPTIONS,
        value);

    _setPropertyValue(
        instance,
        _PROPERTY_SUBSCRIPTIONSPERSISTED,
        CIMValue(Boolean(true)));

    CIMObjectPath path = instance.buildPath(returnedClass);
    path.setNameSpace(PEGASUS_NAMESPACENAME_INTEROP);
    instance.setPath(path);
    instance.filter(includeQualifiers, includeClassOrigin, propertyList);

    PEG_METHOD_EXIT();
    return instance;
}

PEGASUS_NAMESPACE_END
