/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Implements the logical disk enumeration pal for static information.

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/staticlogicaldiskenumeration.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Constructor.

    \param       deps A StaticDiscDepend object which can be used.

*/
    StaticLogicalDiskEnumeration::StaticLogicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) : m_deps(0)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.staticlogicaldiskenumeration");
        m_deps = deps;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Virtual destructor.
    */
    StaticLogicalDiskEnumeration::~StaticLogicalDiskEnumeration()
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Init method.

       Initial caching of data is performed here.

    */
    void StaticLogicalDiskEnumeration::Init()
    {
        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Cleanup method.

       Release of cached resources.

    */
    void StaticLogicalDiskEnumeration::CleanUp()
    {
        EntityEnumeration<StaticLogicalDiskInstance>::CleanUp();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Update the enumeration.

       \param updateInstances If true (default) all instances will be updated.
                              Otherwise only the content of teh enumeration will be updated.

    */
    void StaticLogicalDiskEnumeration::Update(bool updateInstances/*=true*/)
    {
        for (EntityIterator iter=Begin(); iter!=End(); iter++)
        {
            SCXCoreLib::SCXHandle<StaticLogicalDiskInstance> disk = *iter;
            disk->m_online = false;
        }

        m_deps->RefreshMNTTab();
        for (std::vector<MntTabEntry>::const_iterator it = m_deps->GetMNTTab().begin();
             it != m_deps->GetMNTTab().end(); it++)
        {
            if ( ! m_deps->FileSystemIgnored(it->fileSystem) && ! m_deps->DeviceIgnored(it->device))
            {
                SCXCoreLib::SCXHandle<StaticLogicalDiskInstance> disk = GetInstance(it->mountPoint);
                if (0 == disk)
                {
                    disk = new StaticLogicalDiskInstance(m_deps);
                    disk->m_device = it->device;
                    disk->m_mountPoint = it->mountPoint;
                    disk->SetId(disk->m_mountPoint);
                    disk->m_fileSystemType = it->fileSystem;
                    AddInstance(disk);
                }
                disk->m_online = true;
            }
        }

        if (updateInstances)
        {
            UpdateInstances();
        }
    }
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
