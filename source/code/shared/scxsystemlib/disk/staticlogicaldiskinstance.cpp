/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Implements the logical disk instance pal for static information.
    
    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/staticlogicaldiskinstance.h>

#include <errno.h>


namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Constructor.
    
    \param       deps A StaticDiscDepend object which can be used.
*/
    StaticLogicalDiskInstance::StaticLogicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps)
        : m_deps(0), m_online(false), m_sizeInBytes(0), m_isReadOnly(false), m_persistenceType(0), m_availableSpace(0),
          m_isNumFilesSupported(false), m_numTotalInodes(0), m_numAvailableInodes(0),
          m_isCaseSensitive(false), m_isCasePreserved(false), m_codeSet(0),
          m_maxFilenameLen(0), m_blockSize(0)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.staticlogicaldiskinstance");
        m_deps = deps;
    }

/*----------------------------------------------------------------------------*/
/**
    Virtual destructor.
*/
    StaticLogicalDiskInstance::~StaticLogicalDiskInstance()
    {

    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the disk health state.

    \param       healthy - output parameter where the health status of the disk is stored.
    \returns     true if value was set, otherwise false.
    */
    bool StaticLogicalDiskInstance::GetHealthState(bool& healthy) const
    {
        healthy = m_online;
        return true;
    }
/*----------------------------------------------------------------------------*/
/**
    Retrieve the device name (i.e. '/').

    \param       value - output parameter where the name of the device is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetDeviceName(std::wstring& value) const
    {
        value = GetId();
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the device ID (i.e. '/dev/sda2' on Linux).

    \param       value - output parameter where the ID of the device is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetDeviceID(std::wstring& value) const
    {
        value = m_device;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the file system type

    \param       value - output parameter where the file system type
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetFileSystemType(std::wstring& value) const
    {
        value = m_fileSystemType;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the mountpoint (i.e. '/').

    \param       value - output parameter where the mountpoint is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetMountpoint(std::wstring& value) const
    {
        value = m_mountPoint;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the size of the file system in bytes

    \param       value - output parameter where the size is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetSizeInBytes(scxulong& value) const
    {
        value = m_sizeInBytes;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the compression method.

    Valid compression methods are: "Unknown", "Compressed", or "Uncompressed".

    \param       value - output parameter where the compression method is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetCompressionMethod(std::wstring& value) const
    {
        value = m_compressionMethod;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the Read-Only state of the device.

    \param       value - output parameter where the read-only state is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetIsReadOnly(bool& value) const
    {
        value = m_isReadOnly;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the encryption method.

    Valid encryption methods are: "Unknown", "Encrypted", or "Not Encrypted"

    \param       value - output parameter where the encryption method is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetEncryptionMethod(std::wstring& value) const
    {
        value = m_encryptionMethod;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the Persistence Type.

    Valid persistence types are: 0, 1, 2, 3, or 4
    to refer to: "Unknown", "Other", "Persistent", "Temporary", or "External"

    \param       value - output parameter where the persistence type is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetPersistenceType(int& value) const
    {
        value = m_persistenceType;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the space available on the volume in bytes.

    \param       value - output parameter where the available space is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetAvailableSpaceInBytes(scxulong& value) const
    {
        value = m_availableSpace;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the block size in bytes.

    \param       value - output parameter where the block size is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetBlockSize(scxulong& value) const
    {
        value = m_blockSize;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the number of inodes allocated to the file system.

    Note that some implementations of file systems return this while others
    do not.  If a file system does not support this, zero is returned and the
    method will return false.

    \param       value - output parameter where the number of inodes is stored.
    \returns     true if value is supported on file system, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetTotalInodes(scxulong& value) const
    {
        value = m_numTotalInodes;
        return (m_isNumFilesSupported);
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the number of inodes available in the file system.

    This call retrieves the number of inodes available in the file system
    (regardless of privilege level).  No "buffer" is left aside for
    privileged users.

    Note that some implementations of file systems return this while others
    do not.  If a file system does not support this, zero is returned and the
    method will return false.

    \param       value - output parameter where the number of files is stored.
    \returns     true if value is supported on file system, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetAvailableInodes(scxulong& value) const
    {
        value = m_numAvailableInodes;
        return (m_isNumFilesSupported);
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the case sensitivity of the file system (true/false)

    \param       value - output parameter where the case sensitivity is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetIsCaseSensitive(bool& value) const
    {
        value = m_isCaseSensitive;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the case preservation of the file system (true/false)

    \param       value - output parameter where the case preservation is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetIsCasePreserved(bool& value) const
    {
        value = m_isCasePreserved;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the code set of the file system.

    Valid code sets are: 0, 1, 2, 3, 4, 5, 6, 7, or 8
    to refer to: "Unknown", "Other", "ASCII", "Unicode", "ISO2022", "ISO8859",
                 "Extended UNIX Code", "UTF-8", or "UCS-2" respectively.

    \param       value - output parameter where the code set is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetCodeSet(int& value) const
    {
        value = m_codeSet;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the maximum filename length of the file system.

    \param       value - output parameter where the maximum filename length is stored.
    \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticLogicalDiskInstance::GetMaxFilenameLen(scxulong& value) const
    {
        value = m_maxFilenameLen;
        return true;
    }


/*----------------------------------------------------------------------------*/
/**
    Dump the object as a string for logging purposes.
    
    \returns     String representation of object, suitable for logging.
 */
    const std::wstring StaticLogicalDiskInstance::DumpString() const
    {
        return SCXDumpStringBuilder("StaticLogicalDiskInstance")
            .Text("Name", GetId())
            .Text("Device", m_device)
            .Text("MountPoint", m_mountPoint)
            .Text("FileSystemType", m_fileSystemType)
            .Scalar("SizeInBytes", m_sizeInBytes)
            .Text("CompressionMethod", m_compressionMethod)
            .Scalar("ReadOnly", m_isReadOnly)
            .Text("EncryptionMethod", m_encryptionMethod)
            .Scalar("PersistenceType", m_persistenceType)
            .Scalar("AvailableSpace", m_availableSpace)
            .Scalar("isNumFilesSupported", m_isNumFilesSupported)
            .Scalar("TotalFilesAllowed", m_numTotalInodes)
            .Scalar("TotalFilesAvailable", m_numAvailableInodes)
            .Scalar("CaseSensitive", m_isCaseSensitive)
            .Scalar("CasePreserved", m_isCasePreserved)
            .Scalar("CodeSet", m_codeSet)
            .Scalar("MaxFilenameLen", m_maxFilenameLen)
            .Scalar("BlockSize", m_blockSize);
    }

/*----------------------------------------------------------------------------*/
/**
    Get default settings for this file system

    On UNIX, almost all file systems are the same with the sorts of things we care about.
    But this isn't necessarily ALWAYS the case.  This routine gives us the opportunity to
    change some of the values easily for specific file systems.
*/
    void StaticLogicalDiskInstance::UpdateDefaults()
    {
        static struct {
            std::wstring fsType;
            std::wstring compression;
            std::wstring encryption;
            int persistenceType;
            bool isCasePreserved;
            bool isCaseSensitive;
            int codeSet;
        } fsProperties[] =
        {
          /*    fsType         Compression          Encryption        PI   CaseP   CaseS   CS  */
            { L"ext2",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"ext3",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"ext4",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
                /* Hi Performance FileSystem on HP-UX (not HPFS or Hierarchical File System) */
            { L"hfs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"jfs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"jfs2",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"reiserfs",   L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"ufs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"vxfs",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"xfs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
            { L"zfs",        L"Unknown",          L"Unknown",         2,   true,   true,   0  },

            /*
             * Taking liberties with the case-sensitive/case-preserved values
             * (Boolean anyway, so there's no concept of "Unknown" here)
             */

            { L"",           L"Unknown",          L"Unknown",         0,   true,   true,   0 }
        };

        /* Look for the file system in list of known file systems */

        for (int i=0; /* None; we break out when done */ ; i++)
        {
            if (0 == SCXCoreLib::StrCompare(m_fileSystemType, fsProperties[i].fsType, true)
                || 0 == fsProperties[i].fsType.size())
            {
                m_compressionMethod = fsProperties[i].compression;
                m_encryptionMethod = fsProperties[i].encryption;
                m_persistenceType = fsProperties[i].persistenceType;
                m_isCasePreserved = fsProperties[i].isCasePreserved;
                m_isCaseSensitive = fsProperties[i].isCaseSensitive;
                m_codeSet = fsProperties[i].codeSet;
                break;
            }
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Update the instance.
*/
    void StaticLogicalDiskInstance::Update()
    {
        UpdateDefaults();

        /* Do a statvfs() call to get file system statistics */

        SCXStatVfs fsstat;
        if (0 != m_deps->statvfs(SCXCoreLib::StrToMultibyte(GetId()).c_str(), &fsstat))
        {
            // Ignore EOVERFLOW (if disk is too big) to keep disk 'on-line' even without statistics
            if ( EOVERFLOW == errno ){
                m_online = true;
                SCX_LOGHYSTERICAL(m_log, SCXCoreLib::StrAppend(L"statvfs() failed with EOVERFLOW for ", GetId()));
            } 
            else 
            {
                SCX_LOGERROR(m_log, 
                    SCXCoreLib::StrAppend(L"statvfs() failed for " + GetId() + L"; errno = ", errno ) );
                m_online = false;
            }

            return;
        }

        m_online = true;
        m_sizeInBytes = static_cast<scxulong> (fsstat.f_blocks) * fsstat.f_frsize;
        m_isReadOnly = (fsstat.f_flag & ST_RDONLY);
        m_availableSpace = static_cast<scxulong> (fsstat.f_bfree) * fsstat.f_frsize;
        if (fsstat.f_files) {
            m_isNumFilesSupported = true;
            m_numTotalInodes = fsstat.f_files;
            m_numAvailableInodes = fsstat.f_ffree;
        }
        m_maxFilenameLen = fsstat.f_namemax;
        m_blockSize =  fsstat.f_bsize;
    }
    
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
