/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

 */
/**
    \file

    \brief     Regex implementation

    \date      2008-08-20 14:19:15

 */
/*----------------------------------------------------------------------------*/


#include <scxcorelib/scxregex.h>
#include <scxcorelib/stringaid.h>

namespace SCXCoreLib {
    /*--------------------------------------------------------------*/
    /**
        Initializes and compiles a new instance of the Regex class
        for the specified regular expression.

        \param[in] expression Regular expression to compile.
        \throws SCXInvalidRegexException if compilation of regex fails.
    */
    SCXRegex::SCXRegex(const std::wstring& expression) : m_Expression(expression)
    {
        int errcode = regcomp(&m_Preq, StrToMultibyte(expression).c_str(), REG_EXTENDED|REG_NOSUB);
        if (errcode != 0)
        {
            throw SCXInvalidRegexException(expression, errcode, &m_Preq, SCXSRCLOCATION);
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Indicates whether the regular expression specified in the
        SCXRegex constructor finds a match in the input string.
        
        \param[in] text Input string to match.
        \returns true if a match is found.
     */
    bool SCXRegex::IsMatch(const std::wstring& text) const
    {
        return (0 == regexec(&m_Preq, StrToMultibyte(text).c_str(), 0, 0, 0));
    }

    /*--------------------------------------------------------------*/
    /**
       Get the regular expression in wstring type

       \returns Regular expression used when generating the object
    */
    std::wstring SCXRegex::Get() const
    {
        return m_Expression;
    }

    /*--------------------------------------------------------------*/
    /**
        Destructor
     */
    SCXRegex::~SCXRegex()
    {
        regfree(&m_Preq);
    }

    /*--------------------------------------------------------------*/
    /**
        Creates a new regex exception

        \param[in] expression Regular expression that failed to compile
        \param[in] errcode    System error code
        \param[in] preq       Pattern buffer storage area
        \param[in] l          Source code location object

    */
    SCXInvalidRegexException::SCXInvalidRegexException(const std::wstring& expression,
                                                       int errcode, const regex_t* preq,
                                                       const SCXCodeLocation& l) :
        SCXException(l),
        m_Expression(expression),
        m_Errcode(errcode)
    {
        char buf[80];
        regerror(m_Errcode, preq, buf, sizeof(buf));
        m_Errtext = buf;
    }

    std::wstring SCXInvalidRegexException::What() const {
        std::wostringstream txt;
        txt << L"Compiling " << m_Expression << L" returned an error code = " << m_Errcode << L" (" << m_Errtext.c_str() << L")";
        return txt.str();
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
