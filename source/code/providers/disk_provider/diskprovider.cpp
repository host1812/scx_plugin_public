/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Disk provider

    \date        07-08-08 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxproviderlib/scxprovidercapabilities.h>

#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxassert.h>
#include <scxcorelib/stringaid.h>

#include "../meta_provider/startuplog.h"

#include "diskprovider.h"

#include <scxcorelib/scxlog.h>

using namespace SCXProviderLib;
using namespace SCXSystemLib;
using namespace SCXCoreLib;

namespace SCXCore {

    /*----------------------------------------------------------------------------*/
    /**
       Algorithm handler for statistical physical disk info.
    */
    class StatisticalPhysicalDiskAlgorithm : public ProviderAlgorithmInterface
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor

           \param pEnum The enumeration object that should be used with this handler.
        */
        StatisticalPhysicalDiskAlgorithm(SCXCoreLib::SCXHandle<StatisticalPhysicalDiskEnumeration> pEnum)
            : m_pEnum(pEnum)
        {
            SCXASSERT(0 != m_pEnum);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::Update
        */
        void Update(bool instances)
        {
            m_pEnum->Update(instances);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddKeys
        */
        void AddKeys(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StatisticalPhysicalDiskInstance* diskinst = static_cast<const StatisticalPhysicalDiskInstance*>(einst.GetData());

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            std::wstring name;
            if (diskinst->GetDiskName(name))
            {
                SCXProperty name_prop(L"Name", name);
                inst.AddKey(name_prop);
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddProperties
        */
        void AddProperties(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StatisticalPhysicalDiskInstance* diskinst = static_cast<const StatisticalPhysicalDiskInstance*>(einst.GetData());
            scxulong data1;
            scxulong data2;
            double ddata1;
            double ddata2;
            bool healthy;

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            if (diskinst->GetHealthState(healthy))
            {
                SCXProperty online_prop(L"IsOnline", healthy);
                inst.AddProperty(online_prop);
            }

            SCXProperty total_prop(L"IsAggregate", diskinst->IsTotal());
            inst.AddProperty(total_prop);

            if (diskinst->GetIOPercentageTotal(data1))
            {
                SCXProperty prop1(L"PercentBusyTime", (unsigned char) data1);
                SCXProperty prop2(L"PercentIdleTime", (unsigned char) (100-data1));
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
            }

            if (diskinst->GetBytesPerSecondTotal(data1))
            {
                SCXProperty prop(L"BytesPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetBytesPerSecond(data1, data2))
            {
                SCXProperty prop1(L"ReadBytesPerSecond", data1);
                SCXProperty prop2(L"WriteBytesPerSecond", data2);
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
            }

            if (diskinst->GetTransfersPerSecond(data1))
            {
                SCXProperty prop(L"TransfersPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetReadsPerSecond(data1))
            {
                SCXProperty prop(L"ReadsPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetWritesPerSecond(data1))
            {
                SCXProperty prop(L"WritesPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetIOTimesTotal(ddata1))
            {
                SCXProperty prop(L"AverageTransferTime", ddata1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetIOTimes(ddata1, ddata2))
            {
                SCXProperty prop1(L"AverageReadTime", ddata1);
                SCXProperty prop2(L"AverageWriteTime", ddata2);
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
            }

            if (diskinst->GetDiskQueueLength(ddata1))
            {
                SCXProperty prop1(L"AverageDiskQueueLength", ddata1);
                inst.AddProperty(prop1);
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::FindInstance
        */
        SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> FindInstance(const SCXProviderLib::SCXInstance& key)
        {
            return m_pEnum->GetInstance(BaseProvider::GetKeyRef(L"Name", key).GetStrValue());
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstanceKeys
        */
        void GetInstanceKeys(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                // For each Disk instance, create an SCXInstance and add it to the
                // names collection.
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                // There will always be one total instance
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }

        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstances
        */
        void GetInstances(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                AddProperties(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                AddProperties(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }
        }

    private:
        SCXCoreLib::SCXHandle<StatisticalPhysicalDiskEnumeration> m_pEnum; //!< handled enumeration.

        StatisticalPhysicalDiskAlgorithm();     //!< Private constructor (intentionally not implemented)
    };

    /*----------------------------------------------------------------------------*/
    /**
       Algorithm handler for statistical logical disk info.
    */
    class StatisticalLogicalDiskAlgorithm : public ProviderAlgorithmInterface
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor

           \param pEnum The enumeration object that should be used with this handler.
        */
        StatisticalLogicalDiskAlgorithm(SCXCoreLib::SCXHandle<StatisticalLogicalDiskEnumeration> pEnum)
            : m_pEnum(pEnum)
        {
            SCXASSERT(0 != m_pEnum);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::Update
        */
        void Update(bool instances)
        {
            m_pEnum->Update(instances);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddKeys
        */
        void AddKeys(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StatisticalLogicalDiskInstance* diskinst = static_cast<const StatisticalLogicalDiskInstance*>(einst.GetData());

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            std::wstring name;
            if (diskinst->GetDiskName(name))
            {
                SCXProperty name_prop(L"Name", name);
                inst.AddKey(name_prop);
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddProperties
        */
        void AddProperties(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StatisticalLogicalDiskInstance* diskinst = static_cast<const StatisticalLogicalDiskInstance*>(einst.GetData());
            scxulong data1;
            scxulong data2;
            double ddata1;
            bool healthy;

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            if (diskinst->GetHealthState(healthy))
            {
                SCXProperty online_prop(L"IsOnline", healthy);
                inst.AddProperty(online_prop);
            }

            SCXProperty total_prop(L"IsAggregate", diskinst->IsTotal());
            inst.AddProperty(total_prop);

            if (diskinst->GetIOPercentageTotal(data1))
            {
                SCXProperty prop1(L"PercentBusyTime", (unsigned char) data1);
                SCXProperty prop2(L"PercentIdleTime", (unsigned char) (100-data1));
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
            }

            if (diskinst->GetBytesPerSecondTotal(data1))
            {
                SCXProperty prop(L"BytesPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetBytesPerSecond(data1, data2))
            {
                SCXProperty prop1(L"ReadBytesPerSecond", data1);
                SCXProperty prop2(L"WriteBytesPerSecond", data2);
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
            }

            if (diskinst->GetTransfersPerSecond(data1))
            {
                SCXProperty prop(L"TransfersPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetReadsPerSecond(data1))
            {
                SCXProperty prop(L"ReadsPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetWritesPerSecond(data1))
            {
                SCXProperty prop(L"WritesPerSecond", data1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetIOTimesTotal(ddata1))
            {
                SCXProperty prop(L"AverageTransferTime", ddata1);
                inst.AddProperty(prop);
            }

            if (diskinst->GetDiskSize(data1, data2))
            {
                SCXProperty prop1(L"FreeMegabytes", data2);
                SCXProperty prop2(L"UsedMegabytes", data1);
                unsigned char freeSpace = 100;
                unsigned char usedSpace = 0;
                if (0 < data1+data2)
                {
                    freeSpace = (unsigned char) SCXCoreLib::GetPercentage(0, data2, 0, data1+data2);
                    usedSpace = (unsigned char) SCXCoreLib::GetPercentage(0, data2, 0, data1+data2, true);
                }
                SCXProperty prop3(L"PercentFreeSpace", freeSpace);
                SCXProperty prop4(L"PercentUsedSpace", usedSpace);
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
                inst.AddProperty(prop3);
                inst.AddProperty(prop4);
            }

			// Report percentages for inodes even if inode data is not known
            {
	            if (!diskinst->GetInodeUsage(data1, data2))
				{
					data1 = data2 = 0;
				}
                unsigned char freeInodes = 100;
                unsigned char usedInodes = 0;

                if (0 < data1+data2)
                {
                    freeInodes = (unsigned char) SCXCoreLib::GetPercentage(0, data2, 0, data1);
                    usedInodes = (unsigned char) SCXCoreLib::GetPercentage(0, data2, 0, data1, true);
                }
                SCXProperty prop1(L"PercentFreeInodes", freeInodes);
                SCXProperty prop2(L"PercentUsedInodes", usedInodes);
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
            }

            if (diskinst->GetDiskQueueLength(ddata1))
            {
                SCXProperty prop1(L"AverageDiskQueueLength", ddata1);
                inst.AddProperty(prop1);
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::FindInstance
        */
        SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> FindInstance(const SCXProviderLib::SCXInstance& key)
        {
            return m_pEnum->GetInstance(BaseProvider::GetKeyRef(L"Name", key).GetStrValue());
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstanceKeys
        */
        void GetInstanceKeys(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                // For each Disk instance, create an SCXInstance and add it to the
                // names collection.
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                // There will always be one total instance
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }

        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstances
        */
        void GetInstances(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                AddProperties(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                AddProperties(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }
        }

    private:
        SCXCoreLib::SCXHandle<StatisticalLogicalDiskEnumeration> m_pEnum; //!< handled enumeration.

        StatisticalLogicalDiskAlgorithm();      //!< Private constructor (intentionally not implemented)
    };

    /*----------------------------------------------------------------------------*/
    /**
       Algorithm handler for static physical disk info.
    */
    class StaticPhysicalDiskAlgorithm : public ProviderAlgorithmInterface
    {
    public:
        /**
            Constructor.

            \param pEnum  Handled enumeration.
        */
        StaticPhysicalDiskAlgorithm(SCXCoreLib::SCXHandle<StaticPhysicalDiskEnumeration> pEnum)
            : m_pEnum(pEnum)
        {
            SCXASSERT(0 != m_pEnum);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::Update
        */
        void Update(bool instances)
        {
            m_pEnum->Update(instances);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddKeys
        */
        void AddKeys(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StaticPhysicalDiskInstance* diskinst = static_cast<const StaticPhysicalDiskInstance*>(einst.GetData());

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            std::wstring deviceId;
            if (diskinst->GetDiskName(deviceId))
            {
                inst.AddKey(SCXProperty(L"DeviceID", deviceId));
            }

            inst.AddKey(SCXProperty(L"CreationClassName", L"SCX_DiskDrive"));
            BaseProvider::AddScopingSystemKeys(inst);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddProperties
        */
        void AddProperties(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StaticPhysicalDiskInstance* diskinst = static_cast<const StaticPhysicalDiskInstance*>(einst.GetData());
            scxulong data;
            std::wstring sdata;
            bool healthy;

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            std::wstring name;
            if (diskinst->GetDiskName(name))
            {
                SCXProperty name_prop(L"Name", name);
                inst.AddProperty(name_prop);
            }

            if (diskinst->GetHealthState(healthy))
            {
                SCXProperty online_prop(L"IsOnline", healthy);
                inst.AddProperty(online_prop);
            }

            DiskInterfaceType ifcType;
            if (diskinst->GetInterfaceType(ifcType))
            {
                SCXProperty prop(L"InterfaceType", L"Unknown");
                switch (ifcType)
                {
                case eDiskIfcIDE:
                    prop.SetValue(L"IDE");
                    break;
                case eDiskIfcSCSI:
                    prop.SetValue(L"SCSI");
                    break;
                case eDiskIfcVirtual:
                    prop.SetValue(L"Virtual");
                    break;
                case eDiskIfcUnknown:
                case eDiskIfcMax:
                default:
                    break;
                }
                inst.AddProperty(prop);
            }

            if (diskinst->GetManufacturer(sdata))
            {
                SCXProperty prop(L"Manufacturer", sdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetModel(sdata))
            {
                SCXProperty prop(L"Model", sdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetSizeInBytes(data))
            {
                SCXProperty prop(L"MaxMediaSize", data);
                inst.AddProperty(prop);
            }

            if (diskinst->GetTotalCylinders(data))
            {
                SCXProperty prop(L"TotalCylinders", data);
                inst.AddProperty(prop);
            }

            if (diskinst->GetTotalHeads(data))
            {
                SCXProperty prop(L"TotalHeads", data);
                inst.AddProperty(prop);
            }

            if (diskinst->GetTotalSectors(data))
            {
                SCXProperty prop(L"TotalSectors", data);
                inst.AddProperty(prop);
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::FindInstance
        */
        SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> FindInstance(const SCXProviderLib::SCXInstance& key)
        {
            BaseProvider::ValidateKeyValue(L"CreationClassName", key, L"SCX_DiskDrive");
            BaseProvider::ValidateScopingSystemKeys(key);
            return m_pEnum->GetInstance(BaseProvider::GetKeyRef(L"DeviceID", key).GetStrValue());
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstanceKeys
        */
        void GetInstanceKeys(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                // For each Disk instance, create an SCXInstance and add it to the
                // names collection.
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                // There will always be one total instance
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }

        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstances
        */
        void GetInstances(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                AddProperties(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                AddProperties(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }
        }

    private:
        SCXCoreLib::SCXHandle<StaticPhysicalDiskEnumeration> m_pEnum; //!< Handled enumeration.

        StaticPhysicalDiskAlgorithm();  //!< Private constructor (intentionally not implemented)
    };

    /*----------------------------------------------------------------------------*/
    /**
       Algorithm handler for static logical disk info.
    */
    class StaticLogicalDiskAlgorithm : public ProviderAlgorithmInterface
    {
    public:
        /**
            Constructor.

            \param pEnum  Handled enumeration.
        */
        StaticLogicalDiskAlgorithm(SCXCoreLib::SCXHandle<StaticLogicalDiskEnumeration> pEnum)
            : m_pEnum(pEnum)
        {
            SCXASSERT(0 != m_pEnum);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::Update
        */
        void Update(bool instances)
        {
            m_pEnum->Update(instances);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddKeys
        */
        void AddKeys(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StaticLogicalDiskInstance* diskinst = static_cast<const StaticLogicalDiskInstance*>(einst.GetData());

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            std::wstring name;
            if (diskinst->GetDeviceName(name))
            {
                SCXProperty name_prop(L"Name", name);
                inst.AddKey(name_prop);
            }

            inst.AddKey(SCXProperty(L"CreationClassName", L"SCX_FileSystem"));
            BaseProvider::AddScopingComputerSystemKeys(inst);
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::AddProperties
        */
        void AddProperties(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst)
        {
            const StaticLogicalDiskInstance* diskinst = static_cast<const StaticLogicalDiskInstance*>(einst.GetData());
            scxulong data;
            std::wstring sdata;
            bool bdata;

            if (NULL == diskinst)
            {
                throw SCXInvalidArgumentException(L"einst", L"invalid parameter", SCXSRCLOCATION);
            }

            if (diskinst->GetHealthState(bdata))
            {
                SCXProperty  prop(L"IsOnline", bdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetMountpoint(sdata))
            {
                SCXProperty prop(L"Root", sdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetFileSystemType(sdata))
            {
                SCXProperty prop(L"FileSystemType", sdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetSizeInBytes(data))
            {
                SCXProperty prop(L"FileSystemSize", data);
                inst.AddProperty(prop);
            }

            if (diskinst->GetCompressionMethod(sdata))
            {
                SCXProperty prop(L"CompressionMethod", sdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetIsReadOnly(bdata))
            {
                SCXProperty prop(L"ReadOnly", bdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetEncryptionMethod(sdata))
            {
                SCXProperty prop(L"EncryptionMethod", sdata);
                inst.AddProperty(prop);
            }

            int idata;
            if (diskinst->GetPersistenceType(idata))
            {
                SCXProperty prop(L"PersistenceType", static_cast<unsigned short>(idata));
                inst.AddProperty(prop);
            }

            if (diskinst->GetBlockSize(data))
            {
                SCXProperty prop(L"BlockSize", data);
                inst.AddProperty(prop);
            }

            if (diskinst->GetAvailableSpaceInBytes(data))
            {
                SCXProperty prop(L"AvailableSpace", data);
                inst.AddProperty(prop);
            }

            scxulong inodesTotal, inodesFree;
            if (diskinst->GetTotalInodes(inodesTotal) && diskinst->GetAvailableInodes(inodesFree))
            {
                SCXProperty prop1(L"TotalInodes", inodesTotal);
                SCXProperty prop2(L"FreeInodes", inodesFree);
                SCXProperty prop3(L"NumberOfFiles", inodesTotal - inodesFree);
                inst.AddProperty(prop1);
                inst.AddProperty(prop2);
                inst.AddProperty(prop3);
            }

            if (diskinst->GetIsCaseSensitive(bdata))
            {
                SCXProperty prop(L"CaseSensitive", bdata);
                inst.AddProperty(prop);
            }

            if (diskinst->GetIsCasePreserved(bdata))
            {
                SCXProperty prop(L"CasePreserved", bdata);
                inst.AddProperty(prop);
            }

            /* Array support not yet checked in This property needs to be an property array
            if (diskinst->GetCodeSet(idata))
            {
                SCXProperty prop(L"CodeSet", static_cast<unsigned short>(idata));
                inst.AddProperty(prop);
                }*/

            if (diskinst->GetMaxFilenameLen(data))
            {
                SCXProperty prop(L"MaxFileNameLength", static_cast<unsigned int>(data));
                inst.AddProperty(prop);
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::FindInstance
        */
        SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> FindInstance(const SCXProviderLib::SCXInstance& key)
        {
            BaseProvider::ValidateKeyValue(L"CreationClassName", key, L"SCX_FileSystem");
            BaseProvider::ValidateScopingComputerSystemKeys(key);
            return m_pEnum->GetInstance(BaseProvider::GetKeyRef(L"Name", key).GetStrValue());
        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstanceKeys
        */
        void GetInstanceKeys(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                // For each Disk instance, create an SCXInstance and add it to the
                // names collection.
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                // There will always be one total instance
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }

        }

        /*----------------------------------------------------------------------------*/
        /**
           \copydoc SCXCore::ProviderAlgorithmInterface::GetInstances
        */
        void GetInstances(SCXProviderLib::SCXInstanceCollection &instances)
        {
            for(size_t i=0; i<m_pEnum->Size(); i++)
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetInstance(i), inst);
                AddProperties(m_pEnum->GetInstance(i), inst);
                instances.AddInstance(inst);
            }

            if (0 != m_pEnum->GetTotalInstance())
            {
                SCXInstance inst;
                AddKeys(m_pEnum->GetTotalInstance(), inst);
                AddProperties(m_pEnum->GetTotalInstance(), inst);
                instances.AddInstance(inst);
            }
        }

    private:
        SCXCoreLib::SCXHandle<StaticLogicalDiskEnumeration> m_pEnum; //!< handled enumeration

        StaticLogicalDiskAlgorithm();   //!< Private constructor (intentionally not implemented)
    };

    /*----------------------------------------------------------------------------*/
    /**
        Provide CMPI interface for this class

        The class implementation (concrete class) is DiskProvider and the name of the
        provider in CIM registration terms is SCX_DiskProvider.

    */
    SCXProviderDef(DiskProvider, SCX_DiskProvider)

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor

        The Singleton thread lock will be held during this call.

    */
    DiskProvider::DiskProvider() : BaseProvider(L"scx.core.providers.diskprovider"),
        m_statisticalLogicalDisks(NULL), m_statisticalPhysicalDisks(NULL),
        m_staticLogicalDisks(NULL), m_staticPhysicalDisks(NULL)
    {
                LogStartup();
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.providers.diskprovider");
        SCX_LOGTRACE(m_log, L"DiskProvider constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor

    */
    DiskProvider::~DiskProvider()
    {
        // Do not log here since when this destructor is called the objects neccesary for logging might no longer be alive
    }

    /*----------------------------------------------------------------------------*/
    /**
        Registration of supported capabilities

        Callback from the BaseProvider in which the provider registers the supported
        classes and methods. The registrations should match the contents of the
        MOF files exactly.

    */
    void DiskProvider::DoInit()
    {
        if (m_statisticalPhysicalDisks != NULL ||
            m_statisticalLogicalDisks != NULL ||
            m_staticPhysicalDisks != NULL ||
            m_staticLogicalDisks != NULL)
        {
            SCXASSERTFAIL(L"DoInit() called multiple times without a call to DoCleanup() between");
            DoCleanup();
        }

        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskDependDefault() );
        m_statisticalPhysicalDisks = new StatisticalPhysicalDiskEnumeration(deps);
        m_statisticalPhysicalDisks->Init();

        deps = new DiskDependDefault(); // need new diskdepend since it is not thread safe
        m_statisticalLogicalDisks = new StatisticalLogicalDiskEnumeration(deps);
        m_statisticalLogicalDisks->Init();

        deps = new DiskDependDefault() ; // need new diskdepend since it is not thread safe
        m_staticPhysicalDisks = new StaticPhysicalDiskEnumeration(deps);
        m_staticPhysicalDisks->Init();

        deps = new DiskDependDefault(); // need new diskdepend since it is not thread safe
        m_staticLogicalDisks = new StaticLogicalDiskEnumeration(deps);
        m_staticLogicalDisks->Init();

        m_ProviderCapabilities.RegisterCimClass(eSCX_DiskDrive,
                                                L"SCX_DiskDrive");
        m_ProviderCapabilities.RegisterCimClass(eSCX_FileSystem,
                                                L"SCX_FileSystem");
        m_ProviderCapabilities.RegisterCimClass(eSCX_DiskDriveStatisticalInformation,
                                                L"SCX_DiskDriveStatisticalInformation");
        m_ProviderCapabilities.RegisterCimClass(eSCX_FileSystemStatisticalInformation,
                                                L"SCX_FileSystemStatisticalInformation");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_DiskDrive, eRemoveByNameMethod,
                                                 L"RemoveByName");
        m_ProviderCapabilities.RegisterCimMethod(eSCX_FileSystem, eRemoveByNameMethod,
                                                 L"RemoveByName");

        m_pProvAlgIfc.resize( eSCX_SupportedCimClassMax, SCXCoreLib::SCXHandle<ProviderAlgorithmInterface>(0) );
        m_pProvAlgIfc[eSCX_DiskDrive] = new StaticPhysicalDiskAlgorithm(m_staticPhysicalDisks);
        m_pProvAlgIfc[eSCX_FileSystem] = new StaticLogicalDiskAlgorithm(m_staticLogicalDisks);
        m_pProvAlgIfc[eSCX_DiskDriveStatisticalInformation] = new StatisticalPhysicalDiskAlgorithm(m_statisticalPhysicalDisks);
        m_pProvAlgIfc[eSCX_FileSystemStatisticalInformation] = new StatisticalLogicalDiskAlgorithm(m_statisticalLogicalDisks);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Do cleanup

    */
    void DiskProvider::DoCleanup()
    {
        m_ProviderCapabilities.Clear();
        if (m_statisticalPhysicalDisks != NULL)
        {
            m_statisticalPhysicalDisks->CleanUp();
            m_statisticalPhysicalDisks = NULL;
        }

        if (m_statisticalLogicalDisks != NULL)
        {
            m_statisticalLogicalDisks->CleanUp();
            m_statisticalLogicalDisks = NULL;
        }
        if (m_staticPhysicalDisks != NULL)
        {
            m_staticPhysicalDisks->CleanUp();
            m_staticPhysicalDisks = NULL;
        }

        if (m_staticLogicalDisks != NULL)
        {
            m_staticLogicalDisks->CleanUp();
            m_staticLogicalDisks = NULL;
        }

        m_pProvAlgIfc.clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add a SCXInstance with the name property set frmo the DiskInstance to the collection

        \param  einst Disk instance to get data from
        \param  names Collection to add instance to
        \param  cimtype Requested cim class

        \throws SCXInvalidArgumentException if the instance can not be converted to a DiskInstance

        This method contains knowledge on which are the key fields for the class.
        The key properties are defined in the MOF file.

    */
    void DiskProvider::AddKeys(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst, SupportedCimClasses cimtype) // private
    {
        SCX_LOGTRACE(m_log, L"DiskProvider AddKeys()");

        GetProviderAlgIfc(cimtype)->AddKeys(einst, inst);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set all properties from the DiskInstance in the SCXInstance

        \param  einst Disk instance to get data from
        \param  inst  Instance to populate
        \param  cimtype Requested cim class

        \throws SCXInvalidArgumentException if the instance can not be converted to a DiskInstance

        This method knows how to map the values of the Disk PAL to the CMPI class
        definition.

    */
    void DiskProvider::AddProperties(SCXCoreLib::SCXHandle<EntityInstance> einst, SCXInstance &inst, SupportedCimClasses cimtype) // private
    {
        SCX_LOGTRACE(m_log, L"DiskProvider AddPropeties()");

        GetProviderAlgIfc(cimtype)->AddProperties(einst, inst);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Lookup the instance representation, given keys provided from CIMOM

        \param  cimtype Requested cim class
        \param  keys SCXInstance with property keys set
        \returns Pointer to located instance

        \throws SCXInvalidArgumentException
        \throws SCXInternalErrorException
        \throws SCXCIMInstanceNotFound if the instance with given keys cannot be found

        This method knows which the key properties of the entity are and returns
        pointer to that item if found.

    */
    SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> DiskProvider::FindInstance(SupportedCimClasses cimtype,
                                                                                   const SCXInstance& keys) const // private
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::EntityInstance> inst = GetProviderAlgIfc(cimtype)->FindInstance(keys);

        if (NULL != inst)
        {
            return inst;
        }

        throw SCXCIMInstanceNotFound(keys.DumpString(), SCXSRCLOCATION);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Get the correct provider algoritm handler given a disk type

        \param   disktype Supported disk type
        \returns Disk provider algorithm interface matching the given call context

    */
    SCXCoreLib::SCXHandle<ProviderAlgorithmInterface> DiskProvider::GetProviderAlgIfc(SupportedCimClasses disktype) const
    {
        if (disktype >= eSCX_SupportedCimClassMax)
        {
            throw SCXInternalErrorException(StrAppend(L"Unknown disk type: ", disktype), SCXSRCLOCATION);
        }
        SCX_LOGTRACE(m_log, StrAppend(L"DiskProvider GetDiskEnumeration - type ", disktype));
        return m_pProvAlgIfc[disktype];
    }

    /*----------------------------------------------------------------------------*/
    /**
        Enumerate instance names

        \param[in]   callContext CallContext to get disk enumeration from.
        \param[out]  names collection of instances with key properties

    */
    void DiskProvider::DoEnumInstanceNames(const SCXCallContext& callContext,
                                           SCXInstanceCollection &names)
    {
        SCX_LOGTRACE(m_log, L"DiskProvider DoEnumInstanceNames");

        SupportedCimClasses disktype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        SCXCoreLib::SCXHandle<ProviderAlgorithmInterface> disks = GetProviderAlgIfc(disktype);

        disks->Update(false);

        disks->GetInstanceKeys(names);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Enumerate instances

        \param[in]   callContext CallContext to get disk enumeration from.
        \param[out]  instances collection of instances
        \returns     SCX_OK if OK

    */
    void DiskProvider::DoEnumInstances(const SCXCallContext& callContext,
                                       SCXInstanceCollection &instances)
    {
        SCX_LOGTRACE(m_log, L"DiskProvider DoEnumInstances");

        SupportedCimClasses disktype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        SCXCoreLib::SCXHandle<ProviderAlgorithmInterface> disks = GetProviderAlgIfc(disktype);

        // Update Disk PAL instance. This is both update of number of Disks and
        // current statistics for each Disk.
        disks->Update(true);

        disks->GetInstances(instances);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get an instance

        \param[in]   callContext CallContext to get disk enumeration from.
        \param[out]  instance the selected instance

        \throws      SCXInvalidArgumentException if no Name property in keys
        \throws      SCXInternalErrorException if instances in list are not DiskInstance

    */
    void DiskProvider::DoGetInstance(const SCXCallContext& callContext, SCXInstance& instance)
    {
        SCX_LOGTRACE(m_log, L"DiskProvider::DoGetInstance()");

        SupportedCimClasses disktype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        SCXCoreLib::SCXHandle<ProviderAlgorithmInterface> disks = GetProviderAlgIfc(disktype);

        // Refresh the collection (both keys and current data)
        disks->Update(true);

        SCXCoreLib::SCXHandle<EntityInstance> testinst = FindInstance(disktype, callContext.GetObjectPath());

        // If we get here whithout exception we got a match - set keys and properties,
        // the instance is returned as out value
        AddKeys(testinst, instance, disktype);
        AddProperties(testinst, instance, disktype);
    }

    /**
        Invoke a method on an instance

        \param[in]     callContext Keys indicating instance to execute method on
        \param[in]     methodname  Name of method called
        \param[in]     args        Arguments provided for method call
        \param[out]    outargs     Output arguments
        \param[out]    result      Result value
    */
    void DiskProvider::DoInvokeMethod(
        const SCXCallContext& callContext,
        const std::wstring& methodname,
        const SCXArgs& args,
        SCXArgs& /*outargs*/,
        SCXProperty& result)
    {
        SCX_LOGTRACE(m_log, L"SCXDiskProvider DoInvokeMethod");

        SupportedCimMethods cimmethod = static_cast<SupportedCimMethods>(m_ProviderCapabilities.GetCimMethodId(callContext.GetObjectPath(), methodname));
        SupportedCimClasses disktype = static_cast<SupportedCimClasses>(m_ProviderCapabilities.GetCimClassId(callContext.GetObjectPath()));

        if (cimmethod == eRemoveByNameMethod)
        {
            const SCXProperty* name = args.GetProperty(L"name");

            if (name == NULL)
            {
                throw SCXInternalErrorException(L"missing arguments to RemoveByName method", SCXSRCLOCATION);
            }

            if (name->GetType() != SCXProperty::SCXStringType)
            {
                throw SCXInternalErrorException(L"Wrong type of arguments to RemoveByname method", SCXSRCLOCATION);
            }

            bool cmdok = false;
            if (eSCX_DiskDrive == disktype)
            {
                cmdok = m_statisticalPhysicalDisks->RemoveInstanceById(name->GetStrValue()) &&
                    m_staticPhysicalDisks->RemoveInstanceById(name->GetStrValue());
            }
            else if (eSCX_FileSystem == disktype)
            {
                cmdok = m_statisticalLogicalDisks->RemoveInstanceById(name->GetStrValue()) &&
                    m_staticLogicalDisks->RemoveInstanceById(name->GetStrValue());
            }
            else
            {
                SCX_LOGERROR(m_log, StrAppend(L"DiskProvider::DoInvokeMethod: Unknown disk type: ", disktype));
            }
            result.SetValue(cmdok);
        }
        else
        {
            throw SCXInternalErrorException(StrAppend(L"Unhandled method name: ", methodname), SCXSRCLOCATION);
        }
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

