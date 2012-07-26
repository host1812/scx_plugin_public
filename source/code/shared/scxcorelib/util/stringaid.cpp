/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief     A set of functions working on the the wstring type

    \date      07-05-14 12:00:00

    Wstring helper functions
*/
/*----------------------------------------------------------------------------*/

#ifdef WIN32

#include <windows.h>

#else

#include <iconv.h>
#include <wctype.h>
#include <wchar.h>
#endif

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxdumpstring.h>

#include <sstream>
#include <typeinfo>

#include <scxcorelib/scxstream.h>


using namespace std;

namespace
{
    /*----------------------------------------------------------------------------*/
    /**
       convert multibyte representation to wstring according to system settings.
       Does not throw any exception in case of problem, instead it returns
       an incomplete string. Useful in exception handlers and sometimes in destructors

       \param    str  Multibyte encoded string to convert

       \returns     New wstring created from multibyte input

       \note In exception handlers and destructors exceptions should not be thrown.
             This method is a helper in those cases
    */
    std::wstring SomeStrFromMultibyte(const std::string& str)
    {
        std::vector<wchar_t> strAfterConversion(str.length() + 1);
        mbstate_t state;
        memset(&state, '\0', sizeof(state));
        const char *strBeforeConversion = str.c_str();
        size_t newCharCount = mbsrtowcs(&strAfterConversion[0], &strBeforeConversion, strAfterConversion.size(), &state);
        return newCharCount < (size_t) -1
            ? &strAfterConversion[0]
            : L"";
    }

}

namespace SCXCoreLib
{

    /*----------------------------------------------------------------------------*/
    /**
       Convert UTF-8 string to wstring

       \param    utf8_str  UTF-8 encoded string to convert

       \returns            New wstring created from UTF-8 input
    */
    wstring StrFromUTF8(const string &utf8_str)
    {
        try
        {
            istringstream utf8source(utf8_str);
            wostringstream wtarget;
            while (utf8source.peek() != EOF && utf8source.good())
            {
                wtarget.put(SCXStream::ReadCharAsUTF8(utf8source));
            }
            return wtarget.str();
        }
        catch (SCXLineStreamContentException &)
        {
            throw SCXStringConversionException(SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Convert wstring to UTF-8

       \param        str      wstring to convert

       \returns      String with UTF-8 representation of \a str.
    */
    string StrToUTF8(const wstring& str)
    {
        ostringstream utf8target;
        SCXStream::WriteAsUTF8(utf8target, str);
        return utf8target.str();
    }

    /*----------------------------------------------------------------------------*/
    /**
     * Extract what the exception refers to in a "safe" way, that is,
     * preferring an incomplete string over an exception
     * \param[in]  e    Exception of interest
     * \returns    Textual representation
     */
    std::wstring DumpString(const std::exception &e) {
        return SCXDumpStringBuilder(typeid(e).name())
            .Scalar("What",  SomeStrFromMultibyte(string(e.what())));
    }

    /*----------------------------------------------------------------------------*/
    /**
       Convert multibyte representation to wstring according to system settings

       \param    str  Multibyte encoded string to convert

       \returns     New wstring created from multibyte input
    */
    std::wstring StrFromMultibyte(const std::string& str)
    {
        std::vector<wchar_t> strAfterConversion(str.length() + 1);
        mbstate_t state;
        memset(&state, '\0', sizeof(state));
        const char *strBeforeConversion = str.c_str();
        size_t newCharCount = mbsrtowcs(&strAfterConversion[0], &strBeforeConversion, strAfterConversion.size(), &state);
        if (newCharCount >= (size_t) - 1)
        {
            throw SCXStringConversionException(SCXSRCLOCATION);
        }
        return &strAfterConversion[0];
    }

    /*----------------------------------------------------------------------------*/
    /**
       Convert wstring to multibyte representation according to system settings

       \param        str      wstring to convert

       \returns      String with multibyte representation of \a str.
    */
    std::string StrToMultibyte(const std::wstring& str)
    {
        std::vector<char> strAfterConversion(MB_CUR_MAX * str.length() + 1);
        mbstate_t state;
        memset(&state, '\0', sizeof(state));
        const wchar_t *strBeforeConversion = str.c_str();
        size_t newCharCount = wcsrtombs(&strAfterConversion[0], &strBeforeConversion, strAfterConversion.size(), &state);
        if (newCharCount >= (size_t) - 1)
        {
            throw SCXStringConversionException(SCXSRCLOCATION);
        }
        return &strAfterConversion[0];
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove whitespace at the left side (beginning) of the string

        \param      str  String to trim

        \returns    Trimmed string
    */
    wstring StrTrimL(const wstring& str)
    {
        return StrStripL(str, L" \t\n");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Remove whitespace at the right side (end) of the string

       \param       str   String to trim

       \returns     Trimmed string
    */
    wstring StrTrimR(const wstring& str)
    {
        return StrStripR(str, L" \t\n");
    }


    /*----------------------------------------------------------------------------*/
    /**
       Remove whitespace at both sides of the string

       \param       str    String to trim

       \returns            Trimmed string
    */
    wstring StrTrim(const wstring& str)
    {
        return StrTrimR(StrTrimL(str));
    }

    /*----------------------------------------------------------------------------*/
    /**
       Removes any of a list of characters from the beginning of a string.

       \param      str   String to strip characters from.
       \param      what  A string with all characters that should be removed.
       \returns          Stripped string.

       \date       2007-05-16 14:36:00

    */
    wstring StrStripL(const wstring& str, const wstring& what)
    {
        wstring::size_type pos(0);
        wstring tmp_str(str);

        for ( ; pos < tmp_str.size() && wstring::npos != what.find(str[pos]); ++pos )
        {
        }
        tmp_str.erase(0, pos);

        return tmp_str;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Removes any of a list of characters from the end of a string.

       \param   str    String to strip characters from.
       \param   what   A string with all characters that should be removed.
       \returns        Stripped string.

       \date           2007-05-16 14:36:00

    */
    wstring StrStripR(const wstring& str, const wstring& what)
    {
        wstring::size_type pos(str.size());
        wstring tmp_str(str);

        for ( ; pos > 0 && wstring::npos != what.find(str[pos-1]); --pos)
        {
        }
        tmp_str.erase(pos, tmp_str.size()-pos);

        return tmp_str;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Removes any of a list of characters from both sides of a string.

       \param   str    String to strip characters from.
       \param   what   A string with all characters that should be removed.
       \retval         Stripped string.

       \date           2007-05-16 14:36:00

    */
    wstring StrStrip(const wstring& str, const wstring& what)
    {
        return StrStripR(StrStripL(str, what), what);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve an unsigned integer from a string

        \param       str   String retrieve unsigned integer from
        \returns           The unsigned integer retrieved

        \throws            SCXNotSupportedException Str cannot be parsed.

    */
    unsigned int StrToUInt(const wstring& str)
    {
        unsigned int tmp;
        wstringstream ss(str);

        // Note: This line may not indicate an error on some systems,
        // even if the number is negative.  Thus, we always check for
        // a minus sign (below) to guarentee consistent behavior.
        bool conv_result = (ss >> tmp) != 0;

        wstringstream::pos_type s = ss.tellg();

        // If eof is set we must look for minus signs in whole string, else look
        // for them in the part that was parsed.

        if (!conv_result ||
            (ss.eof() && (wstring::npos != str.find(L'-'))) ||
            (!ss.eof() && (wstring::npos != str.substr(0, static_cast<unsigned> (s)).find(L'-')))) 
        {
            throw SCXCoreLib::SCXNotSupportedException(L"Cannot parse unsigned int in: '" + str + L"'", SCXSRCLOCATION);
        }
        return tmp;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Retrieve a double from a string

        \param       str   String retrieve double from
        \returns           The double retrieved

        throws             SCXNotSupportedException  Str cannot be parsed.

    */
    double StrToDouble(const wstring& str)
    {
        double tmp;
        wstringstream ss(str);

        bool conv_result = (ss >> tmp) != 0;

        if (!conv_result) {
            throw SCXCoreLib::SCXNotSupportedException(L"Cannot parse double in: '" + str + L"'", SCXSRCLOCATION);
        }
        return tmp;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve a long integer from a string.

       \param     str    String retrieve long from
       \returns          The long value retrieved.

       \throws           SCXNotSupportedException    Str cannot be parsed.

       \date             07-07-02 11:00:00

    */
    scxlong StrToLong(const std::wstring& str)
    {
        scxlong tmp;
        wstringstream ss(str);

        bool conv_result = (ss >> tmp) != 0;

        if (!conv_result) {
            throw SCXCoreLib::SCXNotSupportedException(L"Cannot parse scxlong in: '" + str + L"'", SCXSRCLOCATION);
        }
        return tmp;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve an unsigned long integer from a string.

       \param    str    String retrieve unsigned long from
       \returns         The unsigned long value retrieved.

       \throws          SCXNotSupportedException if str cannot be parsed.

       \date            07-07-02 11:00:00

    */
    scxulong StrToULong(const std::wstring& str)
    {
        scxulong tmp;
        wstringstream ss(str);

        // Note: This line may not indicate an error on some systems,
        // even if the number is negative.  Thus, we always check for
        // a minus sign (below) to guarentee consistent behavior.
        bool conv_result = (ss >> tmp) != 0;

        wstringstream::pos_type s = ss.tellg();

        // If eof is set we must look for minus signs in whole string, else look
        // for them in the part that was parsed.

        if (!conv_result ||
            (ss.eof() && (wstring::npos != str.find(L'-'))) ||
            (!ss.eof() && (wstring::npos != str.substr(0, static_cast<unsigned> (s)).find(L'-'))))
          {
            throw SCXCoreLib::SCXNotSupportedException(L"Cannot parse scxulong in: '" + str + L"'", SCXSRCLOCATION);

          }
        return tmp;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Convert string to all uppercase

        \param       str   String to convert

        \returns           Converted string

    */
    wstring StrToUpper(const wstring& str)
    {
        wstring tmp_str(str);

        for (wstring::size_type i = 0; i < tmp_str.size(); i++)
        {
            tmp_str[i] = towupper(tmp_str[i]);
        }

        return tmp_str;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Convert string to all lowercase

        \param       str  String to convert

        \returns          Converted string

    */
    wstring StrToLower(const wstring& str)
    {
        wstring tmp_str(str);

        for (wstring::size_type i = 0; i < tmp_str.size(); i++)
        {
            tmp_str[i] = towlower(tmp_str[i]);
        }

        return tmp_str;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Compare strings case insensitive

        \param       str1   First string to compare
        \param       str2   Second string to compare
        \param       ci     Boolean whether comparision should be case-insensitive or not

        \returns     0 if equal, <0 if first string is less, >0 if first string is greater

    */
    int StrCompare(const wstring& str1, const wstring& str2, bool ci)
    {
        if (ci)
        {
            return StrToUpper(str1).compare(StrToUpper(str2));
        }
        else
        {
            return str1.compare(str2);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Extract a vector of the substrings that are separated by one of the delimter characters

        \param    str          String to tokenize
        \param    tokens       Return a vector of tokens
        \param    delimiters   String containing the delimeter charachters
        \param    trim         true if all tokens should be trimmed (default), otherwise false
        \param    emptyTokens  false if empty tokens should be removed from the result (default), otherwise true.
        \param    keepDelimiters true if delimiters found should be added to the token vector, otherwise (default)
                                 delimiters are removed.

    */
    void StrTokenize(const wstring& str, vector<std::wstring>& tokens,
                     const wstring& delimiters, bool trim, bool emptyTokens, bool keepDelimiters)
    {
        tokens.clear();

        wstring::size_type lastPos = 0;
        wstring::size_type pos = delimiters.empty()?wstring::npos:str.find_first_of(delimiters);

        while (wstring::npos != pos)
        {
            wstring tmp = str.substr(lastPos, pos - lastPos);
            if ( ! tmp.empty() && trim)
            {
                tmp = StrTrim(tmp);
            }
            if ( ! tmp.empty() || emptyTokens)
            {
                tokens.push_back(tmp);
            }
            if ( keepDelimiters )
            {
                tokens.push_back(str.substr(pos, 1));
            }
            lastPos = pos + 1;
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, lastPos);
        }

        wstring tmp = str.substr(lastPos, wstring::npos);
        if ( ! tmp.empty() && trim)
        {
            tmp = StrTrim(tmp);
        }
        if ( ! tmp.empty() || emptyTokens)
        {
            tokens.push_back(tmp);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Extract a vector of the substrings that are separated by the delimter string

        \param    str          String to tokenize
        \param    tokens       Return a vector of tokens
        \param    delimiter    Delimeter string
        \param    trim         true if all tokens should be trimmed (default), otherwise false
        \param    emptyTokens  false if empty tokens should be removed from the result (default), otherwise true.

    */
    void StrTokenizeStr(const wstring& str, vector<std::wstring>& tokens, const wstring& delimiter, bool trim/*=true*/, bool emptyTokens/*=false*/)
    {
        tokens.clear();

        wstring::size_type lastPos = 0;
        wstring::size_type pos = delimiter.empty()?wstring::npos:str.find(delimiter, lastPos);

        while (wstring::npos != pos)
        {
            wstring tmp = str.substr(lastPos, pos - lastPos);
            if ( ! tmp.empty() && trim)
            {
                tmp = StrTrim(tmp);
            }
            if ( ! tmp.empty() || emptyTokens)
            {
                tokens.push_back(tmp);
            }
            lastPos = pos + delimiter.length();
            //if (pos != wstring::npos)
            {
                pos = str.find(delimiter, lastPos);
            }
        }
        wstring tmp = str.substr(lastPos, wstring::npos);
        if ( ! tmp.empty() && trim)
        {
            tmp = StrTrim(tmp);
        }
        if ( ! tmp.empty() || emptyTokens)
        {
            tokens.push_back(tmp);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Merge tokens using pairs of merge identifiers. For example this can be used to merge tokens
       "a,b" to "ab".

       \param tokens     Vector of string tokens.
       \param mergePairs A map where the key is the beginning of a merge identifier and the value is the
                         end identifier.
       \param glue       a string to insert between tokens merged.
       \returns true if merge is ok or false if merge fails because there is no end merge identifier found.

       \date 2008-01-29 08:00:00

       The merge identifiers need not be seperate tokens if the start is the found in the beginning of a token
       and the end identifier is found in the ent of a token.
       \note merge identifiers will be removed from the merged tokens.
    */
    bool StrMergeTokens(std::vector<std::wstring>& tokens, const std::map<std::wstring,std::wstring>& mergePairs, const std::wstring& glue)
    {
        //size_t start_size = tokens.size();
        std::vector<std::wstring>::iterator next_token = tokens.begin();
        std::vector<std::wstring>::iterator merge = tokens.end();
        std::map<std::wstring,std::wstring>::const_iterator pair = mergePairs.end();
        while (next_token != tokens.end())
        {
            std::vector<std::wstring>::iterator token = next_token++;

            if (pair == mergePairs.end())
            { // Need to find a new merge delimiter
                pair = mergePairs.find(*token); // First try to find a merge delimiter matching the complete token.
                if (pair == mergePairs.end())
                { // We have to find partial matches
                    for (pair = mergePairs.begin(); pair != mergePairs.end(); pair++)
                    {
                        if (pair->first == token->substr(0, pair->first.length()))
                        {
                            *token = token->substr(pair->first.length()); // Remove merge identifier
                            merge = token;
                            if (pair->second == token->substr(token->length()-pair->second.length()))
                            { // merge started and ended in same token
                                *token = token->substr(0,token->length()-pair->second.length()); // Remove merge identifier
                                pair = mergePairs.end();
                            }
                            break;
                        }
                    }
                }
                else
                {
                    merge = tokens.erase(token); // Remove merge identifier
                    if (merge == tokens.end())
                    {
                        next_token = merge;
                    }
                    else
                    {
                        next_token = merge+1;
                    }
                }
            }
            else
            { //Merging
                if (pair->second == token->substr(token->length()-pair->second.length()))
                {
                    *token = token->substr(0,token->length()-pair->second.length()); // Remove merge identifier
                    pair = mergePairs.end();
                }
                if (token->length() > 0)
                {
                    merge->append(glue);
                }
                merge->append(*token);
                next_token = tokens.erase(token);
            }
        }

        return pair == mergePairs.end();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if a string is the prefix of another

        \param   str     Base string to check
        \param   prefix  Prefix string to compare with
        \param   ci      Should the comparision be made case insensitive

        Retval:      true if it is a prefix, othervise false
    */
    bool StrIsPrefix(const std::wstring& str, const std::wstring& prefix, bool ci)
    {
        if (prefix.length() > str.length())
        {
            return false;
        }

        return (0 == StrCompare(str.substr(0, prefix.length()), prefix, ci));
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
