/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Defines the static disk information instance PAL for physical disks.

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/
#ifndef STATICPHYSICALDISKINSTANCE_H
#define STATICPHYSICALDISKINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/diskdepend.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

#if defined(aix)

#include <odmi.h>

// Forward declaration
struct CuVPD;

#endif

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Represents a single disk instance with static data.
*/
    class StaticPhysicalDiskInstance : public EntityInstance
    {
        friend class StaticPhysicalDiskEnumeration;

    public:
        StaticPhysicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps);
        virtual ~StaticPhysicalDiskInstance();

        bool GetHealthState(bool& healthy) const;
        bool GetDiskName(std::wstring& value) const;
        bool GetDiskDevice(std::wstring& value) const;

        bool GetInterfaceType(DiskInterfaceType& value) const;
        bool GetManufacturer(std::wstring& value) const;
        bool GetModel(std::wstring& value) const;
        bool GetSizeInBytes(scxulong& value) const;
        bool GetTotalCylinders(scxulong& value) const;
        bool GetTotalHeads(scxulong& value) const;
        bool GetTotalSectors(scxulong& value) const;
        bool GetSectorSize(unsigned int& value) const;

        /** Set the device ID for this instance, e.g. /dev/sda */
        void SetDevice( const std::wstring& device ) {m_device = device;}

        virtual const std::wstring DumpString() const;
        virtual void Update();
        
        virtual void  SetUnexpectedException( const SCXCoreLib::SCXException& e );

    private:
        //! Private constructor (this should never be called!)
        StaticPhysicalDiskInstance();            //!< Default constructor (intentionally not implemented)

#if defined(aix)
        // Some AIX-specific routines to reduce complexity
        void DecodeVPD(const struct CuVPD *vpdItem);
        int LookupODM(CLASS_SYMBOL c, const std::wstring &criteria, void *pData);
#endif

        SCXCoreLib::SCXHandle<DiskDepend> m_deps;//!< StaticDiskDepend object
        SCXCoreLib::SCXLogHandle m_log;          //!< Log handle
        bool m_online;                           //!< Tells if disk is still connected.
        std::wstring m_device;                   //!< Device ID (i.e. /dev/sda)
        std::wstring m_rawDevice;                //!< Raw device name (internal use only)

        DiskInterfaceType m_intType;             //!< Interface type of device (IDE, SCSI, etc)
        std::wstring m_manufacturer;             //!< Disk drive manufacturer
        std::wstring m_model;                    //!< Disk drive model
        scxulong m_sizeInBytes;                  //!< Total size, in bytes
        scxulong m_totalCylinders;               //!< Total number of cylinders
        scxulong m_totalHeads;                   //!< Total number of heads
        scxulong m_totalSectors;                 //!< Total number of sectors
        unsigned int m_sectorSize;               //!< Sector size, in bytes
    };
} /* namespace SCXSystemLib */
#endif /* STATICPHYSICALDISKINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
