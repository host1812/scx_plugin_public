//%LICENSE////////////////////////////////////////////////////////////////
//
// Licensed to The Open Group (TOG) under one or more contributor license
// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
// this work for additional information regarding copyright ownership.
// Each contributor licenses this file to you under the OpenPegasus Open
// Source License; you may not use this file except in compliance with the
// License.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////
//
//%/////////////////////////////////////////////////////////////////////////////

#ifndef _Pegasus_WsmServer_Matching_h
#define _Pegasus_WsmServer_Matching_h

#include <Pegasus/Common/Config.h>
#include "Linkage.h"

PEGASUS_NAMESPACE_BEGIN

/** Attempts to match the shortest prefix of 'str' given by 'pat'. Asterisks
    from 'pat' match zero or more charadters in 'str' (there are no other 
    special characters). If 'pat' matches prefix os 'str', returns a pointer 
    within 'str' to the end of that prefix. Else, returns zero, indicating 
    that 'pat' does not match a suffix of 'str'. Note that the 'shortest prefix'
    algorithm is somewhat unconventional and may produce unexpected results.
    For example, consider

        MatchPrefix("xxx", "*")

    In this case, the shortest prefix is empty and the result is "xxx". This
    function is tpyically used to match things like

        MatchPrefix("http://me.com/cgi/foo.cgi", "http://me.com/cgi/")

    which yields "foo.cgi".
*/
PEGASUS_WSMSERVER_LINKAGE 
const char* MatchPrefix(const char* str, const char* pat);

/** Attempts to match 'pat' to 'str', using the reules described for
    'MatchPrefix()'. Returns true if 'pat' matches entires 'str'.
*/
inline bool Match(const char* str, const char* pat)
{
    const char* p = MatchPrefix(str, pat);

    if (!p)
        return false;

    return *p == '\0';
}

PEGASUS_NAMESPACE_END

#endif /* _Pegasus_WsmServer_Matching_h */
