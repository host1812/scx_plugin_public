/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains implementation of the SCXodm class for AIX.
    
    \date        2011-08-15 16:05:00

    
*/
/*----------------------------------------------------------------------------*/

// Only for AIX!

#if defined(aix)

#include <scxsystemlib/scxodm.h>
#include <scxcorelib/stringaid.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    
        Creates a SCXodmDependencies object.
    */
    SCXodmDependencies::SCXodmDependencies()
        : m_fInitialized(false)
    {
        m_lock = SCXCoreLib::ThreadLockHandleGet(L"SCXSystemLib::SCXodmDependencies");
    }

    /** Virtual destructor */
    SCXodmDependencies::~SCXodmDependencies()
    {
        if ( m_fInitialized )
        {
            Terminate();
        }
    }

    /** Initialize the ODM database accessor functions.
        \returns Result of the odm_initialize() call
    */
    int SCXodmDependencies::Initialize()
    {
        SCXASSERT( ! m_fInitialized );

        m_lock.Lock();
        int status = odm_initialize();
        if ( 0 != status )
        {
            throw SCXodmException(L"odm_initialize failed", odmerrno, SCXSRCLOCATION);
        }
        m_fInitialized = true;
        return status;
    }

    /** Terminate the ODM database accessor functions.
        \returns Result of the odm_terminate() call
    */
    int SCXodmDependencies::Terminate()
    {
        SCXASSERT( m_fInitialized );

        int status = odm_terminate();
        if ( 0 != status )
        {
            throw SCXodmException(L"odm_terminate failed", odmerrno, SCXSRCLOCATION);
        }
        m_fInitialized = false;
        m_lock.Unlock();
        return status;
    }

    /** Gets (first) information from the ODM database.
        \param cs           Pointer to class symbol (generally ClassName_CLASS from header)
        \param criteria     Search criteria (string containing qualifying search criteria)
        \param returnData   Pointer to data structure (from header) returned for this class
        \returns -1 on error (odmerrno is set), NULL if no match is found
    */
    void *SCXodmDependencies::GetFirst(CLASS_SYMBOL cs, char *criteria, void *returnData) {
        SCXASSERT( m_fInitialized );
        return odm_get_first(cs, criteria, returnData);
    }

    /** Gets (next) information from the ODM database.
        \param cs           Pointer to class symbol (generally ClassName_CLASS from header)
        \param returnData   Pointer to data structure (from header) returned for this class
        \returns -1 on error (odmerrno is set), NULL if no match is found
    */
    void *SCXodmDependencies::GetNext(CLASS_SYMBOL cs, void *returnData) {
        SCXASSERT( m_fInitialized );
        return odm_get_next(cs, returnData);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    
        Creates a SCXodm object.
    */
    SCXodm::SCXodm()
        : m_deps(0),
          m_fGetFirst(true)
    {
        m_deps = new SCXodmDependencies();

        m_deps->Initialize();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    
        Note that distructor for m_deps will get called, causing cleanup of it's
        structures (and calling odm_terminate() as needed).
    */
    SCXodm::~SCXodm()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
         Dump object as string (for logging).
    
         Parameters:  None
         Retval:      String representation of object.
    */
    std::wstring SCXodm::DumpString() const
    {
        return L"SCXodm: <No data>";
    }

    /** Gets information from the ODM database.
        \param cs           Pointer to class symbol (generally ClassName_CLASS from header)
        \param criteria     Search criteria (string containing qualifying search criteria)
        \param returnData   Pointer to data structure (from header) returned for this class

        \throws  SCXodmException if odm internal error occurred
        \returns NULL if no match is found
    */
    void *SCXodm::Get(CLASS_SYMBOL cs, const std::wstring &wCriteria, void *returnData)
    {
        void *pData;

        if (m_fGetFirst)
        {
            // Convert criteria into a form for the odm_get_* functions (non-const C-style string)
            std::string sCriteria = SCXCoreLib::StrToMultibyte(wCriteria);
            std::vector<char> criteria(sCriteria.c_str(), sCriteria.c_str() + sCriteria.length() + 1);

            pData = m_deps->GetFirst(cs, &criteria[0], returnData);
            m_fGetFirst = false;
        }
        else
        {
            pData = m_deps->GetNext(cs, returnData);
        }

        if ((int) pData == -1)
        {
            throw SCXodmException(L"odm_get_first/odm_get_next failed", odmerrno, SCXSRCLOCATION);
        }
        else if (NULL == pData)
        {
            // If there is no "next", then reset so next Get() call will be first

            m_fGetFirst = true;
        }

        return pData;
    }

    /*----------------------------------------------------------------------------*/
    /**
         Format details of violation
    */
    std::wstring SCXodmException::What() const 
    { 
        std::wstring s = L"SCXodm error: ODM error because ";
        s.append(m_Reason);
        s.append(L": ODM error ");
        s.append(SCXCoreLib::StrFrom(GetErrno()));
        return s;
    }
} // SCXSystemLib

#endif // defined(aix)
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
