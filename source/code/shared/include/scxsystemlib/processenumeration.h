/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 

*/
/**
    \file

    \brief          Enumeration of Process Items
    \date           07-10-29 15:24:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef PROCESSENUMERATION_H
#define PROCESSENUMERATION_H

#include <map>

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/processinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxhandle.h>
 
namespace SCXSystemLib
{
    /** Time between each sample in seconds. */
    const int PROCESS_SECONDS_PER_SAMPLE = 60;

    /** Type of live process map. One pid corresponds to one process. */
    typedef std::map<scxpid_t, SCXCoreLib::SCXHandle<ProcessInstance> > ProcMap;

    /*----------------------------------------------------------------------------*/
    /**
        Class that represents a collection of Process:s.
        
        PAL Holding collection of Process:s.
    */
    class ProcessEnumeration : public EntityEnumeration<ProcessInstance>
    {
    public:
        static const wchar_t *moduleIdentifier;         //!< Module identifier

        ProcessEnumeration();
        ~ProcessEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances=true);
        virtual void CleanUp();

        size_t Size() const;

        const SCXCoreLib::SCXThreadLockHandle& GetLockHandle() const;
        void UpdateNoLock(SCXCoreLib::SCXThreadLock& lck, bool updateInstances=true);

        /* This one is public for testing purposes */
        void SampleData();

        SCXCoreLib::SCXHandle<ProcessInstance> Find(scxpid_t pid);
        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > Find(const std::wstring& name);
        static bool SendSignalByName(const std::wstring& name, int sig);
        static bool GetNumberOfProcesses(unsigned int& numberOfProcesses);

    private:
        SCXCoreLib::SCXLogHandle m_log;                         //!< Handle to log file 
        SCXCoreLib::SCXThreadLockHandle m_lock; //!< Handles locking in the process enumeration.

        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> m_dataAquisitionThread; //!< Thread pointer.
        static void DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param);

        /** Map of active processes */
        ProcMap m_procs;

        int m_EnumErrorCount;    //!< Number of consecutive enumeration attempts with errors.
        int m_EnumGoodCount;     //!< Number of consecutive enumeration attempts without errors.
        SCXCoreLib::SCXLogSeverity m_EnumLogLevel;  //!< Log level to use when logging execption during instance update
    };

}

#endif /* PROCESSENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
