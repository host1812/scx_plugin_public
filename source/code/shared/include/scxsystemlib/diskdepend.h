/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Defines the dependency interface for disk data retrieval

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/
#ifndef DISKDEPEND_H
#define DISKDEPEND_H

#if defined(aix)
#include <libperfstat.h>
#include <sys/mntctl.h>
#elif defined(hpux)
#include <sys/pstat.h>
#elif defined(sun)
#include <scxsystemlib/scxkstat.h>
#endif
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/logsuppressor.h>
#include <scxsystemlib/scxlvmtab.h>
#include <scxsystemlib/scxraid.h>
#include <map>

#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

namespace SCXSystemLib
{
    /**
       Common data type for 64-bit statvfs() system call
     */
    typedef struct statvfs64  SCXStatVfs;

    /**
       The types of disks interfaces recognized by the disk PAL.

    */
    enum DiskInterfaceType
    {
        eDiskIfcUnknown = 0,
        eDiskIfcIDE,
        eDiskIfcSCSI,
        eDiskIfcVirtual,
        eDiskIfcMax
    };

    /** Represents a device instance. */
    struct DeviceInstance
    {
        std::wstring m_name;     //!< Instance name.
        scxlong      m_instance; //!< Instance number.
        scxlong      m_devID;    //!< Device ID.
    };

    /** Represents a single row in /etc/mtab (/etc/mnttab) */
    struct MntTabEntry
    {
        std::wstring device;       //!< Device path
        std::wstring fileSystem;   //!< File system name
        std::wstring mountPoint;   //!< Mount point (root) of file system.
        std::wstring devAttribute; //!< Device attribute value (or empty if no such attribute).
    };

    /*----------------------------------------------------------------------------*/
    /**
       Define the interface for disk dependencies.
    */
    class DiskDepend
    {
    public:
        static const scxlong s_cINVALID_INSTANCE; //!< Constant representing an invalid instance.

        virtual ~DiskDepend() { } //!< Virtual destructor

        /**
            Get the path to mount tab file.

            \returns path to mount tab file.
        */
        virtual const SCXCoreLib::SCXFilePath& LocateMountTab() = 0;

        /**
           Get the path to the diskstats file.

           \returns the path to the diskstats file.
        */
        virtual const SCXCoreLib::SCXFilePath& LocateProcDiskStats() = 0;

        /**
           Refresh the disk stats file cache.
        */
        virtual void RefreshProcDiskStats() = 0;

        /**
           Get a proc disk stats row.

           \param device The device we want statistics for.
           \returns A vector with the stats tokenized as a vector of strings.
        */
        virtual const std::vector<std::wstring>& GetProcDiskStats(const std::wstring& device) = 0;

        /**
           Get a list of files in a directory.

           \param[in] path Path to a directory.
           \param[out] files A vector of file paths to all files in the given directory.

           \note The files vector is cleared and will be empty if the given directory does
           not exist.
        */
        virtual void GetFilesInDirectory(const std::wstring& path, std::vector<SCXCoreLib::SCXFilePath>& files) = 0;

        /**
            Get a parsed version of lvmtab.

            \returns a SCXLvmTab object.
        */
        virtual const SCXLvmTab& GetLVMTab() = 0;

        /**
            Get a parsed version of mount tab.

            \returns a vector of MntTabEntry objects.
        */
        virtual const std::vector<MntTabEntry>& GetMNTTab() = 0;

        /**
            Refresh the mount tab state.
        */
        virtual void RefreshMNTTab() = 0;

        /**
            Check if a given file system should be ignored or not.

            \param       fs Name of file system.
            \returns     true if the file system should be ignored, otherwise false.

            Ignored file systems are file systems we know we will not want to monitor.
            For example CD/DVD devices, system devices etc.
        */
        virtual bool FileSystemIgnored(const std::wstring& fs) = 0;

        /**
            Checks if the given device should be given in the given enumeration.

            \param[in] device  the device to check.

            \return This method will return true if the given device should be
                    ignored; false otherwise.

            Devices may be ignored because they are know to cause problems.  For
            example CD/DVD devices on Solaris, and LVM on old Linux distributions.
        */
        virtual bool DeviceIgnored(const std::wstring& device) = 0;

        /**
            Check if a given file system is represented by 'known' physical device.
            in mnttab file. Currently, we do not know how to get list of
            physical device(s) for ZFS filesystem, there is also the issue that
            on for example a solaris zone there is no physical disk so the info
            in mnttab is the same for device and mountpoint.

            \param       fs Name of file system.
            \param       dev_path Device path fount in mount table.
            \param       mountpoint Mount point found in mount table.
            \returns     true if the file system has 'real' device in mnt-tab.
        */
        virtual bool LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint) = 0;

        /**
            Decide interface type from the device name.

            \param dev device name.
            \returns a disk interface type.
        */
        virtual DiskInterfaceType DeviceToInterfaceType(const std::wstring& dev) const = 0;

        /**
            Given a device path from mount tab file, return related physical devices.

            \param device A device path as found in mount tab file.
            \returns A string map (name -> device path) with all physical devices
            related to given device.

            Several devices may be returned if the device for example is a logical volume.
        */
        virtual std::map<std::wstring, std::wstring> GetPhysicalDevices(const std::wstring& device) = 0;

#if defined(sun)
        /**
           Read a kstat object from a given path.

           \param kstat Kstat object to use when reading.
           \param[out] kstat_name Will contain the name of the kstat object read if successful.
           \param dev_path Path to device ex: /dev/dsk/c0t0d0s0
           \param isPhysical True if the device is a physical disk, otherwise false.
           \returns true if the read was successful, otherwise false.
        */
        virtual bool ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, std::wstring& kstat_name, const std::wstring& dev_path, bool isPhysical) = 0;
#endif
        /**
           Add a device instance to the device instance cache.

           \param device Device path
           \param name Device name
           \param instance Device instance number
           \param devID Device ID number

           Typically used to cache information needed to create KStat paths.
        */
        virtual void AddDeviceInstance(const std::wstring& device, const std::wstring& name, scxlong instance, scxlong devID) = 0;

        /**
           Find a device instance in the device instance cache.

           \param device Device path searched for.
           \returns A pointer to a device instance object or zero if no object is found in the cache.
        */
        virtual SCXCoreLib::SCXHandle<DeviceInstance> FindDeviceInstance(const std::wstring& device) const = 0;

        /**
           Wrapper for the system call open.

           \param pathname Path to file to open.
           \param flags open flags.
           \returns A new file descriptor or -1 if open fails.
        */
        virtual int open(const char* pathname, int flags) = 0;

        /**
           Wrapper for system call close.

           \param fd File descriptor to close.
           \returns Zero on success, otherwise -1.
        */
        virtual int close(int fd) = 0;

        /**
           Wrapper for the system call ioctl.

           \param d File descriptor.
           \param request Type of ioctl request.
           \param data Data related to the request.
           \returns -1 for errors.
        */
        virtual int ioctl(int d, int request, void* data) = 0;

        /**
            Wrapper for the system call statvfs.

            \param path Path to file to check.
            \param buf statvfs structure to fill with result.
            \returns zero on success, otherwise -1.
        */
        virtual int statvfs(const char* path, SCXStatVfs* buf) = 0;

        /**
           Wrapper for the system call lstat.

           \param path Path to file to stat.
           \param buf stat structure to fill with result.
           \returns zero on success, otherwise -1.
        */
        virtual int lstat(const char* path, struct stat *buf) = 0;

        /**
           Wrapper for file exists calls to static method.

           \param path Path to test if it exists.
           \returns true if path exists, otherwise false.
        */
        virtual bool FileExists(const std::wstring& path) = 0;

#if defined(hpux)
        /**
           Wrapper for sysem call pstat_getdisk

           \param buf Pointer to one or more pst_diskinfo structure.
           \param elemsize Size of each element (size of the struct).
           \param elemcount Number of elements to retreive (number of structs pointed to).
           \param index Element offset.
           \returns -1 on failure or number of elements returned.
        */
        virtual int pstat_getdisk(struct pst_diskinfo* buf, size_t elemsize, size_t elemcount, int index) = 0;

        /**
           Wrapper for sysem call pstat_getlv

           \param buf Pointer to one or more pst_lvinfo structure.
           \param elemsize Size of each element (size of the struct).
           \param elemcount Number of elements to retreive (number of structs pointed to).
           \param index Element offset.
           \returns -1 on failure or number of elements returned.
        */
        virtual int pstat_getlv(struct pst_lvinfo* buf, size_t elemsize, size_t elemcount, int index) = 0;
#endif /* hpux */
#if defined(aix)
        /**
           Wrapper for system call perfstat_disk.

           \param name first perfstat ID wanted and next is returned.
           \param buf Buffer to hold returned data.
           \param struct_size Size of the struct returned.
           \param n Desired number of structs to return.
           \returns Number of structures returned.
        */
        virtual int perfstat_disk(perfstat_id_t* name, perfstat_disk_t* buf, size_t struct_size, int n) = 0;

        /**
           Wrapper for system call mntctl.

           \param command Command to perform.
           \param size Size of buffer.
           \param buf Buffer to fill with data.
           \returns Number of vmount structures copied into the buffer
        */
        virtual int mntctl(int command, int size, char* buf) = 0;
#endif /* aix */
    protected:
        DiskDepend() { } //!< Protected default constructor
    };

    /*----------------------------------------------------------------------------*/
    /**
       Implement default behaviour for DiskDepend.
    */
    class DiskDependDefault : public DiskDepend
    {
    private:
        void InitializeObject();
    public:
        DiskDependDefault();
        DiskDependDefault(const SCXCoreLib::SCXLogHandle& log);
        virtual ~DiskDependDefault();

        virtual const SCXCoreLib::SCXFilePath& LocateMountTab();
        virtual const SCXCoreLib::SCXFilePath& LocateProcDiskStats();
        virtual void RefreshProcDiskStats();
        virtual const std::vector<std::wstring>& GetProcDiskStats(const std::wstring& device);
        virtual void GetFilesInDirectory(const std::wstring& path, std::vector<SCXCoreLib::SCXFilePath>& files);
        virtual const SCXLvmTab& GetLVMTab();
        virtual const std::vector<MntTabEntry>& GetMNTTab();
        virtual void RefreshMNTTab();
        virtual bool FileSystemIgnored(const std::wstring& fs);
        virtual bool DeviceIgnored(const std::wstring& device);
        virtual bool LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint);
        virtual bool LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint, SCXCoreLib::LogSuppressor& suppressor);
        virtual DiskInterfaceType DeviceToInterfaceType(const std::wstring& dev) const;
        virtual std::map<std::wstring, std::wstring> GetPhysicalDevices(const std::wstring& device);
#if defined(sun)
        virtual bool ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, std::wstring& kstat_name, const std::wstring& dev_path, bool isPhysical);
#endif
        virtual void AddDeviceInstance(const std::wstring& device, const std::wstring& name, scxlong instance, scxlong devID);
        virtual SCXCoreLib::SCXHandle<DeviceInstance> FindDeviceInstance(const std::wstring& device) const;

        /**
           \copydoc SCXSystemLib::DiskDepend::open
        */
        virtual int open(const char* pathname, int flags) { return ::open(pathname, flags); }

        /**
           \copydoc SCXSystemLib::DiskDepend::close
        */
        virtual int close(int fd) { return ::close(fd); }

        /**
           \copydoc SCXSystemLib::DiskDepend::ioctl
        */
        virtual int ioctl(int d, int request, void* data) { return ::ioctl(d, request, data); }

        /**
           \copydoc SCXSystemLib::DiskDepend::statvfs
        */
        virtual int statvfs(const char* path, SCXStatVfs* buf) { return ::statvfs64(path, buf); }

        /**
           \copydoc SCXSystemLib::DiskDepend::lstat
        */
        virtual int lstat(const char* path, struct stat *buf) { return ::lstat(path, buf); }

        virtual bool FileExists(const std::wstring& path);
#if defined(hpux)
        /**
           \copydoc SCXSystemLib::DiskDepend::pstat_getdisk
        */
        virtual int pstat_getdisk(struct pst_diskinfo* buf, size_t elemsize, size_t elemcount, int index)
        {
            return ::pstat_getdisk(buf, elemsize, elemcount, index);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::pstat_getlv
        */
        virtual int pstat_getlv(struct pst_lvinfo* buf, size_t elemsize, size_t elemcount, int index)
        {
            return ::pstat_getlv(buf, elemsize, elemcount, index);
        }
#endif /* hpux */
#if defined(aix)
        /**
           \copydoc SCXSystemLib::DiskDepend::perfstat_disk
        */
        virtual int perfstat_disk(perfstat_id_t* name, perfstat_disk_t* buf, size_t struct_size, int n)
        {
            return ::perfstat_disk(name, buf, struct_size, n);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::mntctl
        */
        virtual int mntctl(int command, int size, char* buf)
        {
            return ::mntctl(command, size, buf);
        }
#endif /* aix */
    private:
        SCXCoreLib::SCXLogHandle m_log; //!< Log handle.
    protected:
        typedef std::map<std::wstring, SCXCoreLib::SCXHandle<DeviceInstance> >  DeviceMapType;  //!< Type used for the device-path-to-instance map
        SCXCoreLib::SCXFilePath m_MntTabPath; //!< path to mount tab file.
        SCXCoreLib::SCXFilePath m_ProcDiskStatsPath; //!< path to proc diskstats file.
        SCXCoreLib::SCXHandle<SCXLvmTab> m_pLvmTab; //!< A parsed lvmtab file object.
        SCXCoreLib::SCXHandle<SCXRaid> m_pRaid; //!< A parsed RAID configuration.
        std::vector<MntTabEntry> m_MntTab; //!< A parsed mnttab object.
        DeviceMapType m_deviceMap; //!< Device path to instance map.
        std::map<std::wstring, std::vector<std::wstring> > m_ProcDiskStats; //!< parsed /proc/diskstats data.
        std::map<std::wstring, std::wstring> m_fsMap; //!< Used to map filesystem identifiers to names.

        virtual bool FileSystemNoLinkToPhysical(const std::wstring& fs);
#if defined(sun)
        virtual bool GuessKstatPath(const std::wstring& dev_path, std::wstring& module, std::wstring& name, scxlong& instance, bool isPhysical);
        virtual bool GuessKstatPath(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path, std::wstring& module, std::wstring& name, scxlong& instance, bool isPhysical);
#endif
        std::wstring GuessPhysicalFromLogicalDevice(const std::wstring& logical_dev);
        std::wstring RemoveTailNumberOrOther(const std::wstring& str);

        /** Declaring a compare function type to be used with IsStringInArray method */
        typedef bool CompareFunction(const std::wstring& needle, const std::wstring& heystack);

        static bool IsStringInArray(const std::wstring& str, const std::wstring* arr, CompareFunction compare);

        static bool CompareEqual(const std::wstring& needle, const std::wstring& heystack);
        static bool CompareStartsWith(const std::wstring& needle, const std::wstring& heystack);
        static bool CompareContains(const std::wstring& needle, const std::wstring& heystack);
    };


} /* namespace SCXSystemLib */
#endif /* DISKDEPEND_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
