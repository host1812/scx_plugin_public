/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief     Implementation of SCXWQLSelectStatementCMPI.

*/
#include "scxwqlselectstatementcmpi.h"
#include "scxproviderlib/scxproperty.h"
#include "scxcorelib/scxwql.h"
#include "scxcorelib/scxhandle.h"

using namespace SCXCoreLib;
using namespace std;

namespace SCXProviderLib
{
    // CMPI implementation

    SCXWQLSelectStatementCMPI::SCXWQLSelectStatementCMPI(const std::wstring& queryString, CMPIBroker *broker) :  SCXWQLSelectStatementBase(queryString)
    {
        if( NULL == broker)
        {
            throw SCXInternalErrorException(L"Broker passed into select statement is null.", SCXSRCLOCATION);
        }
        this->cmpiBroker = broker;
        this->cmpiExp = NULL;
        this->cmpiProjection = NULL;
    }

    SCXWQLSelectStatementCMPI::~SCXWQLSelectStatementCMPI()
    {
        if(NULL != cmpiProjection)
        {
            this->cmpiProjection->ft->release(cmpiProjection);
            this->cmpiProjection = NULL;
        }
        if(NULL != cmpiExp)
        {
            this->cmpiExp->ft->release(cmpiExp);
            this->cmpiExp = NULL;
        }
    }

    SCXHandle<Condition> CreateCondition(CMPISubCond *c);

    void SCXWQLSelectStatementCMPI::SetExp(CMPISelectExp *exp)
    {
        SCX_LOGTRACE(this->m_log, L"SetExp(CMPISelectExp *exp)");
        if(exp)
        {
            cmpiExp = exp;
            CMPIStatus st;
            CMPISelectCond *cond = exp->ft->getDOC(exp, &st);
            if(CMPI_RC_OK != st.rc)
            {
                throw SCXInternalErrorException(L"GetDoc failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
            }

            unsigned i = cond->ft->getCountAndType(cond, NULL, &st);
            if(CMPI_RC_OK != st.rc)
            {
                throw SCXInternalErrorException(L"GetCountAndType failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
            }

            for (unsigned j = 0; j< i; j++)
            {
                SCXHandle<Condition> c = CreateCondition(cond->ft->getSubCondAt(cond, j, &st));
                if(CMPI_RC_OK != st.rc)
                    throw SCXInternalErrorException(L"getSubCondAt failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
                this->doc.push_back(*c);
            } 
        }
        SCX_LOGTRACE(this->m_log, L"Exit SetExp(CMPISelectExp *exp)");
    }

    void SCXWQLSelectStatementCMPI::SetProjection(CMPIArray *projectionPtr)
    {
        SCX_LOGTRACE(this->m_log, L"SetProjection(CMPIArray *projectionPtr)");
        if(projectionPtr)
        {
            cmpiProjection = projectionPtr;
            CMPIStatus st;
            unsigned i = projectionPtr->ft->getSize(projectionPtr, &st);
            if(CMPI_RC_OK != st.rc)
            {
                throw SCXInternalErrorException(L"GetSize failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
            }

            for (unsigned j = 0; j< i; j++)
            {
                CMPIData d = projectionPtr->ft->getElementAt(projectionPtr, j, &st);
                if(CMPI_RC_OK != st.rc)
                {
                    throw SCXInternalErrorException(L"getElementAt failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
                }

                if(CMPI_string ==  d.type) 
                {
                    SCXHandle<SCXProperty> p(new SCXProperty(StrFromUTF8(d.value.string->ft->getCharPtr(d.value.string, &st)), 
                        StrFromUTF8(d.value.string->ft->getCharPtr(d.value.string, &st))));
                    this->projection.push_back(*p);
                }
                else
                {
                    throw SCXInternalErrorException(L"Projection contains property which is not a string type", SCXSRCLOCATION);
                }
            }
        }
        SCX_LOGTRACE(this->m_log, L"Exit SetProjection(CMPIArray *projectionPtr)");
    }

    void SCXWQLSelectStatementCMPI::Parse()
    {
        SCX_LOGTRACE(this->m_log, L"Parse()");
        CMPIArray *projectionPtr; 
        CMPIStatus status;
        CMPISelectExp *exp = CMNewSelectExp(cmpiBroker, StrToUTF8(query).c_str(), "WQL", &projectionPtr, &status);
        if(CMPI_RC_OK != status.rc) 
        {
            if(CMPI_RC_ERR_QUERY_LANGUAGE_NOT_SUPPORTED == status.rc)
            {
                throw new SCXInternalErrorException(L"Get CMPI select statement faild. Invalid language specification: " + StrFromUTF8(status.msg->ft->getCharPtr(status.msg, NULL) ? status.msg->ft->getCharPtr(status.msg, NULL) : ""), SCXSRCLOCATION); 
            }

            if(CMPI_RC_ERR_INVALID_QUERY == status.rc) 
            {
                throw new SCXParseException(L"Get CMPI select statement faild. Invalid query string: " + StrFromUTF8(status.msg->ft->getCharPtr(status.msg, NULL) ? status.msg->ft->getCharPtr(status.msg, NULL) : ""), SCXSRCLOCATION); 
            }

            throw new SCXInternalErrorException(L"Get CMPI select statement faild: " + StrFromUTF8(status.msg->ft->getCharPtr(status.msg, NULL) ? status.msg->ft->getCharPtr(status.msg, NULL) : ""), SCXSRCLOCATION); 
        }
        this->SetExp(exp);
        this->SetProjection(projectionPtr);
        SCX_LOGTRACE(this->m_log, L"Exit Parse()");
    }
}
