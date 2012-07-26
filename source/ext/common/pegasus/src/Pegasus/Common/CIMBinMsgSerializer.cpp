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

#include <Pegasus/Common/OperationContextInternal.h>
#include "CIMBinMsgSerializer.h"

PEGASUS_NAMESPACE_BEGIN

void _putXMLInstance(
    CIMBuffer& out,
    const CIMInstance& ci)
{
    if (ci.isUninitialized())
    {
        out.putUint32(0);
        out.putUint32(0);
        out.putString(String());
        out.putNamespaceName(CIMNamespaceName());
    }
    else
    {
        Buffer buf(4096);

        // Serialize instance as XML.
        {
            XmlWriter::appendInstanceElement(buf, ci);
            buf.append('\0');

            out.putUint32(buf.size());
            out.putBytes(buf.getData(), buf.size());
            buf.clear();
        }

        // Serialize object path as XML.

        const CIMObjectPath& cop = ci.getPath();

        if (cop.getClassName().isNull())
        {
            out.putUint32(0);
            out.putString(String());
            out.putNamespaceName(CIMNamespaceName());
        }
        else
        {
            XmlWriter::appendValueReferenceElement(buf, cop, true);
            buf.append('\0');

            out.putUint32(buf.size());
            out.putBytes(buf.getData(), buf.size());
            out.putString(cop.getHost());
            out.putNamespaceName(cop.getNameSpace());
        }
    }
}

void _putXMLNamedInstance(
    CIMBuffer& out,
    const CIMInstance& ci)
{
    if (ci.isUninitialized())
    {
        out.putUint32(0);
        out.putUint32(0);
        out.putString(String());
        out.putNamespaceName(CIMNamespaceName());
    }
    else
    {
        Buffer buf(4096);

        // Serialize instance as XML.
        {
            XmlWriter::appendInstanceElement(buf, ci);
            buf.append('\0');

            out.putUint32(buf.size());
            out.putBytes(buf.getData(), buf.size());
            buf.clear();
        }

        // Serialize object path as XML.

        const CIMObjectPath& cop = ci.getPath();

        if (cop.getClassName().isNull())
        {
            out.putUint32(0);
            out.putString(String());
            out.putNamespaceName(CIMNamespaceName());
        }
        else
        {
            XmlWriter::appendInstanceNameElement(buf, cop);
            buf.append('\0');

            out.putUint32(buf.size());
            out.putBytes(buf.getData(), buf.size());
            out.putString(cop.getHost());
            out.putNamespaceName(cop.getNameSpace());
        }
    }
}

void CIMBinMsgSerializer::serialize(
    CIMBuffer& out, 
    CIMMessage* cimMessage)
{
    if (cimMessage == 0)
        return;

    // [messageId]
    out.putString(cimMessage->messageId);

    // [binaryRequest]
    out.putBoolean(cimMessage->binaryRequest);

    // [binaryResponse]
    out.putBoolean(cimMessage->binaryResponse);

    // [type]
    out.putUint32(Uint32(cimMessage->getType()));

#ifndef PEGASUS_DISABLE_PERFINST
    // [serverStartTimeMicroseconds]
    out.putUint64(cimMessage->getServerStartTime());
    // [providerTimeMicroseconds]
    out.putUint64(cimMessage->getProviderTime());
#endif

    // [isComplete]
    out.putBoolean(cimMessage->isComplete());

    // [index]
    out.putUint32(cimMessage->getIndex());

    // [operationContext]

    _serializeOperationContext(out, cimMessage->operationContext);

    // [CIMRequestMessage]

    CIMRequestMessage* req;

    if ((req = dynamic_cast<CIMRequestMessage*>(cimMessage)))
    {
        out.putPresent(true);
        _putRequestMessage(out, req);
    }
    else
        out.putPresent(false);

    // [CIMResponseMessage]

    CIMResponseMessage* rsp;

    if ((rsp = dynamic_cast<CIMResponseMessage*>(cimMessage)))
    {
        out.putPresent(true);
        _putResponseMessage(out, rsp);
    }
    else
        out.putPresent(false);
}

void CIMBinMsgSerializer::_putRequestMessage(
    CIMBuffer& out,
    CIMRequestMessage* msg)
{
    PEGASUS_ASSERT(msg != 0);

    // [queueIdStack]

    _serializeQueueIdStack(out, msg->queueIds);

    // [CIMOperationRequestMessage]

    CIMOperationRequestMessage* oreq;

    if ((oreq = dynamic_cast<CIMOperationRequestMessage*>(msg)))
    {
        out.putPresent(true);

        // [userInfo]
        _serializeUserInfo(out, oreq->authType, oreq->userName);

        // [nameSpace]
        _putNamespaceName(out, oreq->nameSpace);

        // [className]
        _putName(out, oreq->className);

        // [providerType]
        out.putUint32(oreq->providerType);

        // [message]

        switch (msg->getType())
        {
            case CIM_GET_INSTANCE_REQUEST_MESSAGE:
                _putGetInstanceRequestMessage(
                    out, (CIMGetInstanceRequestMessage*)msg);
                break;
            case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
                _putDeleteInstanceRequestMessage(
                    out, (CIMDeleteInstanceRequestMessage*)msg);
                break;
            case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
                _putCreateInstanceRequestMessage(
                    out, (CIMCreateInstanceRequestMessage*)msg);
                break;
            case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
                _putModifyInstanceRequestMessage(
                    out, (CIMModifyInstanceRequestMessage*)msg);
                break;
            case CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE:
                _putEnumerateInstancesRequestMessage(
                    out, (CIMEnumerateInstancesRequestMessage*)msg);
                break;
            case CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE:
                _putEnumerateInstanceNamesRequestMessage(
                    out, (CIMEnumerateInstanceNamesRequestMessage*)msg);
                break;
            case CIM_EXEC_QUERY_REQUEST_MESSAGE:
                _putExecQueryRequestMessage(
                    out, (CIMExecQueryRequestMessage*)msg);
                break;
            case CIM_GET_PROPERTY_REQUEST_MESSAGE:
                _putGetPropertyRequestMessage(
                    out, (CIMGetPropertyRequestMessage*)msg);
                break;
            case CIM_SET_PROPERTY_REQUEST_MESSAGE:
                _putSetPropertyRequestMessage(
                    out, (CIMSetPropertyRequestMessage*)msg);
                break;
            case CIM_ASSOCIATORS_REQUEST_MESSAGE:
                _putAssociatorsRequestMessage(
                    out, (CIMAssociatorsRequestMessage*)msg);
                break;
            case CIM_ASSOCIATOR_NAMES_REQUEST_MESSAGE:
                _putAssociatorNamesRequestMessage(
                    out, (CIMAssociatorNamesRequestMessage*)msg);
                break;
            case CIM_REFERENCES_REQUEST_MESSAGE:
                _putReferencesRequestMessage(
                    out, (CIMReferencesRequestMessage*)msg);
                break;
            case CIM_REFERENCE_NAMES_REQUEST_MESSAGE:
                _putReferenceNamesRequestMessage(
                    out, (CIMReferenceNamesRequestMessage*)msg);
                break;
            case CIM_INVOKE_METHOD_REQUEST_MESSAGE:
                _putInvokeMethodRequestMessage(
                    out, (CIMInvokeMethodRequestMessage*)msg);
                break;

            default:
                PEGASUS_ASSERT(0);
        }
    }
    else
        out.putPresent(false);

    // [CIMIndicationRequestMessage]

    CIMIndicationRequestMessage* ireq;

    if ((ireq = dynamic_cast<CIMIndicationRequestMessage*>(msg)))
    {
        out.putPresent(true);

        // [userInfo]
        _serializeUserInfo(out, ireq->authType, ireq->userName);

        // [message]

        switch (msg->getType())
        {
            case CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE:
                _putCreateSubscriptionRequestMessage(
                    out, (CIMCreateSubscriptionRequestMessage*)msg);
                break;
            case CIM_MODIFY_SUBSCRIPTION_REQUEST_MESSAGE:
                _putModifySubscriptionRequestMessage(
                    out, (CIMModifySubscriptionRequestMessage*)msg);
                break;
            case CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE:
                _putDeleteSubscriptionRequestMessage(
                    out, (CIMDeleteSubscriptionRequestMessage*)msg);
                break;
            default:
                PEGASUS_ASSERT(0);
        }
    }
    else
        out.putPresent(false);

    // [Other]

    if (!oreq && !ireq)
    {
        out.putPresent(true);

        switch (msg->getType())
        {
        case CIM_EXPORT_INDICATION_REQUEST_MESSAGE:
            _putExportIndicationRequestMessage(
                out, (CIMExportIndicationRequestMessage*)msg);
            break;
        case CIM_PROCESS_INDICATION_REQUEST_MESSAGE:
            _putProcessIndicationRequestMessage(
                out, (CIMProcessIndicationRequestMessage*)msg);
            break;
        case CIM_DISABLE_MODULE_REQUEST_MESSAGE:
            _putDisableModuleRequestMessage(
                out, (CIMDisableModuleRequestMessage*)msg);
            break;
        case CIM_ENABLE_MODULE_REQUEST_MESSAGE:
            _putEnableModuleRequestMessage(
                out, (CIMEnableModuleRequestMessage*)msg);
            break;
        case CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE:
            _putStopAllProvidersRequestMessage(
                out, (CIMStopAllProvidersRequestMessage*)msg);
            break;
        case CIM_INITIALIZE_PROVIDER_AGENT_REQUEST_MESSAGE:
            _putInitializeProviderAgentRequestMessage(
                out, (CIMInitializeProviderAgentRequestMessage*)msg);
            break;

        case CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE:
            _putNotifyConfigChangeRequestMessage(
                out, (CIMNotifyConfigChangeRequestMessage*)msg);
            break;

        case CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE:
            _putSubscriptionInitCompleteRequestMessage(
                out, (CIMSubscriptionInitCompleteRequestMessage*)msg);
            break;

        default:
            PEGASUS_ASSERT(0);
        }
    }
    else
        out.putPresent(false);
}

void CIMBinMsgSerializer::_putResponseMessage(
    CIMBuffer& out,
    CIMResponseMessage* cimMessage)
{
    PEGASUS_ASSERT(cimMessage != 0);

    // [queueIdStack]
    _serializeQueueIdStack(out, cimMessage->queueIds);

    // [cimException]
    _putException(out, cimMessage->cimException);

    // [message]

    switch (cimMessage->getType())
    {
        case CIM_GET_INSTANCE_RESPONSE_MESSAGE:
            _putGetInstanceResponseMessage(
                out, (CIMGetInstanceResponseMessage*)cimMessage);
            break;
        case CIM_DELETE_INSTANCE_RESPONSE_MESSAGE:
            _putDeleteInstanceResponseMessage(
                out, (CIMDeleteInstanceResponseMessage*)cimMessage);
            break;
        case CIM_CREATE_INSTANCE_RESPONSE_MESSAGE:
            _putCreateInstanceResponseMessage(
                out, (CIMCreateInstanceResponseMessage*)cimMessage);
            break;
        case CIM_MODIFY_INSTANCE_RESPONSE_MESSAGE:
            _putModifyInstanceResponseMessage(
                out, (CIMModifyInstanceResponseMessage*)cimMessage);
            break;
        case CIM_ENUMERATE_INSTANCES_RESPONSE_MESSAGE:
            _putEnumerateInstancesResponseMessage(
                out, (CIMEnumerateInstancesResponseMessage*)cimMessage);
            break;
        case CIM_ENUMERATE_INSTANCE_NAMES_RESPONSE_MESSAGE:
            _putEnumerateInstanceNamesResponseMessage(
                out, (CIMEnumerateInstanceNamesResponseMessage*)cimMessage);
            break;
        case CIM_EXEC_QUERY_RESPONSE_MESSAGE:
            _putExecQueryResponseMessage(
                out, (CIMExecQueryResponseMessage*)cimMessage);
            break;
        case CIM_GET_PROPERTY_RESPONSE_MESSAGE:
            _putGetPropertyResponseMessage(
                out, (CIMGetPropertyResponseMessage*)cimMessage);
            break;
        case CIM_SET_PROPERTY_RESPONSE_MESSAGE:
            _putSetPropertyResponseMessage(
                out, (CIMSetPropertyResponseMessage*)cimMessage);
            break;
        case CIM_ASSOCIATORS_RESPONSE_MESSAGE:
            _putAssociatorsResponseMessage(
                out, (CIMAssociatorsResponseMessage*)cimMessage);
            break;
        case CIM_ASSOCIATOR_NAMES_RESPONSE_MESSAGE:
            _putAssociatorNamesResponseMessage(
                out, (CIMAssociatorNamesResponseMessage*)cimMessage);
            break;
        case CIM_REFERENCES_RESPONSE_MESSAGE:
            _putReferencesResponseMessage(
                out, (CIMReferencesResponseMessage*)cimMessage);
            break;
        case CIM_REFERENCE_NAMES_RESPONSE_MESSAGE:
            _putReferenceNamesResponseMessage(
                out, (CIMReferenceNamesResponseMessage*)cimMessage);
            break;
        case CIM_INVOKE_METHOD_RESPONSE_MESSAGE:
            _putInvokeMethodResponseMessage(
                out, (CIMInvokeMethodResponseMessage*)cimMessage);
            break;
        case CIM_CREATE_SUBSCRIPTION_RESPONSE_MESSAGE:
            _putCreateSubscriptionResponseMessage(
                out, (CIMCreateSubscriptionResponseMessage*)cimMessage);
            break;
        case CIM_MODIFY_SUBSCRIPTION_RESPONSE_MESSAGE:
            _putModifySubscriptionResponseMessage(
                out, (CIMModifySubscriptionResponseMessage*)cimMessage);
            break;
        case CIM_DELETE_SUBSCRIPTION_RESPONSE_MESSAGE:
            _putDeleteSubscriptionResponseMessage(
                out, (CIMDeleteSubscriptionResponseMessage*)cimMessage);
            break;
        case CIM_EXPORT_INDICATION_RESPONSE_MESSAGE:
            _putExportIndicationResponseMessage(
                out, (CIMExportIndicationResponseMessage*)cimMessage);
            break;
        case CIM_PROCESS_INDICATION_RESPONSE_MESSAGE:
            _putProcessIndicationResponseMessage(
                out, (CIMProcessIndicationResponseMessage*)cimMessage);
            break;
        case CIM_DISABLE_MODULE_RESPONSE_MESSAGE:
            _putDisableModuleResponseMessage(
                out, (CIMDisableModuleResponseMessage*)cimMessage);
            break;
        case CIM_ENABLE_MODULE_RESPONSE_MESSAGE:
            _putEnableModuleResponseMessage(
                out, (CIMEnableModuleResponseMessage*)cimMessage);
            break;
        case CIM_STOP_ALL_PROVIDERS_RESPONSE_MESSAGE:
            _putStopAllProvidersResponseMessage(
                out, (CIMStopAllProvidersResponseMessage*)cimMessage);
            break;
        case CIM_INITIALIZE_PROVIDER_AGENT_RESPONSE_MESSAGE:
            _putInitializeProviderAgentResponseMessage(
                out, (CIMInitializeProviderAgentResponseMessage*)cimMessage);
            break;
        case CIM_NOTIFY_CONFIG_CHANGE_RESPONSE_MESSAGE:
            _putNotifyConfigChangeResponseMessage(
                out, (CIMNotifyConfigChangeResponseMessage*)cimMessage);
            break;
        case CIM_SUBSCRIPTION_INIT_COMPLETE_RESPONSE_MESSAGE:
            _putSubscriptionInitCompleteResponseMessage(
                out,
                (CIMSubscriptionInitCompleteResponseMessage *)
                cimMessage);
            break;

        default:
            PEGASUS_ASSERT(0);
    }
}

void CIMBinMsgSerializer::_serializeUserInfo(
    CIMBuffer& out,
    const String& authType,
    const String& userName)
{
    out.putString(authType);
    out.putString(userName);
}

void CIMBinMsgSerializer::_serializeQueueIdStack(
    CIMBuffer& out,
    const QueueIdStack& stack)
{
    out.putUint32(stack.size());

    for (Uint32 i = 0; i < stack.size(); i++)
        out.putUint32(stack[i]);
}

void CIMBinMsgSerializer::_serializeOperationContext(
    CIMBuffer& out,
    const OperationContext& operationContext)
{
    // [IdentityContainer]

    if (operationContext.contains(IdentityContainer::NAME))
    {
        out.putPresent(true);

        const IdentityContainer container =
            operationContext.get(IdentityContainer::NAME);

        out.putString(container.getUserName());
    }
    else
        out.putPresent(false);

    // [SubscriptionInstanceContainer]

    if (operationContext.contains(SubscriptionInstanceContainer::NAME))
    {
        out.putPresent(true);

        const SubscriptionInstanceContainer container =
            operationContext.get(SubscriptionInstanceContainer::NAME);

        _putInstance(out, container.getInstance());
    }
    else
        out.putPresent(false);

    // [SubscriptionFilterConditionContainer]

    if (operationContext.contains(SubscriptionFilterConditionContainer::NAME))
    {
        out.putPresent(true);

        const SubscriptionFilterConditionContainer container =
            operationContext.get(SubscriptionFilterConditionContainer::NAME);

        out.putString(container.getFilterCondition());
        out.putString(container.getQueryLanguage());
    }
    else
        out.putPresent(false);

    // [SubscriptionFilterQueryContainer]

    if (operationContext.contains(SubscriptionFilterQueryContainer::NAME))
    {
        out.putPresent(true);

        const SubscriptionFilterQueryContainer container =
            operationContext.get(SubscriptionFilterQueryContainer::NAME);

        out.putString(container.getFilterQuery());
        out.putString(container.getQueryLanguage());
        out.putString(container.getSourceNameSpace().getString());
    }
    else
        out.putPresent(false);

    // [SubscriptionInstanceNamesContainer]

    if (operationContext.contains(SubscriptionInstanceNamesContainer::NAME))
    {
        out.putPresent(true);

        const SubscriptionInstanceNamesContainer container =
            operationContext.get(SubscriptionInstanceNamesContainer::NAME);

        out.putObjectPathA(container.getInstanceNames());
    }
    else
        out.putPresent(false);

    // [TimeoutContainer]

    if (operationContext.contains(TimeoutContainer::NAME))
    {
        out.putPresent(true);

        const TimeoutContainer container =
            operationContext.get(TimeoutContainer::NAME);

        out.putUint32(container.getTimeOut());
    }
    else
        out.putPresent(false);

    // [AcceptLanguageListContainer]

    if (operationContext.contains(AcceptLanguageListContainer::NAME))
    {
        out.putPresent(true);

        const AcceptLanguageListContainer container =
            operationContext.get(AcceptLanguageListContainer::NAME);

        _serializeAcceptLanguageList(out, container.getLanguages());
    }
    else
        out.putPresent(false);

    // [ContentLanguageListContainer]

    if (operationContext.contains(ContentLanguageListContainer::NAME))
    {
        out.putPresent(true);

        const ContentLanguageListContainer container =
            operationContext.get(ContentLanguageListContainer::NAME);

        _serializeContentLanguageList(out, container.getLanguages());
    }
    else
        out.putPresent(false);

    // [SnmpTrapOidContainer]

    if (operationContext.contains(SnmpTrapOidContainer::NAME))
    {
        out.putPresent(true);

        const SnmpTrapOidContainer container =
            operationContext.get(SnmpTrapOidContainer::NAME);

        out.putString(container.getSnmpTrapOid());
    }
    else
        out.putPresent(false);

    // [LocaleContainer]

    if (operationContext.contains(LocaleContainer::NAME))
    {
        out.putPresent(true);

        const LocaleContainer container =
            operationContext.get(LocaleContainer::NAME);

        out.putString(container.getLanguageId());
    }
    else
        out.putPresent(false);

    // [ProviderIdContainer]

    if (operationContext.contains(ProviderIdContainer::NAME))
    {
        out.putPresent(true);

        const ProviderIdContainer container =
            operationContext.get(ProviderIdContainer::NAME);

        _putInstance(out, container.getModule());
        _putInstance(out, container.getProvider());
        out.putBoolean(container.isRemoteNameSpace());
        out.putString(container.getRemoteInfo());
        out.putString(container.getProvMgrPath());
    }
    else
        out.putPresent(false);

    // [CachedClassDefinitionContainer]

    if (operationContext.contains(CachedClassDefinitionContainer::NAME))
    {
        out.putPresent(true);

        const CachedClassDefinitionContainer container =
            operationContext.get(CachedClassDefinitionContainer::NAME);

        CIMConstClass ccc = container.getClass();
        out.putClass(*((CIMClass*)(void*)&ccc));
    }
    else
        out.putPresent(false);
}

void CIMBinMsgSerializer::_serializeContentLanguageList(
    CIMBuffer& out,
    const ContentLanguageList& contentLanguages)
{
    out.putUint32(contentLanguages.size());

    for (Uint32 i = 0; i < contentLanguages.size(); i++)
        out.putString(contentLanguages.getLanguageTag(i).toString());
}

void CIMBinMsgSerializer::_serializeAcceptLanguageList(
    CIMBuffer& out,
    const AcceptLanguageList& acceptLanguages)
{
    out.putUint32(acceptLanguages.size());

    for (Uint32 i = 0; i < acceptLanguages.size(); i++)
    {
        out.putString(acceptLanguages.getLanguageTag(i).toString());
        out.putReal32(acceptLanguages.getQualityValue(i));
    }
}

void CIMBinMsgSerializer::_putException(
    CIMBuffer& out,
    const CIMException& cimException)
{
    TraceableCIMException e(cimException);

    out.putUint32(Uint32(e.getCode()));
    out.putString(e.getMessage());
    out.putString(e.getCIMMessage());
    out.putString(e.getFile());
    out.putUint32(e.getLine());
    _serializeContentLanguageList(out, e.getContentLanguages());
}

void CIMBinMsgSerializer::_putPropertyList(
    CIMBuffer& out,
    const CIMPropertyList& cimPropertyList)
{
    out.putPropertyList(cimPropertyList);
}

void CIMBinMsgSerializer::_putObjectPath(
    CIMBuffer& out,
    const CIMObjectPath& cimObjectPath)
{
    out.putObjectPath(cimObjectPath);
}

void CIMBinMsgSerializer::_putInstance(
    CIMBuffer& out,
    const CIMInstance& cimInstance)
{
    out.putInstance(cimInstance);
}

void CIMBinMsgSerializer::_putNamespaceName(
    CIMBuffer& out,
    const CIMNamespaceName& cimNamespaceName)
{
    out.putString(cimNamespaceName.getString());
}

void CIMBinMsgSerializer::_putName(
    CIMBuffer& out,
    const CIMName& cimName)
{
    out.putString(cimName.getString());
}

void CIMBinMsgSerializer::_putObject(
    CIMBuffer& out,
    const CIMObject& object)
{
    if (object.isUninitialized())
        out.putPresent(false);
    else
    {
        out.putPresent(true);
        out.putObject(object);
    }
}

void CIMBinMsgSerializer::_putParamValue(
    CIMBuffer& out,
    const CIMParamValue& paramValue)
{
    out.putParamValue(paramValue);
}

void CIMBinMsgSerializer::_putGetInstanceRequestMessage(
    CIMBuffer& out,
    CIMGetInstanceRequestMessage* msg)
{
    _putObjectPath(out, msg->instanceName);
    out.putBoolean(msg->localOnly);
    out.putBoolean(msg->includeQualifiers);
    out.putBoolean(msg->includeClassOrigin);
    _putPropertyList(out, msg->propertyList);
}

void CIMBinMsgSerializer::_putDeleteInstanceRequestMessage(
    CIMBuffer& out,
    CIMDeleteInstanceRequestMessage* msg)
{
    _putObjectPath(out, msg->instanceName);
}

void CIMBinMsgSerializer::_putCreateInstanceRequestMessage(
    CIMBuffer& out,
    CIMCreateInstanceRequestMessage* msg)
{
    _putInstance(out, msg->newInstance);
}

void CIMBinMsgSerializer::_putModifyInstanceRequestMessage(
    CIMBuffer& out,
    CIMModifyInstanceRequestMessage* msg)
{
    _putInstance(out, msg->modifiedInstance);
    out.putBoolean(msg->includeQualifiers);
    _putPropertyList(out, msg->propertyList);
}

void CIMBinMsgSerializer::_putEnumerateInstancesRequestMessage(
    CIMBuffer& out,
    CIMEnumerateInstancesRequestMessage* msg)
{
    out.putBoolean(msg->deepInheritance);
    out.putBoolean(msg->localOnly);
    out.putBoolean(msg->includeQualifiers);
    out.putBoolean(msg->includeClassOrigin);
    _putPropertyList(out, msg->propertyList);
}

void CIMBinMsgSerializer::_putEnumerateInstanceNamesRequestMessage(
    CIMBuffer& out,
    CIMEnumerateInstanceNamesRequestMessage* msg)
{
}

void CIMBinMsgSerializer::_putExecQueryRequestMessage(
    CIMBuffer& out,
    CIMExecQueryRequestMessage* msg)
{
    out.putString(msg->queryLanguage);
    out.putString(msg->query);
}

void CIMBinMsgSerializer::_putAssociatorsRequestMessage(
    CIMBuffer& out,
    CIMAssociatorsRequestMessage* msg)
{
    _putObjectPath(out, msg->objectName);
    _putName(out, msg->assocClass);
    _putName(out, msg->resultClass);
    out.putString(msg->role);
    out.putString(msg->resultRole);
    out.putBoolean(msg->includeQualifiers);
    out.putBoolean(msg->includeClassOrigin);
    _putPropertyList(out, msg->propertyList);
}

void CIMBinMsgSerializer::_putAssociatorNamesRequestMessage(
    CIMBuffer& out,
    CIMAssociatorNamesRequestMessage* msg)
{
    _putObjectPath(out, msg->objectName);
    _putName(out, msg->assocClass);
    _putName(out, msg->resultClass);
    out.putString(msg->role);
    out.putString(msg->resultRole);
}

void CIMBinMsgSerializer::_putReferencesRequestMessage(
    CIMBuffer& out,
    CIMReferencesRequestMessage* msg)
{
    _putObjectPath(out, msg->objectName);
    _putName(out, msg->resultClass);
    out.putString(msg->role);
    out.putBoolean(msg->includeQualifiers);
    out.putBoolean(msg->includeClassOrigin);
    _putPropertyList(out, msg->propertyList);
}

void CIMBinMsgSerializer::_putReferenceNamesRequestMessage(
    CIMBuffer& out,
    CIMReferenceNamesRequestMessage* msg)
{
    _putObjectPath(out, msg->objectName);
    _putName(out, msg->resultClass);
    out.putString(msg->role);
}

void CIMBinMsgSerializer::_putGetPropertyRequestMessage(
    CIMBuffer& out,
    CIMGetPropertyRequestMessage* msg)
{
    _putObjectPath(out, msg->instanceName);
    _putName(out, msg->propertyName);
}

void CIMBinMsgSerializer::_putSetPropertyRequestMessage(
    CIMBuffer& out,
    CIMSetPropertyRequestMessage* msg)
{
    _putObjectPath(out, msg->instanceName);

    _putParamValue(out,
        CIMParamValue(msg->propertyName.getString(), msg->newValue, true));
}

void CIMBinMsgSerializer::_putInvokeMethodRequestMessage(
    CIMBuffer& out,
    CIMInvokeMethodRequestMessage* msg)
{
    _putObjectPath(out, msg->instanceName);
    _putName(out, msg->methodName);
    out.putParamValueA(msg->inParameters);
}

void CIMBinMsgSerializer::_putCreateSubscriptionRequestMessage(
    CIMBuffer& out,
    CIMCreateSubscriptionRequestMessage* msg)
{
    _putNamespaceName(out, msg->nameSpace);
    _putInstance(out, msg->subscriptionInstance);
    out.putNameA(msg->classNames);
    _putPropertyList(out, msg->propertyList);
    out.putUint16(msg->repeatNotificationPolicy);
    out.putString(msg->query);
}

void CIMBinMsgSerializer::_putModifySubscriptionRequestMessage(
    CIMBuffer& out,
    CIMModifySubscriptionRequestMessage* msg)
{
    _putNamespaceName(out, msg->nameSpace);
    _putInstance(out, msg->subscriptionInstance);
    out.putNameA(msg->classNames);
    _putPropertyList(out, msg->propertyList);
    out.putUint16(msg->repeatNotificationPolicy);
    out.putString(msg->query);
}

void CIMBinMsgSerializer::_putDeleteSubscriptionRequestMessage(
    CIMBuffer& out,
    CIMDeleteSubscriptionRequestMessage* msg)
{
    _putNamespaceName(out, msg->nameSpace);
    _putInstance(out, msg->subscriptionInstance);
    out.putNameA(msg->classNames);
}

void CIMBinMsgSerializer::_putExportIndicationRequestMessage(
    CIMBuffer& out,
    CIMExportIndicationRequestMessage* msg)
{
    _serializeUserInfo(out, msg->authType, msg->userName);
    out.putString(msg->destinationPath);
    _putInstance(out, msg->indicationInstance);
}

void CIMBinMsgSerializer::_putProcessIndicationRequestMessage(
    CIMBuffer& out,
    CIMProcessIndicationRequestMessage* msg)
{
    _putNamespaceName(out, msg->nameSpace);
    _putInstance(out, msg->indicationInstance);
    out.putObjectPathA(msg->subscriptionInstanceNames);
    _putInstance(out, msg->provider);
}

void CIMBinMsgSerializer::_putDisableModuleRequestMessage(
    CIMBuffer& out,
    CIMDisableModuleRequestMessage* msg)
{
    _serializeUserInfo(out, msg->authType, msg->userName);
    _putInstance(out, msg->providerModule);
    out.putInstanceA(msg->providers);
    out.putBoolean(msg->disableProviderOnly);
    out.putBooleanA(msg->indicationProviders);
}

void CIMBinMsgSerializer::_putEnableModuleRequestMessage(
    CIMBuffer& out,
    CIMEnableModuleRequestMessage* msg)
{
    _serializeUserInfo(out, msg->authType, msg->userName);
    _putInstance(out, msg->providerModule);
}

void CIMBinMsgSerializer::_putStopAllProvidersRequestMessage(
    CIMBuffer& out,
    CIMStopAllProvidersRequestMessage* msg)
{
}

void CIMBinMsgSerializer::_putInitializeProviderAgentRequestMessage(
    CIMBuffer& out,
    CIMInitializeProviderAgentRequestMessage* msg)
{
    out.putString(msg->pegasusHome);
    out.putUint32(msg->configProperties.size());

    for (Uint32 i = 0; i < msg->configProperties.size(); i++)
    {
        out.putString(msg->configProperties[i].first);
        out.putString(msg->configProperties[i].second);
    }

    out.putBoolean(msg->bindVerbose);
    out.putBoolean(msg->subscriptionInitComplete);
}

void CIMBinMsgSerializer::_putNotifyConfigChangeRequestMessage(
    CIMBuffer& out,
    CIMNotifyConfigChangeRequestMessage* msg)
{
    out.putString(msg->propertyName);
    out.putString(msg->newPropertyValue);
    out.putBoolean(msg->currentValueModified);
}

void CIMBinMsgSerializer::_putSubscriptionInitCompleteRequestMessage(
    CIMBuffer& out,
    CIMSubscriptionInitCompleteRequestMessage* msg)
{
}

void CIMBinMsgSerializer::_putGetInstanceResponseMessage(
    CIMBuffer& out,
    CIMGetInstanceResponseMessage* msg)
{
    if (msg->binaryResponse)
    {
        CIMBuffer data(4096);
        data.putInstance(msg->getCimInstance(), false, false);
        out.putUint32(data.size());
        out.putBytes(data.getData(), data.size());
    }
    else
    {
        _putXMLInstance(out, msg->getCimInstance());
    }
}

void CIMBinMsgSerializer::_putDeleteInstanceResponseMessage(
    CIMBuffer& out,
    CIMDeleteInstanceResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putCreateInstanceResponseMessage(
    CIMBuffer& out,
    CIMCreateInstanceResponseMessage* msg)
{
    _putObjectPath(out, msg->instanceName);
}

void CIMBinMsgSerializer::_putModifyInstanceResponseMessage(
    CIMBuffer& out,
    CIMModifyInstanceResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putEnumerateInstancesResponseMessage(
    CIMBuffer& out,
    CIMEnumerateInstancesResponseMessage* msg)
{
    if (msg->binaryResponse)
    {
        // Save offset of dummy size:
        Uint32 offset0 = out.size();

        // Put dummy size now and update below.
        out.putUint32(0xABCD1234);

        // Save offset of instances:
        Uint32 offset1 = out.size();

        // Put instance array.
        out.putInstanceA(msg->getNamedInstances(), false);

        // Save offset of end of instances:
        Uint32 offset2 = out.size();

        // Patch dummy size with real size of instance array:
        Uint32* p = reinterpret_cast<Uint32*>(
            const_cast<char*>(out.getData() + offset0));
        *p = offset2 - offset1;
    }
    else
    {
        const Array<CIMInstance>& a = msg->getNamedInstances();

        Uint32 n = a.size();
        out.putUint32(n);

        for (Uint32 i = 0; i < n; i++)
        {
            _putXMLNamedInstance(out, a[i]);
        }
    }
}

void CIMBinMsgSerializer::_putEnumerateInstanceNamesResponseMessage(
    CIMBuffer& out,
    CIMEnumerateInstanceNamesResponseMessage* msg)
{
    out.putObjectPathA(msg->instanceNames);
}

void CIMBinMsgSerializer::_putExecQueryResponseMessage(
    CIMBuffer& out,
    CIMExecQueryResponseMessage* msg)
{
    out.putObjectA(msg->cimObjects);
}

void CIMBinMsgSerializer::_putAssociatorsResponseMessage(
    CIMBuffer& out,
    CIMAssociatorsResponseMessage* msg)
{
    out.putObjectA(msg->cimObjects);
}

void CIMBinMsgSerializer::_putAssociatorNamesResponseMessage(
    CIMBuffer& out,
    CIMAssociatorNamesResponseMessage* msg)
{
    out.putObjectPathA(msg->objectNames);
}

void CIMBinMsgSerializer::_putReferencesResponseMessage(
    CIMBuffer& out,
    CIMReferencesResponseMessage* msg)
{
    out.putObjectA(msg->cimObjects);
}

void CIMBinMsgSerializer::_putReferenceNamesResponseMessage(
    CIMBuffer& out,
    CIMReferenceNamesResponseMessage* msg)
{
    out.putObjectPathA(msg->objectNames);
}

void CIMBinMsgSerializer::_putGetPropertyResponseMessage(
    CIMBuffer& out,
    CIMGetPropertyResponseMessage* msg)
{
    _putParamValue(out, CIMParamValue(String("ignore"), 
        msg->value, true));
}

void CIMBinMsgSerializer::_putSetPropertyResponseMessage(
    CIMBuffer& out,
    CIMSetPropertyResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putInvokeMethodResponseMessage(
    CIMBuffer& out,
    CIMInvokeMethodResponseMessage* msg)
{
    _putParamValue(out,
        CIMParamValue(String("ignore"), msg->retValue, true));
    out.putParamValueA(msg->outParameters);
    _putName(out, msg->methodName);
}

void CIMBinMsgSerializer::_putCreateSubscriptionResponseMessage(
    CIMBuffer& out,
    CIMCreateSubscriptionResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putModifySubscriptionResponseMessage(
    CIMBuffer& out,
    CIMModifySubscriptionResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putDeleteSubscriptionResponseMessage(
    CIMBuffer& out,
    CIMDeleteSubscriptionResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putExportIndicationResponseMessage(
    CIMBuffer& out,
    CIMExportIndicationResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putProcessIndicationResponseMessage(
    CIMBuffer& out,
    CIMProcessIndicationResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putDisableModuleResponseMessage(
    CIMBuffer& out,
    CIMDisableModuleResponseMessage* msg)
{
    out.putUint16A(msg->operationalStatus);
}

void CIMBinMsgSerializer::_putEnableModuleResponseMessage(
    CIMBuffer& out,
    CIMEnableModuleResponseMessage* msg)
{
    out.putUint16A(msg->operationalStatus);
}

void CIMBinMsgSerializer::_putStopAllProvidersResponseMessage(
    CIMBuffer& out,
    CIMStopAllProvidersResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putInitializeProviderAgentResponseMessage(
    CIMBuffer& out,
    CIMInitializeProviderAgentResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putNotifyConfigChangeResponseMessage(
    CIMBuffer& out,
    CIMNotifyConfigChangeResponseMessage* msg)
{
}

void CIMBinMsgSerializer::_putSubscriptionInitCompleteResponseMessage(
    CIMBuffer& out,
    CIMSubscriptionInitCompleteResponseMessage* msg)
{
}

PEGASUS_NAMESPACE_END
