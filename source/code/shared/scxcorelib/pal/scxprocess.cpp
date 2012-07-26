/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        scxprocess.cpp

    \brief       Implements the process handling PAL. 
    
    \date        2007-10-12 15:43:00
 
*/
/*----------------------------------------------------------------------------*/

#if defined(hpux)
#undef _XOPEN_SOURCE // Or else chroot is not defined.
#include <sys/time.h>
#endif
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxthread.h>

#include <iostream>
#include <stdio.h>
#include <vector>

#if defined(WIN32)
#include <process.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(macos) || defined(sun)
#include <signal.h>
#endif

#include <errno.h>

namespace SCXCoreLib
{
 
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the calling process', process id.
    
        \returns      SCXProcessId of the calling thread.
        
    */
    SCXProcessId SCXProcess::GetCurrentProcessID()
    {
#if defined(WIN32)
        return GetCurrentProcessId();
#elif defined(SCX_UNIX)
        return getpid();
#else
#error "Platform not supported"
#endif
    }
     
    //! Split a command into its separate parts
    //! \param[in]  command like those entered in a command shell
    //! \returns    Parts that was separated by space in "command"
    //! A part is allowed to contain spaces if those are quoted by single or double quotes.
    //! If a part itself consists of a single quote, that part may be quoted by a double quote,
    //! and vice versa.
    //! \note   Parts are delimited by spaces, not the quotes. If one were to list all files
    //          of the directory /usr/local/apache-tomcat/, it could be written as follows:
    //!         ls "/usr/"local/'apache-tomcat/'. That is useful if a command consists of 
    //!         single as well as double quotes.
    std::vector<std::wstring> SCXProcess::SplitCommand(const std::wstring &command) {
        std::vector<std::wstring> parts;
        
        std::wostringstream part;
        bool newPart = false;
        wchar_t quote = 0;
        for (std::wstring::size_type i = 0; i < command.length(); i++) 
        {
            wchar_t c = command.at(i);        
            if (c == ' ') 
            {
                if (quote) 
                {
                    part << c;
                }
                else if (newPart) 
                {
                    parts.push_back(part.str());
                    part.str(L"");                    
                    newPart = false;
                }
            } 
            else if (c == '\'' || c == '"') 
            {
                if (quote == c)   
                {
                    quote = 0;
                }
                else if (quote)
                {
                    part << c;                    
                }
                else 
                {
                    quote = c;
                }
            }
            else 
            {
                part << c;                
                newPart = true;
            }
        }
        if (newPart) {
            parts.push_back(part.str());      
        }
        return parts;
    }
    
#if defined(SCX_UNIX)
    /**********************************************************************************/
    //! Run a process by passing it arguments and streams for stdin, stdout and stderr
    //! \param[in]  command     Corresponding to what would be entered by a user in a command shell
    //! \param[in]  mystdin     stdin for the process
    //! \param[in]  mystdout    stdout for the process
    //! \param[in]  mystderr    stderr for the process
    //! \param[in]  timeout     Accepted number of millieconds to wait for return
    //! \param[in]  cwd         Directory to be set as current working directory for process.
    //! \param[in]  chrootPath  Directory to be used for chrooting process.
    //! \returns exit code of run process.
    //! \throws     SCXInternalErrorException           Failure that could not be prevented
    //! This call will block as long as the process writes to its stdout or stderr
    //! \note   Make sure that "mystdout" and "mystderr" does not block when written to.
    //! 
    //! \date        2007-12-05 15:43:00
    int SCXProcess::Run(const std::wstring &command,
                        std::istream &mystdin,
                        std::ostream &mystdout,
                        std::ostream &mystderr,
                        unsigned timeout,
                        const SCXFilePath& cwd/* = SCXFilePath()*/,
                        const SCXFilePath& chrootPath /* = SCXFilePath() */)
    {
        return Run(SplitCommand(command), mystdin, mystdout,mystderr, timeout, cwd, chrootPath);
    }
    

    /**********************************************************************************/
    //! Class for running a process with an option to timeout 
    class ProcessThreadParam : public SCXThreadParam
    {   
    public:
        //! Constructor that starts a process
        //! \param[in]  process     Process to monitor
        //! \param[in]  timeout     Accepted wait before process terminates
        ProcessThreadParam(SCXProcess *process, unsigned timeout)
            : m_process(process), m_processTerminated(false), m_timeout(timeout)
        {
        }

        /**********************************************************************************/
        //! Wait for the process to terminate
        //! \param[in]  mystdin     Represents stdin of process
        //! \param[in]  mystdout    Represents stdout of process
        //! \param[in]  mystderr    Represents stderr of process
        //! \returns Return code from process being run.
        int WaitForReturn(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr)
        {               
            int returnCode = -1;
            try 
            {
                returnCode =  m_process->WaitForReturn(mystdin, mystdout, mystderr);
                m_processTerminated = true;
            }
            catch (...)
            {
                m_processTerminated = true;
                throw;
            }
            return returnCode;
        }

        /**********************************************************************************/
        //! Wait a limited amount of time for the process to terminate.
        void WaitForReturn() 
        {
            // Good enough way of waiting for the process
            int timeoutLeft = m_timeout;             
            while (timeoutLeft > 0 && !m_processTerminated)
            {
                const int timeBetweenChecks = 1000;
                SCXThread::Sleep(timeBetweenChecks);
                timeoutLeft -= timeBetweenChecks;
            }
            if (!m_processTerminated)
            {
                m_process->Kill();
            }
        }

    private:
        ProcessThreadParam(const ProcessThreadParam &);             //!< Prevent copying               
        ProcessThreadParam &operator=(const ProcessThreadParam &);  //!< Prevent assignment

        SCXProcess *m_process;      //!< Process to run
        bool m_processTerminated;   //!< True iff the process has terminated
        unsigned m_timeout;              //!< Accepted wait before termination
    };

    /**********************************************************************************/
    //! Monitors the process
    //! \param[in]  handle      Parameters of thread
    //! \note Meant to be called in a separate thread by being passed as argument to Thread constructor
    void WaitForReturnFn(SCXCoreLib::SCXThreadParamHandle& handle)
    {        
        ProcessThreadParam* param = static_cast<ProcessThreadParam *>(handle.GetData());
        param->WaitForReturn();
    }

    /**********************************************************************************/
    //! Run a process by passing it arguments and streams for stdin, stdout and stderr
    //! \param[in]  myargv      Arguments corresponding to argv in the main function of the process
    //! \param[in]  mystdin     stdin for the process
    //! \param[in]  mystdout    stdout for the process
    //! \param[in]  mystderr    stderr for the process
    //! \param[in]  timeout     Max number of milliseconds the process is allowed to run (0 means no limit)
    //! \param[in]  cwd         Directory to be set as current working directory for process.
    //! \param[in]  chrootPath  Directory to be used for chrooting process.
    //! \returns exit code of run process.
    //! \throws     SCXInternalErrorException           Failure that could not be prevented
    //! This call will block as long as the process writes to its stdout or stderr
    //! \note   Make sure that "mystdout" and "mystderr" does not block when written to.
    //! 
    //! \date        2007-12-05 15:43:00
    int SCXProcess::Run(const std::vector<std::wstring> &myargv,
                        std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr,
                        unsigned timeout,
                        const SCXFilePath& cwd/* = SCXFilePath()*/,
                        const SCXFilePath& chrootPath /* = SCXFilePath() */) 
    {
        SCXProcess process(myargv, cwd, chrootPath);
        return Run(process, mystdin, mystdout, mystderr, timeout);
    }

    /**********************************************************************************/
    //! Run a process by passing it arguments and streams for stdin, stdout and stderr
    //! \param[in]  process     A process instance to execute.
    //! \param[in]  mystdin     stdin for the process
    //! \param[in]  mystdout    stdout for the process
    //! \param[in]  mystderr    stderr for the process
    //! \param[in]  timeout     Max number of milliseconds the process is allowed to run (0 means no limit)
    //! \returns exit code of run process.
    //! \throws     SCXInternalErrorException           Failure that could not be prevented
    //! This call will block as long as the process writes to its stdout or stderr
    //! \note   Make sure that "mystdout" and "mystderr" does not block when written to.
    //!
    //! \date        2009-05-13 10:20:00
    int SCXProcess::Run(SCXProcess& process, 
                        std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr, unsigned timeout /*= 0*/)
    {
        int returnCode = -1;
        if (timeout <= 0)
        {
            returnCode = process.WaitForReturn(mystdin, mystdout, mystderr);
        }
        else
        {
            ProcessThreadParam *ptp = new ProcessThreadParam(&process, timeout);
            //ptp = 0;
            SCXThreadParamHandle tph(ptp);

            SCXThread t(WaitForReturnFn, tph);
            returnCode = ptp->WaitForReturn(mystdin, mystdout, mystderr);
            t.Wait();
        }
        return returnCode;
    }

    /**********************************************************************************/
    //! Constructor
    //! \param[in]  myargv       Arguments to process
    //! \param[in]  cwd         Directory to be set as current working directory for process.
    //! \param[in]  chrootPath  Directory to be used for chrooting process.

    SCXProcess::SCXProcess(const std::vector<std::wstring> myargv,
                           const SCXFilePath& cwd/* = SCXFilePath()*/,
                           const SCXFilePath& chrootPath /* = SCXFilePath() */) :
            m_stdinChars(1000),
            m_buffer(1000),
            m_stdinCharCount(0),
            m_pid(-1),
            m_processExitCode(-1),
            m_waitCompleted(false),
            m_stdinActive(true),
            m_stdoutActive(true),
            m_stderrActive(true)
    {
        // Convert arguments to types expected by the system function for running processes
        for (std::vector<char *>::size_type i = 0; i < myargv.size(); i++)
        {
            m_cargv.push_back(strdup(StrToMultibyte(myargv[i]).c_str()));
        }
        m_cargv.push_back(0);
        
        // Create pipes for communicating with the child process
        pipe(m_inForChild);
        pipe(m_outForChild);
        pipe(m_errForChild);
        m_pid = fork();                         // Create child process, duplicates file descriptors
        if (m_pid == 0)
        {
            // Child process.
            // The file descriptors are duplicates, created by "fork",  of those in the parent process
            dup2(m_inForChild[R], STDIN_FILENO);      // Make the child process read from the parent process
            close(m_inForChild[R]);                   // Close duplicate
            close(m_inForChild[W]);                   // The child only reads from its stdin
            dup2(m_outForChild[W], STDOUT_FILENO);    // Make the child process write to the parent process
            close(m_outForChild[R]);                  // The child only writes to its stdout
            close(m_outForChild[W]);                  // Close duplicate
            dup2(m_errForChild[W], STDERR_FILENO);    // Make the child process write to the parent process
            close(m_errForChild[R]);                  // The child only writes to its stdout
            close(m_errForChild[W]);                  // Close duplicate

            char error_msg[1024];
            if (L"" != chrootPath.Get())
            {
                if ( 0 != ::chroot(StrToMultibyte(chrootPath.Get()).c_str()))
                {
                    snprintf(error_msg,
                             sizeof(error_msg),
                             "Failed to chroot '%s' errno=%d",
                             StrToMultibyte(chrootPath.Get()).c_str(), errno);
                    DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
                    exit(1);
                }
                if ( 0 != ::chdir("/"))
                {
                    snprintf(error_msg,
                             sizeof(error_msg),
                             "Failed to change root directory. errno=%d", errno);
                    DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
                    exit(1);
                }
            }
            if (L"" != cwd.Get())
            {
                if (0 != ::chdir(StrToMultibyte(cwd.Get()).c_str()))
                {
                    snprintf(error_msg,
                             sizeof(error_msg),
                             "Failed to change cwd. errno=%d", errno);
                    DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
                    exit(1);
                }
            }

            // Close open file descriptors except stdin/out/err
            // (Some systems have UNLIMITED of 2^64; limit to something reasonable)

            int fdLimit = 0;
            fdLimit = getdtablesize();
            if (fdLimit > 2500)
            {
                fdLimit = 2500;
            }

            for (int fd = 3; fd < fdLimit; ++fd)
            {
                close(fd);
            }

            execvp(m_cargv[0], &m_cargv[0]);                // Replace the child process image
            snprintf(error_msg, sizeof(error_msg), "Failed to start child process '%s' errno=%d", m_cargv[0], errno);
            DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);                                // Failed to load correct process.
        } 
        else
        {
            // Same (parent) process.
            // All file descriptors were duplicated in the child process by "fork"
            close(m_inForChild[R]);                   // Only the child process reads from its stdin
            close(m_outForChild[W]);                  // Only the child process writes to its stdout
            close(m_errForChild[W]);                  // Only the child process writes to its stderr
            if (m_pid < 0)
            {
                // Free remaining resources held by the parent process
                // The child process manages its own resources
                close(m_inForChild[W]);
                close(m_outForChild[R]);
                close(m_errForChild[R]);

                // Free everything except the terminating NULL
                for (std::vector<char *>::size_type i = 0; i < m_cargv.size() - 1; i++)
                {
                    free(m_cargv[i]);
                }
                //  No child process was created
                throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
            }

            // Set non-blocking I/O for the output and error channels
            // We need to use this to insure we have all our data from
            // the subprocess ...

            if (-1 == fcntl(m_outForChild[R], F_SETFL, O_NONBLOCK))
            {
                throw SCXInternalErrorException(UnexpectedErrno(L"Failed to set non-blocking I/O on stdout pipe", errno), SCXSRCLOCATION);
            }

            if (-1 == fcntl(m_errForChild[R], F_SETFL, O_NONBLOCK))
            {
                throw SCXInternalErrorException(UnexpectedErrno(L"Failed to set non-blocking I/O on stderr pipe", errno), SCXSRCLOCATION);
            }
        }
    }

    /**********************************************************************************/
    //! Send input to process
    //! \param[in]  mystdin     Source
    //! \returns true if there is possibly more data to send (potentially more in stream)
    //! \throws SCXInternalErrorException On syscall failures.
    bool SCXProcess::SendInput(std::istream &mystdin) {
        // Modify to read all available input - synchronously (blocking)
        std::streamsize stdinCharsRead;

        mystdin.read(&m_stdinChars[m_stdinCharCount], m_stdinChars.size() - m_stdinCharCount);
        stdinCharsRead = mystdin.gcount();

        if (!mystdin.eof() && !mystdin.good()) 
        {
            throw SCXInternalErrorException(L"Process parent communication failed", SCXSRCLOCATION);            
        }

        m_stdinCharCount += static_cast<int>(stdinCharsRead);
        ssize_t bytesWritten = 0;
        if (m_stdinCharCount > 0)
        {
            bytesWritten = DoWrite(m_inForChild[W], &m_stdinChars[0], m_stdinCharCount);
        }

        if (bytesWritten < 0)
        {
            if (EPIPE == errno)
            {
                // If pipe has closed, we're done (nothing to read)
                return false;
            }
            else
            {
                throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
            }
        }
        strncpy(&m_stdinChars[0], &m_stdinChars[bytesWritten], m_stdinCharCount - bytesWritten);
        m_stdinCharCount -= static_cast<int>(bytesWritten);

        return (0 != stdinCharsRead || 0 != m_stdinCharCount);
    }
    
    /**********************************************************************************/
    //! Perform I/O to/from stdin, stdout, and stderr for a process
    //! \param[in]  mystdin     Content that must be sent to the process stdin
    //! \param[in]  mystdout    Receiver of content that the process writes to stdout
    //! \param[in]  mystderr    Receiver of content that the process writes to stderr
    //! \returns true if there is possibly more data to fetch (stderr and/or stdout still open for read).
    bool SCXProcess::InternalPerformIO(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr) {
        const int pollTimeoutInSecs = 2;     // Timeout waiting for subprocess

        // Wait some time for something available to be read from stdout or
        // stderr of the child process.
        //
        // Only try and read stdout/stderr if we haven't previously received
        // an error. If we previously received an error, don't read that.

        // Short circuit and exit if we know that both output pipes are dead
        // (No point in checking input pipe if we can't get output anyway)
        if (!m_stdoutActive && !m_stderrActive)
        {
            return false;
        }

        // We use FDs for poll as follows:
        //  0: processInput (sense if we can write to child process)
        //  1: processOutput (sense if we can read from stdout of child)
        //  2: processError (sense if we can read from stderr of child)

        struct pollfd fds[3];
        fds[0].fd = fds[1].fd = fds[2].fd = -1;
        fds[0].events = POLLOUT;
        fds[1].events = fds[2].events = POLLIN;

        if (m_stdinActive)
        {
            fds[0].fd = m_inForChild[W];
        }
        if (m_stdoutActive)
        {
            fds[1].fd = m_outForChild[R];
        }
        if (m_stderrActive)
        {
            fds[2].fd = m_errForChild[R];
        }

        int pollStatus = poll(fds, 3, pollTimeoutInSecs * 1000);
        if (pollStatus < 0)             /* Error occurred */
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
        }

        if (pollStatus > 0)             /* No timeout */
        {
            // Check if we can write to the child's stdin channel
            if (fds[0].revents & POLLOUT)
            {
                m_stdinActive = SendInput(mystdin);
            }
            if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                m_stdinActive = false;
            }

            // Check if we have data to read from streams
            // (If no data to read, check if stream has closed)
            if (fds[1].revents & POLLIN)
            {
                ReadToStream(m_outForChild[R], mystdout);
            }
            if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                m_stdoutActive = false;
            }

            if (fds[2].revents & POLLIN)
            {
                ReadToStream(m_errForChild[R], mystderr);
            }
            if (fds[2].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                m_stderrActive = false;
            }
        }

        return m_stdoutActive || m_stderrActive;
    }

    /**********************************************************************************/
    //! Perform I/O to/from stdin, stdout, and stderr for a process
    //! \param[in]  mystdin     Content that must be sent to the process stdin
    //! \param[in]  mystdout    Receiver of content that the process writes to stdout
    //! \param[in]  mystderr    Receiver of content that the process writes to stderr
    //! \returns true if there is possibly more data to fetch (process still executing 
    //!          and stderr and/or stdout still open for read.
    bool SCXProcess::PerformIO(
        std::istream &mystdin,
        std::ostream &mystdout,
        std::ostream &mystderr)
    {
        // Just read until we're told that both mystdout and mystderr are dead

        bool retval = InternalPerformIO(mystdin, mystdout, mystderr);

        // On some systems (AIX, Solaris and HPUX) a process can die without closing STDERR
        if (DoWaitPID(NULL, false) != 0)
        {
            // On at least one of our systems (AIX 6.1 build host) we have seen that
            // we need to do one more try to select and read to get all of STDERR.
            InternalPerformIO(mystdin, mystdout, mystderr);
            return false;
        }

        return retval;
    }

    /**********************************************************************************/
    //! Wait for the process to return, that is, terminate normally or by beeing signaled
    //! \returns Exit code of process being waited for.
    //! \throws     SCXInternalErrorException       Did not succeed to wait for the process
    int SCXProcess::WaitForReturn() 
    {
        int child_status = 0;
        if (m_pid != DoWaitPID(&child_status, true))
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"Failed to wait for child process", errno), SCXSRCLOCATION);
        }
        if (!WIFEXITED(child_status)) 
        {
            throw SCXInterruptedProcessException(SCXSRCLOCATION);
        }
        return WEXITSTATUS(child_status);       
    }

    /**********************************************************************************/
    //! Interacts with the process while waiting for the it to return, 
    //! that is, terminate normally or by beeing signaled
    //! \param[in]  mystdin     Stdin of process
    //! \param[in]  mystdout    Stdout of process
    //! \param[in]  mystderr    Stderr of process
    int SCXProcess::WaitForReturn(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr)
    {
        bool fetched = true;
        while (fetched)
        {
            fetched = PerformIO(mystdin, mystdout, mystderr);
        }

        return WaitForReturn();
    }

    /**********************************************************************************/
    //! Destructor
    SCXProcess::~SCXProcess() 
    {
        // Free remaining resources held by the parent process
        // The child process manages its own resources
        close(m_inForChild[W]);
        close(m_outForChild[R]);
        close(m_errForChild[R]);

        // Free everything except the terminating NULL
        for (std::vector<char *>::size_type i = 0; i < m_cargv.size() - 1; i++) 
        {
            free(m_cargv[i]);
        }
    }

    /**********************************************************************************/
    //! Terminate the process
    void SCXProcess::Kill() 
    {
        if (kill(m_pid, SIGKILL) < 0 && errno != ESRCH) 
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"Unable to kill child process", errno), SCXSRCLOCATION);
        }
    }
    
    /**********************************************************************************/
    //! Read available data from socket and put it in the stream.
    //! \param fd Socket/file descriptor to read from.
    //! \param stream Stream to write data to.
    //! \returns true if there might be more data to read or false if the socket is closed.
    //! \throws SCXInternalErrorException if the socket read fails.
    bool SCXProcess::ReadToStream(int fd, std::ostream& stream)
    {
        // The pipe is now opened non-blocking, so keep reading until no bytes
        // are read (we won't block). Non-blocking reads are necessary to avoid
        // a race condition at child process shutdown: If we did a partial read
        // and then the child process ended, remaining bytes can be discarded.

        ssize_t bytesRead;
        do
        {
            bytesRead = read(fd, &m_buffer[0], m_buffer.size());
            if (bytesRead == 0) {
                return false;
            } else if (bytesRead < 0) {
                // If no data is available for reading, just return
                if (EAGAIN == errno)
                {
                    return true;
                }

                throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
            }
            stream.write(&m_buffer[0], bytesRead);
        } while (true);

        return true;
    }
    
    /**********************************************************************************/
    //! Thin wrapper around write system call.
    //! \param fd File descriptor to write to.
    //! \param buf Buffer with data.
    //! \param size Size of data in buffer.
    //! \returns Number of bytes written or negative for errors.
    ssize_t SCXProcess::DoWrite(int fd, const void* buf, size_t size)
    {
        return write(fd, buf, size);
    }

    /**********************************************************************************/
    //! Wrapper around the waitpid systemcall. Caches the result so you can call this
    //! method several times.
    //! \param[out] pExitCode If not null the content of this pointer will be set to the
    //!                       exit code of the process if it has exited.
    //! \param[in] blocking True if caller want to block while waiting otherwise false.
    //! \returns The PID of the process being waited for if it has exited, zero if process
    //!          is still running or negative for errors (check errno).
    int SCXProcess::DoWaitPID(int* pExitCode, bool blocking)
    {
        SCXProcessId pid = 0;
        if (m_waitCompleted)
        {
            pid = m_pid;            
        }
        else
        {
            pid = waitpid(m_pid, &m_processExitCode, blocking?0:WNOHANG);
            if (pid == m_pid)
            {
                m_waitCompleted = true;
            }
        }

        if (pExitCode != NULL)
        {
            *pExitCode = m_processExitCode;
        }
        return pid;
    }

#endif /* SCX_UNIX */


} /* namespace SCXCoreLib */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
