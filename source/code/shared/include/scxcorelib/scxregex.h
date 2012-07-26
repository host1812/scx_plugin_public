/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

 */
/**
    \file

    \brief     Regular expression class

    \date      2008-08-20 14:08:17

 */
/*----------------------------------------------------------------------------*/
#ifndef SCXREGEX_H
#define SCXREGEX_H
 
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>

#include <string>
#include <sys/types.h>
#include <regex.h>

namespace SCXCoreLib {

    /*----------------------------------------------------------------------------*/
    /**
        The SCXRegex class represents an immutable (read-only) regular expression.
        It also contains static methods that allow use of other regular expression
        classes without explicitly creating instances of the other classes.
     */
    class SCXRegex
    {
    public:
        SCXRegex(const std::wstring& expression); 
        bool IsMatch(const std::wstring& text) const;
        ~SCXRegex();
        std::wstring Get() const;
    private:
        std::wstring m_Expression;
        regex_t m_Preq; //!< Pattern buffer storage area.
    };


    /*----------------------------------------------------------------------------*/
    /**
        This exception is thrown by the constructor of SCXRegex when regular
        expression can't be compiled.
     */
    class SCXInvalidRegexException : public SCXException
    {
    public:
        SCXInvalidRegexException(const std::wstring& expression,
                                 int errcode, const regex_t* preq,
                                 const SCXCodeLocation& l);
        std::wstring What() const;
    protected:
        std::wstring m_Expression; //!< Regular expression that caused exception.
        int m_Errcode;             //!< System error code.
        std::string  m_Errtext;    //!< Text describing the error.
    };

    /**
       Helper structure to hold a regular expression together with its index
    */

    struct SCXRegexWithIndex
    {
        size_t index;     //!< index for regular expression
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXRegex> regex;  //!< the regular expression
    };
}

#endif  /* SCXFILE_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
