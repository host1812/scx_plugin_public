/*------------------------------------------------------------------------------
    Copyright (C) 2007, Microsoft Corp.
    
*/
/**
    \file
 
    \brief    Definition for boolean condition used in WQL
 
    \author    
    \date      
 
*/
/*----------------------------------------------------------------------------*/
#ifndef CONDITION_H
#define CONDITION_H

#include <string>
#include <vector>
#include "scxcorelib/scxdumpstring.h"
#include "scxproviderlib/predicate.h"
#include "scxcorelib/scxlog.h"

namespace SCXProviderLib
{
	
    /*----------------------------------------------------------------------------*/
    /**
     *  Condition implements a WQL where clause condition.  A condition is the AND of a set of Predicates
     *
     */
	class Condition
	{
	public:
		/*----------------------------------------------------------------------------*/
        /**
        *  \fn Condition::Condition()
        *  \brief Ctor for Condition that takes no parameter.
        */
		Condition();
		/*----------------------------------------------------------------------------*/
		/**
        *  \fn std::vector<Predicate>& GetPredicates()
        *  \brief Accessor for retrieving the vector of predicates.
        */
		std::vector<Predicate>& GetPredicates() { return this->predicates; };
		/*----------------------------------------------------------------------------*/
		/**
        *  \fn void AddPredicate(const Predicate& p)
        *  \brief Add a predicate to the vactor of predicates.
        */
		void AddPredicate(const Predicate& p);
		/*----------------------------------------------------------------------------*/
        /**
        *  \fn std::wstring DumpString() const
        *  \brief Returns the string representation of the condition.
        *
        *  \returns The string representation.
        */
		std::wstring DumpString() const
		{
			return SCXDumpStringBuilder("Condition").Instances("predicates", predicates);
		}
		/*----------------------------------------------------------------------------*/
        /**
        *  \fn Condition::~Condition()
        *  \brief Dtor for Condition.
        */
		virtual ~Condition(){};
	private:
		/*----------------------------------------------------------------------------*/
        /** Vector holding the set of predicates. */
		std::vector<Predicate> predicates;
		/*----------------------------------------------------------------------------*/
        /** Log handle. */
		SCXCoreLib::SCXLogHandle m_log;
	};
}

#endif // CONDITION_H
