/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     LogFile provider header file
 
    \date      2008-0-08 09:35:36
 
*/
/*----------------------------------------------------------------------------*/
#ifndef LOGFILEPROVIDER_H
#define LOGFILEPROVIDER_H

#include <scxproviderlib/cmpibase.h>
#include "logfileutils.h"

namespace SCXCore
{
    /*----------------------------------------------------------------------------*/
    /**
       LogFile provider
   
       Concrete instance of the CMPI BaseProvider delivering CIM
       information about content in log files on current host. 

       A provider-specific thread lock will be held at each call to 
       the Do* methods, so this implementation class does not need 
       to worry about that. 

    */
    class LogFileProvider : public SCXProviderLib::BaseProvider
    {
    public:
        LogFileProvider();
        LogFileProvider(SCXCoreLib::SCXHandle<LogFileReader> pReader);
        ~LogFileProvider();

        virtual const std::wstring DumpString() const;

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_LogFile,          //!< LogFile
            eSCX_LogFileRecord     //!< LogFileRecord
        };

        /** CIM methods supported */
        enum SupportedCimMethods {
            eGetMatchedRowsMethod    //!< GetMatchedRowsMethod
        };

        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        virtual void DoCleanup();
        virtual void DoEnumInstanceNames(const SCXProviderLib::SCXCallContext& callContext, 
                                         SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext, 
                                     SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoExecQuery(const SCXProviderLib::SCXCallContext& callContext, SCXProviderLib::SCXInstanceCollection& instances,
                                 std::wstring query, std::wstring language);
        virtual void DoInvokeMethod(const SCXProviderLib::SCXCallContext& callContext,
                                    const std::wstring& methodname, const SCXProviderLib::SCXArgs& args,
                                    SCXProviderLib::SCXArgs& outargs, SCXProviderLib::SCXProperty& result);

    private:
        void InitializeObject();
        bool InvokeLogFileReader(const std::wstring& filename,
                                 const std::wstring& qid,
                                 const std::vector<SCXCoreLib::SCXRegexWithIndex>& regexps,
                                 bool fPerformElevation,
                                 std::vector<std::wstring>& matchedLines);

        SCXCoreLib::SCXHandle<LogFileReader> m_pLogFileReader;
    };
}

#endif /* LOGFILEPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
