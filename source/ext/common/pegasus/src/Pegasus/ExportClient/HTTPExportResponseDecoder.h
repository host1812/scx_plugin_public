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

#ifndef Pegasus_HTTPExportResponseDecoder_h
#define Pegasus_HTTPExportResponseDecoder_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Message.h>
#include <Pegasus/Common/Exception.h>
#include <Pegasus/Common/HTTPMessage.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/XmlParser.h>
#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/ExportClient/Linkage.h>

PEGASUS_NAMESPACE_BEGIN

/**
    The ClientExceptionMessage class represents an exception encapsulated in a
    response message.  A ClientExceptionMessage is generated by the HTTP Export
    Response Decoder when an error is detected in the export response.
*/
class ClientExceptionMessage : public Message
{
public:
    /**
        Constructs a ClientExceptionMessage.

        @param  clientException_  INPUT   Exception* exception to be
                                            encapsulated in the message
     */
    ClientExceptionMessage(Exception* clientException_)
        :
        Message(CLIENT_EXCEPTION_MESSAGE),
        clientException(clientException_)
    {
    }

    Exception* clientException;
};

/**
    The HTTPExportResponseDecoder class provides interfaces to parse and
    validate HTTP headers, and decode an export response message.
*/
class PEGASUS_EXPORT_CLIENT_LINKAGE HTTPExportResponseDecoder
{
public:

    /**
        Parses the headers and status line of the HTTP message.

        This method is called by the CIMExportResponseDecoder before calling
        the client authenticator to check the response header for an
        authentication challenge.

        @param  httpMessage       INPUT   HTTPMessage* message to be processed
        @param  exceptionMessage  OUTPUT  ClientExceptionMessage* response
                                            containing exception when error is
                                            detected in export response message
        @param  headers           OUTPUT  Array<HTTPHeader> containing headers
        @param  contentLength     OUTPUT  Uint32 length of message content
        @param  statusCode        OUTPUT  Uint32 status code from status line
        @param  reasonPhrase      OUTPUT  String reasonPhrase from status line
        @param  cimReconnect      OUTPUT  Boolean indicating whether close and
                                            reconnect are necessary
        @param  valid             OUTPUT  Boolean indicating whether any errors
                                            were encountered
     */
    static void parseHTTPHeaders(
        HTTPMessage* httpMessage,
        ClientExceptionMessage*& exceptionMessage,
        Array<HTTPHeader>& headers,
        Uint32& contentLength,
        Uint32& statusCode,
        String& reasonPhrase,
        Boolean& cimReconnect,
        Boolean& valid);

    /**
        Validates the HTTP headers of the HTTP message.

        This method is called by the CIMExportResponseDecoder after calling
        the client authenticator to check the response header for an
        authentication challenge.

        @param  httpMessage       INPUT   HTTPMessage* message to be processed
        @param  headers           INPUT   Array<HTTPHeader> containing headers
        @param  contentLength     INPUT   Uint32 length of message content
        @param  statusCode        INPUT   Uint32 status code from status line
        @param  cimReconnect      INPUT   Boolean indicating whether close and
                                            reconnect are necessary
        @param  reasonPhrase      INPUT   String reasonPhrase from status line
        @param  content           OUTPUT  char* containing message content
        @param  exceptionMessage  OUTPUT  ClientExceptionMessage* response
                                            containing exception when error is
                                            detected in export response message
        @param  valid             OUTPUT  Boolean indicating whether any errors
                                            were encountered
     */
    static void validateHTTPHeaders(
        HTTPMessage* httpMessage,
        Array<HTTPHeader>& headers,
        Uint32 contentLength,
        Uint32 statusCode,
        Boolean cimReconnect,
        const String& reasonPhrase,
        char*& content,
        ClientExceptionMessage*& exceptionMessage,
        Boolean& valid);

    /**
        Decodes the Export Response in the HTTP message.

        @param  content           INPUT   char* containing message content
        @param  cimReconnect      INPUT   Boolean indicating whether close and
                                            reconnect are necessary
        @param  responseMessage   OUTPUT  Message* response containing either
                                            export indication response, or
                                            exception when error is detected
     */
    static void decodeExportResponse(
        char* content,
        Boolean cimReconnect,
        Message*& responseMessage);

private:

    /**
        Decodes an Export Indication response.

        @param   parser                       XmlParser the XML parser
        @param   messageId                    String ID from MESSAGE element
        @param   isEmptyExpMethodResponseTag  Boolean indicating whether
                                                EXPMETHODRESPONSE was empty tag

        @return  pointer to a CIM Export Indication Response Message
     */
    static CIMExportIndicationResponseMessage* _decodeExportIndicationResponse(
        XmlParser& parser,
        const String& messageId,
        Boolean isEmptyExpMethodResponseTag);
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_HTTPExportResponseDecoder_h */
