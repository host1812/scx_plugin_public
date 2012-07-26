/*------------------------------------------------------------------------------
Copyright (C) 2007, Microsoft Corp.

*/
/**
\file

\brief    Definition for WQL select statement 

\author    
\date      

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXWQLSELECTSTATEMENT_H
#define SCXWQLSELECTSTATEMENT_H

#include <string>
#include <vector>

#include "scxproviderlib/scxinstance.h"
#include "scxproviderlib/condition.h"
#include "scxproviderlib/scxproperty.h"
#include "scxcorelib/scxlog.h"

namespace SCXProviderLib
{
	/*----------------------------------------------------------------------------*/
	/**
	*  SCXWQLSelectStatementBase is an interface for scx wql query support.  Subclasses of this class can be used to perform parse and analyze on wql query strings
	*
	*/
	class SCXWQLSelectStatementBase
	{
	public:
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn SCXWQLSelectStatementBase::SCXWQLSelectStatementBase(const std::wstring& queryString)
		*  \brief Ctor for SCXWQLSelectStatementBase that takes a query string
		*
		*  \param[in] queryString: Query string 
		*/
		SCXWQLSelectStatementBase(const std::wstring& queryString);
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn const std::wstring& GetClassName() const;
		*  \brief Accessor for retrieving the class name of the query
		*/
		const std::wstring& GetClassName() const { return this->className; };
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn std::vector<Condition>& GetWhereClauseDOC()
		*  \brief Accessor for retrieving the where clause of the query in DOC (disjunction of conductions) form. 
		*  This method is not const becaus from GetWhereClauseDOC we can call GetPredicates which is not
		*  const method.  GetPredicates() is not const because making it const would require re-declaring all the 
		*  overloaded "set" methods for SCXPRoperty
		*/
		std::vector<Condition>& GetWhereClauseDOC() { return this->doc; };
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn const std::vector<SCXProperty>& GetProjection() const
		*  \brief Accessor for retrieving the projection of the query
		*/
		const std::vector<SCXProperty>& GetProjection() const { return this->projection; };
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void Parse()
		*  \brief Parse the query string
		*/
		virtual void Parse() = 0;
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void Analyze()
		*  \brief Perform semantic analysis on the query string
		*/
		virtual void Analyze() = 0;
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void ApplyFilter(SCXInstance& instance)
		*  \brief Set the property filter for a SCXInstance
		*  \param[in] instance: The SCXInstance for which the property filter will be set
		*/
		virtual void ApplyFilter(SCXInstance& instance);
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn std::wstring DumpString() const
		*  \brief Returns the string representation of the condition.
		*
		*  \returns The string representation.
		*/
		virtual std::wstring DumpString() const;
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn SCXWQLSelectStatementBase::~SCXWQLSelectStatementBase()
		*  \brief Dtor for SCXWQLSelectStatementBase.
		*/
		virtual ~SCXWQLSelectStatementBase(){};

	protected:
		/*----------------------------------------------------------------------------*/
		/** The query string. */
		std::wstring query;
		/*----------------------------------------------------------------------------*/
		/** The class name of the query. */
		std::wstring className;
		/*----------------------------------------------------------------------------*/
		/** Vector holding the where clause in the form of a set of Conditions. */
		std::vector<Condition> doc;
		/*----------------------------------------------------------------------------*/
		/** Vector holding the set of projections. */
		std::vector<SCXProperty> projection;
		/*----------------------------------------------------------------------------*/
		/** The handle to logger object. */
		SCXCoreLib::SCXLogHandle m_log;
	};


	/*----------------------------------------------------------------------------*/
	/**
	*  SCXWQLSelectStatement is an concrete class of the SCXWQLSelectStatementBase interface.  Provider specific wql selectStatement should inherit this class
	*
	*/
	class SCXWQLSelectStatement : public SCXWQLSelectStatementBase
	{
	public:
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn SCXWQLSelectStatement::SCXWQLSelectStatement(const std::wstring& queryString, SCXWQLSelectStatementBase *internalInstance)
		*  \brief Ctor for SCXWQLSelectStatementBase that takes a query string and an implementation specific internal instance
		*
		*  \param[in] queryString: Query string 
		*  \param[in] internalInstance: pointer to implementation specific internal selectStatement instance
		*/
		SCXWQLSelectStatement(const std::wstring& queryString, SCXWQLSelectStatementBase *internalInstance);
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn SCXWQLSelectStatement::SCXWQLSelectStatement(const std::wstring& queryString, CMPIBroker *broker)
		*  \brief Ctor for SCXWQLSelectStatement that takes a query string and a Pegasus CMPIBroker pointer as input.
		*
		*  \param[in] queryString: Query string 
		*  \param[in] broker: Pointer to a CMPIBroker object 
		*/
		SCXWQLSelectStatement(const std::wstring& queryString, CMPIBroker *broker);
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void Parse()
		*  \brief Parse the query string
		*/
		virtual void Parse();
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void Analyze()
		*  \brief Perform semantic analysis on the query string
		*/
		virtual void Analyze();
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn SCXWQLSelectStatement::~SCXWQLSelectStatement()
		*  \brief Dtor for SCXWQLSelectStatementBase.
		*/
		virtual ~SCXWQLSelectStatement();

	protected:
		/*----------------------------------------------------------------------------*/
		/** The pointer to either an instance of SCXWQLSelectStatementInternal or SCXWQLSelectStatementCMPI. */
		SCXWQLSelectStatementBase *instance;
	};
}

#endif //SCXWQLSELECTSTATEMENT_H
