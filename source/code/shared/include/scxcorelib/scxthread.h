/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Defines the public interface for the thread PAL.
    \date        2007-06-19 14:00:00

    \ex
    \code
    class ThreadLockParam : public SCXCoreLib::SCXThreadParam
    {
    public:
        ThreadLockParam(const SCXCoreLib::SCXThreadLockHandle& lock) : m_lock(lock){ }
        const SCXCoreLib::SCXThreadLockHandle& GetLockHandle() const { return m_lock; }
    protected:
        SCXCoreLib::SCXThreadLockHandle m_lock;
    };

    void SimpleThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        ThreadLockParam* pl = dynamic_cast<const ThreadLockParam*>(param->GetData());
        SCXCoreLib::SCXThreadLock l(pl->GetLockHandle());
    }

    ThreadLockParam* pl = new ThreadLockParam(SCXCoreLib::ThreadLockHandleGet())
    SCXCoreLib::SCXThreadParamHandle p(pl);
    SCXCoreLib::SCXThreadLock l(pl->GetLockHandle());
    SCXCoreLib::SCXThread thread(SCXThreadTest::SimpleThreadBody, p);
    \endcode

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXTHREAD_H
#define SCXTHREAD_H

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxhandle.h>
#include <map>

#if defined(WIN32)
#include <windows.h>
#elif defined(linux) | defined(sun) | defined(hpux) | defined(aix)
#include <pthread.h>
#endif

namespace SCXCoreLib
{
    /** Definition of a thread id. */

#if defined(WIN32)
    typedef DWORD SCXThreadId;
#elif defined(SCX_UNIX)
    typedef pthread_t SCXThreadId;
#else
#error Not implemented on this platform
#endif

    /*----------------------------------------------------------------------------*/
    /**
        Represents all parameters for a thread.

        Wrap all thread params in an object whith name mapping in stead
        of having each thread having their own special struct.

    */
    class SCXThreadParam
    {
    public:
        SCXThreadParam();
        virtual ~SCXThreadParam();
        const std::wstring DumpString() const;

        const std::wstring& GetString(const std::wstring& key) const;
        void SetString(const std::wstring& key, const std::wstring& value);

        /*-----------------------------------------------------------------------*/
        /**
           Get value of termination flag.

           \returns     true if termination flag is set, otherwise false.

           Threads supporting graceful termination using RequestTerminate should
           use this method to see if termination has been requested or not.
        */
        bool GetTerminateFlag() const { return m_terminateFlag; }

        /*-----------------------------------------------------------------------*/
        /**
           Set the termination flag.

           When we need to terminate, this method is called when signalling condition.
        */
        void SetTerminateFlag() { m_terminateFlag = true; }

    public:
        SCXCondition m_cond;         //!< Condition class to assist in timely shutdown

    protected:
        SCXThreadLockHandle m_lock;  //!< Handles locking of the thread param.
        bool m_terminateFlag;        //!< Set this flag to terminate the thread gracefully.

    private:
        std::map<std::wstring, std::wstring> m_stringValues; //!< Thread parameters represented as strings.
    };

    /** Reference counted thread parameter handle. */
    typedef SCXHandle<SCXThreadParam> SCXThreadParamHandle;

    /** Function pointer for thread bodies. */
    typedef void (*SCXThreadProc)(SCXThreadParamHandle&);

    /*----------------------------------------------------------------------------*/
    /**
        Represents a reference to a thread.

        Provide a reference to thread instances.

    */
    class SCXThread
    {
    private:
        SCXThreadId m_threadID;             //!< Id of thread.
        SCXThreadParamHandle m_paramHandle; //!< Thread parameters.
#if defined(WIN32)
        SCXHandle<HANDLE> m_threadHandle;   //!< Cached thread handle.
#endif
        bool m_threadMaySurviveDestruction;      //!< Whether thread lifetime is managed or not

        /** Prevent multiple implicit handles to the same thread, use SCXHandle */
        SCXThread(const SCXThread&);
    protected:
        void SCXThreadStartHelper(SCXThreadProc proc);
    public:
        SCXThread();
        SCXThread(SCXThreadProc proc, SCXThreadParam* param = 0);
        SCXThread(SCXThreadProc proc, const SCXThreadParamHandle& param);
        virtual ~SCXThread();
        const std::wstring DumpString() const;

        void Start(SCXThreadProc proc, SCXThreadParam* param = 0);
        void Start(SCXThreadProc proc, const SCXThreadParamHandle& param);

        SCXThreadId GetThreadID() const;
        SCXThreadParamHandle& GetThreadParam();

        bool IsAlive() const;

        void RequestTerminate();

        void Wait();

        static void Sleep(scxulong milliseconds);
        static SCXThreadId GetCurrentThreadID();
    };

    /*----------------------------------------------------------------------------*/
    /**
        Base class for thread exceptions.

        Adds thread ID tracking to the exception.

    */
    class SCXThreadException : public SCXException
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] l Source code location.

        */
        SCXThreadException(const SCXCodeLocation& l)
            : SCXException(l)
        {
            m_id = SCXThread::GetCurrentThreadID();
        }

        /*----------------------------------------------------------------------------*/
        /**
            Get thread id

            \returns Thread id.

        */
        SCXThreadId GetThreadID()
        {
            return m_id;
        }
    protected:
        SCXThreadId m_id; //!< Thread id.
    };

    /*----------------------------------------------------------------------------*/
    /**
        Indicates use of a thread param that does not exist.

        Thrown when a non-existing thread param value is queried.

    */
    class SCXInvalidThreadParamValueException : public SCXThreadException
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] value Faulting thread parameter value.
            \param[in] l Source code location.

        */
        SCXInvalidThreadParamValueException(const std::wstring& value, const SCXCodeLocation& l)
            : SCXThreadException(l)
        {
            m_value = value;
        }

        virtual std::wstring What() const
        {
            return L"Invalid param value: " + m_value;
        }
    protected:
        std::wstring m_value;  //!< Faulting thread parameter value.
    };

    /*----------------------------------------------------------------------------*/
    /**
        Thrown when threads cannot be started.

        Used to indicate thread start failures.

    */
    class SCXThreadStartException : public SCXThreadException
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason why thread could not be started.
            \param[in] l Source code location.

        */
        SCXThreadStartException(const std::wstring& reason, const SCXCodeLocation& l)
            : SCXThreadException(l)
        {
            m_reason = reason;
        }

        virtual std::wstring What() const
        {
            return L"Failed to start thread: " + m_reason;
        }
    protected:
        std::wstring m_reason; //!< Reason why thread could not be started.
    };

} /* namespace SCXCoreLib */

#endif /* SCXTHREAD_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
