/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Demonstration provider

    \date      07-05-14 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>

#include <scxproviderlib/scxprovidercapabilities.h>

#include <string>

#include "wkprovider.h"

using namespace std;
using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;


namespace SCXCore {

    /*----------------------------------------------------------------------------*/
    /**
        Provide CMPI interface for this class

        The class implementation (concrete class) is SCXTestProvider and the name of the
        provider in CIM registration terms is SCX_TestProvider.

    */
    SCXProviderDef(SCXTestProvider, SCX_TestProvider)

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor

        The Singleton thread lock will be held during this call.

    */
    SCXTestProvider::SCXTestProvider() : BaseProvider(L"scx.core.providers.wkprovider")
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider default constructor");

        m_TestItems = new WkEnumeration();
        m_TestItems->Init();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor

    */
    SCXTestProvider::~SCXTestProvider()
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider destructor");
    }


    /*----------------------------------------------------------------------------*/
    /**
        Registration of supported capabilities

        Callback from the BaseProvider in which the provider registers the supported
        classes and methods. The registrations should match the contents of the
        MOF files exactly.

    */
    void SCXTestProvider::DoInit()
    {
        m_ProviderCapabilities.RegisterCimClass(eSCX_Test1, L"SCX_Wk1");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_Test1, eSomeMethod,  L"SomeMethod");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_Test1, eOtherMethod, L"OtherMethod");

        m_ProviderCapabilities.RegisterCimClass(eSCX_Test2, L"SCX_Test2");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_Test2, eThirdMethod,  L"ThirdMethod");
    }


    /*----------------------------------------------------------------------------*/
    /**
        Cleanup callback - the provider is about to be unloaded by Pegasus

    */
    void SCXTestProvider::DoCleanup()
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider::Clenaup()");
    }


    /*----------------------------------------------------------------------------*/
    /**
        Set the keys of the SCXInstance object from the information in the entity instance

        \param[in]     einst   Internal instance representation to get data from
        \param[in,out] names   Collection to add instance to

        \throws        SCXInvalidArgumentException  If the instance can not be converted to a test instance

        This method contains knowledge on which are the key fields for the class.
        The key properties are defined in the MOF file.

    */
    void SCXTestProvider::AddKeys(const SCXHandle<TestInstance> einst, SCXInstance &inst) const // private
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider::AddKeys()");

        // Set all the properties which are key properties to the SCXInstance
        SCXProperty name_prop(L"Name", einst->GetItemName());
        inst.AddKey(name_prop);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Set all properties of the SCXInstance from the information in the EntityInstance

        \param[in]      einst  Internal instance representation to get data from
        \param[in,out]  inst   Instance to populate

        \throws         SCXInvalidArgumentException  If the instance can not be converted to a test instance

        This method knows how to map the values of the internal instance representation
        to the CMPI class definition.

    */
    void SCXTestProvider::AddProperties(const SCXHandle<TestInstance> einst, SCXInstance &inst) const // private
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider::AddPropeties()");

        unsigned int data;

        // For enumerations, set the IsAggregate bool property if this instance
        // represents the total
        SCXProperty total_prop(L"IsAggregate", einst->IsTotal());
        inst.AddProperty(total_prop);

        // Start setting SCXInstance properties from the internal representation
        if (einst->GetAValue(data))
        {
            SCXProperty data_prop(L"ValueA", data);
            inst.AddProperty(data_prop);
        }

        if (einst->GetBValue(data))
        {
            SCXProperty data_prop(L"ValueB", data);
            inst.AddProperty(data_prop);
        }

        SCXCoreLib::SCXCalendarTime tmpTime;
        if (einst->GetCValue(tmpTime))
        {
            SCXProperty data_prop(L"ValueC", tmpTime);
            inst.AddProperty(data_prop);
        }

        std::vector<unsigned int> vuidata;
        if (einst->GetDValue(vuidata))
        {
            std::vector<SCXProperty> vdata;
            for (std::vector<unsigned int>::const_iterator iter = vuidata.begin();
                 iter != vuidata.end();
                 ++iter)
            {
                SCXProperty item(L"", *iter);
                vdata.push_back(item);
            }
            SCXProperty data_prop(L"ValueD", vdata);
            inst.AddProperty(data_prop);
        }

        std::vector<std::wstring> vsdata;
        if (einst->GetEValue(vsdata))
        {
            std::vector<SCXProperty> vdata;
            for (std::vector<std::wstring>::const_iterator iter = vsdata.begin();
                 iter != vsdata.end();
                 ++iter)
            {
                SCXProperty item(L"", *iter);
                vdata.push_back(item);
            }
            SCXProperty data_prop(L"ValueE", vdata);
            inst.AddProperty(data_prop);
        }

        std::vector<SCXCoreLib::SCXCalendarTime> vcdata;
        if (einst->GetFValue(vcdata))
        {
            std::vector<SCXProperty> vdata;
            for (std::vector<SCXCoreLib::SCXCalendarTime>::const_iterator iter = vcdata.begin();
                 iter != vcdata.end();
                 ++iter)
            {
                SCXProperty item(L"", *iter);
                vdata.push_back(item);
            }
            SCXProperty data_prop(L"ValueF", vdata);
            inst.AddProperty(data_prop);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
        Lookup the instance representation, given keys provided from CIMOM

        \param[in]   keys  SCXInstance with property keys set
        \returns           Pointer to located instance

        \throws      SCXInvalidArgumentException
        \throws      SCXInternalErrorException
        \throws      SCXCIMInstanceNotFound  The instance with given keys cannot be found

        This method knows which the key properties of the entity are and returns
        pointer to that item if found.

    */
    SCXCoreLib::SCXHandle<SCXSystemLib::TestInstance> SCXTestProvider::FindInstance(const SCXInstance& keys) const // private
    {
        // Start by extracting all key properties
        const SCXProperty* nameprop = keys.GetProperty(L"Name");
        if (!nameprop)
        {
            throw SCXInvalidArgumentException(L"keys", L"No Name property found", SCXSRCLOCATION);
        }

        for(size_t i=0; i<m_TestItems->Size(); i++)
        {
            SCXHandle<TestInstance> testinst = m_TestItems->GetInstance(i);
            if (testinst == NULL)
            {
                throw SCXInternalErrorException(L"Instance from list not an SCXTestInstance", SCXSRCLOCATION);
            }

            // Compare key values of input args and the current instance
            if (testinst->GetItemName() == nameprop->GetStrValue())
            {
                // Match
                return testinst;
            }
        }

        // As last resort, check if we the request is for the _Total instance
        if (m_TestItems->GetTotalInstance() != 0)
        {
            SCXHandle<TestInstance> testinst = m_TestItems->GetTotalInstance();
            if (testinst == NULL)
            {
                throw SCXInternalErrorException(L"Total instance not an SCXTestInstance", SCXSRCLOCATION);
            }

            if (testinst->GetItemName() == nameprop->GetStrValue())
            {
                return testinst;
            }
        }

        throw SCXCIMInstanceNotFound(keys.DumpString(), SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Enumerate instance names

        \param[in]     callContext SCXCallContext object
        \param[out]    names Rreturn collection of instances with key properties

    */
    void SCXTestProvider::DoEnumInstanceNames(const SCXCallContext& /* callContext */,
                                              SCXInstanceCollection& names)
    {
        SCX_LOGTRACE(m_log, L"SCXTestInstance::DoEnumInstanceNames()");

        // Do an update on the collection to get latest number of instances
        // (in this call we are not interested in dynamic values of dynamic
        // properties, only keys. Hence the false flag.)
        m_TestItems->Update(false);

        SCX_LOGTRACE(m_log, StrAppend(L"Number of Test Items = ", m_TestItems->Size()));

        // Iterate over all ordinary instances
        for(size_t i=0; i<m_TestItems->Size(); i++)
        {
            // For each ordinary Test instance, create an SCXInstance with the
            // proper key properties set and add it to the names collection.
            SCXInstance inst;
            AddKeys(m_TestItems->GetInstance(i), inst);
            names.AddInstance(inst);
        }

        // Special handling of the _Total instance
        if (m_TestItems->GetTotalInstance() != 0)
        {
            // There will always be one total instance
            SCXInstance inst;
            AddKeys(m_TestItems->GetTotalInstance(), inst);
            names.AddInstance(inst);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Enumerate instances

        \param[in]     callContext SCXCallContext object
        \param[out]    instances    Return collection of instances

    */
    void SCXTestProvider::DoEnumInstances(const SCXCallContext& /* callContext */,
                                          SCXInstanceCollection& instances)
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider::DoEnumInstances()");

        // Do an update on the collection to get latest number of instances.
        // Each instance will be updated as well, with current values of all
        // properties.
        m_TestItems->Update();

        SCX_LOGTRACE(m_log, StrAppend(L"Number of Test Items = ", m_TestItems->Size()));

        for(size_t i=0; i<m_TestItems->Size(); i++)
        {
            SCXInstance inst;
            AddKeys(m_TestItems->GetInstance(i), inst);
            AddProperties(m_TestItems->GetInstance(i), inst);
            instances.AddInstance(inst);
        }

        if (m_TestItems->GetTotalInstance() != 0)
        {
            SCXInstance inst;
            AddKeys(m_TestItems->GetTotalInstance(), inst);
            AddProperties(m_TestItems->GetTotalInstance(), inst);
            instances.AddInstance(inst);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Execute query

        \param[in]     callContext SCXCallContext object
        \param[out]    instances    Return collection of instances
        \param[in]     query        Query to run, expressed according to "language"
        \param[in]     language     Language that "query" is expressed in

    */
    void SCXTestProvider::DoExecQuery(const SCXCallContext& /* callContext */,
                                          SCXInstanceCollection& instances,
                                          std::wstring query, std::wstring language)
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider::DoExecQuery()");
        SCX_LOGTRACE(m_log, StrAppend(L"query = ", query));
        SCX_LOGTRACE(m_log, StrAppend(L"language = ", language));

        // Do an update on the collection to get latest number of instances.
        // Each instance will be updated as well, with current values of all
        // properties.
        m_TestItems->Update();

        SCX_LOGTRACE(m_log, StrAppend(L"Number of Test Items = ", m_TestItems->Size()));

        for(size_t i=0; i<m_TestItems->Size(); i++)
        {
            SCXInstance inst;
            AddKeys(m_TestItems->GetInstance(i), inst);
            AddProperties(m_TestItems->GetInstance(i), inst);
            instances.AddInstance(inst);
        }

        if (m_TestItems->GetTotalInstance() != 0)
        {
            SCXInstance inst;
            AddKeys(m_TestItems->GetTotalInstance(), inst);
            AddProperties(m_TestItems->GetTotalInstance(), inst);
            instances.AddInstance(inst);
        }

    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve a specific instance

        \param[in]     callContext SCXCallContext object containing instance with key properties, indicating which instance to retrieve
        \param[out]    instance    The selected instance

        \throws        SCXInvalidArgumentException  If no Name property in keys
        \throws        SCXInternalErrorException    If instances in list are not TestInstance

    */
    void SCXTestProvider::DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider::DoGetInstance()");

        // Refresh the collection (both keys and current data)
        m_TestItems->Update();

        SCXHandle<TestInstance> testinst = FindInstance(callContext.GetObjectPath());

        // If we get here whithout exception we got a match - set keys and properties,
        // the instance is returned as out value
        AddKeys(testinst, instance);
        AddProperties(testinst, instance);

        // All done, simply return
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve a specific instance

        \param[in]     callContext SCXCallContext object
        \param[in]     instance    The instance to create
        \param[out]    objectPath     Object path to new instance.

    */
    void SCXTestProvider::DoCreateInstance(const SCXCallContext& callContext, const SCXInstance& instance, SCXInstance& objectPath)
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider::DoCreateInstance()");

        //
        SCX_LOGTRACE(m_log, L"Got request to create object with ObjectPath <" +
                     callContext.GetObjectPath().DumpString() +
                     L">, properties <" + instance.DumpString());

        SCXHandle<TestInstance> testinst = m_TestItems->Create(atoi(SCXCoreLib::StrToUTF8(instance.GetProperty(L"Name")->GetStrValue()).c_str()));
        AddKeys(testinst, objectPath);
        AddProperties(testinst, objectPath);

        // All done, simply return
    }

    /*----------------------------------------------------------------------------*/
    /**
        Invoke a method on an instance

        \param[in]     callContext Keys indicating instance to execute method on
        \param[in]     methodname  Name of method called
        \param[in]     args        Arguments provided for method call
        \param[out]    outargs     Output arguments
        \param[out]    result      Result value
    */
    void SCXTestProvider::DoInvokeMethod(
        const SCXCallContext& /* callContext */,
        const std::wstring& methodname,
        const SCXArgs& /* args */,
        SCXArgs& /* outargs */,
        SCXProperty& result)
    {
        SCX_LOGTRACE(m_log, L"SCXTestProvider DoInvokeMethod");

        // Refresh the collection, not individual instances
        m_TestItems->Update(false);

        // Commentted out due to unused variable warning
        // EntityInstance* testinst = FindInstance(keys);

        // If we get here whithout exception we got a match - do whatever
        // necessary here...

        result.SetValue(methodname + L" - Return");
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
