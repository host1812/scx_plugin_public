/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Defines the static disk information enumeration PAL for logical disks.
    
    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/
#ifndef STATICLOGICALDISKENUMERATION_H
#define STATICLOGICALDISKENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/staticlogicaldiskinstance.h>
#include <scxsystemlib/diskdepend.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Represents a set of discovered logical disks and their static data.
*/
    class StaticLogicalDiskEnumeration : public EntityEnumeration<StaticLogicalDiskInstance>
    {
    public:
        StaticLogicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps);
        virtual ~StaticLogicalDiskEnumeration();

        virtual void Init();
        virtual void Update(bool updateInstances=true);
        virtual void CleanUp();

    private:
        StaticLogicalDiskEnumeration();         //!< Private constructor (intentionally not implemented)

        SCXCoreLib::SCXHandle<DiskDepend> m_deps; //!< Disk dependency object for dependency injection
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle
    };

} /* namespace SCXSystemLib */
#endif /* STATICLOGICALDISKENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
