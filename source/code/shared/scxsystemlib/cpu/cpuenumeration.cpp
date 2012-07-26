/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file        cpuenumeration.cpp

   \brief       Enumeration of CPU:s

   \date        07-05-21 12:00:00

   \date        08-05-29 12:28:00
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>

#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>

#include <scxsystemlib/cpuenumeration.h>
#include <scxsystemlib/cpuinstance.h>

#include <fstream>
#include <iostream>
#include <set>
#include <vector>
#include <string>

#if defined(linux) || defined(sun) || defined(hpux) || defined(aix)
# include <unistd.h>
# include <errno.h>
#endif

// System-specific includes

#if defined(aix)

#include <scxsystemlib/scxodm.h>

#elif defined(hpux)

#include <sys/pstat.h>
#include <sys/mp.h>

#elif defined(sun)

#include <sstream>

#include <scxsystemlib/scxkstat.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/sysinfo.h>

#endif

#if defined(WIN32)
        /** Stubbing out sysconf */
# define _SC_NPROCESSORS_ONLN 1
#endif

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{

    /**
       Returns a stream for reading from /proc/stat
    */
    SCXHandle<wistream> CPUPALDependencies::OpenStatFile() const
    {
#if defined(WIN32)
        return SCXFile::OpenWFstream(SCXFilePath(L"C:\\stat.txt"), ios::in);
#elif defined(linux)
        return SCXFile::OpenWFstream(SCXFilePath(L"/proc/stat"), ios::in);
#else
        return SCXHandle<wistream>(0);
#endif
    }

    SCXHandle<wistream> CPUPALDependencies::OpenCpuinfoFile() const
    {
#if defined(linux)
        return SCXFile::OpenWFstream(SCXFilePath(L"/proc/cpuinfo"), ios::in);
#else
        return SCXHandle<wistream>(0);
#endif
    }

    /**
       Calls the sysconf system call.

       \param[in] name Parameter to sysconf.
       \returns returnvalue from sysconf.

       \note We add error checking to sysconf. This is the only practical way do it
       on AIX where we call sysconf in a member initializer.
    */
    long CPUPALDependencies::sysconf(int name) const
    {
#if defined(linux) || defined(sun) || defined(aix)
        long retval = ::sysconf(name);
        if (-1 == retval) {
            throw SCXErrnoException(L"sysconf", errno, SCXSRCLOCATION);
        }
        return retval;
#else
        return 1;
#endif
    }

#if defined(sun)
    /**
       Creates a new SCXKstat object with cpu information. Provided for dependency injection purposes. 

       \returns Handle to newly created kstat.
    */
    const SCXHandle<SCXKstat> CPUPALDependencies::CreateKstat(void) const
    {
        return SCXHandle<SCXKstat>(new SCXKstat());
        // Only on Solaris V10: return new SCXKstat(L"cpu", L"sys", cpuid);
    }

    /**
       Calls p_online system call.

       \param[in] processorid parameter to p_online.
       \param[in] flag parameter to p_online.
       \returns returnvalue from p_online.
    */
    int CPUPALDependencies::p_online(processorid_t processorid, int flag) const
    {
        return ::p_online(processorid, flag);
    }
#endif /* sun */

#if defined(hpux)
    /**
       Calls the pstat_getprocessor system call.

       \param[out] buf buffer to store results in.
       \param[in] size of each element in the buf structure.
       \param[in] elemcount specifies the number of pst_processor structures
       that are available at buf to be filled in.
       \param[in] index specifies the starting index within the context of processors.
       \returns Number of processors for which information was retrieved.
    */
    int CPUPALDependencies::pstat_getprocessor(struct pst_processor *buf,
                                               size_t elemsize,
                                               size_t elemcount,
                                               int index) const
    {
        return ::pstat_getprocessor(buf, elemsize, elemcount, index);
    }

    /**
       Calls the pstat_getdynamic system call.

       \param[out] buf buffer to store results in.
       \param[in] size of each element in the buf structure.
       \param[in] elemcount specifies the number of pst_dynamic structures
       that are available at buf to be filled in.
       \param[in] index must be 0.
       \returns Number of processors for which information was retrieved.
    */
    int CPUPALDependencies::pstat_getdynamic(struct pst_dynamic *buf,
                                             size_t elemsize,
                                             size_t elemcount,
                                             int index) const
    {
        return ::pstat_getdynamic(buf, elemsize, elemcount, index);
    }

#endif /* hpux */

#if defined(aix)

    /**
       Creates a new SCXodm object. Provided for dependency injection purposes. 

       \returns Handle to newly created SCXodm class.
    */
    const SCXHandle<SCXodm> CPUPALDependencies::CreateOdm(void) const
    {
        return SCXHandle<SCXodm>(new SCXodm());
    }

    /** Gets global CPU usage

        \param[in] name Must be NULL
        \param[out] buf Memory area that receives data
        \param[in] bufsz Size of perfstat_cpu_total_t structure
        \param[in] number Must be 1
        \returns In case of failure, -1 is returned, or else 1
    */
    int CPUPALDependencies::perfstat_cpu_total(perfstat_id_t *name,
                                               perfstat_cpu_total_t* buf,
                                               int bufsz,
                                               int number) const
    {
        return ::perfstat_cpu_total(name, buf, bufsz, number);
    }

    /** Gets usage data for one or more individual CPUs

        \param[in] name Identifies first cpu to collect statistics from
        \param[out] buf Memory area that receives data
        \param[in] bufsz Size of perfstat_cpu_t structure
        \param[in] number The number of perfstat_cpu_t structure to collect
        \returns Number of filled structures, or -1 in case of failure
    */
    int CPUPALDependencies::perfstat_cpu(perfstat_id_t *name,
                                         perfstat_cpu_t* buf,
                                         int bugsz,
                                         int number) const
    {
        return ::perfstat_cpu(name, buf, bugsz, number);
    }

#endif /* aix */

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents values passed between the threads of the memory instance.

       Representation of values passed between the threads of the memory instance.

    */
    class CPUEnumerationThreadParam : public SCXThreadParam
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor

           \param[in] cpuenum Pointer to cpu enumeration associated with the thread.
        */
        CPUEnumerationThreadParam(CPUEnumeration *cpuenum)
            : SCXThreadParam(), m_cpuenum(cpuenum)
        {}

        /*----------------------------------------------------------------------------*/
        /**
           Retrieves the cpu enumeration parameter.

           \returns Pointer to cpu enumeration associated with the thread.
        */
        CPUEnumeration* GetCPUEnumeration()
        {
            return m_cpuenum;
        }
    private:
        CPUEnumeration* m_cpuenum; //!< Pointer to cpu enumeration associated with the thread.
    };

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor

       \param[in] deps Dependencies for the CPU Enumeration.
    */
    CPUEnumeration::CPUEnumeration(SCXCoreLib::SCXHandle<CPUPALDependencies> deps) :
        EntityEnumeration<CPUInstance>(),
        m_deps(deps),
        m_lock(SCXCoreLib::ThreadLockHandleGet()),
        m_dataAquisitionThread(NULL)
#if defined(aix)
        , m_dataarea(deps->sysconf(_SC_NPROCESSORS_CONF))
#endif /* aix */
#if defined(sun)
        , m_kstatHandle(deps->CreateKstat())
#endif
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.cpu.cpuenumeration");

        SCX_LOGTRACE(m_log, L"CPUEnumeration default constructor");

#if defined(aix)
        // Initiate CPU identifier structure
        strncpy(m_cpuid.name, FIRST_CPU, sizeof(m_cpuid.name));
#endif /* aix */
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor

    */
    CPUEnumeration::~CPUEnumeration()
    {
        SCX_LOGTRACE(m_log, L"CPUEnumeration destructor");
        if (NULL != m_dataAquisitionThread)
        {
            if (m_dataAquisitionThread->IsAlive())
            {
                CleanUp();
            }
            m_dataAquisitionThread = NULL;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
       Create CPU instances

    */
    void CPUEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"CPUEnumeration Init()");

        SetTotalInstance(SCXCoreLib::SCXHandle<CPUInstance>(new CPUInstance(0, true)));

        Update(false);

        if (NULL == m_dataAquisitionThread)
        {
            CPUEnumerationThreadParam* params = new CPUEnumerationThreadParam(this);
            m_dataAquisitionThread = new SCXCoreLib::SCXThread(CPUEnumeration::DataAquisitionThreadBody, params);
        }
    }

#if defined(sun) || defined(hpux)
    /*----------------------------------------------------------------------------*/
    /**
       Check to see if a particular CPU is enabled.

       \param[in]  cpuid   The id of the cpu.
       \returns    True if cpu is enabled, otherwise false.
       \throw      SCXInternalErrorException on HP-UX

    */
    bool CPUEnumeration::IsCPUEnabled(const int cpuid)
    {
#if defined(hpux)
        struct pst_dynamic psd;
        if (m_deps->pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1)
        {
            // !!!!!! Should we throw here?
            throw SCXInternalErrorException(L"pstat_getdynamic() failed", SCXSRCLOCATION);
        }

        // Get the number of maxium CPU's that can be active on this platform
        size_t max_cpus = psd.psd_max_proc_cnt;
        std::vector<struct pst_processor> psp_vector(max_cpus);
        struct pst_processor* psp = &psp_vector[0];

        // Get the processor info, count will contain the number of CPU's, active AND in-active.
        size_t count = m_deps->pstat_getprocessor(psp, sizeof(struct pst_processor), max_cpus, 0);
        if ( count <= 0)
        {
            // !!!!!! Should we throw here?
            throw SCXInternalErrorException(L"pstat_getprocessor() failed", SCXSRCLOCATION);
        }

        for (size_t i = 0; i < count; i++)
        {
            // Match the logical id for the in-active CPU's
            if (cpuid == psp[i].psp_logical_id && PSP_SPU_ENABLED == psp[i].psp_processor_state)
            {
                return true;
            }
        }

#elif defined(sun)
        int cpu_state = m_deps->p_online(cpuid, P_STATUS);

        if (P_ONLINE == cpu_state || P_NOINTR == cpu_state)
        {
            return true;
        } else if (P_OFFLINE == cpu_state || P_POWEROFF == cpu_state)
        {
            return false;
        } else if (-1 == cpu_state)
        {
            SCX_LOGWARNING(m_log, StrAppend(L"error calling p_online(", cpuid) + L", P_STATUS)");
        }

#else

#error "Not implemented for this platform"

#endif

        return false;
    }
#endif

    /**
       The number of physical processors, or zero if we don't know it.

       \param[in] deps Dependencies (primarily for unit test purposes)
       \param[in] logH Log handle (for logging purposes)
       \param[in] fForceComputation Force recomputing values (for unit test purposes)
       \returns The number of physical processors, or zero if unknown.

       \note The physical processor count represents the number of installed
       physical processors (CPU sockets).  A processor with multiple cores is a
       single physical processor, a processor that is hyperthreading is
       considered a single physical processor, and a processor with multiple
       cores and hyperthreading is also considered a single physical processor.

       For example, a physical UNIX phost with 4 physical CPUs, with 4 cores
       each and hyperthraeding would have 4 * 4 * 2 = 32 logical processors
       and 4 physical processors.


       Certain systems compute the number of physical processors very slowly.
       On these systems, we compute only once and save the value.  But, for
       unit test purposes, we can force recomputing if needed.
    */
    size_t CPUEnumeration::ProcessorCountPhysical(
        SCXCoreLib::SCXHandle<CPUPALDependencies> deps,
        SCXCoreLib::SCXLogHandle& logH,
        bool fForceComputation /* = false */)
    {
#if defined(linux)

        (void)fForceComputation;

        SCXHandle<wistream> statFile = deps->OpenCpuinfoFile();
        set<size_t> uniquePhysicalIDs;
        wstring line;
        while (getline(*statFile, line) > 0)
        {
            vector<wstring> tokens;

            SCX_LOGHYSTERICAL(logH, wstring(L"CPUEnumeration ProcessorCountPhysical - Read line: ").append(line));

            StrTokenize(line, tokens, L":");

            if (tokens.size() > 0)
            {
                // See example of stat file at the end of this source code file
                //
                // Count the unique "physical id" lines in the cpuinfo file.
                // Note that physical IDs need not be monotonically increasing
                // (see WI 44326 for more information on this).

                if (0 == tokens[0].compare(L"physical id"))
                {
                    SCX_LOGHYSTERICAL(logH, L"CPUEnumeration ProcessorCountPhysical - Found \"physical id\" row");

                    size_t thisID = StrToUInt(tokens[1]);
                    uniquePhysicalIDs.insert( thisID );
                }
            }
        }

        return uniquePhysicalIDs.size();

#elif defined(sun)

        // On Solaris, kstat calls can be very expensive, especially with very
        // long chains (which will happen if you have lots of CPUs on your
        // system).  As a result, unless forced (for unit test purposes),
        // we look up the proper value once and use it until agent restart.

        static size_t numPhysicalProcs = LONG_MAX;

        if (LONG_MAX == numPhysicalProcs || fForceComputation)
        {
            SCXHandle<SCXSystemLib::SCXKstat> kstat = deps->CreateKstat();
            size_t logicalInstances = ProcessorCountLogical(deps);
            set<scxulong> uniquePhysicalIDs;

            for (int i = 0; i < logicalInstances; i++)
            {
                kstat->Lookup(L"cpu_info", i);
                scxulong thisID = kstat->GetValue(L"chip_id");
                uniquePhysicalIDs.insert( thisID );
            }

            numPhysicalProcs = uniquePhysicalIDs.size();
        }

        return numPhysicalProcs;

#elif defined(hpux) && ((PF_MAJOR > 11) || (PF_MINOR >= 31))

        (void)fForceComputation;

        // We could use ProcessorCountLogical(), but that would result in calls to
        // pstat_getprocessor twice ... in interest of performance, we duplicate
        // that code here

        // Get the number of maxiumum CPU's that can be active on this platform
        struct pst_dynamic psd;
        if (deps->pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1)
        {
            throw SCXInternalErrorException(L"pstat_getdynamic() failed", SCXSRCLOCATION);
        }

        size_t max_cpus = psd.psd_max_proc_cnt;
        std::vector<struct pst_processor> psp_vector(max_cpus);
        struct pst_processor* psp = &psp_vector[0];

        // Do a single call to get all of the processor information (for both active & in-active CPUs)
        size_t count = deps->pstat_getprocessor(psp, sizeof(struct pst_processor), max_cpus, 0);
        if ( count <= 0)
        {
            throw SCXInternalErrorException(L"pstat_getprocessor() failed", SCXSRCLOCATION);
        }

        set<_T_LONG_T> uniquePhysicalIDs;
        for (size_t i = 0; i < count; i++)
        {
            uniquePhysicalIDs.insert( psp[i].psp_socket_id );
        }

        return uniquePhysicalIDs.size();

#elif defined(hpux) && (PF_MAJOR == 11 && PF_MINOR == 23)

        // Kernel patch PHKL_34912 adds support for <struct pst_processor>.psp_socket_id
        // in HP 11i v2.  However, we've chosen not to support that platform at this time.
        //
        // To do so, we'd need to determine if the strucutre provided psp_socket_id at
        // runtime (presumably by querying the size of the structure).  Considering the
        // age of HP 11i v2, it doesn't seem worth it.

        (void) deps;
        return 0;

#elif defined(aix)

        // On AIX, we really just count the number of processor, similar to
        // 'lsdev -c processor | wc -l'.  However, just as a sanity check,
        // verify we are actually getting processors back ...

        SCXHandle<SCXSystemLib::SCXodm> odm = deps->CreateOdm();
        struct CuDv dvData;
        size_t count = 0;

        do {
            void *pResult = odm->Get(CuDv_CLASS, L"name like 'proc*'", &dvData);
            if (NULL == pResult)
            {
                break;
            }
            SCXASSERT( 0 == strncmp("proc", dvData.name, 4) );
            count++;
        } while (true);

        return count;

#else
#error "Not implemented for this platform"
#endif
    }

    /**
       The number of logical processors, or zero if we don't know it.

       \param[in] deps Dependencies (primarily for unit test purposes)
       \returns The number of logical processors.

       \note The logical processor count is the total count of uniquely
       identified processor instances known to the O/S kernel.  The
       logical count value should not distinguish between physical
       processors, processor cores, or hyperthreading.
    */
    size_t CPUEnumeration::ProcessorCountLogical(SCXCoreLib::SCXHandle<CPUPALDependencies> deps)
    {
#if defined(hpux)
        struct pst_dynamic psd;
        if (deps->pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1)
        {
            throw SCXInternalErrorException(L"pstat_getdynamic() failed", SCXSRCLOCATION);
        }

        // Get the number of maxium CPU's that can be active on this platform
        size_t max_cpus = psd.psd_max_proc_cnt;
        std::vector<struct pst_processor> psp_vector(max_cpus);
        struct pst_processor* psp = &psp_vector[0];

        // Get the processor info, count will contain the number of CPU's, active AND in-active.
        size_t count = deps->pstat_getprocessor(psp, sizeof(struct pst_processor), max_cpus, 0);
        if (count <= 0)
        {
            throw SCXInternalErrorException(L"pstat_getprocessor() failed", SCXSRCLOCATION);
        }
#elif defined(linux) || defined(WIN32) || defined(sun) || defined (aix)
        size_t count = deps->sysconf(_SC_NPROCESSORS_ONLN);
#else

#error "Not implemented for this platform"

#endif

        return count;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Update all CPU data

       This is done collectively for all instances by using a platform dependent
       mechanism. The following platforms use the specified mechanisms:
       - Linux       Counters are read from the /proc/stat file.
       - Solaris     Counters are read from the kstat() API.
       - HPUX        Counters are read from the pstat() API:
       - Win32       Counters are read from the stat.txt file in the root directory of drive C.

       Last in this file there is an example of a Linux /proc/stat file

    */
    void CPUEnumeration::Update(bool updateInstances)
    {
        SCXCoreLib::SCXThreadLock lock(m_lock);

#if defined(hpux)
        // Note: The HPUX implementation can't use ProcessorCountLogical because
        // code below requires some of the intermediate values ("psp", for example)

        struct pst_dynamic psd;
        if (m_deps->pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1)
        {
            // !!!!!! Should we throw here?
            throw SCXInternalErrorException(L"pstat_getdynamic() failed", SCXSRCLOCATION);
        }

        // Get the number of maxium CPU's that can be active on this platform
        size_t max_cpus = psd.psd_max_proc_cnt;
        std::vector<struct pst_processor> psp_vector(max_cpus);
        struct pst_processor* psp = &psp_vector[0];

        // Get the processor info, count will contain the number of CPU's, active AND in-active.
        size_t count = m_deps->pstat_getprocessor(psp, sizeof(struct pst_processor), max_cpus, 0);
        if (count <= 0)
        {
            throw SCXInternalErrorException(L"pstat_getprocessor() failed", SCXSRCLOCATION);
        }
#else
        size_t count = ProcessorCountLogical(m_deps);
#endif // defined(hpux)

        SCX_LOGTRACE(m_log, StrAppend(StrAppend(L"CPUEnumeration Update() - ", updateInstances).append(L" - "), count));

#if defined(linux) || defined(WIN32)

        // add cpus if needed
        for (size_t i=Size(); i<count; i++)
        {
            SCX_LOGTRACE(m_log, StrAppend(L"CPUEnumeration Update() - Adding CPU ", i));
            AddInstance(SCXCoreLib::SCXHandle<CPUInstance>(new CPUInstance(static_cast<unsigned int>(i))));
        }

        // remove cpus if needed
        while (count < Size())
        {
            bool found = false;

            for (EntityIterator iter = Begin(); !found && iter != End(); iter++)
            {
                SCXCoreLib::SCXHandle<CPUInstance> inst = *iter;

                if (inst->GetProcNumber() == Size()-1)
                {
                    found = true;
                    SCX_LOGTRACE(m_log, StrAppend(L"CPUEnumeration Update() - Removing CPU ", inst->GetProcNumber()));
                    RemoveInstance(iter);
                }
            }

            if (!found)
            {
                throw SCXInternalErrorException(L"CPU with expected Proc Number not found in internal list",
                                                SCXSRCLOCATION);
            }
        }

#elif defined(sun) || defined(hpux)

        // Remove CPUs no longer online
        for (EntityIterator iter = Begin(); iter != End(); iter++)
        {
            bool proc_checked=false;

            while (!proc_checked && iter != End())
            {
                SCXCoreLib::SCXHandle<CPUInstance> inst = *iter;

                if (!IsCPUEnabled(inst->GetProcNumber()))
                {
                    RemoveInstance(iter);
                    iter = Begin();
                }
                else
                {
                    proc_checked = true;
                }
            }
        }

        // add cpus if needed
#if defined(hpux)
        size_t ix = 0; // index used to get the logical id of a cpu
        for (size_t i = 0; i = static_cast<size_t>(psp[ix].psp_logical_id), ix < max_cpus; ix++)
        {
#elif defined(sun)
        //        for (size_t i = 0; i < max_cpus; i++)
        /* On Solaris 8 we don't know the highest CPU index beforehand, just how many
           we will find. (On Solaris 10 sysconf(_SC_CPUID_MAX) would tell us.) This means 
           that we must test if a certain index is valid before we test if it's online.
           See the man-page for p_online() for an example.
           (And the code-sharing between Solaris and HPUX make the structure just awful.)
        */
             
        // This is the number of CPUs that we expect to find. Not necessarily 
        // consecutively numbered.
        size_t max_cpus = m_deps->sysconf(_SC_NPROCESSORS_CONF);
        size_t cpu_counter = 0;         // Number of found CPUs so far.

        for (size_t i = 0; cpu_counter < max_cpus; i++) 
        {
            // Start by testing if processor with index i exists at all
            int status = m_deps->p_online(i, P_STATUS);
            if (-1 == status) {         // Failed, but why?
                if (EINVAL == errno) {
                    continue;           // Processor with index i not existing, try next index
                } else {
                    throw SCXErrnoException(L"p_online", errno, SCXSRCLOCATION);
                }
            }
            cpu_counter++;
            
#else
#error "This code is only for hpux & sun"
#endif
            if (IsCPUEnabled(i))
            {
                bool found = false;
                
                for (EntityIterator iter = Begin(); !found && iter != End(); iter++)
                {
                    SCXCoreLib::SCXHandle<CPUInstance> inst = *iter;
                    
                    if (inst->GetProcNumber() == i)
                    {
                        found = true;
                    }
                }
                
                if (!found)
                {
                    SCX_LOGTRACE(m_log, StrAppend(L"CPUEnumeration Update() - Adding CPU ", i));
                    AddInstance(SCXCoreLib::SCXHandle<CPUInstance>(new CPUInstance(i)));
                }
            }
        }
#elif defined(aix)

        /* There is nothing to do here for AIX. All instance maintenance is done
           in the updater thread. The actual instances update themselves
           through the code below.
        */

#else

#error "Not implemented for this platform"

#endif

        if (updateInstances)
        {
            UpdateInstances();
        }

    }

    /*----------------------------------------------------------------------------*/
    /**
       Cleanup

    */
    void CPUEnumeration::CleanUp()
    {
        SCX_LOGTRACE(m_log, L"CPUEnumeration CleanUp()");
        m_dataAquisitionThread->RequestTerminate();
        m_dataAquisitionThread->Wait();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Store new data for all instances

    */
    void CPUEnumeration::SampleData()
    {
        SCX_LOGTRACE(m_log, L"CPUEnumeration - Start SampleData");
        SCX_LOGHYSTERICAL(m_log, L"CPUEnumeration SampleData - Aquire lock ");

        SCXCoreLib::SCXThreadLock lock(m_lock);

        SCX_LOGHYSTERICAL(m_log, L"CPUEnumeration SampleData - Lock aquired, get data ");

#if defined(linux) || defined(WIN32)

        SCXHandle<wistream> statFile = m_deps->OpenStatFile();
        wstring line;
        while (getline(*statFile, line) > 0)
        {
            vector<wstring> tokens;
            SCXCoreLib::SCXHandle<CPUInstance> inst(0);

            SCX_LOGHYSTERICAL(m_log, wstring(L"CPUEnumeration SampleData - Read line: ").append(line));

            StrTokenize(line, tokens);

            if (tokens.size() > 0)
            {
                // See example of stat file at the end of this source code file
                if (StrIsPrefix(tokens[0], L"cpu"))
                {
                    if (tokens[0].compare(L"cpu") == 0)
                    {
                        inst = GetTotalInstance();
                        SCX_LOGHYSTERICAL(m_log, L"CPUEnumeration SampleData - Found total row");
                    }

                for (size_t i=0; inst == NULL && i<Size(); i++)
                    {
                        SCXCoreLib::SCXHandle<CPUInstance> tmp_inst = GetInstance(i);

                        // Concatinate a string to match representing a specific cpu
                        if (tokens[0].compare(wstring(L"cpu").append(tmp_inst->GetProcName())) == 0)
                        {
                            inst = tmp_inst;
                            SCX_LOGHYSTERICAL(m_log, StrAppend(L"CPUEnumeration SampleData - Found instance row - ", inst->GetProcNumber()));
                        }
                    }

                    if (inst != NULL)
                    {
                        scxulong user = 0;
                        scxulong nice = 0;
                        scxulong system = 0;
                        scxulong idle = 0;
                        scxulong iowait = 0;
                        scxulong irq = 0;
                        scxulong softirq = 0;

                        if (tokens.size() >= 5)
                        {
                            try
                            {
                                user = StrToULong(tokens[1]);
                                SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Read user = ", user));
                                nice = StrToULong(tokens[2]);
                                SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Read nice = ", nice));
                                system = StrToULong(tokens[3]);
                                SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Read system = ", system));
                                idle = StrToULong(tokens[4]);
                                SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Read idle = ", idle));
                            }
                            catch (const SCXNotSupportedException& e)
                            {
                                SCX_LOGWARNING(m_log, std::wstring(L"Could not parse line from stat file: ").append(line).append(L" - ").append(e.What()));
                            }
                            if (tokens.size() >= 8)
                            {
                                try
                                {
                                    iowait = StrToULong(tokens[5]);
                                    SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Read iowait = ", iowait));
                                    irq = StrToULong(tokens[6]);
                                    SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Read irq = ", irq));
                                    softirq = StrToULong(tokens[7]);
                                    SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Read softirq = ", softirq));
                                }
                                catch (const SCXNotSupportedException& e)
                                {
                                    SCX_LOGWARNING(m_log, std::wstring(L"Could not parse line from stat file: ").append(line).append(L" - ").append(e.What()));
                                }
                            }
                            else
                            {
                                iowait = irq = softirq = 0;
                            }

                            scxulong total_tics = user + nice + system + iowait + irq + softirq + idle;

                            SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Calculate total = ", total_tics));

                            // Add new values using friendship declared on the
                            // instance class (the m_*_tics properties are private)
                            inst->m_UserCPU_tics.AddSample(user);
                            inst->m_NiceCPU_tics.AddSample(nice);
                            inst->m_SystemCPUTime_tics.AddSample(system);
                            inst->m_IdleCPU_tics.AddSample(idle);
                            inst->m_IOWaitTime_tics.AddSample(iowait);
                            inst->m_IRQTime_tics.AddSample(irq);
                            inst->m_SoftIRQTime_tics.AddSample(softirq);
                            inst->m_Total_tics.AddSample(total_tics);

                            SCX_LOGHYSTERICAL(m_log, L"CPUEnumeration SampleData - All Values stored");

                        }
                        else
                        {
                            SCX_LOGERROR(m_log, StrAppend(L"CPUEnumeration SampleData - Too few column in data file - ", tokens.size()));
                        }
                    }
                    else
                    {
                        SCX_LOGERROR(m_log, wstring(L"CPUEnumeration SampleData - No cpu in list found that match row in data file - ").append(tokens[0]));
                    }
                }
            }
        }

#elif defined(sun) || defined(hpux)

        scxulong user_tot = 0;
        scxulong system_tot = 0;
        scxulong idle_tot = 0;
        scxulong iowait_tot = 0;

        scxulong nice_tot = 0;
        scxulong irq_tot = 0;
        scxulong softirq_tot = 0;

#if defined(sun)
// Refresh the Kstat chain - adding/removing instances
                m_kstatHandle->Update();
#endif

        for (EntityIterator iter = Begin(); iter != End(); iter++)
        {
            SCXCoreLib::SCXHandle<CPUInstance> inst = *iter;

            if (IsCPUEnabled(inst->GetProcNumber()))
            {
                try
                {
#if defined(sun)
                    CPUStatHelper stat(inst->GetProcNumber(), m_kstatHandle, m_deps);
#else /* hp */
                    CPUStatHelper stat(inst->GetProcNumber(), m_deps);
#endif
                    user_tot    += stat.User;
                    system_tot  += stat.System;
                    idle_tot    += stat.Idle;
                    iowait_tot  += stat.IOWait;
                    nice_tot    += stat.Nice;
                    irq_tot     += stat.Irq;
                    softirq_tot += stat.SoftIrq;

                    // cerr << endl << "Total tics: " << stat.Total << endl;
                    // cerr << "idle: " << stat.Idle << endl;

                    SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Calculate total = ", stat.Total));

                    // Add new values using friendship declared on the
                    // instance class (the m_*_tics properties are private)
                    inst->m_UserCPU_tics.AddSample(stat.User);
                    inst->m_NiceCPU_tics.AddSample(stat.Nice);
                    inst->m_SystemCPUTime_tics.AddSample(stat.System);
                    inst->m_IdleCPU_tics.AddSample(stat.Idle);
                    inst->m_IOWaitTime_tics.AddSample(stat.IOWait);
                    inst->m_IRQTime_tics.AddSample(stat.Irq);
                    inst->m_SoftIRQTime_tics.AddSample(stat.SoftIrq);
                    inst->m_Total_tics.AddSample(stat.Total);
                }
                catch (const SCXException& e)
                {
                    SCX_LOGWARNING(m_log, StrAppend(L"CPUStatHelper failed for cpu: ", inst->GetProcNumber()).append(L" - ").append(e.What()));
                }
            }
            else
            {
                SCX_LOGINFO(m_log, StrAppend(L"Processor no longer online: ", inst->GetProcNumber()));
            }
        }

        SCXCoreLib::SCXHandle<CPUInstance> inst = GetTotalInstance();

        scxulong total_tics = user_tot + nice_tot + system_tot +
            iowait_tot + irq_tot + softirq_tot + idle_tot;

        // cerr << endl << "Total tics: " << total_tics << endl;
        // cerr << "idle: " << idle_tot << endl;

        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Calculate total = ", total_tics));

        // Add new values using friendship declared on the
        // instance class (the m_*_tics properties are private)
        inst->m_UserCPU_tics.AddSample(user_tot);
        inst->m_NiceCPU_tics.AddSample(nice_tot);
        inst->m_SystemCPUTime_tics.AddSample(system_tot);
        inst->m_IdleCPU_tics.AddSample(idle_tot);
        inst->m_IOWaitTime_tics.AddSample(iowait_tot);
        inst->m_IRQTime_tics.AddSample(irq_tot);
        inst->m_SoftIRQTime_tics.AddSample(softirq_tot);
        inst->m_Total_tics.AddSample(total_tics);

#elif defined(aix)

        unsigned int conf_cpus = m_deps->sysconf(_SC_NPROCESSORS_CONF);
        unsigned int cpucount = ProcessorCountLogical(m_deps);

        /* Sanity check: The number of CPUs online can never be greater than
           the number of configured CPUs. This is a fatal error. */
        if (cpucount > conf_cpus) {
            throw SCXInternalErrorException(L"Number of actual CPUs is greater that the"
                                            L"number of configured CPUs", SCXSRCLOCATION);
        }

        /* Here we increase the number of managed instances to match the number of
           online (logical) CPUs. This cannot happen during normal execution according
           to the documentation; Once a CPU goes offline it won't come back until reboot.
           But it will happen the first time this code is executed since the set of
           instances is initially empty.
        */
        for (unsigned int i = Size(); i < cpucount; i++)
        {
            SCX_LOGTRACE(m_log, StrAppend(L"CPUEnumeration Update() - Adding CPU ", i));
            AddInstance(SCXCoreLib::SCXHandle<CPUInstance>(new CPUInstance(i)));
        }

        /* Remove instances that has gone off-line. AIX guarantees that it always
           the highest numbered (logical) CPU that disappears. This has the side-effect
           that statistics that was accumulated for one (physical) processor is suddently
           reported reported for another processor. But this can only happen if a
           processor malfunction. There are no admin tools that take CPUs offline or online.
        */
        while (Size() > cpucount) {
            // This is a bit iffy way to remove the last instance, but the services
            // provided by the EntityEnumeration class are rather limited.
            unsigned int i = Size();
            EntityIterator iter = Begin();
            SCX_LOGTRACE(m_log, StrAppend(L"CPUEnumeration Update() - Removing CPU ", i - 1));
            while (--i) {
                ++iter;
            }
            RemoveInstance(iter);
        }

        /* Extract the real CPU statistics. */
        int res = m_deps->perfstat_cpu(&m_cpuid, &m_dataarea[0], sizeof(perfstat_cpu_t), cpucount);
        if (res < 0) {
            throw SCXErrnoException(L"perfstat_cpu", errno, SCXSRCLOCATION);
        }

        /* Unlike on Sun and HPUX, we iterate over the most recent CPU statistics
           and match it to the existing list of managed CPUs. */
        for (int i = 0; i < cpucount; i++)
        {
            SCXCoreLib::SCXHandle<CPUInstance> inst = GetInstance(i);
            inst->UpdateDataSampler(&m_dataarea[i]);
        }


        /* Update the total instance. On AIX there is a special syscall to get the
           total CPU data. */
        res = m_deps->perfstat_cpu_total(NULL, &m_dataarea_total, sizeof(perfstat_cpu_total_t), 1);
        if (res < 0) {
            throw SCXErrnoException(L"perfstat_cpu_total", errno, SCXSRCLOCATION);
        }

        SCXCoreLib::SCXHandle<CPUInstance> inst = GetTotalInstance();
        inst->UpdateDataSampler(&m_dataarea_total);

#else
#error "Not implemented for this platform"
#endif
        SCX_LOGTRACE(m_log, L"CPUEnumeration - End SampleData");
    }


    /*----------------------------------------------------------------------------*/
    /**
       Thread body that updates all values

       \param  param Must contain a parameter named "ParamValues" of type CPUEnumerationThreadParam*

       The thread stores new values in all instances once every 10 seconds.

    */
    void CPUEnumeration::DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.cpu.cpuenumeration");
        SCX_LOGTRACE(log, L"CPUEnumeration::DataAquisitionThreadBody()");

        if (0 == param)
        {
            SCXASSERT( ! "No parameters to DagaAquisitionThreadBody");
            return;
        }

        CPUEnumerationThreadParam* params = static_cast<CPUEnumerationThreadParam*>(param.GetData());
        if (0 == params)
        {
            SCXASSERT( ! "Invalid parameters to DataAquisitionThreadBody");
            return;
        }

        CPUEnumeration* cpuenum = params->GetCPUEnumeration();
        if (0 == cpuenum)
        {
            SCXASSERT( ! "CPU Enumeration not set");
            return;
        }

        bool bUpdate = true;
        params->m_cond.SetSleep(CPU_SECONDS_PER_SAMPLE * 1000);
        {
            SCXConditionHandle h(params->m_cond);

            while ( ! params->GetTerminateFlag())
            {
                if (bUpdate)
                {
                    cpuenum->SampleData();
                    bUpdate = false;
                }

                SCX_LOGHYSTERICAL(log, L"CPUEnumeration DataAquisition - Sleep ");
                enum SCXCondition::eConditionResult r = h.Wait();
                if (SCXCondition::eCondTimeout == r)
                {
                    bUpdate = true;
                }
            }
        }

        SCX_LOGHYSTERICAL(log, L"CPUEnumeration DataAquisition - Ending ");
    }

#if defined(sun) || defined(hpux)
    /*----------------------------------------------------------------------------*/
    /**
       Constructor that instantiates any needed api's/objects that are needed for
       the current platform and then reads the available cpu counters.

       \param[in]  cpuid   The id of the cpu you want to read the counters from.
           \param[in]  kstatHandle Handle to an SCXKstat instance to use
       \param[in]  deps    Dependencies for the CPU Enumeration.

       \throw      SCXInternalErrorException if pstat fails.
       \throw      SCXKstatErrorException if kstat internal error.
       \throw      SCXKstatNotFoundException if requested kstat is not found in kstat system.
    */
#if defined(sun)
    CPUEnumeration::CPUStatHelper::CPUStatHelper(unsigned int cpuid, SCXCoreLib::SCXHandle<SCXKstat> kstatHandle, SCXHandle<CPUPALDependencies> deps)
        :User(0), System(0), Idle(0), IOWait(0), Nice(0), Irq(0), SoftIrq(0), Total(0)
        ,m_cpuid(cpuid), m_deps(deps)
        ,m_kstat(kstatHandle)
#else /* hp */
    CPUEnumeration::CPUStatHelper::CPUStatHelper(unsigned int cpuid, SCXHandle<CPUPALDependencies> deps)
        :User(0), System(0), Idle(0), IOWait(0), Nice(0), Irq(0), SoftIrq(0), Total(0)
        ,m_cpuid(cpuid), m_deps(deps)
#endif
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.cpu.cpuenumeration.cpustathelper");

        SCX_LOGTRACE(m_log, L"CPUStatHelper constructor");

        // Initialize
        Init();

#if defined(sun)
        wostringstream id;
        id << "cpu_stat" << cpuid;

                // This will actually change the state of the CPUStatHelper instance to point at a specific cpu
                // Necessary due to the commonality with HP 
                m_kstat->Lookup(L"cpu_stat", id.str(), cpuid);
#endif          

        // Read values;
        Update();
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       Retrieves a cpu statistic counter from kstat.

       \param[in]  statistic   The name of the cpu counter to read.

       \returns    The contents of the counter if it exists, otherwise 0.
    */
    scxulong CPUEnumeration::CPUStatHelper::GetValue(const std::wstring & statistic)
    {
        try
        {
            return m_kstat->GetValue(statistic);
        }
        catch (const SCXKstatException& e)
        {
            SCX_LOGWARNING(m_log, StrAppend(L"kstat.GetValue() failed for " + statistic + L" for cpu: ", m_cpuid).append(L" - ").append(e.What()));
        }
        return 0;
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
       Initializes necessary API's.
    */
    void CPUEnumeration::CPUStatHelper::Init()
    {
#if defined(sun)
        // Nothing to do...
#elif defined(hpux)
        struct pst_dynamic psd;
        if (m_deps->pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1)
        {
            // !!!!!! Should we throw here?
            throw SCXInternalErrorException(L"pstat_getdynamic() failed", SCXSRCLOCATION);
        }

        size_t cpu_count = psd.psd_max_proc_cnt;
        std::vector<struct pst_processor> psp_vector(cpu_count);
        struct pst_processor* psp = &psp_vector[0];
        cpu_count = m_deps->pstat_getprocessor(psp, sizeof(pst_processor), cpu_count, 0);

        if (0 >= cpu_count)
        {
            throw SCXInternalErrorException(L"pstat_getprocessor() failed", SCXSRCLOCATION);
        }

        size_t i;
        for (i = 0; i < cpu_count; i++)
        {
            if (psp[i].psp_logical_id == static_cast<int>(m_cpuid) && psp[i].psp_processor_state == PSP_SPU_ENABLED)
            {
                memcpy(&m_pst_processor, &psp[i], sizeof(m_pst_processor));
                break;
            }
        }

        // Didn't find the CPU.
        if (cpu_count == i)
        {
            memset(&m_pst_processor, 0, sizeof(m_pst_processor));
            m_pst_processor.psp_logical_id = m_cpuid;
            m_pst_processor.psp_processor_state = PSP_SPU_DISABLED;
            User = 0;
            System = 0;
            Idle = 0;
            IOWait = 0;
            Nice = 0;
            Irq = 0;
            SoftIrq = 0;
            SCX_LOGWARNING(m_log, StrAppend(L"Can't find CPU with logical id: ", m_cpuid));
        }
#else

#error "Not implemented for this platform"

#endif
    }
    /*----------------------------------------------------------------------------*/
    /**
       Retrieves all counters relevant for the platform.
    */
    void CPUEnumeration::CPUStatHelper::Update()
    {
#if defined(sun)
//      The Solaris V10 implementation
//          User    = GetValue(L"cpu_ticks_user");
//          System  = GetValue(L"cpu_ticks_kernel");
//          Idle    = GetValue(L"cpu_ticks_idle");
//          IOWait  = GetValue(L"cpu_ticks_wait");

        const cpu_stat *cpu_stat_p = 0;
        m_kstat->GetValueRaw(cpu_stat_p);
        
        User   = cpu_stat_p->cpu_sysinfo.cpu[CPU_USER];
        System = cpu_stat_p->cpu_sysinfo.cpu[CPU_KERNEL];
        Idle   = cpu_stat_p->cpu_sysinfo.cpu[CPU_IDLE];
        IOWait = cpu_stat_p->cpu_sysinfo.cpu[CPU_WAIT];
        
#elif defined(hpux)
        Init();
        User    = m_pst_processor.psp_cpu_time[CP_USER];
        Nice    = m_pst_processor.psp_cpu_time[CP_NICE];
        Idle    = m_pst_processor.psp_cpu_time[CP_IDLE];
        System  = m_pst_processor.psp_cpu_time[CP_SYS];
        IOWait  = m_pst_processor.psp_cpu_time[CP_WAIT];
#else


#error "Not implemented for this platform"

#endif
        // cout << " User " <<  User
        //      << " System " <<  System
        //      << " Idle " <<  Idle
        //      << " IOWait " <<  IOWait << endl;
            
        // Calculate total
        Total = User + Nice + Idle + System + IOWait + Irq + SoftIrq;
    }
    /*----------------------------------------------------------------------------*/
    /**
       Destructor.
    */
    CPUEnumeration::CPUStatHelper::~CPUStatHelper()
    {
    }

#endif
}

/**
   \page Example of /proc/stat file on SuSE 10

   \ex
   \code
   cpu  1576354 1488 3743739 119530419 17217 29494 9955328 0
   cpu0 1576354 1488 3743739 119530419 17217 29494 9955328 0
   intr 672722294 337114039 58 0 1 1 0 4 0 2 0 0 0 5797 0 0 13094823 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1577700 0 0 0 0 0 0 0 320929058 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 811 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   ctxt 973896784
   btime 1180290802
   processes 1067872
   procs_running 6
   procs_blocked 0
   \endcode
*/

/**
   \page Example of /proc/cpuinfo file from a RHEL 5 system on a Hyper-V VM
   with 3 processors allocated, on a host with a single quad-core CPU

   \ex
   \code
   processor       : 0
   vendor_id       : GenuineIntel
   cpu family      : 6
   model           : 26
   model name      : Intel(R) Xeon(R) CPU           W3520  @ 2.67GHz
   stepping        : 5
   cpu MHz         : 2596.697
   cache size      : 8192 KB
   physical id     : 0
   siblings        : 3
   core id         : 0
   cpu cores       : 3
   apicid          : 0
   …

   processor       : 1
   vendor_id       : GenuineIntel
   cpu family      : 6
   model           : 26
   model name      : Intel(R) Xeon(R) CPU           W3520  @ 2.67GHz
   stepping        : 5
   cpu MHz         : 2596.697
   cache size      : 8192 KB
   physical id     : 0
   siblings        : 3
   core id         : 1
   cpu cores       : 3
   apicid          : 1
   …

   processor       : 2
   vendor_id       : GenuineIntel
   cpu family      : 6
   model           : 26
   model name      : Intel(R) Xeon(R) CPU           W3520  @ 2.67GHz
   stepping        : 5
   cpu MHz         : 2596.697
   cache size      : 8192 KB
   physical id     : 0
   siblings        : 3
   core id         : 2
   cpu cores       : 3
   apicid          : 2
   …
   \endcode
 */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
