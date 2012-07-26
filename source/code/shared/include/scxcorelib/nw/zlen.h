#ifndef _base_strings_h
#define _base_strings_h

#include <scxcorelib/nw/common.h>
#include <wchar.h>
#include <string.h>

/* Maximum path length */
#define MAX_PATH_SIZE 1024

BEGIN_EXTERNC

MI_INLINE int Wcscmp(const wchar_t* s1, const wchar_t* s2)
{
    return wcscmp(s1, s2);
}

MI_INLINE int Strcmp(const char* s1, const char* s2)
{
    return strcmp(s1, s2);
}

MI_INLINE int Zcmp(const MI_Char* s1, const MI_Char* s2)
{
#if (MI_CHAR_TYPE == 1)
    return Strcmp(s1, s2);
#else
    return Wcscmp(s1, s2);
#endif
}

MI_INLINE size_t Strlen(const char* str)
{
    return strlen(str);
}

MI_INLINE size_t Wcslen(const wchar_t* str)
{
    return wcslen(str);
}

MI_INLINE size_t Zlen(const MI_Char* str)
{
#if (MI_CHAR_TYPE == 1)
    return Strlen(str);
#else
    return Wcslen(str);
#endif
}

MI_INLINE char* Strdup(const char* s)
{
#if defined(_MSC_VER)
    return _strdup(s);
#else
    return strdup(s);
#endif
}

#if (MI_CHAR_TYPE != 1)
MI_INLINE wchar_t* Wcsdup(const wchar_t* s)
{
#if defined(_MSC_VER)
    return _wcsdup(s);
#else
    extern wchar_t* wcsdup(const wchar_t*);
    return wcsdup(s);
#endif
}
#endif

MI_INLINE MI_Char* Zdup(const MI_Char* s)
{
#if (MI_CHAR_TYPE == 1)
    return Strdup(s);
#else
    return Wcsdup(s);
#endif
}

MI_INLINE int Strcasecmp(const char* s1, const char* s2)
{
#if defined(_MSC_VER)
    return _stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

#if (MI_CHAR_TYPE != 1)
MI_INLINE int Wcscasecmp(const wchar_t* s1, const wchar_t* s2)
{
#if defined(_MSC_VER)
    return _wcsicmp(s1, s2);
#else
# if defined(__GNUC__)
    extern int wcscasecmp(const wchar_t*, const wchar_t*);
# endif
    return wcscasecmp(s1, s2);
#endif
}
#endif

MI_INLINE int Zcasecmp(const MI_Char* s1, const MI_Char* s2)
{
#if (MI_CHAR_TYPE == 1)
    return Strcasecmp(s1, s2);
#else
    return Wcscasecmp(s1, s2);
#endif
}

MI_INLINE int Strncasecmp(const char* s1, const char* s2, size_t n)
{
#if defined(_MSC_VER)
    return _strnicmp(s1, s2, n);
#else
    return strncasecmp(s1, s2, n);
#endif
}

#if (MI_CHAR_TYPE != 1)
MI_INLINE int Wcsncasecmp(const wchar_t* s1, const wchar_t* s2, size_t n)
{
#if defined(_MSC_VER)
    return _wcsnicmp(s1, s2, n);
#else
# if defined(__GNUC__)
    extern int wcsncasecmp(const wchar_t*, const wchar_t*, size_t n);
# endif
    return wcsncasecmp(s1, s2, n);
#endif
}
#endif

MI_INLINE int Zncasecmp(const MI_Char* s1, const MI_Char* s2, size_t n)
{
#if (MI_CHAR_TYPE == 1)
    return Strncasecmp(s1, s2, n);
#else
    return Wcsncasecmp(s1, s2, n);
#endif
}

MI_INLINE unsigned long Strtoul(const char* str, char** end, int base)
{
    return strtoul(str, end, base);
}

#if (MI_CHAR_TYPE != 1)
MI_INLINE unsigned long Wcstoul(const wchar_t* str, wchar_t** end, int base)
{
    return wcstoul(str, end, base);
}
#endif

MI_INLINE unsigned long Ztoul(const MI_Char* str, MI_Char** end, int base)
{
#if (MI_CHAR_TYPE == 1)
    return Strtoul(str, end, base);
#else
    return Wcstoul(str, end, base);
#endif
}

MI_INLINE long Strtol(const char* str, char** end, int base)
{
    return strtol(str, end, base);
}

#if (MI_CHAR_TYPE != 1)
MI_INLINE long Wcstol(const wchar_t* str, wchar_t** end, int base)
{
    return wcstol(str, end, base);
}
#endif

MI_INLINE long Ztol(const MI_Char* str, MI_Char** end, int base)
{
#if (MI_CHAR_TYPE == 1)
    return Strtol(str, end, base);
#else
    return Wcstol(str, end, base);
#endif
}

MI_INLINE MI_Uint64 Strtoull(const char* str, char** end, int base)
{
#if defined(_MSC_VER)
    return _strtoui64(str, end, base);
#else
    return strtoull(str, end, base);
#endif
}

#if (MI_CHAR_TYPE != 1)
MI_INLINE MI_Uint64 Wcstoull(const wchar_t* str, wchar_t** end, int base)
{
#if defined(_MSC_VER)
    return _wcstoui64(str, end, base);
#else
    extern unsigned long long wcstoull(const wchar_t* s, wchar_t** e, int b);
    return wcstoull(str, end, base);
#endif
}
#endif

MI_INLINE MI_Uint64 Ztoull(const MI_Char* str, MI_Char** end, int base)
{
#if (MI_CHAR_TYPE == 1)
    return Strtoull(str, end, base);
#else
    return Wcstoull(str, end, base);
#endif
}

/*BOOKMARK*/
MI_INLINE MI_Sint64 Strtoll(const char* str, char** end, int base)
{
#if defined(_MSC_VER)
    return _strtoi64(str, end, base);
#else
    return strtoll(str, end, base);
#endif
}

#if (MI_CHAR_TYPE != 1)
MI_INLINE MI_Sint64 Wcstoll(const wchar_t* str, wchar_t** end, int base)
{
#if defined(_MSC_VER)
    return _wcstoi64(str, end, base);
#else
    extern long long wcstoll(const wchar_t* s, wchar_t** e, int b);
    return wcstoll(str, end, base);
#endif
}
#endif

MI_INLINE MI_Sint64 Ztoll(const MI_Char* str, MI_Char** end, int base)
{
#if (MI_CHAR_TYPE == 1)
    return Strtoll(str, end, base);
#else
    return Wcstoll(str, end, base);
#endif
}

MI_INLINE double Strtod(const char* str, char** end)
{
    return strtod(str, end);
}

MI_INLINE double Wcstod(const wchar_t* str, wchar_t** end)
{
    return wcstod(str, end);
}

MI_INLINE double Ztod(const MI_Char* str, MI_Char** end)
{
#if (MI_CHAR_TYPE == 1)
    return Strtod(str, end);
#else
    return Wcstod(str, end);
#endif
}

MI_INLINE char* Strcat(char* dest, size_t count, const char* src)
{
#if defined(_MSC_VER)
    strcat_s(dest, count, src);
    return dest;
#else
    if (count > 0)
    {
        return strcat(dest, src);
    }
    else
    {
        return dest;
    }
#endif
}

MI_INLINE char* Strtok(char* str, const char* delim, char** context)
{
#if defined(_MSC_VER)
    return strtok_s(str, delim, context);
#else
    return strtok_r(str, delim, context);
#endif
}

size_t Strlcat(char* dest, const char* src, size_t size);

size_t Strlcpy(char* dest, const char* src, size_t size);

size_t ZStrlcat(MI_Char* dest, const char* src, size_t size);

size_t ZStrlcpy(MI_Char* dest, const char* src, size_t size);

size_t Zlcat(MI_Char* dest, const MI_Char* src, size_t size);

size_t Zlcpy(MI_Char* dest, const MI_Char* src, size_t size);

const char* Uint32ToStr(char buf[12], MI_Uint32 x, size_t* size);

const char* Uint64ToStr(char buf[12], MI_Uint64 x, size_t* size);

#if (MI_CHAR_TYPE == 1)
INLINE const MI_Char* Uint32ToZStr(MI_Char buf[11], MI_Uint32 x, size_t* size)
{
    return Uint32ToStr(buf, x, size);
}
INLINE const MI_Char* Uint64ToZStr(MI_Char buf[21], MI_Uint64 x, size_t* size)
{
    return Uint64ToStr(buf, x, size);
}
#else
const MI_Char* Uint32ToZStr(MI_Char buf[12], MI_Uint32 x, size_t* size);
const MI_Char* Uint64ToZStr(MI_Char buf[12], MI_Uint64 x, size_t* size);
#endif

END_EXTERNC

#endif /* _base_strings_h */
