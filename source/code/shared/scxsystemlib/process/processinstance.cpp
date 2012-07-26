/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       PAL representation of a Process Instance

   \date        07-10-29 15:21:00


*/
/*----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <signal.h>

#if defined(linux)
#include <stdio.h>
#include <sys/stat.h>
#endif

#if defined(sun) || defined(aix)
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#endif

#if defined(hpux)
#include <string.h>
#include <strings.h>
#include <devnm.h>
#endif

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/logsuppressor.h>
#include <scxcorelib/stringaid.h>

#include <scxsystemlib/processinstance.h>
#include <scxsystemlib/scxsysteminfo.h>

#include <string>
#include <fstream>

#if defined(aix)

#if PF_MAJOR < 6
extern "C" int getprocs64 (void *procsinfo, int sizproc, void *fdsinfo, int sizfd, pid_t *index, int count);
#endif // PF_MAJOR < 6

#include <procinfo.h>

extern "C" int getargs(struct procentry64*, int, char*, int);

#endif // defined(aix)

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{
    const wchar_t *ProcessInstance::moduleIdentifier = L"scx.core.common.pal.system.process.processinstance";

    /** Semi-secret flag to bypass checking for root access.
        It enables us to call functions that would otherwise cast an
        exception, but will on the other hand make them return dummy values.
        Can only be used by the unit tests.
    */
    bool ProcessInstance::m_inhibitAccessViolationCheck = false;

    /*----------------------------------------------------------------------------*/
#if defined(linux)
    SCXCoreLib::SCXCalendarTime ProcessInstance::m_system_boot;

    /**
       Sets the boot time member variable.

       Information is read from the file /proc/uptime that contains the
       number of seconds since the system was last rebooted.

       This is specific to Linux and so far only needed on Linux.

       It is called by every instance, but read from file only the first time.
    */
    void ProcessInstance::SetBootTime(void)
    {
        if (!m_system_boot.IsInitialized()) {
            // First read seconds since boot
            double upsec;
            SCXFileHandle f(fopen("/proc/uptime", "r"));
            if (!f.GetFile()) { throw SCXErrnoException(L"fopen", errno, SCXSRCLOCATION); }
            int s = fscanf(f.GetFile(), "%lf", &upsec);
            if (s != 1) {
                throw SCXInternalErrorException(L"Getting wrong number of parameters from "
                                                L"/proc/uptime", SCXSRCLOCATION);
            }
            // Then read current time and subtract time since boot.
            SCXCalendarTime now(SCXCalendarTime::CurrentLocal());
            SCXAmountOfTime delta;
            delta.SetSeconds(upsec);
            m_system_boot = now - delta;
        }
    }

    /** Format for scanf() when reading /proc/#/stat. */
    const char *LinuxProcStat::scanstring =
    " %c %d %d %d %d %d %lu %lu "                        // 3 to 10
    "%lu %lu %lu %lu %lu %ld %ld %ld %ld %*ld "         // 11 to 20
    "%ld %lu %lu %ld %lu %lu %lu %lu %lu %lu "          // 21 to 30
    "%lu %lu %lu %lu %lu %lu %lu %d %d %lu %lu";        // 31 to 41

    /**
     * Reads the /proc/#/stat file.
     *
     * \param filePointer File pointer to an open file.
     * \param filename    Name of the file
     * \returns true if the file was successfully read, or false if it was deleted already
     *
     */
    bool LinuxProcStat::ReadStatFile(FILE *filePointer, const char* filename)
    {
        int nscanned = fscanf(filePointer, "%d", &processId);

        // Test if the file was deleted before we had a chance to read it.
        // This can happen on RedHat, but not on Suse10.
        // On Suse10 the fscanf will return normally, but all values are 0.
        if (ferror(filePointer)) {
            if (errno == ESRCH) {
              // Race condition. This is ok.
              return false;
            }
            int eno = errno;
            throw SCXErrnoException(L"fscanf", eno, SCXSRCLOCATION);
        }

        if (nscanned != 1) {
            wostringstream errtxt;
            errtxt << L"Getting wrong number of parameters from " << StrFromMultibyte(filename) << L" file. "
                   << L"Expecting 1 but getting " << nscanned << '.';
            throw SCXInternalErrorException(errtxt.str(), SCXSRCLOCATION);
        }

        int ch = 0;
        int pos = 0;
        bool start = false;
        // get process name to comm member variable
        while (ch != ')' && !feof(filePointer) && pos < 29)
        {
            ch = getc(filePointer);
            if (start && ch != ')')
            {
                command[pos++] = static_cast<char>(ch);
            } 
            else if (ch == '(')
            {
                start = true;
            }
        }
        command[pos] = 0;

        nscanned = fscanf(filePointer, scanstring,
                          &state, &parentProcessId, &processGroupId,
                          &sessionId, &controllingTty, &terminalProcessId, &flags, &minorFaults,
                          &childMinorFaults, &majorFaults, &childMajorFaults, &userTime, &systemTime,
                          &childUserTime, &childSystemTime, &priority, &nice, &intervalTimerValue,
                          &startTime, &virtualMemSizeBytes, &residentSetSize, &residentSetSizeLimit,
                          &startAddress, &endAddress, &startStackAddress, &kernelStackPointer, 
                          &kernelInstructionPointer, &signal, &blocked, &sigignore, &sigcatch, 
                          &waitChannel, &numPagesSwapped, &cumNumPagesSwapped, &exitSignal, 
                          &processorNum, &realTimePriority, &schedulingPolicy);

        // Test if the file was deleted before we had a chance to read it.
        // This can happen on RedHat, but not on Suse10.
        // On Suse10 the fscanf will return normally, but all values are 0.
        if (ferror(filePointer)) {
            if (errno == ESRCH) {
              // Race condition. This is ok.
              return false;
            }
            int eno = errno;
            throw SCXErrnoException(L"fscanf", eno, SCXSRCLOCATION);
        }

        // -2 since we read pid and name separatly
        if (nscanned != procstat_len-2) {
            wostringstream errtxt;
            errtxt << L"Getting wrong number of parameters from " << StrFromMultibyte(filename) << L" file. "
                   << L"Expecting " << procstat_len-2 << " but getting " << nscanned << '.';
            throw SCXInternalErrorException(errtxt.str(), SCXSRCLOCATION);
        }

        return true;
    }

    /** Format for scanf() when reading /proc/#/statm. */
    const char *LinuxProcStatM::scanstring = "%lu %lu %lu %lu %lu %lu";

    /**
     * Reads the /proc/#/stat file.
     *
     * \param filePointer File pointer to an open file.
     * \param filename    Name of the file
     * \returns true if the file was successfully read, or false if it was deleted already
     *
     */
    bool LinuxProcStatM::ReadStatMFile(FILE *filePointer, const char* filename)
    {
        int nscanned = fscanf(filePointer, scanstring,
                              &size, &resident, &share, &text, &lib, &data);

        // Test if the file was deleted before we had a chance to read it.
        // This can happen on RedHat, but not on Suse10.
        // On Suse10 the fscanf will return normally, but all values are 0.
        if (ferror(filePointer)) {
            if (errno == ESRCH) {
              // Race condition. This is ok.
              return false;
            }
            int eno = errno;
            throw SCXErrnoException(L"fscanf", eno, SCXSRCLOCATION);
        }

        // If ALL values are zero then assume that the process has died.
        // This is very ad-hoc, but this behaviour has been observed on Suse10,
        // and it is the last chance to avoid getting false data into the system
        if (size + resident + share + text + lib + data == 0) 
        { 
            return false; 
        }

        if (nscanned != procstat_len) {
            wostringstream errtxt;
            errtxt << L"Getting wrong number of parameters from " << StrFromMultibyte(filename) << L" file. "
                   << L"Expecting " << procstat_len << " but getting " << nscanned << '.';
            throw SCXInternalErrorException(errtxt.str(), SCXSRCLOCATION);
        }
        return true;
    }

    /**
     * Constructor for Linux.
     *
     * \param pid Process number for this process
     * \param basename Directory name in /proc where this instance resides
     *
     * Creates a new process instance without any content.
     * This constructor is declared private since it can only be used by
     * the ProcessEnumerator class.
     */
    ProcessInstance::ProcessInstance(scxpid_t pid, const char* basename) :
        EntityInstance(false), m_pid(pid), m_found(true), m_accessViolationEncountered(false),
        m_uid(0), m_gid(0), m_delta_UserTime(0), m_delta_SystemTime(0),
        m_delta_HardPageFaults(0)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGHYSTERICAL(m_log, L"ProcessInstance constructor");

        /* Rememeber files that we read regularly */
        snprintf(m_procStatName,  sizeof(m_procStatName),  "/proc/%s/stat",  basename);
        snprintf(m_procStatMName, sizeof(m_procStatMName), "/proc/%s/statm", basename);

        // The instance id m_Id is of type wstring. (Old relic, I'm told)
        SetId(StrFrom(m_pid));

        // Nullify process name and pid
        m.command[0] = '\0';
        m.processId = 0;

        /* Set clock frequency to proper value.
           "Jiffies" is a measure of frequency that many times reported by the system
           are measured in. It's the system clock. The _SC_CLK_TCK sysconf call
           also reports the system clock frequency. We can't find an explicit
           statement in the documentation that the two are the same, but on our
           system that is the case.
        */
        long retval = sysconf(_SC_CLK_TCK);
        if (retval < 0) {
            throw SCXErrnoException(L"sysconf", errno, SCXSRCLOCATION);
        }
        m_jiffies_per_second = static_cast<unsigned int>(retval);
        // The jiffies are important. If they are wrong, many values are wrong
        if (m_jiffies_per_second == 0) {
            SCXInvalidArgumentException(L"_SC_CLK_TCK",
                                        L"System clock not available from sysconf()",
                                        SCXSRCLOCATION);
        }

        m_timeOfDeath.tv_sec = 0; m_timeOfDeath.tv_usec = 0;
        m_delta_RealTime.tv_sec = 0; m_delta_RealTime.tv_usec = 0;
    }

    /**
     * Updates instance to reflect current status.
     *
     * \param basename Not needed but yet required
     * \param initial If this is a newly discovered process
     *
     * \returns true If successful, or false if it was deleted during update
     *
     * This is a private method that does the low-level updating of the
     * system specific data structure from an external source. In the Linux
     * case this is various files under /proc/#/.
     *
     */
    bool ProcessInstance::UpdateInstance(const char*, bool initial)
    {
        struct stat statbuf;
        bool found = false;

        SCXFileHandle f(fopen(m_procStatName, "r"));
        if (!f.GetFile()) {
            // If process is currently being removed, we can get spurious EBADF/EINVAL errors
            if (ENOENT == errno || EBADF == errno || EINVAL == errno) { m_found = false; return false; }
            throw SCXErrnoException(L"fopen", errno, SCXSRCLOCATION);
        }

        if (initial) {
            /* Do an fstat on the open file to find out uid and gid of owner. */
            if (fstat(fileno_unlocked(f.GetFile()), &statbuf) < 0) {
                throw SCXErrnoException(L"fstat", errno, SCXSRCLOCATION); }
            m_uid = statbuf.st_uid;
            m_gid = statbuf.st_gid;
        }

        found = m.ReadStatFile(f.GetFile(), m_procStatName);

        // test if file was deleted before we had a chance to read it
        if (!found) { m_found = false; return false; }

        if (m.state != 'Z')
        {
            SCXFileHandle f2(fopen(m_procStatMName, "r"));
            if (!f2.GetFile()) {
                // If process is currently being removed, we can get spurious EBADF/EINVAL errors
                if (ENOENT == errno || EBADF == errno || EINVAL == errno) { m_found = false; return false; }
                throw SCXErrnoException(L"fopen", errno, SCXSRCLOCATION);
            }

            found = n.ReadStatMFile(f2.GetFile(), m_procStatMName);
            
            // test if file was deleted before we had a chance to read it
            if (!found) 
            { 
                m_found = false; return false; 
            }
        }

        if (initial) {
            SetBootTime();                      // Executed only once
        }

        m_found = true;                         // Mark as processed
        return true;
    }

    /**
     * Updates all those values that should be sampled at regualar intervals.
     *
     * \param realtime Current time
     *
     * This method is called by local thread at regular intervals. It takes
     * a snapshot of some predefined values and saves them. A time stamp of
     * the current time is also saved.
     *
     * It also checks if the process has become a zombie, i.e. terminated, and
     * in that case we record the time of death.
     */
    void ProcessInstance::UpdateDataSampler(struct timeval& realtime)
    {
        m_RealTime_tics.AddSample(realtime);
        m_UserTime_tics.AddSample(m.userTime);             // Data sampler for user time.
        m_SystemTime_tics.AddSample(m.systemTime);         // Data sampler for system time.
        m_HardPageFaults_tics.AddSample(m.majorFaults);    // Data sampler for hard page faults.

        /* If process has become a zombie, record time of death. */
        if (m_timeOfDeath.tv_sec == 0 && m.state == 'Z') { m_timeOfDeath = realtime; }
    }

    /**
     *  Compute percentage values.
     *
     * This method is indirectly called by provider just before getting
     * individual values. It computes those values that are percentages over
     * time.
     */
    void ProcessInstance::UpdateTimedValues(void)
    {
        // How far we go back for measurement (all the way)
        const size_t go_back = MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES;

        m_delta_RealTime = m_RealTime_tics.GetDelta(go_back);
        m_delta_UserTime = m_UserTime_tics.GetDelta(go_back);
        m_delta_SystemTime = m_SystemTime_tics.GetDelta(go_back);
        m_delta_HardPageFaults = m_HardPageFaults_tics.GetDelta(go_back);
    }

#endif /* linux */

#if defined(sun)
    /**
     * Constructor for Solaris.
     *
     * \param pid Process number for this process
     * \param basename Directory name in /proc where this instance resides
     *
     * Creates a new process instance without any content.
     * This constructor is declared private since it can only be used by
     * the ProcessEnumerator class.
     */
    ProcessInstance::ProcessInstance(scxpid_t pid, const char* basename) :
        EntityInstance(false), m_pid(pid), m_found(true), m_accessViolationEncountered(false),
        m_logged64BitError(false), m_delta_BlockOut(0), m_delta_BlockInp(0), m_delta_HardPageFaults(0)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGTRACE(m_log, L"ProcessInstance constructor");

        snprintf(m_procPsinfoName, sizeof(m_procPsinfoName), "/proc/%s/psinfo", basename);
        snprintf(m_procStatusName, sizeof(m_procStatusName), "/proc/%s/status", basename);
        snprintf(m_procUsageName,  sizeof(m_procUsageName),  "/proc/%s/usage", basename);

        // The instance id m_Id is of type wstring.
        SetId(StrFrom(m_pid));

        // Specifically null out our structures to pacify Purify (reduce noise)
        // (Note: process name and pid must be specificaly nullified)
        memset(&m_psinfo, '\0', sizeof(m_psinfo));
        memset(&m_pstat,  '\0', sizeof(m_pstat));
        memset(&m_puse,   '\0', sizeof(m_puse));

        // Set clock frequency to proper value.
        // Actual value not that important, but must NOT be zero!
        m_clocksPerSecond = sysconf(_SC_CLK_TCK);
        if (m_clocksPerSecond == 0) { m_clocksPerSecond = 100; }

        m_timeOfDeath.tv_sec = 0; m_timeOfDeath.tv_usec = 0;
        m_delta_RealTime.tv_sec = 0; m_delta_RealTime.tv_usec = 0;
        m_delta_UserTime.tv_sec = 0; m_delta_UserTime.tv_nsec = 0;
        m_delta_SystemTime.tv_sec = 0; m_delta_SystemTime.tv_nsec = 0;
    }

    /**
     * Used by UpdateInstance(); reads the process information and loads structure
     *
     * \returns true If process exists; false otherwise
     * \throws SCXErrnoException If unexpected error occurs
     * \throws SCXInternalErrorException If number of bytes doesn't match structure size
     */
    bool ProcessInstance::ReadProcessInfo()
    {
        int saved_errno;
        int file;
        ssize_t bytes;

        file = open(m_procPsinfoName, O_RDONLY);
        if (file == -1) {
            // If process is currently being removed, we can get spurious EBADF/EINVAL errors
            if (ENOENT == errno || EBADF == errno || EINVAL == errno) { m_found = false; return false; }
            throw SCXErrnoException(L"open", errno, SCXSRCLOCATION);
        }
        bytes = read(file, &m_psinfo, sizeof(m_psinfo));
        saved_errno = errno;
        close(file);
        if (bytes < 0) { 
            // Test if /proc/#/psinfo was deleted already.
            if (ENOENT == saved_errno || EBADF == saved_errno || EINVAL == saved_errno) { m_found = false; return false; }
            throw SCXErrnoException(L"read", saved_errno, SCXSRCLOCATION); 
        }
        if (bytes != sizeof(m_psinfo)) {
            throw SCXInternalErrorException(L"Getting wrong size when reading parameters from "
                                            L"/proc/#/psinfo file", SCXSRCLOCATION);
        }

        return true;
    }

    /**
     * Used by UpdateInstance(); reads the process usage and loads structure
     *
     * \returns true If process exists; false otherwise
     * \throws SCXErrnoException If unexpected error occurs
     * \throws SCXInternalErrorException If number of bytes doesn't match structure size
     */
    bool ProcessInstance::ReadUsageInfo()
    {
        int saved_errno;
        int file;
        ssize_t bytes;

        file = open(m_procUsageName, O_RDONLY);
        if (file == -1) {
            // If process is currently being removed, we can get spurious EBADF/EINVAL errors
            if (ENOENT == errno || EBADF == errno || EINVAL == errno) { m_found = false; return false; }
            throw SCXErrnoException(L"open", errno, SCXSRCLOCATION);
        }
        bytes = read(file, &m_puse, sizeof(m_puse));
        saved_errno = errno;
        close(file);
        if (bytes < 0) { 
            // Test if /proc/#/usage was deleted already.
            if (ENOENT == saved_errno || EBADF == saved_errno || EINVAL == saved_errno) { m_found = false; return false; }
            throw SCXErrnoException(L"read", errno, SCXSRCLOCATION); 
        }
        if (bytes != sizeof(m_puse)) {
            throw SCXInternalErrorException(L"Getting wrong size when reading parameters from "
                                            L"/proc/#/usage file", SCXSRCLOCATION);
        }

        return true;
    }

    /**
     * Used by UpdateInstance(); reads the process status information and loads structure
     *
     * \returns true If process exists; false otherwise
     * \throws SCXErrnoException If unexpected error occurs
     * \throws SCXInternalErrorException If number of bytes doesn't match structure size
     */
    bool ProcessInstance::ReadStatusInfo()
    {
        int saved_errno;
        int file;
        ssize_t bytes;

        /* This file is special since it requires root access for most processes */
        file = open(m_procStatusName, O_RDONLY);
        if (file == -1) {
            // File gone -> process gone
            // If process is currently being removed, we can get spurious EBADF/EINVAL errors
            // WI11727: open() may set errno to EAGAIN if file temporarily locked by the kernel
            // during process creation. The same may apply to process destruction.
            if (ENOENT == errno || EBADF == errno || EINVAL == errno || EAGAIN == errno) { 
                m_found = false; return false; 
            }
            if (EACCES == errno) {
                // Insufficient rights. Rememeber that until we access depended values.
                m_accessViolationEncountered = true;
                // In case we chose to ignore this problem we get dummy values instead.
                m_pstat.pr_brksize = 2000000;
                m_pstat.pr_stksize = 500000;
                m_pstat.pr_cstime.tv_nsec = 666666;
                m_pstat.pr_cstime.tv_sec = 10;
                m_pstat.pr_cutime.tv_nsec = 123456;
                m_pstat.pr_cutime.tv_sec = 33;
            } else { throw SCXErrnoException(L"open", errno, SCXSRCLOCATION); }
        } else {
            // We're OK to read
            bytes = read(file, &m_pstat, sizeof(m_pstat));
            saved_errno = errno;
            close(file);
            if (bytes < 0)
            {
                // Test if /proc/#/status was deleted already.
                if (ENOENT == saved_errno || EBADF == saved_errno || EINVAL == saved_errno) { m_found = false; return false; }

                // Trying to get info for a 64-bit process from a 32-bit can not be done
                if (EOVERFLOW == saved_errno)
                {
                    if (!m_logged64BitError)
                    {
                        SCX_LOGWARNING(m_log, StrAppend(StrAppend(StrAppend(L"No data can be gathered for 64-bit process : ", m_psinfo.pr_fname), L" - "), m_psinfo.pr_pid));
                        m_logged64BitError = true;
                    }
                    // Setting all values to 0 to get 0 for used memory and CPU
                    m_pstat.pr_brksize = 0;
                    m_pstat.pr_stksize = 0;
                    m_pstat.pr_cstime.tv_nsec = 0;
                    m_pstat.pr_cstime.tv_sec = 0;
                    m_pstat.pr_cutime.tv_nsec = 0;
                    m_pstat.pr_cutime.tv_sec = 0;
                }
                else
                {
                    throw SCXErrnoException(StrAppend(StrFromMultibyte(m_procStatusName), L" read"), saved_errno, SCXSRCLOCATION);
                }
            }
            else if (bytes != sizeof(m_pstat)) {
                throw SCXInternalErrorException(L"Getting wrong size when reading parameters "
                                                L"from /proc/#/status file", SCXSRCLOCATION);
            }
        }

        return true;
    }

    /**
     * Used by UpdateInstance(); determines if we're running in the global zone or not.
     * This method is overridden during test.
     *
     * \returns true If we're running in the global zone, false otherwise
     */
    bool ProcessInstance::isInGlobalZone()
    {
        SystemInfo si;
        bool fIsInGlobalZone;
        si.GetSUN_IsInGlobalZone( fIsInGlobalZone );

        return fIsInGlobalZone;
    }

    /**
     * Updates instance to reflect current status.
     *
     * Not used: basename Not needed but yet required
     * Not used: inital If this is a newly discovered process
     *
     * \returns true If successful, or false if it was deleted during update
     *
     * This is a protected method that does the low-level updating of the
     * system specific data structure from an external source. In the Solaris
     * case this is various files under /proc/#/.
     *
     */
    bool ProcessInstance::UpdateInstance(const char*, bool)
    {
        if (! ReadProcessInfo())
        {
            return false;
        }

#if (PF_MAJOR > 5) || (PF_MAJOR == 5 && PF_MINOR >= 10)
        // If we're in the global zone, filter out processes that aren't in global zone
        // Note: We chould check m_pstat.pr_zoneid, but the process status file doesn't
        //       exist for zombies.  By checking process info file, we can assure that
        //       zombies are indicated in the correct zone.

        if (isInGlobalZone() && 0 != m_psinfo.pr_zoneid)
        {
            m_found = false;
            return false;
        }
#endif

        if (! ReadUsageInfo())
        {
            return false;
        }

        // If we're a zombie, then the process status file no longer exists ...

        if (m_psinfo.pr_lwp.pr_sname != 'Z' && (! ReadStatusInfo()))
        {
            return false;
        }

        m_found = true;                         // Mark as processed
        return true;                            // All was read. Process still exists.
    }

    /**
     * Updates all those values that should be sampled at regualar intervals.
     *
     * \param realtime Current time
     *
     * This method is called by local thread at regular intervals. It takes
     * a snapshot of some predefined values and saves them. A time stamp of
     * the current time is also saved.
     *
     * It also checks if the process has become a zombie, i.e. terminated, and
     * in that case we record the time of death.
     */
    void ProcessInstance::UpdateDataSampler(struct timeval& realtime)
    {
        // Note: We tried to use pr_rtime to collect the elapsed time.
        //  That was very bad! Turns out that pr_rtime is the sum of elapsed
        //  time for all threads that run in the process. So, for a 10 thread
        //  process this number is updated by 10s per real second. Weird stuff!
        // Note2: The total CPU time is also available in m_psinfo.pr_time.tv_sec,
        //  but we sum system and user time instead.

        m_RealTime_tics.AddSample(realtime);
        m_UserTime_tics.AddSample(m_puse.pr_utime);     // User time
        m_SystemTime_tics.AddSample(m_puse.pr_stime);   // System time

        m_BlockOut_tics.AddSample(m_puse.pr_oublk);     // Written blocks
        m_BlockInp_tics.AddSample(m_puse.pr_inblk);     // Read blocks
        m_HardPageFaults_tics.AddSample(m_puse.pr_majf);// Data sampler for hard page faults.

        /* If process has become a zombie, record time of death.
           On Sun, there is a better resoltion termination time of available in
           puse.pr_term but when we read it, it's just junk.
        */
        if (m_timeOfDeath.tv_sec == 0 && m_psinfo.pr_lwp.pr_sname == 'Z') 
        { 
            m_timeOfDeath = realtime; 
        }
    }

    /**
     * Compute percentage values.
     *
     * This method is indirectly called by provider just before getting
     * individual values. It computes those values that are percentages over
     * time.
     */
    void ProcessInstance::UpdateTimedValues(void)
    {
        // How far we go back for measurement (all the way)
        const size_t go_back = MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES;

        m_delta_RealTime = m_RealTime_tics.GetDelta(go_back);
        m_delta_UserTime = m_UserTime_tics.GetDelta(go_back);
        m_delta_SystemTime = m_SystemTime_tics.GetDelta(go_back);

        m_delta_BlockOut = m_BlockOut_tics.GetDelta(go_back);
        m_delta_BlockInp = m_BlockInp_tics.GetDelta(go_back);
        m_delta_HardPageFaults = m_HardPageFaults_tics.GetDelta(go_back);
    }

#endif /* sun */

#if defined(hpux)
    /**
     * Constructor for HP/UX
     *
     * \param pid Process number for this process
     * \param pstatus Struct with info about this instance. Not needed.
     *
     * Creates a new process instance without any content.
     * This constructor is declared private since it can only be used by
     * the ProcessEnumerator class.
     */
    ProcessInstance::ProcessInstance(scxpid_t pid, struct pst_status *) :
        EntityInstance(false), m_pid(pid), m_found(true), m_accessViolationEncountered(false),
        m_delta_UserTime(0), m_delta_SystemTime(0), m_delta_BlockOut(0),
        m_delta_BlockInp(0), m_delta_HardPageFaults(0)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGTRACE(m_log, L"ProcessInstance constructor");

        // The instance id m_Id is of type wstring.
        SetId(StrFrom(m_pid));

        // Nullify process name and pid
        m_pstatus.pst_ucomm[0] = '\0';
        m_pstatus.pst_pid = 0;

        m_timeOfDeath.tv_sec = 0; m_timeOfDeath.tv_usec = 0;
        m_delta_RealTime.tv_sec = 0; m_delta_RealTime.tv_usec = 0;
    }

    /**
     * Updates instance to reflect current status.
     *
     * \param pstatus Struct with new info about this instance
     * \param inital If this is a newly discovered process
     *
     * \returns true always on HPUX
     *
     * This is a private method that does the low-level updating of the
     * system specific data structure from an external source. On HPUX
     * this is a structure that was read with the system call pst_status().
     */
    bool ProcessInstance::UpdateInstance(struct pst_status *pstatus, bool)
    {
        // Save the full structure so that we can pick values on demand
        memcpy(&m_pstatus, pstatus, sizeof(struct pst_status));

        m_found = true;                         // Mark as processed
        return true;
    }

    /**
     * Updates all those values that should be sampled at regualar intervals.
     *
     * \param realtime Current time
     *
     * This method is called by local thread at regular intervals. It takes
     * a snapshot of some predefined values and saves them. A time stamp of
     * the current time is also saved.
     *
     * It also checks if the process has become a zombie, i.e. terminated, and
     * in that case we record the time of death.
     */
    void ProcessInstance::UpdateDataSampler(struct timeval& realtime)
    {
        m_RealTime_tics.AddSample(realtime);
        m_UserTime_tics.AddSample(m_pstatus.pst_utime);            // User time (seconds).
        m_SystemTime_tics.AddSample(m_pstatus.pst_stime);          // System time (seconds).

        // Alt. implementation. May be used in the future
        //m_CPUTime_tics.AddSample(m_pstatus.pst_cpticks);           // Ticks of Cpu time
        //m_CPUTime_total_tics.AddSample(m_pstatus.pst_cptickstotal);// Total Cpu ticks (Not entirely sure what this is)

        m_BlockOut_tics.AddSample(m_pstatus.pst_oublock);          // Written blocks
        m_BlockInp_tics.AddSample(m_pstatus.pst_inblock);          // Read blocks
        m_HardPageFaults_tics.AddSample(m_pstatus.pst_majorfaults);// Hard page faults.

        /* If process has become a zombie, record time of death. */
        if (m_timeOfDeath.tv_sec == 0 && m_pstatus.pst_stat == PS_ZOMBIE) { m_timeOfDeath = realtime; }
    }

    /**
     * Compute percentage values.
     *
     * This method is indirectly called by provider just before getting
     * individual values. It computes those values that are percentages over
     * time.
     */
    void ProcessInstance::UpdateTimedValues(void)
    {
        // How far we go back for measurement (all the way)
        const size_t go_back = MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES;

        m_delta_RealTime = m_RealTime_tics.GetDelta(go_back);
        m_delta_UserTime = m_UserTime_tics.GetDelta(go_back);
        m_delta_SystemTime = m_SystemTime_tics.GetDelta(go_back);

        // Alt. implementation. May be used in the future
        //m_delta_CPUTime_total = m_CPUTime_total_tics.GetDelta(go_back);
        //m_delta_CPUTime = m_CPUTime_tics.GetDelta(go_back);

        m_delta_BlockOut = m_BlockOut_tics.GetDelta(go_back);
        m_delta_BlockInp = m_BlockInp_tics.GetDelta(go_back);
        m_delta_HardPageFaults = m_HardPageFaults_tics.GetDelta(go_back);
    }

#endif /* hpux */

#if defined(aix)
    namespace { // put it in anonymous namespace
    /** 
    * Funciotn checks if process is zombie;
    * based on "man /proc" 
    *           The pr_lwp flag describes the representative thread chosen. 
    *           If the process is a zombie, the pr_nlwp and pr_lwp.pr_lwpid flags are zero 
    *           and the other fields of pr_lwp are undefined. 
    */
        static inline bool IsZombie( const psinfo_t& psinfo)
        {
            return psinfo.pr_nlwp == 0 &&
                psinfo.pr_lwp.pr_lwpid == 0;
        }
    }

    /**
     * Constructor for AIX.
     *
     * \param pid Process number for this process
     * \param basename Directory name in /proc where this instance resides
     *
     * Creates a new process instance without any content.
     * This constructor is declared private since it can only be used by
     * the ProcessEnumerator class.
     */
    ProcessInstance::ProcessInstance(scxpid_t pid, const char* basename) :
        EntityInstance(false), m_pid(pid), m_found(true), m_accessViolationEncountered(false)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGTRACE(m_log, L"ProcessInstance constructor");

        snprintf(m_procPsinfoName, sizeof(m_procPsinfoName), "/proc/%s/psinfo", basename);
        snprintf(m_procStatusName, sizeof(m_procStatusName), "/proc/%s/status", basename);

        memset(&m_procentry, 0, sizeof(m_procentry));
        // The instance id m_Id is of type wstring.
        SetId(StrFrom(m_pid));

        // Nullify process name and pid
        m_psinfo.pr_fname[0] = '\0';
        m_psinfo.pr_pid = 0;

        // Set clock frequency to proper value.
        // Actual value not that important, but must NOT be zero!
        m_clocksPerSecond = sysconf(_SC_CLK_TCK);
        if (m_clocksPerSecond == 0) { m_clocksPerSecond = 100; }

        m_timeOfDeath.tv_sec = 0; m_timeOfDeath.tv_usec = 0;
        m_delta_RealTime.tv_sec = 0; m_delta_RealTime.tv_usec = 0;
        m_delta_UserTime.tv_sec = 0; m_delta_UserTime.tv_nsec = 0;
        m_delta_SystemTime.tv_sec = 0; m_delta_SystemTime.tv_nsec = 0;
    }

    /**
     * Updates instance to reflect current status.
     *
     * \param basename Not needed but yet required
     * \param inital If this is a newly discovered process
     *
     * \returns true If successful, or false if it was deleted during update
     *
     * This is a private method that does the low-level updating of the
     * system specific data structure from an external source. In the AIX
     * case this is various files under /proc/#/.
     *
     */
    bool ProcessInstance::UpdateInstance(const char*, bool)
    {
        static bool lastReadWasEbusy = false;
        int saved_errno;

        pid_t firstproc = m_pid;
        struct procentry64 pe;

        if (1 != getprocs64(&pe, sizeof(pe), 0, 0, &firstproc, 1)) {
            m_found = false;
            return false;
        }

        // Copy fields to ProcEntry structure.
        memset(&m_procentry, 0, sizeof(m_procentry));
        m_procentry.pi_pri = pe.pi_pri;
        m_procentry.pi_nice = pe.pi_nice;

        int file = open(m_procPsinfoName, O_RDONLY);
        if (file == -1) {
            // If process is currently being removed, we can get spurious EBADF/EINVAL errors
            if (ENOENT == errno || EBADF == errno || EINVAL == errno) { m_found = false; return false; }
            throw SCXErrnoException(L"open", errno, SCXSRCLOCATION);
        }
        ssize_t bytes = read(file, &m_psinfo, sizeof(m_psinfo));
        saved_errno = errno;
        close(file);
        if (bytes < 0) { 
            // Test if /proc/#/psinfo was deleted already.
            if (ENOENT == saved_errno || EBADF == saved_errno || EINVAL == saved_errno) { m_found = false; return false; }
            throw SCXErrnoException(L"read", saved_errno, SCXSRCLOCATION); 
        }
        if (bytes != sizeof(m_psinfo)) {
            throw SCXInternalErrorException(L"Getting wrong size when reading parameters from "
                                            L"/proc/#/psinfo file", SCXSRCLOCATION);
        }

        if ( !IsZombie(m_psinfo) ) {
            /* This file is special since it requires root access for most processes */
            file = open(m_procStatusName, O_RDONLY);
            if (file == -1) {
                // File gone -> process gone
                // If process is currently being removed, we can get spurious EBADF/EINVAL errors
                if (ENOENT == errno || EBADF == errno || EINVAL == errno) {
                    m_found = false; 
                    return false; 
                }
                if (EACCES == errno) {
                    // Insufficient rights. Remember that until we access dependent values.
                    m_accessViolationEncountered = true;
                    // In case we chose to ignore this problem we get dummy values instead.
                    m_pstat.pr_brksize = 2000000;
                    m_pstat.pr_stksize = 500000;
                    m_pstat.pr_cstime.tv_nsec = 666666;
                    m_pstat.pr_cstime.tv_sec = 10;
                    m_pstat.pr_cutime.tv_nsec = 123456;
                    m_pstat.pr_cutime.tv_sec = 33;
                } else if (EBUSY == errno) {
                    // We don't know the exact time this could happen.
                    // There is nothing in the Man pages.
                    // For now we will log at INFO first time and WARNING
                    // after that. Hopefully this is harmless and we just
                    // miss one snapshot. WI 7459.
                    if (lastReadWasEbusy)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Got errno=EBUSY from open at least 2 consecutive times on file ")
                                       .append(StrFromMultibyte(m_procStatusName)));
                    } else {
                        SCX_LOGTRACE(m_log, std::wstring(L"Got errno=EBUSY from open. File: ")
                                                                         .append(StrFromMultibyte(m_procStatusName)));
                    }
                    lastReadWasEbusy = true;
                } else {
                    throw SCXErrnoException(L"open", errno, SCXSRCLOCATION);
                }
            } else {
                lastReadWasEbusy = false;

                // We're OK to read
                {
                    pstatus_t tmp;
                    memset(&tmp, 0, sizeof(pstatus_t));
                    bytes = read(file, &tmp, sizeof(pstatus_t));

                    // Copy into smaller structure:
                    memset(&m_pstat, 0, sizeof(m_pstat));
                    m_pstat.pr_brksize = tmp.pr_brksize;
                    m_pstat.pr_stksize = tmp.pr_stksize;
                    m_pstat.pr_cstime = tmp.pr_cstime;
                    m_pstat.pr_cutime = tmp.pr_cutime;
                    m_pstat.pr_utime = tmp.pr_utime;
                    m_pstat.pr_stime = tmp.pr_stime;
                }

                saved_errno = errno;
                close(file);
                if (bytes < 0)
                {
                    // Test if /proc/#/status was deleted already. (Could mean we're a zombie.)
                    // If process is currently being removed, we can get spurious EBADF/EINVAL errors
                    if (ENOENT == saved_errno || EBADF == saved_errno || EINVAL == saved_errno)
                    {
                        m_found = false; return false;
                    }

                    throw SCXErrnoException(StrAppend(StrFromMultibyte(m_procStatusName), 
                                                      L" read"), saved_errno, SCXSRCLOCATION);
                }
                else if (bytes != sizeof(pstatus_t)) {
                    throw SCXInternalErrorException(L"Getting wrong size when reading parameters "
                                                    L"from /proc/#/status file", SCXSRCLOCATION);
                }
            }
        }

        m_found = true;                         // Mark as processed
        return true;                            // All was read. Process still exists.
    }

    /**
     * Updates all those values that should be sampled at regualar intervals.
     *
     * \param realtime Current time
     *
     * This method is called by local thread at regular intervals. It takes
     * a snapshot of some predefined values and saves them. A time stamp of
     * the current time is also saved.
     *
     * It also checks if the process has become a zombie, i.e. terminated, and
     * in that case we record the time of death.
     */
    void ProcessInstance::UpdateDataSampler(struct timeval& realtime)
    {
        m_RealTime_tics.AddSample(realtime);
        /* Please note: Reading these times requires root access on AIX. */
        m_UserTime_tics.AddSample(m_pstat.pr_utime);     // User time
        m_SystemTime_tics.AddSample(m_pstat.pr_stime);   // System time

        // This is the total time but we can skip it since it's just the u+s sum
        //m_TotalTime_tics.AddSample(m_pbuf.pr_time);   // System time

        /* If process has become a zombie, record time of death. */
        if (m_timeOfDeath.tv_sec == 0 && IsZombie(m_psinfo) ) { m_timeOfDeath = realtime; }
    }

    /**
     * Compute percentage values.
     *
     * This method is indirectly called by provider just before getting
     * individual values. It computes those values that are percentages over
     * time.
     */
    void ProcessInstance::UpdateTimedValues(void)
    {
        // How far we go back for measurement (all the way)
        const size_t go_back = MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES;

        m_delta_RealTime = m_RealTime_tics.GetDelta(go_back);
        m_delta_UserTime = m_UserTime_tics.GetDelta(go_back);
        m_delta_SystemTime = m_SystemTime_tics.GetDelta(go_back);

        // This is the total time but we can skip it since it's just the u+s sum
        //m_delta_TotalTime = m_TotalTime_tics.GetDelta(go_back);
    }

#endif /* aix */

    /*----------------------------------------------------------------------------*/
    /**
     * Destructor for process instance.
     *
     */
    ProcessInstance::~ProcessInstance()
    {
    }

    /**
     * Tests that we are running with the proper privileges.
     *
     * \throws SCXAccessViolation if not root or corresponding
     */
    void ProcessInstance::CheckRootAccess(void) const
    {
        // Check if the secret backdoor is open
        if (m_inhibitAccessViolationCheck) { return; }

        // We could check that geteuid() is root, but instead we have a
        // flag that indicates that there has been a problem encountered in this instance
        if (m_accessViolationEncountered) {
            throw SCXAccessViolationException(L"Root access is required", SCXSRCLOCATION);
        }
    }

    /*====================================================================================*/
    /* Properties of SCX_UnixProcess                                                      */
    /*====================================================================================*/

    /**
       Gets the process identifier.

       \param[out]  pid process identification number
       \returns     true if a value is supported by the implementation

       Correponds to syscall getpid().
    */
    bool ProcessInstance::GetPID(scxulong& pid) const
    {
#if defined(linux)
        pid = m.processId;
        SCXASSERT(pid == m_pid);
        return true;
#elif defined(sun) || defined(aix)
        pid = m_psinfo.pr_pid;
        SCXASSERT(pid == m_pid);
        return true;
#elif defined(hpux)
        pid = m_pstatus.pst_pid;
        SCXASSERT(pid == m_pid);
        return true;
#else
        return false;
#endif
    }


#if defined(aix) || defined(hpux) || defined(sun)
    /**
       Strips path information from the filename passed in by parameter.
       If no path information is found, then no modifications are made.

       Examples of input (and results) from this function:
           Input:  ~/../binary-file
           Output: binary-file

           Input:  /usr/bin/sleep
           Output: sleep

           Input:  sleep
           Output: sleep


       \param[out]  name Return parameter for the command name
       \returns     void
     */
    static void StripPathInfo(std::string& name)
    {
        size_t pos = name.rfind('/');
        if (pos != string::npos)
        {
            name.erase(0, pos + 1);
        }
    }
#endif


    /**
       Gets the name of the command that is executing in this process instance.

       \param[out]  name Return parameter for the command name
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The name of the process."
       By convention, we return the name of the executing process without
       parameters.

       If the process is a zombie ("Terminated" state, not yet reaped), then the
       process name varies by platform:
           AIX:                   \<defunct\>
           HP (Itanium & PARISC): \<defunct\>
           Linux (Redhat & Suse): [process-name] \<defunct\>
           Mac O/S:               (process-name)
           Solaris (x86 & Sparc): \<defunct\>
       The idea is to have consistency with the 'ps' utility
    */
    bool ProcessInstance::GetName(std::string& name) const
    {
        const unsigned short Terminated = 7;
        unsigned short state = 0;

#if defined(linux)
        name = m.command;

        if (GetExecutionState(state) && state == Terminated)
        {
            name = "[" + name + "] <defunct>";
        }

        return true;
#elif defined(sun)
        // WI 27724: Consistent output when starting process from a symlink
        //
        // Solaris normally returns process name in 'm_psinfo.pr_fname', but
        // this field resolves a symbolic link, and we don't want that.  So
        // look at the parameter list instead and use it if non-blank.

        std::vector<std::string> params;
        name.clear();

        // If we can get process parameters, use argv[0] as our process name
        if (GetParameters(params) && params.size())
        {
            name = params.front();
            StripPathInfo(name);
        }

        // If we didn't pick up any information from parameters, fall back
        if (name.empty())
        {
            // This can happen routinely on Solaris, so don't ASSERT here!
            // In particular, defunct processes have no name at all ...
            name = m_psinfo.pr_fname;
        }

        // Handle terminated defunct processes
        if (GetExecutionState(state) && state == Terminated)
        {
            name = "<defunct>";
        }

        return true;
#elif defined(hpux)
        // WI 27724: Consistent output when starting process from a symlink
        //
        // HP normally returns process name in 'm_pstatus.pst_ucomm', but
        // this field resolves a symbolic link, and we don't want that.  So
        // look at the parameter list instead and use it if non-blank.

        std::vector<std::string> params;
        name.clear();

        // If we can get process parameters, use argv[0] as our process name
        if (GetParameters(params) && params.size())
        {
            name = params.front();
            StripPathInfo(name);
        }

        // If we didn't pick up any information from parameters, fall back
        if (name.empty())
        {
            wostringstream wss;
            wss << L"No parameters retrieved for process name; falling back to pr_fname for process: " << m_pid;
            SCXASSERTFAIL(wss.str().c_str());

            name = m_pstatus.pst_ucomm;
        }

        // Handle terminated defunct processes
        if (GetExecutionState(state) && state == Terminated)
        {
            name = "<defunct>";
        }

        return true;
#elif defined(aix)
        // WI 27724: Consistent output when starting process from a symlink
        //
        // AIX normally returns process name in 'm_psinfo.pr_fname', but
        // this field resolves a symbolic link, and we don't want that.  So
        // look at the parameter list instead and use it if non-blank.

        std::vector<std::string> params;
        name.clear();

        // If we can get process parameters, use argv[0] as our process name
        if (GetParameters(params) && params.size())
        {
            name = params.front();
            StripPathInfo(name);
        }

        // If we didn't pick up any information from parameters, fall back
        if (name.empty())
        {
            // This can happen routinely on AIX, so don't ASSERT here!
            // AIX returns an empty name for pid 0. But 'ps' identifies it as swapper and so do we
            if (m_pid == 0) {
                name = "swapper";
            } else {
                name = m_psinfo.pr_fname;
            }
        }

        if (GetExecutionState(state) && state == Terminated)
        {
            name = "<defunct>";
        }

        return true;
#else
        return false;
#endif
    }


    /**
       Gets the priority of this process instance.

       \param[out]  prio Return parameter for the priority
       \returns     true if this value is supported by the implementation

       According to the CIM model: "Priority indicates the urgency or importance
       of execution of a Process. Lower values reflect more favorable process scheduling."

       Most modern UNIX versions have scheduling mechanisms that can't reflect
       process priority as a single number. It is also not clear if we fullfill
       the requirement that a lower number is higher priority. In other words, don't
       attach too much meaning to this number.
    */
    bool ProcessInstance::GetPriority(unsigned int& prio) const
    {
#if defined(linux)
        /* By chance we found out that 'ps' adds 60 to priority field. */
        prio = static_cast<unsigned int>(m.priority + 60);
        return true;
#elif defined(aix)
        prio = m_procentry.pi_pri;
        return true;
#elif defined(sun)
        prio = m_psinfo.pr_lwp.pr_pri;
        return true;
#elif defined(hpux)
        prio = m_pstatus.pst_pri;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the execution state of this process instance.

       \param[out]  state Return parameter for the state
       \returns     true if this value is supported by the implementation

       According to the CIM model: "Indicates the current operating condition
       of the Process".

       We map the execution state of the process to one of 11 values defined
       in the CIM model. Only a few of these are applicable to a specific
       operating system.
    */
    bool ProcessInstance::GetExecutionState(unsigned short& state) const
    {
        /* This enum will convert to an integer that is consistent with the CIM model */
        enum { Unknown, Other, Ready, Running, Blocked, Suspended_Blocked, Suspended_Ready,
               Terminated, Stopped, Growing, Relinquished, Hung } st;

#if defined(linux)
        switch (m.state) {
        case 'R':       st = Running;           break;
        case 'S':       st = Suspended_Ready;   break;
        case 'D':       st = Suspended_Blocked; break;
        case 'Z':       st = Terminated;        break;
        case 'T':       st = Stopped;           break;
        case 'W':       st = Growing;           break;
        case 'X':       st = Terminated;        break;
        default:        st = Unknown;           break;
        }
        state = st;
        return true;
#elif defined(sun)
        switch (m_psinfo.pr_lwp.pr_sname) {
        case 'O':       st = Running;           break;
        case 'S':       st = Suspended_Ready;   break;
        case 'R':       st = Ready;             break;
        case 'Z':       st = Terminated;        break;
        case 'T':       st = Stopped;           break;
        default:        st = Unknown;           break;
        }
        state = st;
        return true;
#elif defined(hpux)
        switch (m_pstatus.pst_stat) {
        case PS_SLEEP:  st = Suspended_Ready;   break;
        case PS_RUN:    st = Running;           break;
        case PS_STOP:   st = Stopped;           break;
        case PS_ZOMBIE: st = Terminated;        break;
        case PS_IDLE:   st = Ready;             break;
        case PS_OTHER:  st = Other;             break;
        default:        st = Unknown;           break;
        }
        state = st;
        return true;
#elif defined(aix)
        switch (m_psinfo.pr_lwp.pr_sname) {
        case 'O':       st = Other;             break;
        case 'A':       st = Ready;             break;
        case 'R':       st = Running;           break;
        case 'S':       st = Suspended_Ready;   break;
        case 'I':       st = Suspended_Ready;   break;
        case 'W':       st = Suspended_Blocked; break;
        case 'Z':       st = Terminated;        break;
        case 'T':       st = Stopped;           break;
        default:        st = IsZombie(m_psinfo) ? Terminated : Unknown;           break;
        }
        state = st;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the creation date of this process instance.

       \param[out]  creation_date Return parameter for the creation date
       \returns     true if this value is supported by the implementation

       According to the CIM model: "Time that the Process began executing."

       The time that is returned is in the local time zone.
       This value is at best accurate to the nearest second
    */
    bool ProcessInstance::GetCreationDate(SCXCoreLib::SCXCalendarTime& creation_date) const
    {
#if defined(linux)
        SCXAmountOfTime since_start;
        creation_date = m_system_boot;
        since_start.SetSeconds(static_cast<SCXCoreLib::scxseconds>(m.startTime / m_jiffies_per_second));
        creation_date += since_start;
        creation_date.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
        return true;
#elif defined(sun) || defined(aix)
        creation_date = SCXCalendarTime::FromPosixTime(m_psinfo.pr_start.tv_sec);
        SCXAmountOfTime nsec;
        nsec.SetSeconds(m_psinfo.pr_start.tv_nsec / 1000000000.0L);
        creation_date += nsec;
        creation_date.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
        return true;

        // Note: We would like to use the high-resolotion timer in m_puse.pr_create.tv_sec,
        // that won't work. It would look like this:
//         creation_date = SCXCalendarTime::FromPosixTime(m_puse.pr_create.tv_sec);
//         SCXAmountOfTime nsec;
//         nsec.SetSeconds(m_puse.pr_create.tv_nsec / 1000000000.0L);
//         creation_date += nsec;
//         creation_date.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
//         return true;
#elif defined(hpux)
        creation_date = SCXCalendarTime::FromPosixTime(m_pstatus.pst_start);
        creation_date.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the termination date of this process instance.

       \param[out]  termination_date Return parameter for the termination date
       \returns     true if process is terminated and this value is supported by the implementation

       According to the CIM model: "Time that the Process was stopped or terminated."
       The accururacy if this parameter is low. The operating system doesn't make
       a note of the time when a process is terminated so the best we can do is notice
       the disappearance of a previously existing process and record the time.

       The CIM model does not state what value this property should have for a
       running process. We choose to mark it as not supported.
    */
    bool ProcessInstance::GetTerminationDate(SCXCoreLib::SCXCalendarTime& termination_date)
        const {
#if defined(linux) ||  defined(hpux) || defined(sun) || defined(aix)
        if (0 == m_timeOfDeath.tv_sec) return false;

        termination_date = SCXCalendarTime::FromPosixTime(m_timeOfDeath.tv_sec);
        SCXAmountOfTime usec;
        usec.SetSeconds(static_cast<SCXCoreLib::scxseconds>(m_timeOfDeath.tv_usec / 1000000.0L));
        termination_date += usec;
        termination_date.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
        return true;

        // On solaris we have a termination timestamp with high resultion.
        // We would like to use it, but it returns the wrong result
//         termination_date = SCXCalendarTime::FromPosixTime(m_puse.pr_term.tv_sec);
//         SCXAmountOfTime nsec;
//         nsec.SetSeconds(m_puse.pr_term.tv_nsec / 1000000000.0L);
//         termination_date += nsec;
//         termination_date.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
//         return true;
#else
        return false;
#endif
    }


    /**
       Gets the parent pid of this process instance.

       \param[out]  ppid Return parameter
       \returns     true if this value is supported by the implementation

       According to the CIM model: "Time that the Process was stopped or terminated."
       All processes are created when another process executes the fork() command.
       This is the process id of that process.
    */
    bool ProcessInstance::GetParentProcessID(int& ppid) const
    {
#if defined(linux)
        ppid = m.parentProcessId;
        return true;
#elif defined(sun) || defined(aix)
        ppid = m_psinfo.pr_ppid;
        return true;
#elif defined(hpux)
        ppid = m_pstatus.pst_ppid;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the user id of the owner of this process instance.

       \param[out]  uid Return parameter
       \returns     true if this value is supported by the implementation

       According to the CIM model: "The Real User ID of this currently
       executing process." This is what you would get if you executed
       the system call getuid() on the current process.
    */
    bool ProcessInstance::GetRealUserID(scxulong& uid) const
    {
#if defined(linux)
        uid = m_uid;
        return true;
#elif defined(sun) || defined(aix)
        uid = m_psinfo.pr_uid;
        return true;
#elif defined(hpux)
        uid = m_pstatus.pst_uid;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the process group id of this process instance.

       \param[out]  pgid Return parameter
       \returns     true if this value is supported by the implementation

       According to the CIM model: "The Group ID of this currently
       executing process." This corresponds to the system call getpgrp().
    */
    bool ProcessInstance::GetProcessGroupID(scxulong& pgid) const
    {
#if defined(linux)
        pgid = m.processGroupId;
        return true;
#elif defined(sun) || defined(aix)
        pgid = m_psinfo.pr_pgid;
        return true;
#elif defined(hpux)
        pgid = m_pstatus.pst_pgrp;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the nice value of this process instance.

       \param[out]  nice Return parameter
       \returns     true if this value is supported by the implementation

       According to the CIM model: "The process's 'nice' value.
       Used to compute its priority." This is the traditional UNIX measure of user
       requested priority for a process.
    */
    bool ProcessInstance::GetProcessNiceValue(unsigned int& nice) const
    {
        /* Note: All platforms have signed nice values, but in the CIM model it's unsigned. */
#if defined(linux)
        nice = static_cast<unsigned int>(m.nice + 20); // Offset to insure it's always >= 0
        return true;
#elif defined(aix)
        nice = m_procentry.pi_nice;
        if (nice > 39) nice = 0; // This is comparable with what PS command shows (ps actuially shows "--")
        return true;
#elif defined(sun) 
        nice = m_psinfo.pr_lwp.pr_nice;
        return true;
#elif defined(hpux)
        nice =  m_pstatus.pst_nice;
        return true;
#else
        return false;
#endif
    }


    /*====================================================================================*/
    /* Properties of SCX_UnixProcess, Phase 2                                             */
    /*====================================================================================*/

    /**
       Get textual description of process state in case state is "other".
       \param[out]  description Return parameter
       \returns     true if this value is supported by the implementation

       According to the CIM model: "A string describing the state - used
       when the instance's ExecutionState property is set to 1 (Other)"
    */
    bool ProcessInstance::GetOtherExecutionDescription(std::wstring& description) const
    {
#if defined(linux) || defined(sun)
        // We don't ever return an execution state of type "Other".
        (void) description;
        return false;
#elif defined(hpux)
        if (PS_OTHER == m_pstatus.pst_stat) {
            description = L"Other";
            return true;
        }
        return false;           // Or, return empty string?
#elif defined(aix)
        if ('O' == m_psinfo.pr_lwp.pr_sname) {
            description = L"Other";
            return true;
        }
        return false;           // Or, return empty string?
#else
        return false;
#endif
    }


    /**
       Gets process' time spent in system mode.
       \param[out]  kmt Kernel mode time in milliseconds
       \returns     true if this value is supported by the implementation

       According to the CIM model: "Time in kernel mode, in milliseconds."
    */
    bool ProcessInstance::GetKernelModeTime(scxulong& kmt) const
    {
#if defined(linux)
        kmt = m.systemTime * m_jiffies_per_second;
        return true;
#elif defined(sun)
        kmt = m_puse.pr_stime.tv_nsec / 1000000Ull;
        kmt += m_puse.pr_stime.tv_sec * 1000Ull;
        return true;
#elif defined(hpux)
        kmt = m_pstatus.pst_stime * 1000;
        return true;
#elif defined(aix)
        CheckRootAccess();
        kmt = m_pstat.pr_stime.tv_nsec / 1000000Ull;
        kmt += m_pstat.pr_stime.tv_sec * 1000Ull;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets process' time spent in user mode.
       \param[out]  umt Kernel mode time in milliseconds
       \returns     true if this value is supported by the implementation

       According to the CIM model: "Time in kernel mode, in milliseconds."
    */
    bool ProcessInstance::GetUserModeTime(scxulong& umt) const
    {
#if defined(linux)
        umt = m.userTime * m_jiffies_per_second;
        return true;
#elif defined(sun)
        umt = m_puse.pr_utime.tv_nsec / 1000000Ull;
        umt += m_puse.pr_utime.tv_sec * 1000Ull;
        return true;
#elif defined(hpux)
        umt = m_pstatus.pst_utime * 1000;
        return true;
#elif defined(aix)
        CheckRootAccess();
        umt = m_pstat.pr_utime.tv_nsec / 1000000Ull;
        umt += m_pstat.pr_utime.tv_sec * 1000Ull;
        return true;
#else
        return false;
#endif
    }


    /**
       Does nothing!
       \param[out]  wss Output parameter that is not used
       \returns     true if this value is supported by the implementation

       According to the CIM model: "The amount of memory in bytes that a
       Process needs to execute efficiently for an OperatingSystem that uses
       page-based memory management. If an insufficient amount of memory is
       available (< working set size), thrashing will occur. If this
       information is not known, NULL or 0 should be entered. If this data
       is provided, it could be monitored to understand a Process' changing
       memory requirements as execution proceeds."

       Since the above description is a purely theoretical number we can't
       support it.
    */
    bool ProcessInstance::GetWorkingSetSize(scxulong&) const
    {
        return false;
    }


    /**
       Gets the process group ID of a session leader
       \param[out]  sid Process group id
       \returns     true if this value is supported by the implementation

       According to the CIM model: "If part of a group of processes are
       under the control of a session leader, this property holds the
       session ID for the group."
    */
    bool ProcessInstance::GetProcessSessionID(scxulong& sid) const
    {
#if defined(linux)
        sid = m.sessionId;
        return true;
#elif defined(sun) || defined(aix)
        sid = m_psinfo.pr_sid;
        return true;
#elif defined(hpux)
        sid = m_pstatus.pst_sid;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets
       \param[out]  tty
       \returns     true if this value is supported by the implementation

       According to the CIM model: "The TTY currently associated with this process."
    */
    bool ProcessInstance::GetProcessTTY(std::string& tty) const
    {
        tty = tty;
#if defined(linux)
        // There should be a way to support this since 'ps' does, but we can't find it.
        return false;
#elif defined(sun)
        // Here, too. Can't find out how to do it.
        return false;
#elif defined(hpux)
        // This implementation uses HP's caching mechanism. We could implement
        // a different cache and save some time.
        if (m_pstatus.pst_major == -1 && m_pstatus.pst_minor == -1) {
            tty = "?";
        } else {
            char result[255];
            dev_t devID = (m_pstatus.pst_major << 24) | m_pstatus.pst_minor;
            int rc = devnm(S_IFCHR, devID, result, sizeof(result), 1);
            if (rc == 0 || rc == -3) { // We don't care about truncated output
                tty = result;
            } else {
                tty = "?";
            }
        }
        return true;
#elif defined(aix)
        // AIX has the same problem.
        return false;
#else
        return false;
#endif
    }

    /**
       Gets path to executable file that process runs.
       \param[out]  modpath Name and path of executable file
       \returns     true if this value is supported by the implementation

       According to the CIM model: "The executing process's command path."

       \note Getting this property requires root access on Solaris and Linux
    */
    bool ProcessInstance::GetModulePath(std::string& modpath) const
    {        // XXX We could cache all of this!
#if defined(linux) || defined(sun)
        char pathbuf[255];
        char procExeName[PROCPATH_LEN];

        CheckRootAccess();
#ifdef linux
        snprintf(procExeName, sizeof(procExeName), "/proc/%u/exe", static_cast<unsigned int>(m_pid));
#else /* sun */
        snprintf(procExeName, sizeof(procExeName), "/proc/%u/path/a.out", static_cast<unsigned int>(m_pid));
#endif
        // Note: In theory we could call lstat() to peek on the length
        // of the path, but that don't work for the /proc/ file system.
        int res = static_cast<int>(readlink(procExeName, pathbuf, sizeof(pathbuf)));

        if (res < 0) {
            if (errno == ENOENT) {
                // Process has died since last Update()
                return false;
            }
            // We can't support this for some reason. Note that we need root access!
            SCX_LOGWARNING(m_log, StrAppend(L"Error reading execution path. errno = ", errno));
            return false;
        }

        modpath.assign(pathbuf, res);
        return true;
#elif defined(hpux)
        char filepath[256];

        CheckRootAccess();
        int count = pstat_getpathname(filepath, sizeof(filepath),
                                      const_cast<pst_fid *>(&m_pstatus.pst_fid_text));
        // We're casting away const here! We bet that the lack of constness on the
        // fid parameter was just an mistake by HP.
        if (count > 0) {        // We have a path. Return it.
          modpath.assign(filepath, count);
          return true;
        }
        // If we get here count < 0 and errno indicates the error.
        // OR count == 0 means that the value wasn't cached. Unusual!
        // EOVERFLOW means that the buf wasn't big enough. We can handle it!
        if (count < 0 && errno == EOVERFLOW) {
          modpath.assign(filepath, sizeof filepath);
          return true;
        }
        // ENOENT is normal, and ESRCH means zombie. These are normal and nothing to warn bout
        // EACCES means that we don't have root access. Not a problem either.
        // Now we try to use pst_cmd to get the path.

        if (m_pstatus.pst_cmd[0] == '/') {
          // Example: if pst_ucomm=openwsmand and
          // pst_cmd=/opt/microsoft/scx/bin/openwsmand -S -c /etc/opt/microsoft/scx/
          // then search for "/openwsmand" and eventually return
          // "/opt/microsoft/scx/bin/openwsmand".
          char searchterm[sizeof(m_pstatus.pst_ucomm) + 1];
          size_t ul = strlen(m_pstatus.pst_ucomm);
          searchterm[0] = '/';
          memcpy(searchterm + 1, m_pstatus.pst_ucomm, ul + 1);
          const char *found = strstr(m_pstatus.pst_cmd, searchterm);
          if (found) {
            modpath.assign(m_pstatus.pst_cmd, (found - m_pstatus.pst_cmd) + ul + 1);
            return true;
          }
        }
        return false;
#elif defined(aix)
        // We could do a "best-effort" implementation in same style as on HPUX.
        // Unless we find a way to extract the path from /proc/#/object/a.out...
        return false;
#else
        return false;
#endif
    }

#if defined(sun)
    /** AutoClose destructor */
    ProcessInstance::AutoClose::~AutoClose()
    {
        if (m_fd != 0)
        {
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"GetParameters: AutoClose closing fd: ", m_fd));
            if (close(m_fd) < 0)
            {
                SCX_LOGERROR(m_log,
                             StrAppend(StrAppend(L"Error in ~AutoClose closing fd ", m_fd),
                                       StrAppend(L", errno = ", errno)));
            }
            m_fd = 0;
        }
    }
#endif

    /**
       Gets the command line parameters of the process
       \param[out]  params The command line parameters
       \returns     true if this value is supported by the implementation

       According to the CIM model: ""The operating system parameters
       provided to the executing process. These are the argv[] values."

       A string corresponding to argv[0] is in params[0], etc up to argc.
    */
    bool ProcessInstance::GetParameters(std::vector<std::string>& params) const
    {        // XXX We could buffer these!
#if defined(linux)
        char procCmdName[PROCPATH_LEN];
        std::string parambuf;
        bool bFirstParam = true;
        snprintf(procCmdName, sizeof(procCmdName), "/proc/%u/cmdline", static_cast<unsigned int>(m_pid));
        ifstream file(procCmdName);
        if (!file) { return false; } // Process has died, or does no support this
        params.clear();
        while(getline(file, parambuf, '\0')) {
            // Interface says two consecutive null bytes ends list; honor that
            // (We use flag to tell if we've read at least one parameter)
            if (0 == parambuf.length() && !bFirstParam)
            {
                break;
            }
            params.push_back(parambuf);
            bFirstParam = false;
        }
        return true;
#elif defined(sun)
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);

        // Define arbitary maximum values for parameters on Solaris:
        //   MAX_PARAMETERS:    Maximum number of parameters on a command line
        //   MAX_PARAM_LENGTH:  Maximum length of all parameters (in total) on a command line
        //
        // If these maximums are exceeded, we'll log a one-time error and ignore the parameters
        //
        //
        // WI 26809 (Resolve BVT failures on Solaris 10 x86 machines)
        //
        // Apparently, at times, the argv length is longer than anticipated.  Raised maximum
        // space to allow for this.  A posting on comp.os indicated that the true maximum
        // command line length is 2MB, but I find that very difficult to believe.  Raised it
        // to 1MB to see how that goes.
        //
        // Additionally, we can never read 64-bit processes (since we're built in 32-bit mode)
        // successfully.  In this case, read/pread operations will fail with EOVERFLOW.  When
        // this happens, we log a HYSTERICAL error and bag, which is fine (we'll fallback to use
        // shorter parameters in this case).
        //
        //
        // WI 50210 (Solaris 10 Sparc Agent: An error "argvecsz too large" sometimes occurs)
        //
        // Additional testing indicates that the maximum size for the argument list (at least
        // from the perspective of what can be retrieved, which is all we care about here) is
        // well under 1MB.  Additionally, this limit on the Linux platforms is 4k (4096 bytes);
        // parameters can be much longer than that, but can't be retrieved longer than that.
        //
        // Strangely, this problem (on a test system) fails consistently on process dtlogin.
        // We receive this error on the log file:
        //
        //   2011-10-10T20:49:27,329Z Error      [scx.core.common.pal.system.process.processinstance:21811:2] GetParameters: Process /proc/660/as argvecsz too large, argvecsz = 4495126
        //
        // This indicates that the argvecsz (from the process header) is 4 megabytes, but 'ps'
        // displays this process as follows:
        //
        //   bash-3.00# ps -ef | grep 660
        //   root   660     1   0   Aug 15 ?           0:00 /usr/dt/bin/dtlogin -daemon
        //   root 21814  8481   0 13:50:05 pts/1       0:00 grep 660
        //
        // Given results of testing, I'm comfortable with the 1MB limit here.  I'll change
        // the above to a Warning (rather than an error), and keep the limit of one warning
        // per process.  Making this a warning will stop the group aborts from testing, and
        // this should be put to bed.
        //
        // If we get the above warning, we'll use our fallback mechanism (pr_psargs from file
        // /proc/<pid>/psinfo).  It's pretty short in maximum length, but better than nothing.

        const int MAX_PARAMETERS = 256;
        const int MAX_PARAM_LENGTH = 1048576;   // 1 MB

        char procAsName[PROCPATH_LEN];
        const int argc = m_psinfo.pr_argc;
        const uintptr_t argv_addr = m_psinfo.pr_argv;
        const uintptr_t envp_addr = m_psinfo.pr_envp;

        // Get initial argv.
        //
        // Note: This serves as the "default" set of arguments.  The problem
        // with this is that, on the Solaris platform, this can be "truncated".
        // So we read the "/proc/<pid>/as" file to get the complete set of
        // arguments but, if it fails, we fall back to this set.
        string initialcmd(m_psinfo.pr_psargs);

        if (0 == argv_addr || argc == 0) {
            params.push_back(initialcmd);   // problem = " (Process has no parameters)";
            return true;
        }

        snprintf(procAsName, sizeof(procAsName), "/proc/%u/as", static_cast<unsigned int>(m_pid));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"GetParameters: Process filename = ", procAsName));

        int asfd = open(procAsName, O_RDONLY);
        if (asfd < 0)
        {
            // Failed to open file. Most likely the process died.
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"GetParameters: Error opening process file, err=", errno));
            params.push_back(initialcmd);
            return true;
        }

        AutoClose _fd(m_log, asfd);

        /* Read the full argv vector */
        if (argc <= MAX_PARAMETERS)
        {
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"GetParameters: Allocating arg_vec vector of size: ", argc));
        }
        else
        {
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(StrFromMultibyte(procAsName)));
            SCX_LOG(m_log, severity, StrAppend(
                        StrAppend(L"GetParameters: Process ", procAsName),
                        StrAppend(L" argc too large, argc = ", argc)));
            params.push_back(initialcmd);
            return true;
        }

        vector<uintptr_t> arg_vec(argc);
        int a;
        if (pread(asfd, &arg_vec[0], argc * sizeof(uintptr_t), argv_addr) < 0)
        {
            // Error reading data. Throw something? At least write in the log
            // cout << "Failed to read arg vector from " << procAsName << endl;
            SCX_LOGHYSTERICAL(m_log, StrAppend(
                StrAppend(L"GetParameters: Failed to read arg vector from ", procAsName),
                StrAppend(L", err=", errno)));
            params.push_back(initialcmd);
            return true;
        }

        /* The idea here is that the environment varaibles are stored after the argv
           strings and we can thus read the whole area at once. But for that to work we 
           need the position of the first item in the enviroment.
        */

        uintptr_t env1 = 0;
        if (envp_addr) {
            if (pread(asfd, &env1, sizeof(uintptr_t), envp_addr) < 0)
            {
                // Error reading data. Throw something? At least write in the log
                // cout << "Failed to read envp vector from " << procAsName << endl;
                SCX_LOGHYSTERICAL(m_log, StrAppend(
                    StrAppend(L"GetParameters: Failed to read envp vector from ", procAsName),
                    StrAppend(L", err=", errno)));
                params.push_back(initialcmd);
                return true;
            }
        }

        off_t argvecbase = arg_vec[0]; // Storage area in target coordinates
        // If env1 = 0: problem = " (Can't compute argv area)";
        size_t argvecsz = env1 ? env1 - argvecbase : 256; 
    
        // Test that the CMD exists. (Some programs, incl scxcimserver, clobbers it.)
        if (0 == argvecbase) {        // problem = " (Clobbered its own arguments)";
            SCX_LOGHYSTERICAL(m_log, L"GetParameters: Process clobbered it's own arguments");
            params.push_back(initialcmd);
            return true;
        }

        /* Now read full argv area in one swoop */
        if (argvecsz <= MAX_PARAM_LENGTH)
        {
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"GetParameters: Allocating argvarea vector of size: ", argvecsz));
        }
        else
        {
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(StrFromMultibyte(procAsName)));
            SCX_LOG(m_log, severity, StrAppend(
                        StrAppend(L"GetParameters: Process ", procAsName),
                        StrAppend(L" argvecsz too large, argvecsz = ", argvecsz)));
            params.push_back(initialcmd);
            return true;
        }

        vector<char> argvarea(argvecsz);
        if (pread(asfd, &argvarea[0], argvecsz, argvecbase) < 0)
        {
            // Error reading data. Throw something? At least write in the log
            // cout << "Failed to read argv area from " << procAsName << endl;
            SCX_LOGHYSTERICAL(m_log, StrAppend(
                StrAppend(L"GetParameters: Failed to read argv vector from ", procAsName),
                StrAppend(L", err=", errno)));
            params.push_back(initialcmd);
            return true;
        }

        argvarea[argvecsz - 1] = '\0';

        /* Verify that the argv read is similar to the potentially truncated version */
        for (int i=0; i < argc; ++i) {
            // Check that this argument pointer is valid
            if (0 == arg_vec[i]) {
                // Nope. No more arguments, regardless of what argc may say
                break;
            }
            int idx = arg_vec[i] - argvecbase;
            if (idx < 0) {
                SCX_LOGHYSTERICAL(m_log, StrAppend(L"GetParameters: Buffer underflow: ", idx));
                break;
            }
            if (idx >= argvecsz) {
                SCX_LOGHYSTERICAL(m_log, StrAppend(L"GetParameters: Buffer overflow: ", idx));
                break;
            }

            // Sicne we're reading an address space that may be changed by the process
            // We log what address we're reading so any mysterious crashes and/or
            // access violations may be easier to find if this happens.
            SCX_LOGHYSTERICAL(m_log, StrAppend(StrAppend(StrAppend(L"GetParameters: arg_vec[", i), L"] = "), static_cast<ulong>(arg_vec[i])));
            
            string arg;
            if (0 == i) { /* The CMD part is a special case */
                arg = &argvarea[0];
            }
            else {
                arg = &argvarea[idx];
            }

            while ( ! initialcmd.empty() &&
                   (initialcmd[0] == ' ' ||
                    initialcmd[0] == '\t' ||
                    initialcmd[0] == '\n')) {
                initialcmd = initialcmd.substr(1);
            }
            if ( ! initialcmd.empty() &&
                arg.substr(0, initialcmd.length()) != initialcmd.substr(0, arg.length())) {
                // The process can manipulate its own string area. We try to detect that
                // by comparing the argv[i] with what was stored in /#/psinfo.
                return true;
            }
            if (arg.length() >= initialcmd.length()) {
                initialcmd = "";
            }
            else {
                initialcmd = initialcmd.substr(arg.length());
            }
            SCX_LOGHYSTERICAL(m_log, StrAppend(StrAppend(StrAppend(StrAppend(StrAppend(L"GetParameters: arg_vec[", i), L"] = "), static_cast<ulong>(arg_vec[i])),L", Parameter Value: "), StrFromMultibyte(arg)));
            params.push_back(arg);
        }

        return true;
#elif defined(hpux)
        char cmdbuf[1024];
        int cmdlen = 0;
        const char* cmdline = 0;

        // Check that command line makes sense
        // Note: PST_CLEN is 64 bytes. If we need more of the command line we
        // can use pstat_getcommandline() and get up to 1020 chars.

        const char *endcmd = static_cast<const char*>(memchr(m_pstatus.pst_cmd, '\0', PST_CLEN));
        if (endcmd == 0) { return false; }
        cmdline = m_pstatus.pst_cmd;
        cmdlen  = endcmd - cmdline;

        // If the pst_cmd field is full, then there is a risk that it was truncated.
        // Let's get the longer version with pstat_getcommandline.

        if (cmdlen == PST_CLEN - 1) {
            cmdline = cmdbuf;
            cmdlen = pstat_getcommandline(cmdbuf, sizeof(cmdbuf), 1, 
                                          static_cast<unsigned int>(m_pid));

            if (cmdlen < 0) {
                /* Race: Process may already have died. Use short version instead. */
                if (ESRCH == errno) {
                    cmdline = m_pstatus.pst_cmd;
                    cmdlen  = endcmd - cmdline;
                } else {
                    throw SCXErrnoException(L"pstat_getcommandline", errno, SCXSRCLOCATION);
                }
            }
        }
        
        const string c(cmdline, cmdlen);

        // look for basename in command line (so that we can remove path component)
        string::size_type posstart = c.find(m_pstatus.pst_ucomm);
        if (posstart == string::npos) {
            // Sometimes ucomm isn't part of the command line.
            posstart = 0;
        }

        // First entry (argv[0]) is the basename
        // and then each string separated with one space is a new param
        // Note that a commandline parameter with spaces in it can't be distinguished
        // from multiple parameters. There's no way around it.
        string::size_type possep = c.find(' ', posstart);

        while (possep != string::npos) {
            // Copy range [posstart, possep)
            params.push_back(c.substr(posstart, possep - posstart));
            posstart = possep + 1;
            possep = c.find(' ', posstart);
        }
        
        // Copy last argument (to end of line)
        params.push_back(c.substr(posstart));
        return true;
#elif defined(aix)
//
//      After performing some testing across various platforms it was determined that
//      even though a process with a large commandline can be executed correctly using exec, the 
//      retrieval of the parameters is limited to 4096 bytes (this is true for RH,SLES,AIX)
//      This is why there is a imposed limit of 4096 bytes on the parameters.
//
        char cmdbuf[4096];
        struct procentry64 processBuffer;

        memset(&processBuffer, 0, sizeof(processBuffer));
        memset(&cmdbuf, 0, sizeof(cmdbuf));

        processBuffer.pi_pid = m_pid;
        if (0 != getargs(&processBuffer, sizeof(processBuffer), &cmdbuf[0], sizeof(cmdbuf)))
        {
            // Race: Process may already have died
            if (ESRCH == errno) {
                return false;
            } else {
                throw SCXErrnoException(L"getargs", errno, SCXSRCLOCATION);
            }
        }

        // Let's be certain we can't possibly read beyond our buffer
        // (Set the last two bytes of buffer to null to signfy end)
        cmdbuf[sizeof(cmdbuf)-2] = cmdbuf[sizeof(cmdbuf)-1] = '\0';

        char *argP = &cmdbuf[0];
        while ( argP < &cmdbuf[0] + sizeof(cmdbuf) && *argP )
        {
            params.push_back(argP);
            argP += strlen(argP) + 1;
        }

        return true;

#else
        return false;
#endif
    }


    /**
       Gets the WCHAN data for process.
       \param[out]  event the WCHAN string
       \returns     true if this value is supported by the implementation

       According to the CIM model: "A description of the event this process
       is currently sleeping for. [...] This string only has meaning when the
       "ExecutionState is "Blocked" or "SuspendedBlocked"."
    */
    bool ProcessInstance::GetProcessWaitingForEvent(std::string& event) const
    {
        // Note: We can't cache this!
#if defined(linux)
        char procWchanName[PROCPATH_LEN];
        snprintf(procWchanName, sizeof(procWchanName), "/proc/%u/wchan", static_cast<unsigned int>(m_pid));
        ifstream file(procWchanName);
        if (!file) { return false; } // Process has died, or does no support this
        if ((file >> event)) {
            return true;
        }
        return false;
#elif defined(sun) || defined(aix)
        // We could return a memory address, like ps does, but that would be of no interest.
        return false;
#elif defined(hpux)
        // There is pst_wchan, but that's just an address. Right?
        return false;
#else
        return false;
#endif
    }


    /*====================================================================================*/
    /* Properties of SCX_UnixProcessStatisticalInformation                                */
    /*====================================================================================*/

    /**
       Gets the relative CPU consumption of this process instance.

       \param[out]  cpu Return parameter with an integer percentage value
       \returns     true if this value is supported by the implementation

       According to the CIM model: "The percentage of a CPU's time this
       process is consuming."

       There are built-in ways to get the recent cpu load for a couple of
       these architecture, but we will instead present a 1-minute average.
    */
    bool ProcessInstance::GetCPUTime(unsigned int& cpu) const
    {
#if defined(linux)
        cpu = ComputePercentageOfTime(m_delta_UserTime + m_delta_SystemTime, m_delta_RealTime);
        return true;
#elif defined(sun) || defined(aix)
        cpu = ComputePercentageOfTime(m_delta_UserTime + m_delta_SystemTime, m_delta_RealTime);
        return true;
#elif defined(hpux)
        cpu = ComputePercentageOfTime(m_delta_UserTime + m_delta_SystemTime, m_delta_RealTime);

        // There might be an alternative way to compute this, with better resolution:
        //cpu = ComputePercentageOfTime(m_delta_CPUTime, m_delta_CPUTime_total);
        return true;
#else
        return false;
#endif
    }


    /**
       Gets a measure of the number of recent block write operations per second
       performed by this process.

       \param[out]  bws Return parameter for the number of block writes
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Block writes per second".
       This is an approximation of file writing activity per process.
       Those platforms that support this parameter report an cumulative
       number of block writes. We sample that number and divide by
       the interval.
    */
    bool ProcessInstance::GetBlockWritesPerSecond(scxulong &bws) const
    {
#if defined(linux) || defined(aix)
        /* This is not available on Linux or AIX */
        bws = 0;
        return false;
#elif defined(sun) || defined(hpux)
        bws = ComputeItemsPerSecond(m_delta_BlockOut, m_delta_RealTime);
        return true;
#else
        return false;
#endif
    }

    /**
       Gets a measure of the number of recent block read operations per second
       performed by this process.

       \param[out]  bwr Return parameter for the number of block reads
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Block reads per second".
       This is an approximation of file read activity per process.
       Those platforms that support this parameter report an cumulative
       number of block reads. We sample that number and divide by
       the interval.
    */
    bool ProcessInstance::GetBlockReadsPerSecond(scxulong &bwr) const
    {
#if defined(linux) || defined(aix)
        /* This is not available on Linux or AIX */
        bwr = 0;
        return false;
#elif defined(sun) || defined(hpux)
        bwr = ComputeItemsPerSecond(m_delta_BlockInp, m_delta_RealTime);
        return true;
#else
        return false;
#endif
    }


    /**
       Gets a measure of the number of recent block transfer operations per second
       performed by this process.

       \param[out]  bts Return parameter for the number of block transfers
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Total block I/Os per second".
       This is an approximation of all file I/O activity per process.
       Those platforms that support this parameter report an cumulative
       number of block reads. We sample that number and divide by
       the interval. Unless directly supported by platform, the sum
       of block reads and blocks writes are reported instead.
    */
    bool ProcessInstance::GetBlockTransfersPerSecond(scxulong &bts) const
    {
#if defined(linux) || defined(aix)
        /* This is not available on Linux or AIX */
        bts = 0;
        return false;
#elif defined(sun) || defined(hpux)
        bts = ComputeItemsPerSecond(m_delta_BlockOut + m_delta_BlockInp, m_delta_RealTime);
        return true;
#else
        return false;
#endif
    }


    /**
       Gets cpu load in user mode.

       \param[out]  put Return parameter for the load.
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Percentage of non-idle processor time spent
       in user mode". We implement this property to mean recently consumed CPU time
       in user mode divided by wall clock time.
    */
    bool ProcessInstance::GetPercentUserTime(scxulong &put) const
    {
#if defined(linux) || defined(sun) || defined(hpux) || defined(aix)
        put = ComputePercentageOfTime(m_delta_UserTime, m_delta_RealTime);
        return true;
#else
        return false;
#endif
    }


    /**
       Gets cpu load in privileged mode.

       \param[out]  ppt Return parameter for the load.
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Percentage of non-idle processor time spent
       in privileged  mode". We implement this property to mean recently consumed CPU time
       in system mode divided by wall clock time.
    */
    bool ProcessInstance::GetPercentPrivilegedTime(scxulong &ppt) const
    {
#if defined(linux) || defined(sun) || defined(hpux) || defined(aix)
        ppt = ComputePercentageOfTime(m_delta_SystemTime, m_delta_RealTime);
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the amount of physical memory in use by a process.

       \param[out]  um Return parameter for amount of memory
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Used physical memory in kilobytes".

       We interpret this value to be the resident set size of the process.
       This is a measure of total size of those pages that belong to a process that
       are currently swapped in.
    */
    bool ProcessInstance::GetUsedMemory(scxulong &um) const
    {
#if defined(linux)
        um = m.residentSetSize * m_pageSize;
        return true;
#elif defined(sun) || defined(aix)
        um = m_psinfo.pr_rssize;
        return true;
#elif defined(hpux)
        um = m_pstatus.pst_rssize;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the relative size of physical memory used by process.

       \param[out]  pum Return parameter for use memory percentage
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Ratio of Resident Set Size to Virtual
       Memory for process (essentially percentage of process loaded into
       memory)".

       We implement this propery as the quotient of phyical memory in use
       (a.k.a. resident set size) and virtual size of the process image.
    */
    bool ProcessInstance::GetPercentUsedMemory(scxulong &pum) const
    {
#if defined(linux)
        pum = (m.virtualMemSizeBytes > 0) ? 100 * m.residentSetSize * m_pageSize * 1024 / m.virtualMemSizeBytes : 0;
        return true;
#elif defined(sun) || defined(aix)
        pum = (m_psinfo.pr_size > 0) ? 100 * m_psinfo.pr_rssize / m_psinfo.pr_size : 0;
        if (pum > 100) { pum = 100; }           // Limit this to 100%
        return true;
#elif defined(hpux)
        // We don't use pst_rssize here. There is a built in symmetry in
        // the process status values on HPUX that we just can't resist using.
        scxulong vsize = m_pstatus.pst_vdsize + m_pstatus.pst_vtsize + m_pstatus.pst_vssize;
        scxulong rsize = m_pstatus.pst_dsize + m_pstatus.pst_tsize + m_pstatus.pst_ssize;
        pum = (vsize > 0) ? 100 * rsize / vsize : 0;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the recent number of hard page faults per second performed by this process.

       \param[out]  prs Return parameter for the page faults
       \returns     true if a value is supported by the implementation

       According to the CIM model: "Pages read from disk per second to resolve
       hard page faults".

       The process usually delivers a cumulative count of page faults. We sample
       this number and divide the delta by the sample interval.

       \note There is no GetPagesWrittenPerSec method. This is not an omission.
       Page reclaims are not made on a per-process level, so that kind of
       function would have no meaning.
    */
    bool ProcessInstance::GetPagesReadPerSec(scxulong &prs) const
    {
#if defined(linux) || defined(sun) || defined(hpux)
        prs = ComputeItemsPerSecond(m_delta_HardPageFaults, m_delta_RealTime);
        return true;
#elif defined(aix)
        return false;
#else
        return false;
#endif
    }

    /*====================================================================================*/
    /* Properties of SCX_UnixProcessStatisticalInformation, Phase 2                       */
    /*====================================================================================*/

    /**
       Gets the size of the currently paged in parts of the program code as
       reported by the system.

       \param[out]  rt Return parameter for code size in kilobytes
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of real text
       space used by the process."

       It's undefined whether this includes constant data or not.
    */
    bool ProcessInstance::GetRealText(scxulong &rt) const
    {
        rt = 0;
#if defined(linux)
        return false;
#elif defined(sun)
        return false;
#elif defined(hpux)
        rt = m_pstatus.pst_tsize * m_pageSize;
        return true;
#elif defined(aix)
        return false;
#else
        return false;
#endif
    }


    /**
       Gets the paged in size of process data.

       \param[out]  rd Return parameter for data size in kilobytes
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of real data
       space used by the process."

       It's undefined whether this includes constant data or not.
    */
    bool ProcessInstance::GetRealData(scxulong &rd) const
    {
        rd = 0;
#if defined(linux)
        return false;
#elif defined(sun)
        return false;
#elif defined(hpux)
        rd = m_pstatus.pst_dsize * m_pageSize;
        return true;
#elif defined(aix)
        return false;
#else
        return false;
#endif
    }


    /**
       Gets the paged in size of the process stack.

       \param[out]  rs Return parameter for stack size in kilobytes
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of real stack
       space used by the process."

       It's undefined whether this includes thread stacks.
    */
    bool ProcessInstance::GetRealStack(scxulong &rs) const
    {
        rs = 0;
#if defined(linux)
        return false;
#elif defined(sun)
        return false;
#elif defined(hpux)
        rs = m_pstatus.pst_ssize * m_pageSize;
        return true;
#elif defined(aix)
        return false;
#else
        return false;
#endif
    }


    /**
       Gets the program code size as reported by the system.

       \param[out]  vt Return parameter for code size
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of virtual text
       space used by the process."

       It's undefined whether this includes constant data or not.

       \note Getting this property requires root access on Solaris
    */
    bool ProcessInstance::GetVirtualText(scxulong &vt) const
    {
#if defined(linux)
        vt = n.text * m_pageSize * 1024;
        return true;
#elif defined(sun)
        CheckRootAccess();
        // Note: This is a rather gross approximation of the text size
        vt = m_psinfo.pr_size - (m_pstat.pr_brksize + m_pstat.pr_stksize) / 1024;
        return true;
#elif defined(hpux)
        vt = m_pstatus.pst_vtsize * m_pageSize;
        return true;
#elif defined(aix)
        // The value m_psinfo.pr_size is program size plus heap. But it lags 
        // m_pstat.pr_brksize so much that we would get negative figures if we
        // tried to compute the text size by removing the heap size.
        // There don't seem to be any usable way to get this property.
        return false;
#else
        return false;
#endif
    }


    /**
       Gets size of data for process.

       \param[out]  vd Return parameter for data size
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of virtual data
       space used by the process."

       It's undefined whether this includes constant data or just the heap.

       \note Getting this property requires root access on AIX and Solaris.
    */
    bool ProcessInstance::GetVirtualData(scxulong &vd) const
    {
#if defined(linux)
        // On Linux, we can't subtract out the size of the stack because there's
        // no reliable way to get it.  Computing 'm.startstack - m.kstkesp' is
        // fine if it's a single-threaded program, but we can't determine that
        // either.  So just include size of stack in the returned value.
        //
        // Strictly speaking, we could read the 'maps' file in /proc, but
        // given the overhead, it's hardly worth it just for this.
        vd = n.data * m_pageSize * 1024;
        return true;
#elif defined(sun) || defined(aix)
        CheckRootAccess();
        // This is really heap size. Better than nothing.
        // Convert bytes to kilobytes and round up.
        vd = m_pstat.pr_brksize >> 10;
        if ((m_pstat.pr_brksize & (1024 - 1)) != 0) { ++vd; }
        return true;
#elif defined(hpux)
        vd = m_pstatus.pst_vdsize * m_pageSize;
        return true;
#else
        return false;
#endif
    }

    /**
       Gets the size of the process stack.

       \param[out]  vs Return parameter for stack size in kilobytes
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of virtual stack
       space used by the process."

       It's undefined whether this includes thread stacks.

       \note Getting this property requires root access on Solaris and AIX.

       \note At AIX this seems to be the reserved stack for all threads. 
       For ordinary processes it's always 256MB and does not grow at all.
    */
    bool ProcessInstance::GetVirtualStack(scxulong &vs) const
    {
#if defined(linux)
        // On Linux, there's no reliable way to get the size of the stack.
        // Computing 'm.startstack - m.kstkesp' is fine if it's a
        // single-threaded program, but we can't determine that
        // either.  So we can't support this value.
        //
        // Strictly speaking, we could read the 'maps' file in /proc, but
        // given the overhead, it's hardly worth it just for this.
        (void) vs;
        return false;
#elif defined(sun) || defined(aix)
        CheckRootAccess();
        // Convert bytes to kilobytes and round up.
        vs = m_pstat.pr_stksize >> 10;
        if ((m_pstat.pr_stksize & (1024 - 1)) != 0) { ++vs; }
        return true;
#elif defined(hpux)
        vs = m_pstatus.pst_vssize * m_pageSize;
        return true;
#else
        return false;
#endif
    }


    /**
       Gets the size of the memory mapped files.

       \param[out]  vmmfs Return parameter for memory mapped file size in kilobytes
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of virtual space
       used for memory mapped files by the process."
    */
    bool ProcessInstance::GetVirtualMemoryMappedFileSize(scxulong &vmmfs) const
    {
        vmmfs = 0;
#if defined(linux)
        return false;           // Nope, not here.
#elif defined(sun)
        return false;           // Can't do.
#elif defined(hpux)
        vmmfs = m_pstatus.pst_vmmsize * m_pageSize;
        return true;
#elif defined(aix)
        return false;           // Me too. Not.
#else
        return false;
#endif
    }


    /**
       Gets the size of shared memory in use by process

       \param[out]  vsm Return parameter for shared memory size in kilobytes
       \returns     true if a value is supported by the implementation

       According to the CIM model: "The number of KiloBytes of shared memory
       used by the process."

       It's undefined whether this includes sizes already reported by GetVirtualText
       and GetVirtualData.
    */
    bool ProcessInstance::GetVirtualSharedMemory(scxulong &vsm) const
    {
#if defined(linux)
        vsm = n.share * m_pageSize;
        return true;
#elif defined(sun)
        vsm = vsm;
        return false;           // Would be hard to support
#elif defined(hpux)
        vsm = m_pstatus.pst_vshmsize * m_pageSize;
        return true;
#elif defined(aix)
        return false;
#else
        return false;
#endif
    }


    /**
       Gets the CPU ticks of terminated child processes.

       \param[out]  ctdc Return parameter measured in clock ticks!
       \returns     true if a value is supported by the implementation

       According to the CIM model: "CPU time of terminated child processes
       in clock ticks."

       Note that times reported here is most likely limited to time from
       child processes that have been waited for.

       \note Getting this property requires root access on Solaris
    */
    bool ProcessInstance::GetCpuTimeDeadChildren(scxulong &ctdc) const
    {
#if defined(linux)
        ctdc = m.childUserTime + m.childSystemTime;
        return true;
#elif defined(sun) || defined(aix)
        CheckRootAccess();
        // Various trickery to avoid numeric overflow
        unsigned long clockCycleInNanoSeconds = 1000000000UL / m_clocksPerSecond;
        unsigned long nsTicks = m_pstat.pr_cstime.tv_nsec / clockCycleInNanoSeconds;
        nsTicks += m_pstat.pr_cutime.tv_nsec / clockCycleInNanoSeconds;
        ctdc = m_pstat.pr_cstime.tv_sec + m_pstat.pr_cutime.tv_sec;
        ctdc *= m_clocksPerSecond;;
        ctdc += nsTicks;
        return true;
#elif defined(hpux)
        scxulong hi = m_pstatus.pst_child_usercycles.psc_hi;
        ctdc = (hi << 32) + m_pstatus.pst_child_usercycles.psc_lo;

        hi = m_pstatus.pst_child_systemcycles.psc_hi;
        ctdc += (hi << 32) + m_pstatus.pst_child_systemcycles.psc_lo;

        hi = m_pstatus.pst_child_interruptcycles.psc_hi;
        ctdc += (hi << 32) + m_pstatus.pst_child_interruptcycles.psc_lo;

        return true;
#else
        return false;
#endif
    }


    /**
       Gets the system CPU ticks of terminated child processes.

       \param[out]  stdc Return parameter measured in clock ticks!
       \returns     true if a value is supported by the implementation

       According to the CIM model: "System time of terminated child processes
       in clock ticks."

       Note that times reported here is most likely limited to time from
       child processes that have been waited for.

       \note Getting this property requires root access on Solaris
    */
    bool ProcessInstance::GetSystemTimeDeadChildren(scxulong &stdc) const
    {
        stdc = 0;
#if defined(linux)
        stdc = m.childSystemTime;
        return true;
#elif defined(sun) || defined(aix)
        CheckRootAccess();
        unsigned long clockCycleInNanoSeconds = 1000000000UL / m_clocksPerSecond;
        unsigned long nsTicks = m_pstat.pr_cstime.tv_nsec / clockCycleInNanoSeconds;
        stdc = m_pstat.pr_cstime.tv_sec;
        stdc *= m_clocksPerSecond;;
        stdc += nsTicks;
        return true;
#elif defined(hpux)
        scxulong hi = m_pstatus.pst_child_systemcycles.psc_hi;
        stdc = (hi << 32) + m_pstatus.pst_child_systemcycles.psc_lo;
        return true;
#else
        return false;
#endif
    }

    /**************************************************************************/

    /**
       Sends a signal to the process. 
       This is the normal POSIX signal(2) call applied to the current instance.
       
       \param  signl The name of signal to be sent. Only use symbolic names from <signal.h>.
       \returns false is process was gone, or true is everything went well.
       \throws SCXAccessViolationException if attempt was made to signal a process to
       which the current process has no privileges for.
       \throws SCXErrnoException if some other error condition occured. Most likely
       an invalid signal.
    
       \note 
    */
    bool ProcessInstance::SendSignal(int signl) const
    {
        int res = ::kill(static_cast<pid_t>(m_pid), signl);
        if (res < 0) {
            if (ESRCH == errno) { return false; } // Process gone. That's ok
            if (EPERM == errno) { 
                throw SCXAccessViolationException(L"Attempt to signal a privileged process", 
                                                  SCXSRCLOCATION); }
            throw SCXErrnoException(L"kill", errno, SCXSRCLOCATION); 
        }
        return true;
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
