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

#ifndef Pegasus_Security_Authentication_CredentialCache_h
#define Pegasus_Security_Authentication_CredentialCache_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Array.h>
#include <Pegasus/Common/ArrayInternal.h>
#include <Pegasus/Security/Authentication/Linkage.h>

PEGASUS_NAMESPACE_BEGIN

class PEGASUS_SECURITY_LINKAGE CredentialCache
{
public:

    /** Cache entries expire after 60 seconds by default.
    */
    enum { DEFAULT_LIFE_SPAN_MSEC = 60000 };

    /** The longest username we will accept (including zero-terminator).
    */
    enum { MAX_USERNAME = 64 };

    /** The largest hash size used by this implementation is 512 bits (sha512).
    */
    enum { MAX_HASH_BYTES = 64 };
    
    /** The number of cached items (different users)
    */
    enum { MAX_CACHE_ITEMS = 16 };

    /** Constructor. The lifeSpan parameter specifies how long it takes
        for a cache entry to expire.
    */
    CredentialCache(Uint64 lifeSpan = DEFAULT_LIFE_SPAN_MSEC);

    /** Copy constructor.
    */
    CredentialCache(const CredentialCache& x);

    /** Destructor.
    */
    ~CredentialCache();

    /** Assignment operator.
    */
    CredentialCache& operator=(const CredentialCache& x);

    /** Put or update the entry with the given username.
    */
    void put(const char* username, const char* password);

    /** Return true if the cache has a non-expired entry with the given 
        username and password. Else, return false. It does not matter why
        the check failed, since all failures should result in a full OS-level
        authentication.
    */
    bool check(const char* username, const char* password);

    /** Print out cache.
    */
    void print() const;

private:

    /** Find the entry with the given username.
    */
    Uint32 _find(const char* username) const;

    /** return position for insertion a new item - either last empty or the 
        olderst item
    */
    Uint32 _getInsertPosition() const;

    /** remove entry from internal array
    */
    void _remove(Uint32 pos);

    struct Entry
    {
        // The username (64-byte maximum).
        char username[MAX_USERNAME];

        // Hash of the password (512 bits maximum).
        unsigned char hash[MAX_HASH_BYTES];

        // Actual length of hash in bytes.
        unsigned int hashSize;

        // Time entry was created (number of milliseconds since epoch).
        Uint64 timestamp;
    };

    // The cache is represented as a simple array.
    Entry* _cache;
    Uint32 _size;
    Uint64 _lifeSpan;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_Security_Authentication_CredentialCache_h */
