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

#include <cstdio>
#include <cstring>
#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/WsmServer/Matching.h>

PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

int main(int argc, char** argv)
{
    {
	const char* p = MatchPrefix("xxx", "xxx");
	PEGASUS_TEST_ASSERT(*p == '\0');
    }

    {
	const char* p = MatchPrefix("xxxzzz", "xxx");
	PEGASUS_TEST_ASSERT(strcmp(p, "zzz") == 0);
    }

    {
	const char* p = MatchPrefix("xxxzzz", "xxx*");
	PEGASUS_TEST_ASSERT(strcmp(p, "zzz") == 0);
    }

    {
	const char* p = MatchPrefix("xxx", "xxx*");
	PEGASUS_TEST_ASSERT(strcmp(p, "") == 0);
    }

    {
	const char* p = MatchPrefix(
	    "http://schemas.microsoft.com/xxx/yyy/zzz",
	    "http://schemas.microsoft.*/xxx/yyy/");
	PEGASUS_TEST_ASSERT(strcmp(p, "zzz") == 0);
    }

    {
	const char* p = MatchPrefix(
	    "http://schemas.microsoft.com/xxx/yyy/zzz",
	    "http://*.*.*/xxx/yyy/");
	PEGASUS_TEST_ASSERT(strcmp(p, "zzz") == 0);
    }

    {
	const char* p = MatchPrefix(
	    "http://me.com/cgi/foo.cgi", "http://me.com/cgi/");
	PEGASUS_TEST_ASSERT(strcmp(p, "foo.cgi") == 0);
    }

    {
	bool flag = Match(
	    "http://schemas.microsoft.com/xxx/yyy/zzz",
	    "http://schemas.microsoft.*/xxx/yyy/zzz");
	PEGASUS_TEST_ASSERT(flag);
    }

    {
	const char* p = MatchPrefix("xxx", "*");
	PEGASUS_TEST_ASSERT(strcmp(p, "xxx") == 0);
    }

    {
	const char* p = MatchPrefix("xxx", "x*");
	PEGASUS_TEST_ASSERT(strcmp(p, "xx") == 0);
    }

    {
	const char* p = MatchPrefix("xxxyyy", "x*y");
	PEGASUS_TEST_ASSERT(strcmp(p, "yy") == 0);
    }

    {
	const char* p = MatchPrefix("/usr/lib/*/xxx", "/usr/lib/*/");
	PEGASUS_TEST_ASSERT(strcmp(p, "xxx") == 0);
    }

    {
	const char* p = MatchPrefix("/usr/lib/libfoo.so", "/usr/lib/*.so");
	PEGASUS_TEST_ASSERT(strcmp(p, "") == 0);
    }

    {
	const char* p = MatchPrefix(
	    "http://schemas.*.*/wbem/wscim/1/*",
	    "http://schemas.*.*/wbem/wscim/1/");
	PEGASUS_TEST_ASSERT(strcmp(p, "*") == 0);
    }

    printf("%s +++++ passed all tests\n", argv[0]);
    return 0;
}
