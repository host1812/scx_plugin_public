#include <scxcorelib/nw/base/io.h>

#if defined(__GNUC__)
//extern int vfwprintf(FILE*, const wchar_t*, va_list);
//extern int vwprintf(const wchar_t*, va_list);
//extern int vswprintf(wchar_t*, size_t, const wchar_t*, va_list);
#endif

FILE* Fopen(const char* path, const char* mode)
{
#if defined(_MSC_VER)
    FILE* fp;
    return fopen_s(&fp, path, mode) == 0 ? fp : NULL;
#else
    return fopen(path, mode);
#endif
}

int Snprintf(char* buf, size_t size, const char* fmt, ...)
{
    va_list ap;
    int r;

    memset(&ap, 0, sizeof(ap));

    va_start(ap, fmt);
#if defined(_MSC_VER)
    r = _vsnprintf_s(buf, size, size, fmt, ap);
#else
    r = vsnprintf(buf, size, fmt, ap);
#endif
    va_end(ap);

    return r;
}

int Vsnprintf(char* buf, size_t size, const char* fmt, va_list ap)
{
#if defined(_MSC_VER)
    return _vsnprintf_s(buf, size, size, fmt, ap);
#else
    return vsnprintf(buf, size, fmt, ap);
#endif
}

void Fzprintf(FILE* os, const MI_Char* format, ...)
{
    va_list ap;
    memset(&ap, 0, sizeof(ap));
    va_start(ap, format);

#if (MI_CHAR_TYPE == 1)
    vfprintf(os, format, ap);
#else
    vfwprintf(os, format, ap);
#endif

    va_end(ap);
}

void Zprintf(const MI_Char* format, ...)
{
    va_list ap;
    memset(&ap, 0, sizeof(ap));

    va_start(ap, format);
#if (MI_CHAR_TYPE == 1)
    vprintf(format, ap);
#else
    vwprintf(format, ap);
#endif
    va_end(ap);
}

int Szprintf(MI_Char* buffer, size_t count, const MI_Char* format, ...)
{
    int result;
    va_list ap;
    memset(&ap, 0, sizeof(ap));

    va_start(ap, format);
#if (MI_CHAR_TYPE == 1)
# if defined(_MSC_VER)
    result = _vsnprintf_s(buffer, count, _TRUNCATE, format, ap);
# else
    result = vsnprintf(buffer, count, format, ap);
# endif
#else
# if defined(_MSC_VER)
    result = _vsnwprintf_s(buffer, count, _TRUNCATE, format, ap);
# else
    result = vswprintf(buffer, count, format, ap);
# endif
#endif
    va_end(ap);

    return result;
}
