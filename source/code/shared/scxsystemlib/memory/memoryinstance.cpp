/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       PAL representation of system memory

    \date        2007-07-02 17:50:20

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>
#include <scxsystemlib/memoryinstance.h>
#include <string>
#include <sstream>

#if defined(sun)
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/swap.h>
#include <sys/sysinfo.h>
#endif

#if defined(hpux)
#include <sys/pstat.h>
#endif

#if defined(aix)
#include <libperfstat.h>
#endif

using namespace SCXCoreLib;

namespace SCXSystemLib
{

#if defined(linux)

    /*----------------------------------------------------------------------------*/
    /**
        Get all lines from meminfo file

        \returns     vector of lines
    */
    std::vector<std::wstring> MemoryDependencies::GetMemInfoLines()
    {
        const std::wstring meminfoFileName = L"/proc/meminfo";
        std::vector<std::wstring> lines;
        SCXStream::NLFs nlfs;

        SCXFile::ReadAllLines(SCXFilePath(meminfoFileName), lines, nlfs);

        return lines;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get all lines from vmstat file

        \returns     vector of lines
    */
    std::vector<std::wstring> MemoryDependencies::GetVMStatLines()
    {
        const std::wstring vmstatFileName = L"/proc/vmstat";
        std::vector<std::wstring> lines;
        SCXStream::NLFs nlfs;

        SCXFile::ReadAllLines(SCXFilePath(vmstatFileName), lines, nlfs);

        return lines;
    }

#elif defined(sun)

    /*----------------------------------------------------------------------------*/
    /**
        Get page size

        \returns     size of a page
    */
    scxulong MemoryDependencies::GetPageSize()
    {
        long pageSizeL = sysconf(_SC_PAGESIZE);
        SCXASSERT(-1 != pageSizeL && "_SC_PAGESIZE not found");
        return static_cast<scxulong>(pageSizeL);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages of physical memory

        \returns     number of pages
    */
    scxulong MemoryDependencies::GetPhysicalPages()
    {
        long physPagesL = sysconf(_SC_PHYS_PAGES);
        SCXASSERT(-1 != physPagesL && "_SC_PHYS_PAGES not found");
        return static_cast<scxulong>(physPagesL);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages of available physical memory

        \returns     number of pages
    */
    scxulong MemoryDependencies::GetAvailablePhysicalPages()
    {
        long availPhysPagesL = sysconf(_SC_AVPHYS_PAGES);
        SCXASSERT(-1 != availPhysPagesL && "_SC_AVPHYS_PAGES not found");
        return static_cast<scxulong>(availPhysPagesL);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pus.

        \returns     number of cpus
    */
    long MemoryDependencies::GetNumberOfCPUs()
    {
        long number_of_cpus = sysconf(_SC_NPROCESSORS_CONF);
        SCXASSERT(-1 != number_of_cpus && "_SC_NPROCESSORS_CONF not found");
        return number_of_cpus;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total number of pages for swap and number of reserved pages

        \param[out]  max_pages        max number of pages for swap
        \param[out]  reserved_pages   number of reserved pages in swap
    */
    void MemoryDependencies::GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages)
    {
        struct anoninfo swapinfo;
        int result = swapctl(SC_AINFO, &swapinfo);
        SCXASSERT(-1 != result && "swapctl failed");

        max_pages = swapinfo.ani_max;
        reserved_pages = swapinfo.ani_resv;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Tests if a specified processor is installed on system.

        \param[in]   id               Processor id
        \returns     True if specified processor exists on system
        \throws      SCXErrnoException if test fails
    */
    bool MemoryDependencies::IsProcessorPresent(int id)
    {
        int status = ::p_online(id, P_STATUS);
        if (-1 == status) {         // Failed, but why?
            if (EINVAL == errno) {
                return false;           // Processor not present
            } else {
                throw SCXErrnoException(L"p_online", errno, SCXSRCLOCATION);
            }
        }
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Creates a new SCXKstat object with cpu/vm information.

       \param[in]  cpuid  Id number of cpu to create kstat object for.

       \returns  Handle to newly created kstat.
    */
    SCXHandle<SCXKstat> MemoryDependencies::CreateKstat()
    {
        return SCXHandle<SCXKstat>(new SCXKstat());
    }

#elif defined(hpux)

        /*
          HP kindly provides an easy way to read all kind of system and kernel data.
          This is collectively known as the pstat interface.
          It's supposed to be relatively upgrade friendly, even without recompilation.
          What is lacking however, is documentation. There is a whitepaper on pstat that
          you can look for at HP's site, which is very readable. But the exact semantics of
          each and every parameter is subject to experimentation and guesswork. I read
          somewhere that, to truly understand them you would need to have access to the
          kernel source. Needless to say, we don't. I've written a document called
          "Memory monitoring on HPUX.docx" that summarizes the needs and what's available.

          These are the system variables that we use together with ALL the documentation that HP provide

          psts.page_size       - page size in bytes/page
          psts.physical_memory - system physical memory in 4K pages
          pstd.psd_rm          - total real memory
          pstd.psd_free        - free memory pages
          pstv.psv_swapspc_max - max pages of on-disk backing store
          pstv.psv_swapspc_cnt - pages of on-disk backing store
          pstv.psv_swapmem_max - max pages of in-memory backing store
          pstv.psv_swapmem_cnt - pages of in-memory backing store
          pstv.psv_swapmem_on  - in-memory backing store enabled

          For usedMemory we use a measure of all real (physical) memory assigned to processes
          For availableMemory we use the size of unassigned memory
        */

    /*----------------------------------------------------------------------------*/
    /**
        Get page size and size in pages of physical memory

        \param[out]  page_size        size of a page
        \param[out]  physical_memory  number of pages of physical memory
    */
    void MemoryDependencies::GetStaticMemoryInfo(scxulong& page_size, scxulong& physical_memory)
    {
        struct pst_static psts;

        if (pstat_getstatic(&psts, sizeof(psts), 1, 0) < 0)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do pstat_getstatic()", SCXSRCLOCATION);
        }

        physical_memory = static_cast<scxulong>(psts.physical_memory);
        page_size = static_cast<scxulong>(psts.page_size);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages used and free memory

        \param[out]  real_pages  number of used pages
        \param[out]  free_pages  number of free pages
    */
    void MemoryDependencies::GetDynamicMemoryInfo(scxulong& real_pages, scxulong& free_pages)
    {
        struct pst_dynamic pstd;

        if (pstat_getdynamic(&pstd, sizeof(pstd), 1, 0) < 0)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do pstat_getdynamic()", SCXSRCLOCATION);
        }

        real_pages = static_cast<scxulong>(pstd.psd_rm);
        free_pages = static_cast<scxulong>(pstd.psd_free);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total number of pages for swap and number of reserved pages

        \param[out]  max_pages        max number of pages for swap
        \param[out]  reserved_pages   number of reserved pages in swap
    */
    void MemoryDependencies::GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages)
    {
        struct pst_vminfo pstv;

        if (pstat_getvminfo(&pstv, sizeof(pstv), 1, 0) < 0)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do pstat_getvminfo()", SCXSRCLOCATION);
        }

        max_pages = static_cast<scxulong>(pstv.psv_swapspc_max + pstv.psv_swapmem_on * pstv.psv_swapmem_max);
        reserved_pages = static_cast<scxulong>(pstv.psv_swapspc_cnt + pstv.psv_swapmem_on * pstv.psv_swapmem_cnt);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Get total number of page reads and writes

        \param[out]  reads   number of reads
        \param[out]  writes  number of writes

        \returns     true if a values gould be retrieved
    */
    bool MemoryDependencies::GetPageingData(scxulong& reads, scxulong& writes)
    {
        struct pst_vminfo pstv;

        /* Get information about the system virtual memory variables */
        if (pstat_getvminfo(&pstv, sizeof(pstv), 1, 0) != 1) {
            return false;
        }

        // These are the system variables that we use together with ALL the documentation that HP provide
        // pstv.psv_spgpgin     - pages paged in
        // pstv.psv_spgpgout    - pages paged out

        reads = pstv.psv_spgpgin;
        writes = pstv.psv_spgpgout;

        /*
          Note: There's a variable that count the total number of faults taken: pstv.psv_sfaults
          There are also measures of the rates for all these. They are, respectively:
          pstv.psv_rpgin, pstv.psv_rpgout, and pstv.psv_rfaults.
        */

        return true;
    }

#elif defined(aix)

    /*----------------------------------------------------------------------------*/
    /**
        Get total memory size, free memory size, swap size and free swap size

        \param[out]  total_pages      Total memory size
        \param[out]  free_pages       Free memory size
        \param[out]  max_swap_pages   Swap size
        \param[out]  free_swap_pages  Free swap size

        All sizes in pages.
        A page on AIX is 4K.
    */
    void MemoryDependencies::GetMemInfo(scxulong& total_pages, scxulong& free_pages, scxulong& max_swap_pages, scxulong& free_swap_pages)
    {
        perfstat_memory_total_t mem;
   
        if (perfstat_memory_total(NULL, &mem, sizeof(perfstat_memory_total_t), 1) != 1)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do perfstat_memory_total()", SCXSRCLOCATION);
        }

        total_pages = static_cast<scxulong>(mem.real_total);
        free_pages = static_cast<scxulong>(mem.real_free + mem.numperm); // take in account file buffers

        max_swap_pages = static_cast<scxulong>(mem.pgsp_total);
        free_swap_pages = static_cast<scxulong>(mem.pgsp_free);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total number of page reads and writes

        \param[out]  reads   number of reads
        \param[out]  writes  number of writes

        \returns     true if a values gould be retrieved
    */
    bool MemoryDependencies::GetPageingData(scxulong& reads, scxulong& writes)
    {
        perfstat_memory_total_t mem;
   
        if (perfstat_memory_total(NULL, &mem, sizeof(perfstat_memory_total_t), 1) != 1)
        {
            return false;
        }

        reads = mem.pgins;
        writes = mem.pgouts;

        return true;
    }

#else
#error "Not implemented for this platform."
#endif

    /*----------------------------------------------------------------------------*/
    /**
        Class that represents values passed between the threads of the memory instance.

        Representation of values passed between the threads of the memory instance.

    */
    class MemoryInstanceThreadParam : public SCXThreadParam
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor.

            \param[in] pageReads   Datasampler for holding measurements of page reads.
            \param[in] pageWrites  Datasampler for holding measurements of page writes.
            \param[in] deps        Dependencies for the Memory data colletion.

        */
        MemoryInstanceThreadParam(MemoryInstanceDataSampler* pageReads,
                                  MemoryInstanceDataSampler* pageWrites,
                                  SCXCoreLib::SCXHandle<MemoryDependencies> deps,
                                  MemoryInstance* inst)
            : SCXThreadParam(),
              m_pageReads(pageReads),
              m_pageWrites(pageWrites),
              m_deps(deps),
              m_inst(inst)
        {}

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the page reads parameter.

            \returns Pointer to datasampler for holding measurements of page reads.

        */
        MemoryInstanceDataSampler* GetPageReads()
        {
            return m_pageReads;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the page writes parameter.

            \returns Pointer to datasampler for holding measurements of page writes.

        */
        MemoryInstanceDataSampler* GetPageWrites()
        {
            return m_pageWrites;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the dependency structure.

            \returns Pointer to the dependency structure

        */
        SCXCoreLib::SCXHandle<MemoryDependencies> GetDeps()
        {
            return m_deps;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the instance pointer.

            \returns Pointer to the memory instance

        */
        MemoryInstance* GetInst()
        {
            return m_inst;
        }

    private:
        MemoryInstanceDataSampler* m_pageReads;           //!< Pointer to datasampler for holding measurements of page reads.
        MemoryInstanceDataSampler* m_pageWrites;          //!< Pointer to datasampler for holding measurements of page writes.
        SCXCoreLib::SCXHandle<MemoryDependencies> m_deps; //!< Collects external dependencies.
        MemoryInstance* m_inst;                           //!< Pointer to to the memory instance
    };


    /*----------------------------------------------------------------------------*/
    /**
        Constructor

       \param[in] deps  Dependencies for the Memory data colletion.

    */
    MemoryInstance::MemoryInstance(SCXCoreLib::SCXHandle<MemoryDependencies> deps, bool startThread /* = true */) :
        EntityInstance(true),
        m_deps(deps),
        m_totalPhysicalMemory(0),
        m_availableMemory(0),
        m_usedMemory(0),
        m_totalSwap(0),
        m_availableSwap(0),
        m_usedSwap(0),
        m_pageReads(),
        m_pageWrites(),
#if defined(hpux)
        m_reservedMemoryIsSupported(true),
#else
        m_reservedMemoryIsSupported(false),
#endif
        m_dataAquisitionThread(0)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.memory.memoryinstance");
        SCX_LOGTRACE(m_log, L"MemoryInstance default constructor");

#if defined(sun)
        m_kstat = deps->CreateKstat();
#endif

        if (startThread)
        {
            MemoryInstanceThreadParam* params = new MemoryInstanceThreadParam(&m_pageReads, &m_pageWrites, m_deps, this);
        m_dataAquisitionThread = new SCXCoreLib::SCXThread(MemoryInstance::DataAquisitionThreadBody, params);
    }
    }


    /*----------------------------------------------------------------------------*/
    /**
        Destructor

    */
    MemoryInstance::~MemoryInstance()
    {
        SCX_LOGTRACE(m_log, L"MemoryInstance destructor");
        if (NULL != m_dataAquisitionThread) {
            if (m_dataAquisitionThread->IsAlive())
            {
                CleanUp();
            }
            m_dataAquisitionThread = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total memory.

        \param[out]  totalPhysicalMemory Total physical memory of machine in MB.

        \returns     true if a value is supported by the implementation

    */
    bool MemoryInstance::GetTotalPhysicalMemory(scxulong& totalPhysicalMemory) const
    {
        totalPhysicalMemory = m_totalPhysicalMemory;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get available (free) memory.

        \param[out]  availableMemory Available memory in MB.

        \returns     true if a value is supported by this implementation

        This is the amount of physical memory that is currently available for
        use by user processes. In other words; memory that is not reported under
        UsedMemory or ReservedMemory.
    */
    bool MemoryInstance::GetAvailableMemory(scxulong& availableMemory) const
    {
        availableMemory = m_availableMemory;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get reserved memory.

        \param[out]  reservedMemory Amount of reserved memory in MB.

        \returns     true if a value is supported by this implementation

        This is the amount of memory that the system has reserved for special
        purposes, and that will never be available for user processes.
        On some, if not most, systems this figure is unavailabe. In those cases
        any reserved memory will included in the number for UsedMemory.

        \note This was added because HPUX can potentially reserve a huge
        amount of pysical memory for its pseudo-swap feature that would seriosly
        skew the UsedMemory reading.
    */
    bool MemoryInstance::GetReservedMemory(scxulong& reservedMemory) const
    {
        reservedMemory = m_reservedMemory;
        return m_reservedMemoryIsSupported;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get used memory.

        \param[out]  usedMemory Amount of used memory.

        \returns     true if a value is supported by this implementation

        The amount of physical memory that is currenly allocated. If ReservedMemory
        is supported this number is mostly memory used by user processes. If not,
        this figure includes memory reserved by the system.
    */
    bool MemoryInstance::GetUsedMemory(scxulong& usedMemory) const
    {
        usedMemory = m_usedMemory;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages read from disk per second to resolve a hard page fault.

        \param[out] pageReads Number of page reads per second.

        \returns    true if a value is supported by this implementation

    */
    bool MemoryInstance::GetPageReads(scxulong& pageReads) const
    {
        pageReads = m_pageReads.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES)
            / MEMORY_SECONDS_PER_SAMPLE;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages written to disk per second to resolve hard page faults.

        \param[out] pageWrites Number of page writes per second.

        \returns    true if a value is supported by this implementation

    */
    bool MemoryInstance::GetPageWrites(scxulong& pageWrites) const
    {
        pageWrites = m_pageWrites.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES)
            / MEMORY_SECONDS_PER_SAMPLE;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the total amount of swap space in MB.

        \param[out]  totalSwap Total amount of swap space.

        \returns     true if a value is supported by this implementation

    */
    bool MemoryInstance::GetTotalSwap(scxulong& totalSwap) const
    {
        totalSwap = m_totalSwap;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get amount of available (free) swap space in MB.

        \param[out]  availableSwap Amount of available swap space.

        \returns     true if a value is supported by this implementation

    */
    bool MemoryInstance::GetAvailableSwap(scxulong& availableSwap) const
    {
        availableSwap = m_availableSwap;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the amount of used swap space in MB.

        \param[out]  usedSwap The amount of used swap space.
        \returns     true if a value is supported by this implementation

    */
    bool MemoryInstance::GetUsedSwap(scxulong& usedSwap) const
    {
        usedSwap = m_usedSwap;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update the object members with values from hardware.
        This method updates all values that are not time dependent.
        Time dependent values are updated from a separate thread.

    */
    void MemoryInstance::Update()
    {
        SCX_LOGTRACE(m_log, L"MemoryInstance Update()");

#if defined(linux)

    /**
        Update the object members with values from /proc/meminfo.

        \ex
        /proc/meminfo:
          MemTotal:       516400 kB
          MemFree:         91988 kB
          Buffers:         46148 kB
          Cached:         277860 kB
          SwapCached:      64136 kB
          Active:         299804 kB
          Inactive:        97372 kB
          HighTotal:           0 kB
          HighFree:            0 kB
          LowTotal:       516400 kB
          LowFree:         91988 kB
          SwapTotal:      514040 kB
          SwapFree:       402508 kB
          Dirty:             676 kB
          Writeback:           0 kB
          Mapped:          96832 kB
          Slab:            20356 kB
          CommitLimit:    772240 kB
          Committed_AS:   281692 kB
          PageTables:       1700 kB
          VmallocTotal:   507896 kB
          VmallocUsed:      3228 kB
          VmallocChunk:   504576 kB
          HugePages_Total:     0
          HugePages_Free:      0
          HugePages_Rsvd:      0
          Hugepagesize:     4096 kB

        We are interested in the following fields:
          MemTotal
          MemFree
          SwapTotal
          SwapFree
    */
        std::vector<std::wstring> lines = m_deps->GetMemInfoLines();

        bool foundTotalPhysMem = false;
        bool foundAvailMem = false;
        bool foundTotalSwap = false;
        bool foundAvailSwap = false;
        scxulong buffers = 0, cached = 0;

        for (size_t i=0; i<lines.size(); i++)
        {
            std::wstring line = lines[i];

            SCX_LOGHYSTERICAL(m_log, std::wstring(L"UpdateFromMemInfo() - Read line: ").append(line));

            std::vector<std::wstring> tokens;
            StrTokenize(line, tokens);
            if (tokens.size() >= 2)
            {
                if (L"MemTotal:" == tokens[0])
                {
                    try
                    {
                        m_totalPhysicalMemory = KiloBytesToMegaBytes(StrToULong(tokens[1]));
                        foundTotalPhysMem = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    totalPhysicalMemory = ", m_totalPhysicalMemory));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_totalPhysicalMemory from :").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"MemFree:" == tokens[0])
                {
                    try
                    {
                        m_availableMemory = KiloBytesToMegaBytes(StrToULong(tokens[1]));
                        foundAvailMem = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    availableMemory = ", m_availableMemory));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_availableMemory from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"Buffers:" == tokens[0])
                {
                    try
                    {
                        buffers = KiloBytesToMegaBytes(StrToULong(tokens[1]));
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    buffers = ", buffers));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read buffers from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"Cached:" == tokens[0])
                {
                    try
                    {
                        cached = KiloBytesToMegaBytes(StrToULong(tokens[1]));
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Cached = ", cached));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read buffers from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"SwapTotal:" == tokens[0])
                {
                    try
                    {
                        m_totalSwap = KiloBytesToMegaBytes(StrToULong(tokens[1]));
                        foundTotalSwap = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    totalSwap = ", m_totalSwap));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_totalSwap from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"SwapFree:" == tokens[0])
                {
                    try
                    {
                        m_availableSwap = KiloBytesToMegaBytes(StrToULong(tokens[1]));
                        foundAvailSwap = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    availableSwap = ", m_availableSwap));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_availableSwap from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
            }
        }

        // perform some adjustments and calculations
        m_availableMemory += buffers + cached;
        m_usedMemory = m_totalPhysicalMemory - m_availableMemory;
        m_usedSwap = m_totalSwap - m_availableSwap;
        
        SCXASSERT(foundTotalPhysMem && "MemTotal not found");
        SCXASSERT(foundAvailMem && "MemFree not found");
        SCXASSERT(foundTotalSwap && "SwapTotal not found");
        SCXASSERT(foundAvailSwap && "SwapFree not found");

#elif defined(sun)

        /*
          Update the object members with info from sysconf and swapctl.
        */

        scxulong pageSize = m_deps->GetPageSize();
        m_totalPhysicalMemory = BytesToMegaBytes(m_deps->GetPhysicalPages() * pageSize);
        m_availableMemory = BytesToMegaBytes(m_deps->GetAvailablePhysicalPages() * pageSize);
        m_usedMemory = m_totalPhysicalMemory - m_availableMemory;

        scxulong max_pages = 0;
        scxulong reserved_pages = 0;
        m_deps->GetSwapInfo(max_pages, reserved_pages);

        m_totalSwap = BytesToMegaBytes(max_pages * pageSize);
        m_availableSwap = BytesToMegaBytes((max_pages - reserved_pages) * pageSize);
        m_usedSwap = BytesToMegaBytes(reserved_pages * pageSize);

#elif defined(hpux)

        scxulong page_size = 0;
        scxulong physical_memory = 0;
        scxulong real_pages = 0;
        scxulong free_pages = 0;

        m_deps->GetStaticMemoryInfo(page_size, physical_memory);
        m_deps->GetDynamicMemoryInfo(real_pages, free_pages);

        m_totalPhysicalMemory = BytesToMegaBytes(physical_memory * page_size);
        m_usedMemory = BytesToMegaBytes(real_pages * page_size);
        m_availableMemory = BytesToMegaBytes(free_pages * page_size);

        // The reservedMemory size varies with a few MB up and down, so it's best to recompute 
        // this number every time so that the used and free percentages adds up.
        m_reservedMemory = m_totalPhysicalMemory - m_usedMemory - m_availableMemory;

        scxulong max_pages = 0;
        scxulong reserved_pages = 0;

        m_deps->GetSwapInfo(max_pages, reserved_pages);

        // totalSwap is the total size of all external swap devices plus swap memory, if enabled
        // availableSwap is the size of remaining device swap (with reserved memory subtracted)
        // plus remaining swap memory, if that was enabled in system configuration.
        // usedSwap is the difference between those. This is consistent with the 'total'
        // numbers when you do 'swapinfo -t'.
        m_totalSwap = BytesToMegaBytes(max_pages * page_size);
        m_availableSwap = BytesToMegaBytes(reserved_pages * page_size);
        m_usedSwap = m_totalSwap - m_availableSwap;

#elif defined(aix)

        scxulong total_pages = 0;
        scxulong free_pages = 0;
        scxulong max_swap_pages = 0;
        scxulong free_swap_pages = 0;

        m_deps->GetMemInfo(total_pages, free_pages, max_swap_pages, free_swap_pages);

        // All memory data given in 4KB pages

        m_totalPhysicalMemory = (total_pages / 1024) * 4;
        m_availableMemory = (free_pages / 1024) * 4;
        m_usedMemory = m_totalPhysicalMemory - m_availableMemory;

        m_totalSwap = (max_swap_pages / 1024) * 4;
        m_availableSwap = (free_swap_pages / 1024) * 4;
        m_usedSwap = m_totalSwap - m_availableSwap;

#else
#error "Not implemented for this platform."
#endif

    }

    /*----------------------------------------------------------------------------*/
    /**
        Clean up the instance. Closes the thread.

    */
    void MemoryInstance::CleanUp()
    {
        SCX_LOGTRACE(m_log, L"MemoryInstance CleanUp()");
        if (NULL != m_dataAquisitionThread) {
        m_dataAquisitionThread->RequestTerminate();
        m_dataAquisitionThread->Wait();
            m_dataAquisitionThread = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns     The object represented as a string suitable for logging.

    */
    const std::wstring MemoryInstance::DumpString() const
    {
        std::wstringstream ss;
        ss << L"MemoryInstance: totalPhysMem = " << m_totalPhysicalMemory
           << L", availableMem = " << m_availableMemory
           << L", usedMem = " << m_usedMemory
           << L", pageReads = " << m_pageReads.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES) / MEMORY_SECONDS_PER_SAMPLE
           << L", pageWrites = " << m_pageWrites.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES) / MEMORY_SECONDS_PER_SAMPLE
           << L", totalSwap = " << m_totalSwap
           << L", availableSwap = " << m_availableSwap
           << L", usedSwap = " << m_usedSwap;
        return ss.str();
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
        Get the SCXKstat handle

        \returns     SCXKstat handle

    */
    SCXCoreLib::SCXHandle<SCXKstat> MemoryInstance::GetKstat() 
    { 
        return m_kstat; 
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
        Utility function to retrieve the page reads and page writes since boot
        from the system.

        \param[out]  pageReads   Number of page reads since boot.
        \param[out]  pageWrites  Number of page writes since boot.
        \param[in]   deps        Dependencies for the Memory data colletion.

        \returns     true if the values are supported by this implementation

    */
    bool MemoryInstance::GetPagingSinceBoot(scxulong& pageReads, scxulong& pageWrites, MemoryInstance* inst, SCXCoreLib::SCXHandle<MemoryDependencies> deps)
    {
        SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.memory.memoryinstance");
        SCX_LOGHYSTERICAL(log, L"MemoryInstance::GetPagingSinceBoot()");

        if (NULL == inst)
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"inst", L"MemoryInstance instance parameter to GetPagingSinceBoot() is NULL", SCXSRCLOCATION);
        }

#if defined(linux)

        /**
           Get pageReads and pageWrites from values in /proc/vmstat.

           Example /proc/vmstat:
           nr_dirty 134
           nr_writeback 0
           nr_unstable 0
           nr_page_table_pages 432
           nr_mapped 24087
           nr_slab 5073
           pgpgin 1467244
           pgpgout 7330560
           pswpin 34123
           pswpout 248659
           pgalloc_high 0
           pgalloc_normal 119867240
           pgalloc_dma32 0
           pgalloc_dma 138137
           pgfree 120028486
           pgactivate 1852759
           pgdeactivate 2003853
           pgfault 261199917
           pgmajfault 9456
           pgrefill_high 0
           pgrefill_normal 3227693
           pgrefill_dma32 0
           pgrefill_dma 94902
           pgsteal_high 0
           pgsteal_normal 601920
           pgsteal_dma32 0
           pgsteal_dma 14627
           pgscan_kswapd_high 0
           pgscan_kswapd_normal 683001
           pgscan_kswapd_dma32 0
           pgscan_kswapd_dma 17469
           pgscan_direct_high 0
           pgscan_direct_normal 1641420
           pgscan_direct_dma32 0
           pgscan_direct_dma 44062
           pginodesteal 432
           slabs_scanned 232960
           kswapd_steal 412728
           kswapd_inodesteal 29634
           pageoutrun 8016
           allocstall 2488
           pgrotated 248502
           nr_bounce 0

           We are interested in the following fields:
           pgpgin
           pgpgout

        */
        try 
        {
            std::vector<std::wstring> lines = deps->GetVMStatLines();

            bool foundPgpgin = false;
            bool foundPgpgout = false;
    
            for (size_t i=0; (!foundPgpgin || !foundPgpgout) && i<lines.size(); i++)
            {
                std::wstring line = lines[i];

                SCX_LOGHYSTERICAL(log, std::wstring(L"DataAquisitionThreadBody() - Read line: ").append(line));
    
                std::vector<std::wstring> tokens;
                StrTokenize(line, tokens);
                if (tokens.size() >= 2)
                {
                    if (L"pgpgin" == tokens[0])
                    {
                        try
                        {
                            pageReads = StrToULong(tokens[1]);
                            foundPgpgin = true;
                            SCX_LOGHYSTERICAL(log, StrAppend(L"    pageReads = ", pageReads));
                        }
                        catch (const SCXNotSupportedException& e)
                        {
                            SCX_LOGWARNING(log, std::wstring(L"Could not read pageReads from: ").append(line).append(L" - ").append(e.What()));
                        }
                    }
                    if (L"pgpgout" == tokens[0])
                    {
                        try
                        {
                            pageWrites = StrToULong(tokens[1]);
                            foundPgpgout = true;
                            SCX_LOGHYSTERICAL(log, StrAppend(L"    pageWrites = ", pageWrites));
                        }
                        catch (const SCXNotSupportedException& e)
                        {
                            SCX_LOGWARNING(log, std::wstring(L"Could not read pageWrites from: ").append(line).append(L" - ").append(e.What()));
                        }
                    }
                }
            }
            SCXASSERT(foundPgpgin && "pgpgin not found.");
            SCXASSERT(foundPgpgout && "pgpgout not found.");
        } 
        catch (SCXFileSystemException &e) 
        {
            SCX_LOGERROR(log, std::wstring(L"Could not open /proc/vmstat for reading: ").append(e.What()));
            return false;            
        }

#elif defined(sun)

        long number_of_cpus = deps->GetNumberOfCPUs();
        long cpu_counter = 0;
        pageReads = 0;
        pageWrites = 0;
        inst->GetKstat()->Update();
        for(int cpu_index = 0; cpu_counter < number_of_cpus; ++cpu_index) {
            
            /* Is this CPU installed? */
            if (!deps->IsProcessorPresent(cpu_index)) {
                continue;       // Processor with this index not existing, try next index
            }
            cpu_counter++;

            try {
                std::wostringstream id;
                id << "cpu_stat" << cpu_index;
                inst->GetKstat()->Lookup(L"cpu_stat", id.str(), cpu_index);
                
                const cpu_stat *cpu_stat_p = 0;
                inst->GetKstat()->GetValueRaw(cpu_stat_p);

                pageReads  += cpu_stat_p->cpu_vminfo.pgpgin;
                pageWrites += cpu_stat_p->cpu_vminfo.pgpgout;

            } catch (const SCXKstatException& e) {
                SCX_LOGWARNING(log, std::wstring(L"Kstat failed for memory: ").append(e.What()));
                return false;
            }
        }

#elif defined(hpux) || defined(aix)

        if (!deps->GetPageingData(pageReads, pageWrites))
        {
            return false;
        }

#else
#error "Not implemented on this platform"
#endif

        return true;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Thread body that updates values that are time dependent.

        \param param Must contain a parameter named "ParamValues" of type MemoryInstanceThreadParam*

        The thread updates all members that are time dependent. Like for example
        page reads per second.

    */
    void MemoryInstance::DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.memory.memoryinstance");
        SCX_LOGTRACE(log, L"MemoryInstance::DataAquisitionThreadBody()");

        if (0 == param.GetData())
        {
            SCXASSERT( ! "No parameters to DataAquisitionThreadBody");
            return;
        }
        MemoryInstanceThreadParam* params = static_cast<MemoryInstanceThreadParam*>(param.GetData());

        if (0 == params)
        {
            SCXASSERT( ! "Parameters to DagaAquisitionThreadBody not instance of MemoryInstanceThreadParam");
            return;
        }

        MemoryInstanceDataSampler* pageReadsParam = params->GetPageReads();
        if (0 == pageReadsParam)
        {
            SCXASSERT( ! "PageReads not set");
            return;
        }
        MemoryInstanceDataSampler* pageWritesParam = params->GetPageWrites();
        if (0 == pageWritesParam)
        {
            SCXASSERT( ! "PageWrites not set");
            return;
        }

        SCXCoreLib::SCXHandle<MemoryDependencies> deps = params->GetDeps();

        bool bUpdate = true;
        params->m_cond.SetSleep(MEMORY_SECONDS_PER_SAMPLE * 1000);
        {
            SCXConditionHandle h(params->m_cond);
            while ( ! params->GetTerminateFlag())
            {
                if (bUpdate)
                {
                    scxulong pageReads = 0;
                    scxulong pageWrites = 0;

                    if ( ! GetPagingSinceBoot(pageReads, pageWrites, params->GetInst(), deps))
                    {
                        return;
                    }

                    pageReadsParam->AddSample(pageReads);
                    pageWritesParam->AddSample(pageWrites);
                    bUpdate = false;
                }

                enum SCXCondition::eConditionResult r = h.Wait();
                if (SCXCondition::eCondTimeout == r)
                {
                    bUpdate = true;
                }
            }
        }
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
