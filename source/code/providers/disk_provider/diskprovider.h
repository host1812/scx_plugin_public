/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Disk provider

    \date        07-08-08 12:00:00

*/
/*----------------------------------------------------------------------------*/
#ifndef DISKPROVIDER_H
#define DISKPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>
#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/staticphysicaldiskenumeration.h>
#include <scxsystemlib/staticlogicaldiskenumeration.h>
#include <scxsystemlib/statisticalphysicaldiskenumeration.h>
#include <scxsystemlib/statisticallogicaldiskenumeration.h>
#include <scxcorelib/scxlog.h>

namespace SCXCore
{
    /*----------------------------------------------------------------------------*/
    /**
       An interface defining methods needed to handle the provider algorithm.
    */
    class ProviderAlgorithmInterface
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Virtual destructor
        */
        virtual ~ProviderAlgorithmInterface() { }

        /*----------------------------------------------------------------------------*/
        /**
           Update related PALs.

           \param instances Use tru if instances should also be updated. Otherwise false.
        */
        virtual void Update(bool instances) = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Add keys to a single instance.

           \param einst EntityInstance who's keys should be added.
           \param inst Instance to add keys to.
        */
        virtual void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> einst, SCXProviderLib::SCXInstance &inst) = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Add properties to a single instance

           \param einst EntityInstance who's properties should be added.
           \param inst Instance to add properties to.
        */
        virtual void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> einst, SCXProviderLib::SCXInstance &inst) = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Fetch keys for all instances

           \param instances Instances with keys are created here.
        */
        virtual void GetInstanceKeys(SCXProviderLib::SCXInstanceCollection &instances) = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Fetch all instances with their properties.

           \param instances All instances are created in this collection.
        */
        virtual void GetInstances(SCXProviderLib::SCXInstanceCollection &instances) = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Search for a certain instance.

           \param key Key to search for.
           \returns A pointer to a found instance or zero if not found.
        */
        virtual SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> FindInstance(const SCXProviderLib::SCXInstance& key) = 0;

    };

    /*----------------------------------------------------------------------------*/
    /**
        Disk provider.

        Concrete instance of the CMPI BaseProvider delivering CIM
        information about Disks on current host.

        A provider-specific thread lock will be held at each call to
        the Do* methods, so this implementation class does not need
        to worry about that.

    */
    class DiskProvider : public SCXProviderLib::BaseProvider
    {
    public:
        DiskProvider();
        ~DiskProvider();

    protected:

        /** CIM classes supported by this provider. */
        enum SupportedCimClasses {
            eSCX_DiskDrive = 0,                     //!< DiskDrive
            eSCX_FileSystem,                        //!< FileSystem
            eSCX_DiskDriveStatisticalInformation,   //!< DiskDriveStatisticalInformation
            eSCX_FileSystemStatisticalInformation,  //!< FileSystemStatisticalInformation
            eSCX_SupportedCimClassMax               //!< enum max marker
        };

        /** CIM methods supported */
        enum SupportedCimMethods {
            eRemoveByNameMethod    //!< RemoveByNameMethod
        };

        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        virtual void DoEnumInstanceNames(const SCXProviderLib::SCXCallContext& callContext,
                                         SCXProviderLib::SCXInstanceCollection &names);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext,
                                     SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoGetInstance(const SCXProviderLib::SCXCallContext& callContext,
                                   SCXProviderLib::SCXInstance& instance);
        virtual void DoCleanup();

        virtual void DoInvokeMethod(const SCXProviderLib::SCXCallContext& callContext,
                                    const std::wstring& methodname, const SCXProviderLib::SCXArgs& args,
                                    SCXProviderLib::SCXArgs& outargs, SCXProviderLib::SCXProperty& result);

    protected:
        void AddKeys(SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> einst, SCXProviderLib::SCXInstance& names, SupportedCimClasses cimtype);
        void AddProperties(SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> einst, SCXProviderLib::SCXInstance& inst, SupportedCimClasses cimtype);
        SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> FindInstance(SupportedCimClasses cimtype, const SCXProviderLib::SCXInstance& keys) const;
        SCXCoreLib::SCXHandle<ProviderAlgorithmInterface> GetProviderAlgIfc(SupportedCimClasses disktype) const;

        //! PAL implementation retrieving logical disk information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskEnumeration> m_statisticalLogicalDisks;
        //! PAL implementation retrieving physical disk information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskEnumeration> m_statisticalPhysicalDisks;
        //! PAL implementation retrieving static logical disk information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskEnumeration> m_staticLogicalDisks;
        //! PAL implementation retrieving static physical disk information for local host
        SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskEnumeration> m_staticPhysicalDisks;
        //! An array of provider algorithm interfaces.
        vector< SCXCoreLib::SCXHandle<ProviderAlgorithmInterface> > m_pProvAlgIfc; //[eSCX_SupportedCimClassMax];
    };
}

#endif /* DISKPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
