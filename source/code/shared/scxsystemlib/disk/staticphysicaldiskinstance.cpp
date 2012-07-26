/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Implements the physical disk instance pal for static information.

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/staticphysicaldiskinstance.h>

/* System specific includes - generally for Update() method */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(linux)

#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/types.h>
#include <sys/ioctl.h>

#elif defined(aix)

#include <scxsystemlib/scxodm.h>

#elif defined(hpux)

#include <sys/diskio.h>
#include <sys/scsi.h>

#elif defined(sun)

#include <sys/dkio.h>
#include <sys/vtoc.h>

#endif


#if defined(aix)

// Debug output decoding a CuVPD structure on AIX
#include <iostream>
const bool debugDumpVPD = 0;

#endif


using namespace SCXCoreLib;

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Constructor.

    \param       deps A StaticDiscDepend object which can be used.

*/
    StaticPhysicalDiskInstance::StaticPhysicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps)
        : m_deps(0), m_online(0), m_intType(eDiskIfcUnknown),
          m_sizeInBytes(0), m_totalCylinders(0), m_totalHeads(0), m_totalSectors(0),
          m_sectorSize(0)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.staticphysicaldiskinstance");
        m_deps = deps;
        m_rawDevice = L"";
    }

/*----------------------------------------------------------------------------*/
/**
    Virtual destructor.
*/
    StaticPhysicalDiskInstance::~StaticPhysicalDiskInstance()
    {
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the disk health state.

    \param       healthy - output parameter where the health status of the disk is stored.
    \returns     true if value was set, otherwise false.
    */
    bool StaticPhysicalDiskInstance::GetHealthState(bool& healthy) const
    {
        healthy = m_online;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the disk name (i.e. 'sda' on Linux).

    \param       value - output parameter where the name of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetDiskName(std::wstring& value) const
    {
        value = GetId();
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the disk name (i.e. '/dev/sda' on Linux).

    \param       value - output parameter where the device of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetDiskDevice(std::wstring& value) const
    {
        value = m_device;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the disk interface type.

    \param       value - output parameter where the interface of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetInterfaceType(DiskInterfaceType& value) const
    {
        value = m_intType;
#if defined(linux) || defined(hpux) || defined(sun) || defined(aix)
        return true;
#else
#error Define return type on platform for method GetInterfaceType
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the manufacturer of the device.

    \param       value - output parameter where the manufacturer of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetManufacturer(std::wstring& value) const
    {
        value = m_manufacturer;
#if defined(linux) || defined(sun)
        return false;
#elif defined(aix) || defined(hpux)
        return true;
#else
#error Define return type on platform for method GetManufacturer
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the model of the device.

    \param       value - output parameter where the model of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetModel(std::wstring& value) const
    {
        value = m_model;
#if defined(linux) || defined(aix) || defined(hpux)
        return true;
#elif defined(sun)
        return false;
#else
#error Define return type on platform for method GetModel
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the total size of the device, in bytes.

    \param       value - output parameter where the size of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSizeInBytes(scxulong& value) const
    {
        value = m_sizeInBytes;
#if defined(linux) || defined(aix) || defined(hpux) || defined(sun)
        return true;
#else
#error Define return type on platform for method GetSizeInBytes
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the cylinder count of the device.

    \param       value - output parameter where the cylinder count of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTotalCylinders(scxulong& value) const
    {
        value = m_totalCylinders;
#if defined(linux) || defined(sun)
        return true;
#elif defined(aix) || defined(hpux)
        return false;
#else
#error Define return type on platform for method GetTotalCylinders
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the head count of the device.

    \param       value - output parameter where the head count of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTotalHeads(scxulong& value) const
    {
        value = m_totalHeads;
#if defined(linux) || defined(sun)
        return true;
#elif defined(aix) || defined(hpux)
        return false;
#else
#error Define return type on platform for method GetTotalHeads
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the sector count of the device.

    \param       value - output parameter where the sector count of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTotalSectors(scxulong& value) const
    {
        value = m_totalSectors;
#if defined(linux) || defined(sun)
        return true;
#elif defined(aix) || defined(hpux)
        return false;
#else
#error Define return type on platform for method GetTotalSectors
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the sector size of the device.

    \param       value - output parameter where the sector size of the disk is stored.
    \returns     true if value is supported on platform, false otherwise.

    Note: The sector size is almost always 512 bytes, if not always.  However, if we can't get
    it for a platform, we return FALSE, and the provider can provide a default if desired.
*/
    bool StaticPhysicalDiskInstance::GetSectorSize(unsigned int& value) const
    {
        value = m_sectorSize;
#if defined(aix) || defined(linux)
        return false;
#elif defined(hpux) || defined(sun)
        return true;
#else
#error Define return type on platform for method GetSectorSize
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Dump the object as a string for logging purposes.
    
    \returns     String representation of object, suitable for logging.
 */
    const std::wstring StaticPhysicalDiskInstance::DumpString() const
    {
        return SCXDumpStringBuilder("StaticPhysicalDiskInstance")
            .Text("Name", GetId())
            .Text("Device", m_device)
            .Text("RawDevice", m_rawDevice)
            .Scalar("Online", m_online)
            .Scalar("InterfaceType", m_intType)
            .Text("Manufacturer", m_manufacturer)
            .Text("Model", m_model)
            .Scalar("SizeInBytes", m_sizeInBytes)
            .Scalar("TotalCylinders", m_totalCylinders)
            .Scalar("TotalHeads", m_totalHeads)
            .Scalar("TotalSectors", m_totalSectors)
            .Scalar("SectorSize", m_sectorSize);
    }

#if defined(aix)
/*----------------------------------------------------------------------------*/
/**
    Decode a VPD ("Vital Product Data") structure on the AIX platform.
    Object is updated as appropriate for the AIX platform.

    \param       vpdItem - CuVPD structure from AIX platform
    \returns     Updated object (for manufacturer and model)
 */

    void StaticPhysicalDiskInstance::DecodeVPD(const struct CuVPD *vpdItem)
    {
        // Decodes a VPD (Vital Product Data) on the AIX platform.
        //
        // A VPD structure can be in two forms:
        // 1. Section 12.5 of document "Standard for Power Architecture Platform
        //    Requirements (Workstation, Server)", Version 2.2, dated 10/9/1007.
        //    This document is available on power.org, and has also been posted
        //    to the Wiki (Documents/Networking and Protocols), as well as a
        //    link in WI 3344.
        // 2. From IBM's old microchannel bus machines which IBM still uses
        //    for devices that are still around from those days.
        //
        // We only decode the old microchannel bus machines.  All disks
        // (except perhaps SAS disks - we can't test) return their VPD data
        // in the old microchannel bus format.  If we can't decode the VPD
        // data, then the manufacturer & model simply isn't provided.
        //
        // If, at a future date, AIX returns data as per section 12.5, then
        // this code can be updated to support that.
        //
        // This information comes via a personal contact at IBM, and is not
        // formally documented by IBM.
        //
        // The old microchannel bus format will always start with an asterick
        // (*) character.  In general, it can be described as consisting of one
        // or more VPD keywords with each one having the following structure:
        //
        //    *KKLdd...d
        //
        // where:
        //    *       = The asterick character ('*')
        //    KK      = A two-character keyword
        //    L       = A one byte binary value of half the length of the entire
        //              "*KKLdd...d" string of bytes.  In other words, 2*L is
        //              the length of the string of bytes. 
        //    dd....d = The string of actual VPD data for the keyword.  This may
        //              be binary or ascii string (not NULL terminated) depending
        //              on the keyword.
        //
        // Note: 2*L includes the 4 bytes for the *KKL bytes as well as the
        // number of bytes in dd...d.  Also note that because L has to be
        // doubled, the length is always an even number of bytes.  If there is
        // an odd number of bytes in the dd...d value, then it must be padded
        // with an 0x00 byte.
        //
        // There should be an 0x00 byte following the last *KKdd...d.  So you
        // process the data in a loop looking for * characters.  The L value can
        // be used to determine where the next '*' character should be.  However,
        // if it is 0x00, then you are done.

        const char *p = vpdItem->vpd;
        if (*p != '*')
        {
            // If we're not in old microchannel bus format, we're done
            return;
        }

        std::string tag;
        std::vector<char> value;
        value.resize(sizeof(vpdItem->vpd) + 1, 0);

        while (*p == '*' && p < (vpdItem->vpd + sizeof(vpdItem->vpd)) ) {
            // Length includes "*xxl", where XX=2 char ID and l==total len
            int totalLen = ( p[3] * 2 );
            int itemLen = totalLen - 4;

            tag.clear();
            tag.assign( p+1, p+1+2 );

            SCXASSERT( itemLen < sizeof(vpdItem->vpd) );
            memcpy(&value[0], &p[4], itemLen);
            value[itemLen] = '\0';

            if ( debugDumpVPD )
            {
                std::cout << "  Tag: " << tag.c_str() << ", Value: " << &value[0] << std::endl;
            }

            if (0 == tag.compare("MF"))
            {
                m_manufacturer = StrTrim(StrFromMultibyte(&value[0]));
            }
            else if (0 == tag.compare("TM"))
            {
                m_model = StrTrim(StrFromMultibyte(&value[0]));
            }

            p += totalLen;
        }

        return;
    }

/*----------------------------------------------------------------------------*/
/**
    Look up some data via the ODM (Object Data Model) interface on AIX.

    \param       class - Class symbol to be looked up
    \param       criteria - Search criteria to use
    \param       pData - Class-specific data structure to return data into

    \returns     -1 if internal error occurred, 0 if search criteria failed, > 1 if lookup succeeded
 */

    int StaticPhysicalDiskInstance::LookupODM(CLASS_SYMBOL c, const std::wstring &wCriteria, void *pData)
    {
        void *pResult;

        try {
            SCXodm odm;

            pResult = odm.Get(c, wCriteria, pData);
            if (NULL == pResult)
            {
                return 0;
            }
        }
        catch (SCXodmException)
        {
            return -1;
        }

        SCXASSERT( pResult == pData );
        return 1;
    }
#endif

/*----------------------------------------------------------------------------*/
/**
    Update the instance.

       \throws      SCXErrnoOpenException when system calls fail.
*/
    void StaticPhysicalDiskInstance::Update()
    {
        /*
         * In some cases, we must modify the device ID (m_device) that was
         * passed to us by provider (for example, the provider never passes
         * the raw device name, but we need that). Rather than changing the
         * real device, and then potentially running into problems if the
         * device ID doesn't match with the device ID from other sources,
         * we make a copy of the device in m_rawDevice.  Then we can change
         * this, as needed, for various platforms.
         *
         * The m_rawDevice is for internal use only, and never shared via a
         * Get() method.
         */

        if (m_rawDevice.length() == 0)
        {
            m_rawDevice = m_device;
        }

#if defined(linux)

        int fd = 0;

        /* Get an FD to the device (Note: We must have privileges for this to work) */

        if (0 > (fd = m_deps->open(StrToMultibyte(m_rawDevice).c_str(), O_RDONLY)))
        {
            throw SCXErrnoOpenException(m_rawDevice, errno, SCXSRCLOCATION);
        }

        /*
         * Determine the interface for the device
         *
         * For Linux, our choices are "sd*" for SCSI, "hd*" for IDE.  Just look at the first
         * byte to figure out what device this is.  Crude but effective.  For things (like
         * fiber) that look like SCSI to the O/S, we'll just return SCSI.
         */

        switch (GetId()[0])
        {
        case L'h':
            m_intType = eDiskIfcIDE;
            break;

        case L's':
            m_intType = eDiskIfcSCSI;
            break;

        case L'x':
            if (GetId()[1] == 'v' && GetId()[2] == 'd')
            {
                // On Linux, 'xvd' devices are virtual disks hosted on Xen
                m_intType = eDiskIfcVirtual;
                break;
            }
            // Fall Through

        default:
            m_intType = eDiskIfcUnknown;
        }

        /* Get the blocksize of the device */

        unsigned int blksize32 = 0;
        u_int64_t blksize64 = 0;

#ifdef BLKGETSIZE64
        if (0 == ioctl(fd, BLKGETSIZE64, &blksize64))
        {
            // Returns bytes, so convert
            blksize64 /= 512;
        }
#endif

        if (blksize64 == 0)
        {
            if (0 == m_deps->ioctl(fd, BLKGETSIZE, &blksize32))
            {
                blksize64 = blksize32;
            }
            else
            {
                SCX_LOGERROR(m_log, L"System error getting disk blocksize, errno=" + StrFrom(errno));
            }
        }

        m_sizeInBytes = blksize64 / 2 * 1024;

        /* Get the drive geometry */

        struct hd_geometry g;
        memset(&g, '\0', sizeof(g));
        if (0 == m_deps->ioctl(fd, HDIO_GETGEO, &g))
        {
            m_totalCylinders = g.cylinders;
            m_totalHeads = g.heads;
            m_totalSectors = g.sectors;
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting disk geometry, errno=" + StrFrom(errno));
        }

        /* Get drive identity (note: only works for IDE drives; silently ignore any error) */

        __u16 id[256];
        memset(&id, '\0', sizeof(id));
        if (0 == m_deps->ioctl(fd, HDIO_GET_IDENTITY, id))
        {
            /*
             * We get: Model (%.40s, starting at id[27]),
             *         Firmware Revision (%.8s, starting at id[23]), and
             *         Serial Number (%.20s, starting at id[10])
             *
             * We want:
             *         Manufacturer
             *         Model
             *
             * On Linux, we leave Manufacturer blank and just map Model
             */

            /* We get a space-filled string - make it null terminated */
            char model[40 + 1];
            memset(model, '\0', sizeof(model));
            memcpy(model, &id[27], sizeof(model)-1);
            m_model = StrTrim(StrFromMultibyte(model));
        }

        /* Close the handle to the drive */

        if (m_deps->close(fd))
        {
            /* Can't imagine that we can't close the fd, but if so, we don't want to miss that */

            throw SCXErrnoException(L"close", errno, SCXSRCLOCATION);
        }

#elif defined(aix)

        int res;

        /*
         * On AIX, we use the ODM interface (Object Data Model).  We query the
         * ODM for various information that we need.  The ODM is populated at
         * boot time with the hardware that the system contains (plus a bunch
         * of other stuff).
         */

        std::wstring criteria = L"name=" + GetId();
        bool fIsVirtualDisk = false;

        // Get the CuDv object to give us the interface type
        // This also tells us if this is a virtual disk ...
        //
        // If it's a virtual disk, minimal information is avalable, so we do
        // the best we can do and provide reasonable defaults for the rest.

        struct CuDv dvData;
        memset(&dvData, '\0', sizeof(dvData));
        switch (LookupODM(CuDv_CLASS, criteria, &dvData))
        {
            case 0: /* Huh?  Criteria failed???  We're broken ... */
                throw SCXInternalErrorException(L"Invalid ODM (CuDv) criteria: " + criteria, SCXSRCLOCATION);
                break;

            case 1: /* Success case */
            {
                // The interface, found in PdDvLn_Lvalue, is of the form:
                //    <class>/<subclass>/<type>
                // The <subclass> will be a string like "scsi", or "iscsi",
                // or "ide" (perhaps on AIX for x86).

                std::vector<std::wstring> parts;
                StrTokenize(StrFromMultibyte(dvData.PdDvLn_Lvalue), parts, L"/");

                if (0 == StrCompare(parts[1], L"scsi", true)
                    || 0 == StrCompare(parts[1], L"iscsi", true))
                {
                    m_intType = eDiskIfcSCSI;
                }
                else if (0 == StrCompare(parts[1], L"ide", true))
                {
                    m_intType = eDiskIfcIDE;
                }
                else if (0 == StrCompare(parts[1], L"vscsi", true))
                {
                    m_intType = eDiskIfcSCSI;
                    fIsVirtualDisk = true;
                }
                else if (0 == StrCompare(parts[1], L"vide", true))
                {
                    m_intType = eDiskIfcIDE;
                    fIsVirtualDisk = true;
                }

                break;
            }

            default:
                /* Ignore all other values (just case -1) */
                break;
        }

        if ( ! fIsVirtualDisk )
        {
            // Get the VPD data, which gives us manufacturer and model

            struct CuVPD vpdData;
            memset(&vpdData, '\0', sizeof(vpdData));
            switch (LookupODM(CuVPD_CLASS, criteria, &vpdData))
            {
                case 0: /* Huh?  Criteria failed???  We're broken ... */
                    throw SCXInternalErrorException(L"Invalid ODM (CuVPD) criteria: " + criteria, SCXSRCLOCATION);
                    break;

                case 1: /* Success case */
                    DecodeVPD(&vpdData);
                    break;

                default:
                    /* Ignore all other values (just case -1) */
                    break;
            }

            // Next get the CuAt object, which gives us the size
            // Note: On some devices, this isn't available ...
            std::wstring attrCriteria = criteria + L" and attribute=size_in_mb";
            struct CuAt atData;
            memset(&atData, '\0', sizeof(atData));
            if (1 == LookupODM(CuAt_CLASS, attrCriteria, &atData))
            {
                m_sizeInBytes = atol(atData.value);
                m_sizeInBytes *= 1024 * 1024; /* Get from MB to bytes */
            }
        }
        else
        {
            // Note: CuAt is present for virtual disks but doesn't have the size_in_mb attribute.
            //       CuVPD isn't available at all for firtual disks (thus our defaults here)

            m_manufacturer = L"IBM";        // Obviously
            m_model = L"Virtual";           // Reasonabe value (sort of)
            m_sizeInBytes = 0;              // Not able to determinate
        }

#elif defined(hpux)

        int fd = 0;

        /*
         * Get an FD to the device (Note: we must have privileges for this to work)
         *
         * Note: We usually get a device like "/dev/disk/disk3".  While we can open
         * this device, the ioctl()'s we call won't work unless we have the raw device
         * open.
         *
         * As a result, we bag the device we received and construct our own given the
         * name of the device.
         */

        if (m_rawDevice == m_device)
        { // Happens first time called
            if (std::wstring::npos != m_rawDevice.find(L"/dsk/"))
            {
                m_rawDevice = SCXCoreLib::StrAppend(L"/dev/rdsk/", GetId());
            }
            else
            {
                m_rawDevice = SCXCoreLib::StrAppend(L"/dev/rdisk/", GetId());
            }
        }
        if (0 > (fd = m_deps->open(StrToMultibyte(m_rawDevice).c_str(), O_RDONLY)))
        {
            throw SCXErrnoOpenException(m_rawDevice, errno, SCXSRCLOCATION);
        }

        /* Look up the disk manufacturer and model */

#if defined(hpux) && (PF_MAJOR==11) && (PF_MINOR<31)
        //
        // Need to use old-style structures for HP-UX 11i v2
        //

        struct inquiry_2 scsiData;
        memset(&scsiData, 0, sizeof(scsiData));
        if (0 == m_deps->ioctl(fd, SIOC_INQUIRY, &scsiData))
        {
            /* We got space-filled strings in our structure - null terminate 'em */

            char vendorID[sizeof(scsiData.vendor_id)+1], productID[sizeof(scsiData.product_id)+1];

            memset(vendorID, '\0', sizeof(vendorID));
            memset(productID, '\0', sizeof(productID));

            memcpy(vendorID, scsiData.vendor_id, sizeof(vendorID)-1);
            memcpy(productID, scsiData.product_id, sizeof(productID)-1);

            m_manufacturer = StrTrim(StrFromMultibyte(vendorID));
            m_model = StrTrim(StrFromMultibyte(productID));
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive inquiry data, errno=" + StrFrom(errno));
        }

        /* Look up capacity, interface type, etc */

        disk_describe_type diskDescr;
        memset(&diskDescr, 0, sizeof(diskDescr));
        if (0 == m_deps->ioctl(fd, DIOC_DESCRIBE, &diskDescr))
        {
            /* Set SCSI if we know that's what we've got (20=SCSI) */

            if (20 == diskDescr.intf_type)
            {
                m_intType = eDiskIfcSCSI;
            }

            /* Capacity and sector size */

            m_sizeInBytes = static_cast<scxulong> (diskDescr.maxsva);

            m_sectorSize = diskDescr.lgblksz;
            if (0 == m_sectorSize)
            {
                /* Hmm, didn't get a sector size - let's make a reasonable guess */
                m_sizeInBytes *= 512;
            }
            else
            {
                m_sizeInBytes *= m_sectorSize;
            }
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive description, errno=" + StrFrom(errno));
        }
#else
        inquiry_3_t scsiData;
        memset(&scsiData, 0, sizeof(scsiData));
        if (0 == m_deps->ioctl(fd, SIOC_INQUIRY, &scsiData))
        {
            /* We got space-filled strings in our structure - null terminate 'em */

            char vendorID[sizeof(scsiData.vendor_id)+1], productID[sizeof(scsiData.product_id)+1];

            memset(vendorID, '\0', sizeof(vendorID));
            memset(productID, '\0', sizeof(productID));

            memcpy(vendorID, scsiData.vendor_id, sizeof(vendorID)-1);
            memcpy(productID, scsiData.product_id, sizeof(productID)-1);

            m_manufacturer = StrTrim(StrFromMultibyte(vendorID));
            m_model = StrTrim(StrFromMultibyte(productID));
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive inquiry data, errno=" + StrFrom(errno));
        }

        /* Look up capacity, interface type, etc */

        disk_describe_type_ext_t diskDescr;
        memset(&diskDescr, 0, sizeof(diskDescr));
        if (0 == m_deps->ioctl(fd, DIOC_DESCRIBE_EXT, &diskDescr))
        {
            /* Set SCSI if we know that's what we've got (20=SCSI) */

            if (20 == diskDescr.intf_type)
            {
                m_intType = eDiskIfcSCSI;
            }

            /* Capacity and sector size */

            m_sizeInBytes = static_cast<scxulong> (diskDescr.maxsva_high);
            m_sizeInBytes = (m_sizeInBytes << 32) + diskDescr.maxsva_low;

            m_sectorSize = diskDescr.lgblksz;
            if (0 == m_sectorSize)
            {
                /* Hmm, didn't get a sector size - let's make a reasonable guess */
                m_sizeInBytes *= 512;
            }
            else
            {
                m_sizeInBytes *= m_sectorSize;
            }
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive description, errno=" + StrFrom(errno));
        }
#endif

        /* Close the handle to the drive */

        if (m_deps->close(fd))
        {
            /* Can't imagine that we can't close the fd, but if so, we don't want to miss that */

            throw SCXErrnoException(L"close", errno, SCXSRCLOCATION);
        }

#elif defined(sun)

        int fd = 0;

        /*
         * Get an FD to device (Note: We must have privileges for this to work)
         *
         * Note: We usually get a device like "/dev/dsk/c0d0", but this won't
         * work.  We try once (just in case), but if that fails, we build our
         * own path to look like: "/dev/rdsk/c0d0s0".  If that fails too, then
         * we just bag it.
         */

        SCX_LOGHYSTERICAL(m_log, L"Update(): trying disk device " + m_rawDevice );
        if (0 > (fd = m_deps->open(StrToMultibyte(m_rawDevice).c_str(), O_RDONLY)))
        {
            /* Reconstruct the path from the name and try again */
            /* Note that we need to check several slices if the disk does not use all of them. */
            for (int i = 0; i<=15 && fd < 0; ++i)
            {
                m_rawDevice = L"/dev/rdsk/" + GetId() + StrAppend(L"s", i);
                SCX_LOGHYSTERICAL(m_log, L"Update(): re-trying disk device " + m_rawDevice );
                if (0 > (fd = m_deps->open(StrToMultibyte(m_rawDevice).c_str(), O_RDONLY)))
                {
                    if ((EIO != errno && ENXIO != errno) || 15 <= i) // EIO _or_ ENXIO is received if the slice is not used.
                    {
                        throw SCXErrnoOpenException(m_rawDevice, errno, SCXSRCLOCATION);
                    }
                }
            }
        } 

        SCX_LOGHYSTERICAL(m_log, L"Update(): opened disk device " + m_rawDevice );
        

        /*
          Look up the size of the disk.

          We also use this call to determine if this is a disk at all.  On Sun,
          we get called for all sorts of devices that may not be hard drives at
          all ...
        */

        struct dk_minfo dkmedia;
        memset(&dkmedia, '\0', sizeof(dkmedia));
        if (0 == m_deps->ioctl(fd, DKIOCGMEDIAINFO, &dkmedia))
        {
            m_sizeInBytes = static_cast<scxulong> (dkmedia.dki_capacity) * dkmedia.dki_lbsize;

            // verify that media is supported
            if ( dkmedia.dki_media_type != DK_FIXED_DISK ){
                // close the descriptor - strange, but we don't use helper class
                // with 'close-in-dtor' here
                m_deps->close(fd);
                // this exception means that disk is not of interest (CD, tape, etc)
                throw SCXNotSupportedException(m_rawDevice + L" has unsupported type " + StrFrom(dkmedia.dki_media_type)
                    , SCXSRCLOCATION);
            }
        }
        else
        {
            SCX_LOGTRACE(m_log, L"System error getting disk media information, errno=" + StrFrom(errno));
        }

        /* Look up the drive interface type */

        struct dk_cinfo dkinfo;
        memset(&dkinfo, '\0', sizeof(dkinfo));
        if (0 == m_deps->ioctl(fd, DKIOCINFO, &dkinfo))
        {
            switch (dkinfo.dki_ctype)
            {
            case DKC_DIRECT:
                m_intType = eDiskIfcIDE;
                break;

            case DKC_SCSI_CCS:
                m_intType = eDiskIfcSCSI;
                break;
            }
        }
        else
        {
            SCX_LOGTRACE(m_log, L"System error getting disk interface information, errno=" + StrFrom(errno));
        }

        /* Look up drive geometry information */

        struct dk_geom dkgeom;
        memset(&dkgeom, '\0', sizeof(dkgeom));
        if (0 == m_deps->ioctl(fd, DKIOCGGEOM, &dkgeom))
        {
            m_totalCylinders = dkgeom.dkg_ncyl;
            m_totalHeads = dkgeom.dkg_nhead;
            m_totalSectors = dkgeom.dkg_nsect;
        }
        else
        {
            SCX_LOGTRACE(m_log, L"System error getting disk geometry, errno=" + StrFrom(errno));
        }

        /* Look up the drive's sector size */

        struct vtoc dkvtoc;
        memset(&dkvtoc, '\0', sizeof(dkvtoc));
        if (0 == m_deps->ioctl(fd, DKIOCGVTOC, &dkvtoc))
        {
            m_sectorSize = dkvtoc.v_sectorsz;
        }
        else
        {
            SCX_LOGTRACE(m_log, L"System error getting disk sector size, errno=" + StrFrom(errno));
        }

        /* Close the handle to the drive */

        if (m_deps->close(fd))
        {
            /* Can't imagine that we can't close the fd, but if so, we don't want to miss that */

            throw SCXErrnoException(L"close", errno, SCXSRCLOCATION);
        }

#else
    #error Implementation for Update() method not provided for platform
#endif        
    }

    /*----------------------------------------------------------------------------*/
    /**
        Called each time when 'Update' throws

        input parametr (exception from Update) is ignored
     */
    void StaticPhysicalDiskInstance::SetUnexpectedException( const SCXCoreLib::SCXException& )
    {
        // this function is invoked if Update throws;
        // mark disk as 'off-line'; keep the rest from previous
        m_online = false;
    }
    
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
