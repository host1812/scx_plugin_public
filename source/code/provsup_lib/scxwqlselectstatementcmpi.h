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
#ifndef SCXWQLSELECTSTATEMENTCMPI_H
#define SCXWQLSELECTSTATEMENTCMPI_H

#include <string>
#include <vector>

#include "scxproviderlib/scxinstance.h"
#include "scxproviderlib/condition.h"
#include "scxproviderlib/scxproperty.h"
#include "scxcorelib/scxlog.h"
#include "scxproviderlib/scxwqlselectstatement.h"

namespace SCXProviderLib
{
	/*----------------------------------------------------------------------------*/
	/**
	*  SCXWQLSelectStatementCMPI is a subclass of SCXWQLSelectStatementBase which provide a conerete implementation of the SCXWQLSelectStatementBase interface
	*
	*/
	class SCXWQLSelectStatementCMPI : public SCXWQLSelectStatementBase
	{
	public:
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn SCXWQLSelectStatementCMPI::SCXWQLSelectStatementCMPI(const std::wstring& queryString, CMPIBroker *broker)
		*  \brief Ctor for SCXWQLSelectStatementCMPI that takes a query string and a Pegasus CMPIBroker pointer as input.
		*
		*  \param[in] queryString: Query string 
		*  \param[in] broker: Pointer to a CMPIBroker object 
		*/
		SCXWQLSelectStatementCMPI(const std::wstring& queryString, CMPIBroker *broker);
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
		virtual void Analyze(){};
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn SCXWQLSelectStatementCMPI::~SCXWQLSelectStatementCMPI()
		*  \brief Dtor for SCXWQLSelectStatementCMPI.
		*/
		virtual ~SCXWQLSelectStatementCMPI();

	private:
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void SetClassName(const std::wstring& name)
		*  \brief Set the className property of the select statement.
		*
		*  \param[in] name: class name to be set to 
		*/
		void SetClassName(const std::wstring& name);
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void SetExp(CMPISelectExp *exp)
		*  \brief Set pointer to the CMPI data structure that represents a CMPI select statement.
		*
		*  \param[in] exp: Pointer to the CMPI select data structure 
		*/
		void SetExp(CMPISelectExp *exp);
		/*----------------------------------------------------------------------------*/
		/**
		*  \fn void SetProjection(CMPIArray *projectionPtr)
		*  \brief Set the pointer to the CMPI data structure that represents the projection of a query statement.
		*
		*  \param[in] projectionPtr: Pointer to the projection data structure
		*/
		void SetProjection(CMPIArray *projectionPtr);
		/*----------------------------------------------------------------------------*/
		/** Pointer to the CMPI data structure that represents the projection of the query */
		CMPIArray *cmpiProjection;
		/*----------------------------------------------------------------------------*/
		/** Pointer to the CMPI data structure that represents the select statement */
		CMPISelectExp *cmpiExp;
		/*----------------------------------------------------------------------------*/
		/** Pointer to the CMPI broker. */
		CMPIBroker *cmpiBroker;
	};
}

#endif //SCXWQLSELECTSTATEMENTCMPI_H
