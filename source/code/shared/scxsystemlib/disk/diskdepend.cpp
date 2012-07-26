/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Implements the default implementation for disk dependencies

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>

#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/scxsysteminfo.h>

#if defined(aix)
#include <sys/vmount.h>
#include <vector>
#elif defined(linux)
#include <scxsystemlib/scxlvmutils.h>
#endif

namespace SCXSystemLib
{
    const scxlong DiskDepend::s_cINVALID_INSTANCE = -1;

    /*----------------------------------------------------------------------------*/
    /**
       Constructor used for injecting a log handle.
    */
    DiskDependDefault::DiskDependDefault(const SCXCoreLib::SCXLogHandle& log):
        m_log(log),
        m_pLvmTab(0),
        m_pRaid(0)
    {
        InitializeObject();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor
    */
    DiskDependDefault::DiskDependDefault():
        m_log(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.diskdepend")),
        m_pLvmTab(0),
        m_pRaid(0)
    {
        InitializeObject();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Private method that initializes the object. Called from the constructors.
    */
    void DiskDependDefault::InitializeObject()
    {
#if defined(aix)
#elif defined(linux)
        m_ProcDiskStatsPath.Set(L"/proc/diskstats");
        m_MntTabPath.Set(L"/etc/mtab");
#elif defined(sun) || defined(hpux)
        m_MntTabPath.Set(L"/etc/mnttab");
#else
#error "Platform not supported"
#endif
#if defined(aix)
        SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(L"/etc/vfs", std::ios::in));
        fs.SetOwner();
        while ( ! fs->eof() && fs->is_open())
        {
            std::wstring line;
            getline(*fs, line);
            if (line.length() == 0)
            {
                continue;
            }
            if (line.substr(0,1) == L"#" || line.substr(0,1) == L"%")
            { // Line is a comment.
                continue;
            }
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(line, parts, L" \n\t\r");
            if (parts.size() < 4)
            {
                continue;
            }
            m_fsMap[parts[1]] = parts[0];
        }
        fs->close();
#elif defined(sun)
        try
        {
            SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(L"/etc/path_to_inst", std::ios::in));
            fs.SetOwner();
            while ( ! fs->eof() && fs->is_open())
            {
                std::wstring line;
                getline(*fs, line);
                if (line.length() == 0)
                {
                    continue;
                }
                if (line.substr(0,1) == L"#")
                { // Line is a comment.
                    continue;
                }
                std::vector<std::wstring> parts;
                SCXCoreLib::StrTokenize(line, parts, L" \n\t");
                if (parts.size() < 3)
                {
                    continue;
                }
                SCXCoreLib::SCXHandle<DeviceInstance> di( new DeviceInstance );
                try
                {
                    di->m_instance = SCXCoreLib::StrToLong(parts[1]);
                }
                catch (SCXCoreLib::SCXNotSupportedException)
                {
                    di->m_instance = s_cINVALID_INSTANCE;
                }
                di->m_name = SCXCoreLib::StrStrip(parts[2], L"\" \t\n\r");
                m_deviceMap[SCXCoreLib::StrStrip(parts[0], L"\" \t\n\r")] = di;
            }
            fs->close();
        }
        catch (SCXCoreLib::SCXFilePathNotFoundException)
        {
            // The file '/etc/path_to_inst' may not exist (like in a zone).  If this
            // happens, then we have no physical disk devices that can be found ...
            //
            // If we're not in the global zone, then this is okay ...

            bool fIsInGlobalZone;
            SCXSystemLib::SystemInfo si;
            si.GetSUN_IsInGlobalZone( fIsInGlobalZone );

            if ( fIsInGlobalZone ) throw;
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       Virtual destructor
    */
    DiskDependDefault::~DiskDependDefault()
    {
    }


    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::LocateMountTab
    */
    const SCXCoreLib::SCXFilePath& DiskDependDefault::LocateMountTab()
    {
        return m_MntTabPath;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::LocateProcDiskStats
    */
    const SCXCoreLib::SCXFilePath& DiskDependDefault::LocateProcDiskStats()
    {
        return m_ProcDiskStatsPath;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::RefreshProcDiskStats
    */
    void DiskDependDefault::RefreshProcDiskStats()
    {
        m_ProcDiskStats.clear();

        SCXCoreLib::SCXHandle<std::wfstream> fsDiskStats(SCXCoreLib::SCXFile::OpenWFstream(LocateProcDiskStats(), std::ios::in));
        fsDiskStats.SetOwner();
        std::wstring line;
        while ( ! fsDiskStats->eof() && fsDiskStats->is_open())
        {
            getline(*fsDiskStats, line);
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(line, parts, L" \n\t");
            if (parts.size() < 3)
            {
                continue;
            }
            m_ProcDiskStats[parts[2]] = parts;
        }
        fsDiskStats->close();
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetProcDiskStats
    */
    const std::vector<std::wstring>& DiskDependDefault::GetProcDiskStats(const std::wstring& device)
    {
        SCXCoreLib::SCXFilePath dev(device);
        std::map<std::wstring, std::vector<std::wstring> >::const_iterator it =
            m_ProcDiskStats.find(dev.GetFilename());

        if (it == m_ProcDiskStats.end())
        {
            static std::vector<std::wstring> empty;
            return empty;
        }
        return it->second;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetFilesInDirectory
    */
    void DiskDependDefault::GetFilesInDirectory(const std::wstring& path, std::vector<SCXCoreLib::SCXFilePath>& files)
    {
        files.clear();
        if (SCXCoreLib::SCXDirectory::Exists(path))
        {
            files = SCXCoreLib::SCXDirectory::GetFileSystemEntries(path, SCXCoreLib::eDirSearchOptionFile | SCXCoreLib::eDirSearchOptionSys);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetLVMTab
    */
    const SCXLvmTab& DiskDependDefault::GetLVMTab()
    {
        if (0 ==  m_pLvmTab)
        {
            try
            {
                m_pLvmTab = new SCXLvmTab(L"/etc/lvmtab");
            }
            catch (SCXSystemLib::SCXLvmTabFormatException& e)
            {
                SCXRETHROW(e, SCXCoreLib::StrAppend(L"Wrong lvmtab format: ", e.What()));
            }
            catch (SCXCoreLib::SCXUnauthorizedFileSystemAccessException& e2)
            {
                SCXRETHROW(e2, L"Unable to parse /etc/lvmtab without root access");
            }
        }
        return *m_pLvmTab;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetMNTTab

       \note Not thread safe.
    */
    const std::vector<MntTabEntry>& DiskDependDefault::GetMNTTab()
    {
        return m_MntTab;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::RefreshMNTTab

       \note Not thread safe.
    */
    void DiskDependDefault::RefreshMNTTab()
    {
        if (0 < m_MntTab.size())
        {
            m_MntTab.clear();
        }
#if defined(aix)
        int needed = 0;
        // Get the number of bytes needed for all mntctl data.
        int r = mntctl(MCTL_QUERY, sizeof(needed), reinterpret_cast<char*>(&needed));
        if (0 == r)
        {
            std::vector<char> buf(needed);
            char* p = &buf[0];

            // Returns number of structs in buffer; use that to limit data walk
            r = mntctl(MCTL_QUERY, needed, &buf[0]);

            for (int i = 0; i < r; i++)
            {
                struct vmount* vmt = reinterpret_cast<struct vmount*>(p);
                std::wstring fs = SCXCoreLib::StrFrom(vmt->vmt_gfstype);
                if (vmt->vmt_data[VMT_OBJECT].vmt_size > 0 &&
                    vmt->vmt_data[VMT_STUB].vmt_size > 0 &&
                    m_fsMap.find(fs) != m_fsMap.end())
                {
                    MntTabEntry entry;
                    std::string device(p+vmt->vmt_data[VMT_OBJECT].vmt_off, vmt->vmt_data[VMT_OBJECT].vmt_size);
                    std::string mountPoint(p+vmt->vmt_data[VMT_STUB].vmt_off, vmt->vmt_data[VMT_STUB].vmt_size);

                    entry.device = SCXCoreLib::StrFromMultibyte(device);
                    entry.mountPoint = SCXCoreLib::StrFromMultibyte(mountPoint);
                    entry.fileSystem = m_fsMap.find(fs)->second;
                    m_MntTab.push_back(entry);
                }
                p += vmt->vmt_length;
            }
        }
        // TODO: Error handling?
#else
        SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(
                                            LocateMountTab(), std::ios::in));
        fs.SetOwner();
        while ( ! fs->eof() && fs->is_open())
        {
            std::wstring line;
            std::vector<std::wstring> parts;
            getline(*fs, line);
            SCXCoreLib::StrTokenize(line, parts, L" \n\t");
            if (parts.size() > 3)
            {
                if (std::wstring::npos != parts[0].find('#')) // Comment
                {
                    continue;
                }
                MntTabEntry entry;
                entry.device = parts[0];
                entry.mountPoint = parts[1];
                entry.fileSystem = parts[2];
                if (parts[3].find(L"dev=") != std::wstring::npos)
                {
                    entry.devAttribute = parts[3].substr(parts[3].find(L"dev="));
                    if (entry.devAttribute.length() > 0)
                    {
                        entry.devAttribute = entry.devAttribute.substr(4); // Removing "dev="
                        entry.devAttribute = entry.devAttribute.substr(0,entry.devAttribute.find_first_not_of(L"0123456789abcdef"));
                    }
                }
                m_MntTab.push_back(entry);
            }
        }
        fs->close();
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::FileSystemIgnored

    */
    bool DiskDependDefault::FileSystemIgnored(const std::wstring& fs)
    {
        // Remember to NEVER change this list without a first failing the test called:
        // IgnoredFilesystemShouldNotBeCaseSensitive
        static std::wstring IGFS[] = {
            L"autofs",
            L"bdev", L"binfmt_misc",
            L"cachefs", L"cdfs", L"cdrfs", L"cifs", L"ctfs",
#if defined(sun) && ((PF_MAJOR == 5 && PF_MINOR >= 11) || (PF_MAJOR > 5))
            // On Solaris 11, /dev is a pseudo file system.
            // Always ignore to eliminate inode detection, etc
            L"dev",
#endif
            L"debugfs", L"devfs", L"devpts", L"devtmpfs",
            L"eventpollfs",
            L"fd", L"ffs", L"fifofs", L"fusectl", L"futexfs",
            L"hsfs", L"hugetlbfs",
            L"inotifyfs", L"iso9660",
            L"lofs",
            L"mntfs", L"mqueue",
            L"namefs",
            // WI 24875: Ignore file system type "none" (these are NFS-mounted on the local system)
            L"none",
            L"objfs",
            L"pipefs", L"proc", L"procfs",
            L"ramfs", L"rootfs", L"rpc_pipefs",
            L"securityfs", L"sharefs", L"sockfs", L"specfs", L"subfs", L"sysfs",
            L"tmpfs",
            L"udfs", L"usbfs",
            L"vmblock", L"vmhgfs", L"vmware-hgfs",
#if ! defined(sun)
            L"zfs",
#endif
            L"" };

        // File systems that are identified if they begin with anything in this list.
        static std::wstring IGFS_START[] = {
            L"nfs",
            L"" };

        // File systems that are identified if any part matches anything in this list.
        static std::wstring IGFS_PARTS[] = {
            L"gvfs",
            L"" };

        std::wstring fs_in_lower_case = SCXCoreLib::StrToLower(fs);
        return IsStringInArray(fs_in_lower_case, IGFS_PARTS, CompareContains)
            || IsStringInArray(fs_in_lower_case, IGFS_START, CompareStartsWith)
            || IsStringInArray(fs_in_lower_case, IGFS, CompareEqual);
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::DeviceIgnored
    */
    bool DiskDependDefault::DeviceIgnored(const std::wstring& device)
    {
#if defined (sun)
        // Bug #15583: UFS CD/DVD-ROMs on Solaris systems are causing
        // false alarms about the disk being full. Prior to this fix,
        // logic for determining whether or not to report a disk was
        // based solely on the file system type. This does not hold
        // because ufs is the default file system type for Solaris.
        // The location of the mount point must also be examined.
        // For Solaris, CD-ROMs are automatically mounted in
        // '/vol/dev/dsk/'
        std::wstring volPath(L"/vol/dev/dsk/");
        return SCXCoreLib::StrIsPrefix(device, volPath);
#else
        // warnings as errors, so deal with the unused param
        (void) device;

        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if a given file system is represented by 'known' physical device.
        in mnttab file. Currently, we do not know how to get list of
        physical device(s) for ZFS filesystem

        \param       fs Name of file system.
        \returns     true if the file system has no 'real' device in mnt-tab.

    */
    bool DiskDependDefault::FileSystemNoLinkToPhysical(const std::wstring& fs)
    {
        static std::wstring IGFS[] = {
#if defined(sun)
            L"zfs",
#endif
            L"" };

        return IsStringInArray(SCXCoreLib::StrToLower(fs), IGFS, CompareEqual);
    }


    /*----------------------------------------------------------------------------*/
    /*
      \copydoc SCXSystemLib::DiskDepend::LinkToPhysicalExists
    */
    bool DiskDependDefault::LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);

        return LinkToPhysicalExists(fs, dev_path, mountpoint, suppressor);
    }

    /*----------------------------------------------------------------------------*/
    /**
      Used for injecting a log suppressor, otherwise same as SCXSystemLib::DiskDepend::LinkToPhysicalExists().

      \param       fs Name of file system.
      \param       dev_path Device path fount in mount table.
      \param       mountpoint Mount point found in mount table.
      \param[in] suppressor log suppressor to inject.

      \returns     true if the file system has 'real' device in mnt-tab.

    */
    bool DiskDependDefault::LinkToPhysicalExists(const std::wstring& fs,
                                                 const std::wstring& dev_path,
                                                 const std::wstring& mountpoint,
                                                 SCXCoreLib::LogSuppressor& suppressor)
    {
        if (dev_path == mountpoint || FileSystemNoLinkToPhysical(fs))
        {
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(dev_path));
            SCX_LOG(m_log, severity,
                    L"No link exists between the logical device \"" + dev_path +
                    L"\" at mount point \"" + mountpoint +
                    L"\" with filesystem \"" + fs +
                    L"\". Some statistics will be unavailable.");

            return false;
        }
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      \copydoc SCXSystemLib::DiskDepend::DeviceToInterfaceType
    */
#if defined(linux)
    DiskInterfaceType DiskDependDefault::DeviceToInterfaceType(const std::wstring& dev) const
    {
        SCXCoreLib::SCXFilePath path(dev);
        switch (path.GetFilename()[0])
        {
        case 'h':
            return eDiskIfcIDE;
        case 's':
            return eDiskIfcSCSI;
        default:
            return eDiskIfcUnknown;
        }
    }
#else
    DiskInterfaceType DiskDependDefault::DeviceToInterfaceType(const std::wstring& ) const
    {
        return eDiskIfcUnknown;
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetPhysicalDevices
    */
    std::map<std::wstring, std::wstring> DiskDependDefault::GetPhysicalDevices(const std::wstring& device)
    {
        std::map<std::wstring, std::wstring> devices;

        SCXCoreLib::SCXFilePath path(device);
        std::wstring name = path.GetFilename();
#if defined(aix)
        // Since we do not have the associaction between logical and physical disks - return them all
        perfstat_id_t id;
        perfstat_disk_t data;
        strcpy(id.name, FIRST_DISKPATH);
        path.SetDirectory(L"/dev");
        do {
            int r = perfstat_disk(&id, &data, sizeof(data), 1);
            if (1 == r && 0 != strncmp(data.name, "cd", 2)) // TODO: better way to exclude CD/DVD!
            {
                name = SCXCoreLib::StrFromMultibyte(data.name);
                path.SetFilename(name);
                devices[name] = path.Get();
            }
            // TODO: Error handling?
        } while (0 != strcmp(id.name, FIRST_DISKPATH));
#elif defined(hpux)
        size_t vg_idx;
        for (vg_idx = 0; vg_idx < GetLVMTab().GetVGCount(); ++vg_idx)
        {
            // Stored name is without trailing slash.
            if (SCXCoreLib::StrAppend(GetLVMTab().GetVG(vg_idx),L"/") == path.GetDirectory())
            {
                for (size_t pidx = 0; pidx < GetLVMTab().GetPartCount(vg_idx); ++pidx)
                {
                    path.Set(GetLVMTab().GetPart(vg_idx, pidx));
                    name = path.GetFilename();
                    if (0 == SCXCoreLib::StrCompare(name.substr(0,4),L"disk",true))
                    { // New style: disk1_p2 (or just disk3)
                        name = name.substr(0, name.find_last_of(L"_"));
                    }
                    else
                    { // "sun" style: c1t2d3s4
                        if (name.substr(name.find_last_not_of(L"0123456789"),1) == L"s")
                        { // Remove partition identifier
                            name = name.substr(0,name.find_last_not_of(L"0123456789"));
                        }
                    }
                    path.SetFilename(name);
                    // Bug 6755 & 6883: partial discoveries of disks.
                    // Using algorithm from static PAL for exclusion.
                    std::wstring rawDevice = path.Get();
                    if (std::wstring::npos != rawDevice.find(L"/dsk/"))
                    {
                        rawDevice = SCXCoreLib::StrAppend(L"/dev/rdsk/", path.GetFilename());
                    }
                    else
                    {
                        rawDevice = SCXCoreLib::StrAppend(L"/dev/rdisk/", path.GetFilename());
                    }
                    int fd = this->open(SCXCoreLib::StrToMultibyte(rawDevice).c_str(), O_RDONLY);
                    if (-1 != fd)
                    {
                        devices[name] = path.Get();
                        this->close(fd);
                    }
                }
                break;
            }
        }
#elif defined(linux)
        // Given a device path to a partition (for example /dev/hda5), convert
        // it to a path to the base device (for example /dev/hda).

        try
        {
            static SCXLVMUtils lvmUtils;

            // Try to convert the potential LVM device path into its matching
            // device mapper (dm) device path.
            std::wstring dmDevice = lvmUtils.GetDMDevice(device);

            if (dmDevice.empty())
            {
                // device is a normal partition device path
                path = GuessPhysicalFromLogicalDevice(device);
                name = path.GetFilename();
                devices[name] = path.Get();
            }
            else
            {
                // device was an LVM device path and dmDevice is the path to
                // the same device using the device mapper name.  The dm device
                // is located on one or more normal devices, known as slaves.
                std::vector< std::wstring > slaves = lvmUtils.GetDMSlaves(dmDevice);
                if (slaves.size() == 0)
                {
                    // this condition can only be reached on RHEL4/SLES9 systems
                    static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eInfo, SCXCoreLib::eHysterical);
                    std::wstringstream               out;

                    out << L"Because of limited support for LVM on "
#   if defined(PF_DISTRO_SUSE)
                        << L"SuSE Linux Enterprise Server 9"
#   else
                        << L"Red Hat Enterprise Linux 4"
#   endif
                        << L", the logical device " << device << L": cannot be mapped to the physical device(s) that contain it.";
                    SCX_LOG(m_log, suppressor.GetSeverity(device), out.str());
                }
                else
                {
                    for (std::vector< std::wstring >::const_iterator iter = slaves.begin();
                         iter != slaves.end(); iter++)
                    {
                        path = GuessPhysicalFromLogicalDevice(*iter);
                        name = path.GetFilename();
                        devices[name] = path.Get();
                    }
                }
            }
        }
        catch (SCXCoreLib::SCXException& e)
        {
            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
            std::wstringstream               out;

            out << L"An exception occurred resolving the physical devices that contain the LVM device " << device << L": " << e.What();
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }
#elif defined(sun)
        std::vector<std::wstring> devs;
        if (device.find(L"/md/") != std::wstring::npos)
        { // A meta device
            if (0 == m_pRaid)
            {
                SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new SCXSystemLib::SCXRaidCfgParserDefault() );
                m_pRaid = new SCXRaid(deps);
            }

            m_pRaid->GetDevices(name, devs);
            path.SetDirectory(L"/dev/dsk/"); // Need to rewrite the path for Kstat guess below.
        }
        else
        {
            devs.push_back(name);
        }
        for (std::vector<std::wstring>::const_iterator it = devs.begin();
            it != devs.end(); ++it)
        {
            name = it->substr(0,it->find_last_not_of(L"0123456789"));
            std::wstring dev = SCXCoreLib::StrAppend(L"/dev/dsk/", *it);

            try
            {
                std::wstring m,n;
                scxlong i;
                if (GuessKstatPath(path.GetDirectory() + L"/" + name, m, n, i, true))
                {
                    devices[name] = dev.substr(0,dev.find_last_not_of(L"0123456789"));
                }
            }
            catch (SCXCoreLib::SCXException& )
            {

            }
        }
#endif
        return devices;
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       Convenience method if you do not have a kstat object available. Then it uses
       a static one protected by global lock.

       \param[in]   dev_path Path to device ex: /dev/dsk/c0t0d0s0
       \param[out]  module Output parameter with guessed kstat module.
       \param[out]  name Output parameter with guessed kstat name.
       \param[out]  instance Output parameter with guessed kstat instance number.
       \param[in]   isPhysical Type of disk for given dev_path - true if you want
                    physical disk, otherwise false.
       \returns     true if a Kstat path could be guessed, otherwise false.
       \throws      SCXErrnoException when system calls fail.
       \throws      SCXInternalErrorException For errors that should not occur normally.
       \throws      SCXNotImplementedException On plattforms not supported by method.
    */
    bool DiskDependDefault::GuessKstatPath(const std::wstring& dev_path, std::wstring& module, std::wstring& name, scxlong& instance, bool isPhysical)
    {
        static SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat(new SCXSystemLib::SCXKstat());
        static SCXCoreLib::SCXThreadLockHandle handle = SCXCoreLib::ThreadLockHandleGet(L"Guess Kstat Global");

        SCXCoreLib::SCXThreadLock lock(handle);

        return GuessKstatPath(kstat, dev_path, module, name, instance, isPhysical);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Given a device path, guess the Kstat path for that device.

       \param[in]   kstat A kstat object to use in this method.
       \param[in]   dev_path Path to device ex: /dev/dsk/c0t0d0s0
       \param[out]  module Output parameter with guessed kstat module.
       \param[out]  name Output parameter with guessed kstat name.
       \param[out]  instance Output parameter with guessed kstat instance number.
       \param[in]   isPhysical Type of disk for given dev_path - true if you want
                    physical disk, otherwise false.
       \returns     true if a Kstat path could be guessed, otherwise false.
       \throws      SCXErrnoException when system calls fail.
       \throws      SCXInternalErrorException For errors that should not occur normally.
       \throws      SCXNotImplementedException On plattforms not supported by method.

       \note On solaris do:
       \verbatim
       readlink(dev_path, link_path)
       find link_path (except "/devices") in "/etc/path_to_inst" and conclude kstat path.
       For logical disks use filename of link_path = <s1>@<i1>,<i2>:<s2>
       example: sd@0,0:a
       Logical disk name = <module><instance>,<s2>
       \endverbatim
       If the deduced kstat module, name and instance does not match a kstat entry,
       the method will assume module=unix and name=vopstats_XXX where XXX is a device
       number from attributes in /etc/mnttab.
    */
    bool DiskDependDefault::GuessKstatPath(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path, std::wstring& module, std::wstring& name, scxlong& instance, bool isPhysical)
    {
        char buf[1024];
        std::wstring dpath = dev_path;
        if (isPhysical &&
            dpath.substr(dpath.rfind(L"/")+1,1) == L"c") //real disks start with a "c"
        {
            dpath += L"s0"; //Assume at least one partition.
        }
        memset(buf, 0, sizeof(buf));
        if (-1 == readlink(SCXCoreLib::StrToMultibyte(dpath).c_str(), buf, sizeof(buf)))
        {
            throw SCXCoreLib::SCXErrnoException(L"readlink", errno, SCXSRCLOCATION);
        }

        SCXCoreLib::SCXFilePath link_path(SCXCoreLib::StrFromMultibyte(buf));
        if (link_path.GetDirectory().find(L"pseudo") != std::wstring::npos)
        {
            if (isPhysical)
            {
                return false;
            }
        }
        else
        {
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(link_path.GetFilename(), parts, L":");

            if (parts.size() != 2)
            {
                throw SCXCoreLib::SCXInternalErrorException(L"Tokenized wrong number of parts", SCXSRCLOCATION);
            }

            link_path.SetFilename(parts[0]);
            // Remove any ".." or "devices" in link path begining
            while (link_path.GetDirectory().substr(0,1) == L"/")
            {
                link_path.SetDirectory(link_path.GetDirectory().substr(1));
            }
            while (link_path.GetDirectory().substr(0,3) == L"../")
            {
                link_path.SetDirectory(link_path.GetDirectory().substr(3));
            }
            while (link_path.GetDirectory().substr(0,7) == L"devices")
            {
                link_path.SetDirectory(link_path.GetDirectory().substr(7));
            }

            SCXCoreLib::SCXHandle<DeviceInstance> di = m_deviceMap.find(link_path.Get())->second;
            if (0 == di)
            {
                return false;
            }

            module = di->m_name;
            instance = di->m_instance;
            name = module + SCXCoreLib::StrFrom(instance);
            if ( ! isPhysical)
            {
                try
                {
                    name += L"," + parts[1];
                    kstat->Update();
                    kstat->Lookup(module, name, static_cast<int>(instance));
                    return true;
                }
                catch(SCXKstatNotFoundException& )
                {
                }
            }
        }

        if ( ! isPhysical)
        {
            // Some systems have partition stats in vopstats instances. If on can be
            // found- use it.
            for (std::vector<MntTabEntry>::const_iterator mtab_it = GetMNTTab().begin();
                mtab_it != GetMNTTab().end(); mtab_it++)
            {
                if (mtab_it->device == dpath)
                {
                    module = L"unix";
                    name = SCXCoreLib::StrAppend(L"vopstats_", mtab_it->devAttribute);
                    instance = 0;
                    try
                    {
                        kstat->Update();
                        kstat->Lookup(module, name, static_cast<int>(instance));
                    }
                    catch(SCXKstatNotFoundException& )
                    {
                        return false;
                    }
                    return true;
                }
            }
            return false;
        }
        return true;
    }

#endif
    /*----------------------------------------------------------------------------*/
    /**
       Removes all numbers or one other character from end of given string.

       \param[in]   str Input string.
       \returns     The input string where the last character is removed or if the
                    string ends with numbers all ending numbers are removed.

    */
    std::wstring DiskDependDefault::RemoveTailNumberOrOther(const std::wstring& str)
    {
        if (str.length() == 0)
        {
            return str;
        }
        std::wstring result = str;
        if (str.find_last_of(L"0123456789") == (str.length()-1))
        {
            while (result.length() > 0 && result.find_last_of(L"0123456789") == (result.length()-1))
            {
                result.resize(result.length()-1);
            }
        }
        else
        {
            result.resize(result.length()-1);
        }
        return result;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Given a logical disk device path, returns a device path to physical device
       on which tyhe logical device should be residing.

       \param[in]   logical_dev Path to logical disk device.
       \returns     physical_dev Path to physical disk device.

       \note This algorithm is based on the fact that physical disks resides in the same
       folder as logical disks. It also assumes that logical disks (partitions/slices) have
       names that are [physical device] followed by an optional letter and one or more digits.
       Whis is what we've observed so far.
    */
    std::wstring DiskDependDefault::GuessPhysicalFromLogicalDevice(const std::wstring& logical_dev)
    {
        std::wstring physical_dev = logical_dev;
        SCXCoreLib::SCXFilePath path(physical_dev);
        while (path.GetFilename().length() > 0)
        {
            physical_dev = RemoveTailNumberOrOther(physical_dev);
            if (this->FileExists(physical_dev))
            {
                return physical_dev;
            }
            path = physical_dev;
        }
        return logical_dev;
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::ReadKstat
    */
    bool DiskDependDefault::ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, std::wstring& kstat_name, const std::wstring& dev_path, bool isPhysical)
    {
        std::wstring module;
        scxlong instance;

        if (GuessKstatPath(kstat, dev_path, module, kstat_name, instance, isPhysical))
        {
            kstat->Update();
            kstat->Lookup(module, kstat_name, static_cast<int>(instance));
            return true;
        }

        return false;
    }
#endif /* sun */

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::AddDeviceInstance
    */
    void DiskDependDefault::AddDeviceInstance(const std::wstring& device, const std::wstring& name, scxlong instance, scxlong devID)
    {
        SCXCoreLib::SCXHandle<DeviceInstance> di( new DeviceInstance() );
        di->m_name = name;
        di->m_devID = devID;
        di->m_instance = instance;
        m_deviceMap[device] = di;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::FindDeviceInstance
    */
    SCXCoreLib::SCXHandle<DeviceInstance> DiskDependDefault::FindDeviceInstance(const std::wstring& device) const
    {
        if (m_deviceMap.find(device) != m_deviceMap.end())
        {
            return m_deviceMap.find(device)->second;
        }
        return SCXCoreLib::SCXHandle<DeviceInstance>(0);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Returns true if the string is found in the array.
       \param str String to look for.
       \param arr Array of strings to search. An empty string marks end of array.
       \param compare A compare function to be used when determine if string is in array.
       \returns true if the string is found in the array or otherwise false.
       \throws SCXInvalidArgumentException if arr or compare is NULL.
    */
    bool DiskDependDefault::IsStringInArray(const std::wstring& str,
                                            const std::wstring* arr,
                                            CompareFunction compare)
    {
        if (NULL == arr)
    {
            throw SCXCoreLib::SCXInvalidArgumentException(L"arr",L"Should never be NULL", SCXSRCLOCATION);
        }
        if (NULL == compare)
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"compare",L"Should never be NULL", SCXSRCLOCATION);
        }
        for (int i = 0; arr[i].length() != 0; i++)
        {
            if  (compare(str, arr[i]))
            {
                return true;
            }
        }

        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Check if needle equals heystack.
       \param needle a string
       \param heystack a string
       \returns true if needle equals heystack. Otherwise false.
       Example:
    */
    bool DiskDependDefault::CompareEqual(const std::wstring& needle, const std::wstring& heystack)
            {
        return needle == heystack;
            }

    /*----------------------------------------------------------------------------*/
    /**
       Check if needle starts with the heystack.
       \param needle a string
       \param heystack a string
       \returns true if needle starts with heystack. Otherwise false.
    */
    bool DiskDependDefault::CompareStartsWith(const std::wstring& needle, const std::wstring& heystack)
    {
        return needle.substr(0, heystack.length()) == heystack;
        }

    /*----------------------------------------------------------------------------*/
    /**
       Check if needle contains heystack.
       \param needle a string
       \param heystack a string
       \returns true if needle contains heystack. Otherwise false.
    */
    bool DiskDependDefault::CompareContains(const std::wstring& needle, const std::wstring& heystack)
    {
        return needle.find(heystack) != std::wstring::npos;
    }

    /*----------------------------------------------------------------------------*/
    /**
           \copydoc SCXSystemLib::DiskDepend::FileExists
    */
    bool DiskDependDefault::FileExists(const std::wstring& path)
    {
        SCXCoreLib::SCXFileInfo fi(path);
        return fi.PathExists();
    }
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
