/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file        

    \brief         Implements the thread lock factory PAL.

    \date          2007-05-28 10:46:00
  
*/
/*----------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/stringaid.h>

namespace SCXCoreLib
{
    SCXThreadLockFactory SCXThreadLockFactory::s_instance;

/*----------------------------------------------------------------------------*/
/**
    Convenience function to access the thread lock factory and get an anonymous
    lock handle.
    
    Parameters:  None
    Retval:      An anonymous SCXThreadLockHandle.
        
*/
    SCXThreadLockHandle ThreadLockHandleGet(void)
    {
        return SCXThreadLockFactory::GetInstance().GetLock();
    }

/*----------------------------------------------------------------------------*/
/**
    Convenience function to access the thread lock factory and get a named
    lock handle.
    
    Parameters:  nameOfLock - name of lock handle to get.
    Retval:      A SCXThreadLockHandle associated with the given name.
        
    Note that calling this method with an empty string generates the same result
    as calling ThreadLockFactory(void).
    
*/
    SCXThreadLockHandle ThreadLockHandleGet(const std::wstring& nameOfLock)
    {
        return SCXThreadLockFactory::GetInstance().GetLock(nameOfLock);
    }

/*----------------------------------------------------------------------------*/
/**
    Default constructor.
    
    Parameters:  None
    Retval:      N/A
        
    Creates a single anonymous lock handle to be used as thread lock to make the
    factory class thread safe.
    
*/
    SCXThreadLockFactory::SCXThreadLockFactory(void):
        m_lockHandle(L"")
    {
        Reset();
    }

/*----------------------------------------------------------------------------*/
/**
    Virtual destructor.
    
    Parameters:  None
    Retval:      N/A
        
    Releases all allocated resources.
    
*/
    SCXThreadLockFactory::~SCXThreadLockFactory(void)
    {
        Reset();
    }

/*----------------------------------------------------------------------------*/
/**
    Dump object as string (for logging).
    
    Parameters:  None
    Retval:      N/A
        
*/
    const std::wstring SCXThreadLockFactory::DumpString() const
    {
        return L"SCXThreadLockFactory locks=" + SCXCoreLib::StrFrom(static_cast<scxlong>(m_locks.size()));
    }

/*----------------------------------------------------------------------------*/
/**
    Static method to retrieve singleton instance.
    
    Parameters:  None
    Retval:      Singleton instance of factory class.
        
*/
    SCXThreadLockFactory& SCXThreadLockFactory::GetInstance(void)
    {
        return s_instance;
    }

/*----------------------------------------------------------------------------*/
/**
    Create an anonymous thread lock handle.
    
    Parameters:  None
    Retval:      An anonymous SCXThreadLockHandle object.
        
*/
    SCXThreadLockHandle SCXThreadLockFactory::GetLock(void)
    {
        SCXThreadLockHandle l(L"");
        return l;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve a named thread lock handle.
    
    Parameters:  nameOfLock - Name of lock to retrieve.
    Retval:      A SCXThreadLockHandle associated with the given name.
        
    Using this method with an empty lock name will create an anonymous lock handles.
    If name is non-empty the internal list is searched for the given name. The
    found lock handle is returned if one is found. Otherwise a new lock handle
    is created and added to the list.
    
*/
    SCXThreadLockHandle SCXThreadLockFactory::GetLock(const std::wstring& nameOfLock)
    {
        if (nameOfLock.empty())
            return GetLock();

        SCXThreadLock lock(m_lockHandle);

        const std::map<std::wstring,SCXThreadLockHandle>::iterator item = m_locks.find(nameOfLock);

        if (item != m_locks.end())
        {
            return (item->second);
        }
        
        SCXThreadLockHandle l(nameOfLock);
        m_locks[nameOfLock] = l;
        return l;
    }

/*----------------------------------------------------------------------------*/
/**
    Reset the factory.
    
    Parameters:  None
    Retval:      None
        
    Will remove all references to any previously created locks. In practice this
    method is probably only usable to ease memory leak detection.
    
*/
    void SCXThreadLockFactory::Reset(void)
    {
        SCXThreadLock lock(m_lockHandle);

        m_locks.clear();
    }

/*----------------------------------------------------------------------------*/
/**
    Get the number of used locks.
    
    Parameters:  None
    Retval:      Number of loch handles in used.
        
    Will traverse the lock handle list in order to count number of used locks.
    A lock is considered to be in use if its reference counter is not one since
    one reference is that in the factory lock handle list.
    
*/
    unsigned int SCXThreadLockFactory::GetLocksUsed(void) const
    {
        SCXThreadLock lock(m_lockHandle);

        unsigned int r = 0;
        std::map<std::wstring,SCXThreadLockHandle>::const_iterator cur = m_locks.begin();
        std::map<std::wstring,SCXThreadLockHandle>::const_iterator end = m_locks.end();

        for ( ; cur != end; ++cur)
        {
            if (cur->second.GetRefCount() > 1)
            { /* Ref count should be one for unused locks since they are in the list (=one reference) */
                ++r;
            }
        }
        return r;
    }

} /* namespace SCXCoreLib */
