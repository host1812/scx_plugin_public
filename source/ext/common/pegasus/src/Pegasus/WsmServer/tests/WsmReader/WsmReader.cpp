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
//%/////////////////////////////////////////////////////////////////////////////

#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/String.h>

#include <Pegasus/WsmServer/WsmConstants.h>
#include <Pegasus/WsmServer/WsmReader.h>
#include <Pegasus/WsmServer/WsmWriter.h>
#include <Pegasus/WsmServer/WsmValue.h>
#include <Pegasus/WsmServer/WsmEndpointReference.h>
#include <Pegasus/WsmServer/WsmInstance.h>
#include <Pegasus/WsmServer/WsmSelectorSet.h>

PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

static Boolean verbose;

static void _testSelectors(WsmReader& reader)
{
    XmlEntry entry;
    reader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Envelope");

    //
    // This section tests acceptable input for selectors/EPRs
    //
    WsmSelector sel;

    // Test regular value selector
    if (!reader.getSelectorElement(sel))
        throw Exception("Expected selector element.");

    if (sel.type != WsmSelector::VALUE || sel.name != "first" || 
        sel.value != "John")
        throw Exception(
            "Invalid selector element. Expected name=first, value=John");

    // Test empty value selector
    if (!reader.getSelectorElement(sel))
        throw Exception("Expected selector element.");

    if (sel.type != WsmSelector::VALUE || sel.name != "last" || 
        sel.value != String::EMPTY)
        throw Exception("Expected value selector: name=last, value=<empty>");

    // Test EPR selector
    if (!reader.getSelectorElement(sel))
        throw Exception("Expected selector element.");

    if (sel.type != WsmSelector::EPR || sel.name != "CityOfBirth" ||
        sel.epr.address != "http://www.acme.com:5988/wsman" ||
        sel.epr.resourceUri != "City" ||
        sel.epr.selectorSet->selectors[0].name != "Name" ||
        sel.epr.selectorSet->selectors[0].value != "San Jose" ||
        sel.epr.selectorSet->selectors[1].name != "State" ||
        sel.epr.selectorSet->selectors[1].value != "CA")
        throw Exception("Expected EPR selector: name=CityOfBirth");

    // Test recursive EPR selector
    if (!reader.getSelectorElement(sel))
        throw Exception("Expected selector element.");

    // Now we need to veryfy this thing...
    if (sel.type != WsmSelector::EPR || sel.name != "Recursive_0" ||
        sel.epr.address != "http://www.acme_0.com:5988/wsman" ||
        sel.epr.resourceUri != "ResourceURI_0")
        throw Exception("Invalid recursive EPR at level 0.");

    WsmSelector& sel_1 = sel.epr.selectorSet->selectors[0];
    if (sel_1.type != WsmSelector::EPR || sel_1.name != "Recursive_1" ||
        sel_1.epr.address != "http://www.acme_1.com:5988/wsman" ||
        sel_1.epr.resourceUri != "ResourceURI_1")
        throw Exception("Invalid recursive EPR at level 1.");

    WsmSelector& sel_2 = sel_1.epr.selectorSet->selectors[0];
    if (sel_2.type != WsmSelector::EPR || sel_2.name != "Recursive_2" ||
        sel_2.epr.address != "http://www.acme_2.com:5988/wsman" ||
        sel_2.epr.resourceUri != "ResourceURI_2" ||
        sel_2.epr.selectorSet->selectors[0].name != "Name" ||
        sel_2.epr.selectorSet->selectors[0].value != "San Jose" ||
        sel_2.epr.selectorSet->selectors[1].name != "State" ||
        sel_2.epr.selectorSet->selectors[1].value != "CA")
        throw Exception("Invalid recursive EPR at level 2.");
}

static void _testSelectorErrors(WsmReader& reader)
{
    XmlEntry entry;
    WsmSelector sel;

    //
    // This section tests exceptions in Selectors
    //

    // Test no Name attrubute in a selector
    try
    {
        if (reader.getSelectorElement(sel))
            throw Exception("Expected no Name attrubute exception.");
    }
    catch (XmlException&)
    {
        reader.expectContentOrCData(entry);
        reader.expectEndTag(WsmNamespaces::WS_MAN, "Selector");
    }

    // Test unexpected content in a selector
    try
    {
        if (reader.getSelectorElement(sel))
            throw Exception("Expected bad content exception.");
    }
    catch (XmlException&)
    {
        reader.expectEndTag(WsmNamespaces::WS_MAN, "Selector");
    }

    // Test empty selector set
    WsmSelectorSet selSet;
    try
    {
        if (reader.getSelectorSetElement(selSet))
            throw Exception("Expected empty selector set exception.");
    }
    catch (XmlException&)
    {
    }
    
    // Test garbage in a selector set
    try
    {
        if (reader.getSelectorSetElement(selSet))
            throw Exception("Expected garbage in selector set exception.");
    }
    catch (XmlException&)
    {
        reader.expectEndTag(WsmNamespaces::WS_MAN, "SelectorSet");
    }
    
    // Test more garbage in a selector set
    try
    {
        if (reader.getSelectorSetElement(selSet))
            throw Exception("Expected garbage in selector set exception.");
    }
    catch (XmlException&)
    {
        reader.expectEndTag(WsmNamespaces::WS_MAN, "SelectorSet");
    }

    //
    // This section tests exceptions in EPRs
    //

    WsmEndpointReference epr;

    // wsa:Address1 element is incorrect
    try
    {
        if (reader.getSelectorEPRElement(epr))
            throw Exception("Expected garbage in selector EPR exception.");
    }
    catch (XmlException&)
    {
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               entry.nsType != WsmNamespaces::WS_ADDRESSING ||
               strcmp(entry.localName, "EndpointReference") != 0));
    }

    // missing wsa:ReferenceParameters element
    try
    {
        if (reader.getSelectorEPRElement(epr))
            throw Exception("Expected garbage in selector EPR exception.");
    }
    catch (XmlException&)
    {
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               entry.nsType != WsmNamespaces::WS_ADDRESSING ||
               strcmp(entry.localName, "EndpointReference") != 0));
    }

    // ATTN WSMAN: It's unclear which elements of an EPR are optional. 
    // The implementation hasn't been finalized yet...
    // Add test points for exceptions in EPR XML.


    reader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Envelope");
}

static void _testProperties(WsmReader& reader)
{
    XmlEntry entry;
    reader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Envelope");

    //
    // This section tests valid simple (non-array) properties
    //

    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        if (inst.getPropertyCount() != 7)
            throw Exception("Expected 7 properties.");

        {
            WsmProperty& prop = inst.getProperty(0);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_1" || 
                val.getType() != WSMTYPE_OTHER)
                throw Exception("Expected string property 1.");

            String str;
            val.get(str);
            if (str != "Property value 1" || val.isNull() || val.isArray())
                throw Exception("Invalid property 1.");
        }

        {
            WsmProperty& prop = inst.getProperty(1);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_2" || 
                val.getType() != WSMTYPE_OTHER)
                throw Exception("Expected string property 2.");
            
            String str;
            val.get(str);
            if (str != "222" || val.isNull() || val.isArray())
                throw Exception("Invalid property 2.");
        }
        
        {
            WsmProperty& prop = inst.getProperty(2);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_3" || 
                !val.isNull() || val.isArray())
                throw Exception("Expected NULL property 3.");
        }
        
        {
            WsmProperty& prop = inst.getProperty(3);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_4" || 
                !val.isNull() || val.isArray())
                throw Exception("Expected NULL property 4.");
        }

        {
            WsmProperty& prop = inst.getProperty(4);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_5" || 
                val.getType() != WSMTYPE_OTHER)
                throw Exception("Expected string property 5.");
            
            String str;
            val.get(str);
            if (str != "" || val.isNull() || val.isArray())
                throw Exception("Invalid property 5.");
        }
        
        {
            WsmProperty& prop = inst.getProperty(5);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_6" || 
                val.getType() != WSMTYPE_REFERENCE ||
                val.isNull() || val.isArray())
                throw Exception("Expected EPR property 6.");
            
            WsmEndpointReference epr;
            val.get(epr);
            if (epr.address != "http://www.acme.com:5988/wsman" ||
                epr.resourceUri != "ResourceURI_6" ||
                epr.selectorSet->selectors[0].name != "first" ||
                epr.selectorSet->selectors[0].value != "John" ||
                epr.selectorSet->selectors[1].name != "last" ||
                epr.selectorSet->selectors[1].value != "Doe")
                throw Exception("Invalid EPR value."); 
        }
        
        {
            WsmProperty& prop = inst.getProperty(6);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_7" || 
                val.getType() != WSMTYPE_INSTANCE ||
                val.isNull() || val.isArray())
                throw Exception("Expected instance property 7.");
            
            WsmInstance inst1;
            val.get(inst1);
            if (inst1.getClassName() != "Class1" || 
                inst1.getPropertyCount() != 2)
                throw Exception("Invalid instance value."); 

            WsmProperty& prop1 = inst1.getProperty(0);
            WsmProperty& prop2 = inst1.getProperty(1);
            WsmValue& val1 = prop1.getValue();
            WsmValue& val2 = prop2.getValue();
            if (prop1.getName() != "prop_1" || 
                val1.getType() != WSMTYPE_OTHER ||
                prop2.getName() != "prop_2" || 
                val2.getType() != WSMTYPE_OTHER ||
                val1.isNull() || val1.isArray() || 
                val2.isNull() || val2.isArray())
                throw Exception("Invalid instance value."); 

            String str1, str2;
            val1.get(str1);
            val2.get(str2);
            if (str1 != "1111" || str2 != "Property value 2")
                throw Exception("Invalid instance value."); 
        }
    }

    //
    // This section tests String array properties
    //
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        if (inst.getPropertyCount() != 1)
            throw Exception("Expected 1 property.");

        WsmProperty& prop = inst.getProperty(0);
        WsmValue& val = prop.getValue();
        if (prop.getName() != "property_1" || 
            val.getType() != WSMTYPE_OTHER ||
            val.isNull() || !val.isArray())
            throw Exception("Expected string array property.");

        Array<String> stra;
        val.get(stra);
        if (stra.size() != 4 || stra[0] != "100" ||
            stra[1] != "101" || stra[2] != "102" || stra[3] != "103")
            throw Exception("Invalid string array value.");
            
    }

    //
    // This section tests EPR array properties
    //
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        if (inst.getPropertyCount() != 1)
            throw Exception("Expected 1 property.");

        WsmProperty& prop = inst.getProperty(0);
        WsmValue& val = prop.getValue();
        if (prop.getName() != "property_1" || 
            val.getType() != WSMTYPE_REFERENCE ||
            val.isNull() || !val.isArray())
            throw Exception("Expected EPR array property.");

        Array<WsmEndpointReference> epra;
        val.get(epra);
        if (epra.size() != 3 ||
            epra[0].address != "http://www.acme_1.com:5988/wsman" ||
            epra[0].resourceUri != "ResourceURI_1" ||
            epra[0].selectorSet->selectors[0].name != "first" ||
            epra[0].selectorSet->selectors[0].value != "John" ||
            epra[1].address != "http://www.acme_2.com:5988/wsman" ||
            epra[1].resourceUri != "ResourceURI_2" ||
            epra[1].selectorSet->selectors[0].name != "first" ||
            epra[1].selectorSet->selectors[0].value != "Jack" ||
            epra[2].address != "http://www.acme_3.com:5988/wsman" ||
            epra[2].resourceUri != "ResourceURI_3" ||
            epra[2].selectorSet->selectors[0].name != "first" ||
            epra[2].selectorSet->selectors[0].value != "Fred")
            throw Exception("Invalid EPR array value.");
    }

    //
    // This section tests instance array properties
    //
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        if (inst.getPropertyCount() != 1)
            throw Exception("Expected 1 property.");

        WsmProperty& prop = inst.getProperty(0);
        WsmValue& val = prop.getValue();
        if (prop.getName() != "property_1" || 
            val.getType() != WSMTYPE_INSTANCE ||
            val.isNull() || !val.isArray())
            throw Exception("Expected instance array property.");

        Array<WsmInstance> insta;
        val.get(insta);

        if (insta.size() != 3 ||
            insta[0].getPropertyCount() != 1 ||
            insta[1].getPropertyCount() != 1 ||
            insta[2].getPropertyCount() != 1)
            throw Exception("Invalid instance array value.");

        WsmProperty& prop1 = insta[0].getProperty(0);
        WsmProperty& prop2 = insta[1].getProperty(0);
        WsmProperty& prop3 = insta[2].getProperty(0);
        WsmValue& val1 = prop1.getValue();
        WsmValue& val2 = prop2.getValue();
        WsmValue& val3 = prop3.getValue();

        if (prop1.getName() != "prop_1" || val1.getType() != WSMTYPE_OTHER ||
            val1.isNull() || val1.isArray() ||
            prop2.getName() != "prop_1" || val2.getType() != WSMTYPE_OTHER ||
            val2.isNull() || val2.isArray() ||
            prop3.getName() != "prop_1" || val3.getType() != WSMTYPE_OTHER ||
            val3.isNull() || val3.isArray())
            throw Exception("Invalid instance array value.");
        
        String str1, str2, str3;
        val1.get(str1);
        val2.get(str2);
        val3.get(str3);
        if (str1 != "1111" || str2 != "2222" || str3 != "3333")
            throw Exception("Invalid instance array value.");
    }
}

static void _testPropertyErrors(WsmReader& reader)
{
    //
    // This section tests errors in property XML
    //

    // Test garbage in property value
    try
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception("Expected garbage in property value exception.");
    }
    catch (XmlException&)
    {
        XmlEntry entry;
        reader.next(entry);
        reader.next(entry);
    }

    // Test null elements in an array property value
    try
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception("Expected null element in array value exception.");
    }
    catch (XmlException&)
    {
        XmlEntry entry;
        reader.next(entry);
        reader.next(entry);
        reader.next(entry);
        reader.next(entry);
        reader.next(entry);
        reader.next(entry);
    }

    try
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception("Expected null element in array value exception.");
    }
    catch (XmlException&)
    {
        XmlEntry entry;
        reader.next(entry);
        reader.next(entry);
        reader.next(entry);
        reader.next(entry);
    }

    try
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception("Expected null element in array value exception.");
    }
    catch (XmlException&)
    {
        XmlEntry entry;
        reader.next(entry);
    }

    // Test empty property tag with no xsi:nil="true"
    try
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception(
            "Expected empty property tag with no xsi:nil exception.");
    }
    catch (XmlException&)
    {
        XmlEntry entry;
        reader.next(entry);
    }

    // Test error in EPR property
    try
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception(
            "Expected error in EPR property value exception.");
    }
    catch (XmlException&)
    {
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Class") != 0));
    }

    // Test error in instance property
    try
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception(
            "Expected error in instance property value exception.");
    }
    catch (XmlException&)
    {
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Class") != 0));
    }

    reader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Envelope");
}

static void _testInstances(WsmReader& reader)
{
    XmlEntry entry;
    reader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Envelope");

    // Test instance with multiple array properties of different types
    {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        if (inst.getPropertyCount() != 3)
            throw Exception("Expected 3 properties.");

        {
            WsmProperty& prop = inst.getProperty(0);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_1" || 
                val.getType() != WSMTYPE_OTHER ||
                val.isNull() || !val.isArray())
                throw Exception("Expected string array property.");

            Array<String> stra;
            val.get(stra);
            if (stra.size() != 4 || stra[0] != "100" ||
                stra[1] != "101" || stra[2] != "102" || stra[3] != "103")
                throw Exception("Invalid string array value.");
        }
        {
            WsmProperty& prop = inst.getProperty(1);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_2" || 
                val.getType() != WSMTYPE_REFERENCE ||
                val.isNull() || !val.isArray())
                throw Exception("Expected EPR array property.");

            Array<WsmEndpointReference> epra;
            val.get(epra);
            if (epra.size() != 3 ||
                epra[0].address != "http://www.acme_1.com:5988/wsman" ||
                epra[0].resourceUri != "ResourceURI_1" ||
                epra[0].selectorSet->selectors[0].name != "first" ||
                epra[0].selectorSet->selectors[0].value != "John" ||
                epra[1].address != "http://www.acme_2.com:5988/wsman" ||
                epra[1].resourceUri != "ResourceURI_2" ||
                epra[1].selectorSet->selectors[0].name != "first" ||
                epra[1].selectorSet->selectors[0].value != "Jack" ||
                epra[2].address != "http://www.acme_3.com:5988/wsman" ||
                epra[2].resourceUri != "ResourceURI_3" ||
                epra[2].selectorSet->selectors[0].name != "first" ||
                epra[2].selectorSet->selectors[0].value != "Fred")
                throw Exception("Invalid EPR array value.");
        }
        {
            WsmProperty& prop = inst.getProperty(2);
            WsmValue& val = prop.getValue();
            if (prop.getName() != "property_3" || 
                val.getType() != WSMTYPE_INSTANCE ||
                val.isNull() || !val.isArray())
                throw Exception("Expected instance array property.");
            
            Array<WsmInstance> insta;
            val.get(insta);
            
            if (insta.size() != 3 ||
                insta[0].getPropertyCount() != 1 ||
                insta[1].getPropertyCount() != 1 ||
                insta[2].getPropertyCount() != 1)
                throw Exception("Invalid instance array value.");

            WsmProperty& prop1 = insta[0].getProperty(0);
            WsmProperty& prop2 = insta[1].getProperty(0);
            WsmProperty& prop3 = insta[2].getProperty(0);
            WsmValue& val1 = prop1.getValue();
            WsmValue& val2 = prop2.getValue();
            WsmValue& val3 = prop3.getValue();
            
            if (prop1.getName() != "prop_1" || 
                val1.getType() != WSMTYPE_OTHER ||
                val1.isNull() || val1.isArray() ||
                prop2.getName() != "prop_1" || 
                val2.getType() != WSMTYPE_OTHER ||
                val2.isNull() || val2.isArray() ||
                prop3.getName() != "prop_1" || 
                val3.getType() != WSMTYPE_OTHER ||
                val3.isNull() || val3.isArray())
            throw Exception("Invalid instance array value.");
            
            String str1, str2, str3;
            val1.get(str1);
            val2.get(str2);
            val3.get(str3);
            if (str1 != "1111" || str2 != "2222" || str3 != "3333")
                throw Exception("Invalid instance array value.");
        }
    }
}

static void _testInstanceErrors(WsmReader& reader)
{
    // Invalid class name: namespace schema prefix is wrong
    try {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception(
            "Expected error in class name exception.");
    }
    catch (XmlException& e) {
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Class") != 0));
    }

    // Invalid instance: not namespace  qualified
    try {
        WsmInstance inst;
        reader.getInstanceElement(inst);
        throw Exception(
            "Expected error in class name exception.");
    }
    catch (XmlException& e) {
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Class") != 0));
    }

    reader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Envelope");
}

static void _testHeaders(WsmReader& reader)
{
    XmlEntry entry;
    reader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Envelope");

    String wsaMessageId;
    String wsaAction;
    String wsaFrom;
    String wsaReplyTo;
    String wsaFaultTo;
    WsmEndpointReference epr;
    Uint32 wsmMaxEnvelopeSize = 0;
    AcceptLanguageList wsmLocale;
    Boolean wsmRequestEpr = false;
    Boolean wsmRequestItemCount = false;
    reader.decodeRequestSoapHeaders(
        wsaMessageId, epr.address, wsaAction, wsaFrom, wsaReplyTo,
        wsaFaultTo, epr.resourceUri, *epr.selectorSet, wsmMaxEnvelopeSize,
        wsmLocale, wsmRequestEpr, wsmRequestItemCount);

    if (wsaMessageId != "1111")
        throw Exception("Invalid message ID");

    if (epr.address != "http://www.to_addr.com:5988/wsman" ||
        epr.resourceUri != WSM_RESOURCEURI_CIMSCHEMAV2 ||
        epr.selectorSet->selectors[0].type != WsmSelector::VALUE ||
        epr.selectorSet->selectors[0].name != "Name" ||
        epr.selectorSet->selectors[0].value != "San Jose")
        throw Exception("Invalid EPR");

    if (wsaAction !=  WSM_ACTION_GET)
        throw Exception("Invalid action");

    if (wsaFrom != "http://www.from_addr.com")
        throw Exception("Invalid From header");

    if (wsaReplyTo != "http://www.reply_to__addr.com")
        throw Exception("Invalid ReplyTo header");

    if (wsaFaultTo != "http://www.fault_to_addr.com")
        throw Exception("Invalid FaultTo header");

    if (wsmMaxEnvelopeSize != 12345)
        throw Exception("Invalid MaxEnvelopeSize");

    if (wsmRequestEpr != true)
        throw Exception("Invalid RequestEpr");

    if (wsmLocale.size() != 1 ||
        wsmLocale.getLanguageTag(0).getLanguage() != "en" ||
        wsmLocale.getLanguageTag(0).getCountry() != "us")
        throw Exception("Invalid Locale");
}

static void _testHeaderErrors(WsmReader& reader)
{
    // Test duplicate headers
    try
    {
        String wsaMessageId, wsaAction, wsaFrom, wsaReplyTo, wsaFaultTo;
        WsmEndpointReference epr;
        Uint32 wsmMaxEnvelopeSize = 0;
        AcceptLanguageList wsmLocale;
        Boolean wsmRequestEpr = false;
        Boolean wsmRequestItemCount = false;
        reader.decodeRequestSoapHeaders(
            wsaMessageId, epr.address, wsaAction, wsaFrom, wsaReplyTo,
            wsaFaultTo, epr.resourceUri, *epr.selectorSet, wsmMaxEnvelopeSize,
            wsmLocale, wsmRequestEpr, wsmRequestItemCount);
        throw Exception("Expected duplicate headers fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsa:InvalidMessageInformationHeader")
            throw Exception("Invalid duplicate header fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Header") != 0));
    }

    // Test MustUnderstand
    try
    {
        String wsaMessageId, wsaAction, wsaFrom, wsaReplyTo, wsaFaultTo;
        WsmEndpointReference epr;
        Uint32 wsmMaxEnvelopeSize = 0;
        AcceptLanguageList wsmLocale;
        Boolean wsmRequestEpr = false;
        Boolean wsmRequestItemCount = false;
        reader.decodeRequestSoapHeaders(
            wsaMessageId, epr.address, wsaAction, wsaFrom, wsaReplyTo,
            wsaFaultTo, epr.resourceUri, *epr.selectorSet, wsmMaxEnvelopeSize,
            wsmLocale, wsmRequestEpr, wsmRequestItemCount);
        throw Exception("Expected Soap NotUnderstood fault");
    }
    catch (SoapNotUnderstoodFault&)
    {
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Header") != 0));
    }

    // Test OperationTimeout unsupported feature
    try
    {
        String wsaMessageId, wsaAction, wsaFrom, wsaReplyTo, wsaFaultTo;
        WsmEndpointReference epr;
        Uint32 wsmMaxEnvelopeSize = 0;
        AcceptLanguageList wsmLocale;
        Boolean wsmRequestEpr = false;
        Boolean wsmRequestItemCount = false;
        reader.decodeRequestSoapHeaders(
            wsaMessageId, epr.address, wsaAction, wsaFrom, wsaReplyTo,
            wsaFaultTo, epr.resourceUri, *epr.selectorSet, wsmMaxEnvelopeSize,
            wsmLocale, wsmRequestEpr, wsmRequestItemCount);
        throw Exception("Expected OperationTimeout unsupported feature fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsman:UnsupportedFeature")
            throw Exception("Invalid OperationTimeout unsupported fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Header") != 0));
    }

    // Test invalid MaxEnvelopeSize value
    try
    {
        String wsaMessageId, wsaAction, wsaFrom, wsaReplyTo, wsaFaultTo;
        WsmEndpointReference epr;
        Uint32 wsmMaxEnvelopeSize = 0;
        AcceptLanguageList wsmLocale;
        Boolean wsmRequestEpr = false;
        Boolean wsmRequestItemCount = false;
        reader.decodeRequestSoapHeaders(
            wsaMessageId, epr.address, wsaAction, wsaFrom, wsaReplyTo,
            wsaFaultTo, epr.resourceUri, *epr.selectorSet, wsmMaxEnvelopeSize,
            wsmLocale, wsmRequestEpr, wsmRequestItemCount);
        throw Exception("Expected invalid MaxEnvelopeSize fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsa:InvalidMessageInformationHeader")
            throw Exception("Invalid duplicate header fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Header") != 0));
    }

    // Test OptionSet mustUnderstand
    try
    {
        String wsaMessageId, wsaAction, wsaFrom, wsaReplyTo, wsaFaultTo;
        WsmEndpointReference epr;
        Uint32 wsmMaxEnvelopeSize = 0;
        AcceptLanguageList wsmLocale;
        Boolean wsmRequestEpr = false;
        Boolean wsmRequestItemCount = false;
        reader.decodeRequestSoapHeaders(
            wsaMessageId, epr.address, wsaAction, wsaFrom, wsaReplyTo,
            wsaFaultTo, epr.resourceUri, *epr.selectorSet, wsmMaxEnvelopeSize,
            wsmLocale, wsmRequestEpr, wsmRequestItemCount);
        throw Exception("Expected Soap NotUnderstood fault");
    }
    catch (SoapNotUnderstoodFault&)
    {
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Header") != 0));
    }
 
    // Test Locale mustUnderstand unsupported feature
    try
    {
        String wsaMessageId, wsaAction, wsaFrom, wsaReplyTo, wsaFaultTo;
        WsmEndpointReference epr;
        Uint32 wsmMaxEnvelopeSize = 0;
        AcceptLanguageList wsmLocale;
        Boolean wsmRequestEpr = false;
        Boolean wsmRequestItemCount = false;
        reader.decodeRequestSoapHeaders(
            wsaMessageId, epr.address, wsaAction, wsaFrom, wsaReplyTo,
            wsaFaultTo, epr.resourceUri, *epr.selectorSet, wsmMaxEnvelopeSize,
            wsmLocale, wsmRequestEpr, wsmRequestItemCount);
        throw Exception("Expected Locale unsupported feature fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsman:UnsupportedFeature")
            throw Exception("Invalid OperationTimeout unsupported fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Header") != 0));
    }

    reader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Envelope");
}

static void _testEnumerateBody(WsmReader& reader)
{
    XmlEntry entry;
    reader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Envelope");

    String expiration; 
    WsmbPolymorphismMode polymorphismMode = WSMB_PM_UNKNOWN;
    WsenEnumerationMode enumerationMode = WSEN_EM_UNKNOWN;
    Boolean optimized = false;
    Uint32 maxElements = 0;
    SharedPtr<WQLSelectStatement> selectStatement;
    String lang;
    String query;

    reader.decodeEnumerateBody(expiration, polymorphismMode, enumerationMode,
        optimized, maxElements, lang, query);
    if (expiration != "PT123S" || 
        polymorphismMode != WSMB_PM_EXCLUDE_SUBCLASS_PROPERTIES ||
        enumerationMode != WSEN_EM_EPR ||
        optimized != true ||
        maxElements != 33)
        throw Exception("Invalid Enumerate body");
}

static void _testEnumerateBodyErrors(WsmReader& reader)
{
    // Test duplicate headers
    try
    {
        String expiration; 
        WsmbPolymorphismMode polymorphismMode = WSMB_PM_UNKNOWN;
        WsenEnumerationMode enumerationMode = WSEN_EM_UNKNOWN;
        Boolean optimized = false;
        Uint32 maxElements = 0;
        SharedPtr<WQLSelectStatement> selectStatement;
        String lang;
        String query;

        reader.decodeEnumerateBody(expiration, polymorphismMode, 
            enumerationMode, optimized, maxElements, lang, query);

        throw Exception("Expected duplicate headers fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsa:InvalidMessageInformationHeader")
            throw Exception("Invalid duplicate header fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Enumerate") != 0));
    }
    // Test unsupported EndTo header
    try
    {
        String expiration; 
        WsmbPolymorphismMode polymorphismMode = WSMB_PM_UNKNOWN;
        WsenEnumerationMode enumerationMode = WSEN_EM_UNKNOWN;
        Boolean optimized = false;
        Uint32 maxElements = 0;
        SharedPtr<WQLSelectStatement> selectStatement;
        String lang;
        String query;
        
        reader.decodeEnumerateBody(expiration, polymorphismMode, 
            enumerationMode, optimized, maxElements, lang, query);

        throw Exception("Expected unsupported feature fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsman:UnsupportedFeature")
            throw Exception("Invalid unsupported feature fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Enumerate") != 0));
    }
    // Test unsupported Filter header
    try
    {
        String expiration; 
        WsmbPolymorphismMode polymorphismMode = WSMB_PM_UNKNOWN;
        WsenEnumerationMode enumerationMode = WSEN_EM_UNKNOWN;
        Boolean optimized = false;
        Uint32 maxElements = 0;
        SharedPtr<WQLSelectStatement> selectStatement;
        String lang;
        String query;
        
        reader.decodeEnumerateBody(expiration, polymorphismMode, 
            enumerationMode, optimized, maxElements, lang, query);

        throw Exception("Expected filtering not supported fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsen:FilteringNotSupported")
            throw Exception("Invalid unsupported feature fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Enumerate") != 0));
    }
    // Test unsupported enumeration mode
    try
    {
        String expiration; 
        WsmbPolymorphismMode polymorphismMode = WSMB_PM_UNKNOWN;
        WsenEnumerationMode enumerationMode = WSEN_EM_UNKNOWN;
        Boolean optimized = false;
        Uint32 maxElements = 0;
        SharedPtr<WQLSelectStatement> selectStatement;
        String lang;
        String query;
        
        reader.decodeEnumerateBody(expiration, polymorphismMode, 
            enumerationMode, optimized, maxElements, lang, query);

        throw Exception("Expected unsupported feature fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsman:UnsupportedFeature")
            throw Exception("Invalid unsupported feature fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Enumerate") != 0));
    }
    // Test unsupported polymorphism mode
    try
    {
        String expiration; 
        WsmbPolymorphismMode polymorphismMode = WSMB_PM_UNKNOWN;
        WsenEnumerationMode enumerationMode = WSEN_EM_UNKNOWN;
        Boolean optimized = false;
        Uint32 maxElements = 0;
        SharedPtr<WQLSelectStatement> selectStatement;
        String lang;
        String query;
        
        reader.decodeEnumerateBody(expiration, polymorphismMode, 
            enumerationMode, optimized, maxElements, lang, query);

        throw Exception("Expected unsupported polymorphism mode fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsmb:PolymorphismModeNotSupported")
            throw Exception("Invalid unsupported feature fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Enumerate") != 0));
    }
}

static void _testPullBody(WsmReader& reader)
{
    XmlEntry entry;
    reader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Envelope");

    Uint64 enumerationContext = 0;
    String maxTime;
    Uint32 maxElements = 0;
    Uint32 maxCharacters = 0;

    reader.decodePullBody(enumerationContext, 
        maxTime, maxElements, maxCharacters);
    if (enumerationContext != 22 ||
        maxTime != "PT123S" ||
        maxElements != 222 ||
        maxCharacters != 2222)
        throw Exception("Invalid Pull body");
}

static void _testPullBodyErrors(WsmReader& reader)
{
    // Test duplicate headers
    try
    {
        Uint64 enumerationContext = 0;
        String maxTime;
        Uint32 maxElements = 0;
        Uint32 maxCharacters = 0;

        reader.decodePullBody(enumerationContext, 
            maxTime, maxElements, maxCharacters);

        throw Exception("Expected duplicate headers fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsa:InvalidMessageInformationHeader")
            throw Exception("Invalid duplicate header fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Pull") != 0));
    }
    // Test invalid enumeration context
    try
    {
        Uint64 enumerationContext = 0;
        String maxTime;
        Uint32 maxElements = 0;
        Uint32 maxCharacters = 0;

        reader.decodePullBody(enumerationContext, 
            maxTime, maxElements, maxCharacters);

        throw Exception("Expected invalid enumeration context fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsen:InvalidEnumerationContext")
            throw Exception("Invalid enumeration context fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Pull") != 0));
    }
    // Test invalid MaxElements
    try
    {
        Uint64 enumerationContext = 0;
        String maxTime;
        Uint32 maxElements = 0;
        Uint32 maxCharacters = 0;

        reader.decodePullBody(enumerationContext, 
            maxTime, maxElements, maxCharacters);

        throw Exception("Expected invalid MaxElements fault");
    }
    catch (WsmFault& fault)
    {
        if (fault.getSubcode() != "wsa:InvalidMessageInformationHeader")
            throw Exception("Invalid MaxElements fault");
        XmlEntry entry;
        while (reader.next(entry) && (entry.type != XmlEntry::END_TAG ||
               strcmp(entry.localName, "Pull") != 0));
    }
}

int main(int argc, char** argv)
{
    verbose = getenv("PEGASUS_TEST_VERBOSE") ? true : false;

    try
    {
        {
            Buffer text;
            FileSystem::loadFileToMemory(text, "./selectors.xml");
            WsmReader reader((char*)text.getData());

            if (verbose)
                cout << "Testing selectors and EPRs." << endl;
            _testSelectors(reader);
            _testSelectorErrors(reader);
        }
        
        {
            Buffer text;
            FileSystem::loadFileToMemory(text, "./properties.xml");
            WsmReader reader((char*)text.getData());

            if (verbose)
                cout << "Testing instance properties." << endl;
            _testProperties(reader);
            _testPropertyErrors(reader);
        }

        {
            Buffer text;
            FileSystem::loadFileToMemory(text, "./instances.xml");
            WsmReader reader((char*)text.getData());

            if (verbose)
                cout << "Testing instances." << endl;
            _testInstances(reader);
            _testInstanceErrors(reader);
        }

        {
            Buffer text;
            FileSystem::loadFileToMemory(text, "./headers.xml");
            WsmReader reader((char*)text.getData());

            if (verbose)
                cout << "Testing instances." << endl;
            _testHeaders(reader);
            _testHeaderErrors(reader);
        }

        {
            Buffer text;
            FileSystem::loadFileToMemory(text, "./enumerate_body.xml");
            WsmReader reader((char*)text.getData());

            if (verbose)
                cout << "Testing instances." << endl;
            _testEnumerateBody(reader);
            _testEnumerateBodyErrors(reader);
        }

        {
            Buffer text;
            FileSystem::loadFileToMemory(text, "./pull_body.xml");
            WsmReader reader((char*)text.getData());

            if (verbose)
                cout << "Testing instances." << endl;
            _testPullBody(reader);
            _testPullBodyErrors(reader);
        }
    }
    catch(Exception& e)
    {
        cerr << "Error: " << e.getMessage() << endl;
        exit(1);
    }

    cout << argv[0] << " +++++ passed all tests" << endl;

    return 0;
}
