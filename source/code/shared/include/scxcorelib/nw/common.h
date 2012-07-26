#ifndef _common_h
#define _common_h

#include <scxcorelib/nw/config.h>

#if defined(CONFIG_OS_LINUX)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <scxcorelib/nw/MI.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if defined(_MSC_VER)
# define GetPath __GetPath__
# include <winsock2.h>
# include <windows.h>
# include <io.h>
# include <sal.h>
# include <direct.h>
# undef GetPath
#else
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <wchar.h>
#endif


/*
**==============================================================================
**
** UNREACHABLE_RETURN
**
**==============================================================================
*/

#if defined(CONFIG_PLATFORM_HPUX_IA64_HP)
# define UNREACHABLE_RETURN(EXPR) EXPR
#else
# define UNREACHABLE_RETURN(EXPR) /* empty */
#endif

/*
**==============================================================================
**
** TRACE macro
**
**==============================================================================
*/

#define TRACE printf("TRACE: %s(%u)\n", __FILE__, __LINE__)

/*
**==============================================================================
**
** Extended error codes (requires casting).
**
**==============================================================================
*/

#define MI_RESULT_CANCELED ((MI_Result)1000)
#define MI_RESULT_OPEN_FAILED ((MI_Result)1001)
#define MI_RESULT_INVALID_CLASS_HIERARCHY ((MI_Result)1002)
#define MI_RESULT_WOULD_BLOCK ((MI_Result)1003)
#define MI_RESULT_TIME_OUT ((MI_Result)1004)
#define MI_RESULT_IN_PROGRESS ((MI_Result)1005)
#define MI_RESULT_NOT_FOUND ((MI_Result)1006)

/*
**==============================================================================
**
** For testing the array bit of the MI_Type enumeration.
**
**==============================================================================
*/

#define MI_ARRAY_BIT 0x10

/*
**==============================================================================
**
** ATTN: Temporary placeholders for various following argument types (replace
** these later with correct parameters).
**
**==============================================================================
*/

#define __nameSpace ((const MI_Char*)0)
#define __className ((const MI_Char*)0)
#define __methodName ((const MI_Char*)0)
#define __bookmark ((const MI_Char*)0)
#define __subscriptionIDCount ((MI_Uint32)0)
#define __subscriptionSelfPtr ((void**)0)
#define __subscriptionSelf ((void*)0)
#define __postIndication NULL

/*
**==============================================================================
**
** Basic type definitions.
**
**==============================================================================
*/

typedef MI_Char* MI_String;
typedef const MI_Char* MI_ConstString;

/*
**==============================================================================
**
** MI_UNUSED
**
**==============================================================================
*/
#define MI_UNUSED(X) ((void)X)

/*
**==============================================================================
**
** EXTERNC
** BEGIN_EXTERNC
** END_EXTERNC
**
**==============================================================================
*/

#if defined(__cplusplus)
# define EXTERNC extern "C"
# define BEGIN_EXTERNC extern "C" {
# define END_EXTERNC }
#else
# define EXTERNC /* empty */
# define BEGIN_EXTERNC /* empty */
# define END_EXTERNC /* empty */
#endif

/*
**==============================================================================
**
** INLINE macro
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define INLINE static __inline
#elif defined(__GNUC__)
# define INLINE static __inline
#elif defined(sun)
# define INLINE static inline
#elif defined(aix)
# define INLINE static inline
#else
# define INLINE static __inline
#endif

/*
**==============================================================================
**
** FUNCTION_NEVER_RETURNS macro
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define FUNCTION_NEVER_RETURNS __declspec(noreturn)
#else
# define FUNCTION_NEVER_RETURNS
#endif

/* 
**==============================================================================
**
** UINT64_FMT macro
** SINT64_FMT macro
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define UINT64_FMT "%I64u"
# define SINT64_FMT "%I64d"
# define UINT64_FMT_T MI_T("%I64u")
# define SINT64_FMT_T MI_T("%I64d")
#else
# define UINT64_FMT "%llu"
# define SINT64_FMT "%lld"
# define UINT64_FMT_T MI_T("%llu")
# define SINT64_FMT_T MI_T("%lld")
#endif

/*
**==============================================================================
**
** MI_RETURN(RESULT)
**
**     Macro to return the given RESULT (other modules may override this macro 
**     to add tracing or logging of error return values).
**
**==============================================================================
*/

#define MI_RETURN(RESULT) \
    for (;;) \
    { \
        return RESULT; \
    }

/*
**==============================================================================
**
** MI_RETURN_ERR(RESULT)
**
**     Macro to return when RESULT is not MI_RESULT_OK (other modules may
**     override this macro to add tracing and logging of error return values).
**
**==============================================================================
*/

#define MI_RETURN_ERR(RESULT) \
    for (;;) \
    { \
        MI_Result r = RESULT; \
        if (r != MI_RESULT_OK) \
            return r; \
        break; \
    }

/*
**==============================================================================
**
** PRINTF_FORMAT(N,M)
**
**     Use this macro on functions that support a printf-style format argument.
**     This permits some compilers to check the arguments.
**
**==============================================================================
*/

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define PRINTF_FORMAT(N,M) __attribute__((format(printf, N, M)))
#else
# define PRINTF_FORMAT(N,M) /* empty */
#endif

/*
**==============================================================================
**
**  DEBUG_ASSERT()
**
**==============================================================================
*/

#if defined(CONFIG_ENABLE_DEBUG)
# define DEBUG_ASSERT(COND) assert(COND)
#else
# define DEBUG_ASSERT(COND) /* empty */
#endif

/*
**==============================================================================
**
**  stat() macros
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define access _access
# define F_OK 0
# define W_OK 2
# define R_OK 4
#endif

/*
**==============================================================================
**
** NULL_FILE macro
**
**==============================================================================
*/

/* null-file - always can be opened/written and always ignored */
#if defined(_MSC_VER)
# define NULL_FILE "nul"
#else
# define NULL_FILE "/dev/null"
#endif

/*
**==============================================================================
**
**  __FUNCTION__
**
**==============================================================================
*/

#if !defined(CONFIG_HAVE_FUNCTION_MACRO)
# if !defined(_MSC_VER)
#  define __FUNCTION__ "<unknown>"
# endif
#endif

/*
**==============================================================================
**
**  CallSite type (used to track site of function invocations when enabled).
**
**==============================================================================
*/

#if defined(CONFIG_ENABLE_DEBUG)

typedef struct _CallSite
{
    const char* file;
    size_t line;
    const char* func;
}
CallSite;

MI_INLINE CallSite MakeCallSite(const char* file, size_t line, const char* func)
{
    CallSite result;
    result.file = file;
    result.line = line;
    result.func = func;
    return result;
}

# define CALLSITE MakeCallSite(__FILE__, __LINE__, __FUNCTION__)

#else /* defined(CONFIG_ENABLE_DEBUG) */

typedef int CallSite;

# define CALLSITE 0

#endif /* !defined(CONFIG_ENABLE_DEBUG) */

#endif /* _common_h */
