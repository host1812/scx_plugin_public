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

#include "CredentialCache.h"
#include <Executor/Strlcat.h>
#include <Executor/Strlcpy.h>
#include <Pegasus/Common/System.h>
#include <Pegasus/Common/Mutex.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

PEGASUS_NAMESPACE_BEGIN

//==============================================================================
//
// Local definitions
//
//==============================================================================

static const int SALT_SIZE = 16;

// Return per-process salt (create on first call).
static unsigned char* _getSalt()
{
    static Mutex _mutex;
    static unsigned char* _salt = 0;
    static int _initialized = 0;

    if (_initialized == 0)
    {
        _mutex.lock();

        if (_initialized == 0)
        {
            for (;;)
            {
                // Seed the random number generator (use /dev/urandom if 
                // available).

                if (RAND_load_file("/dev/urandom", 1024) == 0)
                {
                    // Use current time to seed (4 bytes of expected entropy).
                    Uint32 sec;
                    Uint32 usec;
                    System::getCurrentTimeUsec(sec, usec);
                    Uint64 bytes = Uint64(sec) * 100000 + Uint64(usec);
                    RAND_add(&bytes, sizeof(bytes), 4.0);
                }

                // Allocate salt.

                if (!(_salt = (unsigned char*)malloc(SALT_SIZE)))
                {
                    break;
                }

                // Initialize memory to be sure it is committed.
                memset(_salt, 0, SALT_SIZE);

                // Attempt to lock salt memory. If it fails, then don't put 
                // salt in this memory location.

                if (!System::mlock(_salt, SALT_SIZE))
                {
                    free(_salt);
                    _salt = 0;
                    break;
                }

                // Seed random number generator.

                if (RAND_bytes(_salt, SALT_SIZE) == 0)
                {
                    memset(_salt, 0xFF, SALT_SIZE);
                    free(_salt);
                    _salt = 0;
                    break;
                }

                break;
            }

            _initialized = 1;
        }

        _mutex.unlock();
    }

    return _salt;
}

// Get a message digest object for the strongest supported hash algorithm.
static const EVP_MD* _getDigest()
{
    static Mutex _mutex;
    static const EVP_MD* _md = 0;
    static int _initialized = 0;

    // Use double-checked locking to avoid mutex locking after 1st call.
    if (_initialized == 0)
    {
        _mutex.lock();

        if (_initialized == 0)
        {
            OpenSSL_add_all_digests();

            if (!(_md = EVP_get_digestbyname("sha512")) &&
                !(_md = EVP_get_digestbyname("sha384")) &&
                !(_md = EVP_get_digestbyname("sha256")) &&
                !(_md = EVP_get_digestbyname("sha224")) &&
                !(_md = EVP_get_digestbyname("sha1")))
            {
                // No suitable message digtest found.
            }

            _initialized = 1;
        }

        _mutex.unlock();
    }

    return _md;
}

// Create a hash from the given data using SSL routines.
static bool _makeHash(
    const char* data,
    unsigned int dataSize,
    unsigned char* salt,
    unsigned int saltSize,
    unsigned char hash[CredentialCache::MAX_HASH_BYTES],
    unsigned int& hashSize)
{
    const EVP_MD* md;

    if (!data || !salt)
    {
        return false;
    }

    if (!(md = _getDigest()))
    {
        // Could not find a suitable hashing digest object.
        return false;
    }

    EVP_MD_CTX ctx;
    EVP_DigestInit(&ctx, md);
    EVP_DigestUpdate(&ctx, data, dataSize);
    EVP_DigestUpdate(&ctx, salt, saltSize);
    EVP_DigestFinal(&ctx, hash, &hashSize);

    // Success
    return true;
}

// Returns a timestamp, which is the number of milliseconds elapsed since
// the Epoch.
static Uint64 _currentTime()
{
    Uint32 sec;
    Uint32 msec;
    System::getCurrentTime(sec, msec);

    return (sec * 1000) + msec;
}

static void _formatHex(const unsigned char* hash, unsigned int size, char* str)
{
    for (unsigned int i = 0; i < size; i++)
        sprintf(&str[i*2], "%02X", hash[i]);
}

//==============================================================================
//
// class CredentialCache
//
//==============================================================================

CredentialCache::CredentialCache(Uint64 lifeSpan) : 
    _cache(0),
    _size(0),
    _lifeSpan(lifeSpan)
{
    _cache = new Entry[MAX_CACHE_ITEMS];

    // init memory and make sure it's comitted
    memset(_cache, 0, sizeof(Entry) * MAX_CACHE_ITEMS);
   
    // lock memory to prevent it from being 'swapped'
    if (!System::mlock(_cache, sizeof(Entry) * MAX_CACHE_ITEMS))
    {   // if failed - do not store any cache
        delete [] _cache;
        _cache = 0;
    }
}

CredentialCache::CredentialCache(const CredentialCache& x) :
    _cache(0),
    _size(0),
    _lifeSpan(0)
{
    *this = x;
}

CredentialCache::~CredentialCache()
{
    if (_cache)
    {
        memset(_cache, 0, sizeof(Entry) * MAX_CACHE_ITEMS);
        System::munlock(_cache, sizeof(Entry) * MAX_CACHE_ITEMS);
        delete [] _cache;
    }
}

CredentialCache& CredentialCache::operator=(const CredentialCache& x)
{
    if (this== &x)
        return *this;
    
    if (_cache)
    {
        memset(_cache, 0, sizeof(Entry) * MAX_CACHE_ITEMS);
        System::munlock(_cache, sizeof(Entry) * MAX_CACHE_ITEMS);
        delete [] _cache;
    }
    
    _lifeSpan = x._lifeSpan;
    _size = x._size;

    if (!x._cache)
        return *this;
    
    _cache = new Entry[MAX_CACHE_ITEMS];
    
    // init memory and make sure it's comitted
    memcpy(_cache, x._cache, sizeof(Entry) * MAX_CACHE_ITEMS);
    
    // lock memory to prevent it from being 'swapped'
    if (!System::mlock(_cache, sizeof(Entry) * MAX_CACHE_ITEMS))
    {   // if failed - do not store any cache
        memset(_cache, 0, sizeof(Entry) * MAX_CACHE_ITEMS);
        delete [] _cache;
        _cache = 0;
        _size = 0;
    }
    
    return *this;
}

void CredentialCache::put(const char* username, const char* password)
{
    // check if cache was disabled
    if (!_cache)
        return;
    
    // replace existing entry if it already exists.

    Uint32 pos = _find(username);

    if (pos == Uint32(-1))
        pos = _getInsertPosition(); // or find suitable position for new entry
    
    // Create new entry.

    Entry e;
    {
        // username:
        Strlcpy(e.username, username, sizeof(e.username));

        // hash:
        if (!_makeHash(password, strlen(password), _getSalt(), SALT_SIZE,
            e.hash, e.hashSize))
        {
            // Hashing failed, so reject caching attempt. This means check()
            // will fail, forcing OS-level authentication.
            return;
        }

        // timestamp:
        e.timestamp = _currentTime();
    }

    // Add entry to end of cache.
    _cache[pos] = e;

    // increment size if it's a new element
    if (pos == _size)
        _size++;
}

bool CredentialCache::check(const char* username, const char* password)
{
    // check if cache was disabled
    if (!_cache)
        return false;
    
    // If not found in cache, then return false.

    Uint32 pos = _find(username);

    if (pos == Uint32(-1))
        return false;

    const Entry& e = _cache[pos];

    // Remove entry and reject if entry is expired.

    if (e.timestamp + _lifeSpan < _currentTime())
    {
        _remove(pos);
        return false;
    }

    // Hash the incoming password.

    unsigned char hash[CredentialCache::MAX_HASH_BYTES];
    unsigned int hashSize;

    if (!_makeHash(password, strlen(password), _getSalt(), SALT_SIZE, hash, 
        hashSize))
    {
        return false;
    }

    // Check whether hash of incoming password is identical with hashed
    // passord.

    if (hashSize != e.hashSize || memcmp(hash, e.hash, hashSize) != 0)
    {
        return false;
    }

    // Success!
    return true;
}

Uint32 CredentialCache::_find(const char* username) const
{
    // Search for the given entry.

    for (Uint32 i = 0; i < _size; i++)
    {
    const Entry& e = _cache[i];

        if (*e.username == *username && strcmp(e.username, username) == 0)
            return i;
    }

    // Not found!
    return Uint32(-1);
}

Uint32 CredentialCache::_getInsertPosition() const
{
    if (_size < MAX_CACHE_ITEMS)
        return _size;

    Uint32 min_pos = 0;
    Uint64 min_timestamp = _cache[0].timestamp;
    
    // find the oldest one
    for (Uint32 i = 0; i < _size; i++)
    {
            const Entry& e = _cache[i];

        if (e.timestamp < min_timestamp)
        {
            min_pos = i;
            min_timestamp = e.timestamp;
        }
    }
    
    return min_pos;
}

void CredentialCache::_remove(Uint32 pos)
{
    // move tail
    memmove(
        _cache + pos, 
        _cache + pos + 1, 
        sizeof(_cache[0]) * (MAX_CACHE_ITEMS - pos - 1));

    // dec size
    _size--;
}

void CredentialCache::print() const
{
    printf("==== CredentialCache\n");

    for (Uint32 i = 0; i < _size; i++)
    {
        const Entry& e = _cache[i];

        char str[CredentialCache::MAX_HASH_BYTES * 2 + 1];
        _formatHex(e.hash, e.hashSize, str);

        printf("username{%s} hash{%s} timestamp{%"
            PEGASUS_64BIT_CONVERSION_WIDTH "u}\n",
            e.username,
            str,
            e.timestamp);
    }
}

PEGASUS_NAMESPACE_END
