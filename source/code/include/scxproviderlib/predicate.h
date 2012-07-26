/*------------------------------------------------------------------------------
    Copyright (C) 2007, Microsoft Corp.
    
*/
/**
    \file
 
    \brief    Definition for boolean predicate used in WQL
 
    \author    
    \date      
 
*/
/*----------------------------------------------------------------------------*/
#ifndef PREDICATE_H
#define PREDICATE_H

//This header file is necessary because of CMPIPredOp which is an enum.  We could define coresponding SCX enum to get rid of this if desired. 
#include "Pegasus/Provider/CMPI/cmpidt.h"

#include "scxproviderlib/scxproperty.h"
#include "scxcorelib/stringaid.h"
#include "scxcorelib/scxlog.h"

namespace SCXProviderLib
{
        /*----------------------------------------------------------------------------*/
    /**
     *  Predicate implements a WQL where clause Predicate.  A Predicate is of the form "operand operator operand" such as name = "george"
     *
     */
        class Predicate
        {
        public:
                /*----------------------------------------------------------------------------*/
        /**
        *  \fn Predicate::Predicate()
        *  \brief Ctor for Predicate that takes no parameter.
        */
                Predicate();
                /*----------------------------------------------------------------------------*/
        /**
        *  \fn Predicate::~Predicate()
        *  \brief Dtor for Predicate.
        */
                virtual ~Predicate(){};
                /*----------------------------------------------------------------------------*/
                /**
        *  \fn CMPIPredOp GetOperation() const
        *  \brief Accessor for retrieving the operator of the predicate.
        */
                CMPIPredOp GetOperation() const { return this->op; };
                /*----------------------------------------------------------------------------*/
                /**
        *  \fn void SetOperation(const CMPIPredOp& o)
        *  \brief Modifier for the operator of the predicate.
                *  \param[in] o: The operator to set
        */
                void SetOperation(const CMPIPredOp& o) { this->op = o; };
                /*----------------------------------------------------------------------------*/
                /**
        *  \fn SCXProperty& GetLeftOperand()
        *  \brief Accessor for retrieving the left hand operand of the predicate.
                *   This method is not const because we need to set the returned SCXProperty
                *   Alternative would require redeclaring all the 7-8 "Set" methods for SCXProperty here
        */
                SCXProperty& GetLeftOperand() { return this->lhs; };
                /*----------------------------------------------------------------------------*/
                /**
        *  \fn SCXProperty& GetRightOperand()
        *  \brief Accessor for retrieving the right hand operand of the predicate.
        */
                SCXProperty& GetRightOperand() { return this->rhs; };
                /*----------------------------------------------------------------------------*/
        /**
        *  \fn std::wstring DumpString() const
        *  \brief Returns the string representation of the predicate.
        *
        *  \returns The string representation.
        */
                std::wstring DumpString() const;
        private:
                /*----------------------------------------------------------------------------*/
        /** The operator of the predicate. */
                CMPIPredOp op;
            /*----------------------------------------------------------------------------*/
        /** The left hand operand of the predicate. */
                SCXProperty lhs;
        /*----------------------------------------------------------------------------*/
        /** The right hand operand of the predicate. */
                SCXProperty rhs;
                /*----------------------------------------------------------------------------*/
        /** Log handle. */
                SCXCoreLib::SCXLogHandle m_log;
        };
}

#endif // PREDICATE_H
