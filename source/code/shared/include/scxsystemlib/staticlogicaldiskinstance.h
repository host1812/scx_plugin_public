/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Defines the static disk information instance PAL for logical disks.
    
    \date        2008-03-19 11:42:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef STATICLOGICALDISKINSTANCE_H
#define STATICLOGICALDISKINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/diskdepend.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Represents a single disk instance with static data.
*/
    class StaticLogicalDiskInstance : public EntityInstance
    {
        friend class StaticLogicalDiskEnumeration;

    public:
        StaticLogicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps);
        virtual ~StaticLogicalDiskInstance();

        bool GetHealthState(bool& healthy) const;
        bool GetDeviceName(std::wstring& value) const;
        bool GetDeviceID(std::wstring& value) const;
        bool GetMountpoint(std::wstring& value) const;
        bool GetSizeInBytes(scxulong& value) const;
        bool GetCompressionMethod(std::wstring& value) const;
        bool GetIsReadOnly(bool& value) const;
        bool GetEncryptionMethod(std::wstring& value) const;
        bool GetPersistenceType(int& value) const;
        bool GetAvailableSpaceInBytes(scxulong& value) const;
        bool GetTotalInodes(scxulong& value) const;
        bool GetAvailableInodes(scxulong& value) const;
        bool GetIsCaseSensitive(bool& value) const;
        bool GetIsCasePreserved(bool& value) const;
        bool GetCodeSet(int& value) const;
        bool GetMaxFilenameLen(scxulong& value) const;
        bool GetFileSystemType(std::wstring& value) const;
        bool GetBlockSize(scxulong& value) const;

        virtual const std::wstring DumpString() const;
        virtual void Update();

    private:
        //! Private constructor (this should never be called!)
        StaticLogicalDiskInstance();            //!< Default constructor (intentionally not implemented)

        virtual void UpdateDefaults();

        SCXCoreLib::SCXHandle<DiskDepend> m_deps;//!< StaticDiskDepend object
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle
        bool m_online;                          //!< Tells if disk is still connected
        std::wstring m_device;                  //!< Device ID (i.e. /dev/sda1)
        std::wstring m_mountPoint;              //!< Mount point of device (i.e. "/")
        std::wstring m_fileSystemType;          //!< File system type (internal use only)
        scxulong m_sizeInBytes;                 //!< Total size, in bytes
        std::wstring m_compressionMethod;       //!< Compression method - Unknown/Compressed/Not Compressed
        bool m_isReadOnly;                      //!< Is file system read-only?
        std::wstring m_encryptionMethod;        //!< Encryption method - Unknown/Encrypted/Not Encrypted
        int m_persistenceType;                  //!< Persistence type (0-4 for Unknown/Other/Persistent/Temporary/External
        scxulong m_availableSpace;              //!< Available space on volume
        bool m_isNumFilesSupported;             //!< Does this file system support number of files?
        scxulong m_numTotalInodes;              //!< Total number of inodes allowed on file system
        scxulong m_numAvailableInodes;          //!< Number of inodes available (for privileged users)
        bool m_isCaseSensitive;                 //!< Is file system case sensitive?
        bool m_isCasePreserved;                 //!< Is file system case preserved?
        int m_codeSet;                          //!< Code set (0-8 for Unknown/Other/ASCII/Unicode/ISO2022/ISO8859/Extended UNIX Code/UTF-8/UCS-2
        scxulong m_maxFilenameLen;              //!< Maximum file name length
        scxulong m_blockSize;                   //!< Block size
    };
} /* namespace SCXSystemLib */
#endif /* STATICLOGICALDISKINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
