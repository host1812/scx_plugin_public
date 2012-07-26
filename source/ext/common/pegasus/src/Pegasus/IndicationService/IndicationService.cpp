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

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/CIMInstance.h>
#include <Pegasus/Common/ArrayInternal.h>
#include <Pegasus/Common/CIMDateTime.h>
#include <Pegasus/Common/CIMProperty.h>
#include <Pegasus/Common/MessageQueue.h>
#ifdef PEGASUS_INDICATION_PERFINST
#include <Pegasus/Common/Stopwatch.h>
#endif
#include <Pegasus/Common/System.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/XmlWriter.h>
#include <Pegasus/Common/PegasusVersion.h>
#include <Pegasus/Common/AcceptLanguageList.h>
#include <Pegasus/Common/ContentLanguageList.h>
#include <Pegasus/Common/LanguageParser.h>
#include <Pegasus/Common/OperationContextInternal.h>
#include <Pegasus/Common/MessageLoader.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/IndicationFormatter.h>
#include <Pegasus/Server/ProviderRegistrationManager/\
ProviderRegistrationManager.h>
#include <Pegasus/Query/QueryExpression/QueryExpression.h>
#include <Pegasus/Query/QueryCommon/QueryException.h>
#include <Pegasus/Repository/RepositoryQueryContext.h>

#include "IndicationConstants.h"
#include "SubscriptionRepository.h"
#include "SubscriptionTable.h"
#include "IndicationService.h"


PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

//
// Message constants
//

static const char _MSG_PROPERTY_KEY[] =
   "IndicationService.IndicationService._MSG_PROPERTY";
static const char _MSG_PROPERTY[] = "The required property $0 is missing.";

static const char _MSG_NO_PROVIDERS_KEY[] =
    "IndicationService.IndicationService._MSG_NO_PROVIDERS";
static const char _MSG_NO_PROVIDERS[] =
    "No providers are capable of servicing the subscription.";

static const char _MSG_INVALID_TYPE_FOR_PROPERTY_KEY[] =
    "IndicationService.IndicationService._MSG_INVALID_TYPE_FOR_PROPERTY";
static const char _MSG_INVALID_TYPE_FOR_PROPERTY[] =
    "The value of type $0 is not valid for property $1.";

static const char _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY_KEY[] =
    "IndicationService.IndicationService."
        "_MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY";
static const char _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY[] =
    "The value of an array of type $0 is not valid for property $1.";

static const char _MSG_INVALID_VALUE_FOR_PROPERTY_KEY[] =
    "IndicationService.IndicationService._MSG_INVALID_VALUE_FOR_PROPERTY";
static const char _MSG_INVALID_VALUE_FOR_PROPERTY[] =
    "The value $0 is not valid for property $1.";

static const char _MSG_UNSUPPORTED_VALUE_FOR_PROPERTY_KEY[] =
    "IndicationService.IndicationService._MSG_UNSUPPORTED_VALUE_FOR_PROPERTY";
static const char _MSG_UNSUPPORTED_VALUE_FOR_PROPERTY[] =
    "The value $0 is not supported for property $1.";

static const char _MSG_CLASS_NOT_SERVED_KEY[] =
    "IndicationService.IndicationService._MSG_CLASS_NOT_SERVED";
static const char _MSG_CLASS_NOT_SERVED[] =
    "The specified class is not serviced by the CIM Indication service.";

static const char _MSG_INVALID_INSTANCES_KEY[] =
    "IndicationService.IndicationService."
        "INVALID_SUBSCRIPTION_INSTANCES_IGNORED";
static const char _MSG_INVALID_INSTANCES[] =
    "One or more subscription instances are not valid and are ignored.";

static const char _MSG_PROVIDER_NO_LONGER_SERVING_KEY[] =
    "IndicationService.IndicationService._MSG_PROVIDER_NO_LONGER_SERVING";
static const char _MSG_PROVIDER_NO_LONGER_SERVING[] =
    "Provider ($0) is no longer serving subscription ($1) in namespace $2";

static const char _MSG_PROVIDER_NOW_SERVING_KEY[] =
    "IndicationService.IndicationService._MSG_PROVIDER_NOW_SERVING";
static const char _MSG_PROVIDER_NOW_SERVING[] =
    "Provider ($0) is now serving subscription ($1) in namespace $2";

static const char _MSG_NO_PROVIDER_KEY[] =
    "IndicationService.IndicationService._MSG_NO_PROVIDER";
static const char _MSG_NO_PROVIDER[] =
    "Subscription ($0) in namespace $1 has no provider";


// ATTN-RK-20020730: Temporary solution to fix Windows build
Boolean ContainsCIMName(const Array<CIMName>& a, const CIMName& x)
{
    Uint32 n = a.size();

    for (Uint32 i = 0; i < n; i++)
    {
        if (a[i].equal(x))
            return true;
    }

    return false;
}

Mutex IndicationService::_mutex;

IndicationService::IndicationService(
    CIMRepository* repository,
    ProviderRegistrationManager* providerRegManager)
    : MessageQueueService(
          PEGASUS_QUEUENAME_INDICATIONSERVICE, MessageQueue::getNextQueueId()),
      _providerRegManager(providerRegManager),
      _cimRepository(repository)
{
    _enableSubscriptionsForNonprivilegedUsers = false;
    _authenticationEnabled = true;

    try
    {
        // Determine the value for the configuration parameter
        // enableSubscriptionsForNonprivilegedUsers
        ConfigManager* configManager = ConfigManager::getInstance();

        if (ConfigManager::parseBooleanValue(
            configManager->getCurrentValue("enableAuthentication")))
        {
            _enableSubscriptionsForNonprivilegedUsers =
                ConfigManager::parseBooleanValue(
                    configManager->getCurrentValue(
                        "enableSubscriptionsForNonprivilegedUsers"));
        }
        else
        {
            _authenticationEnabled = false;
            // Authentication needs to be enabled to perform authorization
            // tests.
            _enableSubscriptionsForNonprivilegedUsers = true;
        }
     }
     catch (...)
     {
        // If there is an error reading the configuration file then
        // the value of _enableSubscriptionsForNonprivilegedUsers will
        // default to false (i.e., the more restrictive security
        // setting.
        PEG_TRACE_CSTRING(TRC_INDICATION_SERVICE, Tracer::LEVEL2,
            "Failure attempting to read configuration parameters during "
                "initialization.");
     }

    PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
        "Value of _enableSubscriptionsForNonprivilegedUsers is %d",
        _enableSubscriptionsForNonprivilegedUsers));

    try
    {
        //
        //  Create Subscription Repository
        //
        _subscriptionRepository = new SubscriptionRepository(repository);

       //
       // Create IndicationsProfileInstance Repository
       //
#ifdef PEGASUS_ENABLE_DMTF_INDICATION_PROFILE_SUPPORT
        _indicationServiceConfiguration = 
            new IndicationServiceConfiguration(repository);
#endif

        //
        //  Create Subscription Table
        //
        _subscriptionTable = new SubscriptionTable(_subscriptionRepository);

        // Initialize the Indication Service
        _initialize();
    }
    catch (Exception& e)
    {
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
           "Exception caught in attempting to "
           "initialize Indication Service: %s",
           (const char*)e.getMessage().getCString()));
    }
}

IndicationService::~IndicationService()
{
    delete _subscriptionTable;
    delete _subscriptionRepository;

#ifdef PEGASUS_ENABLE_DMTF_INDICATION_PROFILE_SUPPORT
    delete _indicationServiceConfiguration;
#endif
}

void IndicationService::_handle_async_request(AsyncRequest *req)
{
    if (req->getType() == ASYNC_CIMSERVICE_STOP)
    {
        //
        //  Call _terminate
        //
        _terminate();

        handle_CimServiceStop(static_cast<CimServiceStop *>(req));
    }
    else if (req->getType() == ASYNC_CIMSERVICE_START)
    {
        handle_CimServiceStart(static_cast<CimServiceStart *>(req));
    }
    else if (req->getType() == ASYNC_ASYNC_LEGACY_OP_START)
    {
        try
        {
            Message* legacy =
                static_cast<AsyncLegacyOperationStart *>(req)->get_action();
            legacy->put_async(req);

            handleEnqueue(legacy);
        }
        catch(Exception& )
        {
            PEG_TRACE_CSTRING(TRC_INDICATION_SERVICE, Tracer::LEVEL1,
                "Caught Exception in IndicationService while handling a "
                    "wrapped legacy message ");
                _make_response(req, async_results::CIM_NAK);
        }

        return;
    }
    else
        MessageQueueService::_handle_async_request(req);
}

void IndicationService::handleEnqueue(Message* message)
{
#ifdef PEGASUS_INDICATION_PERFINST
    Stopwatch stopWatch;

    stopWatch.start();
#endif

    CIMRequestMessage* cimRequest = dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(cimRequest);

    // Set the client's requested language into this service thread.
    // This will allow functions in this service to return messages
    // in the correct language.
    cimRequest->updateThreadLanguages();

    try
    {
        switch(message->getType())
        {
            case CIM_GET_INSTANCE_REQUEST_MESSAGE:
                _handleGetInstanceRequest(message);
                break;

            case CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE:
                _handleEnumerateInstancesRequest(message);
                break;

            case CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE:
                _handleEnumerateInstanceNamesRequest(message);
                break;

            case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
                _handleCreateInstanceRequest(message);
                break;

            case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
                _handleModifyInstanceRequest(message);
                break;

            case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
                _handleDeleteInstanceRequest(message);
                break;

            case CIM_PROCESS_INDICATION_REQUEST_MESSAGE:
                _handleProcessIndicationRequest(message);
                break;

            case CIM_NOTIFY_PROVIDER_REGISTRATION_REQUEST_MESSAGE:
                _handleNotifyProviderRegistrationRequest(message);
                break;

            case CIM_NOTIFY_PROVIDER_TERMINATION_REQUEST_MESSAGE:
                _handleNotifyProviderTerminationRequest(message);
                break;

            case CIM_NOTIFY_PROVIDER_ENABLE_REQUEST_MESSAGE:
                _handleNotifyProviderEnableRequest(message);
                break;

            case CIM_NOTIFY_PROVIDER_FAIL_REQUEST_MESSAGE:
                _handleNotifyProviderFailRequest(message);
                break;

            default:
                //
                //  A message type not supported by the Indication Service
                //  Should not reach here
                //
                PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
                    "IndicationService::handleEnqueue(msg *) rcv'd unsupported "
                        "message of type %s.",
                    MessageTypeToString(message->getType())));

                // Note: not setting Content-Language in the response
                CIMResponseMessage* response = cimRequest->buildResponse();
                response->cimException = PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_NOT_SUPPORTED,
                    MessageLoaderParms(
                        "IndicationService.IndicationService."
                            "UNSUPPORTED_OPERATION",
                        "The requested operation is not supported or not "
                            "recognized by the indication service."));

                _enqueueResponse(cimRequest, response);
                break;
        }
    }
    catch (CIMException& e)
    {
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
            "CIMException caught in IndicationService::handleEnqueue: %s",
            (const char*)e.getMessage().getCString()));
        CIMResponseMessage* response = cimRequest->buildResponse();
        response->cimException = e;
        _enqueueResponse(cimRequest, response);
    }
    catch (Exception& e)
    {
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
            "Exception caught in IndicationService::handleEnqueue: %s",
            (const char*)e.getMessage().getCString()));
        CIMResponseMessage* response = cimRequest->buildResponse();
        response->cimException =
            PEGASUS_CIM_EXCEPTION(CIM_ERR_FAILED, e.getMessage());
        _enqueueResponse(cimRequest, response);
    }
    catch (...)
    {
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
            "Unknown exception caught in IndicationService::handleEnqueue."));
        CIMResponseMessage* response = cimRequest->buildResponse();
        response->cimException = PEGASUS_CIM_EXCEPTION_L(
            CIM_ERR_FAILED,
            MessageLoaderParms(
                "IndicationService.IndicationService.UNKNOWN_ERROR",
                "Unknown Error"));
        _enqueueResponse(cimRequest, response);
    }

#ifdef PEGASUS_INDICATION_PERFINST
    stopWatch.stop();

    PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
        "%s: %.3f seconds",
        MessageTypeToString(message->getType()),
        stopWatch.getElapsed()));
#endif

   delete message;
}

void IndicationService::handleEnqueue()
{
    Message * message = dequeue();

    PEGASUS_ASSERT(message != 0);
    handleEnqueue(message);
}

void IndicationService::_initialize()
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE, "IndicationService::_initialize");

#ifdef PEGASUS_INDICATION_PERFINST
    Stopwatch stopWatch;

    stopWatch.start();
#endif

    Array<CIMInstance> activeSubscriptions;
    Array<CIMInstance> noProviderSubscriptions;
    Boolean invalidInstance = false;

    //
    //  Find required services
    _providerManager = find_service_qid(PEGASUS_QUEUENAME_PROVIDERMANAGER_CPP);
    _handlerService = find_service_qid(PEGASUS_QUEUENAME_INDHANDLERMANAGER);

    //
    //  Set arrays of valid and supported property values
    //
    //  Note: Valid values are defined by the CIM Event Schema MOF
    //  Supported values are a subset of the valid values
    //  Some valid values, as defined in the MOF, are not currently supported
    //  by the Pegasus IndicationService
    //
    _validStates.append(STATE_UNKNOWN);
    _validStates.append(STATE_OTHER);
    _validStates.append(STATE_ENABLED);
    _validStates.append(STATE_ENABLEDDEGRADED);
    _validStates.append(STATE_DISABLED);
    _supportedStates.append(STATE_ENABLED);
    _supportedStates.append(STATE_DISABLED);
    _validRepeatPolicies.append(_POLICY_UNKNOWN);
    _validRepeatPolicies.append(_POLICY_OTHER);
    _validRepeatPolicies.append(_POLICY_NONE);
    _validRepeatPolicies.append(_POLICY_SUPPRESS);
    _validRepeatPolicies.append(_POLICY_DELAY);
    _supportedRepeatPolicies.append(_POLICY_UNKNOWN);
    _supportedRepeatPolicies.append(_POLICY_OTHER);
    _supportedRepeatPolicies.append(_POLICY_NONE);
    _supportedRepeatPolicies.append(_POLICY_SUPPRESS);
    _supportedRepeatPolicies.append(_POLICY_DELAY);
    _validErrorPolicies.append(_ERRORPOLICY_OTHER);
    _validErrorPolicies.append(_ERRORPOLICY_IGNORE);
    _validErrorPolicies.append(_ERRORPOLICY_DISABLE);
    _validErrorPolicies.append(_ERRORPOLICY_REMOVE);
    _supportedErrorPolicies.append(_ERRORPOLICY_IGNORE);
    _supportedErrorPolicies.append(_ERRORPOLICY_DISABLE);
    _supportedErrorPolicies.append(_ERRORPOLICY_REMOVE);
    _validPersistenceTypes.append(PERSISTENCE_OTHER);
    _validPersistenceTypes.append(PERSISTENCE_PERMANENT);
    _validPersistenceTypes.append(PERSISTENCE_TRANSIENT);
    _supportedPersistenceTypes.append(PERSISTENCE_PERMANENT);
    _supportedPersistenceTypes.append(PERSISTENCE_TRANSIENT);
    _validSNMPVersion.append(SNMPV1_TRAP);
    _validSNMPVersion.append(SNMPV2C_TRAP);
    _validSNMPVersion.append(SNMPV2C_INFORM);
    _validSNMPVersion.append(SNMPV3_TRAP);
    _validSNMPVersion.append(SNMPV3_INFORM);
    _supportedSNMPVersion.append(SNMPV1_TRAP);
    _supportedSNMPVersion.append(SNMPV2C_TRAP);

    //
    //  Set arrays of names of supported properties for each class
    //
    //  Currently, all properties in these classes in CIM 2.5 through CIM 2.9
    //  final schemas are supported.  If support for a new class is added, a new
    //  list of names of supported properties for the class must be added as a
    //  private member to the IndicationService class, and the array values
    //  must be appended here.  When support for a new property is added, the
    //  property name must be appended to the appropriate array(s) here.
    //
    _supportedSubscriptionProperties.append(PEGASUS_PROPERTYNAME_FILTER);
    _supportedSubscriptionProperties.append(PEGASUS_PROPERTYNAME_HANDLER);
    _supportedSubscriptionProperties.append(_PROPERTY_ONFATALERRORPOLICY);
    _supportedSubscriptionProperties.append(_PROPERTY_OTHERONFATALERRORPOLICY);
    _supportedSubscriptionProperties.append(
        _PROPERTY_FAILURETRIGGERTIMEINTERVAL);
    _supportedSubscriptionProperties.append(
        PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE);
    _supportedSubscriptionProperties.append(_PROPERTY_OTHERSTATE);
    _supportedSubscriptionProperties.append(_PROPERTY_LASTCHANGE);
    _supportedSubscriptionProperties.append(_PROPERTY_DURATION);
    _supportedSubscriptionProperties.append(_PROPERTY_STARTTIME);
    _supportedSubscriptionProperties.append(_PROPERTY_TIMEREMAINING);
    _supportedSubscriptionProperties.append(_PROPERTY_REPEATNOTIFICATIONPOLICY);
    _supportedSubscriptionProperties.append(
        _PROPERTY_OTHERREPEATNOTIFICATIONPOLICY);
    _supportedSubscriptionProperties.append(
        _PROPERTY_REPEATNOTIFICATIONINTERVAL);
    _supportedSubscriptionProperties.append(_PROPERTY_REPEATNOTIFICATIONGAP);
    _supportedSubscriptionProperties.append(_PROPERTY_REPEATNOTIFICATIONCOUNT);

    _supportedFormattedSubscriptionProperties =
        _supportedSubscriptionProperties;
    _supportedFormattedSubscriptionProperties.append(
        _PROPERTY_TEXTFORMATOWNINGENTITY);
    _supportedFormattedSubscriptionProperties.append(
        _PROPERTY_TEXTFORMATID);
    _supportedFormattedSubscriptionProperties.append(
        _PROPERTY_TEXTFORMAT);
    _supportedFormattedSubscriptionProperties.append(
        _PROPERTY_TEXTFORMATPARAMETERS);

    _supportedFilterProperties.append(_PROPERTY_CAPTION);
    _supportedFilterProperties.append(_PROPERTY_DESCRIPTION);
    _supportedFilterProperties.append(_PROPERTY_ELEMENTNAME);
    _supportedFilterProperties.append(_PROPERTY_SYSTEMCREATIONCLASSNAME);
    _supportedFilterProperties.append(_PROPERTY_SYSTEMNAME);
    _supportedFilterProperties.append(PEGASUS_PROPERTYNAME_CREATIONCLASSNAME);
    _supportedFilterProperties.append(PEGASUS_PROPERTYNAME_NAME);
    _supportedFilterProperties.append(_PROPERTY_SOURCENAMESPACE);
    _supportedFilterProperties.append(PEGASUS_PROPERTYNAME_QUERY);
    _supportedFilterProperties.append(PEGASUS_PROPERTYNAME_QUERYLANGUAGE);

    Array<CIMName> commonListenerDestinationProperties;
    commonListenerDestinationProperties.append(_PROPERTY_CAPTION);
    commonListenerDestinationProperties.append(_PROPERTY_DESCRIPTION);
    commonListenerDestinationProperties.append(_PROPERTY_ELEMENTNAME);
    commonListenerDestinationProperties.append(
        _PROPERTY_SYSTEMCREATIONCLASSNAME);
    commonListenerDestinationProperties.append(_PROPERTY_SYSTEMNAME);
    commonListenerDestinationProperties.append(
        PEGASUS_PROPERTYNAME_CREATIONCLASSNAME);
    commonListenerDestinationProperties.append(PEGASUS_PROPERTYNAME_NAME);
    commonListenerDestinationProperties.append(
        PEGASUS_PROPERTYNAME_PERSISTENCETYPE);
    commonListenerDestinationProperties.append(_PROPERTY_OTHERPERSISTENCETYPE);

    _supportedCIMXMLHandlerProperties = commonListenerDestinationProperties;
    _supportedCIMXMLHandlerProperties.append(_PROPERTY_OWNER);
    _supportedCIMXMLHandlerProperties.append(
        PEGASUS_PROPERTYNAME_LSTNRDST_DESTINATION);

    _supportedCIMXMLListenerDestinationProperties =
        commonListenerDestinationProperties;
    _supportedCIMXMLListenerDestinationProperties.append(
        PEGASUS_PROPERTYNAME_LSTNRDST_DESTINATION);

    _supportedSNMPHandlerProperties = commonListenerDestinationProperties;
    _supportedSNMPHandlerProperties.append(_PROPERTY_OWNER);
    _supportedSNMPHandlerProperties.append(
        PEGASUS_PROPERTYNAME_LSTNRDST_TARGETHOST);
    _supportedSNMPHandlerProperties.append(_PROPERTY_TARGETHOSTFORMAT);
    _supportedSNMPHandlerProperties.append(_PROPERTY_OTHERTARGETHOSTFORMAT);
    _supportedSNMPHandlerProperties.append(_PROPERTY_PORTNUMBER);
    _supportedSNMPHandlerProperties.append(PEGASUS_PROPERTYNAME_SNMPVERSION);
    _supportedSNMPHandlerProperties.append(_PROPERTY_SNMPSECURITYNAME);
    _supportedSNMPHandlerProperties.append(_PROPERTY_SNMPENGINEID);

    _supportedSyslogListenerDestinationProperties =
        commonListenerDestinationProperties;

    _supportedEmailListenerDestinationProperties =
        commonListenerDestinationProperties;
    _supportedEmailListenerDestinationProperties.append(
        PEGASUS_PROPERTYNAME_LSTNRDST_MAILTO);
    _supportedEmailListenerDestinationProperties.append(
        PEGASUS_PROPERTYNAME_LSTNRDST_MAILCC);
    _supportedEmailListenerDestinationProperties.append(
        PEGASUS_PROPERTYNAME_LSTNRDST_MAILSUBJECT);

    //
    //  Get existing active subscriptions from each namespace in the repository
    //
    invalidInstance = _subscriptionRepository->getActiveSubscriptions(
        activeSubscriptions);
    noProviderSubscriptions.clear();

    PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
        "%u active subscription(s) found on initialization",
        activeSubscriptions.size()));

    String condition;
    String query;
    String queryLanguage;
    CIMPropertyList propertyList;
    Array<ProviderClassList> indicationProviders;

    for (Uint32 i = 0; i < activeSubscriptions.size(); i++)
    {
        //
        //  Check for expired subscription
        //
        try
        {
            if (_isExpired(activeSubscriptions[i]))
            {
                CIMObjectPath path = activeSubscriptions[i].getPath();

                PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
                    "Deleting expired subscription on initialization: %s",
                    (const char *) path.toString().getCString()));

                _deleteExpiredSubscription(path);
                // If subscription is expired delete the subscription
                // and continue on to the next one.
                continue;
            }
        }
        catch (DateTimeOutOfRangeException& e)
        {
            //
            //  This instance from the repository is invalid
            //  Log a message and skip it
            //
            Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER,
                Logger::WARNING,
                MessageLoaderParms(
                    "IndicationService.IndicationService."
                        "INVALID_SUBSCRIPTION_INSTANCE_IGNORED",
                    "An invalid Subscription instance was ignored: $0.",
                    e.getMessage()));
            continue;
        }

        CIMNamespaceName sourceNameSpace;
        Array<CIMName> indicationSubclasses;
        _getCreateParams(activeSubscriptions[i], indicationSubclasses,
            indicationProviders, propertyList, sourceNameSpace, condition,
            query, queryLanguage);

        if (indicationProviders.size() == 0)
        {
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL2,
                "No providers found for subscription on initialization: %s",
                (const char *)
                    activeSubscriptions[i].getPath().toString().getCString()));

            //
            //  There are no providers that can support this subscription
            //  Implement the subscription's On Fatal Error Policy
            //  If subscription is not disabled or removed,
            //  Append this subscription to no provider list and
            //  Insert entries into the subscription hash tables
            //
            if (!_subscriptionRepository->reconcileFatalError(
                    activeSubscriptions[i]))
            {
                noProviderSubscriptions.append(activeSubscriptions[i]);

                _subscriptionTable->insertSubscription(activeSubscriptions[i],
                    indicationProviders, indicationSubclasses, sourceNameSpace);
            }
            continue;
        }

        //
        //  Send Create request message to each provider
        //  NOTE: These Create requests are not associated with a user request,
        //  so there is no associated authType or userName
        //  The Creator from the subscription instance is used for userName,
        //  and authType is not set
        //
        CIMInstance instance = activeSubscriptions[i];
        String creator;
        if (!_getCreator(instance, creator))
        {
            //
            //  This instance from the repository is corrupted
            //  Skip it
            //
            invalidInstance = true;
            continue;
        }

        // Get the language tags that were saved with the subscription instance
        AcceptLanguageList acceptLangs;
        Uint32 propIndex = instance.findProperty(
            PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
        if (propIndex != PEG_NOT_FOUND)
        {
            String acceptLangsString;
            instance.getProperty(propIndex).getValue().get(acceptLangsString);
            if (acceptLangsString.size())
            {
                acceptLangs = LanguageParser::parseAcceptLanguageHeader(
                    acceptLangsString);
            }
        }
        ContentLanguageList contentLangs;
        propIndex = instance.findProperty(
            PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
        if (propIndex != PEG_NOT_FOUND)
        {
            String contentLangsString;
            instance.getProperty(propIndex).getValue().get(contentLangsString);
            if (contentLangsString.size())
            {
                contentLangs = LanguageParser::parseContentLanguageHeader(
                    contentLangsString);
            }
        }

        //
        //  Send Create request message to each provider
        //  Note: SendWait is used instead of SendAsync.  Initialization must
        //  deal with multiple subscriptions, each with multiple providers.
        //  Using SendWait eliminates the need for a callback and the necessity
        //  to handle multiple levels of aggregation, which would add
        //  significant complexity.  Since initialization cannot complete
        //  anyway until responses have been received for all subscriptions,
        //  from all the providers, use of SendWait should not cause a
        //  significant performance issue.
        //
        Array<ProviderClassList> acceptedProviders;
        acceptedProviders = _sendWaitCreateRequests(
            indicationProviders, sourceNameSpace,
            propertyList, condition, query, queryLanguage,
            activeSubscriptions[i],
            acceptLangs,
            contentLangs,
            creator);

        if (acceptedProviders.size() == 0)
        {
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL2,
                "No providers accepted subscription on initialization: %s",
                (const char *)
                    activeSubscriptions[i].getPath().toString().getCString()));

            //
            //  No providers accepted the subscription
            //  Implement the subscription's On Fatal Error Policy
            //  If subscription is not disabled or removed, send alert and
            //  Insert entries into the subscription hash tables
            //
            if (!_subscriptionRepository->reconcileFatalError(
                    activeSubscriptions[i]))
            {
                //
                //  Insert entries into the subscription hash tables
                //
                _subscriptionTable->insertSubscription(
                    activeSubscriptions[i],
                    acceptedProviders,
                    indicationSubclasses,
                    sourceNameSpace);

#if 0
                //
                //  Send alert
                //
                //
                //  Send NoProviderAlertIndication to handler instances
                //  ATTN: NoProviderAlertIndication must be defined
                //
                Array<CIMInstance> subscriptions;
                subscriptions.append(activeSubscriptions[i]);
                CIMInstance indicationInstance = _createAlertInstance(
                    _CLASS_NO_PROVIDER_ALERT, subscriptions);

                PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
                    "Sending NoProvider Alert for %u subscriptions",
                    subscriptions.size()));
                _sendAlerts(subscriptions, indicationInstance);
#endif

                //
                //  Get Subscription Filter Name and Handler Name
                //
                String logString = _getSubscriptionLogString(
                    activeSubscriptions[i]);

                //
                //  Log a message for the subscription
                //
                Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER,
                    Logger::WARNING,
                    MessageLoaderParms(
                        _MSG_NO_PROVIDER_KEY, _MSG_NO_PROVIDER,
                        logString,
                        activeSubscriptions[i].getPath().getNameSpace().
                            getString()));
            }
        }
        else
        {
            //
            //  At least one provider accepted the subscription
            //  Insert entries into the subscription hash tables
            //
            _subscriptionTable->insertSubscription(
                activeSubscriptions[i],
                acceptedProviders,
                indicationSubclasses,
                sourceNameSpace);
        }
    }  // for each active subscription

    //
    //  Log a message if any invalid instances were found
    //
    if (invalidInstance)
    {
        Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING,
            MessageLoaderParms(
                _MSG_INVALID_INSTANCES_KEY, _MSG_INVALID_INSTANCES));
    }

    //
    //  Log a message for any subscription for which there is no longer any
    //  provider
    //
    if (noProviderSubscriptions.size() > 0)
    {
#if 0
        //
        //  Send NoProviderAlertIndication to handler instances
        //  ATTN: NoProviderAlertIndication must be defined
        //
        CIMInstance indicationInstance = _createAlertInstance(
            _CLASS_NO_PROVIDER_ALERT, noProviderSubscriptions);

        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
            "Sending NoProvider Alert for %u subscriptions",
            noProviderSubscriptions.size()));
        _sendAlerts(noProviderSubscriptions, indicationInstance);
#endif
        //
        //  Log a message for each subscription
        //
        for (Uint32 i = 0; i < noProviderSubscriptions.size(); i++)
        {
            //
            //  Get Subscription Filter Name and Handler Name
            //
            String logString =
                _getSubscriptionLogString(noProviderSubscriptions[i]);

            Logger::put_l(
                Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING,
                MessageLoaderParms(
                    _MSG_NO_PROVIDER_KEY,
                    _MSG_NO_PROVIDER,
                    logString,
                    noProviderSubscriptions[i].getPath().getNameSpace().
                        getString()));
        }
    }

    //
    //  Send message to tell Provider Manager that subscription
    //  initialization is complete
    //  Provider Manager calls providers' enableIndications method
    //
    _sendSubscriptionInitComplete();

#ifdef PEGASUS_INDICATION_PERFINST
    stopWatch.stop();

    PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
        "%s: %.3f seconds", "Initialize", stopWatch.getElapsed()));
#endif

    PEG_METHOD_EXIT();
}

void IndicationService::_terminate()
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE, "IndicationService::_terminate");

    Array<CIMInstance> activeSubscriptions;
    CIMInstance indicationInstance;

    //
    //  A message is already logged that CIM Server is shutting down --
    //  no need to log a message
    //
#if 0
    //
    //  Get existing active subscriptions from hash table
    //
    activeSubscriptions = _getActiveSubscriptions();

    if (activeSubscriptions.size() > 0)
    {
        //
        //  Create CimomShutdownAlertIndication instance
        //  ATTN: CimomShutdownAlertIndication must be defined
        //
        indicationInstance = _createAlertInstance(
            _CLASS_CIMOM_SHUTDOWN_ALERT, activeSubscriptions);

        //
        //  Send CimomShutdownAlertIndication to each unique handler instance
        //
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
            "Sending CIMServerShutdown Alert for %u subscriptions",
            activeSubscriptions.size()));
        _sendAlerts(activeSubscriptions, indicationInstance);
    }
#endif

    //
    //  Remove entries from the SubscriptionTable's Active Subscriptions and
    //  Subscription Classes tables
    //
    //  NOTE: The table entries are removed when the SubscriptionTable
    //  destructor is called by the IndicationService destructor.  However,
    //  currently the IndicationService destructor is never called, so the
    //  IndicationService must call the SubscriptionTable clear() function to
    //  remove the table entries.
    _subscriptionTable->clear();

    PEG_METHOD_EXIT();
}

void IndicationService::_checkNonprivilegedAuthorization(
    const String& userName)
{
#ifndef PEGASUS_OS_ZOS
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_checkNonprivilegedAuthorization");

    if (!_enableSubscriptionsForNonprivilegedUsers)
    {
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
            "_checkNonprivilegedAuthorization - checking whether user %s is "
                "privileged",
            (const char*) userName.getCString()));
        if (!System::isPrivilegedUser(userName))
        {
            MessageLoaderParms parms(
                "IndicationService.IndicationService."
                    "_MSG_NON_PRIVILEGED_ACCESS_DISABLED",
                "User ($0) is not authorized to perform this operation.",
                userName);
            PEG_METHOD_EXIT();
            throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_ACCESS_DENIED, parms);
        }
    }

    PEG_METHOD_EXIT();
#endif
}

void IndicationService::_handleCreateInstanceRequest(const Message * message)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleCreateInstanceRequest");

    CIMCreateInstanceRequestMessage* request =
        (CIMCreateInstanceRequestMessage*) message;

    Boolean responseSent = false;

    CIMObjectPath instanceRef;
    CIMObjectPath subscriptionPath;

    CIMInstance instance = request->newInstance.clone();

    String userName = ((IdentityContainer)request->operationContext.get(
        IdentityContainer::NAME)).getUserName();
    _checkNonprivilegedAuthorization(userName);

    AcceptLanguageList acceptLangs =
        ((AcceptLanguageListContainer)request->operationContext.get(
            AcceptLanguageListContainer::NAME)).getLanguages();
    ContentLanguageList contentLangs =
        ((ContentLanguageListContainer)request->operationContext.get(
            ContentLanguageListContainer::NAME)).getLanguages();

    if (_canCreate(instance, request->nameSpace))
    {
        //
        //  If the instance is of the PEGASUS_CLASSNAME_INDSUBSCRIPTION
        //  class or the PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION
        //  class and subscription state is enabled, determine if any
        //  providers can serve the subscription
        //
        Uint16 subscriptionState;
        String condition;
        String query;
        String queryLanguage;
        CIMPropertyList requiredProperties;
        CIMNamespaceName sourceNameSpace;
        Array<CIMName> indicationSubclasses;
        Array<ProviderClassList> indicationProviders;

        if ((instance.getClassName().equal(
                 PEGASUS_CLASSNAME_INDSUBSCRIPTION)) ||
            (instance.getClassName().equal(
                 PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION)))
        {
            _subscriptionRepository->
                beginCreateSubscription(instance.getPath());

            try
            {
                subscriptionPath = instance.getPath();
                //
                //  Get subscription state
                //
                //  NOTE: _canCreate has already validated the
                //  SubscriptionState property in the instance; if missing, it
                //  was added with the default value; if null, it was set to
                //  the default value; if invalid, an exception was thrown
                //
                CIMValue subscriptionStateValue;
                subscriptionStateValue = instance.getProperty(
                    instance.findProperty(
                        PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE)).getValue();
                subscriptionStateValue.get(subscriptionState);

                if ((subscriptionState == STATE_ENABLED) ||
                    (subscriptionState == STATE_ENABLEDDEGRADED))
                {
                    _getCreateParams(instance, indicationSubclasses,
                        indicationProviders, requiredProperties,
                        sourceNameSpace, condition, query, queryLanguage);

                    if (indicationProviders.size() == 0)
                    {
                        //
                        //  There are no providers that can support this
                        //  subscription
                        //

                        throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_NOT_SUPPORTED,
                            MessageLoaderParms(_MSG_NO_PROVIDERS_KEY,
                                _MSG_NO_PROVIDERS));
                    }

                    //
                    //  Send Create request message to each provider
                    //
                    _sendAsyncCreateRequests(indicationProviders,
                        sourceNameSpace, requiredProperties, condition,
                        query, queryLanguage, instance,
                        acceptLangs,
                        contentLangs,
                        request,
                        indicationSubclasses,
                        userName, request->authType);

                    //
                    //  Response is sent from _handleCreateResponseAggregation
                    //
                    responseSent = true;
                }
                else
                {
                    //
                    //  Create instance for disabled subscription
                    //
                    instanceRef = _subscriptionRepository->createInstance(
                        instance, request->nameSpace, userName,
                        acceptLangs, contentLangs, false);
                }
            }
            catch (...)
            {
                _subscriptionRepository->cancelCreateSubscription(
                    subscriptionPath);
                throw;
            }
        }
        else
        {
            //
            //  Create instance for filter or handler
            //
            instanceRef = _subscriptionRepository->createInstance(
                instance, request->nameSpace, userName,
                acceptLangs, contentLangs, false);
        }
    }

    //
    //  Send response, if not sent from callback
    //  (for example, if there are no indication providers that can support a
    //  subscription)
    //
    if (!responseSent)
    {
// l10n - no Content-Language in response
        CIMCreateInstanceResponseMessage* response =
            dynamic_cast<CIMCreateInstanceResponseMessage*>(
                request->buildResponse());
        PEGASUS_ASSERT(response != 0);
        response->instanceName = instanceRef;
        _enqueueResponse(request, response);
    }

    PEG_METHOD_EXIT();
}

void IndicationService::_handleGetInstanceRequest(const Message* message)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleGetInstanceRequest");

    CIMGetInstanceRequestMessage* request =
        (CIMGetInstanceRequestMessage*) message;

    CIMInstance instance;
    String contentLangsString;

    String userName = ((IdentityContainer)request->operationContext.
        get(IdentityContainer::NAME)).getUserName();

#ifdef PEGASUS_ENABLE_DMTF_INDICATION_PROFILE_SUPPORT
    if (request->className.equal(PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE)||
        request->className.equal(
            PEGASUS_CLASSNAME_CIM_INDICATIONSERVICECAPABILITIES))
    {
        _checkNonprivilegedAuthorization(userName);
        instance = _indicationServiceConfiguration->getInstance(
            request->nameSpace,
            request->instanceName,
            request->localOnly,
            request->includeQualifiers,
            request->includeClassOrigin,
            request->propertyList);
    }
    else
#endif

#ifdef PEGASUS_ENABLE_INDICATION_COUNT
    if (request->className.equal(PEGASUS_CLASSNAME_PROVIDERINDDATA))
    {
        instance = _providerIndicationCountTable.
            getProviderIndicationDataInstance(request->instanceName);
    }
    else if (request->className.equal(
             PEGASUS_CLASSNAME_SUBSCRIPTIONINDDATA))
    {
        instance = _subscriptionTable->
            getSubscriptionIndicationDataInstance(request->instanceName);
    }
    else
#endif
    {
        _checkNonprivilegedAuthorization(userName);

        //
        //  Add Creator to property list, if not null
        //  Also, if a Subscription and Time Remaining is requested,
        //  Ensure Subscription Duration and Start Time are in property list
        //
        Boolean setTimeRemaining;
        Boolean startTimeAdded;
        Boolean durationAdded;
        CIMPropertyList propertyList = request->propertyList;
        CIMName className = request->instanceName.getClassName();
        _updatePropertyList(
            className,
            propertyList,
            setTimeRemaining,
            startTimeAdded,
            durationAdded);

        //
        //  Get instance from repository
        //
        instance = _subscriptionRepository->getInstance(
            request->nameSpace,
            request->instanceName,
            request->localOnly,
            request->includeQualifiers,
            request->includeClassOrigin,
            propertyList);

        //
        //  Remove Creator property from instance before returning
        //
        String creator;
        if (!_getCreator(instance, creator))
        {
            //
            //  This instance from the repository is corrupted
            //
            MessageLoaderParms parms(
                _MSG_INVALID_INSTANCES_KEY,
                _MSG_INVALID_INSTANCES);
            throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED, parms);
        }
        instance.removeProperty(
            instance.findProperty(
                PEGASUS_PROPERTYNAME_INDSUB_CREATOR));

        //
        //  Remove the language properties from instance before returning
        //
        Uint32 propIndex = instance.findProperty(
            PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
        if (propIndex != PEG_NOT_FOUND)
        {
            instance.removeProperty(propIndex);
        }

        propIndex = instance.findProperty(
            PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
        if (propIndex != PEG_NOT_FOUND)
        {
             // Get the content languages to be sent in the Content-Language
             // header
             instance.getProperty(propIndex).getValue().
                 get(contentLangsString);
             instance.removeProperty(propIndex);
        }

        //
        //  If a subscription with a duration, calculate subscription time
        //  remaining, and add property to the instance
        //
        if (setTimeRemaining)
        {
            _setTimeRemaining(instance);
            if (startTimeAdded)
            {
                instance.removeProperty(
                    instance.findProperty(
                        _PROPERTY_STARTTIME));
            }
            if (durationAdded)
            {
                instance.removeProperty(
                    instance.findProperty(
                        _PROPERTY_DURATION));
            }
        }
    }

    CIMGetInstanceResponseMessage * response =
        dynamic_cast<CIMGetInstanceResponseMessage *>(request->buildResponse());
    if (contentLangsString.size())
    {
        // Note: setting Content-Language in the response to the
        // contentLanguage in the repository.
        response->operationContext.set(ContentLanguageListContainer(
            LanguageParser::parseContentLanguageHeader(contentLangsString)));
    }
    response->setCimInstance(instance);
    _enqueueResponse(request, response);

    PEG_METHOD_EXIT();
}

void IndicationService::_handleEnumerateInstancesRequest(const Message* message)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleEnumerateInstancesRequest");

    CIMEnumerateInstancesRequestMessage* request =
        (CIMEnumerateInstancesRequestMessage*) message;

    Array<CIMInstance> returnedInstances;
    String aggregatedLangs;

    String userName = ((IdentityContainer)request->operationContext.
        get(IdentityContainer::NAME)).getUserName();

#ifdef PEGASUS_ENABLE_DMTF_INDICATION_PROFILE_SUPPORT
    if (request->className.equal(PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE) ||
        request->className.equal(
            PEGASUS_CLASSNAME_CIM_INDICATIONSERVICECAPABILITIES))
    {
        _checkNonprivilegedAuthorization(userName);
        returnedInstances = _indicationServiceConfiguration->
            enumerateInstancesForClass(
                request->nameSpace,
                request->className,
                request->localOnly,
                request->includeQualifiers,
                request->includeClassOrigin,
                request->propertyList);
    }
    else
#endif

#ifdef PEGASUS_ENABLE_INDICATION_COUNT
    if (request->className.equal(PEGASUS_CLASSNAME_PROVIDERINDDATA))
    {
        returnedInstances = _providerIndicationCountTable.
            enumerateProviderIndicationDataInstances();
    }
    else if (request->className.equal(
             PEGASUS_CLASSNAME_SUBSCRIPTIONINDDATA))
    {
        returnedInstances = _subscriptionTable->
            enumerateSubscriptionIndicationDataInstances();
    }
    else
#endif
    {
        _checkNonprivilegedAuthorization(userName);
        Array<CIMInstance> enumInstances;

        //
        //  Add Creator to property list, if not null
        //  Also, if a Subscription and Time Remaining is requested,
        //  Ensure Subscription Duration and Start Time are in property
        //  list
        //
        Boolean setTimeRemaining;
        Boolean startTimeAdded;
        Boolean durationAdded;
        CIMPropertyList propertyList = request->propertyList;
        _updatePropertyList(request->className,
            propertyList, setTimeRemaining, startTimeAdded, durationAdded);

        enumInstances =
            _subscriptionRepository->enumerateInstancesForClass(
                request->nameSpace, request->className, request->localOnly,
                request->includeQualifiers, request->includeClassOrigin,
                propertyList);

        // Vars used to aggregate the content languages of the subscription
        // instances.
        Boolean langMismatch = false;
        Uint32 propIndex;

        //
        //  Remove Creator and language properties from instances before
        //  returning
        //
        for (Uint32 i = 0; i < enumInstances.size(); i++)
        {
            String creator;
            if (!_getCreator(enumInstances[i], creator))
            {
                //
                //  This instance from the repository is corrupted
                //  Skip it
                //
                continue;
            }
            enumInstances[i].removeProperty(
                enumInstances[i].findProperty(
                    PEGASUS_PROPERTYNAME_INDSUB_CREATOR));

            propIndex = enumInstances[i].findProperty(
                PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
            String contentLangs;
            if (propIndex != PEG_NOT_FOUND)
            {
                enumInstances[i].getProperty(propIndex).getValue().get(
                    contentLangs);
                enumInstances[i].removeProperty(propIndex);
            }

            propIndex = enumInstances[i].findProperty(
                PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                enumInstances[i].removeProperty(propIndex);
            }

            // Determine what to set into the Content-Language header back
            // to the client
            if (!langMismatch)
            {
                if (contentLangs == String::EMPTY)
                {
                    langMismatch = true;
                    aggregatedLangs = String::EMPTY;
                }
                else
                {
                    if (aggregatedLangs == String::EMPTY)
                    {
                        aggregatedLangs = contentLangs;
                    }
                    else if (aggregatedLangs != contentLangs)
                    {
                        langMismatch = true;
                        aggregatedLangs = String::EMPTY;
                    }
                }
            }

            //
            //  If a subscription with a duration, calculate subscription
            //  time remaining, and add property to the instance
            //
            if (setTimeRemaining)
            {
                try
                {
                    _setTimeRemaining(enumInstances[i]);
                }
                catch (DateTimeOutOfRangeException&)
                {
                    //
                    //  This instance from the repository is invalid
                    //  Skip it
                    //
                    continue;
                }
                if (startTimeAdded)
                {
                    enumInstances[i].removeProperty(enumInstances[i].
                        findProperty(_PROPERTY_STARTTIME));
                }
                if (durationAdded)
                {
                    enumInstances[i].removeProperty(
                        enumInstances[i].findProperty(_PROPERTY_DURATION));
                }
            }

            returnedInstances.append(enumInstances[i]);
        }
    }

    CIMEnumerateInstancesResponseMessage* response =
        dynamic_cast<CIMEnumerateInstancesResponseMessage*>(
            request->buildResponse());
    PEGASUS_ASSERT(response != 0);
    if (aggregatedLangs.size())
    {
        // Note: setting Content-Language in the response to the aggregated
        // contentLanguage from the instances in the repository.
        response->operationContext.set(ContentLanguageListContainer(
            LanguageParser::parseContentLanguageHeader(aggregatedLangs)));
    }
    response->setNamedInstances(returnedInstances);
    _enqueueResponse(request, response);

    PEG_METHOD_EXIT();
}

void IndicationService::_handleEnumerateInstanceNamesRequest(
    const Message* message)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleEnumerateInstanceNamesRequest");

    CIMEnumerateInstanceNamesRequestMessage* request =
        (CIMEnumerateInstanceNamesRequestMessage*) message;

    Array<CIMObjectPath> enumInstanceNames;

    String userName = ((IdentityContainer)request->operationContext.get(
        IdentityContainer::NAME)).getUserName();

#ifdef PEGASUS_ENABLE_DMTF_INDICATION_PROFILE_SUPPORT
    if (request->className.equal(
            PEGASUS_CLASSNAME_CIM_INDICATIONSERVICE) ||
        request->className.equal(
            PEGASUS_CLASSNAME_CIM_INDICATIONSERVICECAPABILITIES))
    {
        _checkNonprivilegedAuthorization(userName);
        enumInstanceNames = _indicationServiceConfiguration->
            enumerateInstanceNamesForClass(
                request->nameSpace,
                request->className);
    }
    else
#endif

#ifdef PEGASUS_ENABLE_INDICATION_COUNT
    if (request->className.equal(PEGASUS_CLASSNAME_PROVIDERINDDATA))
    {
        enumInstanceNames = _providerIndicationCountTable.
            enumerateProviderIndicationDataInstanceNames();
    }
    else if (request->className.equal(
             PEGASUS_CLASSNAME_SUBSCRIPTIONINDDATA))
    {
        enumInstanceNames = _subscriptionTable->
            enumerateSubscriptionIndicationDataInstanceNames();
    }
    else
#endif
    {
        _checkNonprivilegedAuthorization(userName);
        enumInstanceNames =
            _subscriptionRepository->enumerateInstanceNamesForClass(
                request->nameSpace,
                request->className);
    }

    // Note: not setting Content-Language in the response
    CIMEnumerateInstanceNamesResponseMessage* response =
        dynamic_cast<CIMEnumerateInstanceNamesResponseMessage *>(
            request->buildResponse());
    PEGASUS_ASSERT(response != 0);
    response->instanceNames = enumInstanceNames;
    _enqueueResponse(request, response);

    PEG_METHOD_EXIT();
}

void IndicationService::_handleModifyInstanceRequest(const Message* message)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleModifyInstanceRequest");

    CIMModifyInstanceRequestMessage* request =
        (CIMModifyInstanceRequestMessage*) message;

    Boolean responseSent = false;

    String userName = ((IdentityContainer)request->operationContext.get(
        IdentityContainer::NAME)).getUserName();
    _checkNonprivilegedAuthorization(userName);

    //
    //  Get the instance name
    //
    CIMObjectPath instanceReference = request->modifiedInstance.getPath();

    //
    //  Get instance from repository
    //
    CIMInstance instance;

    instance = _subscriptionRepository->getInstance(
        request->nameSpace, instanceReference);

    CIMInstance modifiedInstance = request->modifiedInstance;
    if (_canModify(request, instanceReference, instance, modifiedInstance))
    {
        //
        //  Set path in instance
        //
        instanceReference.setNameSpace(request->nameSpace);
        instance.setPath(instanceReference);

        //
        //  Check for expired subscription
        //
        try
        {
            if (_isExpired(instance))
            {
                //
                //  Delete the subscription instance
                //
                _deleteExpiredSubscription(instanceReference);

                PEG_METHOD_EXIT();

                throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED,
                    MessageLoaderParms(
                        "IndicationService.IndicationService._MSG_EXPIRED",
                        "An expired subscription cannot be modified:  the "
                            "subscription is deleted."));
            }
        }
        catch (DateTimeOutOfRangeException&)
        {
            //
            //  This instance from the repository is invalid
            //
            PEG_METHOD_EXIT();
            throw;
        }

        //
        //  _canModify, above, already checked that propertyList is not
        //  null, and that numProperties is 0 or 1
        //
        CIMPropertyList propertyList = request->propertyList;
        if (request->propertyList.size() > 0)
        {
            //
            //  Get current state from instance
            //
            Uint16 currentState;
            Boolean valid = true;
            if (_subscriptionRepository->getState(instance, currentState))
            {
                valid = _validateState(currentState);
            }

            if (!valid)
            {
                //
                //  This instance from the repository is corrupted
                //
                PEG_METHOD_EXIT();
                MessageLoaderParms parms(_MSG_INVALID_INSTANCES_KEY,
                    _MSG_INVALID_INSTANCES);
                throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED, parms);
            }

            //
            //  Get new state
            //
            //  NOTE: _canModify has already validated the
            //  SubscriptionState property in the instance; if missing, it
            //  was added with the default value; if null, it was set to
            //  the default value; if invalid, an exception was thrown
            //
            Uint16 newState;
            modifiedInstance.getProperty(modifiedInstance.findProperty(
                PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE)).getValue().get(
                    newState);

            //
            //  If Subscription State has changed,
            //  Set Time of Last State Change to current date time
            //
            CIMDateTime currentDateTime =
                CIMDateTime::getCurrentDateTime();
            if (newState != currentState)
            {
                if (modifiedInstance.findProperty(_PROPERTY_LASTCHANGE) !=
                    PEG_NOT_FOUND)
                {
                    CIMProperty lastChange = modifiedInstance.getProperty(
                        modifiedInstance.findProperty(
                            _PROPERTY_LASTCHANGE));
                    lastChange.setValue(CIMValue(currentDateTime));
                }
                else
                {
                    modifiedInstance.addProperty(CIMProperty(
                        _PROPERTY_LASTCHANGE, CIMValue(currentDateTime)));
                }
                Array<CIMName> properties =
                    propertyList.getPropertyNameArray();
                properties.append(_PROPERTY_LASTCHANGE);
                propertyList.set(properties);
            }

            //
            //  If Subscription is to be enabled, and this is the first
            //  time, set Subscription Start Time
            //
            if ((newState == STATE_ENABLED) ||
                (newState == STATE_ENABLEDDEGRADED))
            {
                //
                //  If Subscription Start Time is null, set value
                //  to the current date time
                //
                CIMDateTime startTime;
                CIMProperty startTimeProperty = instance.getProperty(
                    instance.findProperty(_PROPERTY_STARTTIME));
                CIMValue startTimeValue = startTimeProperty.getValue();
                Boolean setStart = false;
                if (startTimeValue.isNull())
                {
                    setStart = true;
                }
                else
                {
                    startTimeValue.get(startTime);

                    if (startTime.isInterval())
                    {
                        if (startTime.equal(
                                CIMDateTime(_ZERO_INTERVAL_STRING)))
                        {
                            setStart = true;
                        }
                    }
                }

                if (setStart)
                {
                    if (modifiedInstance.findProperty(_PROPERTY_STARTTIME)
                        != PEG_NOT_FOUND)
                    {
                        CIMProperty startTimeProperty =
                            modifiedInstance.getProperty(
                                modifiedInstance.findProperty(
                                    _PROPERTY_STARTTIME));
                        startTimeProperty.setValue(CIMValue(currentDateTime));
                    }
                    else
                    {
                        modifiedInstance.addProperty(CIMProperty(
                            _PROPERTY_STARTTIME,
                            CIMValue(currentDateTime)));
                    }

                    Array<CIMName> properties =
                        propertyList.getPropertyNameArray();
                    properties.append(_PROPERTY_STARTTIME);
                    propertyList.set(properties);
                }
            }

            // Add the language properties to the modified instance.
            // Note:  These came from the Accept-Language and
            // Content-Language headers in the HTTP messages, and may be
            // empty.
            AcceptLanguageList acceptLangs =
                ((AcceptLanguageListContainer)request->operationContext.get(
                AcceptLanguageListContainer::NAME)).getLanguages();
            modifiedInstance.addProperty(CIMProperty(
                PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS,
                LanguageParser::buildAcceptLanguageHeader(acceptLangs)));

            ContentLanguageList contentLangs =
                ((ContentLanguageListContainer)request->operationContext.get
                (ContentLanguageListContainer::NAME)).getLanguages();
            modifiedInstance.addProperty (CIMProperty
                (PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS,
                LanguageParser::buildContentLanguageHeader(contentLangs)));

            Array<CIMName> properties = propertyList.getPropertyNameArray();
            properties.append (PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
            properties.append (PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
            propertyList.set (properties);

            //
            //  If subscription is to be enabled, determine if there are
            //  any indication providers that can serve the subscription
            //
            Array<ProviderClassList> indicationProviders;
            CIMPropertyList requiredProperties;
            CIMNamespaceName sourceNameSpace;
            String condition;
            String query;
            String queryLanguage;
            Array<CIMName> indicationSubclasses;

            if (((newState == STATE_ENABLED) ||
                 (newState == STATE_ENABLEDDEGRADED))
                && ((currentState != STATE_ENABLED) &&
                    (currentState != STATE_ENABLEDDEGRADED)))
            {
                //
                //  Subscription was previously not enabled but is now to
                //  be enabled
                //
                _getCreateParams(instance, indicationSubclasses,
                    indicationProviders, requiredProperties,
                    sourceNameSpace, condition, query, queryLanguage);

                if (indicationProviders.size() == 0)
                {
                    //
                    //  There are no providers that can support this
                    //  subscription
                    //
                    instance.setPath(instanceReference);
                    _subscriptionRepository->reconcileFatalError(instance);
                    PEG_METHOD_EXIT();

                    throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_NOT_SUPPORTED,
                        MessageLoaderParms(_MSG_NO_PROVIDERS_KEY,
                        _MSG_NO_PROVIDERS));
                }
            }

            //
            //  Modify the instance in the repository
            //
            modifiedInstance.setPath(instanceReference);
            _subscriptionRepository->modifyInstance(
                request->nameSpace, modifiedInstance,
                request->includeQualifiers, propertyList);

            PEG_TRACE((
                TRC_INDICATION_SERVICE,
                Tracer::LEVEL3,
                "IndicationService::_handleModifyInstanceRequest - "
                    "Name Space: %s  Instance name: %s",
                (const char*)
                request->nameSpace.getString().getCString(),
                (const char*)
                modifiedInstance.getClassName().getString().getCString()
            ));

            //
            //  If subscription is newly enabled, send Create requests
            //  and enable providers
            //
            if (((newState == STATE_ENABLED) ||
                 (newState == STATE_ENABLEDDEGRADED))
                && ((currentState != STATE_ENABLED) &&
                    (currentState != STATE_ENABLEDDEGRADED)))
            {
                instanceReference.setNameSpace(request->nameSpace);
                instance.setPath(instanceReference);

                _sendAsyncCreateRequests(
                    indicationProviders,
                    sourceNameSpace,
                    requiredProperties,
                    condition,
                    query,
                    queryLanguage,
                    instance,
                    acceptLangs,
                    contentLangs,
                    request,
                    indicationSubclasses,
                    userName,
                    request->authType);

                //
                //  Response is sent from _handleCreateResponseAggregation
                //
                responseSent = true;
            }
            else if ((newState == STATE_DISABLED) &&
                     ((currentState == STATE_ENABLED) ||
                      (currentState == STATE_ENABLEDDEGRADED)))
            {
                //
                //  Subscription was previously enabled but is now to be
                //  disabled
                //
                instanceReference.setNameSpace(request->nameSpace);
                instance.setPath(instanceReference);
                indicationProviders = _getDeleteParams(instance,
                    indicationSubclasses, sourceNameSpace);

                //
                //  Send Delete requests
                //
                if (indicationProviders.size() > 0)
                {
                    _sendAsyncDeleteRequests(
                        indicationProviders,
                        sourceNameSpace,
                        instance,
                        acceptLangs,
                        contentLangs,
                        request,
                        indicationSubclasses,
                        userName,
                        request->authType);

                    //
                    //  Response is sent from
                    //  _handleDeleteResponseAggregation
                    //
                    responseSent = true;
                }
            }
        }
    }

    //
    //  Send response, if not sent from callback
    //  (for example, if there are no indication providers that can support a
    //  subscription)
    //
    if (!responseSent)
    {
        // Note: don't need to set content-language in the response.
        CIMResponseMessage * response = request->buildResponse();
        _enqueueResponse(request, response);
    }

    PEG_METHOD_EXIT();
}

void IndicationService::_handleDeleteInstanceRequest(const Message* message)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleDeleteInstanceRequest");

    CIMDeleteInstanceRequestMessage* request =
        (CIMDeleteInstanceRequestMessage*) message;

    Boolean responseSent = false;

    String userName = ((IdentityContainer)request->operationContext.get(
        IdentityContainer::NAME)).getUserName();
    _checkNonprivilegedAuthorization(userName);

    //
    //  Check if instance may be deleted -- a filter or handler instance
    //  referenced by a subscription instance may not be deleted
    //
    if (_canDelete(request->instanceName, request->nameSpace, userName))
    {
        //
        //  If a subscription, get the instance from the repository
        //
        CIMInstance subscriptionInstance;
        if (request->instanceName.getClassName().equal(
                PEGASUS_CLASSNAME_INDSUBSCRIPTION) ||
            request->instanceName.getClassName ().equal(
                PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION))
        {
            subscriptionInstance =
                _subscriptionRepository->getInstance(
                    request->nameSpace, request->instanceName);
        }

        //
        //  Delete instance from repository
        //
        _subscriptionRepository->deleteInstance(
            request->nameSpace, request->instanceName);

        PEG_TRACE((
            TRC_INDICATION_SERVICE,
            Tracer::LEVEL3,
            "IndicationService::_handleDeleteInstanceRequest - "
                "Name Space: %s  Instance name: %s",
            (const char*) request->nameSpace.getString().getCString(),
            (const char*)
           request->instanceName.getClassName().getString().getCString()
        ));

        if (request->instanceName.getClassName().equal(
                PEGASUS_CLASSNAME_INDSUBSCRIPTION) ||
            request->instanceName.getClassName ().equal(
                PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION))
        {
            //
            //  If subscription is active, send delete requests to providers
            //  and update hash tables
            //
            Uint16 subscriptionState;
            CIMValue subscriptionStateValue;
            subscriptionStateValue = subscriptionInstance.getProperty(
                subscriptionInstance.findProperty(
                    PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE)).getValue();
            subscriptionStateValue.get(subscriptionState);

            if ((subscriptionState == STATE_ENABLED) ||
                (subscriptionState == STATE_ENABLEDDEGRADED))
            {
                Array<ProviderClassList> indicationProviders;
                Array<CIMName> indicationSubclasses;
                CIMNamespaceName sourceNamespaceName;
                CIMObjectPath instanceReference = request->instanceName;
                instanceReference.setNameSpace(request->nameSpace);
                subscriptionInstance.setPath(instanceReference);

                indicationProviders = _getDeleteParams(
                    subscriptionInstance,
                    indicationSubclasses,
                    sourceNamespaceName);

                if (indicationProviders.size() > 0)
                {
                    //
                    //  Send Delete requests
                    //
                    _sendAsyncDeleteRequests(
                        indicationProviders,
                        sourceNamespaceName,
                        subscriptionInstance,
                        ((AcceptLanguageListContainer)
                            request->operationContext.get(
                                AcceptLanguageListContainer::NAME)).
                                    getLanguages(),
                        ((ContentLanguageListContainer)
                            request->operationContext.get(
                                ContentLanguageListContainer::NAME)).
                                    getLanguages(),
                        request,
                        indicationSubclasses,
                        userName,
                        request->authType);

                    //
                    //  Response is sent from
                    //  _handleDeleteResponseAggregation
                    //
                    responseSent = true;
                }
                else
                {
                    //
                    //  Subscription was enabled, but had no providers
                    //  Remove entries from the subscription hash tables
                    //
                    _subscriptionTable->removeSubscription(
                        subscriptionInstance,
                        indicationSubclasses,
                        sourceNamespaceName,
                        indicationProviders);
                }
            }
        }
    }

    //
    //  Send response, if not sent from callback
    //  (for example, if a subscription had no indication providers)
    //
    if (!responseSent)
    {
        CIMResponseMessage * response = request->buildResponse();
        _enqueueResponse(request, response);
    }

    PEG_METHOD_EXIT();
}

// l10n TODO - might need to globalize another flow and another consumer
// interface (ie. mdd's) if we can't agree on one export flow and consumer
// interface (see PEP67)

void IndicationService::_handleProcessIndicationRequest(Message* message)
{
#ifdef PEGASUS_INDICATION_PERFINST
    Stopwatch stopWatch;
#endif

    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleProcessIndicationRequest");

#ifdef PEGASUS_INDICATION_PERFINST
        stopWatch.reset();
        stopWatch.start();
#endif

    CIMProcessIndicationRequestMessage* request = dynamic_cast<
        CIMProcessIndicationRequestMessage*> (message);
    PEGASUS_ASSERT(request != 0);

    Array<CIMInstance> matchedSubscriptions;
    Array<String> matchedSubscriptionsKeys;

    CIMInstance indication = request->indicationInstance;

    try
    {
        PEG_TRACE ((TRC_INDICATION_GENERATION, Tracer::LEVEL4,
           "Received %s Indication %s from namespace %s from provider %s",
           (const char*)(indication.getClassName().getString().getCString()),
           (const char*)(request->messageId.getCString()),
           (const char*)(request->nameSpace.getString().getCString()),
           (const char*)(request->provider.getProperty(request->provider.
               findProperty(PEGASUS_PROPERTYNAME_NAME)).getValue().toString().
                   getCString())));

        //
        // Get supported properties by the indication provider 
        // Get Indication class properties
        // Check if the provider supports all properties of the indication 
        // class, if so, set to null 
        //
        Array<CIMName> providerSupportedProperties;
        Array<CIMName> indicationClassProperties;
        CIMPropertyList supportedPropertyList;

        for (Uint32 i = 0; i < indication.getPropertyCount(); i++)
        {
            providerSupportedProperties.append(
                indication.getProperty(i).getName());
        }

        supportedPropertyList = _checkPropertyList(providerSupportedProperties,
                                                   request->nameSpace, 
                                                   indication.getClassName(), 
                                                   indicationClassProperties);

        //
        // Get initial subscriptions based on the class name, namespace
        // of the generated indication, and subscriptions specified by the 
        // indication provider if the provider included subscriptions 
        // in the subscriptionInstanceNamesContainer 
        //
        Array<CIMInstance> subscriptions;
        Array<String> subscriptionKeys;
        _getRelevantSubscriptions(
            request->subscriptionInstanceNames,
            indication.getClassName(),
            request->nameSpace,
            request->provider,
            subscriptions,
            subscriptionKeys);

        for (Uint32 i = 0; i < subscriptions.size(); i++)
        {
            try
            {
                QueryExpression queryExpr;
                String filterQuery;
                String queryLanguage;
                String filterName;
                CIMNamespaceName sourceNameSpace;

                //
                //  Get filter query expression of the subscription
                //
                _subscriptionRepository->getFilterProperties
                    (subscriptions[i], filterQuery, sourceNameSpace,
                     queryLanguage, filterName);

                queryExpr = _getQueryExpression(
                    filterQuery, queryLanguage, sourceNameSpace);

                //
                // Evaluate if the subscription matches the indication by 
                // checking:
                // 1) Whether the properties (in WHERE clause) from filter 
                //    query are supported by the indication provider;
                // 2) Whether the subscripton is expired;
                // 3) Whether the filter criteria are met by the generated 
                //    indication 
                //
                if (_subscriptionMatch (subscriptions[i], indication, 
                    supportedPropertyList, queryExpr, sourceNameSpace))
                { 
                    PEG_TRACE ((TRC_INDICATION_GENERATION, Tracer::LEVEL4,
                        "%s Indication %s satisfies filter %s:%s query "
                            "expression  \"%s\"",
                            (const char*)(indication.getClassName().
                                getString().getCString()),
                            (const char*)(request->messageId.getCString()),
                            (const char*)(sourceNameSpace.getString().
                                getCString()),
                            (const char*)(filterName.getCString()),
                            (const char*)(filterQuery.getCString())));

                    //
                    // Format the indication
                    // This includes two parts:
                    // 1) Use QueryExpression::applyProjection to remove 
                    //    properties not listed in the SELECT clause;
                    // 2) Remove any properties that may be left on the 
                    //    indication that are not in the indication class. 
                    //    These are properties added by the provider 
                    //    incorrectly.
                    //
                    CIMInstance formattedIndication = indication.clone();

                    if (_formatIndication(formattedIndication, 
                                          queryExpr,
                                          providerSupportedProperties, 
                                          indicationClassProperties))
                    {
                        //
                        // get the handler instance and forward the formatted
                        // indication to the handler
                        //
                        CIMInstance handlerInstance = 
                            _subscriptionRepository->getHandler(
                                subscriptions[i]);

                        PEG_TRACE((TRC_INDICATION_GENERATION, Tracer::LEVEL4,
                            "Handler %s:%s.%s found for %s Indication %s",
                            (const char*)(request->nameSpace.getString().
                                getCString()),
                            (const char*)(handlerInstance.getClassName().
                                getString().getCString()),
                            (const char*)(handlerInstance.getProperty(
                                handlerInstance.findProperty(
                                    PEGASUS_PROPERTYNAME_NAME)).getValue().
                                        toString().getCString()),
                            (const char*)(indication.getClassName().
                                getString().getCString()),
                            (const char*)(request->messageId.getCString())));

                        _forwardIndToHandler(subscriptions[i], 
                                             handlerInstance, 
                                             formattedIndication, 
                                             request->nameSpace,
                                             request->operationContext);

                        matchedSubscriptions.append(subscriptions[i]);
                        matchedSubscriptionsKeys.append(subscriptionKeys[i]);
                    }
                }
            }
            catch (Exception& e)
            {
                PEG_TRACE ((TRC_DISCARDED_DATA, Tracer::LEVEL1,
                    "Exception caught in attempting to process indication "
                        "for the subscription %s: %s",
                        (const char *) subscriptions[i].getPath ().toString().
                            getCString(),
                        (const char *) e.getMessage ().getCString())); 
            }
            catch (exception& e)
            {
                PEG_TRACE ((TRC_DISCARDED_DATA, Tracer::LEVEL1,
                    "Exception caught in attempting to process indication "
                        "for the subscription %s: %s",
                    (const char *) subscriptions[i].getPath ().toString().
                        getCString(), e.what()));
           }
           catch (...)
           {
               PEG_TRACE ((TRC_DISCARDED_DATA, Tracer::LEVEL1,
                   "Unknown exception caught in attempting to process "
                       "indication for the subscription %s",
                    (const char *) subscriptions[i].getPath ().toString ().
                        getCString()));
           }

        }

#ifdef PEGASUS_ENABLE_INDICATION_COUNT
        _providerIndicationCountTable.incrementEntry(
            request->provider, matchedSubscriptions.size() == 0);
        _subscriptionTable->updateMatchedIndicationCounts(
            request->provider, matchedSubscriptionsKeys);
#endif

        //
        //  Log subscriptions info to a trace message
        //
        if (matchedSubscriptions.size() == 0)
        {
           PEG_TRACE ((TRC_INDICATION_GENERATION, Tracer::LEVEL1,
               "No matching subscriptions found for %s Indication %s",
               (const char*)(indication.getClassName().getString().
                   getCString()),
               (const char*)(request->messageId.getCString())));
        }
        else
        {
            PEG_TRACE ((TRC_INDICATION_GENERATION, Tracer::LEVEL4,
                "%d subscriptions found for %s Indication %s in namespace %s",
                matchedSubscriptions.size(),
                (const char*)(indication.getClassName().getString().
                    getCString()),
                (const char*)(request->messageId.getCString()),
                (const char*)(request->nameSpace.getString().getCString())));
        }
    }
    catch (Exception& e)
    {
        PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Exception caught while processing indication: %s.  "
                "Indication may be lost.",
            (const char*)e.getMessage().getCString()));
        PEG_METHOD_EXIT();
        throw;
    }
    catch (...)
    {
        PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Exception caught while processing indication.  Indication may be "
                "lost.");
        PEG_METHOD_EXIT();
        throw;
    }

    _enqueueResponse(request, request->buildResponse());

#ifdef PEGASUS_INDICATION_PERFINST
    stopWatch.stop();

    PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
        "%s: %.3f seconds", "Process Indication", stopWatch.getElapsed()));
#endif

    PEG_METHOD_EXIT ();
    return;
}

void IndicationService::_handleIndicationCallBack (
    AsyncOpNode * operation,
    MessageQueue * destination,
    void * userParameter)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_handleIndicationCallBack");

    IndicationService * service =
        static_cast<IndicationService *> (destination);
    CIMInstance * subscription =
        reinterpret_cast<CIMInstance *> (userParameter);
    AsyncReply * asyncReply =
        static_cast<AsyncReply *>(operation->removeResponse());
    CIMHandleIndicationResponseMessage* handlerResponse =
        reinterpret_cast<CIMHandleIndicationResponseMessage *>(
            (static_cast<AsyncLegacyOperationResult *>(
                asyncReply))->get_result());
    PEGASUS_ASSERT (handlerResponse != 0);

    if (handlerResponse->cimException.getCode () != CIM_ERR_SUCCESS)
    {
        PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Sending Indication and HandlerService returns CIMException: %s",
            (const char*)
                handlerResponse->cimException.getMessage().getCString()));

        //
        //  ATTN-CAKG-P1-20020326: Implement subscription's OnFatalErrorPolicy
        //
        //service->_subscriptionRepository->reconcileFatalError (*subscription);
    }

    delete handlerResponse;
    delete asyncReply;
    service->return_op (operation);

    PEG_METHOD_EXIT ();
}

void IndicationService::_handleNotifyProviderRegistrationRequest
    (const Message* message)
{
    PEG_METHOD_ENTER ( TRC_INDICATION_SERVICE,
        "IndicationService::_handleNotifyProviderRegistrationRequest");

    CIMNotifyProviderRegistrationRequestMessage* request =
        (CIMNotifyProviderRegistrationRequestMessage*) message;

    ProviderIdContainer pidc = request->operationContext.get
        (ProviderIdContainer::NAME);
    CIMInstance provider = pidc.getProvider();
    CIMInstance providerModule = pidc.getModule();
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
    Boolean isRemoteNameSpace = pidc.isRemoteNameSpace();
    String remoteInfo = pidc.getRemoteInfo();
#endif

    CIMName className = request->className;
    Array<CIMNamespaceName> newNameSpaces = request->newNamespaces;
    Array<CIMNamespaceName> oldNameSpaces = request->oldNamespaces;
    CIMPropertyList newPropertyNames = request->newPropertyNames;
    CIMPropertyList oldPropertyNames = request->oldPropertyNames;

    Array<CIMInstance> newSubscriptions;
    Array<CIMInstance> formerSubscriptions;
    Array<ProviderClassList> indicationProviders;
    ProviderClassList indicationProvider;

    newSubscriptions.clear ();
    formerSubscriptions.clear ();

    switch (request->operation)
    {
        case OP_CREATE:
        {
            //
            //  Get matching subscriptions
            //
            newSubscriptions = _getMatchingSubscriptions (className,
                newNameSpaces, newPropertyNames);

            break;
        }

        case OP_DELETE:
        {
            //
            //  Get matching subscriptions
            //
            formerSubscriptions = _getMatchingSubscriptions (className,
                oldNameSpaces, oldPropertyNames);

#ifdef PEGASUS_ENABLE_INDICATION_COUNT
            _providerIndicationCountTable.removeEntry(provider);
#endif

            break;
        }

        case OP_MODIFY:
        {
            //
            //  Get lists of affected subscriptions
            //
            _getModifiedSubscriptions (className, newNameSpaces, oldNameSpaces,
                newPropertyNames, oldPropertyNames,
                newSubscriptions, formerSubscriptions);

            break;
        }
        default:
            //
            //  Error condition: operation not supported
            //
            PEG_METHOD_EXIT ();
            throw PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);
            break;
    }  // switch

    //
    //  Construct provider class list from input provider and class name
    //
    indicationProvider.provider = provider;
    indicationProvider.providerModule = providerModule;
    indicationProvider.classList.append (className);
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
    indicationProvider.isRemoteNameSpace = isRemoteNameSpace;
    indicationProvider.remoteInfo = remoteInfo;
#endif
    indicationProviders.append (indicationProvider);

    if (newSubscriptions.size () > 0)
    {
        CIMPropertyList requiredProperties;
        String condition;
        String query;
        String queryLanguage;

        //
        //  Send Create or Modify request for each subscription that can newly
        //  be supported
        //
        for (Uint32 i = 0; i < newSubscriptions.size (); i++)
        {
            CIMNamespaceName sourceNameSpace;
            Array<CIMName> indicationSubclasses;
            _getCreateParams (newSubscriptions[i], indicationSubclasses,
                requiredProperties, sourceNameSpace, condition,
                query, queryLanguage);

            //
            //  NOTE: These Create or Modify requests are not associated with a
            //  user request, so there is no associated authType or userName
            //  The Creator from the subscription instance is used for
            //  userName, and authType is not set
            //
            //  NOTE: the subscriptions in the newSubscriptions list came from
            //  the IndicationService's internal hash tables, and thus
            //  each instance is known to have a valid Creator property
            //
            CIMInstance instance = newSubscriptions[i];
            String creator = instance.getProperty (instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_CREATOR)).getValue ().toString ();

// l10n start
            AcceptLanguageList acceptLangs;
            Uint32 propIndex = instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String acceptLangsString;
                instance.getProperty(propIndex).getValue().get(
                    acceptLangsString);
                if (acceptLangsString.size())
                {
                    acceptLangs = LanguageParser::parseAcceptLanguageHeader(
                        acceptLangsString);
                }
            }
            ContentLanguageList contentLangs;
            propIndex = instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String contentLangsString;
                instance.getProperty(propIndex).getValue().get(
                    contentLangsString);
                if (contentLangsString.size())
                {
                    contentLangs = LanguageParser::parseContentLanguageHeader(
                        contentLangsString);
                }
            }
// l10n end

            //
            //  Look up the subscription in the active subscriptions table
            //
            ActiveSubscriptionsTableEntry tableValue;
            if (_subscriptionTable->getSubscriptionEntry
                (newSubscriptions[i].getPath (), tableValue))
            {
                //
                //  If the provider is already in the subscription's list,
                //  send a Modify request, otherwise send a Create request
                //
                Uint32 providerIndex = _subscriptionTable->providerInList
                    (provider, tableValue);
                if (providerIndex != PEG_NOT_FOUND)
                {
                    //
                    //  Send Modify requests
                    //
                    _sendWaitModifyRequests (indicationProviders,
                        sourceNameSpace,
                        requiredProperties, condition, query, queryLanguage,
                        newSubscriptions[i],
                        acceptLangs,
                        contentLangs,
                        creator);
                }
                else
                {
                    //
                    //  Send Create requests
                    //
                    Array<ProviderClassList> acceptedProviders;
                    acceptedProviders = _sendWaitCreateRequests
                        (indicationProviders,
                        sourceNameSpace, requiredProperties, condition,
                        query, queryLanguage, newSubscriptions[i],
                        acceptLangs,
                        contentLangs,
                        creator);

                    if (acceptedProviders.size () > 0)
                    {
                        //
                        //  Provider is not yet in the list for this
                        //  subscription; add provider to the list
                        //
                        _subscriptionTable->updateProviders
                            (instance.getPath (), indicationProvider, true);
                    }
                }
            }
            else
            {
                //
                //  Subscription not found in Active Subscriptions table
                //
            }
        }

        //
        //  NOTE: When a provider that was previously not serving a subscription
        //  now serves the subscription due to a provider registration change,
        //  a log message is sent, even if there were previously other providers
        //  serving the subscription
        //

        //
        //  Log a message for each subscription
        //
        CIMClass providerClass = _subscriptionRepository->getClass
            (PEGASUS_NAMESPACENAME_INTEROP, PEGASUS_CLASSNAME_PROVIDER,
             true, true, false, CIMPropertyList ());
        CIMInstance providerCopy = provider.clone ();
        CIMObjectPath path = providerCopy.buildPath (providerClass);
        providerCopy.setPath (path);
        String logString1 = getProviderLogString (providerCopy);

        for (Uint32 j = 0; j < newSubscriptions.size (); j++)
        {
            //
            //  Get Provider Name, Subscription Filter Name and Handler Name
            //
            String logString2 = _getSubscriptionLogString
                (newSubscriptions[j]);

            Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER,
                Logger::WARNING,
                MessageLoaderParms(
                    _MSG_PROVIDER_NOW_SERVING_KEY,
                    _MSG_PROVIDER_NOW_SERVING, logString1, logString2,
                    newSubscriptions[j].getPath().getNameSpace().getString()));
        }
    }

    if (formerSubscriptions.size () > 0)
    {
        CIMPropertyList requiredProperties;
        String condition;
        String query;
        String queryLanguage;

        //
        //  Send Delete or Modify request for each subscription that can no
        //  longer be supported
        //
        for (Uint32 i = 0; i < formerSubscriptions.size (); i++)
        {
            //
            //  NOTE: These Delete or Modify requests are not associated with a
            //  user request, so there is no associated authType or userName
            //  The Creator from the subscription instance is used for userName,
            //  and authType is not set
            //
            //  NOTE: the subscriptions in the formerSubscriptions list came
            //  from the IndicationService's internal hash tables, and thus
            //  each instance is known to have a valid Creator property
            //
            CIMInstance instance = formerSubscriptions[i];
            String creator = instance.getProperty (instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_CREATOR)).getValue ().toString ();
            AcceptLanguageList acceptLangs;
            Uint32 propIndex = instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String acceptLangsString;
                instance.getProperty(propIndex).getValue().get(
                    acceptLangsString);
                if (acceptLangsString.size())
                {
                    acceptLangs = LanguageParser::parseAcceptLanguageHeader(
                        acceptLangsString);
                }
            }
            ContentLanguageList contentLangs;
            propIndex = instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String contentLangsString;
                instance.getProperty(propIndex).getValue().get(
                    contentLangsString);
                if (contentLangsString.size())
                {
                    contentLangs = LanguageParser::parseContentLanguageHeader(
                        contentLangsString);
                }
            }

            //
            //  Look up the subscription in the active subscriptions table
            //  If class list contains only the class name from the current
            //  operation, send a Delete request
            //  Otherwise, send a Modify request
            //
            ActiveSubscriptionsTableEntry tableValue;
            if (_subscriptionTable->getSubscriptionEntry
                (formerSubscriptions[i].getPath (), tableValue))
            {
                Uint32 providerIndex = _subscriptionTable->providerInList
                    (provider, tableValue);
                if (providerIndex != PEG_NOT_FOUND)
                {
                    CIMNamespaceName sourceNameSpace;
                    Array<CIMName> indicationSubclasses;
                    _getCreateParams (formerSubscriptions[i],
                        indicationSubclasses, requiredProperties,
                        sourceNameSpace, condition, query, queryLanguage);

                    //
                    //  If class list contains only the class name from the
                    //  current delete, send a Delete request
                    //
                    if ((tableValue.providers[providerIndex].classList.size()
                            == 1) &&
                        (tableValue.providers[providerIndex].classList[0].equal(
                            className)))
                    {
                        _sendWaitDeleteRequests (indicationProviders,
                            sourceNameSpace,
                            formerSubscriptions[i],
                            acceptLangs,
                            contentLangs,
                            creator);

                        //
                        //
                        //
                        _subscriptionTable->updateProviders
                            (instance.getPath (), indicationProvider, false);
                    }

                    //
                    //  Otherwise, send a Modify request
                    //
                    else
                    {
                        Uint32 classIndex = _subscriptionTable->classInList
                            (className, tableValue.providers[providerIndex]);
                        if (classIndex != PEG_NOT_FOUND)
                        {
                            //
                            //  Send Modify requests
                            //
                            _sendWaitModifyRequests (indicationProviders,
                                sourceNameSpace,
                                requiredProperties, condition,
                                query, queryLanguage,
                                formerSubscriptions[i],
                                acceptLangs,
                                contentLangs,
                                creator);
                        }
                        else
                        {
                            PEG_TRACE((TRC_INDICATION_SERVICE,Tracer::LEVEL1,
                                "Class %s not found in tableValue.providers",
                                (const char*)className.getString().getCString()
                                ));
                        }
                    }
                }
                else
                {
                    //
                    //  The subscription was not served by the provider
                    //
                }
            }
            else
            {
                //
                //  Subscription not found in Active Subscriptions table
                //
            }
        }

#if 0
        //
        //  Create NoProviderAlertIndication instance
        //  ATTN: NoProviderAlertIndication must be defined
        //
        CIMInstance indicationInstance = _createAlertInstance
            (_CLASS_NO_PROVIDER_ALERT, formerSubscriptions);

        //
        //  Send NoProviderAlertIndication to each unique handler instance
        //
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
            "Sending NoProvider Alert for %u subscriptions",
            formerSubscriptions.size ()));
        _sendAlerts (formerSubscriptions, indicationInstance);
#endif
        //
        //  NOTE: When a provider that was previously serving a subscription
        //  no longer serves the subscription due to a provider registration
        //  change, a log message is sent, even if there are still other
        //  providers serving the subscription
        //

        //
        //  Log a message for each subscription
        //
        CIMClass providerClass = _subscriptionRepository->getClass
            (PEGASUS_NAMESPACENAME_INTEROP, PEGASUS_CLASSNAME_PROVIDER,
             true, true, false, CIMPropertyList ());
        CIMInstance providerCopy = provider.clone ();
        CIMObjectPath path = providerCopy.buildPath (providerClass);
        providerCopy.setPath (path);
        String logString1 = getProviderLogString (providerCopy);

        for (Uint32 j = 0; j < formerSubscriptions.size (); j++)
        {
            //
            //  Get Provider Name, Subscription Filter Name and Handler Name
            //
            String logString2 = _getSubscriptionLogString
                (formerSubscriptions[j]);

            Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER,
                Logger::WARNING,
                MessageLoaderParms(
                    _MSG_PROVIDER_NO_LONGER_SERVING_KEY,
                    _MSG_PROVIDER_NO_LONGER_SERVING, logString1, logString2,
                    formerSubscriptions[j].getPath().getNameSpace().
                        getString()));
        }
    }

    //
    //  Send response
    //
    CIMResponseMessage * response = request->buildResponse ();
    _enqueueResponse (request, response);

    PEG_METHOD_EXIT ();
}

void IndicationService::_handleNotifyProviderTerminationRequest
    (const Message * message)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_handleNotifyProviderTermination");

    Array<CIMInstance> providerSubscriptions;
    CIMInstance indicationInstance;

    CIMNotifyProviderTerminationRequestMessage* request =
        (CIMNotifyProviderTerminationRequestMessage*) message;

    Array<CIMInstance> providers = request->providers;

    for (Uint32 i = 0; i < providers.size (); i++)
    {
#ifdef PEGASUS_ENABLE_INDICATION_COUNT
        _providerIndicationCountTable.removeEntry(providers[i]);
#endif

        //
        //  Get list of affected subscriptions
        //
        //  _subscriptionTable->reflectProviderDisable also updates the
        //  Active Subscriptions hash table, and implements each subscription's
        //  On Fatal Error policy, if necessary
        //
        providerSubscriptions.clear();
        providerSubscriptions = _subscriptionTable->reflectProviderDisable(
            providers[i]);

        if (providerSubscriptions.size() > 0)
        {
            //
            //  NOTE: When a provider that was previously serving a subscription
            //  no longer serves the subscription due to a provider termination,
            //  an alert is always sent, even if there are still other providers
            //  serving the subscription
            //

#if 0
            //
            //  Create ProviderTerminatedAlertIndication instance
            //  ATTN: ProviderTerminatedAlertIndication must be defined
            //
            indicationInstance = _createAlertInstance
                (_CLASS_PROVIDER_TERMINATED_ALERT, providerSubscriptions);

            //
            //  Send ProviderTerminatedAlertIndication to each unique handler
            //  instance
            //
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
                "Sending ProviderDisabled Alert for %u subscriptions",
                providerSubscriptions.size ()));
            _sendAlerts (providerSubscriptions, indicationInstance);
#endif
            //
            //  Log a message for each subscription
            //
            CIMClass providerClass = _subscriptionRepository->getClass(
                PEGASUS_NAMESPACENAME_INTEROP, PEGASUS_CLASSNAME_PROVIDER,
                true, true, false, CIMPropertyList());
            CIMInstance providerCopy = providers[i].clone();
            CIMObjectPath path = providerCopy.buildPath (providerClass);
            providerCopy.setPath (path);
            for (Uint32 j = 0; j < providerSubscriptions.size (); j++)
            {
                //
                //  Get Provider Name, Subscription Filter Name and Handler Name
                //
                String logString1 = getProviderLogString (providerCopy);
                String logString2 = _getSubscriptionLogString
                    (providerSubscriptions[j]);

                Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER,
                    Logger::WARNING,
                    MessageLoaderParms(
                        _MSG_PROVIDER_NO_LONGER_SERVING_KEY,
                        _MSG_PROVIDER_NO_LONGER_SERVING, logString1, logString2,
                        providerSubscriptions[j].getPath().getNameSpace().
                            getString()));
            }
        }
    }

    CIMResponseMessage * response = request->buildResponse ();
    _enqueueResponse (request, response);

    PEG_METHOD_EXIT ();
}

void IndicationService::_handleNotifyProviderEnableRequest
    (const Message * message)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_handleNotifyProviderEnableRequest");

    CIMNotifyProviderEnableRequestMessage * request =
        (CIMNotifyProviderEnableRequestMessage *) message;
    ProviderIdContainer pidc = request->operationContext.get
        (ProviderIdContainer::NAME);
    CIMInstance providerModule = pidc.getModule();
    CIMInstance provider = pidc.getProvider();
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
    Boolean isRemoteNameSpace = pidc.isRemoteNameSpace();
    String remoteInfo = pidc.getRemoteInfo();
#endif
    Array<CIMInstance> capabilities = request->capInstances;

    CIMException cimException;
    Array<CIMInstance> subscriptions;
    Array<ProviderClassList> indicationProviders;

    //
    //  Get class name, namespace names, and property list
    //  from each capability instance
    //
    Uint32 numCapabilities = capabilities.size ();
    for (Uint32 i = 0; i < numCapabilities; i++)
    {
        CIMName className;
        Array<CIMNamespaceName> namespaceNames;
        CIMPropertyList propertyList;
        Array<CIMInstance> currentSubscriptions;

        try
        {
            String cName;
            capabilities[i].getProperty (capabilities[i].findProperty
                (_PROPERTY_CLASSNAME)).getValue ().get (cName);
            className = CIMName (cName);

            Array<String> nsNames;
            capabilities[i].getProperty (capabilities[i].findProperty
                (_PROPERTY_NAMESPACES)).getValue ().get (nsNames);
            for (Uint32 j = 0; j < nsNames.size (); j++)
            {
                namespaceNames.append (CIMNamespaceName (nsNames[j]));
            }

            Array<String> pNames;
            Array<CIMName> propertyNames;
            Uint32 propertiesIndex = capabilities[i].findProperty
                (_PROPERTY_SUPPORTEDPROPERTIES);
            if (propertiesIndex != PEG_NOT_FOUND)
            {
                CIMValue propertiesValue = capabilities[i].getProperty
                    (propertiesIndex).getValue ();
                //
                //  If the property list is not null, set the property names
                //
                if (!propertiesValue.isNull ())
                {
                    propertiesValue.get (pNames);
                    for (Uint32 k = 0; k < pNames.size (); k++)
                    {
                        propertyNames.append (CIMName (pNames[k]));
                    }
                    propertyList.set (propertyNames);
                }
            }
        }
        catch (Exception& exception)
        {
            //
            //  Error getting information from Capabilities instance
            //
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
               "Exception caught in handling provider enable notification: %s",
                (const char*)exception.getMessage().getCString()));

            cimException = CIMException(CIM_ERR_FAILED, exception.getMessage());
            break;
        }
        catch (...)
        {
            PEG_TRACE_CSTRING (TRC_INDICATION_SERVICE, Tracer::LEVEL1,
               "Error in handling provider enable notification");

            cimException = PEGASUS_CIM_EXCEPTION_L(
                CIM_ERR_FAILED,
                MessageLoaderParms(
                    "IndicationService.IndicationService.UNKNOWN_ERROR",
                    "Unknown Error"));
            break;
        }

        //
        //  Get matching subscriptions
        //
        currentSubscriptions = _getMatchingSubscriptions
            (className, namespaceNames, propertyList);

        for (Uint32 c = 0; c < currentSubscriptions.size (); c++)
        {
            Boolean inList = false;

            for (Uint32 m = 0; m < subscriptions.size (); m++)
            {
                //
                //  If the current subscription is already in the list of
                //  matching subscriptions, add the current class to the
                //  indication provider class list for the subscription
                //
                if (currentSubscriptions[c].identical (subscriptions[m]))
                {
                    inList = true;
                    indicationProviders[m].classList.append (className);
                    break;
                }
            }

            if (!inList)
            {
                //
                //  If the current subscription is not already in the list of
                //  matching subscriptions, add it to the list and add the
                //  indication provider class list for the subscription
                //
                subscriptions.append (currentSubscriptions[c]);
                ProviderClassList indicationProvider;
                indicationProvider.provider = provider;
                indicationProvider.providerModule = providerModule;
                indicationProvider.classList.append (className);
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
                indicationProvider.isRemoteNameSpace = isRemoteNameSpace;
                indicationProvider.remoteInfo = remoteInfo;
#endif
                indicationProviders.append (indicationProvider);
            }
        }
    }  //  for each capability instance

    if (subscriptions.size () > 0)
    {
        CIMPropertyList requiredProperties;
        String condition;
        String query;
        String queryLanguage;

        //
        //  Get Provider Name
        //
        String logString1 = getProviderLogString (provider);

        for (Uint32 s = 0; s < subscriptions.size (); s++)
        {
            CIMNamespaceName sourceNameSpace;
            Array<CIMName> indicationSubclasses;
            CIMInstance instance = subscriptions[s];
            _getCreateParams (instance, indicationSubclasses,
                requiredProperties, sourceNameSpace, condition, query,
                queryLanguage);

            //
            //  NOTE: These Create requests are not associated with a
            //  user request, so there is no associated authType or userName
            //  The Creator from the subscription instance is used for
            //  userName, and authType is not set
            //
            //  NOTE: the subscriptions in the subscriptions list came from
            //  the IndicationService's internal hash tables, and thus
            //  each instance is known to have a valid Creator property
            //
            String creator = instance.getProperty (instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_CREATOR)).getValue
                ().toString ();

            AcceptLanguageList acceptLangs;
            Uint32 propIndex = instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String acceptLangsString;
                instance.getProperty(propIndex).getValue().get(
                    acceptLangsString);
                if (acceptLangsString.size())
                {
                    acceptLangs = LanguageParser::parseAcceptLanguageHeader(
                        acceptLangsString);
                }
            }
            ContentLanguageList contentLangs;
            propIndex = instance.findProperty
                (PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String contentLangsString;
                instance.getProperty(propIndex).getValue().get(
                    contentLangsString);
                if (contentLangsString.size())
                {
                    contentLangs = LanguageParser::parseContentLanguageHeader(
                        contentLangsString);
                }
            }

            //
            //  Send Create requests
            //
            Array<ProviderClassList> currentIndicationProviders;
            currentIndicationProviders.append (indicationProviders[s]);
            Array<ProviderClassList> acceptedProviders;
            acceptedProviders = _sendWaitCreateRequests
                (currentIndicationProviders,
                sourceNameSpace, requiredProperties, condition,
                query, queryLanguage, instance,
                acceptLangs,
                contentLangs,
                creator);

            if (acceptedProviders.size () > 0)
            {
                //
                //  Get Subscription entry from Active Subscriptions table
                //
                ActiveSubscriptionsTableEntry tableValue;
                if (_subscriptionTable->getSubscriptionEntry
                    (instance.getPath (), tableValue))
                {
                    //
                    //  Look for the provider in the subscription's list
                    //
                    Uint32 providerIndex =
                        _subscriptionTable->providerInList
                            (indicationProviders[s].provider, tableValue);
                    if (providerIndex != PEG_NOT_FOUND)
                    {
                        //
                        //  Provider is already in the list for this
                        //  subscription; add class to provider class list
                        //
                        for (Uint32 cn = 0;
                             cn < indicationProviders[s].classList.size ();
                             cn++)
                        {
                            _subscriptionTable->updateClasses
                                (instance.getPath (),
                                indicationProviders[s].provider,
                                indicationProviders[s].classList[cn]);
                        }
                    }
                    else
                    {
                        //
                        //  Provider is not yet in the list for this
                        //  subscription; add provider to the list
                        //
                        _subscriptionTable->updateProviders
                            (instance.getPath (), indicationProviders[s],
                            true);

                        //
                        //  NOTE: When a provider that was previously not
                        //  serving a subscription now serves the
                        //  subscription due to a provider being enabled, a
                        //  log message is sent, even if there were
                        //  previously other providers serving the
                        //  subscription
                        //

                        //
                        //  Get Subscription Filter Name and Handler Name
                        //
                        String logString2 = _getSubscriptionLogString
                            (subscriptions[s]);

                        //
                        //  Log a message for each subscription
                        //
                        Logger::put_l(Logger::STANDARD_LOG,
                            System::CIMSERVER, Logger::WARNING,
                            MessageLoaderParms(
                                _MSG_PROVIDER_NOW_SERVING_KEY,
                                _MSG_PROVIDER_NOW_SERVING,
                                logString1, logString2,
                                subscriptions[s].getPath().getNameSpace().
                                    getString()));
                    }
                }
            }  //  if any provider accepted the create subscription request
        }  //  for each matching subscription
    }  //  if any matching subscriptions

    //
    //  Send response
    //
    CIMResponseMessage * response = request->buildResponse ();
    response->cimException = cimException;
    _enqueueResponse (request, response);

    PEG_METHOD_EXIT ();
}

void IndicationService::_handleNotifyProviderFailRequest
    (Message * message)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_handleNotifyProviderFailRequest");

    CIMNotifyProviderFailRequestMessage* request =
        dynamic_cast<CIMNotifyProviderFailRequestMessage*>(message);
    PEGASUS_ASSERT(request != 0);

    String moduleName = request->moduleName;
    String userName = request->userName;

    //
    //  Determine providers in module that were serving active subscriptions
    //  and update the Active Subscriptions Table
    //
    Array<ActiveSubscriptionsTableEntry> providerModuleSubscriptions =
        _subscriptionTable->reflectProviderModuleFailure
            (moduleName, userName, _authenticationEnabled);

#ifdef PEGASUS_ENABLE_INDICATION_COUNT
    _providerIndicationCountTable.removeModuleEntries(moduleName);
#endif

    //
    //  FUTURE: Attempt to recreate the subscription state
    //

    //
    //  Send response
    //
    CIMResponseMessage * response = request->buildResponse ();
    CIMNotifyProviderFailResponseMessage * failResponse =
        (CIMNotifyProviderFailResponseMessage *) response;
    failResponse->numSubscriptionsAffected =
        providerModuleSubscriptions.size ();
    _enqueueResponse (request, response);
}

Boolean IndicationService::_canCreate (
    CIMInstance& instance,
    const CIMNamespaceName& nameSpace)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE, "IndicationService::_canCreate");

    // REVIEW: Derived classes of CIM_IndicationSubscription not
    // handled. It is reasonable for a user to derive from this
    // class and add extra properties.

    // REVIEW: how does the provider manager know to forward
    // requests to this service? Is it by class name? If so,
    // shouldn't the provider use an is-a operator on the new
    // class?

    //
    //  Validate that all properties in the instance are supported properties,
    //  and reject create if an unknown, unsupported property is found
    //
    _checkSupportedProperties (instance);

    //
    //  Check all required properties exist
    //  For a property that has a default value, if it does not exist or is
    //  null, add or set property with default value
    //  For a property that has a specified set of valid values, validate
    //
    if ((instance.getClassName ().equal (PEGASUS_CLASSNAME_INDSUBSCRIPTION)) ||
        (instance.getClassName ().equal
            (PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION)))
    {
        //
        //  Filter and Handler are key properties for Subscription
        //  No other properties are required
        //
        _checkRequiredProperty(
            instance,
            PEGASUS_PROPERTYNAME_FILTER,
            CIMTYPE_REFERENCE,
            true);
        _checkRequiredProperty(
            instance,
            PEGASUS_PROPERTYNAME_HANDLER,
            CIMTYPE_REFERENCE,
            true);

        //
        //  Get filter and handler property values
        //
        CIMProperty filterProperty = instance.getProperty
            (instance.findProperty (PEGASUS_PROPERTYNAME_FILTER));
        CIMValue filterValue = filterProperty.getValue ();
        CIMObjectPath filterPath;
        filterValue.get (filterPath);

        CIMProperty handlerProperty = instance.getProperty
            (instance.findProperty (PEGASUS_PROPERTYNAME_HANDLER));
        CIMValue handlerValue = handlerProperty.getValue ();
        CIMObjectPath handlerPath;
        handlerValue.get (handlerPath);

        //
        //  Currently, the Indication Service requires that a Subscription
        //  instance and the Filter and Handler instances to which it refers
        //  all be created on the same Host.
        //  Developers are recommended NOT to include Host in the
        //  Filter or Handler reference property values.
        //

        //
        //  If Host is included in a Filter or Handler reference property
        //  value, attempt to validate that it is correct.
        //  If Host cannot be validated, reject the create operation.
        //
        CIMObjectPath origFilterPath = filterPath;
        if (filterPath.getHost () != String::EMPTY)
        {
            if (!System::isLocalHost (filterPath.getHost()))
            {
                //
                //  Reject subscription creation
                //
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        _MSG_INVALID_VALUE_FOR_PROPERTY_KEY,
                        _MSG_INVALID_VALUE_FOR_PROPERTY,
                        origFilterPath.toString(),
                        PEGASUS_PROPERTYNAME_FILTER.getString()));
            }
        }

        CIMObjectPath origHandlerPath = handlerPath;
        if (handlerPath.getHost () != String::EMPTY)
        {
            if (!System::isLocalHost (handlerPath.getHost()))
            {
                //
                //  Reject subscription creation
                //
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        _MSG_INVALID_VALUE_FOR_PROPERTY_KEY,
                        _MSG_INVALID_VALUE_FOR_PROPERTY,
                        origHandlerPath.toString(),
                        PEGASUS_PROPERTYNAME_HANDLER.getString()));
            }
        }

        //
        //  Get Filter namespace - if not set in Filter reference property
        //  value, namespace is the namespace of the subscription
        //
        CIMNamespaceName filterNS = filterPath.getNameSpace ();
        if (filterNS.isNull ())
        {
            filterNS = nameSpace;
        }

        //
        //  Get Handler namespace - if not set in Handler reference property
        //  value, namespace is the namespace of the subscription
        //
        CIMNamespaceName handlerNS = handlerPath.getNameSpace();
        if (handlerNS.isNull())
        {
            handlerNS = nameSpace;
        }

        //
        //  Validate the Filter and Handler reference properties
        //  Ensure Filter and Handler instances can be retrieved from the
        //  repository
        //
        CIMInstance filterInstance =
            _subscriptionRepository->getInstance(filterNS, filterPath,
            true, false, false, CIMPropertyList());

        CIMInstance handlerInstance =
            _subscriptionRepository->getInstance(handlerNS, handlerPath,
            true, false, false, CIMPropertyList());

        //
        //  Set the key bindings in the subscription instance
        //
        Array<CIMKeyBinding> kb;
        kb.append(CIMKeyBinding(PEGASUS_PROPERTYNAME_FILTER, filterValue));
        kb.append(CIMKeyBinding(PEGASUS_PROPERTYNAME_HANDLER, handlerValue));

        CIMObjectPath instanceRef = instance.getPath ();
        instanceRef.setKeyBindings(kb);
        instanceRef.setNameSpace(nameSpace);
        instance.setPath(instanceRef);

        //
        //  Subscription State, Repeat Notification Policy, and On Fatal Error
        //  Policy properties each has a default value, a corresponding
        //  Other___ property, and a set of valid values
        //
        _checkPropertyWithOther(
            instance,
            PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE,
            _PROPERTY_OTHERSTATE,
            (Uint16) STATE_ENABLED,
            (Uint16) STATE_OTHER,
            _validStates,
            _supportedStates);

        _checkPropertyWithOther(
            instance,
            _PROPERTY_REPEATNOTIFICATIONPOLICY,
            _PROPERTY_OTHERREPEATNOTIFICATIONPOLICY,
            (Uint16) _POLICY_NONE,
            (Uint16) _POLICY_OTHER,
            _validRepeatPolicies,
            _supportedRepeatPolicies);

        _checkPropertyWithOther(
            instance,
            _PROPERTY_ONFATALERRORPOLICY,
            _PROPERTY_OTHERONFATALERRORPOLICY,
            (Uint16) _ERRORPOLICY_IGNORE,
            (Uint16) _ERRORPOLICY_OTHER,
            _validErrorPolicies,
            _supportedErrorPolicies);

        //
        //  For each remaining property, verify that if the property exists in
        //  the instance it is of the correct type
        //
        _checkProperty(instance, _PROPERTY_FAILURETRIGGERTIMEINTERVAL,
            CIMTYPE_UINT64);
        _checkProperty(instance, _PROPERTY_LASTCHANGE, CIMTYPE_DATETIME);
        _checkProperty(instance, _PROPERTY_DURATION, CIMTYPE_UINT64);
        _checkProperty(instance, _PROPERTY_STARTTIME, CIMTYPE_DATETIME);
        _checkProperty(instance, _PROPERTY_TIMEREMAINING, CIMTYPE_UINT64);
        _checkProperty(instance, _PROPERTY_REPEATNOTIFICATIONINTERVAL,
            CIMTYPE_UINT64);
        _checkProperty(instance, _PROPERTY_REPEATNOTIFICATIONGAP,
            CIMTYPE_UINT64);
        _checkProperty(instance, _PROPERTY_REPEATNOTIFICATIONCOUNT,
            CIMTYPE_UINT16);

        if (instance.getClassName().equal(
            PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION))
        {
            Array<String> textFormatParams;
            CIMValue textFormatParamsValue;
            CIMClass indicationClass;

            // get TextFormatParameters from instance
            Uint32 textFormatParamsPos =
            instance.findProperty(_PROPERTY_TEXTFORMATPARAMETERS);

            if (textFormatParamsPos != PEG_NOT_FOUND)
            {
                textFormatParamsValue = instance.getProperty(
                    textFormatParamsPos).getValue();

                if (!textFormatParamsValue.isNull())
                {
                    textFormatParamsValue.get(textFormatParams);
                }
            }

            // get indication class
            indicationClass = _getIndicationClass (instance);

            String textFormatStr;
            CIMValue textFormatValue;

            // get TextFormatStr from instance
            Uint32 textFormatPos =
            instance.findProperty(_PROPERTY_TEXTFORMAT);

            if (textFormatPos != PEG_NOT_FOUND)
            {
                textFormatValue = instance.getProperty(
                    textFormatPos).getValue();

#if defined(PEGASUS_ENABLE_SYSTEM_LOG_HANDLER) || \
    defined(PEGASUS_ENABLE_EMAIL_HANDLER)
                // if the value of textFormat is not null
                if (!(textFormatValue.isNull()) &&
                    (textFormatValue.getType() == CIMTYPE_STRING) &&
                    !(textFormatValue.isArray()))
                {
                    textFormatValue.get(textFormatStr);

                    // Validates the syntax and the provided type for the
                    // property TextFormat
                    IndicationFormatter::validateTextFormat (
                        textFormatStr, indicationClass,
                        textFormatParams);

                    // Validates the property names in TextFormatParameters
                    CIMNamespaceName sourceNameSpace;
                    String query;
                    String queryLanguage;
                    String filterName;
                    CIMPropertyList propertyList;

                    //  Get filter properties
                    _subscriptionRepository->getFilterProperties (instance,
                        query, sourceNameSpace, queryLanguage, filterName);

                    //  Build the query expression from the filter query
                    QueryExpression queryExpression = _getQueryExpression(query,
                        queryLanguage, sourceNameSpace);

                    // the select clause projection
                    propertyList = queryExpression.getPropertyList();

                    IndicationFormatter::validateTextFormatParameters(
                    propertyList, indicationClass, textFormatParams);
                }
#endif
            }
        }
    }
    else  // Filter or Handler
    {
        //
        //  Name, CreationClassName, SystemName, and SystemCreationClassName
        //  are key properties for Filter and Handler
        //  Name must exist
        //  If others do not exist, add and set to default
        //  If they exist but are NULL, set value to the default
        //  If they exist and are not NULL, validate the value
        //
        _checkRequiredProperty(
            instance,
            PEGASUS_PROPERTYNAME_NAME,
            CIMTYPE_STRING,
            true);

        _initOrValidateStringProperty(
            instance,
            PEGASUS_PROPERTYNAME_CREATIONCLASSNAME,
            instance.getClassName().getString());

        _initOrValidateStringProperty(
            instance,
            _PROPERTY_SYSTEMNAME,
            System::getFullyQualifiedHostName());

        _initOrValidateStringProperty(
            instance,
            _PROPERTY_SYSTEMCREATIONCLASSNAME,
            System::getSystemCreationClassName());

        if (instance.getClassName ().equal (PEGASUS_CLASSNAME_INDFILTER))
        {
            //
            //  Query and QueryLanguage properties are required for Filter
            //
            _checkRequiredProperty(
                instance,
                PEGASUS_PROPERTYNAME_QUERY,
                CIMTYPE_STRING,
                false);
            _checkRequiredProperty(
                instance,
                PEGASUS_PROPERTYNAME_QUERYLANGUAGE,
                CIMTYPE_STRING,
                false);

            //
            //  Validate the query language is supported
            //
            String queryLanguage;
            instance.getProperty(
                instance.findProperty(PEGASUS_PROPERTYNAME_QUERYLANGUAGE)).
                    getValue().get(queryLanguage);

#ifndef PEGASUS_ENABLE_CQL
            // Special code to block CQL, if CQL is disabled
            if ((queryLanguage == "CIM:CQL") || (queryLanguage == "DMTF:CQL"))
            {
                // CQL is not allowed in this case
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION(
                    CIM_ERR_NOT_SUPPORTED, queryLanguage);
            }
#endif

            //
            //  Default value for Source Namespace is the namespace of the
            //  Filter registration
            //
            CIMNamespaceName sourceNameSpace = CIMNamespaceName(
                _checkPropertyWithDefault(
                    instance,
                    _PROPERTY_SOURCENAMESPACE,
                    nameSpace.getString()));

            //
            //  Validate the query and indication class name
            //  An exception is thrown if the query is invalid or the class
            //  is not an indication class
            //
            String filterQuery = instance.getProperty (instance.findProperty
                (PEGASUS_PROPERTYNAME_QUERY)).getValue ().toString ();

            QueryExpression queryExpression;
            try
            {
                queryExpression = _getQueryExpression(
                    filterQuery, queryLanguage, sourceNameSpace);
            }
            catch (QueryLanguageInvalidException&)
            {
                // The filter query had an invalid language name.
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION(
                    CIM_ERR_NOT_SUPPORTED, queryLanguage);
            }

            CIMName indicationClassName = _getIndicationClassName
                (queryExpression, sourceNameSpace);

            //
            // Make sure that the FROM class exists in the repository.
            //
            CIMClass indicationClass = _subscriptionRepository->getClass
                (sourceNameSpace, indicationClassName,
                false, false, false, CIMPropertyList ());

            //
            // Validate all the properties in the SELECT statement exist
            // on their class context.
            //
            try
            {
              queryExpression.validate();
            }
            catch (QueryMissingPropertyException& qmp)
            {
              // A property does not exist on the class it is scoped to.
              PEG_METHOD_EXIT();
              throw PEGASUS_CIM_EXCEPTION
                (CIM_ERR_INVALID_PARAMETER, qmp.getMessage());
            }
            catch (QueryValidationException& qv)
            {
              // Received some other validation error.
              // This includes detecting an array property
              // is in the WHERE list for WQL.
              PEG_METHOD_EXIT();
              throw PEGASUS_CIM_EXCEPTION
                (CIM_ERR_NOT_SUPPORTED, qv.getMessage());
            }
        }

        //
        //  Currently only five subclasses of the Listener Destination
        //  class are supported -- further subclassing is not currently
        //  supported
        //
        else if ((instance.getClassName ().equal
                  (PEGASUS_CLASSNAME_INDHANDLER_CIMXML)) ||
                 (instance.getClassName ().equal
                  (PEGASUS_CLASSNAME_LSTNRDST_CIMXML)) ||
         (instance.getClassName ().equal
          (PEGASUS_CLASSNAME_LSTNRDST_SYSTEM_LOG)) ||
         (instance.getClassName ().equal
          (PEGASUS_CLASSNAME_LSTNRDST_EMAIL)) ||
                 (instance.getClassName ().equal
                  (PEGASUS_CLASSNAME_INDHANDLER_SNMP)))
        {
#ifndef PEGASUS_ENABLE_SYSTEM_LOG_HANDLER
            if (instance.getClassName ().equal
            (PEGASUS_CLASSNAME_LSTNRDST_SYSTEM_LOG))
            {
                //
                //  The System Log Handler is not enabled currently,
                //  this class is not currently served by the Indication Service
                //
                PEG_METHOD_EXIT ();

                throw PEGASUS_CIM_EXCEPTION_L (CIM_ERR_NOT_SUPPORTED,
                MessageLoaderParms(_MSG_CLASS_NOT_SERVED_KEY,
            _MSG_CLASS_NOT_SERVED));
        }
#endif

#if !defined(PEGASUS_ENABLE_EMAIL_HANDLER)

            if (instance.getClassName ().equal
            (PEGASUS_CLASSNAME_LSTNRDST_EMAIL))
            {
                //
                //  The Email Handler is not enabled currently,
                //  this class is not currently served by the Indication Service
                //
                PEG_METHOD_EXIT ();

                throw PEGASUS_CIM_EXCEPTION_L (CIM_ERR_NOT_SUPPORTED,
                MessageLoaderParms(_MSG_CLASS_NOT_SERVED_KEY,
                   _MSG_CLASS_NOT_SERVED));
        }
#endif
            _checkPropertyWithOther(
                instance,
                PEGASUS_PROPERTYNAME_PERSISTENCETYPE,
                _PROPERTY_OTHERPERSISTENCETYPE,
                (Uint16) PERSISTENCE_PERMANENT,
                (Uint16) PERSISTENCE_OTHER,
                _validPersistenceTypes,
                _supportedPersistenceTypes);

            //
            //  For remaining property, verify that if the property exists in
            //  the instance it is of the correct type
            //
            _checkProperty(instance, _PROPERTY_OWNER, CIMTYPE_STRING);

            if (instance.getClassName().equal(
                    PEGASUS_CLASSNAME_INDHANDLER_CIMXML) ||
                instance.getClassName().equal(
                    PEGASUS_CLASSNAME_LSTNRDST_CIMXML))
            {
                //
                //  Destination property is required for CIMXML
                //  Handler subclass
                //
                _checkRequiredProperty(
                    instance,
                    PEGASUS_PROPERTYNAME_LSTNRDST_DESTINATION,
                    CIMTYPE_STRING,
                    false);
            }

            if (instance.getClassName().equal
                (PEGASUS_CLASSNAME_INDHANDLER_SNMP))
            {
                //
                //  TargetHost property is required for SNMP
                //  Handler subclass
                //
                _checkRequiredProperty(
                    instance,
                    PEGASUS_PROPERTYNAME_LSTNRDST_TARGETHOST,
                    CIMTYPE_STRING,
                    false);

                //
                //  TargetHostFormat property is required for SNMP
                //  Handler subclass
                //
                _checkRequiredProperty(
                    instance,
                    _PROPERTY_TARGETHOSTFORMAT,
                    CIMTYPE_UINT16,
                    false);

                //
                //  SNMPVersion property is required for SNMP Handler
                //
                _checkRequiredProperty(
                    instance,
                    PEGASUS_PROPERTYNAME_SNMPVERSION,
                    CIMTYPE_UINT16,
                    false);

                // Currently, only SNMPv1 trap and SNMPv2C trap are supported,
                // verify if the value of SNMPVersion is one of them

                _checkValue(
                    instance,
                    PEGASUS_PROPERTYNAME_SNMPVERSION,
                    _validSNMPVersion,
                    _supportedSNMPVersion);

                //
                //  For each remaining property, verify that if the property
                //  exists in the instance it is of the correct type
                //
                _checkProperty(instance, _PROPERTY_PORTNUMBER, CIMTYPE_UINT32);
                _checkProperty(instance, _PROPERTY_SNMPSECURITYNAME,
                    CIMTYPE_STRING);
                _checkProperty(instance, _PROPERTY_SNMPENGINEID,
                    CIMTYPE_STRING);
            }

            if (instance.getClassName().equal
                (PEGASUS_CLASSNAME_LSTNRDST_EMAIL))
            {
                //
                //  MailTo property is required for Email
                //  Handler subclass
                //
                _checkRequiredProperty(
                    instance,
                    PEGASUS_PROPERTYNAME_LSTNRDST_MAILTO,
                    CIMTYPE_STRING,
                    false,
                    true);

                // get MailTo from handler instance
                Array<String> mailTo;
                instance.getProperty(instance.findProperty(
                    PEGASUS_PROPERTYNAME_LSTNRDST_MAILTO)).getValue().get(
                        mailTo);

                // Build mail address string
                String mailAddrStr;
                Uint32 mailAddrSize = mailTo.size();

                for (Uint32 i=0; i < mailAddrSize; i++)
                {
                    mailAddrStr.append(mailTo[i]);

                    if (i < (mailAddrSize - 1))
                    {
                        mailAddrStr.append(",");
                    }
                }


                //
                // Email address can not be an empty string
                //
                if (mailAddrStr == String::EMPTY)
                {
                    PEG_METHOD_EXIT();
                    throw PEGASUS_CIM_EXCEPTION_L(
                        CIM_ERR_FAILED,
                        MessageLoaderParms(
                            "IndicationService.IndicationService."
                                "_MSG_DO_NOT_HAVE_EMAIL_ADDRESS",
                            "Do not have an e-mail address."));
                }

                //
                //  MailSubject property is required for Email
                //  Handler subclass
                //
                _checkRequiredProperty(
                    instance,
                    PEGASUS_PROPERTYNAME_LSTNRDST_MAILSUBJECT,
                    CIMTYPE_STRING,
                    false);

                //
                //  For MailCc property, verify that if the property
                //  exists in the instance it is of the correct type
                //
                _checkProperty(
                    instance,
                    PEGASUS_PROPERTYNAME_LSTNRDST_MAILCC,
                    CIMTYPE_STRING,
                    true);
            }
        }

        else
        {
            //
            //  A class not currently served by the Indication Service
            //
            PEG_METHOD_EXIT();

            throw PEGASUS_CIM_EXCEPTION_L(
                CIM_ERR_NOT_SUPPORTED,
                MessageLoaderParms(
                    _MSG_CLASS_NOT_SERVED_KEY,
                    _MSG_CLASS_NOT_SERVED));
        }
    }

    PEG_METHOD_EXIT();
    return true;
}

void IndicationService::_checkRequiredProperty(
    CIMInstance& instance,
    const CIMName& propertyName,
    const CIMType expectedType,
    Boolean isKeyProperty,
    Boolean isArray)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_checkRequiredProperty");

    Boolean missingProperty = false;

    //
    //  Required property must exist in instance
    //
    if (instance.findProperty (propertyName) == PEG_NOT_FOUND)
    {
        missingProperty = true;
    }
    else
    {
        //
        //  Get the property
        //
        CIMProperty theProperty = instance.getProperty
            (instance.findProperty (propertyName));
        CIMValue theValue = theProperty.getValue ();

        //
        //  Required property must have a non-null value
        //
        if (theValue.isNull ())
        {
            missingProperty = true;
        }
        else
        {
            //
            //  Check that the property value is of the correct type
            //
            if ((theValue.getType () != expectedType) ||
                (theValue.isArray () != isArray))
            {
                if (theValue.isArray ())
                {
                    PEG_METHOD_EXIT();
                    throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_INVALID_PARAMETER,
                        MessageLoaderParms(
                            _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY_KEY,
                            _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY,
                            cimTypeToString(theValue.getType()),
                            propertyName.getString()));
                }
                else
                {
                    PEG_METHOD_EXIT();
                    throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_INVALID_PARAMETER,
                        MessageLoaderParms(
                            _MSG_INVALID_TYPE_FOR_PROPERTY_KEY,
                            _MSG_INVALID_TYPE_FOR_PROPERTY,
                            cimTypeToString(theValue.getType()),
                            propertyName.getString()));
                }
            }
        }
    }

    if (missingProperty)
    {
        if (isKeyProperty)
        {
            PEG_METHOD_EXIT();
            throw PEGASUS_CIM_EXCEPTION_L(
                CIM_ERR_INVALID_PARAMETER,
                MessageLoaderParms(
                    "IndicationService.IndicationService._MSG_KEY_PROPERTY",
                    "The key property $0 is missing.",
                    propertyName.getString()));
        }
        else
        {
            PEG_METHOD_EXIT();
            throw PEGASUS_CIM_EXCEPTION_L(
                CIM_ERR_INVALID_PARAMETER,
                MessageLoaderParms(
                    _MSG_PROPERTY_KEY,
                    _MSG_PROPERTY,
                    propertyName.getString()));
        }
    }

    PEG_METHOD_EXIT ();
}

void IndicationService::_checkPropertyWithOther (
    CIMInstance& instance,
    const CIMName& propertyName,
    const CIMName& otherPropertyName,
    const Uint16 defaultValue,
    const Uint16 otherValue,
    const Array<Uint16>& validValues,
    const Array<Uint16>& supportedValues)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_checkPropertyWithOther");

    Uint16 result = defaultValue;

    //
    //  If the property doesn't exist, add it with the default value
    //
    if (instance.findProperty (propertyName) == PEG_NOT_FOUND)
    {
        instance.addProperty (CIMProperty (propertyName,
            CIMValue (defaultValue)));
    }
    else
    {
        //
        //  Get the property
        //
        CIMProperty theProperty = instance.getProperty
            (instance.findProperty (propertyName));
        CIMValue theValue = theProperty.getValue ();

        //
        //  Check that the value is of the correct type
        //
        if ((theValue.getType () != CIMTYPE_UINT16) || (theValue.isArray ()))
        {
            String exceptionStr;
            if (theValue.isArray ())
            {
                MessageLoaderParms parms(
                    "IndicationService.IndicationService."
                        "_MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY",
                    "Invalid type array of $0 for property $1",
                    cimTypeToString(theValue.getType()),
                    propertyName.getString());

                exceptionStr.append(MessageLoader::getMessage(parms));
            }
            else
            {
                MessageLoaderParms parms(
                    "IndicationService.IndicationService."
                        "_MSG_INVALID_TYPE_FOR_PROPERTY",
                    "Invalid type $0 for property $1",
                    cimTypeToString(theValue.getType()),
                    propertyName.getString());

                exceptionStr.append(MessageLoader::getMessage(parms));
            }
            PEG_METHOD_EXIT ();
            throw PEGASUS_CIM_EXCEPTION (CIM_ERR_INVALID_PARAMETER,
                exceptionStr);
        }

        //
        //  If the value is null, set to the default value
        //
        if (theValue.isNull ())
        {
            theProperty.setValue (CIMValue (defaultValue));
        }
        else
        {
            theValue.get (result);

            //
            //  Validate the value
            //
            //  Note: Valid values are defined by the CIM Event Schema MOF
            //
            if (!Contains (validValues, result))
            {
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        _MSG_INVALID_VALUE_FOR_PROPERTY_KEY,
                        _MSG_INVALID_VALUE_FOR_PROPERTY,
                        theValue.toString(),
                        propertyName.getString()));
            }

            //
            //  Check for valid values that are not supported
            //
            //  Note: Supported values are a subset of the valid values
            //  Some valid values, as defined in the MOF, are not currently
            //  supported by the Pegasus IndicationService
            //
            if (!Contains(supportedValues, result))
            {
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_NOT_SUPPORTED,
                    MessageLoaderParms(
                        _MSG_UNSUPPORTED_VALUE_FOR_PROPERTY_KEY,
                        _MSG_UNSUPPORTED_VALUE_FOR_PROPERTY,
                        theValue.toString(),
                        propertyName.getString()));
            }
        }

        //
        //  If the value is Other, the Other
        //  property must exist, value must not be NULL and type must be String
        //
        if (result == otherValue)
        {
            if (instance.findProperty(otherPropertyName) == PEG_NOT_FOUND)
            {
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        _MSG_PROPERTY_KEY,
                        _MSG_PROPERTY,
                        otherPropertyName.getString()));
            }
            else
            {
                CIMProperty otherProperty = instance.getProperty
                    (instance.findProperty(otherPropertyName));
                CIMValue theOtherValue = otherProperty.getValue();
                if (theOtherValue.isNull())
                {
                    PEG_METHOD_EXIT();
                    throw PEGASUS_CIM_EXCEPTION_L(
                        CIM_ERR_INVALID_PARAMETER,
                        MessageLoaderParms(
                            _MSG_PROPERTY_KEY,
                            _MSG_PROPERTY,
                            otherPropertyName.getString()));
                }
                else if (theOtherValue.getType() != CIMTYPE_STRING)
                {
                    //
                    //  Property exists and is not null,
                    //  but is not of correct type
                    //
                    PEG_METHOD_EXIT();
                    throw PEGASUS_CIM_EXCEPTION_L(
                        CIM_ERR_INVALID_PARAMETER,
                        MessageLoaderParms(
                            _MSG_INVALID_TYPE_FOR_PROPERTY_KEY,
                            _MSG_INVALID_TYPE_FOR_PROPERTY,
                            cimTypeToString(theOtherValue.getType()),
                            otherPropertyName.getString()));
                }
            }
        }

        //
        //  If value is not Other, Other property must not exist
        //  or must be NULL
        //
        else if (instance.findProperty (otherPropertyName) != PEG_NOT_FOUND)
        {
            CIMProperty otherProperty = instance.getProperty(
                instance.findProperty(otherPropertyName));
            CIMValue theOtherValue = otherProperty.getValue();
            if (!theOtherValue.isNull())
            {
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        "IndicationService.IndicationService."
                            "_MSG_PROPERTY_PRESENT_BUT_VALUE_NOT",
                        "The $0 property is present, but the $1 value is "
                            "not $2.",
                        otherPropertyName.getString(),
                        propertyName.getString(),
                        CIMValue(otherValue).toString()));
            }
        }
    }

    PEG_METHOD_EXIT();
}

String IndicationService::_checkPropertyWithDefault(
    CIMInstance& instance,
    const CIMName& propertyName,
    const String& defaultValue)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_checkPropertyWithDefault");

    String result = defaultValue;

    //
    //  If the property doesn't exist, add it with the default value
    //
    if (instance.findProperty (propertyName) == PEG_NOT_FOUND)
    {
        instance.addProperty (CIMProperty (propertyName,
            CIMValue (defaultValue)));
    }
    else
    {
        //
        //  Get the property
        //
        CIMProperty theProperty = instance.getProperty
            (instance.findProperty (propertyName));
        CIMValue theValue = theProperty.getValue ();

        //
        //  If the value is null, set to the default value
        //
        if (theValue.isNull ())
        {
            theProperty.setValue (CIMValue (defaultValue));
        }
        else if ((theValue.getType () != CIMTYPE_STRING) ||
                 (theValue.isArray ()))
        {
            //
            //  Property exists and is not null,
            //  but is not of correct type
            //
            if (theValue.isArray ())
            {
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY_KEY,
                        _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY,
                        cimTypeToString(theValue.getType()),
                        propertyName.getString()));
            }
            else
            {
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        _MSG_INVALID_TYPE_FOR_PROPERTY_KEY,
                        _MSG_INVALID_TYPE_FOR_PROPERTY,
                        cimTypeToString(theValue.getType()),
                        propertyName.getString()));
            }
        }
        else
        {
            theValue.get (result);
        }
    }

    PEG_METHOD_EXIT ();
    return result;
}

String IndicationService::_initOrValidateStringProperty (
    CIMInstance& instance,
    const CIMName& propertyName,
    const String& defaultValue)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_initOrValidateStringProperty");

    String result = defaultValue;

    String propertyValue = _checkPropertyWithDefault (instance, propertyName,
        defaultValue);

    if (propertyValue != defaultValue)
    {
#ifdef PEGASUS_SNIA_EXTENSIONS
        // SNIA requires SystemName and SystemCreationClassName to be
        // overridden with the correct values.
        if ((propertyName == _PROPERTY_SYSTEMNAME) ||
            (propertyName == _PROPERTY_SYSTEMCREATIONCLASSNAME))
        {
            // The property must exist after _checkPropertyWithDefault is called
            CIMProperty p =
                instance.getProperty(instance.findProperty(propertyName));
            p.setValue(CIMValue(defaultValue));
            PEG_METHOD_EXIT();
            return result;
        }
#endif

        //
        //  Property value specified is invalid
        //
        PEG_METHOD_EXIT();
        throw PEGASUS_CIM_EXCEPTION_L(
            CIM_ERR_INVALID_PARAMETER,
            MessageLoaderParms(
                _MSG_INVALID_VALUE_FOR_PROPERTY_KEY,
                _MSG_INVALID_VALUE_FOR_PROPERTY,
                propertyValue,
                propertyName.getString()));
    }

    PEG_METHOD_EXIT ();
    return result;
}

void IndicationService::_checkProperty (
    CIMInstance& instance,
    const CIMName& propertyName,
    const CIMType expectedType,
    const Boolean isArray)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_checkProperty");

    //
    //  If the property exists, get it
    //
    Uint32 propPos = instance.findProperty (propertyName);
    if (propPos != PEG_NOT_FOUND)
    {
        CIMProperty theProperty = instance.getProperty (propPos);
        CIMValue theValue = theProperty.getValue ();

        //
        //  If the value is not null, check the type
        //
        if (!theValue.isNull ())
        {
            if ((theValue.getType () != expectedType) ||
                (theValue.isArray () != isArray))
            {
                //
                //  Property exists and is not null, but is not of correct type
                //
                if (theValue.isArray ())
                {
                    PEG_METHOD_EXIT();
                    throw PEGASUS_CIM_EXCEPTION_L(
                        CIM_ERR_INVALID_PARAMETER,
                        MessageLoaderParms(
                            _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY_KEY,
                            _MSG_INVALID_TYPE_ARRAY_OF_FOR_PROPERTY,
                            cimTypeToString(theValue.getType()),
                            propertyName.getString()));
                }
                else
                {
                    PEG_METHOD_EXIT();
                    throw PEGASUS_CIM_EXCEPTION_L(
                        CIM_ERR_INVALID_PARAMETER,
                        MessageLoaderParms(
                            _MSG_INVALID_TYPE_FOR_PROPERTY_KEY,
                            _MSG_INVALID_TYPE_FOR_PROPERTY,
                            cimTypeToString(theValue.getType()),
                            propertyName.getString()));
                }
            }
        }
    }

    PEG_METHOD_EXIT ();
}

void IndicationService::_checkSupportedProperties (
    const CIMInstance& instance)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_checkSupportedProperties");

    CIMName className = instance.getClassName ();
    Array<CIMName> emptyArray;
    Array<CIMName>& supportedProperties = emptyArray;

    //
    //  Get list of supported properties for the class
    //
    if (className.equal (PEGASUS_CLASSNAME_INDSUBSCRIPTION))
    {
        supportedProperties = _supportedSubscriptionProperties;
    }
    else if (className.equal (PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION))
    {
        supportedProperties = _supportedFormattedSubscriptionProperties;
    }
    else if (className.equal (PEGASUS_CLASSNAME_INDFILTER))
    {
        supportedProperties = _supportedFilterProperties;
    }
    else if (className.equal (PEGASUS_CLASSNAME_INDHANDLER_CIMXML))
    {
        supportedProperties = _supportedCIMXMLHandlerProperties;
    }
    else if (className.equal (PEGASUS_CLASSNAME_LSTNRDST_CIMXML))
    {
        supportedProperties = _supportedCIMXMLListenerDestinationProperties;
    }
    else if (className.equal (PEGASUS_CLASSNAME_INDHANDLER_SNMP))
    {
        supportedProperties = _supportedSNMPHandlerProperties;
    }
    else if (className.equal (PEGASUS_CLASSNAME_LSTNRDST_SYSTEM_LOG))
    {
        supportedProperties = _supportedSyslogListenerDestinationProperties;
    }
    else if (className.equal (PEGASUS_CLASSNAME_LSTNRDST_EMAIL))
    {
        supportedProperties = _supportedEmailListenerDestinationProperties;
    }
    else
    {
        PEGASUS_ASSERT (false);
    }

    //
    //  Check if each property in the instance is in the list of supported,
    //  known properties for its class
    //
    for (Uint32 i = 0; i < instance.getPropertyCount (); i++)
    {
        if (!ContainsCIMName (supportedProperties,
            instance.getProperty (i).getName ()))
        {
            //
            //  Throw an exception if an unknown, unsupported property was found
            //
            PEG_METHOD_EXIT ();
            throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_NOT_SUPPORTED,
                MessageLoaderParms(
                    "IndicationService.IndicationService."
                        "_MSG_PROPERTY_NOT_SUPPORTED",
                    "Property $0 is not supported in class $1",
                    instance.getProperty (i).getName ().getString (),
                    className.getString ()));
        }
    }

    PEG_METHOD_EXIT ();
}

void IndicationService::_checkValue (
    const CIMInstance& instance,
    const CIMName& propertyName,
    const Array<Uint16>& validValues,
    const Array<Uint16>& supportedValues)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_checkValue");

    Uint16 theValue;

    // get the property value
    Uint32 propPos = instance.findProperty (propertyName);
    if (propPos != PEG_NOT_FOUND)
    {
        CIMValue propertyValue = (instance.getProperty(propPos)).getValue();

        if (!(propertyValue.isNull()))
        {
            propertyValue.get(theValue);

            // Validate the value
            // Note: Valid values are defined by the PG Events MOF
            if (!Contains(validValues, theValue))
            {
                PEG_METHOD_EXIT();

                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_INVALID_PARAMETER,
                    MessageLoaderParms(
                        _MSG_INVALID_VALUE_FOR_PROPERTY_KEY,
                        _MSG_INVALID_VALUE_FOR_PROPERTY,
                        theValue,
                        propertyName.getString()));

            }

            // Check for valid values that are not supported
            // Note: Supported values are a subset of the valid values
            // Some valid values, as defined in the MOF, are not currently
            // supported
            if (!Contains(supportedValues, theValue))
            {
                PEG_METHOD_EXIT();
                throw PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_NOT_SUPPORTED,
                    MessageLoaderParms(
                        _MSG_UNSUPPORTED_VALUE_FOR_PROPERTY_KEY,
                        _MSG_UNSUPPORTED_VALUE_FOR_PROPERTY,
                        theValue,
                        propertyName.getString()));
            }
        }
    }

    PEG_METHOD_EXIT ();
}

Boolean IndicationService::_canModify (
    const CIMModifyInstanceRequestMessage * request,
    const CIMObjectPath& instanceReference,
    const CIMInstance& instance,
    CIMInstance& modifiedInstance)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE, "IndicationService::_canModify");

    //
    //  Currently, only modification allowed is of Subscription State
    //  property in Subscription class
    //
    if (!(instanceReference.getClassName ().equal
        (PEGASUS_CLASSNAME_INDSUBSCRIPTION)) &&
    !(instanceReference.getClassName ().equal
    (PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION)))
    {
        PEG_METHOD_EXIT ();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);
    }

    if (request->includeQualifiers)
    {
        PEG_METHOD_EXIT ();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);
    }

    //
    //  Request is invalid if property list is null, meaning all properties
    //  are to be updated
    //
    if (request->propertyList.isNull ())
    {
        PEG_METHOD_EXIT ();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);
    }

    //
    //  Request is invalid if more than one property is specified
    //
    else if (request->propertyList.size() > 1)
    {
        PEG_METHOD_EXIT();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);
    }

    //
    //  For request to be valid, zero or one property must be specified
    //  If one property specified, it must be Subscription State property
    //
    else if ((request->propertyList.size() == 1) &&
             (!request->propertyList[0].equal(
                   PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE)))
    {
        PEG_METHOD_EXIT();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);
    }

    //
    //  Check the SubscriptionState property in the modified instance
    //
    _checkPropertyWithOther(
        modifiedInstance,
        PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE,
        _PROPERTY_OTHERSTATE,
        (Uint16) STATE_ENABLED,
        (Uint16) STATE_OTHER,
        _validStates,
        _supportedStates);

    //
    //  Get creator from instance
    //
    String creator;
    if (!_getCreator (instance, creator))
    {
        //
        //  This instance from the repository is corrupted
        //
        PEG_METHOD_EXIT ();
        MessageLoaderParms parms(_MSG_INVALID_INSTANCES_KEY,
            _MSG_INVALID_INSTANCES);
        throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED, parms);
    }

    //
    //  Current user must be privileged user or instance Creator to modify
    //  NOTE: if authentication was not turned on when instance was created,
    //  instance creator will be String::EMPTY
    //  If creator is String::EMPTY, anyone may modify or delete the
    //  instance
    //
    String currentUser = ((IdentityContainer)request->operationContext.get
        (IdentityContainer :: NAME)).getUserName();
    if ((creator != String::EMPTY) &&
#ifndef PEGASUS_OS_ZOS
        (!System::isPrivilegedUser (currentUser)) &&
#endif
        (currentUser != creator))
    {
        PEG_METHOD_EXIT ();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_ACCESS_DENIED, String::EMPTY);
    }

    PEG_METHOD_EXIT ();
    return true;
}

Boolean IndicationService::_canDelete (
    const CIMObjectPath& instanceReference,
    const CIMNamespaceName& nameSpace,
    const String& currentUser)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE, "IndicationService::_canDelete");

    CIMName superClass;
    CIMName propName;

    //
    //  Get the instance to be deleted from the repository
    //
    CIMInstance instance;

    instance = _subscriptionRepository->getInstance
        (nameSpace, instanceReference);

    //
    //  Get creator from instance
    //
    String creator;
    if (!_getCreator (instance, creator))
    {
        //
        //  This instance from the repository is corrupted
        //  Allow the delete if a Privileged User
        //      (or authentication turned off),
        //  Otherwise disallow as access denied
        //
#ifndef PEGASUS_OS_ZOS
        if ((!System::isPrivilegedUser (currentUser)) &&
            (currentUser != String::EMPTY))
        {
            PEG_METHOD_EXIT ();
            throw PEGASUS_CIM_EXCEPTION (CIM_ERR_ACCESS_DENIED, String::EMPTY);
        }
#endif
    }

    //
    //  Current user must be privileged user or instance Creator to delete
    //  NOTE: if authentication was not turned on when instance was created,
    //  instance creator will be String::EMPTY
    //  If creator is String::EMPTY, anyone may modify or delete the
    //  instance
    //
    if ((creator != String::EMPTY) &&
#ifndef PEGASUS_OS_ZOS
        (!System::isPrivilegedUser (currentUser)) &&
#endif
        (currentUser != creator))
    {
        PEG_METHOD_EXIT ();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_ACCESS_DENIED, String::EMPTY);
    }

    //
    //  Get the class and superclass of the instance to be deleted
    //
    CIMClass refClass;

    refClass = _subscriptionRepository->getClass (nameSpace,
        instanceReference.getClassName (), true, true, false,
        CIMPropertyList ());
    superClass = refClass.getSuperClassName();

    //
    //  If the class is Filter or superclass is Handler or Listener Destination,
    //  check for subscription instances referring to the instance to be deleted
    //
    if ((superClass.equal (PEGASUS_CLASSNAME_INDHANDLER)) ||
        (superClass.equal (PEGASUS_CLASSNAME_LSTNRDST)) ||
        (instanceReference.getClassName().equal (PEGASUS_CLASSNAME_INDFILTER)))
    {
        if (instanceReference.getClassName ().equal
               (PEGASUS_CLASSNAME_INDFILTER))
        {
            propName = PEGASUS_PROPERTYNAME_FILTER;
        }
        else if ((superClass.equal (PEGASUS_CLASSNAME_INDHANDLER)) ||
            (superClass.equal (PEGASUS_CLASSNAME_LSTNRDST)))
        {
            propName = PEGASUS_PROPERTYNAME_HANDLER;

            //
            //  If deleting transient handler, first delete any referencing
            //  subscriptions
            //
            if (_subscriptionRepository->isTransient (nameSpace,
                instanceReference))
            {
                _deleteReferencingSubscriptions (nameSpace, propName,
                    instanceReference);
                PEG_METHOD_EXIT ();
                return true;
            }
        }

        //
        //  Get all the subscriptions from the repository
        //
        Array<CIMInstance> subscriptions =
            _subscriptionRepository->getAllSubscriptions ();

        CIMValue propValue;

        //
        //  Check each subscription for a reference to the instance to be
        //  deleted
        //
        for (Uint32 i = 0; i < subscriptions.size(); i++)
        {
            //
            //  Get the subscription Filter or Handler property value
            //
            propValue = subscriptions[i].getProperty
                (subscriptions[i].findProperty
                (propName)).getValue();

            CIMObjectPath ref;
            propValue.get (ref);

            //
            //  If the Filter or Handler reference property value includes
            //  namespace, check if it is the namespace of the Filter or Handler
            //  being deleted.
            //  If the Filter or Handler reference property value does not
            //  include namespace, check if the current subscription namespace
            //  is the namespace of the Filter or Handler being deleted.
            //
            CIMNamespaceName instanceNS = ref.getNameSpace ();
            if (((instanceNS.isNull ()) &&
                (subscriptions[i].getPath ().getNameSpace () == nameSpace))
                || (instanceNS == nameSpace))
            {

                //
                //  Remove Host and Namespace from reference property value, if
                //  present, before comparing
                //
                CIMObjectPath path ("", CIMNamespaceName (),
                    ref.getClassName (), ref.getKeyBindings ());

                //
                //  Remove Host and Namespace from reference of instance to be
                //  deleted, if present, before comparing
                //
                CIMObjectPath iref ("", CIMNamespaceName (),
                    instanceReference.getClassName (),
                    instanceReference.getKeyBindings ());

                //
                //  If the current subscription Filter or Handler is the
                //  instance to be deleted, it may not be deleted
                //
                if (iref == path)
                {
                    PEG_METHOD_EXIT ();
                    throw PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED,
                        MessageLoaderParms(
                            "IndicationService.IndicationService."
                                "_MSG_REFERENCED",
                            "A filter or handler referenced by a subscription "
                                "cannot be deleted."));
                }
            }
        }
    }

    PEG_METHOD_EXIT ();
    return true;
}

Array<CIMInstance> IndicationService::_getMatchingSubscriptions (
    const CIMName& supportedClass,
    const Array<CIMNamespaceName> nameSpaces,
    const CIMPropertyList& supportedProperties,
    const Boolean checkProvider,
    const CIMInstance& provider)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_getMatchingSubscriptions");

    Array<CIMInstance> matchingSubscriptions;
    Array<CIMInstance> subscriptions;

    subscriptions = _subscriptionTable->getMatchingSubscriptions
        (supportedClass, nameSpaces, checkProvider, provider);

    for (Uint32 i = 0; i < subscriptions.size (); i++)
    {
        Boolean match = true;

        //
        //  If supported properties is null (all properties)
        //  the subscription can be supported
        //
        if (!supportedProperties.isNull ())
        {
            String filterQuery;
            String queryLanguage;
            CIMName indicationClassName;
            CIMNamespaceName sourceNameSpace;
            CIMPropertyList propertyList;
            String filterName;

            try
            {
                //
                //  Get filter properties
                //
                _subscriptionRepository->getFilterProperties
                    (subscriptions[i], filterQuery, sourceNameSpace,
                     queryLanguage, filterName);

                QueryExpression queryExpr = _getQueryExpression(
                    filterQuery, queryLanguage, sourceNameSpace);

                // Get the class paths in the FROM list
                // Since neither WQL nor CQL support joins, so we can
                // assume one class path.
                indicationClassName = 
                    queryExpr.getClassPathList()[0].getClassName();

                if (!_subscriptionRepository->validateIndicationClassName(
                    indicationClassName, sourceNameSpace))
                {
                    // Invalid FROM class, skip the subscription
                    continue;
                }

                //
                //  Get required property list from filter query (WHERE clause)
                //
                //  Note that the supportedClass is passed in,
                //  not the indicationClassName.
                //  The supportedClass is the class of the indication
                //  instance, while the indicationClassName is the FROM class.
                //  This is needed because CQL can have class scoping operators
                //  on properties that may not be the same class
                //  as the FROM class.  The required properties
                //  for an indication are based on its class,
                //  not the FROM class.
                //
                //  Also note that for CQL, this does not return
                //  required embedded object properties.
                propertyList = _getPropertyList (queryExpr,
                                             sourceNameSpace,
                                             supportedClass);

                //
                //  If the subscription requires all properties,
                //  but supported property list does not include all
                //  properties, the subscription cannot be supported
                //
                if (propertyList.isNull ())
                {
                    //
                    //  Current subscription does not match
                    //  Continue to next subscription in list
                    //
                    continue;
                }
                else
                {
                    //
                    //  Compare subscription required property list
                    //  with supported property list
                    //
                    for (Uint32 j = 0;
                         j < propertyList.size () && match;
                         j++)
                    {
                        if (!ContainsCIMName
                            (supportedProperties.getPropertyNameArray(),
                            propertyList[j]))
                        {
                            match = false;
                            break;
                        }
                    }
                }
            }
            catch(const Exception & e)
            {
                // This subscription is invalid
                // skip it
               PEG_TRACE ((TRC_DISCARDED_DATA, Tracer::LEVEL2,
                    "Exception caught trying to verify required properties"
                    " in a subscription are all contained in the list of"
                    " supported indication properties: %s",
                    (const char *) e.getMessage ().getCString())); 
                continue;
            }
            catch(const exception & e)
            {
                // This subscription is invalid
                // skip it
               PEG_TRACE ((TRC_DISCARDED_DATA, Tracer::LEVEL2,
                    "Exception caught trying to verify required properties"
                    " in a subscription are all contained in the list of"
                    " supported indication properties: %s", e.what ())); 
                continue;
            }
            catch(...)
            {
                // This subscription is invalid
                // skip it
                PEG_TRACE_CSTRING (TRC_DISCARDED_DATA, Tracer::LEVEL2,
                    "Unknown exception caught trying to verify "
                    "required properties in a subscription are all contained "
                    "in the list of supported indication properties."); 
                continue;
            }
        }

        if (match)
        {
            //
            //  Add current subscription to list
            //
            matchingSubscriptions.append (subscriptions[i]);
        }
    }

    PEG_METHOD_EXIT ();
    return matchingSubscriptions;
}

void IndicationService::_getModifiedSubscriptions (
    const CIMName& supportedClass,
    const Array<CIMNamespaceName>& newNameSpaces,
    const Array<CIMNamespaceName>& oldNameSpaces,
    const CIMPropertyList& newProperties,
    const CIMPropertyList& oldProperties,
    Array<CIMInstance>& newSubscriptions,
    Array<CIMInstance>& formerSubscriptions)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_getModifiedSubscriptions");

    Array<CIMInstance> newList;
    Array<CIMInstance> formerList;
    Array<CIMInstance> bothList;

    newSubscriptions.clear ();
    formerSubscriptions.clear ();

    //
    //  For each newly supported namespace, lookup to retrieve list of
    //  subscriptions for the indication class-source namespace pair
    //
    newList = _subscriptionTable->getMatchingSubscriptions
        (supportedClass, newNameSpaces);

    //
    //  For each formerly supported namespace, lookup to retrieve list of
    //  subscriptions for the indication class-source namespace pair
    //
    formerList = _subscriptionTable->getMatchingSubscriptions
        (supportedClass, oldNameSpaces);

    //
    //  Find subscriptions that appear in both lists, and move them to a third
    //  list
    //
    Sint8 found;
    for (Uint32 p = 0; p < newList.size (); p++)
    {
        found = -1;
        for (Uint32 q = 0; q < formerList.size (); q++)
        {
            if (newList[p].identical (formerList[q]))
            {
                found = q;
                bothList.append (newList[p]);
                break;
            }
        }
        if (found >= 0)
        {
            newList.remove (p);
            p--;
            formerList.remove (found);
        }
    }

    //
    //  For indicationClassName-sourceNamespace pair that is now supported, but
    //  previously was not, add to list of newly supported subscriptions if
    //  required properties are now supported
    //
    for (Uint32 n = 0; n < newList.size (); n++)
    {
        String filterQuery;
        String queryLanguage;
        CIMName indicationClassName;
        CIMNamespaceName sourceNameSpace;
        CIMPropertyList requiredProperties;
        String filterName;

        //
        //  Get filter properties
        //
        _subscriptionRepository->getFilterProperties (newList[n], filterQuery,
            sourceNameSpace, queryLanguage, filterName);
        QueryExpression queryExpression = _getQueryExpression(
            filterQuery, queryLanguage, sourceNameSpace);

        //
        //  Get indication class name from filter query (FROM clause)
        //
        indicationClassName = _getIndicationClassName (queryExpression,
            sourceNameSpace);

        //
        //  Get required property list from filter query (WHERE clause)
        //
        //  Note: the supportedClass is passed to _getPropertyList
        //  rather than the FROM class because CQL could have
        //  class scoping operators that scope properties to
        //  specific subclasses of the FROM.
        //
        requiredProperties = _getPropertyList (queryExpression,
            sourceNameSpace, supportedClass);

        //
        //  Check if required properties are now supported
        //
        if (_inPropertyList (requiredProperties, newProperties))
        {
            newSubscriptions.append (newList[n]);
        }
    }

    //
    //  For indicationClassName-sourceNamespace pair that was previously
    //  supported, but now is not, add to list of formerly supported
    //  subscriptions
    //
    for (Uint32 f = 0; f < formerList.size (); f++)
    {
        formerSubscriptions.append (formerList[f]);
    }

    //
    //  For indicationClassName-sourceNamespace pair that is now supported,
    //  and was also previously supported, add to appropriate list, based on
    //  required properties
    //
    for (Uint32 b = 0; b < bothList.size (); b++)
    {
        String filterQuery;
        String queryLanguage;
        CIMName indicationClassName;
        CIMNamespaceName sourceNameSpace;
        CIMPropertyList requiredProperties;
        Boolean newMatch = false;
        Boolean formerMatch = false;
        String filterName;

        //
        //  Get filter properties
        //
        _subscriptionRepository->getFilterProperties (bothList[b], filterQuery,
            sourceNameSpace, queryLanguage, filterName);
        QueryExpression queryExpression = _getQueryExpression(
            filterQuery, queryLanguage, sourceNameSpace);

        //
        //  Get indication class name from filter query (FROM clause)
        //
        indicationClassName = _getIndicationClassName (queryExpression,
            sourceNameSpace);

        //
        //  Get required property list from filter query (WHERE clause)
        //
        //  Note: the supportedClass is passed to _getPropertyList
        //  rather than the FROM class because CQL could have
        //  class scoping operators that scope properties to
        //  specific subclasses of the FROM.
        //
        requiredProperties = _getPropertyList (queryExpression,
            sourceNameSpace, supportedClass);

        //
        //  Check required properties
        //
        newMatch = _inPropertyList (requiredProperties,
            newProperties);
        formerMatch = _inPropertyList (requiredProperties,
            oldProperties);

        //
        //  Add current subscription to appropriate list
        //
        if (newMatch && !formerMatch)
        {
            newSubscriptions.append (bothList[b]);
        }
        else if (!newMatch && formerMatch)
        {
            formerSubscriptions.append (bothList[b]);
        }
    }

    PEG_METHOD_EXIT ();
}

Boolean IndicationService::_inPropertyList (
    const CIMPropertyList& requiredProperties,
    const CIMPropertyList& supportedProperties)
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_inPropertyList");

    //
    //  If property list is null (all properties)
    //  all the required properties are supported
    //
    if (supportedProperties.isNull ())
    {
        PEG_METHOD_EXIT();
        return true;
    }
    else
    {
        //
        //  If the subscription requires all properties,
        //  but property list does not include all
        //  properties, the required properties cannot be supported
        //
        if (requiredProperties.isNull ())
        {
            PEG_METHOD_EXIT();
            return false;
        }
        else
        {
            //
            //  Compare required property list
            //  with property list
            //
            for (Uint32 i = 0; i < requiredProperties.size (); i++)
            {
                if (!ContainsCIMName
                    (supportedProperties.getPropertyNameArray (),
                    requiredProperties[i]))
                {
                    PEG_METHOD_EXIT();
                    return false;
                }
            }
        }
    }

    PEG_METHOD_EXIT ();
    return true;
}

QueryExpression IndicationService::_getQueryExpression(
    const String& filterQuery,
    const String& queryLanguage,
    const CIMNamespaceName& ns) const
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getQueryExpression");

    try
    {
        RepositoryQueryContext ctx(ns, _cimRepository);
        QueryExpression queryExpression(queryLanguage, filterQuery, ctx);
        PEG_METHOD_EXIT();
        return queryExpression;
    }
    catch (QueryParseException& qpe)
    {
        String exceptionStr = qpe.getMessage();

        PEG_METHOD_EXIT();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_INVALID_PARAMETER, exceptionStr);
    }
    catch (ParseError& pe)
    {
        String exceptionStr = pe.getMessage();

        PEG_METHOD_EXIT();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_INVALID_PARAMETER, exceptionStr);
    }
    catch (MissingNullTerminator& mnt)
    {
        String exceptionStr = mnt.getMessage();

        PEG_METHOD_EXIT();
        throw PEGASUS_CIM_EXCEPTION(CIM_ERR_INVALID_PARAMETER, exceptionStr);
    }
}

CIMName IndicationService::_getIndicationClassName (
    const QueryExpression& queryExpression,
    const CIMNamespaceName& nameSpaceName) const
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_getIndicationClassName");

    CIMName indicationClassName;
    Array<CIMName> indicationSubclasses;

    // Get the class paths in the FROM list.
    // Note: neither WQL nor CQL support joins, so we can
    // assume one class path.
    // Note: neither WQL not CQL support wbem-uri for class paths,
    // so we can ignore the parts of the path before the class name.
    Array<CIMObjectPath> fromPaths = queryExpression.getClassPathList();
    indicationClassName = fromPaths[0].getClassName();

    //
    //  Validate that class is an Indication class
    //  The Indication Qualifier should exist and have the value True
    //
    Boolean validClass = _subscriptionRepository->validateIndicationClassName
        (indicationClassName, nameSpaceName);

    if (!validClass)
    {
        PEG_METHOD_EXIT();
        throw PEGASUS_CIM_EXCEPTION_L(
            CIM_ERR_INVALID_PARAMETER,
            MessageLoaderParms(
                "IndicationService.IndicationService."
                    "_MSG_INVALID_CLASSNAME_IN_FROM_PROPERTY",
                "The Indication class name $0 is not valid in the FROM clause "
                    "of $1 $2 property.",
                indicationClassName.getString(),
                PEGASUS_CLASSNAME_INDFILTER.getString(),
                PEGASUS_PROPERTYNAME_QUERY.getString()));
    }

    PEG_METHOD_EXIT ();
    return indicationClassName;
}

Array<ProviderClassList> IndicationService::_getIndicationProviders (
    const QueryExpression& queryExpression,
    const CIMNamespaceName& nameSpace,
    const CIMName& indicationClassName,
    const Array<CIMName>& indicationSubclasses) const
{
    PEG_METHOD_ENTER (TRC_INDICATION_SERVICE,
        "IndicationService::_getIndicationProviders");

    ProviderClassList provider;
    Array<ProviderClassList> indicationProviders;
    Array<CIMInstance> providerInstances;
    Array<CIMInstance> providerModuleInstances;

    CIMPropertyList requiredPropertyList;

    //
    //  For each indication subclass, get providers
    //
    for (Uint32 i = 0, n = indicationSubclasses.size (); i < n; i++)
    {
        //  Get required property list from filter query (WHERE clause)
        //  from this indication subclass
        //
        requiredPropertyList = _getPropertyList (queryExpression,
                                                 nameSpace,
                                                 indicationSubclasses[i]);

        //
        //  Get providers that can serve the subscription
        //
        providerInstances.clear ();
        providerModuleInstances.clear ();
        if (_providerRegManager->getIndicationProviders
                (nameSpace,
                 indicationSubclasses[i],
                 requiredPropertyList,
                 providerInstances,
                 providerModuleInstances))
        {
            PEGASUS_ASSERT (providerInstances.size () ==
                            providerModuleInstances.size ());

            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
                "%u indication provider(s) found for class %s",
                providerInstances.size (),
                (const char *)
                indicationSubclasses[i].getString ().getCString ()));

            //
            //  Merge into list of ProviderClassList structs
            //
            for (Uint32 j = 0, numI = providerInstances.size (); j < numI; j++)
            {
                provider.classList.clear ();
                Boolean duplicate = false;

                //
                //  See if indication provider is already in list
                //
                for (Uint32 k = 0, numP = indicationProviders.size ();
                     k < numP && !duplicate; k++)
                {
                    if ((providerInstances[j].getPath ().identical
                        (indicationProviders[k].provider.getPath ())) &&
                        (providerModuleInstances[j].getPath ().identical
                        (indicationProviders[k].providerModule.getPath ())))
                    {
                        //
                        //  Indication provider is already in list
                        //  Add subclass to provider's class list
                        //
                        indicationProviders[k].classList.append
                            (indicationSubclasses[i]);
                        duplicate = true;
                    }
                }

                if (!duplicate)
                {
                    //
                    //  Current provider is not yet in list
                    //  Create new list entry
                    //
                    provider.provider = providerInstances[j];
                    provider.providerModule = providerModuleInstances[j];
                    provider.classList.append (indicationSubclasses[i]);
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
                    String remoteInformation;
                    Boolean isRemote = _cimRepository->isRemoteNameSpace(
                        nameSpace, remoteInformation);
                    provider.isRemoteNameSpace = isRemote;
                    provider.remoteInfo = remoteInformation;
#endif
                    indicationProviders.append(provider);
                }
            }  // for each indication provider instance
        }  // if any providers
    }  // for each indication subclass

    PEG_METHOD_EXIT();
    return indicationProviders;
}

CIMPropertyList IndicationService::_getPropertyList(
    const QueryExpression& queryExpression,
    const CIMNamespaceName& nameSpaceName,
    const CIMName& indicationClassName) const
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getPropertyList");

    CIMPropertyList propertyList;

    //  Get all the properties referenced in the condition (WHERE clause)
    //  Note: for CQL, this only returns the properties directly on the
    //  class name passed in, not any properties on embedded objects.
    //
    try
    {
      CIMObjectPath classPath(String::EMPTY,
                               nameSpaceName,
                               indicationClassName);
      propertyList = queryExpression.getWherePropertyList(classPath);
    }
    catch (QueryException& qe)
    {
      // The class path was not the FROM class, or a subclass
      // of the FROM class.
      String exceptionStr = qe.getMessage();

      PEG_METHOD_EXIT();
      throw PEGASUS_CIM_EXCEPTION(CIM_ERR_FAILED, exceptionStr);
    }

    if (propertyList.isNull())
    {
        //
        //  Return null property list for all properties
        //
        PEG_METHOD_EXIT();
        return propertyList;
    }
    else
    {
        Array<CIMName> propertyArray;

        //  Get the property names
        //
        propertyArray = propertyList.getPropertyNameArray();

        Array<CIMName> indicationClassProperties;
        PEG_METHOD_EXIT();
        return _checkPropertyList(propertyArray, nameSpaceName,
            indicationClassName, indicationClassProperties);
    }
}

CIMPropertyList IndicationService::_checkPropertyList(
    const Array<CIMName>& propertyList,
    const CIMNamespaceName& nameSpaceName,
    const CIMName& indicationClassName,
    Array<CIMName>& indicationClassProperties) const
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_checkPropertyList");

    //
    //  Check if list includes all properties in class
    //  If so, must be set to NULL
    //
    CIMClass indicationClass;

    //
    //  Get the indication class object from the repository
    //  Specify localOnly=false because superclass properties are needed
    //  Specify includeQualifiers=false because qualifiers are not needed
    //
    indicationClass = _subscriptionRepository->getClass(
        nameSpaceName, indicationClassName, false, false, false,
        CIMPropertyList());

    Boolean allProperties = true;
    for (Uint32 i = 0; i < indicationClass.getPropertyCount(); i++)
    {
        indicationClassProperties.append(
            indicationClass.getProperty(i).getName());
        if (!ContainsCIMName(propertyList,
            indicationClass.getProperty(i).getName()))
        {
            allProperties = false;
        }
    }

    if (allProperties)
    {
        //
        //  Return NULL CIMPropertyList
        //
        PEG_METHOD_EXIT();
        return CIMPropertyList();
    }
    else
    {
        PEG_METHOD_EXIT();
        return CIMPropertyList(propertyList);
    }
}

String IndicationService::_getCondition(
    const String& filterQuery) const
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getCondition");

    String condition;

    //
    //  Get condition substring from filter query
    //
    if (filterQuery.find(_QUERY_WHERE) != PEG_NOT_FOUND)
    {
        condition = filterQuery.subString(filterQuery.find(_QUERY_WHERE) + 6);
    }

    PEG_METHOD_EXIT();
    return condition;
}

void IndicationService::_deleteReferencingSubscriptions(
    const CIMNamespaceName& nameSpace,
    const CIMName& referenceProperty,
    const CIMObjectPath& handler)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_deleteReferencingSubscriptions");

    Array<CIMInstance> deletedSubscriptions;

    //
    //  Delete referencing subscriptions from the repository
    //
    deletedSubscriptions =
        _subscriptionRepository->deleteReferencingSubscriptions(
            nameSpace, referenceProperty, handler);

    //
    //  Send delete request to each provider for each deleted subscription
    //
    for (Uint32 i = 0; i < deletedSubscriptions.size(); i++)
    {
        Array<ProviderClassList> indicationProviders;
        Array<CIMName> indicationSubclasses;
        CIMNamespaceName sourceNamespaceName;

        indicationProviders = _getDeleteParams(deletedSubscriptions[i],
            indicationSubclasses, sourceNamespaceName);

        //
        //  Send Delete requests
        //
        //  NOTE: These Delete requests are not associated with a user
        //  request, so there is no associated authType or userName
        //  The Creator from the subscription instance is used for userName,
        //  and authType is not set
        //
        CIMInstance instance = deletedSubscriptions[i];
        String creator;
        _getCreator(instance, creator);

// l10n start
        AcceptLanguageList acceptLangs;
        Uint32 propIndex = instance.findProperty(
            PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
        if (propIndex != PEG_NOT_FOUND)
        {
            String acceptLangsString;
            instance.getProperty(propIndex).getValue().get(
                acceptLangsString);
            if (acceptLangsString.size())
            {
                acceptLangs = LanguageParser::parseAcceptLanguageHeader(
                    acceptLangsString);
            }
        }
        ContentLanguageList contentLangs;
        propIndex = instance.findProperty(
            PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
        if (propIndex != PEG_NOT_FOUND)
        {
            String contentLangsString;
            instance.getProperty(propIndex).getValue().get(
                contentLangsString);
            if (contentLangsString.size())
            {
                contentLangs = LanguageParser::parseContentLanguageHeader(
                    contentLangsString);
            }
        }
// l10n end

        _sendAsyncDeleteRequests(
            indicationProviders,
            sourceNamespaceName,
            deletedSubscriptions[i],
            acceptLangs,
            contentLangs,
            0,  // no request
            indicationSubclasses,
            creator);
    }

    PEG_METHOD_EXIT();
}

Boolean IndicationService::_isExpired(
    const CIMInstance& instance) const
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE, "IndicationService::_isExpired");

    Boolean isExpired = true;
    Uint64 timeRemaining = 0;

    //
    //  Get time remaining, if subscription has a duration
    //
    if (_getTimeRemaining(instance, timeRemaining))
    {
        if (timeRemaining > 0)
        {
            isExpired = false;
        }
    }
    else
    {
        //
        //  If there is no duration, the subscription has no expiration date
        //
        isExpired = false;
    }

    PEG_METHOD_EXIT();
    return isExpired;
}

void IndicationService::_deleteExpiredSubscription(
    CIMObjectPath& subscription)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_deleteExpiredSubscription");

    CIMInstance subscriptionInstance;

    //
    //  Delete instance from repository
    //
    subscriptionInstance =
        _subscriptionRepository->deleteSubscription(subscription);

    //
    //  If a valid instance object was returned, the subscription was
    //  successfully deleted
    //
    if (!subscriptionInstance.isUninitialized())
    {
        //
        //  If subscription was active, send delete requests to providers
        //  and update hash tables
        //
        Uint16 subscriptionState;
        CIMValue subscriptionStateValue;
        subscriptionStateValue = subscriptionInstance.getProperty(
            subscriptionInstance.findProperty(
                PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE)).getValue();
        subscriptionStateValue.get(subscriptionState);

        if ((subscriptionState == STATE_ENABLED) ||
            (subscriptionState == STATE_ENABLEDDEGRADED))
        {
            Array<ProviderClassList> indicationProviders;
            Array<CIMName> indicationSubclasses;
            CIMNamespaceName sourceNamespaceName;

            subscriptionInstance.setPath(subscription);

            indicationProviders = _getDeleteParams(subscriptionInstance,
                indicationSubclasses, sourceNamespaceName);

            //
            //  Send Delete requests
            //
            //  NOTE: These Delete requests are not associated with a user
            //  request, so there is no associated authType or userName
            //  The Creator from the subscription instance is used for userName,
            //  and authType is not set
            //
            String creator;
            _getCreator(subscriptionInstance, creator);

            //
            // Get the language tags that were saved with the subscription
            // instance
            //
            AcceptLanguageList acceptLangs;
            Uint32 propIndex = subscriptionInstance.findProperty(
                PEGASUS_PROPERTYNAME_INDSUB_ACCEPTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String acceptLangsString;
                subscriptionInstance.getProperty(propIndex).getValue().get(
                    acceptLangsString);
                if (acceptLangsString.size())
                {
                    acceptLangs = LanguageParser::parseAcceptLanguageHeader(
                        acceptLangsString);
                }
            }
            ContentLanguageList contentLangs;
            propIndex = subscriptionInstance.findProperty(
                PEGASUS_PROPERTYNAME_INDSUB_CONTENTLANGS);
            if (propIndex != PEG_NOT_FOUND)
            {
                String contentLangsString;
                subscriptionInstance.getProperty(propIndex).getValue().get(
                    contentLangsString);
                if (contentLangsString.size())
                {
                    contentLangs = LanguageParser::parseContentLanguageHeader(
                        contentLangsString);
                }
            }

            subscriptionInstance.setPath(subscription);
            _sendAsyncDeleteRequests(indicationProviders,
                sourceNamespaceName, subscriptionInstance,
                acceptLangs,
                contentLangs,
                0, // no request
                indicationSubclasses,
                creator);
        }
    }
    else
    {
        //
        //  The subscription may have already been deleted by another thread
        //
    }

    PEG_METHOD_EXIT();
}

Boolean IndicationService::_getTimeRemaining(
    const CIMInstance& instance,
    Uint64& timeRemaining) const
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getTimeRemaining");

    Boolean hasDuration = true;
    timeRemaining = 0;

    //
    //  Calculate time remaining from subscription
    //  start time, subscription duration, and current date time
    //

    //
    //  NOTE: It is assumed that the instance passed to this method is a
    //  subscription instance, and that the Start Time property exists
    //  and has a value
    //

    //
    //  Get Subscription Start Time
    //
    CIMValue startTimeValue;
    CIMDateTime startTime;
    Uint32 startTimeIndex = instance.findProperty(_PROPERTY_STARTTIME);
    PEGASUS_ASSERT(startTimeIndex != PEG_NOT_FOUND);
    startTimeValue = instance.getProperty(startTimeIndex).getValue();
    PEGASUS_ASSERT(!(startTimeValue.isNull()));
    startTimeValue.get(startTime);

    //
    //  Get Subscription Duration
    //
    Uint32 durationIndex = instance.findProperty(_PROPERTY_DURATION);
    if (durationIndex != PEG_NOT_FOUND)
    {
        CIMValue durationValue;
        durationValue = instance.getProperty(durationIndex).getValue();
        if (durationValue.isNull())
        {
            hasDuration = false;
        }
        else
        {
            Uint64 duration;
            durationValue.get(duration);

            //
            //  A Start Time set to the _ZERO_INTERVAL_STRING indicates that
            //  the subscription has not yet been enabled for the first time
            //  In this case, the time remaining is equal to the Duration
            //
            if (startTime.isInterval())
            {
                if (startTime.equal(CIMDateTime(_ZERO_INTERVAL_STRING)))
                {
                    timeRemaining = (Sint64) duration;
                }

                //
                //  Any interval value other than _ZERO_INTERVAL_STRING
                //  indicates an invalid Start Time value in the instance
                //
                else
                {
                    PEGASUS_ASSERT(false);
                }
            }

            else
            {
                //
                //  Get current date time, and calculate Subscription Time
                //  Remaining
                //
                CIMDateTime currentDateTime = CIMDateTime::getCurrentDateTime();

                Sint64 difference = CIMDateTime::getDifference(
                    startTime, currentDateTime);
                PEGASUS_ASSERT(difference >= 0);
                if (((Sint64) duration - difference) >= 0)
                {
                    timeRemaining = (Sint64) duration - difference;
                }
            }
        }
    }
    else
    {
        hasDuration = false;
    }

    PEG_METHOD_EXIT();
    return hasDuration;
}

void IndicationService::_setTimeRemaining(
    CIMInstance& instance)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_setTimeRemaining");

    Uint64 timeRemaining = 0;
    if (_getTimeRemaining(instance, timeRemaining))
    {
        //
        //  Add or set the value of the property with the calculated value
        //
        if (instance.findProperty(_PROPERTY_TIMEREMAINING) == PEG_NOT_FOUND)
        {
            instance.addProperty(
                CIMProperty(_PROPERTY_TIMEREMAINING, timeRemaining));
        }
        else
        {
            CIMProperty remaining = instance.getProperty(
                instance.findProperty(_PROPERTY_TIMEREMAINING));
            remaining.setValue(CIMValue(timeRemaining));
        }
    }

    PEG_METHOD_EXIT();
}

void IndicationService::_getCreateParams(
    const CIMInstance& subscriptionInstance,
    Array<CIMName>& indicationSubclasses,
    Array<ProviderClassList>& indicationProviders,
    CIMPropertyList& propertyList,
    CIMNamespaceName& sourceNameSpace,
    String& condition,
    String& query,
    String& queryLanguage)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getCreateParams");

    CIMName indicationClassName;
    condition = String::EMPTY;
    query = String::EMPTY;
    queryLanguage = String::EMPTY;
    String filterName;

    //
    //  Get filter properties
    //
    _subscriptionRepository->getFilterProperties(subscriptionInstance, query,
        sourceNameSpace, queryLanguage, filterName);

    //
    //  Build the query expression from the filter query
    //
    QueryExpression queryExpression = _getQueryExpression(query,
                                                         queryLanguage,
                                                         sourceNameSpace);

    //
    //  Get indication class name from filter query (FROM clause)
    //
    indicationClassName = _getIndicationClassName(queryExpression,
                                                   sourceNameSpace);

    //
    //  Get list of subclass names for indication class
    //
    indicationSubclasses = _subscriptionRepository->getIndicationSubclasses(
        sourceNameSpace, indicationClassName);


    //
    //  Get indication provider class lists
    //
    indicationProviders = _getIndicationProviders(
         queryExpression,
         sourceNameSpace,
         indicationClassName,
         indicationSubclasses);

    if (indicationProviders.size() > 0)
    {
        condition = _getCondition(query);
    }

    PEG_METHOD_EXIT();
}

void IndicationService::_getCreateParams(
    const CIMInstance& subscriptionInstance,
    Array<CIMName>& indicationSubclasses,
    CIMPropertyList& propertyList,
    CIMNamespaceName& sourceNameSpace,
    String& condition,
    String& query,
    String& queryLanguage)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getCreateParams");

    condition = String::EMPTY;
    query = String::EMPTY;
    queryLanguage = String::EMPTY;
    String filterName;

    //
    //  Get filter properties
    //
    _subscriptionRepository->getFilterProperties(subscriptionInstance, query,
        sourceNameSpace, queryLanguage, filterName);
    QueryExpression queryExpression = _getQueryExpression(
        query,
        queryLanguage,
        sourceNameSpace);

    //
    //  Get indication class name from filter query (FROM clause)
    //
    CIMName indicationClassName =
        _getIndicationClassName(queryExpression, sourceNameSpace);

    //
    //  Get required property list from filter query (WHERE clause)
    //
    propertyList = _getPropertyList(queryExpression,
        sourceNameSpace, indicationClassName);

    //
    //  Get condition from filter query (WHERE clause)
    //
    condition = _getCondition(query);

    //
    //  Get list of subclass names for indication class
    //
    indicationSubclasses = _subscriptionRepository->getIndicationSubclasses(
        sourceNameSpace, indicationClassName);

    PEG_METHOD_EXIT();
}

Array<ProviderClassList> IndicationService::_getDeleteParams(
    const CIMInstance& subscriptionInstance,
    Array<CIMName>& indicationSubclasses,
    CIMNamespaceName& sourceNameSpace)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getDeleteParams");

    String filterQuery;
    String queryLanguage;
    String filterName;
    CIMName indicationClassName;
    Array<ProviderClassList> indicationProviders;

    //
    //  Get filter properties
    //
    _subscriptionRepository->getFilterProperties(subscriptionInstance,
        filterQuery, sourceNameSpace, queryLanguage, filterName);
    QueryExpression queryExpression =
        _getQueryExpression(filterQuery, queryLanguage, sourceNameSpace);

    //
    //  Get indication class name from filter query (FROM clause)
    //
    indicationClassName =
        _getIndicationClassName(queryExpression, sourceNameSpace);

    //
    //  Get list of subclass names for indication class
    //
    indicationSubclasses = _subscriptionRepository->getIndicationSubclasses(
        sourceNameSpace, indicationClassName);

    //
    //  Get indication provider class lists from Active Subscriptions table
    //
    ActiveSubscriptionsTableEntry tableValue;
    if (_subscriptionTable->getSubscriptionEntry(
            subscriptionInstance.getPath(), tableValue))
    {
        indicationProviders = tableValue.providers;
    }
    else
    {
        //
        //  Subscription not found in Active Subscriptions table
        //
    }

    PEG_METHOD_EXIT();
    return indicationProviders;
}

void IndicationService::_sendAsyncCreateRequests(
    const Array<ProviderClassList>& indicationProviders,
    const CIMNamespaceName& nameSpace,
    const CIMPropertyList& propertyList,
    const String& condition,
    const String& query,
    const String& queryLanguage,
    const CIMInstance& subscription,
    const AcceptLanguageList& acceptLangs,
    const ContentLanguageList& contentLangs,
    const CIMRequestMessage * origRequest,
    const Array<CIMName>& indicationSubclasses,
    const String& userName,
    const String& authType)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_sendAsyncCreateRequests");

    CIMValue propValue;
    Uint16 repeatNotificationPolicy;

    // If there are no providers to accept the subscription, just return
    if (indicationProviders.size() == 0)
    {
        PEG_METHOD_EXIT();
        return;
    }

    //
    //  Get repeat notification policy value from subscription instance
    //
    propValue = subscription.getProperty(
        subscription.findProperty(
            _PROPERTY_REPEATNOTIFICATIONPOLICY)).getValue();
    propValue.get(repeatNotificationPolicy);

    CIMRequestMessage * aggRequest=0;

    if (origRequest == 0)
    {
        //
        //  Initialize -- no request associated with this create
        //
        aggRequest = 0;
    }
    else
    {
        //
        //  Create Instance or Modify Instance
        //
        switch (origRequest->getType())
        {
            case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
            {
                CIMCreateInstanceRequestMessage * request =
                    (CIMCreateInstanceRequestMessage *) origRequest;
                CIMCreateInstanceRequestMessage * requestCopy =
                    new CIMCreateInstanceRequestMessage(*request);
                aggRequest = requestCopy;
                break;
            }

            case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
            {
                CIMModifyInstanceRequestMessage * request =
                    (CIMModifyInstanceRequestMessage *) origRequest;
                CIMModifyInstanceRequestMessage * requestCopy =
                    new CIMModifyInstanceRequestMessage(*request);
                aggRequest = requestCopy;
                break;
            }

            default:
            {
                PEG_TRACE((TRC_INDICATION_SERVICE,Tracer::LEVEL1,
                    "Unexpected origRequest type %s "
                    "in _sendAsyncCreateRequests",
                    MessageTypeToString(origRequest->getType())));
                PEGASUS_ASSERT(false);
                break;
            }
        }
    }

    //
    //  Create an aggregate object for the create subscription requests
    //
    IndicationOperationAggregate * operationAggregate =
        new IndicationOperationAggregate(aggRequest, indicationSubclasses);
    operationAggregate->setNumberIssued(indicationProviders.size());

    //
    //  Send Create request to each provider
    //
    for (Uint32 i = 0; i < indicationProviders.size(); i++)
    {
        //
        //  Create the create subscription request
        //
       CIMCreateSubscriptionRequestMessage * request =
            new CIMCreateSubscriptionRequestMessage(
                XmlWriter::getNextMessageId(),
                nameSpace,
                subscription,
                indicationProviders[i].classList,
                propertyList,
                repeatNotificationPolicy,
                query,
                QueueIdStack(_providerManager, getQueueId()),
                authType,
                userName);

        //
        //  Store a copy of the request in the operation aggregate instance
        //
        CIMCreateSubscriptionRequestMessage * requestCopy =
            new CIMCreateSubscriptionRequestMessage(*request);
        requestCopy->operationContext.insert(ProviderIdContainer(
            indicationProviders[i].providerModule
            ,indicationProviders[i].provider
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
            ,indicationProviders[i].isRemoteNameSpace
            ,indicationProviders[i].remoteInfo
#endif
            ));
        operationAggregate->appendRequest(requestCopy);
        request->operationContext.insert(ProviderIdContainer(
            indicationProviders[i].providerModule
            ,indicationProviders[i].provider
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
            ,indicationProviders[i].isRemoteNameSpace
            ,indicationProviders[i].remoteInfo
#endif
            ));
        request->operationContext.insert(
            SubscriptionInstanceContainer(subscription));
        request->operationContext.insert(
            SubscriptionFilterConditionContainer(condition,queryLanguage));
        request->operationContext.insert(
            SubscriptionFilterQueryContainer(query,queryLanguage,nameSpace));
        request->operationContext.insert(IdentityContainer(userName));
        request->operationContext.set(
            ContentLanguageListContainer(contentLangs));
        request->operationContext.set(AcceptLanguageListContainer(acceptLangs));

        AsyncOpNode * op = this->get_op();

        AsyncLegacyOperationStart * async_req =
            new AsyncLegacyOperationStart(
                op,
                _providerManager,
                request);

        SendAsync(
            op,
            _providerManager,
            IndicationService::_aggregationCallBack,
            this,
            operationAggregate);
    }

    PEG_METHOD_EXIT();
}

Array<ProviderClassList> IndicationService::_sendWaitCreateRequests(
    const Array<ProviderClassList>& indicationProviders,
    const CIMNamespaceName& nameSpace,
    const CIMPropertyList& propertyList,
    const String& condition,
    const String& query,
    const String& queryLanguage,
    const CIMInstance& subscription,
    const AcceptLanguageList& acceptLangs,
    const ContentLanguageList& contentLangs,
    const String& userName,
    const String& authType)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_sendWaitCreateRequests");

    CIMValue propValue;
    Uint16 repeatNotificationPolicy;
    Array<ProviderClassList> acceptedProviders;
    acceptedProviders.clear();

    // If there are no providers to accept the subscription, just return
    if (indicationProviders.size() == 0)
    {
        PEG_METHOD_EXIT();
        return acceptedProviders;
    }

    //
    //  Get repeat notification policy value from subscription instance
    //
    propValue = subscription.getProperty(
        subscription.findProperty(
            _PROPERTY_REPEATNOTIFICATIONPOLICY)).getValue();
    propValue.get(repeatNotificationPolicy);

    //
    //  Send Create request to each provider
    //
    for (Uint32 i = 0; i < indicationProviders.size(); i++)
    {
        //
        //  Create the create subscription request
        //
        CIMCreateSubscriptionRequestMessage * request =
            new CIMCreateSubscriptionRequestMessage(
                XmlWriter::getNextMessageId(),
                nameSpace,
                subscription,
                indicationProviders[i].classList,
                propertyList,
                repeatNotificationPolicy,
                query,
                QueueIdStack(_providerManager, getQueueId()),
                authType,
                userName);

        //
        //  Set operation context
        //
        request->operationContext.insert(ProviderIdContainer(
            indicationProviders[i].providerModule
            ,indicationProviders[i].provider
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
            ,indicationProviders[i].isRemoteNameSpace
            ,indicationProviders[i].remoteInfo
#endif
            ));
        request->operationContext.insert(
            SubscriptionInstanceContainer(subscription));
        request->operationContext.insert(
            SubscriptionFilterConditionContainer(condition,queryLanguage));
        request->operationContext.insert(
            SubscriptionFilterQueryContainer(query,queryLanguage,nameSpace));
        request->operationContext.insert(IdentityContainer(userName));
        request->operationContext.set(
            ContentLanguageListContainer(contentLangs));
        request->operationContext.set(AcceptLanguageListContainer(acceptLangs));

        AsyncLegacyOperationStart * asyncRequest =
            new AsyncLegacyOperationStart(
                0,
                _providerManager,
                request);

        AsyncReply * asyncReply = SendWait(asyncRequest);

        CIMCreateSubscriptionResponseMessage * response =
            reinterpret_cast<CIMCreateSubscriptionResponseMessage *>(
                (static_cast<AsyncLegacyOperationResult *>(
                    asyncReply))->get_result());

        if (response->cimException.getCode() == CIM_ERR_SUCCESS)
        {
            acceptedProviders.append(indicationProviders[i]);
#ifdef PEGASUS_ENABLE_INDICATION_COUNT
            _providerIndicationCountTable.insertEntry(
                indicationProviders[i].provider);
#endif
        }
        else
        {
            //
            //  Provider rejected the subscription
            //
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL2,
                "Provider (%s) rejected create subscription: %s",
                (const char*)indicationProviders[i].provider.getPath()
                       .toString().getCString(),
                (const char*)response->cimException.getMessage().getCString()));
        }

        delete response;
        delete asyncRequest;
        delete asyncReply;
    }  //  for each indication provider

    PEG_METHOD_EXIT();
    return acceptedProviders;
}

void IndicationService::_sendWaitModifyRequests(
    const Array<ProviderClassList>& indicationProviders,
     const CIMNamespaceName& nameSpace,
     const CIMPropertyList& propertyList,
     const String& condition,
     const String& query,
     const String& queryLanguage,
     const CIMInstance& subscription,
     const AcceptLanguageList& acceptLangs,
     const ContentLanguageList& contentLangs,
     const String& userName,
     const String& authType)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_sendWaitModifyRequests");

    CIMValue propValue;
    Uint16 repeatNotificationPolicy;

    // If there are no providers to accept the subscription update, just return
    if (indicationProviders.size() == 0)
    {
        PEG_METHOD_EXIT();
        return;
    }

    //
    //  Get repeat notification policy value from subscription instance
    //
    propValue = subscription.getProperty(
        subscription.findProperty(
            _PROPERTY_REPEATNOTIFICATIONPOLICY)).getValue();
    propValue.get(repeatNotificationPolicy);

    //
    //  Send Modify request to each provider
    //
    for (Uint32 i = 0; i < indicationProviders.size(); i++)
    {
        CIMModifySubscriptionRequestMessage * request =
            new CIMModifySubscriptionRequestMessage(
                XmlWriter::getNextMessageId(),
                nameSpace,
                subscription,
                indicationProviders[i].classList,
                propertyList,
                repeatNotificationPolicy,
                query,
                QueueIdStack(_providerManager, getQueueId()),
                authType,
                userName);

        //
        //  Set operation context
        //
        request->operationContext.insert(ProviderIdContainer(
            indicationProviders[i].providerModule
            ,indicationProviders[i].provider
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
            ,indicationProviders[i].isRemoteNameSpace
            ,indicationProviders[i].remoteInfo
#endif
            ));
        request->operationContext.insert(
            SubscriptionInstanceContainer(subscription));
        request->operationContext.insert(
            SubscriptionFilterConditionContainer(condition,queryLanguage));
        request->operationContext.insert(
            SubscriptionFilterQueryContainer(query,queryLanguage,nameSpace));
        request->operationContext.insert(IdentityContainer(userName));
        request->operationContext.set(
            ContentLanguageListContainer(contentLangs));
        request->operationContext.set(AcceptLanguageListContainer(acceptLangs));

        AsyncLegacyOperationStart * asyncRequest =
            new AsyncLegacyOperationStart(
                0,
                _providerManager,
                request);

        AsyncReply * asyncReply = SendWait(asyncRequest);

        CIMModifySubscriptionResponseMessage * response =
            reinterpret_cast<CIMModifySubscriptionResponseMessage *>(
                (static_cast<AsyncLegacyOperationResult *>(
                    asyncReply))->get_result());

        if (!(response->cimException.getCode() == CIM_ERR_SUCCESS))
        {
            //
            //  Provider rejected the subscription
            //
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL2,
                "Provider (%s) rejected modify subscription: %s",
                (const char*)indicationProviders[i].provider.getPath()
                       .toString().getCString(),
                (const char*)response->cimException.getMessage().getCString()));
        }

        delete response;
        delete asyncRequest;
        delete asyncReply;
    }  //  for each indication provider

    PEG_METHOD_EXIT();
}

void IndicationService::_sendAsyncDeleteRequests(
    const Array<ProviderClassList>& indicationProviders,
    const CIMNamespaceName& nameSpace,
    const CIMInstance& subscription,
    const AcceptLanguageList& acceptLangs,
    const ContentLanguageList& contentLangs,
    const CIMRequestMessage * origRequest,
    const Array<CIMName>& indicationSubclasses,
    const String& userName,
    const String& authType)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_sendAsyncDeleteRequests");

    // If there are no providers to delete the subscription, just return
    if (indicationProviders.size() == 0)
    {
        PEG_METHOD_EXIT();
        return;
    }

    //
    //  Update subscription hash tables
    //
    _subscriptionTable->removeSubscription(
        subscription,
        indicationSubclasses,
        nameSpace,
        indicationProviders);

    CIMRequestMessage * aggRequest = 0;

    if (origRequest == 0)
    {
        //
        //  Delete a referencing or expired subscription -- no request
        //  associated with this delete
        //
        aggRequest = 0;
    }
    else
    {
        //
        //  Delete Instance or Modify Instance
        //
        switch (origRequest->getType())
        {
            case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
            {
                CIMDeleteInstanceRequestMessage * request =
                    (CIMDeleteInstanceRequestMessage *) origRequest;
                CIMDeleteInstanceRequestMessage * requestCopy =
                    new CIMDeleteInstanceRequestMessage(*request);
                aggRequest = requestCopy;
                break;
            }

            case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
            {
                CIMModifyInstanceRequestMessage * request =
                    (CIMModifyInstanceRequestMessage *) origRequest;
                CIMModifyInstanceRequestMessage * requestCopy =
                    new CIMModifyInstanceRequestMessage(*request);
                aggRequest = requestCopy;
                break;
            }

            default:
            {
                PEG_TRACE((TRC_INDICATION_SERVICE,Tracer::LEVEL1, 
                    "Unexpected origRequest type %s "
                    "in _sendAsyncDeleteRequests",
                    MessageTypeToString(origRequest->getType())));
                PEGASUS_ASSERT(false);
                break;
            }
        }
    }

    //
    //  Create an aggregate object for the delete subscription requests
    //
    IndicationOperationAggregate * operationAggregate =
        new IndicationOperationAggregate(aggRequest, indicationSubclasses);
    operationAggregate->setNumberIssued(indicationProviders.size());

    //
    //  Send Delete request to each provider
    //
    for (Uint32 i = 0; i < indicationProviders.size(); i++)
    {
        CIMDeleteSubscriptionRequestMessage * request =
            new CIMDeleteSubscriptionRequestMessage(
                XmlWriter::getNextMessageId(),
                nameSpace,
                subscription,
                indicationProviders[i].classList,
                QueueIdStack(_providerManager, getQueueId()),
                authType,
                userName);

        //
        //  Store a copy of the request in the operation aggregate instance
        //
        CIMDeleteSubscriptionRequestMessage * requestCopy =
            new CIMDeleteSubscriptionRequestMessage(*request);
        requestCopy->operationContext.insert(ProviderIdContainer(
            indicationProviders[i].providerModule
            ,indicationProviders[i].provider
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
            ,indicationProviders[i].isRemoteNameSpace
            ,indicationProviders[i].remoteInfo
#endif
             ));
        operationAggregate->appendRequest(requestCopy);
        request->operationContext.insert(ProviderIdContainer(
            indicationProviders[i].providerModule
            ,indicationProviders[i].provider
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
            ,indicationProviders[i].isRemoteNameSpace
            ,indicationProviders[i].remoteInfo
#endif
            ));

        request->operationContext.insert(
            SubscriptionInstanceContainer(subscription));
        request->operationContext.insert(IdentityContainer(userName));
        request->operationContext.set(
            ContentLanguageListContainer(contentLangs));
        request->operationContext.set(AcceptLanguageListContainer(acceptLangs));

        AsyncOpNode * op = this->get_op();

        AsyncLegacyOperationStart * async_req =
            new AsyncLegacyOperationStart(
                op,
                _providerManager,
                request);

        SendAsync(
            op,
            _providerManager,
            IndicationService::_aggregationCallBack,
            this,
            operationAggregate);
    }

    PEG_METHOD_EXIT();
}

void IndicationService::_sendWaitDeleteRequests(
    const Array<ProviderClassList>& indicationProviders,
    const CIMNamespaceName& nameSpace,
    const CIMInstance& subscription,
    const AcceptLanguageList& acceptLangs,
    const ContentLanguageList& contentLangs,
    const String& userName,
    const String& authType)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_sendWaitDeleteRequests");

    // If there are no providers to delete the subscription, just return
    if (indicationProviders.size() == 0)
    {
        PEG_METHOD_EXIT();
        return;
    }

    //
    //  Send Delete request to each provider
    //
    for (Uint32 i = 0; i < indicationProviders.size(); i++)
    {
        CIMDeleteSubscriptionRequestMessage * request =
            new CIMDeleteSubscriptionRequestMessage(
                XmlWriter::getNextMessageId(),
                nameSpace,
                subscription,
                indicationProviders[i].classList,
                QueueIdStack(_providerManager, getQueueId()),
                authType,
                userName);

        //
        //  Set operation context
        //
        request->operationContext.insert(ProviderIdContainer(
            indicationProviders[i].providerModule
            ,indicationProviders[i].provider
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
            ,indicationProviders[i].isRemoteNameSpace
            ,indicationProviders[i].remoteInfo
#endif
            ));
        request->operationContext.insert(
            SubscriptionInstanceContainer(subscription));
        request->operationContext.insert(IdentityContainer(userName));
        request->operationContext.set(
            ContentLanguageListContainer(contentLangs));
        request->operationContext.set(AcceptLanguageListContainer(acceptLangs));

        AsyncLegacyOperationStart * asyncRequest =
            new AsyncLegacyOperationStart(
                0,
                _providerManager,
                request);

        AsyncReply * asyncReply = SendWait(asyncRequest);

        CIMDeleteSubscriptionResponseMessage * response =
            reinterpret_cast<CIMDeleteSubscriptionResponseMessage *>(
                (static_cast<AsyncLegacyOperationResult *>(
                    asyncReply))->get_result());

        if (!(response->cimException.getCode() == CIM_ERR_SUCCESS))
        {
            //
            //  Provider rejected the subscription
            //
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL2,
                "Provider (%s) rejected delete subscription: %s",
                (const char*)indicationProviders[i].provider.getPath()
                       .toString().getCString(),
                (const char*)response->cimException.getMessage().getCString()));
        }

        delete response;
        delete asyncRequest;
        delete asyncReply;
    }  //  for each indication provider

    PEG_METHOD_EXIT();
}

void IndicationService::_aggregationCallBack(
    AsyncOpNode * op,
    MessageQueue * q,
    void * userParameter)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_aggregationCallBack");

    IndicationService * service = static_cast<IndicationService *>(q);

    AsyncRequest * asyncRequest =
        static_cast<AsyncRequest *>(op->removeRequest());
    AsyncReply * asyncReply = static_cast<AsyncReply *>(op->removeResponse());

    IndicationOperationAggregate * operationAggregate =
        reinterpret_cast<IndicationOperationAggregate *>(userParameter);
    PEGASUS_ASSERT(operationAggregate != 0);

    CIMResponseMessage * response = 0;
    MessageType msgType = asyncReply->getType();
    PEGASUS_ASSERT((msgType == ASYNC_ASYNC_LEGACY_OP_RESULT) ||
                   (msgType == ASYNC_ASYNC_MODULE_OP_RESULT));

    if (msgType == ASYNC_ASYNC_LEGACY_OP_RESULT)
    {
        response = reinterpret_cast<CIMResponseMessage *>(
            (static_cast<AsyncLegacyOperationResult *>(
                asyncReply))->get_result());
    }
    else if (msgType == ASYNC_ASYNC_MODULE_OP_RESULT)
    {
        response = reinterpret_cast<CIMResponseMessage *>(
            (static_cast<AsyncModuleOperationResult *>(
                asyncReply))->get_result());
    }

    PEGASUS_ASSERT(response != 0);

    delete asyncRequest;
    delete asyncReply;
    service->return_op(op);

    Boolean isDoneAggregation = operationAggregate->appendResponse(response);
    if (isDoneAggregation)
    {
        service->_handleOperationResponseAggregation(operationAggregate);
    }

    PEG_METHOD_EXIT();
}

void IndicationService::_handleOperationResponseAggregation(
    IndicationOperationAggregate * operationAggregate)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleOperationResponseAggregation");

    switch (operationAggregate->getRequest(0)->getType())
    {
        case CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE:
        {
            _handleCreateResponseAggregation(operationAggregate);
            break;
        }

        case CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE:
        {
            _handleDeleteResponseAggregation(operationAggregate);
            break;
        }

        default:
        {
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
                "Unexpected request type %s "
                "in _handleOperationResponseAggregation",
                MessageTypeToString(
                    operationAggregate->getRequest(0)->getType())));
            PEGASUS_ASSERT(false);
            break;
        }
    }

    //
    //  Requests and responses are deleted in destructor
    //
    delete operationAggregate;

    PEG_METHOD_EXIT();
}

void IndicationService::_handleCreateResponseAggregation(
    IndicationOperationAggregate * operationAggregate)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleCreateResponseAggregation");

    Array<ProviderClassList> acceptedProviders;
    CIMObjectPath instanceRef;
    CIMException cimException;

    //
    //  Examine provider responses
    //
    acceptedProviders.clear();
    for (Uint32 i = 0; i < operationAggregate->getNumberResponses(); i++)
    {
        //
        //  Find provider from which response was sent
        //
        CIMResponseMessage * response = operationAggregate->getResponse(i);
        ProviderClassList provider = operationAggregate->findProvider(
            response->messageId);
        if (response->cimException.getCode() == CIM_ERR_SUCCESS)
        {
            //
            //  If response is SUCCESS, provider accepted the subscription
            //  Add provider to list of providers that accepted subscription
            //
            acceptedProviders.append(provider);
#ifdef PEGASUS_ENABLE_INDICATION_COUNT
            _providerIndicationCountTable.insertEntry(provider.provider);
#endif
        }
        else
        {
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL2,
                "Provider (%s) rejected create subscription: %s",
                (const char*)
                     provider.provider.getPath().toString().getCString(),
                (const char*)response->cimException.getMessage().getCString()));
        }
    }

    CIMCreateSubscriptionRequestMessage * request =
        (CIMCreateSubscriptionRequestMessage *)
            operationAggregate->getRequest(0);

    if (operationAggregate->getOrigType() ==
        CIM_CREATE_INSTANCE_REQUEST_MESSAGE)
    {
        instanceRef = request->subscriptionInstance.getPath();
    }

    if (acceptedProviders.size() == 0)
    {
        //
        //  No providers accepted this subscription
        //
        if (operationAggregate->requiresResponse())
        {
            //
            //  For Create Instance or Modify Instance request, set CIM
            //  exception for response
            //
            cimException = PEGASUS_CIM_EXCEPTION_L(CIM_ERR_NOT_SUPPORTED,
                MessageLoaderParms(
                    "IndicationService.IndicationService._MSG_NOT_ACCEPTED",
                    "No providers accepted the subscription."));
        }
    }

    else
    {
        //
        //  At least one provider accepted the subscription
        //
        if (operationAggregate->getOrigType() ==
            CIM_CREATE_INSTANCE_REQUEST_MESSAGE)
        {
            //
            //  Create Instance -- create the instance in the repository
            //
            CIMCreateInstanceRequestMessage * origRequest =
                (CIMCreateInstanceRequestMessage *)
                    operationAggregate->getOrigRequest();

            CIMInstance instance;
            try
            {
                instanceRef = _subscriptionRepository->createInstance(
                    request->subscriptionInstance, origRequest->nameSpace,
                    ((IdentityContainer)origRequest->operationContext.get
                        (IdentityContainer::NAME)).getUserName(),
                    ((AcceptLanguageListContainer)request->operationContext.get
                        (AcceptLanguageListContainer::NAME)).getLanguages(),
                    ((ContentLanguageListContainer)request->operationContext.get
                        (ContentLanguageListContainer::NAME)).getLanguages(),
                        true);
                instanceRef.setNameSpace(
                    request->subscriptionInstance.getPath().getNameSpace());
                instance = _subscriptionRepository->getInstance(
                    origRequest->nameSpace, instanceRef, false);
                instance.setPath(instanceRef);
            }
            catch (CIMException& exception)
            {
                cimException = exception;
            }
            catch (Exception& exception)
            {
                cimException = PEGASUS_CIM_EXCEPTION(
                    CIM_ERR_FAILED, exception.getMessage());
            }

            if (cimException.getCode() == CIM_ERR_SUCCESS)
            {
                //
                //  Insert entries into the subscription hash tables
                //
                _subscriptionTable->insertSubscription(
                    instance,
                    acceptedProviders,
                    operationAggregate->getIndicationSubclasses(),
                    request->nameSpace);

            }
        }
        else  //  CIM_MODIFY_INSTANCE_REQUEST_MESSAGE
        {
            PEGASUS_ASSERT(operationAggregate->getOrigType() ==
                CIM_MODIFY_INSTANCE_REQUEST_MESSAGE);

            //
            //  Insert entries into the subscription hash tables
            //
            _subscriptionTable->insertSubscription(
                request->subscriptionInstance,
                acceptedProviders,
                operationAggregate->getIndicationSubclasses(),
                request->nameSpace);
        }
    }

    // If subscription could not be created, cancel create subscription request
    // or commit create subscription request if subscription was created.
    if (instanceRef.getKeyBindings().size())
    {
        if (cimException.getCode() != CIM_ERR_SUCCESS)
        {
            _subscriptionRepository->cancelCreateSubscription(instanceRef);
        }
        else
        {
            _subscriptionRepository->commitCreateSubscription(instanceRef);
        }
    }

    //
    //  For Create Instance or Modify Instance request, send response
    //
    if (operationAggregate->requiresResponse())
    {
        if (operationAggregate->getOrigType() ==
            CIM_CREATE_INSTANCE_REQUEST_MESSAGE)
        {
            // Note: don't need to set Content-language in the response
            CIMCreateInstanceResponseMessage* response =
                dynamic_cast<CIMCreateInstanceResponseMessage*>(
                    operationAggregate->getOrigRequest()->buildResponse());
            PEGASUS_ASSERT(response != 0);
            response->cimException = cimException;
            response->instanceName = instanceRef;
            _enqueueResponse(operationAggregate->getOrigRequest(), response);
        }

        else  //  CIM_MODIFY_INSTANCE_REQUEST_MESSAGE
        {
            PEGASUS_ASSERT(operationAggregate->getOrigType () ==
                CIM_MODIFY_INSTANCE_REQUEST_MESSAGE);
            // l10n
            // Note: don't need to set Content-language in the response
            //
            CIMResponseMessage * response =
                operationAggregate->getOrigRequest()->buildResponse();
            response->cimException = cimException;
            _enqueueResponse(operationAggregate->getOrigRequest(), response);
        }
    }

    PEG_METHOD_EXIT();
}

void IndicationService::_handleDeleteResponseAggregation(
    IndicationOperationAggregate * operationAggregate)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_handleDeleteResponseAggregation");

    CIMException cimException;
    Array<ProviderClassList> checkProviders;

    //
    //  Examine provider responses
    //
    for (Uint32 i = 0; i < operationAggregate->getNumberResponses(); i++)
    {
        //
        //  Find provider from which response was sent and add to list
        //
        CIMResponseMessage * response = operationAggregate->getResponse(i);
        ProviderClassList provider = operationAggregate->findProvider(
            response->messageId);
        checkProviders.append(provider);

        //
        //  If response is not SUCCESS, provider rejected the delete
        //
        if (response->cimException.getCode() != CIM_ERR_SUCCESS)
        {
            //
            //  Log a trace message
            //
            PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL2,
                "Provider (%s) rejected delete subscription: %s",
                (const char*)
                     provider.provider.getPath().toString().getCString(),
                (const char*)response->cimException.getMessage().getCString()));
        }
    }

    //
    //  For Delete Instance or Modify Instance request, send response
    //
    if (operationAggregate->requiresResponse())
    {
        CIMResponseMessage * response;
        if (operationAggregate->getOrigType() ==
            CIM_DELETE_INSTANCE_REQUEST_MESSAGE)
        {
            // l10n
            // Note: don't need to set Content-language in the response
            response = operationAggregate->getOrigRequest()->buildResponse();
            response->cimException = cimException;
        }

        else  //  CIM_MODIFY_INSTANCE_REQUEST_MESSAGE
        {
            PEGASUS_ASSERT(operationAggregate->getOrigType() ==
                CIM_MODIFY_INSTANCE_REQUEST_MESSAGE);
            // l10n
            // Note: don't need to set Content-language in the response
            response = operationAggregate->getOrigRequest()->buildResponse();
            response->cimException = cimException;
        }

        _enqueueResponse(operationAggregate->getOrigRequest(), response);
    }

    PEG_METHOD_EXIT();
}

CIMInstance IndicationService::_createAlertInstance(
    const CIMName& alertClassName,
    const Array<CIMInstance>& subscriptions)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_createAlertInstance");

    CIMInstance indicationInstance(alertClassName);

    //
    //  Add property values for all required properties of CIM_AlertIndication
    //
    indicationInstance.addProperty(
        CIMProperty(_PROPERTY_ALERTTYPE, CIMValue((Uint16) _TYPE_OTHER)));
    //
    //  ATTN: what should Other Alert Type value be??
    //  Currently using Alert class name
    //
    indicationInstance.addProperty(
        CIMProperty(_PROPERTY_OTHERALERTTYPE, alertClassName.getString()));

    indicationInstance.addProperty(
        CIMProperty(_PROPERTY_PERCEIVEDSEVERITY,
            CIMValue((Uint16) _SEVERITY_WARNING)));
    //
    //  ATTN: what should Probable Cause value be??
    //  Currently using Unknown
    //
    indicationInstance.addProperty(
        CIMProperty(_PROPERTY_PROBABLECAUSE,
            CIMValue((Uint16) _CAUSE_UNKNOWN)));

    //
    //  Add properties specific to each alert class
    //  ATTN: update once alert classes have been defined
    //  NB: for _CLASS_NO_PROVIDER_ALERT and _CLASS_PROVIDER_TERMINATED_ALERT,
    //  one of the properties will be a list of affected subscriptions
    //  It is for that reason that subscriptions is passed in as a parameter
    //
    if (alertClassName.equal(_CLASS_CIMOM_SHUTDOWN_ALERT))
    {
    }
    else if (alertClassName.equal(_CLASS_NO_PROVIDER_ALERT))
    {
    }
    else if (alertClassName.equal(_CLASS_PROVIDER_TERMINATED_ALERT))
    {
    }

    PEG_METHOD_EXIT();
    return indicationInstance;
}


#if 0
void IndicationService::_sendAlertsCallBack(AsyncOpNode *op,
    MessageQueue *q,
    void *parm)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_sendAlertsCallBack");

   IndicationService *service =
      static_cast<IndicationService *>(q);
   CIMInstance *_handler =
      reinterpret_cast<CIMInstance *>(parm);

   AsyncRequest *asyncRequest = static_cast<AsyncRequest *>(op->get_request());
   AsyncReply *asyncReply = static_cast<AsyncReply *>(op->get_response());
   CIMRequestMessage *request = reinterpret_cast<CIMRequestMessage *>(
      (static_cast<AsyncLegacyOperationStart *>(asyncRequest))->get_action());

   CIMHandleIndicationResponseMessage* response =
      reinterpret_cast<CIMHandleIndicationResponseMessage *>(
          (static_cast<AsyncLegacyOperationResult *>(
              asyncReply))->get_result());

   PEGASUS_ASSERT(response != 0);
   if (response->cimException.getCode() == CIM_ERR_SUCCESS)
   {
   }
   else
   {
   }

   //
   //  ATTN: Check for return value indicating invalid queue ID
   //  If received, need to find Handler Manager Service queue ID
   //  again
   //

// << Mon Jul 15 09:59:16 2002 mdd >> handler is allocated as an element in
// an array, don't delete here.
//   delete _handler;
   delete request;
   delete response;
   delete asyncRequest;
   delete asyncReply;
   op->release();
   service->return_op(op);

    PEG_METHOD_EXIT();
}


void IndicationService::_sendAlerts(
    const Array<CIMInstance>& subscriptions,
    /* const */ CIMInstance& alertInstance)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE, "IndicationService::_sendAlerts");

    CIMInstance current;

    PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
        "Sending alert: %s",
        (const char*)alertInstance.getClassName().getString().getCString()));

    //
    //  Get list of unique handler instances for all subscriptions in list
    //
    for (Uint32 i = 0; i < subscriptions.size(); i++)
    {
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
            "Alert subscription: %s",
            (const char*)subscriptions[i].getPath().toString().getCString()));

        //
        //  Get handler instance
        //
        current = _subscriptionRepository->getHandler(subscriptions[i]);

    // ATTN: For the handlers which do not need subscription instance
    // need to check duplicate alter

            //
            //  Send handle indication request to the handler
            //
            CIMHandleIndicationRequestMessage * handler_request =
                new CIMHandleIndicationRequestMessage(
                    XmlWriter::getNextMessageId(),
                    current.getPath().getNameSpace(),
                    current,
            subscriptions[i],
                    alertInstance,
                    QueueIdStack(_handlerService, getQueueId()));

            AsyncOpNode* op = this->get_op();

            AsyncLegacyOperationStart *async_req =
                new AsyncLegacyOperationStart(
                    op,
                    _handlerService,
                    handler_request,
                    _queueId);

            SendAsync(op,
                  _handlerService,
                  IndicationService::_sendAlertsCallBack,
                  this,
                  (void *)&current);

    }

    PEG_METHOD_EXIT();
}
#endif

void IndicationService::_sendSubscriptionInitComplete()
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_sendSubscriptionInitComplete");

    //
    //  Create the Subscription Init Complete request
    //
    CIMSubscriptionInitCompleteRequestMessage * request =
        new CIMSubscriptionInitCompleteRequestMessage(
            XmlWriter::getNextMessageId(),
            QueueIdStack(_providerManager, getQueueId()));

    //
    //  Send Subscription Initialization Complete request to provider manager
    //
    AsyncLegacyOperationStart * asyncRequest =
        new AsyncLegacyOperationStart(
            0,
            _providerManager,
            request);

    AutoPtr<AsyncReply> asyncReply(SendWait(asyncRequest));
    //
    //  Note: the response does not contain interesting data
    //
    delete asyncRequest;

    PEG_METHOD_EXIT();
}

Boolean IndicationService::_getCreator(
    const CIMInstance& instance,
    String& creator) const
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE, "IndicationService::_getCreator");

    Uint32 creatorIndex = instance.findProperty(
        PEGASUS_PROPERTYNAME_INDSUB_CREATOR);
    if (creatorIndex != PEG_NOT_FOUND)
    {
        CIMValue creatorValue = instance.getProperty(creatorIndex).getValue();
        if (creatorValue.isNull())
        {
            PEG_TRACE_CSTRING(TRC_INDICATION_SERVICE,Tracer::LEVEL1,
                "Null Subscription Creator property value");

            //
            //  This is a corrupted/invalid instance
            //
            PEG_METHOD_EXIT();
            return false;
        }
        else if ((creatorValue.getType() != CIMTYPE_STRING) ||
                 (creatorValue.isArray()))
        {
            PEG_TRACE((TRC_INDICATION_SERVICE,Tracer::LEVEL1,
                "Subscription Creator property value of incorrect type:%s %s",
                (creatorValue.isArray()) ? " array of" : " ",
                cimTypeToString(creatorValue.getType())));

            //
            //  This is a corrupted/invalid instance
            //
            PEG_METHOD_EXIT();
            return false;
        }
        else
        {
            creatorValue.get(creator);
        }
    }
    else
    {
        PEG_TRACE_CSTRING(TRC_INDICATION_SERVICE,Tracer::LEVEL1,
            "Missing Subscription Creator property");

        //
        //  This is a corrupted/invalid instance
        //
        PEG_METHOD_EXIT();
        return false;
    }

    PEG_METHOD_EXIT();
    return true;
}

Boolean IndicationService::_validateState(
    const Uint16 state) const
{
    //
    //  Validate the value
    //
    if (!Contains(_validStates, state))
    {
        //
        //  This is a corrupted/invalid instance
        //
        return false;
    }

    return true;
}

void IndicationService::_updatePropertyList(
    CIMName& className,
    CIMPropertyList& propertyList,
    Boolean& setTimeRemaining,
    Boolean& startTimeAdded,
    Boolean& durationAdded)
{
    PEG_METHOD_ENTER( TRC_INDICATION_SERVICE,
        "IndicationService::_updatePropertyList");

    //
    //  A null propertyList means all properties
    //  If the class is Subscription, that includes the Time Remaining property
    //
    if (className.equal(PEGASUS_CLASSNAME_INDSUBSCRIPTION) ||
        className.equal(PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION))
    {
        setTimeRemaining = true;
    }
    else
    {
        setTimeRemaining = false;
    }
    startTimeAdded = false;
    durationAdded = false;
    if (!propertyList.isNull())
    {
        setTimeRemaining = false;
        Array<CIMName> properties = propertyList.getPropertyNameArray();

        //
        //  Add Creator to property list
        //
        if (!ContainsCIMName(properties,
            PEGASUS_PROPERTYNAME_INDSUB_CREATOR))
        {
            properties.append(PEGASUS_PROPERTYNAME_INDSUB_CREATOR);
        }

        //
        //  If a Subscription and Time Remaining is requested,
        //  Ensure Subscription Duration and Start Time are in property list
        //
        if (className.equal(PEGASUS_CLASSNAME_INDSUBSCRIPTION) ||
            className.equal(PEGASUS_CLASSNAME_FORMATTEDINDSUBSCRIPTION))
        {
            if (ContainsCIMName(properties, _PROPERTY_TIMEREMAINING))
            {
                setTimeRemaining = true;
                if (!ContainsCIMName(properties, _PROPERTY_STARTTIME))
                {
                    properties.append(_PROPERTY_STARTTIME);
                    startTimeAdded = true;
                }
                if (!ContainsCIMName(properties, _PROPERTY_DURATION))
                {
                    properties.append(_PROPERTY_DURATION);
                    durationAdded = true;
                }
            }
        }
        propertyList.clear();
        propertyList.set(properties);
    }

    PEG_METHOD_EXIT();
}

String IndicationService::_getSubscriptionLogString(CIMInstance& subscription)
{
    //
    //  Get Subscription Filter namespace and Name, and Handler namespace and
    //  Name
    //
    String logString;
    CIMValue filterValue;
    CIMObjectPath filterPath;
    CIMNamespaceName filterNS;
    Array<CIMKeyBinding> filterKeyBindings;
    CIMValue handlerValue;
    CIMObjectPath handlerPath;
    CIMNamespaceName handlerNS;
    Array<CIMKeyBinding> handlerKeyBindings;
    filterValue = subscription.getProperty(subscription.findProperty(
        PEGASUS_PROPERTYNAME_FILTER)).getValue();
    filterValue.get(filterPath);

    //
    //  Get Filter namespace - if not set in Filter reference property
    //  value, namespace is the namespace of the subscription
    //
    filterNS = filterPath.getNameSpace();
    if (filterNS.isNull())
    {
        filterNS = subscription.getPath().getNameSpace();
    }
    logString.append(filterNS.getString());
    logString.append(" ");
    filterKeyBindings = filterPath.getKeyBindings();
    for (Uint32 i = 0; i < filterKeyBindings.size(); i++)
    {
        if (filterKeyBindings[i].getName().equal(PEGASUS_PROPERTYNAME_NAME))
        {
            logString.append(filterKeyBindings[i].getValue());
            logString.append(", ");
            break;
        }
    }
    handlerValue = subscription.getProperty(
        subscription.findProperty(PEGASUS_PROPERTYNAME_HANDLER)).getValue();
    handlerValue.get(handlerPath);

    //
    //  Get Handler namespace - if not set in Handler reference property
    //  value, namespace is the namespace of the subscription
    //
    handlerNS = handlerPath.getNameSpace();
    if (handlerNS.isNull())
    {
        handlerNS = subscription.getPath().getNameSpace();
    }
    logString.append(handlerNS.getString());
    logString.append(" ");
    handlerKeyBindings = handlerPath.getKeyBindings();
    for (Uint32 j = 0; j < handlerKeyBindings.size(); j++)
    {
        if (handlerKeyBindings[j].getName().equal(PEGASUS_PROPERTYNAME_NAME))
        {
            logString.append(handlerKeyBindings[j].getValue());
            break;
        }
    }

    return logString;
}

String IndicationService::getProviderLogString(CIMInstance& provider)
{
    String logString;

    logString = provider.getProperty(
        provider.findProperty(PEGASUS_PROPERTYNAME_NAME)).getValue().toString();

    return logString;
}

CIMClass IndicationService::_getIndicationClass(
    const CIMInstance& subscriptionInstance)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
    "IndicationService::_getIndicationClass");

    CIMNamespaceName sourceNameSpace;
    String query;
    String queryLanguage;
    CIMName indicationClassName;
    CIMClass indicationClass;
    String filterName;

    //  Get filter properties
    _subscriptionRepository->getFilterProperties(subscriptionInstance, query,
        sourceNameSpace, queryLanguage, filterName);

    //  Build the query expression from the filter query
    QueryExpression queryExpression = _getQueryExpression(query,
                              queryLanguage,
                              sourceNameSpace);

    //  Get indication class name from filter query
    indicationClassName = _getIndicationClassName(
    queryExpression, sourceNameSpace);

    //
    //  Get the indication class object from the repository
    //  Specify localOnly=false because superclass properties are needed
    //  Specify includeQualifiers=false because qualifiers are not needed
    //
    indicationClass = _subscriptionRepository->getClass(
        sourceNameSpace, indicationClassName, false, false, false,
        CIMPropertyList());

    PEG_METHOD_EXIT();
    return indicationClass;
}

void IndicationService::_getRelevantSubscriptions(
    const Array<CIMObjectPath> & providedSubscriptionNames,
    const CIMName& className,
    const CIMNamespaceName& nameSpace,
    const CIMInstance& indicationProvider,
    Array<CIMInstance>& subscriptions,
    Array<String>& subscriptionKeys)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_getRelevantlSubscriptions");

    //
    // Retrieves list of enabled subscription instances in the specified
    // namespace, where the subscription indication class matches or is a
    // superclass of the supported class. A subscription is only included
    // in the list if the specified provider accepted the subscription.
    //
    _subscriptionTable->getMatchingClassNamespaceSubscriptions(
        className,
        nameSpace,
        indicationProvider,
        subscriptions,
        subscriptionKeys);

    //
    // If the indication provider included subscriptions in the 
    // SubscriptionInstanceNamesContainer, the subset of subscriptions
    // specified by the indication provider that also appear in the initial
    // subscriptions list is returned.
    //

    if (providedSubscriptionNames.size() > 0)
    {
        for (Uint32 i = 0; i < subscriptions.size(); i++)
        {
            if (!Contains(providedSubscriptionNames, 
                          subscriptions[i].getPath()))
            {
                subscriptions.remove(i);
                subscriptionKeys.remove(i);
                i--;
            }
        }
    }

    PEGASUS_ASSERT(subscriptions.size() == subscriptionKeys.size());
    PEG_METHOD_EXIT();
}

Boolean IndicationService::_subscriptionMatch(
    const CIMInstance& subscription,
    const CIMInstance& indication,
    const CIMPropertyList& supportedPropertyList,
    QueryExpression& queryExpr,
    const CIMNamespaceName sourceNameSpace)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_subscriptionMatch");

    //
    // If supported properties is null (all properties)
    // the subscription can be supported
    //
    if (!supportedPropertyList.isNull ())
    {
        try
        {
            // Get the class paths in the FROM list
            // Since neither WQL nor CQL support joins, so we can
            // assume one class path.
            CIMName indicationClassName = 
                queryExpr.getClassPathList()[0].getClassName();

            if (!_subscriptionRepository->validateIndicationClassName(
                indicationClassName, sourceNameSpace))
            {
                //
                // Invalid FROM class, the subscription does not match
                //
                PEG_METHOD_EXIT();
                return false;
            }

            //
            //  Get required property list from filter query (WHERE clause)
            //
            //  Note: 
            //  The class should be the class of the indication
            //  instance, not the FROM class.
            //  This is needed because CQL can have class scoping operators
            //  on properties that may not be the same class
            //  as the FROM class.  The required properties
            //  for an indication are based on indication instance class,
            //  not the FROM class.
            //

            CIMPropertyList requiredPropertyList = _getPropertyList(
                queryExpr, sourceNameSpace, indication.getClassName());

            //
            //  If the subscription requires all properties,
            //  but supported property list does not include all
            //  properties, the subscription cannot be supported
            //
            if (requiredPropertyList.isNull ())
            {
                //
                //  Current subscription does not match
                //
                PEG_METHOD_EXIT();
                return false;
            }
            else
            {
                //
                //  Compare subscription required property list
                //  with supported property list
                //
                for (Uint32 j = 0; j < requiredPropertyList.size (); j++)
                {
                    if (!ContainsCIMName
                        (supportedPropertyList.getPropertyNameArray(),
                         requiredPropertyList[j]))
                    {
                        //
                        //  Current subscription does not match
                        //
                        PEG_METHOD_EXIT();
                        return false;
                    }
                }
            }
        }
        catch(const Exception & e)
        {
            // This subscription is invalid
           PEG_TRACE ((TRC_DISCARDED_DATA, Tracer::LEVEL1,
                "Exception caught trying to verify required properties "
                    "in a subscription are all contained in the list of "
                    "supported indication properties: %s",
                    (const char *) e.getMessage ().getCString())); 
            PEG_METHOD_EXIT();
            return false;
        }
        catch(const exception & e)
        {
            // This subscription is invalid
           PEG_TRACE ((TRC_DISCARDED_DATA, Tracer::LEVEL1,
                "Exception caught trying to verify required properties "
                    "in a subscription are all contained in the list of "
                    "supported indication properties: %s", e.what ())); 
            PEG_METHOD_EXIT();
            return false;
        }
        catch(...)
        {
            // This subscription is invalid
            // skip it
            PEG_TRACE_CSTRING (TRC_DISCARDED_DATA, Tracer::LEVEL1,
                "Unknown exception caught trying to verify "
                    "required properties in a subscription are all contained "
                    "in the list of supported indication properties."); 
            PEG_METHOD_EXIT();
            return false;
        }
    }

    //
    // Check for expired subscription
    //
    try
    {
        if (_isExpired(subscription))
        {
            // Delete expired subscription
            CIMObjectPath path = subscription.getPath ();
            _deleteExpiredSubscription (path);
            PEG_TRACE ((TRC_INDICATION_GENERATION, Tracer::LEVEL3,
                "%s Indication Subscription expired",
                (const char*)(indication.getClassName().getString().
                    getCString())));
            PEG_METHOD_EXIT();
            return false;
        }
    }
    catch (DateTimeOutOfRangeException&)
    {
        PEG_TRACE_CSTRING(TRC_INDICATION_SERVICE, Tracer::LEVEL2,
            "Caught DateTimeOutOfRangeException in IndicationService while"
                "checking for expired subscription");
        PEG_METHOD_EXIT();
        return false;
    }

    //
    // Evaluate whether the filter criteria are met by the generated 
    // indication
    //
    if (!queryExpr.evaluate(indication))
    {
        PEG_METHOD_EXIT();
        return false;
    }

    PEG_METHOD_EXIT();
    return true;
}

Boolean IndicationService::_formatIndication(
    CIMInstance& formattedIndication,
    QueryExpression& queryExpr,
    const Array<CIMName>& providerSupportedProperties,
    const Array<CIMName>& indicationClassProperties)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_formatIndication");

    //
    // Call QueryExpression::applyProjection to remove properties
    // not listed in the SELECT clause.  Note: for CQL,
    // this will handle properties on embedded objects.
    //
    // QueryExpression::applyProjection throws an exception if
    // the indication is missing a required property in the SELECT
    // clause.  Although we have checked for the indication missing
    // required properties, it would have not detected missing required
    // embedded object properties for CQL.  So, we need to catch the
    // missing property exception here.
    //
    try
    {
        queryExpr.applyProjection(formattedIndication, true);
    }
    catch (QueryRuntimePropertyException& re)
    {
        // The indication was missing a required property.
        PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL1,
            "Apply Projection error: %s",
            (const char*)re.getMessage().getCString()));
        PEG_METHOD_EXIT();
        return false; 
    }

    //
    // Remove any properties that may be left on the indication
    // that are not in the indication class.  These are properties
    // added by the provider incorrectly.  It is possible that
    // these properties will remain after applyProjection if the
    // SELECT clause happens to have a property name not on the
    // indication class, and the indication has that same property.
    // Note: If SELECT includes all properties ("*"), it's still
    // necessary to check, in case the provider added properties
    // not in the indication class.
    //
    for (Uint32 j = 0; j < providerSupportedProperties.size(); j++)
    {
        Uint32 rmIndex =
            formattedIndication.findProperty(providerSupportedProperties[j]);
        if (rmIndex != PEG_NOT_FOUND &&
            !ContainsCIMName(
                 indicationClassProperties, providerSupportedProperties[j]))
        {
            formattedIndication.removeProperty(rmIndex);
        }
    }

    PEG_METHOD_EXIT();
    return true;
}

void IndicationService::_forwardIndToHandler(
    const CIMInstance& matchedSubscription,
    const CIMInstance& handlerInstance,
    const CIMInstance& formattedIndication,
    const CIMNamespaceName& namespaceName,
    const OperationContext& operationContext)
{
    PEG_METHOD_ENTER(TRC_INDICATION_SERVICE,
        "IndicationService::_forwardIndToHandler");

    CIMRequestMessage * handler_request =
        new CIMHandleIndicationRequestMessage (
            XmlWriter::getNextMessageId (),
            namespaceName,
            handlerInstance,
            formattedIndication,
            matchedSubscription,
            QueueIdStack(_handlerService, getQueueId()),
            String::EMPTY,
            String::EMPTY);

    handler_request->operationContext = operationContext;

    AsyncOpNode* op = this->get_op();

    AsyncLegacyOperationStart *async_req =
        new AsyncLegacyOperationStart(
        op,
        _handlerService,
        handler_request);

    PEG_TRACE((TRC_INDICATION_SERVICE, Tracer::LEVEL4,
        "Sending (SendAsync) Indication to %s "
        "via CIMHandleIndicationRequestMessage",
        (MessageQueue::lookup(_handlerService) ?
         MessageQueue::lookup(_handlerService)->getQueueName() :
        "BAD queue name")));

    SendAsync (op,
               _handlerService,
               IndicationService::_handleIndicationCallBack,
               this,
               (void *) &(matchedSubscription));


    PEG_METHOD_EXIT();
}

PEGASUS_NAMESPACE_END
