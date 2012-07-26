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

#include <Pegasus/Common/XmlReader.h>
#include <Pegasus/Common/OperationContextInternal.h>
#include <Pegasus/Common/System.h>

#include "CIMBinMsgDeserializer.h"
#include "BinaryCodec.h"

PEGASUS_NAMESPACE_BEGIN

CIMMessage* CIMBinMsgDeserializer::deserialize(
    CIMBuffer& in,
    size_t size)
{
    if (size == 0)
        return 0;

    CIMMessage* msg = 0;
    String typeString;
    OperationContext operationContext;
    bool present;

    // [messageId]

    String messageID;

    if (!in.getString(messageID))
        return 0;

    // [binaryRequest]

    Boolean binaryRequest;

    if (!in.getBoolean(binaryRequest))
        return 0;

    // [binaryResponse]

    Boolean binaryResponse;

    if (!in.getBoolean(binaryResponse))
        return 0;

    // [type]

    MessageType type;
    {
        Uint32 tmp;

        if (!in.getUint32(tmp))
            return 0;

        type = MessageType(tmp);
    }

#ifndef PEGASUS_DISABLE_PERFINST

    // [serverStartTimeMicroseconds]
    Uint64 serverStartTimeMicroseconds;
    
    if (!in.getUint64(serverStartTimeMicroseconds))
        return false;

    // [providerTimeMicroseconds]
    Uint64 providerTimeMicroseconds;

    if (!in.getUint64(providerTimeMicroseconds))
        return false;

#endif

    // [isComplete]

    Boolean isComplete;

    if (!in.getBoolean(isComplete))
        return false;

    // [index]

    Uint32 index;

    if (!in.getUint32(index))
        return false;

    // [operationContext]

    if (!_getOperationContext(in, operationContext))
        return 0;

    // [CIMRequestMessage]

    if (!in.getPresent(present))
        return 0;

    if (present)
    {
        if (!(msg = _getRequestMessage(in, type)))
            return 0;
    }

    // [CIMResponseMessage]

    if (!in.getPresent(present))
        return 0;

    if (present)
    {
        if (!(msg = _getResponseMessage(in, type, binaryResponse)))
            return 0;
    }

    // Initialize the messge:

    msg->messageId = messageID;
    msg->binaryRequest = binaryRequest;
    msg->binaryResponse = binaryResponse;
#ifndef PEGASUS_DISABLE_PERFINST
    msg->setServerStartTime(serverStartTimeMicroseconds);
    msg->setProviderTime(providerTimeMicroseconds);
#endif
    msg->setComplete(isComplete);
    msg->setIndex(index);
    msg->operationContext = operationContext;

    return msg;
}

CIMRequestMessage* CIMBinMsgDeserializer::_getRequestMessage(
    CIMBuffer& in,
    MessageType type)
{
    CIMRequestMessage* msg = 0;
    XmlEntry entry;
    QueueIdStack queueIdStack;
    Boolean present;

    // [queueIdStack]

    _getQueueIdStack(in, queueIdStack);

    // [CIMOperationRequestMessage]

    if (!in.getPresent(present))
        return 0;

    if (present)
    {
        // [userInfo]

        String authType;
        String userName;

        if (!_getUserInfo(in, authType, userName))
            return 0;

        // [nameSpace]

        CIMNamespaceName nameSpace;

        if (!_getNamespaceName(in, nameSpace))
            return 0;

        // [className]

        CIMName className;

        if (!_getName(in, className))
            return 0;

        // [providerType]

        Uint32 providerType;

        if (!in.getUint32(providerType))
            return 0;

        // [message]

        CIMOperationRequestMessage* oreq = 0;

        switch (type)
        {
            case CIM_GET_INSTANCE_REQUEST_MESSAGE:
                oreq = _getGetInstanceRequestMessage(in);
                break;
            case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
                oreq = _getDeleteInstanceRequestMessage(in);
                break;
            case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
                oreq = _getCreateInstanceRequestMessage(in);
                break;
            case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
                oreq = _getModifyInstanceRequestMessage(in);
                break;
            case CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE:
                oreq = _getEnumerateInstancesRequestMessage(in);
                break;
            case CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE:
                oreq = _getEnumerateInstanceNamesRequestMessage(in);
                break;
            case CIM_EXEC_QUERY_REQUEST_MESSAGE:
                oreq = _getExecQueryRequestMessage(in);
                break;
            case CIM_GET_PROPERTY_REQUEST_MESSAGE:
                oreq = _getGetPropertyRequestMessage(in);
                break;
            case CIM_SET_PROPERTY_REQUEST_MESSAGE:
                oreq = _getSetPropertyRequestMessage(in);
                break;
            case CIM_ASSOCIATORS_REQUEST_MESSAGE:
                oreq = _getAssociatorsRequestMessage(in);
                break;
            case CIM_ASSOCIATOR_NAMES_REQUEST_MESSAGE:
                oreq = _getAssociatorNamesRequestMessage(in);
                break;
            case CIM_REFERENCES_REQUEST_MESSAGE:
                oreq = _getReferencesRequestMessage(in);
                break;
            case CIM_REFERENCE_NAMES_REQUEST_MESSAGE:
                oreq = _getReferenceNamesRequestMessage(in);
                break;
            case CIM_INVOKE_METHOD_REQUEST_MESSAGE:
                oreq = _getInvokeMethodRequestMessage(in);
                break;
            default:
                PEGASUS_ASSERT(0);
                break;
        }

        if (!oreq)
            return 0;

        oreq->authType = authType;
        oreq->userName = userName;
        oreq->nameSpace = nameSpace;
        oreq->className = className;
        oreq->providerType = providerType;
        msg = oreq;
    }

    // [CIMIndicationRequestMessage]

    if (!in.getPresent(present))
        return 0;

    if (present)
    {
        // [userInfo]

        String authType;
        String userName;

        if (!_getUserInfo(in, authType, userName))
        {
        }

        // [message]

        CIMIndicationRequestMessage* ireq = 0;

        switch (type)
        {
            case CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE:
                ireq = _getCreateSubscriptionRequestMessage(in);
                break;
            case CIM_MODIFY_SUBSCRIPTION_REQUEST_MESSAGE:
                ireq = _getModifySubscriptionRequestMessage(in);
                break;
            case CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE:
                ireq = _getDeleteSubscriptionRequestMessage(in);
                break;
            default:
                PEGASUS_ASSERT(0);
                break;
        }

        if (!ireq)
            return 0;

        // Initialize the message:

        ireq->authType = authType;
        ireq->userName = userName;
        msg = ireq;
    }

    // [other]

    if (!in.getPresent(present))
        return 0;

    if (present)
    {
        switch (type)
        {
            case CIM_EXPORT_INDICATION_REQUEST_MESSAGE:
                msg = _getExportIndicationRequestMessage(in);
                break;
            case CIM_PROCESS_INDICATION_REQUEST_MESSAGE:
                msg = _getProcessIndicationRequestMessage(in);
                break;
            case CIM_DISABLE_MODULE_REQUEST_MESSAGE:
                msg = _getDisableModuleRequestMessage(in);
                break;
            case CIM_ENABLE_MODULE_REQUEST_MESSAGE:
                msg = _getEnableModuleRequestMessage(in);
                break;
            case CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE:
                msg = _getStopAllProvidersRequestMessage(in);
                break;
            case CIM_INITIALIZE_PROVIDER_AGENT_REQUEST_MESSAGE:
                msg =
                    _getInitializeProviderAgentRequestMessage(in);
                break;
            case CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE:
                msg = _getNotifyConfigChangeRequestMessage(in);
                break;
            case CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE:
                msg = _getSubscriptionInitCompleteRequestMessage(in);
                break;
            default:
                PEGASUS_ASSERT(0);
                break;
        }

        if (!msg)
            return 0;
    }

    msg->queueIds = queueIdStack;

    return msg;
}

CIMResponseMessage* CIMBinMsgDeserializer::_getResponseMessage(
    CIMBuffer& in,
    MessageType type,
    bool binaryResponse)
{
    CIMResponseMessage* msg = 0;
    QueueIdStack queueIdStack;
    CIMException cimException;

    // [queueIdStack]

    if (!_getQueueIdStack(in, queueIdStack))
        return 0;

    // [cimException]
    if (!_getException(in, cimException))
        return 0;

    // [message]

    switch (type)
    {
        case CIM_GET_INSTANCE_RESPONSE_MESSAGE:
            msg = _getGetInstanceResponseMessage(in, binaryResponse);
            break;
        case CIM_DELETE_INSTANCE_RESPONSE_MESSAGE:
            msg = _getDeleteInstanceResponseMessage(in);
            break;
        case CIM_CREATE_INSTANCE_RESPONSE_MESSAGE:
            msg = _getCreateInstanceResponseMessage(in);
            break;
        case CIM_MODIFY_INSTANCE_RESPONSE_MESSAGE:
            msg = _getModifyInstanceResponseMessage(in);
            break;
        case CIM_ENUMERATE_INSTANCES_RESPONSE_MESSAGE:
            msg = _getEnumerateInstancesResponseMessage(in, binaryResponse);
            break;
        case CIM_ENUMERATE_INSTANCE_NAMES_RESPONSE_MESSAGE:
            msg = _getEnumerateInstanceNamesResponseMessage(in);
            break;
        case CIM_EXEC_QUERY_RESPONSE_MESSAGE:
            msg = _getExecQueryResponseMessage(in);
            break;
        case CIM_GET_PROPERTY_RESPONSE_MESSAGE:
            msg = _getGetPropertyResponseMessage(in);
            break;
        case CIM_SET_PROPERTY_RESPONSE_MESSAGE:
            msg = _getSetPropertyResponseMessage(in);
            break;
        case CIM_ASSOCIATORS_RESPONSE_MESSAGE:
            msg = _getAssociatorsResponseMessage(in);
            break;
        case CIM_ASSOCIATOR_NAMES_RESPONSE_MESSAGE:
            msg = _getAssociatorNamesResponseMessage(in);
            break;
        case CIM_REFERENCES_RESPONSE_MESSAGE:
            msg = _getReferencesResponseMessage(in);
            break;
        case CIM_REFERENCE_NAMES_RESPONSE_MESSAGE:
            msg = _getReferenceNamesResponseMessage(in);
            break;
        case CIM_INVOKE_METHOD_RESPONSE_MESSAGE:
            msg = _getInvokeMethodResponseMessage(in);
            break;
        case CIM_CREATE_SUBSCRIPTION_RESPONSE_MESSAGE:
            msg = _getCreateSubscriptionResponseMessage(in);
            break;
        case CIM_MODIFY_SUBSCRIPTION_RESPONSE_MESSAGE:
            msg = _getModifySubscriptionResponseMessage(in);
            break;
        case CIM_DELETE_SUBSCRIPTION_RESPONSE_MESSAGE:
            msg = _getDeleteSubscriptionResponseMessage(in);
            break;
        case CIM_EXPORT_INDICATION_RESPONSE_MESSAGE:
            msg = _getExportIndicationResponseMessage(in);
            break;
        case CIM_PROCESS_INDICATION_RESPONSE_MESSAGE:
            msg = _getProcessIndicationResponseMessage(in);
            break;
        case CIM_DISABLE_MODULE_RESPONSE_MESSAGE:
            msg = _getDisableModuleResponseMessage(in);
            break;
        case CIM_ENABLE_MODULE_RESPONSE_MESSAGE:
            msg = _getEnableModuleResponseMessage(in);
            break;
        case CIM_STOP_ALL_PROVIDERS_RESPONSE_MESSAGE:
            msg = _getStopAllProvidersResponseMessage(in);
            break;
        case CIM_INITIALIZE_PROVIDER_AGENT_RESPONSE_MESSAGE:
            msg =
                _getInitializeProviderAgentResponseMessage(in);
            break;
        case CIM_NOTIFY_CONFIG_CHANGE_RESPONSE_MESSAGE:
            msg = _getNotifyConfigChangeResponseMessage(in);
            break;
        case CIM_SUBSCRIPTION_INIT_COMPLETE_RESPONSE_MESSAGE:
            msg = _getSubscriptionInitCompleteResponseMessage(in);
            break;
        default:
            PEGASUS_ASSERT(0);
            break;
    }

    if (!msg)
        return 0;

    msg->queueIds = queueIdStack;
    msg->cimException = cimException;

    return msg;
}

Boolean CIMBinMsgDeserializer::_getUserInfo(
    CIMBuffer& in,
    String& authType,
    String& userName)
{
    if (!in.getString(authType))
        return false;

    if (!in.getString(userName))
        return false;

    return true;
}

Boolean CIMBinMsgDeserializer::_getQueueIdStack(
    CIMBuffer& in,
    QueueIdStack& queueIdStack)
{
    Uint32 size;

    if (!in.getUint32(size))
        return false;

    for (Uint32 i = 0; i < size; i++)
    {
        Uint32 tmp;

        if (!in.getUint32(tmp))
            return false;

        queueIdStack.push(tmp);
    }

    return true;
}

Boolean CIMBinMsgDeserializer::_getOperationContext(
    CIMBuffer& in,
    OperationContext& operationContext)
{
    operationContext.clear();

    XmlEntry entry;
    bool present;

    // [IdentityContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        String userName;

        if (!in.getString(userName))
            return false;

        operationContext.insert(IdentityContainer(userName));
    }

    // [SubscriptionInstanceContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        CIMInstance ci;

        if (!_getInstance(in, ci))
            return false;

        operationContext.insert(SubscriptionInstanceContainer(ci));
    }

    // [SubscriptionFilterConditionContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        String filterCondition;
        String queryLanguage;

        if (!in.getString(filterCondition) || !in.getString(queryLanguage))
            return false;

        operationContext.insert(SubscriptionFilterConditionContainer(
            filterCondition, queryLanguage));
    }

    // [SubscriptionFilterQueryContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        String filterQuery;
        String queryLanguage;
        CIMNamespaceName nameSpace;

        if (!in.getString(filterQuery) || !in.getString(queryLanguage))
            return false;

        if (!_getNamespaceName(in, nameSpace))
            return false;

        operationContext.insert(SubscriptionFilterQueryContainer(
            filterQuery, queryLanguage, nameSpace));
    }

    // [SubscriptionInstanceNamesContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        Array<CIMObjectPath> cops;

        if (!in.getObjectPathA(cops))
            return false;

        operationContext.insert(SubscriptionInstanceNamesContainer(cops));
    }

    // [TimeoutContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        Uint32 timeout;

        if (!in.getUint32(timeout))
            return false;

        operationContext.insert(TimeoutContainer(timeout));
    }

    // [AcceptLanguageListContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        AcceptLanguageList acceptLanguages;

        if (!_getAcceptLanguageList(in, acceptLanguages))
            return false;

        operationContext.insert(AcceptLanguageListContainer(acceptLanguages));
    }

    // [ContentLanguageListContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        ContentLanguageList list;

        if (!_getContentLanguageList(in, list))
            return false;

        operationContext.insert(ContentLanguageListContainer(list));
    }

    // [SnmpTrapOidContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        String snmpTrapOid;

        if (!in.getString(snmpTrapOid))
            return false;

        operationContext.insert(SnmpTrapOidContainer(snmpTrapOid));
    }

    // [LocaleContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        String languageId;

        if (!in.getString(languageId))
            return false;

        operationContext.insert(LocaleContainer(languageId));
    }

    // [ProviderIdContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        CIMInstance module;
        CIMInstance provider;
        Boolean isRemoteNameSpace;
        String remoteInfo;
        String provMgrPath;

        if (!_getInstance(in, module))
            return false;

        if (!_getInstance(in, provider))
            return false;

        if (!in.getBoolean(isRemoteNameSpace))
            return false;

        if (!in.getString(remoteInfo))
            return false;

        if (!in.getString(provMgrPath))
            return false;

        ProviderIdContainer pidc(
            module, provider, isRemoteNameSpace, remoteInfo);

        pidc.setProvMgrPath(provMgrPath);

        operationContext.insert(pidc);
    }

    // [CachedClassDefinitionContainer]

    if (!in.getPresent(present))
        return false;

    if (present)
    {
        CIMClass cc;

        if (!in.getClass(cc))
            return false;

        operationContext.insert(CachedClassDefinitionContainer(cc));
    }

    return true;
}

Boolean CIMBinMsgDeserializer::_getContentLanguageList(
    CIMBuffer& in,
    ContentLanguageList& contentLanguages)
{
    contentLanguages.clear();

    Uint32 size;

    if (!in.getUint32(size))
        return false;

    for (Uint32 i = 0; i < size; i++)
    {
        String tmp;

        if (!in.getString(tmp))
            return false;

        contentLanguages.append(LanguageTag(tmp));
    }

    return true;
}

Boolean CIMBinMsgDeserializer::_getAcceptLanguageList(
    CIMBuffer& in,
    AcceptLanguageList& acceptLanguages)
{
    acceptLanguages.clear();

    Uint32 size;

    if (!in.getUint32(size))
        return false;

    for (Uint32 i = 0; i < size; i++)
    {
        String languageTag;
        Real32 qualityValue;

        if (!in.getString(languageTag) || !in.getReal32(qualityValue))
            return false;

        acceptLanguages.insert(LanguageTag(languageTag), qualityValue);
    }

    return true;
}

Boolean CIMBinMsgDeserializer::_getException(
    CIMBuffer& in,
    CIMException& cimException)
{
    Uint32 statusCode;
    String message;
    String cimMessage;
    String file;
    Uint32 line;
    ContentLanguageList contentLanguages;

    if (!in.getUint32(statusCode))
        return false;

    if (!in.getString(message))
        return false;

    if (!in.getString(cimMessage))
        return false;

    if (!in.getString(file))
        return false;

    if (!in.getUint32(line))
        return false;

    if (!_getContentLanguageList(in, contentLanguages))
        return false;

    TraceableCIMException e = TraceableCIMException(contentLanguages,
        CIMStatusCode(statusCode), message, file, line);
    e.setCIMMessage(cimMessage);
    cimException = e;

    return true;
}

Boolean CIMBinMsgDeserializer::_getPropertyList(
    CIMBuffer& in,
    CIMPropertyList& propertyList)
{
    return in.getPropertyList(propertyList);
}

Boolean CIMBinMsgDeserializer::_getObjectPath(
    CIMBuffer& in,
    CIMObjectPath& cop)
{
    return in.getObjectPath(cop);
}

Boolean CIMBinMsgDeserializer::_getInstance(
    CIMBuffer& in,
    CIMInstance& ci)
{
    return in.getInstance(ci);
}

Boolean CIMBinMsgDeserializer::_getNamespaceName(
    CIMBuffer& in,
    CIMNamespaceName& cimNamespaceName)
{
    return in.getNamespaceName(cimNamespaceName);
}

Boolean CIMBinMsgDeserializer::_getName(
    CIMBuffer& in,
    CIMName& cn)
{
    return in.getName(cn);
}

Boolean CIMBinMsgDeserializer::_getObject(
    CIMBuffer& in,
    CIMObject& object)
{
    return in.getObject(object);
}

Boolean CIMBinMsgDeserializer::_getParamValue(
    CIMBuffer& in,
    CIMParamValue& pv)
{
    return in.getParamValue(pv);
}

CIMGetInstanceRequestMessage*
CIMBinMsgDeserializer::_getGetInstanceRequestMessage(CIMBuffer& in)
{
    CIMObjectPath instanceName;
    Boolean localOnly;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    if (!in.getObjectPath(instanceName))
        return 0;

    if (!in.getBoolean(localOnly))
        return 0;

    if (!in.getBoolean(includeQualifiers))
        return 0;

    if (!in.getBoolean(includeClassOrigin))
        return 0;

    if (!in.getPropertyList(propertyList))
        return 0;

    return new CIMGetInstanceRequestMessage(
        String::EMPTY,
        CIMNamespaceName(),
        instanceName,
        localOnly,
        includeQualifiers,
        includeClassOrigin,
        propertyList,
        QueueIdStack());
}

CIMDeleteInstanceRequestMessage*
CIMBinMsgDeserializer::_getDeleteInstanceRequestMessage(
    CIMBuffer& in)
{
    CIMObjectPath instanceName;

    if (!_getObjectPath(in, instanceName))
        return 0;

    return new CIMDeleteInstanceRequestMessage(
        String::EMPTY,
        CIMNamespaceName(),
        instanceName,
        QueueIdStack());
}

CIMCreateInstanceRequestMessage*
CIMBinMsgDeserializer::_getCreateInstanceRequestMessage(
    CIMBuffer& in)
{
    CIMInstance newInstance;

    if (!_getInstance(in, newInstance))
        return 0;

    return new CIMCreateInstanceRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(), 
        newInstance, 
        QueueIdStack());
}

CIMModifyInstanceRequestMessage*
CIMBinMsgDeserializer::_getModifyInstanceRequestMessage(
    CIMBuffer& in)
{
    CIMInstance modifiedInstance;
    Boolean includeQualifiers;
    CIMPropertyList propertyList;

    if (!_getInstance(in, modifiedInstance))
        return false;

    if (!in.getBoolean(includeQualifiers))
        return false;

    if (!_getPropertyList(in, propertyList))
        return false;

    return new CIMModifyInstanceRequestMessage(
        String::EMPTY,
        CIMNamespaceName(),
        modifiedInstance,
        includeQualifiers,
        propertyList,
        QueueIdStack());
}

CIMEnumerateInstancesRequestMessage*
CIMBinMsgDeserializer::_getEnumerateInstancesRequestMessage(
    CIMBuffer& in)
{
    CIMObjectPath instanceName;
    Boolean deepInheritance;
    Boolean localOnly;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    if (!in.getBoolean(deepInheritance))
        return false;

    if (!in.getBoolean(localOnly))
        return false;

    if (!in.getBoolean(includeQualifiers))
        return false;

    if (!in.getBoolean(includeClassOrigin))
        return false;

    if (!_getPropertyList(in, propertyList))
        return false;

    return new CIMEnumerateInstancesRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(), 
        CIMName(),
        deepInheritance,
        localOnly,
        includeQualifiers,
        includeClassOrigin,
        propertyList,
        QueueIdStack());
}

CIMEnumerateInstanceNamesRequestMessage*
CIMBinMsgDeserializer::_getEnumerateInstanceNamesRequestMessage(
    CIMBuffer& in)
{
    return new CIMEnumerateInstanceNamesRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(), 
        CIMName(), 
        QueueIdStack());
}

CIMExecQueryRequestMessage*
CIMBinMsgDeserializer::_getExecQueryRequestMessage(
    CIMBuffer& in)
{
    String queryLanguage;
    String query;

    if (!in.getString(queryLanguage) || !in.getString(query))
        return false;

    return new CIMExecQueryRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(),
        queryLanguage, 
        query, 
        QueueIdStack());
}

CIMAssociatorsRequestMessage*
CIMBinMsgDeserializer::_getAssociatorsRequestMessage(
    CIMBuffer& in)
{
    CIMObjectPath objectName;
    CIMName assocClass;
    CIMName resultClass;
    String role;
    String resultRole;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    if (!_getObjectPath(in, objectName))
        return false;

    if (!_getName(in, assocClass))
        return false;

    if (!_getName(in, resultClass))
        return false;

    if (!in.getString(role))
        return false;

    if (!in.getString(resultRole))
        return false;

    if (!in.getBoolean(includeQualifiers))
        return false;

    if (!in.getBoolean(includeClassOrigin))
        return false;

    if (!_getPropertyList(in, propertyList))
        return false;

    return new CIMAssociatorsRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(),
        objectName, 
        assocClass, 
        resultClass, 
        role, 
        resultRole,
        includeQualifiers, 
        includeClassOrigin, 
        propertyList, 
        QueueIdStack());
}

CIMAssociatorNamesRequestMessage*
CIMBinMsgDeserializer::_getAssociatorNamesRequestMessage(
    CIMBuffer& in)
{
    CIMObjectPath objectName;
    CIMName assocClass;
    CIMName resultClass;
    String role;
    String resultRole;

    if (!_getObjectPath(in, objectName))
        return false;

    if (!_getName(in, assocClass))
        return false;

    if (!_getName(in, resultClass))
        return false;

    if (!in.getString(role))
        return false;

    if (!in.getString(resultRole))
        return false;

    return new CIMAssociatorNamesRequestMessage(
        String::EMPTY,
        CIMNamespaceName(), 
        objectName, 
        assocClass, 
        resultClass, 
        role,
        resultRole, 
        QueueIdStack()); 
}

CIMReferencesRequestMessage*
CIMBinMsgDeserializer::_getReferencesRequestMessage(CIMBuffer& in)
{
    CIMObjectPath objectName;
    CIMName resultClass;
    String role;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    if (!_getObjectPath(in, objectName))
        return false;

    if (!_getName(in, resultClass))
        return false;

    if (!in.getString(role))
        return false;

    if (!in.getBoolean(includeQualifiers))
        return false;

    if (!in.getBoolean(includeClassOrigin))
        return false;

    if (!_getPropertyList(in, propertyList))
        return false;

    return new CIMReferencesRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(),
        objectName, 
        resultClass, 
        role, 
        includeQualifiers, 
        includeClassOrigin, 
        propertyList, 
        QueueIdStack());
}

CIMReferenceNamesRequestMessage*
CIMBinMsgDeserializer::_getReferenceNamesRequestMessage(
    CIMBuffer& in)
{
    CIMObjectPath objectName;
    CIMName resultClass;
    String role;

    if (!_getObjectPath(in, objectName))
        return false;

    if (!_getName(in, resultClass))
        return false;

    if (!in.getString(role))
        return false;

    return new CIMReferenceNamesRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(), 
        objectName, 
        resultClass, 
        role, 
        QueueIdStack());
}

CIMGetPropertyRequestMessage*
CIMBinMsgDeserializer::_getGetPropertyRequestMessage(
    CIMBuffer& in)
{
    CIMObjectPath instanceName;
    CIMName propertyName;

    if (!_getObjectPath(in, instanceName))
        return false;

    if (!_getName(in, propertyName))
        return false;

    return new CIMGetPropertyRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(),
        instanceName, 
        propertyName, 
        QueueIdStack());
}

CIMSetPropertyRequestMessage*
CIMBinMsgDeserializer::_getSetPropertyRequestMessage(
    CIMBuffer& in)
{
    CIMObjectPath instanceName;
    CIMParamValue newValue;

    if (!_getObjectPath(in, instanceName))
        return false;

    if (!_getParamValue(in, newValue))
        return false;

    return new CIMSetPropertyRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(),
        instanceName, 
        newValue.getParameterName(), 
        newValue.getValue(),
        QueueIdStack());
}

CIMInvokeMethodRequestMessage*
CIMBinMsgDeserializer::_getInvokeMethodRequestMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMObjectPath instanceName;
    CIMName methodName;
    Array<CIMParamValue> inParameters;

    if (!_getObjectPath(in, instanceName))
        return false;

    if (!_getName(in, methodName))
        return false;

    if (!in.getParamValueA(inParameters))
        return false;

    return new CIMInvokeMethodRequestMessage(
        String::EMPTY, 
        CIMNamespaceName(),
        instanceName, 
        methodName, 
        inParameters, 
        QueueIdStack());
}

CIMCreateSubscriptionRequestMessage*
CIMBinMsgDeserializer::_getCreateSubscriptionRequestMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMNamespaceName nameSpace;
    CIMInstance subscriptionInstance;
    Array<CIMName> classNames;
    CIMPropertyList propertyList;
    Uint16 repeatNotificationPolicy;
    String query;

    if (!_getNamespaceName(in, nameSpace))
        return false;

    if (!_getInstance(in, subscriptionInstance))
        return false;

    if (!in.getNameA(classNames))
        return false;

    if (!_getPropertyList(in, propertyList))
        return false;

    if (!in.getUint16(repeatNotificationPolicy))
        return false;

    if (!in.getString(query))
        return false;

    return new CIMCreateSubscriptionRequestMessage(
        String::EMPTY, 
        nameSpace,
        subscriptionInstance, 
        classNames, 
        propertyList,
        repeatNotificationPolicy, 
        query, 
        QueueIdStack());
}

CIMModifySubscriptionRequestMessage*
CIMBinMsgDeserializer::_getModifySubscriptionRequestMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMNamespaceName nameSpace;
    CIMInstance subscriptionInstance;
    Array<CIMName> classNames;
    CIMPropertyList propertyList;
    Uint16 repeatNotificationPolicy;
    String query;

    if (!_getNamespaceName(in, nameSpace))
        return false;

    if (!_getInstance(in, subscriptionInstance))
        return false;

    if (!in.getNameA(classNames))
        return false;

    if (!_getPropertyList(in, propertyList))
        return false;

    if (!in.getUint16(repeatNotificationPolicy))
        return false;

    if (!in.getString(query))
        return false;

    return new CIMModifySubscriptionRequestMessage(
        String::EMPTY, 
        nameSpace,
        subscriptionInstance, 
        classNames, 
        propertyList,
        repeatNotificationPolicy, 
        query, 
        QueueIdStack());
}

CIMDeleteSubscriptionRequestMessage*
CIMBinMsgDeserializer::_getDeleteSubscriptionRequestMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMNamespaceName nameSpace;
    CIMInstance subscriptionInstance;
    Array<CIMName> classNames;

    if (!_getNamespaceName(in, nameSpace))
        return false;

    if (!_getInstance(in, subscriptionInstance))
        return false;

    if (!in.getNameA(classNames))
        return false;

    return new CIMDeleteSubscriptionRequestMessage(
        String::EMPTY,
        nameSpace, 
        subscriptionInstance, 
        classNames, 
        QueueIdStack());
}

CIMExportIndicationRequestMessage*
CIMBinMsgDeserializer::_getExportIndicationRequestMessage(
    CIMBuffer& in)
{
    String authType;
    String userName;
    String destinationPath;
    CIMInstance indicationInstance;

    if (!_getUserInfo(in, authType, userName))
        return false;

    if (!in.getString(destinationPath))
        return false;

    if (!_getInstance(in, indicationInstance))
        return false;

    return new CIMExportIndicationRequestMessage(
        String::EMPTY, 
        destinationPath,
        indicationInstance, 
        QueueIdStack(), 
        authType,
        userName);
}

CIMProcessIndicationRequestMessage*
CIMBinMsgDeserializer::_getProcessIndicationRequestMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMNamespaceName nameSpace;
    CIMInstance indicationInstance;
    Array<CIMObjectPath> subscriptionInstanceNames;
    CIMInstance provider;

    if (!_getNamespaceName(in, nameSpace))
        return false;

    if (!_getInstance(in, indicationInstance))
        return false;

    if (!in.getObjectPathA(subscriptionInstanceNames))
        return false;

    if (!_getInstance(in, provider))
        return false;

    return new CIMProcessIndicationRequestMessage(
        String::EMPTY, 
        nameSpace,
        indicationInstance, 
        subscriptionInstanceNames, 
        provider,
        QueueIdStack());
}

CIMDisableModuleRequestMessage*
CIMBinMsgDeserializer::_getDisableModuleRequestMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    String authType;
    String userName;
    CIMInstance providerModule;
    Array<CIMInstance> providers;
    Boolean disableProviderOnly;
    Array<Boolean> indicationProviders;

    if (!_getUserInfo(in, authType, userName))
        return false;

    if (!_getInstance(in, providerModule))
        return false;

    if (!in.getInstanceA(providers))
        return false;

    if (!in.getBoolean(disableProviderOnly))
        return false;

    if (!in.getBooleanA(indicationProviders))
        return false;

    return new CIMDisableModuleRequestMessage(
        String::EMPTY,
        providerModule,
        providers,
        disableProviderOnly,
        indicationProviders,
        QueueIdStack(),
        authType,
        userName);
}

CIMEnableModuleRequestMessage*
CIMBinMsgDeserializer::_getEnableModuleRequestMessage(
    CIMBuffer& in)
{
    String authType;
    String userName;
    CIMInstance providerModule;

    if (!_getUserInfo(in, authType, userName))
        return false;

    if (!_getInstance(in, providerModule))
        return false;

    return new CIMEnableModuleRequestMessage(
        String::EMPTY,
        providerModule,
        QueueIdStack(),
        authType,
        userName);
}

CIMStopAllProvidersRequestMessage*
CIMBinMsgDeserializer::_getStopAllProvidersRequestMessage(
    CIMBuffer& in)
{
    return new CIMStopAllProvidersRequestMessage(
        String::EMPTY,
        QueueIdStack());
}

CIMInitializeProviderAgentRequestMessage*
CIMBinMsgDeserializer::_getInitializeProviderAgentRequestMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    String pegasusHome;
    typedef Pair<String,String> ConfigPair;
    Array<ConfigPair> configProperties;
    Uint32 size;
    Boolean bindVerbose;
    Boolean subscriptionInitComplete;

    if (!in.getString(pegasusHome))
        return false;

    if (!in.getUint32(size))
        return false;

    for (Uint32 i = 0; i < size; i++)
    {
        String first;
        String second;

        if (!in.getString(first) || !in.getString(second))
            return false;

        configProperties.append(ConfigPair(first, second));
    }

    if (!in.getBoolean(bindVerbose))
        return false;

    if (!in.getBoolean(subscriptionInitComplete))
        return false;

    return new CIMInitializeProviderAgentRequestMessage(
        String::EMPTY,
        pegasusHome,
        configProperties,
        bindVerbose,
        subscriptionInitComplete,
        QueueIdStack());
}

CIMNotifyConfigChangeRequestMessage*
CIMBinMsgDeserializer::_getNotifyConfigChangeRequestMessage(
    CIMBuffer& in)
{
    String propertyName;
    String newPropertyValue;
    Boolean currentValueModified;

    if (!in.getString(propertyName))
        return false;

    if (!in.getString(newPropertyValue))
        return false;

    if (!in.getBoolean(currentValueModified))
        return false;

    return new CIMNotifyConfigChangeRequestMessage(
        String::EMPTY,
        propertyName,
        newPropertyValue,
        currentValueModified,
        QueueIdStack());
}

CIMSubscriptionInitCompleteRequestMessage*
CIMBinMsgDeserializer::_getSubscriptionInitCompleteRequestMessage(
    CIMBuffer& in)
{
    return new CIMSubscriptionInitCompleteRequestMessage(
        String::EMPTY,
        QueueIdStack());
}

static Boolean _resolveXMLInstance(
    CIMGetInstanceResponseMessage* msg,
    CIMInstance& cimInstance)
{
    // Deserialize instance:
    {
        XmlParser parser((char*)msg->instanceData.getData());

        if (!XmlReader::getInstanceElement(parser, cimInstance))
        {
            cimInstance = CIMInstance();
            return false;
        }
    }

    // Deserialize path:
    {
        XmlParser parser((char*)msg->referenceData.getData());
        CIMObjectPath cimObjectPath;

        if (XmlReader::getValueReferenceElement(parser, cimObjectPath))
        {
            if (msg->hostData.size())
                cimObjectPath.setHost(msg->hostData);

            if (!msg->nameSpaceData.isNull())
                cimObjectPath.setNameSpace(msg->nameSpaceData);

            cimInstance.setPath(cimObjectPath);
        }
    }

    return true;
}

static Boolean _resolveXMLInstances(
    CIMEnumerateInstancesResponseMessage* msg,
    Array<CIMInstance>& instances)
{
    instances.clear();

    for (Uint32 i = 0; i < msg->instancesData.size(); i++)
    {
        CIMInstance cimInstance;

        // Deserialize instance:
        {
            XmlParser parser((char*)msg->instancesData[i].getData());

            if (!XmlReader::getInstanceElement(parser, cimInstance))
            {
                cimInstance = CIMInstance();
            }
        }

        // Deserialize path:
        {
            XmlParser parser((char*)msg->referencesData[i].getData());
            CIMObjectPath cimObjectPath;

            if (XmlReader::getInstanceNameElement(parser, cimObjectPath))
            {
                if (!msg->nameSpacesData[i].isNull())
                    cimObjectPath.setNameSpace(msg->nameSpacesData[i]);

                if (msg->hostsData[i].size())
                    cimObjectPath.setHost(msg->hostsData[i]);

                cimInstance.setPath(cimObjectPath);
            }
        }

        instances.append(cimInstance);
    }

    return true;
}

static Boolean _resolveBinaryInstance(
    CIMGetInstanceResponseMessage* msg,
    CIMInstance& instance)
{
    CIMBuffer in((char*)msg->binaryData.getData(), msg->binaryData.size());

    if (!in.getInstance(instance))
    {
        instance = CIMInstance();
        in.release();
        return false;
    }

    in.release();
    return true;
}

static Boolean _resolveBinaryInstances(
    CIMEnumerateInstancesResponseMessage* msg,
    Array<CIMInstance>& instances)
{
    instances.clear();

    CIMBuffer in((char*)msg->binaryData.getData(), msg->binaryData.size());

    if (!in.getInstanceA(instances))
    {
        in.release();
        return false;
    }

    in.release();
    return true;
}

CIMGetInstanceResponseMessage*
CIMBinMsgDeserializer::_getGetInstanceResponseMessage(
    CIMBuffer& in,
    bool binaryResponse)
{
    if (binaryResponse)
    {
        CIMGetInstanceResponseMessage* msg = new CIMGetInstanceResponseMessage(
            String::EMPTY,
            CIMException(),
            QueueIdStack());

        if (!in.getUint8A(msg->binaryData))
            return 0;

        msg->resolveCallback = _resolveBinaryInstance;
        msg->binaryEncoding = true;

        return msg;
    }
    else
    {
        Array<Sint8> instanceData;
        Array<Sint8> referenceData;
        String hostData;
        CIMNamespaceName nameSpaceData;

        if (!in.getSint8A(instanceData))
            return NULL;

        if (!in.getSint8A(referenceData))
            return NULL;

        if (!in.getString(hostData))
            return NULL;

        if (!in.getNamespaceName(nameSpaceData))
            return NULL;

        CIMGetInstanceResponseMessage* msg = new CIMGetInstanceResponseMessage(
            String::EMPTY,
            CIMException(),
            QueueIdStack());

        msg->resolveCallback = _resolveXMLInstance;
        msg->instanceData = instanceData;
        msg->referenceData = referenceData;
        msg->hostData = hostData;
        msg->nameSpaceData = nameSpaceData;

        return msg;
    }
}

CIMDeleteInstanceResponseMessage*
CIMBinMsgDeserializer::_getDeleteInstanceResponseMessage(
    CIMBuffer& in)
{
    return new CIMDeleteInstanceResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMCreateInstanceResponseMessage*
CIMBinMsgDeserializer::_getCreateInstanceResponseMessage(
    CIMBuffer& in)
{
    CIMObjectPath instanceName;

    if (!_getObjectPath(in, instanceName))
        return false;

    return new CIMCreateInstanceResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        instanceName);
}

CIMModifyInstanceResponseMessage*
CIMBinMsgDeserializer::_getModifyInstanceResponseMessage(
    CIMBuffer& in)
{
    return new CIMModifyInstanceResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMEnumerateInstancesResponseMessage*
CIMBinMsgDeserializer::_getEnumerateInstancesResponseMessage(
    CIMBuffer& in,
    bool binaryResponse)
{
    if (binaryResponse)
    {
        // Inject data into message so that the instances can be deserialized 
        // on demand later (hopefully never; hopefully it can be written out on
        // the wire intact).

        CIMEnumerateInstancesResponseMessage* msg;

        msg = new CIMEnumerateInstancesResponseMessage(String::EMPTY,
            CIMException(), QueueIdStack());

        if (!in.getUint8A(msg->binaryData))
            return 0;

        msg->resolveCallback = _resolveBinaryInstances;
        msg->binaryEncoding = true;

        return msg;
    }
    else
    {
        Uint32 count;

        if (!in.getUint32(count))
            return 0;

        Array<ArraySint8> instancesData;
        Array<ArraySint8> referencesData;
        Array<String> hostsData;
        Array<CIMNamespaceName> nameSpacesData;

        for (Uint32 i = 0; i < count; i++)
        {
            Array<Sint8> inst;
            Array<Sint8> ref;
            CIMNamespaceName ns;
            String host;

            if (!in.getSint8A(inst))
                return 0;

            if (!in.getSint8A(ref))
                return 0;

            if (!in.getString(host))
                return 0;

            if (!in.getNamespaceName(ns))
                return 0;

            instancesData.append(inst);
            referencesData.append(ref);
            hostsData.append(host);
            nameSpacesData.append(ns);
        }

        CIMEnumerateInstancesResponseMessage* msg;
        
        msg = new CIMEnumerateInstancesResponseMessage(
            String::EMPTY,
            CIMException(),
            QueueIdStack());

        msg->resolveCallback = _resolveXMLInstances;
        msg->instancesData = instancesData;
        msg->referencesData = referencesData;
        msg->hostsData = hostsData;
        msg->nameSpacesData = nameSpacesData;

        return msg;
    }
}

CIMEnumerateInstanceNamesResponseMessage*
CIMBinMsgDeserializer::_getEnumerateInstanceNamesResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    Array<CIMObjectPath> instanceNames;

    if (!in.getObjectPathA(instanceNames))
        return false;

    return new CIMEnumerateInstanceNamesResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        instanceNames);
}

CIMExecQueryResponseMessage*
CIMBinMsgDeserializer::_getExecQueryResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    Array<CIMObject> cimObjects;

    if (!in.getObjectA(cimObjects))
        return false;

    return new CIMExecQueryResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        cimObjects);
}

CIMAssociatorsResponseMessage*
CIMBinMsgDeserializer::_getAssociatorsResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    Array<CIMObject> cimObjects;

    if (!in.getObjectA(cimObjects))
        return false;

    return new CIMAssociatorsResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        cimObjects);
}

CIMAssociatorNamesResponseMessage*
CIMBinMsgDeserializer::_getAssociatorNamesResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    Array<CIMObjectPath> objectNames;

    if (!in.getObjectPathA(objectNames))
        return false;

    return new CIMAssociatorNamesResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        objectNames);
}

CIMReferencesResponseMessage*
CIMBinMsgDeserializer::_getReferencesResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    Array<CIMObject> cimObjects;

    if (!in.getObjectA(cimObjects))
        return false;

    return new CIMReferencesResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        cimObjects);
}

CIMReferenceNamesResponseMessage*
CIMBinMsgDeserializer::_getReferenceNamesResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    Array<CIMObjectPath> objectNames;

    if (!in.getObjectPathA(objectNames))
        return false;

    return new CIMReferenceNamesResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        objectNames);
}

CIMGetPropertyResponseMessage*
CIMBinMsgDeserializer::_getGetPropertyResponseMessage(
    CIMBuffer& in)
{
    CIMParamValue value;

    if (!_getParamValue(in, value))
        return false;

    return new CIMGetPropertyResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        value.getValue());
}

CIMSetPropertyResponseMessage*
CIMBinMsgDeserializer::_getSetPropertyResponseMessage(
    CIMBuffer& in)
{
    return new CIMSetPropertyResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMInvokeMethodResponseMessage*
CIMBinMsgDeserializer::_getInvokeMethodResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMParamValue genericParamValue;
    CIMParamValue retValue;
    CIMName methodName;
    Array<CIMParamValue> outParameters;

    if (!_getParamValue(in, retValue))
        return false;

    if (!in.getParamValueA(outParameters))
        return false;

    if (!_getName(in, methodName))
        return false;

    return new CIMInvokeMethodResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        retValue.getValue(),
        outParameters,
        methodName);
}

CIMCreateSubscriptionResponseMessage*
CIMBinMsgDeserializer::_getCreateSubscriptionResponseMessage(
    CIMBuffer& in)
{
    return new CIMCreateSubscriptionResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMModifySubscriptionResponseMessage*
CIMBinMsgDeserializer::_getModifySubscriptionResponseMessage(
    CIMBuffer& in)
{
    return new CIMModifySubscriptionResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMDeleteSubscriptionResponseMessage*
CIMBinMsgDeserializer::_getDeleteSubscriptionResponseMessage(
    CIMBuffer& in)
{
    return new CIMDeleteSubscriptionResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMExportIndicationResponseMessage*
CIMBinMsgDeserializer::_getExportIndicationResponseMessage(
    CIMBuffer& in)
{
    return new CIMExportIndicationResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMProcessIndicationResponseMessage*
CIMBinMsgDeserializer::_getProcessIndicationResponseMessage(
    CIMBuffer& in)
{
    return new CIMProcessIndicationResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMDisableModuleResponseMessage*
CIMBinMsgDeserializer::_getDisableModuleResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMValue genericValue;
    Uint16 genericUint16;
    Array<Uint16> operationalStatus;

    if (!in.getUint16A(operationalStatus))
        return false;

    return new CIMDisableModuleResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        operationalStatus);
}

CIMEnableModuleResponseMessage*
CIMBinMsgDeserializer::_getEnableModuleResponseMessage(
    CIMBuffer& in)
{
    XmlEntry entry;
    CIMValue genericValue;
    Uint16 genericUint16;
    Array<Uint16> operationalStatus;

    if (!in.getUint16A(operationalStatus))
        return false;

    return new CIMEnableModuleResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack(),
        operationalStatus);
}

CIMStopAllProvidersResponseMessage*
CIMBinMsgDeserializer::_getStopAllProvidersResponseMessage(
    CIMBuffer& in)
{
    return new CIMStopAllProvidersResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMInitializeProviderAgentResponseMessage*
CIMBinMsgDeserializer::_getInitializeProviderAgentResponseMessage(
    CIMBuffer& in)
{
    return new CIMInitializeProviderAgentResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMNotifyConfigChangeResponseMessage*
CIMBinMsgDeserializer::_getNotifyConfigChangeResponseMessage(
    CIMBuffer& in)
{
    return new CIMNotifyConfigChangeResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

CIMSubscriptionInitCompleteResponseMessage*
CIMBinMsgDeserializer::_getSubscriptionInitCompleteResponseMessage(
    CIMBuffer& in)
{
    return new CIMSubscriptionInitCompleteResponseMessage(
        String::EMPTY,
        CIMException(),
        QueueIdStack());
}

PEGASUS_NAMESPACE_END
