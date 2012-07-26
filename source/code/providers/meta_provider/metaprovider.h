/*-------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     Meta provider header file

    \date      2008-02-01 09:35:36

*/
/*----------------------------------------------------------------------------*/
#ifndef METAPROVIDER_H
#define METAPROVIDER_H

#include <string>

#include <scxproviderlib/cmpibase.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxtime.h>
#include <scxsystemlib/scxostypeinfo.h>

namespace SCXCore
{

    /*----------------------------------------------------------------------------*/
    /**
       Meta provider

       Concrete instance of the CMPI BaseProvider delivering CIM
       information about the scx installation on current host.

       A provider-specific thread lock will be held at each call to
       the Do* methods, so this implementation class does not need
       to worry about that.

    */
    class MetaProvider : public SCXProviderLib::BaseProvider
    {
    public:
        MetaProvider();
        ~MetaProvider();

        virtual const std::wstring DumpString() const;

    protected:
        //! The set of CIM classes this provider supports
        enum SupportedCimClasses {
            eSCX_Agent   //!< Agent
        };

        // Overrides from the base class with relevant implementations
        virtual void DoInit();
        virtual void DoCleanup();
        virtual void DoEnumInstanceNames(const SCXProviderLib::SCXCallContext& callContext,
                                         SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoEnumInstances(const SCXProviderLib::SCXCallContext& callContext,
                                     SCXProviderLib::SCXInstanceCollection &instances);
        virtual void DoGetInstance(const SCXProviderLib::SCXCallContext& callContext,
                                   SCXProviderLib::SCXInstance& instance);

    private:
        void AddKeys(SCXProviderLib::SCXInstance& inst);
        void AddProperties(SCXProviderLib::SCXInstance& inst);

        void ReadInstallInfoFile();
        void GetReleaseDate();

        //! Set to true if build time could be parsed OK
        bool m_buildTimeOK;
        //! build time set at compile time
        SCXCoreLib::SCXCalendarTime m_buildTime;
        //! Set to true if all info could be read from the install info file
        bool m_readInstallInfoFile;
        //! Install time read from install info file
        SCXCoreLib::SCXCalendarTime m_installTime;
        //! Install version read from install info file
        std::wstring m_installVersion;

        //! Keep an instance of class with static information about OS type 
        SCXSystemLib::SCXOSTypeInfo  m_osTypeInfo;

    };
}

#endif /* METAPROVIDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
