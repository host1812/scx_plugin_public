/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**
   \file

   \brief      scx cim configuration tool for SCX.

   \date       8/27/2008

*/
#ifndef _CIM_CONFIGURATOR_H
#define _CIM_CONFIGURATOR_H

#include <scxcorelib/scxcmn.h>
#include "admin_api.h"

#include <iostream>

namespace SCXCoreLib
{
    /**
       Class for administration of Pegasus via scxadmin tool
     */
    class SCX_CimConfigurator : public SCX_AdminLogAPI {
    public:
        SCX_CimConfigurator();
        ~SCX_CimConfigurator();

        bool LogRotate();
        bool Print(std::wostringstream& buf) const;
        bool Reset();
        bool Set(LogLevelEnum level);

    protected:
        bool PrintInternal(std::wostringstream& buf, std::ostringstream &mystdout, std::ostringstream &mystderr) const;
        bool ResetInternal(std::ostringstream &mystdout, std::ostringstream &mystderr);
        bool SetInternal(LogLevelEnum level, std::ostringstream &mystdout, std::ostringstream &mystderr);
        virtual void Execute(
            const std::wstring &command,
            std::ostringstream &mystdout,
            std::ostringstream &mystderr,
            bool fErrorOK = false) const;

    private:
        bool ParseGetConfig(std::wstring &value, std::ostringstream &myOutput, std::ostringstream &myError) const;
    };
}

#endif /* _CIM_CONFIGURATOR_H */
