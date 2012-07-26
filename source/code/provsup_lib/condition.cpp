/*------------------------------------------------------------------------------
Copyright (C) 2007, Microsoft Corp.

*/
/**
\file

\brief    Implementation for boolean condition used in WQL

\author    
\date      

*/
/*----------------------------------------------------------------------------*/
#include <string>
#include <vector>

#include <Pegasus/Provider/CMPI/cmpidt.h>
#include <Pegasus/Provider/CMPI/cmpift.h>
#include <Pegasus/Provider/CMPI/cmpimacs.h>

#include "scxcorelib/scxexception.h"
#include "scxcorelib/scxwql.h"
#include "scxcorelib/scxhandle.h"
#include "scxproviderlib/condition.h"

using namespace SCXCoreLib;
using namespace std;
using namespace SCXProviderLib;

namespace
{
    /*----------------------------------------------------------------------------*/
    /**
    *  \fn SCXHandle<Predicate> CreatePredicate(CMPIPredicate *p)
    *  \brief Create a Predicate from a CMPI predicate pointer.
    *  \param[in] p: The pointer to the CMPI predicate
    *  \returns The pointer to the predicate object.
    */
    SCXHandle<Predicate> CreatePredicate(CMPIPredicate *p)
    {
        SCXHandle<Predicate> retVal(new Predicate());
        SCXCoreLib::SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.wqlsupport");
        SCX_LOGTRACE(log, L"CreatePredicate(CMPIPredicate *p)");
        if(p)
        {
            // There seems to be no way to distinguish between {3 = Path, 3 = "Path", Path = 3} or between {"test" = "Path", "test" = Path, Path = "test", "Path" = test}
            // In the first cases the type of the predicate is 176 (integer) and the two operands are in the string form '3' and 'Path'. In the second case the type 
            // of the predicate is 5632 (string) and the operands are in the string form 'test' and 'Path'.
            // Because of this loss of information we assume that a predicate is always written in the form "propertyName = literalValue".  Predicates
            // not written in this form might be interpreted incorrectly either resulting an parsing error of unintended interpretation of the user intention.
            // E.g. "NameValue"  = Name might be interpreted as NameValue = "Name". 

            CMPIType type;
            CMPIString *l, *r;
            CMPIPredOp op;
            CMPIStatus st =  p->ft->getData(p, &type, &op, &l, &r);
            if(CMPI_RC_OK != st.rc)
            {
                throw SCXInternalErrorException(L"getData failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
            }

            retVal->SetOperation(op);
            retVal->GetLeftOperand().SetName(StrFromUTF8(l->ft->getCharPtr(l, &st)));
            retVal->GetLeftOperand().SetValue(StrFromUTF8(l->ft->getCharPtr(l, &st)));
            if(CMPI_RC_OK != st.rc)
            {
                throw SCXInternalErrorException(L"getCharPtr failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
            }

            retVal->GetRightOperand().SetName(L"");
            switch(type)
            {
            case CMPI_charString:
                retVal->GetRightOperand().SetValue(StrFromUTF8(r->ft->getCharPtr(r, &st)));
                break;
            case CMPI_nameString:
                retVal->GetRightOperand().SetName(StrFromUTF8(r->ft->getCharPtr(r, &st)));
                retVal->GetRightOperand().SetValue(StrFromUTF8(r->ft->getCharPtr(r, &st)));
                break;
            case CMPI_uint64:
                try {
                    retVal->GetRightOperand().SetValue(StrToULong(StrFromUTF8((r->ft->getCharPtr(r, &st)))));
                    //      retVal->GetRightOperand().SetValue(std::atoi(r->ft->getCharPtr(r, &st))); // atoi will return 0 if the string is ill formatted
                }
                catch(SCXNotSupportedException& e)
                {
                    throw SCXParseException(L"Predicate type is Int but the right operand is not a int literal: " + StrFromUTF8(l->ft->getCharPtr(l, &st)), SCXSRCLOCATION);
                }
                break;
            case CMPI_boolean:
                if(0 == StrCompare(StrFromUTF8(r->ft->getCharPtr(r, &st) ? r->ft->getCharPtr(r, &st): ""), L"TRUE"))
                {
                    retVal->GetRightOperand().SetValue(true);
                }
                else if(0 == StrCompare(StrFromUTF8(r->ft->getCharPtr(r, &st) ? r->ft->getCharPtr(r, &st): ""), L"FALSE"))
                {
                    retVal->GetRightOperand().SetValue(false);
                }
                else
                {
                    throw SCXParseException(L"Predicate type is Boolean but the right operand is not a boolean literal: " + StrFromUTF8(l->ft->getCharPtr(l, &st)), SCXSRCLOCATION);
                }
                break;
            case CMPI_REAL:
                try {
                    retVal->GetRightOperand().SetValue(StrToDouble(StrFromUTF8(r->ft->getCharPtr(r, &st)))); 
                }
                catch(SCXNotSupportedException& e)
                {
                    throw SCXParseException(L"Predicate type is Real but the right operand is not a real literal: " + StrFromUTF8(l->ft->getCharPtr(l, &st)), SCXSRCLOCATION);
                }
                break;
            default:
                throw SCXInternalErrorException(L"Literal value type not supported: " + StrFromUTF8(l->ft->getCharPtr(l, &st)), SCXSRCLOCATION);
            }
            if(CMPI_RC_OK != st.rc)
            {
                throw SCXInternalErrorException(L"getCharPtr failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), SCXSRCLOCATION);
            }
        }
        SCX_LOGTRACE(log, L"Exit CreatePredicate(CMPIPredicate *p)");
        return retVal;
    }
}

namespace SCXProviderLib
{
    Condition::Condition()
    {
        this->m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.wqlsupport");
    }

    /*----------------------------------------------------------------------------*/
    /**
    *  \fn SCXHandle<Condition> CreateCondition(CMPISubCond *c)
    *  \brief Create a Condition from a CMPISubCond pointer.  This function is used in SCXWQLSelectStatementCMPI implementation so it can't be put in anonymous namespace
    *  \param[in] c: The pointer to the CMPISubCond
    *  \returns The pointer to the condition object.
    */
    SCXHandle<Condition> CreateCondition(CMPISubCond *c)
    {
        SCXHandle<Condition> retVal(new Condition());
        SCXCoreLib::SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.provsup.wqlsupport");
        SCX_LOGTRACE(log, L"CreateCondition(CMPISubCond *c)");
        if(c)
        {
            CMPIStatus st;
            int count = c->ft->getCount(c, &st);
            if(CMPI_RC_OK != st.rc)
            {
                throw SCXCoreLib::SCXInternalErrorException(L"GetCount failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), 
                    SCXSRCLOCATION);
            }

            for (int i = 0; i< count; i++)
            {
                SCXHandle<Predicate> pred = CreatePredicate(c->ft->getPredicateAt(c, i, &st));
                if(CMPI_RC_OK != st.rc)
                {
                    throw SCXCoreLib::SCXInternalErrorException(L"getPredicateAt failed in CMPI: " + StrFromUTF8(st.msg->ft->getCharPtr(st.msg, NULL) ? st.msg->ft->getCharPtr(st.msg, NULL) : ""), 
                        SCXSRCLOCATION);
                }

                retVal->GetPredicates().push_back(*pred);
            } 
        }
        SCX_LOGTRACE(log, L"Exit CreateCondition(CMPISubCond *c)");
        return retVal;
    }

    void Condition::AddPredicate(const Predicate& p)
    {
        this->predicates.push_back(p);
    }
}
