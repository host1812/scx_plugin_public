/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       PAL representation of a Tomcat Application Server

   \date        11-05-18 12:00:00
*/
/*----------------------------------------------------------------------------*/
#ifndef TOMCATAPPSERVERINSTANCE_H
#define TOMCATAPPSERVERINSTANCE_H

#include <string>

#include "appserverinstance.h"

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Class representing all external dependencies from the AppServer PAL.

    */
    class TomcatAppServerInstancePALDependencies
    {
    public:
        virtual SCXCoreLib::SCXHandle<std::istream> OpenVersionFile(std::wstring filename);
        virtual SCXCoreLib::SCXHandle<std::istream> OpenXmlServerFile(std::wstring filename);
        virtual ~TomcatAppServerInstancePALDependencies() {};
    };

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents an instances.

       Concrete implementation of an instance of a Tomcat Application Server

    */
    class TomcatAppServerInstance : public AppServerInstance
    {
        friend class AppServerEnumeration;

    public:

        TomcatAppServerInstance(
            std::wstring id, 
            std::wstring homePath,
            SCXCoreLib::SCXHandle<TomcatAppServerInstancePALDependencies> deps = SCXCoreLib::SCXHandle<TomcatAppServerInstancePALDependencies>(new TomcatAppServerInstancePALDependencies()));
        virtual ~TomcatAppServerInstance();

        virtual void Update();

    private:

        void UpdateVersion();
        void UpdatePorts();
        void GetStringFromStream(SCXCoreLib::SCXHandle<std::istream> mystream, std::string& content);

        std::wstring m_homePath;

        SCXCoreLib::SCXHandle<TomcatAppServerInstancePALDependencies> m_deps;
    };

}

#endif /* TOMCATAPPSERVERINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
