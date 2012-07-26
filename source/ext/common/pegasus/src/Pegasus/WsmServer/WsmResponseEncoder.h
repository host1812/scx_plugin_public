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

#ifndef Pegasus_WsmResponseEncoder_h
#define Pegasus_WsmResponseEncoder_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/WsmServer/WsmResponse.h>
#include <Pegasus/WsmServer/SoapResponse.h>

PEGASUS_NAMESPACE_BEGIN

class WsmResponse;
class SoapResponse;

/** This class encodes WS-Man operation requests and passes them up-stream.
 */
class WsmResponseEncoder
{
public:

    WsmResponseEncoder();
    ~WsmResponseEncoder();

    // Format the response into a SoapResponse. The caller should eventually
    // pass the return value to sendResponse().
    SoapResponse* encode(WsmResponse* response);

    // Send the SoapResponse back to HTTP client.
    void sendResponse(const SoapResponse* response);

    // Enqueue and the send thr response back to HTTP client.
    void enqueue(WsmResponse* response);

private:

    SoapResponse* _encodeUnreportableSuccess(WsmResponse* response);
    SoapResponse* _encodeEncodingLimitFault(WsmResponse* response);

    SoapResponse* _encodeWxfGetResponse(WxfGetResponse* response);
    SoapResponse* _encodeWxfPutResponse(WxfPutResponse* response);
    SoapResponse* _encodeWxfCreateResponse(WxfCreateResponse* response);
    SoapResponse* _encodeWxfDeleteResponse(WxfDeleteResponse* response);
    SoapResponse* _encodeWsenEnumerateResponse(WsenEnumerateResponse* response);
    SoapResponse* _encodeWsenPullResponse(WsenPullResponse* response);
    SoapResponse* _encodeWsenReleaseResponse(WsenReleaseResponse* response);
    SoapResponse* _encodeWsmFaultResponse(WsmFaultResponse* response);
    SoapResponse* _encodeSoapFaultResponse(SoapFaultResponse* response);
    SoapResponse* _encodeWsInvokeResponse(WsInvokeResponse* response);

    Boolean _encodeEnumerationData(
        SoapResponse& soapResponse,
        Buffer& headers,
        WsmOperationType operation,
        Uint64 contextId,
        Boolean isComplete,
        WsenEnumerationData& data,
        const String& resourceUri);
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_WsmResponseEncoder_h */
