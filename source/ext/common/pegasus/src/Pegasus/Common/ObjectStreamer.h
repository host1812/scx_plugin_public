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

#ifndef ObjectStreamer_h
#define ObjectStreamer_h

#include "CIMClass.h"
#include "CIMInstance.h"
#include "DeclContext.h"
#include <Pegasus/Common/Linkage.h>
#include <Pegasus/Common/Buffer.h>

PEGASUS_NAMESPACE_BEGIN

class PEGASUS_COMMON_LINKAGE ObjectStreamer
{
public:

    ObjectStreamer() {}

    virtual ~ObjectStreamer() {}

    virtual void encode(Buffer& out, const CIMClass& cls) = 0;
    virtual void encode(Buffer& out, const CIMInstance& inst) = 0;
    virtual void encode(Buffer& out, const CIMQualifierDecl& qual) = 0;

    virtual void decode(const Buffer& in, unsigned int pos, CIMClass& cls) = 0;
    virtual void decode(
        const Buffer& in,
        unsigned int pos,
        CIMInstance& inst) = 0;
    virtual void decode(
        const Buffer& in,
        unsigned int pos,
        CIMQualifierDecl& qual) = 0;

    virtual void write(PEGASUS_STD(ostream)& os, Buffer& in)
    {
        os.write(in.getData(), static_cast<PEGASUS_STD(streamsize)>(in.size()));
    }
};

PEGASUS_NAMESPACE_END

#endif
