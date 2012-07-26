/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     A collection of string functions assisting the wstring class

    \date      07-05-14 12:00:00

    String helper functions, support for working with std::wstring
*/
/*----------------------------------------------------------------------------*/
#ifndef STRINGAID_H
#define STRINGAID_H

#include <string>
#include <vector>
#include <scxcorelib/scxexception.h>
#include <map>
 

namespace SCXCoreLib
{
    
    std::wstring StrFromUTF8(const std::string& str);
    std::string StrToUTF8(const std::wstring& str);

    std::wstring DumpString(const std::exception &e);
    
    std::wstring StrFromMultibyte(const std::string& str);
    std::string StrToMultibyte(const std::wstring& str);

    std::wstring StrTrimL(const std::wstring& str);
    std::wstring StrTrimR(const std::wstring& str);
    std::wstring StrTrim(const std::wstring& str);

    std::wstring StrStripL(const std::wstring& str, const std::wstring& what);
    std::wstring StrStripR(const std::wstring& str, const std::wstring& what);
    std::wstring StrStrip(const std::wstring& str, const std::wstring& what);

    // These templates are defined below in this file. Usable for at least all integral types.
    template <typename T> std::wstring StrAppend(const std::wstring& str, T i);
    template <typename T> std::wstring StrFrom(T v);

    unsigned int StrToUInt(const std::wstring& str);
    double StrToDouble(const std::wstring& str);
    scxlong StrToLong(const std::wstring& str);
    scxulong StrToULong(const std::wstring& str);

    std::wstring StrToUpper(const std::wstring& str);
    std::wstring StrToLower(const std::wstring& str);

    int StrCompare(const std::wstring& str1, const std::wstring& str2, bool ci=false);

    void StrTokenize(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters = L" \n", bool trim=true, bool emptyTokens=false, bool keepDelimiters = false);
    void StrTokenizeStr(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiter, bool trim=true, bool emptyTokens=false);
    bool StrMergeTokens(std::vector<std::wstring>& tokens, const std::map<std::wstring,std::wstring>& mergePairs, const std::wstring& glue);

    bool StrIsPrefix(const std::wstring& str, const std::wstring& prefix, bool ci=false);

    /** Exception for failed multibyte conversion*/
    class SCXStringConversionException : public SCXException {
    public:
        //! Ctor
        SCXStringConversionException(const SCXCodeLocation& l)
            : SCXException(l)
        {};

        std::wstring What() const { return L"Multibyte string conversion failed"; }
    };

    /*----------------------------------------------------------------------------*/
    /**
        Append an integral datatype to a string

        \param  str    String to append to
        \param  i      Integral datatype to append
        \returns       The appended string

        \date          2007-10-02 14:02:00

        \remarks This template started its life as a number of separate functions
        that took an explicitly typed second arguments. That eventually clashed with
        how size_t was defined on separated platforms. You may think that it is ugly
        to include the definition in the header file and that we should use export
        instead, the so called separation model. Well, that's not possible, export
        is not supported by the compiler we use. What about explicit instantiation
        then? That would have some very bad consequences in shape of wild includes
        and dependencies. This is because for every type we use, there must be very
        explicit instantiation. Since we feed types from pegasus into this function
        that will be very bad.
    */
    template <typename T>
        std::wstring StrAppend(const std::wstring& str, T i)
    {
        std::wstringstream ss;
        ss << str << i;
        return ss.str();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Convert an integral datatype to a string

        \param    v     Integral datatype to convert
        \returns        The integral datatype as a string

        \date           2007-10-02 14:02:00

    */
    template <typename T>
        std::wstring StrFrom(T v)
    {
        std::wstringstream ss;
        ss << v;
        return ss.str();
    }
}


#endif /* STRINGAID_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
