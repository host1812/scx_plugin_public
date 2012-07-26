/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief     Implementation of SCXWQLSelectStatementBase and SCXWQLSelectStatement.

*/

#include "scxwqlselectstatementcmpi.h"
#include "scxproviderlib/scxwqlselectstatement.h"
#include "scxcorelib/scxdumpstring.h"
#include "scxcorelib/scxassert.h"

using namespace std;
using namespace SCXCoreLib;

namespace SCXProviderLib
{
        SCXWQLSelectStatementBase::SCXWQLSelectStatementBase(const wstring& queryString)
        {
                this->query = queryString;
                this->m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.wqlsupport");
        }

        void SCXWQLSelectStatementBase::ApplyFilter(SCXInstance& instance)
        {
                for (size_t i = 0; i < this->projection.size(); i++)
                {
                        instance.AddFilter(this->projection[i].GetName());
                }
        }

        std::wstring SCXWQLSelectStatementBase::DumpString() const
        {
                return SCXDumpStringBuilder("SCXWQLSelectStatementBase")
                           .Instances("projections", this->projection)
                           .Instances("conditions", this->doc);
        }

        SCXWQLSelectStatement::SCXWQLSelectStatement(const std::wstring& queryString, SCXWQLSelectStatementBase *internalInstance) : SCXWQLSelectStatementBase(queryString)
        {
                if(!internalInstance)
                {
                        throw SCXInternalErrorException(L"internalInstance is NULL.", SCXSRCLOCATION);
                }

                this->instance = internalInstance;
        }


        SCXWQLSelectStatement::SCXWQLSelectStatement(const wstring& queryString, CMPIBroker *broker) : SCXWQLSelectStatementBase(queryString)
        {
                this->instance = new SCXWQLSelectStatementCMPI(queryString, broker);
        }

        void SCXWQLSelectStatement::Parse()
        {
                this->instance->Parse();
                this->className = this->instance->GetClassName();
                this->doc = this->instance->GetWhereClauseDOC();
                this->projection = this->instance->GetProjection();
        }

        void SCXWQLSelectStatement::Analyze()
        {
                this->instance->Analyze();
        }

        SCXWQLSelectStatement::~SCXWQLSelectStatement()
        {
                SCXASSERT(this->instance != NULL);
                delete this->instance;
        }
}
