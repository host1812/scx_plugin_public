/*------------------------------------------------------------------------------
    Copyright (C) 2007, Microsoft Corp.
    
*/
/**
    \file
 
    \brief    Implementation for boolean prediate used in WQL
 
    \author    
    \date      
 
*/
/*----------------------------------------------------------------------------*/
#include "scxproviderlib/predicate.h"
#include "scxcorelib/stringaid.h"
#include "scxcorelib/scxdumpstring.h"

#include "Pegasus/Provider/CMPI/cmpift.h"
#include "Pegasus/Provider/CMPI/cmpimacs.h"

using namespace SCXCoreLib;
using namespace std;

namespace SCXProviderLib
{
        Predicate::Predicate()
        {
                this->m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.wqlsupport");
        }

        std::wstring Predicate::DumpString() const
        {
                return SCXDumpStringBuilder("Predicate")
                           .Instance("left-operand", this->lhs)
                           .Scalar("operator", this->op)
                           .Instance("right-operand", this->rhs);
        }
}
