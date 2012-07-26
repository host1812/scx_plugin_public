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

#ifndef Pegasus_IndicationProvider_h
#define Pegasus_IndicationProvider_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Provider/CIMIndicationProvider.h>
#include <Pegasus/Provider/CIMMethodProvider.h>


class IndicationProvider :
    public PEGASUS_NAMESPACE(CIMMethodProvider),
    public PEGASUS_NAMESPACE(CIMIndicationProvider)
{
public:
    IndicationProvider() throw ();
    virtual ~IndicationProvider() throw ();

    // CIMProvider interface
    virtual void initialize(PEGASUS_NAMESPACE(CIMOMHandle)& cimom);
    virtual void terminate();

    // CIMIndicationProvider interface
    virtual void enableIndications(
        PEGASUS_NAMESPACE(IndicationResponseHandler)& handler);
    virtual void disableIndications();

    virtual void createSubscription(
        const PEGASUS_NAMESPACE(OperationContext)& context,
        const PEGASUS_NAMESPACE(CIMObjectPath)& subscriptionName,
        const PEGASUS_NAMESPACE(Array) <PEGASUS_NAMESPACE(CIMObjectPath)>&
            classNames,
        const PEGASUS_NAMESPACE(CIMPropertyList)& propertyList,
        const PEGASUS_NAMESPACE(Uint16) repeatNotificationPolicy);

    virtual void modifySubscription(
        const PEGASUS_NAMESPACE(OperationContext)& context,
        const PEGASUS_NAMESPACE(CIMObjectPath)& subscriptionName,
        const PEGASUS_NAMESPACE(Array) <PEGASUS_NAMESPACE(CIMObjectPath)>&
            classNames,
        const PEGASUS_NAMESPACE(CIMPropertyList)& propertyList,
        const PEGASUS_NAMESPACE(Uint16) repeatNotificationPolicy);

    virtual void deleteSubscription(
        const PEGASUS_NAMESPACE(OperationContext)& context,
        const PEGASUS_NAMESPACE(CIMObjectPath)& subscriptionName,
        const PEGASUS_NAMESPACE(Array) <PEGASUS_NAMESPACE(CIMObjectPath)>&
            classNames);

    // CIMMethodProvider Interface
    virtual void invokeMethod(
        const PEGASUS_NAMESPACE(OperationContext)& context,
        const PEGASUS_NAMESPACE(CIMObjectPath)& objectReference,
        const PEGASUS_NAMESPACE(CIMName)& methodName,
        const PEGASUS_NAMESPACE(Array)<PEGASUS_NAMESPACE(CIMParamValue)>&
            inParameters,
        PEGASUS_NAMESPACE(MethodResultResponseHandler)& handler);
};

#endif
