#ifndef _base_io_h
#define _base_io_h

#include <scxcorelib/nw/common.h>
#include <stdio.h>
#include <stdarg.h>

BEGIN_EXTERNC

FILE* Fopen(const char* path, const char* mode);

PRINTF_FORMAT(3, 4)
int Snprintf(char* buf, size_t size, const char* fmt, ...);

int Vsnprintf(char* buf, size_t size, const char* fmt, va_list ap);

PRINTF_FORMAT(2, 3)
void Fzprintf(FILE* os, const MI_Char* format, ...);

PRINTF_FORMAT(1, 2)
void Zprintf(const MI_Char* format, ...);

PRINTF_FORMAT(3, 4)
int Szprintf(MI_Char* buffer, size_t count, const MI_Char* format, ...);

/*
ATTN:
*/
#if defined(_WIN32) || defined( linux )
#define MI_GET_SAFE_PRINTF_STRING(p)  (p)
#else
#define MI_GET_SAFE_PRINTF_STRING(p)  ((p) ? (p) : "<null>")
#endif

END_EXTERNC

#endif /* _base_io_h */
