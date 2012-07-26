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
//%////////////////////////////////////////////////////////////////////////////

#include <cctype>
#include <cstdio>
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/MessageLoader.h>
#include <Pegasus/Common/StringConversion.h>
#include <Pegasus/Common/AutoPtr.h>
#include "WsmConstants.h"
#include "WsmProcessor.h"

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

Uint64 WsmProcessor::_currentEnumContext = 0;

WsmProcessor::WsmProcessor(
    MessageQueueService* cimOperationProcessorQueue,
    CIMRepository* repository)
    : MessageQueueService(PEGASUS_QUEUENAME_WSMPROCESSOR),
      _wsmResponseEncoder(),
      _wsmRequestDecoder(this),
      _cimOperationProcessorQueue(cimOperationProcessorQueue),
      _repository(repository),
      _wsmToCimRequestMapper(repository)
{
}

WsmProcessor::~WsmProcessor()
{
}

void WsmProcessor::handleEnqueue(Message* message)
{
    if (!message)
    {
        return;
    }

    PEGASUS_ASSERT(dynamic_cast<CIMResponseMessage*>(message) != 0);
    handleResponse(dynamic_cast<CIMResponseMessage*>(message));
}

void WsmProcessor::handleEnqueue()
{
    Message* message = dequeue();
    handleEnqueue(message);
}

void WsmProcessor::handleRequest(WsmRequest* wsmRequest)
{
    PEG_METHOD_ENTER(TRC_WSMSERVER, "WsmProcessor::handleRequest()");

    // Process requests by type.  For now, only WS-Transfer operations are
    // implemented, and they all are handled by forwarding to the CIM Server.

    AutoPtr<WsmRequest> wsmRequestDestroyer(wsmRequest);

    try
    {
        CIMOperationRequestMessage* cimRequest =
            _wsmToCimRequestMapper.mapToCimRequest(wsmRequest);

        // Requests that do not have a CIM representation are mapped to NULL
        // and are meant to be handled by the WSM processor itself.
        if (cimRequest)
        {

            // Save the request until the response comes back.
            // Note that the CIM request has its own unique message ID.
            {
                AutoMutex am(_mutex);
                _requestTable.insert(cimRequest->messageId, wsmRequest);
                wsmRequestDestroyer.release();
            }

            cimRequest->queueIds.push(getQueueId());
            _cimOperationProcessorQueue->enqueue(cimRequest);
        }
        else
        {
            switch (wsmRequest->getType())
            {
                case WS_ENUMERATION_PULL:
                    _handlePullRequest((WsenPullRequest*) wsmRequest);
                    break;

                case WS_ENUMERATION_RELEASE:
                    _handleReleaseRequest((WsenReleaseRequest*) wsmRequest);
                    break;

                default:
                    break;
            }
        }
    }
    catch (WsmFault& fault)
    {
        sendResponse(new WsmFaultResponse(wsmRequest, fault));
    }
    catch (CIMException& e)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            _cimToWsmResponseMapper.mapCimExceptionToWsmFault(e)));
    }
    catch (Exception& e)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            WsmFault(
                WsmFault::wsman_InternalError,
                e.getMessage(),
                e.getContentLanguages())));
    }
    catch (PEGASUS_STD(exception)& e)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            WsmFault(WsmFault::wsman_InternalError, e.what())));
    }
    catch (...)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            WsmFault(WsmFault::wsman_InternalError)));
    }

    // Note this requirement when Enumerate/Pull operations are supported:
    // DSP0226 R6.3-5: For operations that span multiple message sequences,
    // the wsman:Locale element is processed in the initial message only.
    // It should be ignored in subsequent messages because the first
    // message establishes the required locale. The service may issue a
    // fault if the wsman:Locale is present in subsequent messages and the
    // value is different from that used in the initiating request.

    PEG_METHOD_EXIT();
}

void WsmProcessor::handleResponse(CIMResponseMessage* cimResponse)
{
    PEG_METHOD_ENTER(TRC_WSMSERVER, "WsmProcessor::handleResponse()");

    AutoPtr<CIMResponseMessage> cimResponseDestroyer(cimResponse);

    // Lookup the request this response corresponds to
    WsmRequest* wsmRequest = 0;
    {
        AutoMutex am(_mutex);
        Boolean flag = _requestTable.lookup(cimResponse->messageId, wsmRequest);
        PEGASUS_ASSERT(flag);
        _requestTable.remove(cimResponse->messageId);
    }
    AutoPtr<WsmRequest> wsmRequestDestroyer(wsmRequest);

    try
    {
        switch (wsmRequest->getType())
        {
            case WS_ENUMERATION_ENUMERATE:
                _handleEnumerateResponse(
                    cimResponse,
                    (WsenEnumerateRequest*) wsmRequest);
                break;

            default:
                _handleDefaultResponse(cimResponse, wsmRequest);
                break;
        }
    }
    catch (WsmFault& fault)
    {
        sendResponse(new WsmFaultResponse(wsmRequest, fault));
    }
    catch (CIMException& e)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            _cimToWsmResponseMapper.mapCimExceptionToWsmFault(e)));
    }
    catch (Exception& e)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            WsmFault(
                WsmFault::wsman_InternalError,
                e.getMessage(),
                e.getContentLanguages())));
    }
    catch (PEGASUS_STD(exception)& e)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            WsmFault(WsmFault::wsman_InternalError, e.what())));
    }
    catch (...)
    {
        sendResponse(new WsmFaultResponse(
            wsmRequest,
            WsmFault(WsmFault::wsman_InternalError)));
    }

    PEG_METHOD_EXIT();
}

void WsmProcessor::sendResponse(WsmResponse* wsmResponse)
{
    PEG_METHOD_ENTER(TRC_WSMSERVER, "WsmProcessor::sendResponse()");
    _wsmResponseEncoder.enqueue(wsmResponse);

    delete wsmResponse;

    PEG_METHOD_EXIT();
}

Uint32 WsmProcessor::getWsmRequestDecoderQueueId()
{
    return _wsmRequestDecoder.getQueueId();
}

void WsmProcessor::_handleEnumerateResponse(
    CIMResponseMessage* cimResponse,
    WsenEnumerateRequest* wsmRequest)
{
    SoapResponse* soapResponse = 0;

    if (cimResponse->cimException.getCode() != CIM_ERR_SUCCESS)
    {
        _handleDefaultResponse(cimResponse, wsmRequest);
        return;
    }
    else
    {
        AutoMutex lock(_mutex);

        SharedPtr<WsenEnumerateResponse> wsmResponse(
            (WsenEnumerateResponse*) _cimToWsmResponseMapper.
                mapToWsmResponse(wsmRequest, cimResponse));

        // Get the enumeration expiration time
        CIMDateTime expiration;
        _getExpirationDatetime(wsmRequest->expiration, expiration);

        // Create a new context
        Uint64 contextId = _currentEnumContext++;

        SharedPtr<EnumerationContext> enumContext(new EnumerationContext(
                contextId,
                wsmRequest->enumerationMode,
                expiration, 
                wsmRequest->epr,
                wsmResponse));
        _enumerationContextTable.insert(contextId, enumContext);
        wsmResponse->setEnumerationContext(contextId);

        // Get the requsted chunk of results
        AutoPtr<WsenEnumerateResponse> splitResponse(
            _splitEnumerateResponse(wsmRequest, wsmResponse.get(), 
                wsmRequest->optimized ? wsmRequest->maxElements : 0));
        splitResponse->setEnumerationContext(contextId);

        // If no items are left in the orignal response, mark split 
        // response as complete
        if (wsmResponse->getSize() == 0)
        {
            splitResponse->setComplete();
        }

        // Encode the response (enqueue it below).
        soapResponse = _wsmResponseEncoder.encode(splitResponse.get());

        if (splitResponse->getSize() > 0)
        {
            // Add unprocessed items back to the context
            wsmResponse->merge(splitResponse.get());
        }

        // Remove the context if there are no instances left
        if (wsmResponse->getSize() == 0)
        {
            _enumerationContextTable.remove(contextId);
        }
    }

    // Enqueue after unlocking the _enumerationContextTable to avoid deadlock.
    if (soapResponse)
    {
        _wsmResponseEncoder.sendResponse(soapResponse);
        delete soapResponse;
    }
}

void WsmProcessor::_handlePullRequest(WsenPullRequest* wsmRequest)
{
    SoapResponse* soapResponse = 0;

    // Perform all this while locking _enumerationContextTable.
    {
        AutoMutex lock(_mutex);
        SharedPtr<EnumerationContext> enumContext;

        if (_enumerationContextTable.lookup(
                wsmRequest->enumerationContext, enumContext))
        {
            // EPRs of the request and the enumeration context must match
            if (wsmRequest->epr != enumContext->epr)
            {
                throw WsmFault(
                    WsmFault::wsa_MessageInformationHeaderRequired,
                    MessageLoaderParms(
                        "WsmServer.WsmProcessor.INVALID_PULL_EPR",
                        "EPR of a Pull request does not match that of "
                        "the enumeration context."));
            }

            AutoPtr<WsenPullResponse> wsmResponse(_splitPullResponse(
                wsmRequest, 
                enumContext->response.get(), 
                wsmRequest->maxElements));

            wsmResponse->setEnumerationContext(enumContext->contextId);
            if (enumContext->response->getSize() == 0)
            {
                wsmResponse->setComplete();
            }

            SharedPtr<WsenEnumerateResponse> response = enumContext->response;

            // Encode the response (enqueue it below).
            soapResponse = _wsmResponseEncoder.encode(wsmResponse.get());

            if (wsmResponse->getSize() > 0)
            {
                // Add unprocessed items back to the context
                response->merge(wsmResponse.get());
            }

            // Remove the context if there are no instances left
            if (response->getSize() == 0)
            {
                _enumerationContextTable.remove(wsmRequest->enumerationContext);
            }
        }
        else
        {
            throw WsmFault(
                WsmFault::wsen_InvalidEnumerationContext,
                MessageLoaderParms(
                    "WsmServer.WsmProcessor.INVALID_ENUMERATION_CONTEXT",
                    "Enumeration context \"$0\" is not valid.",
                    wsmRequest->enumerationContext));
        }
    }

    // Enqueue after unlocking the _enumerationContextTable to avoid deadlock.
    if (soapResponse)
    {
        _wsmResponseEncoder.sendResponse(soapResponse);
        delete soapResponse;
    }
}

void WsmProcessor::_handleReleaseRequest(WsenReleaseRequest* wsmRequest)
{
    SoapResponse* soapResponse = 0;

    // Perform all this while locking _enumerationContextTable.
    {
        AutoMutex lock(_mutex);
        SharedPtr<EnumerationContext> enumContext;

        if (_enumerationContextTable.lookup(
                wsmRequest->enumerationContext, enumContext))
        {
            // EPRs of the request and the enumeration context must match
            if (wsmRequest->epr != enumContext->epr)
            {
                throw WsmFault(
                    WsmFault::wsa_MessageInformationHeaderRequired,
                    MessageLoaderParms(
                        "WsmServer.WsmProcessor.INVALID_RELEASE_EPR",
                        "EPR of a Release request does not match that of "
                        "the enumeration context."));
            }

            AutoPtr<WsenReleaseResponse> wsmResponse(new WsenReleaseResponse(
                wsmRequest, enumContext->response->getContentLanguages()));

            _enumerationContextTable.remove(wsmRequest->enumerationContext);

            // Encode the response (enqueue it below).
            soapResponse = _wsmResponseEncoder.encode(wsmResponse.get());
        }
        else
        {
            throw WsmFault(
                WsmFault::wsen_InvalidEnumerationContext,
                MessageLoaderParms(
                    "WsmServer.WsmProcessor.INVALID_ENUMERATION_CONTEXT",
                    "Enumeration context \"$0\" is not valid.",
                    wsmRequest->enumerationContext));
        }
    }

    // Enqueue after unlocking the _enumerationContextTable to avoid deadlock.
    if (soapResponse)
    {
        _wsmResponseEncoder.sendResponse(soapResponse);
        delete soapResponse;
    }
}

void WsmProcessor::_handleDefaultResponse(
    CIMResponseMessage* cimResponse, WsmRequest* wsmRequest)
{
    AutoPtr<WsmResponse> wsmResponse(
        _cimToWsmResponseMapper.mapToWsmResponse(wsmRequest, cimResponse));

    cimResponse->updateThreadLanguages();
    cimResponse->queueIds.pop();

    _wsmResponseEncoder.enqueue(wsmResponse.get());
}

WsenEnumerateResponse* WsmProcessor::_splitEnumerateResponse(
    WsenEnumerateRequest* request, WsenEnumerateResponse* response, Uint32 num)
{
    WsenEnumerationData splitData;
    response->getEnumerationData().split(splitData, num);

    return new WsenEnumerateResponse(splitData, response->getItemCount(),
        request, response->getContentLanguages());
}

WsenPullResponse* WsmProcessor::_splitPullResponse(
    WsenPullRequest* request, WsenEnumerateResponse* response, Uint32 num)
{
    WsenEnumerationData splitData;
    response->getEnumerationData().split(splitData, num);

    return new WsenPullResponse(splitData, request, 
        response->getContentLanguages());
}

void WsmProcessor::_getExpirationDatetime(
    const String& wsmDT, CIMDateTime& cimDT)
{
    CIMDateTime dt, currentDT;

    // Default expiration interval = 10 mins 
    // ATTN WSMAN: what should the value be?
    CIMDateTime maxInterval(0, 0, 10, 0, 0, 6);

    // If expiration is not set, use the dafault.
    if (wsmDT == String::EMPTY)
    {
        dt = maxInterval;
    }
    else
    {
        try
        {
            WsmToCimRequestMapper::convertWsmToCimDatetime(wsmDT, dt);
        }
        catch (...)
        {
            throw WsmFault(
                WsmFault::wsen_InvalidExpirationTime,
            MessageLoaderParms(
                "WsmServer.WsmToCimRequestMapper.INVALID_EXPIRATION_TIME",
                "The expiration time \"$0\" is not valid", wsmDT));
        }
    }

    currentDT = CIMDateTime::getCurrentDateTime();
    if (dt.isInterval())
    {
        if (dt > maxInterval)
        {
            dt = maxInterval;
        }
        cimDT = currentDT + dt;
    }
    else
    {
        if ((dt <= currentDT))
        {
            throw WsmFault(
                WsmFault::wsen_InvalidExpirationTime,
            MessageLoaderParms(
                "WsmServer.WsmToCimRequestMapper.INVALID_EXPIRATION_TIME",
                "The expiration time \"$0\" is not valid", wsmDT));
        }

        if (dt - currentDT > maxInterval)
        {
            cimDT = currentDT + maxInterval;
        }
        else
        {
            cimDT = dt;
        }
    }
}

void WsmProcessor::cleanupExpiredContexts()
{
    CIMDateTime currentDT = CIMDateTime::getCurrentDateTime();
    Array<Uint64> expiredContextIds;

    AutoMutex lock(_mutex);

    for (EnumerationContextTable::Iterator i =
             _enumerationContextTable.start (); i; i++)
    {
        SharedPtr<EnumerationContext> context = i.value();
        if (context->expiration < currentDT)
        {
            expiredContextIds.append(context->contextId);
        }
    }

    for (Uint32 i = 0; i < expiredContextIds.size(); i++)
    {
        _enumerationContextTable.remove(expiredContextIds[i]);
    }
}

PEGASUS_NAMESPACE_END
