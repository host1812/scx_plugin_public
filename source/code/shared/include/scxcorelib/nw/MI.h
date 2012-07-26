/*
**==============================================================================
**
** Management Interface (MI)
**
**     This file defines the management interface.
**
**==============================================================================
*/

#ifndef _MI_h
#define _MI_h

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/*
**==============================================================================
**
**  Use eight-byte packing for structures (on Windows)
**
**==============================================================================
*/

#if defined(_MSC_VER)
# pragma pack(push,8)
#endif

/*
**==============================================================================
**
** MI_CHAR_TYPE
**
**     Indicates the character type to use (1='char', 2='wchar_t'). This
**     can be overridden prior to including this file or on the compiler
**     command line (e.g., -DMI_CHAR_TYPE=1).
**
**==============================================================================
*/

#if defined(MI_CHAR_TYPE)
# if (MI_CHAR_TYPE != 1) && (MI_CHAR_TYPE != 2)
#  error "MI_CHAR_TYPE must be 1 or 2"
# endif
#else
# if defined(_MSC_VER)
#  define MI_CHAR_TYPE 2
# else
#  define MI_CHAR_TYPE 1
# endif
#endif

#if (MI_CHAR_TYPE == 2)
# define MI_USE_WCHAR
#endif

/*
**==============================================================================
**
** MI_CONST
** 
**     Provider managers and client transport implementations predefine this
**     macro to relax const checking on various structures within this file.
**
**==============================================================================
*/

#ifndef MI_CONST
# define MI_CONST const
#endif

/*
**==============================================================================
**
** MI_VERSION
**
**     Define the version number used the MI_Module.version field, which is
**     set in the MI_Main() entry point as follows:
**
**         module.version = MI_VERSION;
**
**==============================================================================
*/

#define MI_MAJOR   ((MI_Uint32)1)
#define MI_MINOR   ((MI_Uint32)0)
#define MI_REVISION ((MI_Uint32)0)
#define MI_REVISON MI_REVISION /* ATTN: preserve misspelled form */
#define MI_MAKE_VERSION(MAJ, MIN, REV) ((MAJ << 16) | (MIN << 8) | REV)
#define MI_VERSION MI_MAKE_VERSION(MI_MAJOR, MI_MINOR, MI_REVISON)

/*
**==============================================================================
**
** SAL notation (Windows only)
**
**     If the SAL macros are undefined, define empty ones.
**
**==============================================================================
*/

#if !defined(__in)
# define __in
#endif

#if !defined(__out)
# define __out
#endif

#if !defined(__in_opt)
# define __in_opt
#endif

#if !defined(__deref_out)
# define __deref_out
#endif

#if !defined(__deref_out_opt)
# define __deref_out_opt
#endif

#if !defined(__deref_out_z_opt)
# define __deref_out_z_opt
#endif

#if !defined(__in_z)
# define __in_z
#endif

#if !defined(__out_z)
# define __out_z
#endif

#if !defined(__in_z_opt)
# define __in_z_opt
#endif

#if !defined(__out_opt)
# define __out_opt
#endif

#if !defined(__out_ecount_z)
# define __out_ecount_z(count)
#endif

#if !defined(__inout_ecount_z)
# define __inout_ecount_z(count)
#endif

#if !defined(__format_string)
# define __format_string
#endif

/*
**==============================================================================
**
** MI_EXPORT
**
**     This macro exports the MI_Main() entry point. For example:
**
**         MI_EXPORT MI_Module* MI_MAIN_CALL MI_Main(MI_Server* server)
**         {
**         }
**
**     The macro appears first in the definition.
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define MI_EXPORT __declspec(dllexport)
# define MI_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
# define MI_EXPORT __attribute__((visibility("default")))
# define MI_IMPORT /* empty */
#elif defined(sun)
# define MI_EXPORT __global 
# define MI_IMPORT /* empty */
#else
# define MI_EXPORT 
# define MI_IMPORT
#endif

/*
**==============================================================================
**
** MI_MAIN_CALL
**
**     This macro specifies the MI_Main() calling convention. For example:
**
**         MI_EXPORT MI_Module* MI_MAIN_CALL MI_Main(MI_Server* server)
**         {
**             ...
**         }
**
**     The macro appears directly before the function name.
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define MI_MAIN_CALL __cdecl
#else
# define MI_MAIN_CALL /* empty */
#endif

/*
**==============================================================================
**
** MI_CALL
**
**     This macros specifies the calling convention for all functions other
**     than MI_Main(). For example:
**
**         MI_INLINE MI_Result MI_CALL MI_PostResult(...);
**
**     The macro appears directly before the function name.
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define MI_CALL __stdcall
#else
# define MI_CALL /* empty */
#endif

/*
**==============================================================================
**
** MI_INLINE
**
**     This macro provides a platform-independent method for specifying that
**     a function is in-line. For compilers that do not support in-lining, define
**     MI_INLINE as 'static' instead.
**
**==============================================================================
*/

#if defined(_MSC_VER)
# define MI_INLINE static __inline
#elif defined(__GNUC__)
# define MI_INLINE static __inline
#elif defined(sun)
# define MI_INLINE static inline
#else
# define MI_INLINE static __inline
#endif

/*
**==============================================================================
**
** MI_OFFSETOF
**
**     This macro obtains the byte-offset of a field within a structure. It is
**     used in schema.c to obtain the offsets of generated structure fields.
**
**==============================================================================
*/

#define MI_OFFSETOF(STRUCT,FIELD) (((ptrdiff_t)&(((STRUCT*)1)->FIELD))-1)

/*
**==============================================================================
**
** MI_EXTERN_C
**
**     This macro forces a function to use the C function naming convention 
**     (rather than C++ mangled naming convention).
**
**==============================================================================
*/

#ifdef __cplusplus
#   define  MI_EXTERN_C  extern "C"
#else
#   define  MI_EXTERN_C  extern
#endif

/*
**==============================================================================
**
** MI_COUNT()
**
**     This macro obtains the element count of an array. For example:
**
**         const char* DATA[] = { "Red", "Green", "Blue" };
**         size_t COUNT = MI_COUNT(DATA);
**
**     'COUNT' in this example is 3 (the number of array elements). This
**     macro is used by schema.c to obtain array lengths.
**
**==============================================================================
*/

#define MI_COUNT(X) (sizeof(X)/sizeof(X[0]))

/*
**==============================================================================
**
** MI_T()
**
**     This macro conditionally places the 'L' character in front of a string
**     literal (when using wide-character strings).
**
**==============================================================================
*/

#if (MI_CHAR_TYPE == 1)
# define MI_T(STR) STR
#else
# define MI_T(STR) L##STR
#endif

/*
**==============================================================================
**
** MI_LL()
** MI_ULL()
**
**     Macros for adding endings for sint64 and uint64 literals (used within
**     schema.c).
**
**==============================================================================
*/

#if defined (_MSC_VER)
# define MI_LL(X) X##i64
# define MI_ULL(X) X##ui64
#else
# define MI_LL(X) X##LL
# define MI_ULL(X) X##ULL
#endif

/*
**==============================================================================
**
** Forward structure typedef declarations.
**
**==============================================================================
*/

typedef struct _MI_Server MI_Server;
typedef struct _MI_Context MI_Context;
typedef struct _MI_ClassDecl MI_ClassDecl;
typedef struct _MI_Instance MI_Instance;
typedef struct _MI_Filter MI_Filter;
typedef struct _MI_PropertySet MI_PropertySet;
typedef struct _MI_Qualifier MI_Qualifier;
typedef struct _MI_Session MI_Session;
typedef struct _MI_ServerFT MI_ServerFT;
typedef struct _MI_ProviderFT MI_ProviderFT;
typedef struct _MI_PropertySetFT MI_PropertySetFT;
typedef struct _MI_InstanceFT MI_InstanceFT;
typedef struct _MI_ContextFT MI_ContextFT;
typedef struct _MI_FilterFT MI_FilterFT;

/*
**==============================================================================
**
** MI_Result
**
**     This enumeration defines function return codes. These codes are 
**     specified in [1].
**
**     [1] See DSP0004 (DMTF document number).
**
**==============================================================================
*/

typedef enum _MI_Result
{
    /* The operation was successful */
    MI_RESULT_OK = 0,

    /* A general error occurred, not covered by a more specific error code. */
    MI_RESULT_FAILED = 1,

    /* Access to a CIM resource is not available to the client. */
    MI_RESULT_ACCESS_DENIED = 2,

    /* The target namespace does not exist. */
    MI_RESULT_INVALID_NAMESPACE = 3,

    /* One or more parameter values passed to the method are not valid. */
    MI_RESULT_INVALID_PARAMETER  = 4,

    /* The specified class does not exist. */
    MI_RESULT_INVALID_CLASS = 5,

    /* The requested object cannot be found. */
    MI_RESULT_NOT_FOUND = 6,

    /* The requested operation is not supported. */
    MI_RESULT_NOT_SUPPORTED = 7,

    /* The operation cannot be invoked because the class has subclasses. */
    MI_RESULT_CLASS_HAS_CHILDREN = 8,

    /* The operation cannot be invoked because the class has instances. */
    MI_RESULT_CLASS_HAS_INSTANCES = 9,

    /* The operation cannot be invoked because the superclass does not exist. */
    MI_RESULT_INVALID_SUPERCLASS = 10,

    /* The operation cannot be invoked because an object already exists. */
    MI_RESULT_ALREADY_EXISTS = 11,

    /* The specified property does not exist. */
    MI_RESULT_NO_SUCH_PROPERTY = 12,

    /* The value supplied is not compatible with the type. */
    MI_RESULT_TYPE_MISMATCH = 13,

    /* The query language is not recognized or supported. */
    MI_RESULT_QUERY_LANGUAGE_NOT_SUPPORTED = 14,

    /* The query is not valid for the specified query language. */
    MI_RESULT_INVALID_QUERY = 15,

    /* The extrinsic method cannot be invoked. */
    MI_RESULT_METHOD_NOT_AVAILABLE = 16,

    /* The specified extrinsic method does not exist. */
    MI_RESULT_METHOD_NOT_FOUND = 17,

    /* The specified namespace is not empty. */
    MI_RESULT_NAMESPACE_NOT_EMPTY = 20,

    /* The enumeration identified by the specified context is invalid. */
    MI_RESULT_INVALID_ENUMERATION_CONTEXT = 21,

    /* The specified operation timeout is not supported by the CIM Server. */
    MI_RESULT_INVALID_OPERATION_TIMEOUT = 22,

    /* The Pull operation has been abandoned. */
    MI_RESULT_PULL_HAS_BEEN_ABANDONED = 23,

    /* The attempt to abandon a concurrent Pull operation failed. */
    MI_RESULT_PULL_CANNOT_BE_ABANDONED = 24,

    /* Using a filter in the enumeration is not supported by the CIM server. */
    MI_RESULT_FILTERED_ENUMERATION_NOT_SUPPORTED = 25,

    /* The CIM server does not support continuation on error. */
    MI_RESULT_CONTINUATION_ON_ERROR_NOT_SUPPORTED = 26,

    /* The operation failed because server limits were exceeded. */
    MI_RESULT_SERVER_LIMITS_EXCEEDED = 27,

    /* The CIM server is shutting down and cannot process the operation. */
    MI_RESULT_SERVER_IS_SHUTTING_DOWN = 28
}
MI_Result;

/*
**==============================================================================
**
** MI_ErrorCategory
**
**     This enumeration defines error categories for the PowerShell extensions.
**
**==============================================================================
*/

typedef enum _MI_ErrorCategory
{
    MI_ERRORCATEGORY_NOT_SPECIFIED,
    MI_ERRORCATEGORY_OPEN_ERROR,
    MI_ERRORCATEGORY_CLOS_EERROR,
    MI_ERRORCATEGORY_DEVICE_ERROR,
    MI_ERRORCATEGORY_DEADLOCK_DETECTED,
    MI_ERRORCATEGORY_INVALID_ARGUMENT,
    MI_ERRORCATEGORY_INVALID_DATA,
    MI_ERRORCATEGORY_INVALID_OPERATION,
    MI_ERRORCATEGORY_INVALID_RESULT,
    MI_ERRORCATEGORY_INVALID_TYPE,
    MI_ERRORCATEGORY_METADATA_ERROR,
    MI_ERRORCATEGORY_NOT_IMPLEMENTED,
    MI_ERRORCATEGORY_NOT_INSTALLED,
    MI_ERRORCATEGORY_OBJECT_NOT_FOUND,
    MI_ERRORCATEGORY_OPERATION_STOPPED,
    MI_ERRORCATEGORY_OPERATION_TIMEOUT,
    MI_ERRORCATEGORY_SYNTAX_ERROR,
    MI_ERRORCATEGORY_PARSER_ERROR,
    MI_ERRORCATEGORY_ACCESS_DENIED,
    MI_ERRORCATEGORY_RESOURCE_BUSY,
    MI_ERRORCATEGORY_RESOURCE_EXISTS,
    MI_ERRORCATEGORY_RESOURCE_UNAVAILABLE,
    MI_ERRORCATEGORY_READ_ERROR,
    MI_ERRORCATEGORY_SECURITY_ERROR
}
MI_ErrorCategory;

/*
**==============================================================================
**
** Bit flags
**
**==============================================================================
*/

/* CIM meta types (or qualifier scopes) */
#define MI_FLAG_CLASS           (1 << 0)
#define MI_FLAG_METHOD          (1 << 1)
#define MI_FLAG_PROPERTY        (1 << 2)
#define MI_FLAG_PARAMETER       (1 << 3)
#define MI_FLAG_ASSOCIATION     (1 << 4)
#define MI_FLAG_INDICATION      (1 << 5)
#define MI_FLAG_REFERENCE       (1 << 6)
#define MI_FLAG_ANY             (1|2|4|8|16|32|64)

/* Qualifier flavors */
#define MI_FLAG_ENABLEOVERRIDE  (1 << 7)
#define MI_FLAG_DISABLEOVERRIDE (1 << 8)
#define MI_FLAG_RESTRICTED      (1 << 9)
#define MI_FLAG_TOSUBCLASS      (1 << 10)
#define MI_FLAG_TOINSTANCE      (1 << 11)
#define MI_FLAG_TRANSLATABLE    (1 << 12)

/* Select boolean qualifier */
#define MI_FLAG_KEY             (1 << 13)
#define MI_FLAG_IN              (1 << 14)
#define MI_FLAG_OUT             (1 << 15)
#define MI_FLAG_REQUIRED        (1 << 16)
#define MI_FLAG_STATIC          (1 << 17)
#define MI_FLAG_ABSTRACT        (1 << 18)
#define MI_FLAG_TERMINAL        (1 << 19)
#define MI_FLAG_EXPENSIVE       (1 << 20)
#define MI_FLAG_STREAM          (1 << 21)

/* Special flags */
#define MI_FLAG_NULL            (1 << 29)
#define MI_FLAG_BORROW          (1 << 30)
#define MI_FLAG_ADOPT           ((MI_Uint32)0x80000000)

/*
**==============================================================================
**
** enum MI_Type
**
**     This enumeration defines type tags for the CIM data types [1]. These
**     tags specify the data type of qualifiers, properties, references,
**     parameters, and method return values. Tags ending in 'A' signify
**     arrays. All tags are within the range of 0 to 31, allowing them to be
**     stored in a single byte. The 0x10 bit of array tags is non-zero.
**
**     [1] See DSP0004 (DMTF document number).
**
**==============================================================================
*/

typedef enum _MI_Type
{
    MI_BOOLEAN = 0,
    MI_UINT8 = 1,
    MI_SINT8 = 2,
    MI_UINT16 = 3,
    MI_SINT16 = 4,
    MI_UINT32 = 5,
    MI_SINT32 = 6,
    MI_UINT64 = 7,
    MI_SINT64 = 8,
    MI_REAL32 = 9,
    MI_REAL64 = 10,
    MI_CHAR16 = 11,
    MI_DATETIME = 12,
    MI_STRING = 13,
    MI_REFERENCE = 14,
    MI_INSTANCE = 15,
    MI_BOOLEANA = 16,
    MI_UINT8A = 17,
    MI_SINT8A = 18,
    MI_UINT16A = 19,
    MI_SINT16A = 20,
    MI_UINT32A = 21,
    MI_SINT32A = 22,
    MI_UINT64A = 23,
    MI_SINT64A = 24,
    MI_REAL32A = 25,
    MI_REAL64A = 26,
    MI_CHAR16A = 27,
    MI_DATETIMEA = 28,
    MI_STRINGA = 29,
    MI_REFERENCEA = 30,
    MI_INSTANCEA = 31
}
MI_Type;

/*
** The bit that indicates that a type is an array type.
*/

#define MI_ARRAY_BIT 0x10

/*
**==============================================================================
**
** MI_Uint8
** MI_Sint8
** MI_Uint16
** MI_Sint16
** MI_Uint32
** MI_Sint32
** MI_Uint64
** MI_Sint64
** MI_Real32
** MI_Real64
** MI_Char16
** MI_Char
**
**     The following represent CIM data types.
**
**==============================================================================
*/

typedef unsigned char MI_Boolean;
typedef unsigned char MI_Uint8;
typedef signed char MI_Sint8;
typedef unsigned short MI_Uint16;
typedef signed short MI_Sint16;
typedef unsigned int MI_Uint32;
typedef signed int MI_Sint32;

#if defined(_MSC_VER)
typedef unsigned __int64 MI_Uint64;
typedef signed __int64 MI_Sint64;
#else
#include <sys/types.h>
#if defined(linux)
typedef u_int64_t MI_Uint64;
#else
typedef uint64_t MI_Uint64;
#endif
typedef int64_t MI_Sint64;
#endif

typedef float MI_Real32;
typedef double MI_Real64;
typedef unsigned short MI_Char16;

#if (MI_CHAR_TYPE == 1)
typedef char MI_Char;
#else
typedef wchar_t MI_Char;
#endif

#define MI_TRUE ((MI_Boolean)1)
#define MI_FALSE ((MI_Boolean)0)

/*
**==============================================================================
**
** MI_Timestamp
**
**     Represents a timestamp as described in the CIM Infrastructure 
**     specification
**
**     [1] MI_ee DSP0004 (http://www.dmtf.org/standards/published_documents)
**
**==============================================================================
*/

typedef struct _MI_Timestamp
{
    /* YYYYMMDDHHMMSS.MMMMMMSUTC */
    MI_Uint32 year;
    MI_Uint32 month;
    MI_Uint32 day;
    MI_Uint32 hour;
    MI_Uint32 minute;
    MI_Uint32 second;
    MI_Uint32 microseconds;
    MI_Sint32 utc;
}
MI_Timestamp;

/*
**==============================================================================
**
** struct MI_Interval
**
**     Represents an interval as described in the CIM Infrastructure 
**     specification. This structure is padded to have the same length
**     as a MI_Timestamp structure.
**
**     [1] MI_ee DSP0004 (http://www.dmtf.org/standards/published_documents)
**
**==============================================================================
*/

typedef struct _MI_Interval
{
    /* DDDDDDDDHHMMSS.MMMMMM:000 */
    MI_Uint32 days;
    MI_Uint32 hours;
    MI_Uint32 minutes;
    MI_Uint32 seconds;
    MI_Uint32 microseconds;
    MI_Uint32 __padding1;
    MI_Uint32 __padding2;
    MI_Uint32 __padding3;
}
MI_Interval;

/*
**==============================================================================
**
** struct MI_Datetime
**
**     Represents a CIM datetime type as described in the CIM Infrastructure
**     specification. It contains a union of MI_Timestamp and MI_Interval.
**
**==============================================================================
*/

typedef struct _MI_Datetime
{
    MI_Uint32 isTimestamp;
    union
    {
        MI_Timestamp timestamp;
        MI_Interval interval;
    }
    u;
}
MI_Datetime;

/*
**==============================================================================
**
** struct MI_<TYPE>A
**
**     These structure represent arrays of the types introduced above.
**
**==============================================================================
*/

typedef struct _MI_BooleanA
{
    MI_Boolean* data;
    MI_Uint32 size;
}
MI_BooleanA;

typedef struct _MI_Uint8A
{
    MI_Uint8* data;
    MI_Uint32 size;
}
MI_Uint8A;

typedef struct _MI_Sint8A
{
    MI_Sint8* data;
    MI_Uint32 size;
}
MI_Sint8A;

typedef struct _MI_Uint16A
{
    MI_Uint16* data;
    MI_Uint32 size;
}
MI_Uint16A;

typedef struct _MI_Sint16A
{
    MI_Sint16* data;
    MI_Uint32 size;
}
MI_Sint16A;

typedef struct _MI_Uint32A
{
    MI_Uint32* data;
    MI_Uint32 size;
}
MI_Uint32A;

typedef struct _MI_Sint32A
{
    MI_Sint32* data;
    MI_Uint32 size;
}
MI_Sint32A;

typedef struct _MI_Uint64A
{
    MI_Uint64* data;
    MI_Uint32 size;
}
MI_Uint64A;

typedef struct _MI_Sint64A
{
    MI_Sint64* data;
    MI_Uint32 size;
}
MI_Sint64A;

typedef struct _MI_Real32A
{
    MI_Real32* data;
    MI_Uint32 size;
}
MI_Real32A;

typedef struct _MI_Real64A
{
    MI_Real64* data;
    MI_Uint32 size;
}
MI_Real64A;

typedef struct _MI_Char16A
{
    MI_Char16* data;
    MI_Uint32 size;
}
MI_Char16A;

typedef struct _MI_DatetimeA
{
    MI_Datetime* data;
    MI_Uint32 size;
}
MI_DatetimeA;

typedef struct _MI_StringA
{
    MI_Char** data;
    MI_Uint32 size;
}
MI_StringA;

typedef struct _MI_ReferenceA
{
    struct _MI_Instance** data;
    MI_Uint32 size;
}
MI_ReferenceA;

typedef struct _MI_InstanceA
{
    MI_Instance** data;
    MI_Uint32 size;
}
MI_InstanceA;

typedef struct _MI_Array
{
    void* data;
    MI_Uint32 size;
}
MI_Array;

/*
**==============================================================================
**
** struct MI_Const<TYPE>A
**
**     These structure represent arrays of the types introduced above.
**
**==============================================================================
*/

typedef struct _MI_ConstBooleanA 
{
    MI_CONST MI_Boolean* data;
    MI_Uint32 size;
}
MI_ConstBooleanA;

typedef struct _MI_ConstUint8A 
{
    MI_CONST MI_Uint8* data;
    MI_Uint32 size;
}
MI_ConstUint8A;

typedef struct _MI_ConstSint8A 
{
    MI_CONST MI_Sint8* data;
    MI_Uint32 size;
}
MI_ConstSint8A;

typedef struct _MI_ConstUint16A 
{
    MI_CONST MI_Uint16* data;
    MI_Uint32 size;
}
MI_ConstUint16A;

typedef struct _MI_ConstSint16A 
{
    MI_CONST MI_Sint16* data;
    MI_Uint32 size;
}
MI_ConstSint16A;

typedef struct _MI_ConstUint32A 
{
    MI_CONST MI_Uint32* data;
    MI_Uint32 size;
}
MI_ConstUint32A;

typedef struct _MI_ConstSint32A 
{
    MI_CONST MI_Sint32* data;
    MI_Uint32 size;
}
MI_ConstSint32A;

typedef struct _MI_ConstUint64A 
{
    MI_CONST MI_Uint64* data;
    MI_Uint32 size;
}
MI_ConstUint64A;

typedef struct _MI_ConstSint64A 
{
    MI_CONST MI_Sint64* data;
    MI_Uint32 size;
}
MI_ConstSint64A;

typedef struct _MI_ConstReal32A 
{
    MI_CONST MI_Real32* data;
    MI_Uint32 size;
}
MI_ConstReal32A;

typedef struct _MI_ConstReal64A 
{
    MI_CONST MI_Real64* data;
    MI_Uint32 size;
}
MI_ConstReal64A;

typedef struct _MI_ConstChar16A 
{
    MI_CONST MI_Char16* data;
    MI_Uint32 size;
}
MI_ConstChar16A;

typedef struct _MI_ConstDatetimeA 
{
    MI_CONST MI_Datetime* data;
    MI_Uint32 size;
}
MI_ConstDatetimeA;

typedef struct _MI_ConstStringA 
{
    MI_CONST MI_Char* MI_CONST* data;
    MI_Uint32 size;
}
MI_ConstStringA;

typedef struct _MI_ConstReferenceA 
{
    MI_CONST MI_Instance* MI_CONST* data;
    MI_Uint32 size;
}
MI_ConstReferenceA;

typedef struct _MI_ConstInstanceA
{
    MI_CONST MI_Instance* MI_CONST* data;
    MI_Uint32 size;
}
MI_ConstInstanceA;

/*
**==============================================================================
**
** union MI_Value
**
**     This structure defines a union of all CIM data types.
**
**==============================================================================
*/

typedef union _MI_Value
{
    MI_Boolean boolean;
    MI_Uint8 uint8;
    MI_Sint8 sint8;
    MI_Uint16 uint16;
    MI_Sint16 sint16;
    MI_Uint32 uint32;
    MI_Sint32 sint32;
    MI_Uint64 uint64;
    MI_Sint64 sint64;
    MI_Real32 real32;
    MI_Real64 real64;
    MI_Char16 char16;
    MI_Datetime datetime;
    MI_Char* string;
    MI_Instance* instance;
    MI_Instance* reference;
    MI_BooleanA booleana;
    MI_Uint8A uint8a;
    MI_Sint8A sint8a;
    MI_Uint16A uint16a;
    MI_Sint16A sint16a;
    MI_Uint32A uint32a;
    MI_Sint32A sint32a;
    MI_Uint64A uint64a;
    MI_Sint64A sint64a;
    MI_Real32A real32a;
    MI_Real64A real64a;
    MI_Char16A char16a;
    MI_DatetimeA datetimea;
    MI_StringA stringa;
    MI_ReferenceA referencea;
    MI_InstanceA instancea;
    MI_Array array;
}
MI_Value;

/*
**==============================================================================
**
** MI_ValuePtr
**
**     A union that reflects the types of things that are pointed to by the
**     class, attribute and qualifier _value_ fields present in the structures
**     passed in the callbacks from the MOF compiler.
*/

typedef union _MI_ValuePtr
{
    MI_Boolean* boolean;
    MI_Uint8* uint8;
    MI_Sint8* sint8;
    MI_Uint16* uint16;
    MI_Sint16* sint16;
    MI_Uint32* uint32;
    MI_Sint32* sint32;
    MI_Uint64* uint64;
    MI_Sint64* sint64;
    MI_Real32* real32;
    MI_Real64* real64;
    MI_Char16* char16;
    MI_Datetime* datetime;
    MI_Char* string;
    MI_Instance* instance;
    MI_Instance* reference;

    MI_BooleanA* booleana;
    MI_Uint8A* uint8a;
    MI_Sint8A* sint8a;
    MI_Uint16A* uint16a;
    MI_Sint16A* sint16a;
    MI_Uint32A* uint32a;
    MI_Sint32A* sint32a;
    MI_Uint64A* uint64a;
    MI_Sint64A* sint64a;
    MI_Real32A* real32a;
    MI_Real64A* real64a;
    MI_Char16A* char16a;
    MI_DatetimeA* datetimea;
    MI_StringA* stringa;
    MI_ReferenceA* referencea;
    MI_InstanceA* instancea;
    MI_Array* array;

    void* generic;

} MI_ValuePtr;

/*
**==============================================================================
**
** struct MI_<TYPE>Field
**
**     These structures represent property or parameter fields within generated
**     structures. Each structure definition defines two fields.
**
**         value - a field of the given type.
**         exists - a flag indicating whether the field is non-null.
**
**     There are no field structure definitions for references or embedded
**     instances. These are represented by including a pointer of the
**     corresponding type directly in the generated structure.
**
**==============================================================================
*/

typedef struct _MI_BooleanField
{
    MI_Boolean value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_BooleanField;

typedef struct _MI_Sint8Field
{
    MI_Sint8 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint8Field;

typedef struct _MI_Uint8Field
{
    MI_Uint8 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint8Field;

typedef struct _MI_Sint16Field
{
    MI_Sint16 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint16Field;

typedef struct _MI_Uint16Field
{
    MI_Uint16 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint16Field;

typedef struct _MI_Sint32Field
{
    MI_Sint32 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint32Field;

typedef struct _MI_Uint32Field
{
    MI_Uint32 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint32Field;

typedef struct _MI_Sint64Field
{
    MI_Sint64 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint64Field;

typedef struct _MI_Uint64Field
{
    MI_Uint64 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint64Field;

typedef struct _MI_Real32Field
{
    MI_Real32 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Real32Field;

typedef struct _MI_Real64Field
{
    MI_Real64 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Real64Field;

typedef struct _MI_Char16Field
{
    MI_Char16 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Char16Field;

typedef struct _MI_DatetimeField
{
    MI_Datetime value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_DatetimeField;

typedef struct _MI_StringField
{
    MI_Char* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_StringField;

typedef struct _MI_ReferenceField
{
    MI_Instance* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ReferenceField;

typedef struct _MI_InstanceField
{
    MI_Instance* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_InstanceField;

typedef struct _MI_BooleanAField
{
    MI_BooleanA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_BooleanAField;

typedef struct _MI_Uint8AField
{
    MI_Uint8A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint8AField;

typedef struct _MI_Sint8AField
{
    MI_Sint8A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint8AField;

typedef struct _MI_Uint16AField
{
    MI_Uint16A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint16AField;

typedef struct _MI_Sint16AField
{
    MI_Sint16A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint16AField;

typedef struct _MI_Uint32AField
{
    MI_Uint32A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint32AField;

typedef struct _MI_Sint32AField
{
    MI_Sint32A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint32AField;

typedef struct _MI_Uint64AField
{
    MI_Uint64A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Uint64AField;

typedef struct _MI_Sint64AField
{
    MI_Sint64A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Sint64AField;

typedef struct _MI_Real32AField
{
    MI_Real32A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Real32AField;

typedef struct _MI_Real64AField
{
    MI_Real64A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Real64AField;

typedef struct _MI_Char16AField
{
    MI_Char16A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_Char16AField;

typedef struct _MI_DatetimeAField
{
    MI_DatetimeA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_DatetimeAField;

typedef struct _MI_StringAField
{
    MI_StringA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_StringAField;

typedef struct _MI_ReferenceAField
{
    MI_ReferenceA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ReferenceAField;

typedef struct _MI_InstanceAField
{
    MI_InstanceA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_InstanceAField;

typedef struct _MI_ArrayField
{
    MI_Array value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ArrayField;


/*
**==============================================================================
**
** struct MI_Const<TYPE>Field
**
**     These structures represent property or parameter fields within generated
**     structures. Each structure definition defines two fields.
**
**         value - a field of the given type.
**         exists - a flag indicating whether the field is non-null.
**
**     There are no field structure definitions for references or embedded
**     instances. These are represented by including a pointer of the
**     corresponding type directly in the generated structure.
**
**==============================================================================
*/

typedef struct _MI_ConstBooleanField
{
    MI_Boolean value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstBooleanField;

typedef struct _MI_ConstSint8Field
{
    MI_Sint8 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint8Field;

typedef struct _MI_ConstUint8Field
{
    MI_Uint8 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint8Field;

typedef struct _MI_ConstSint16Field
{
    MI_Sint16 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint16Field;

typedef struct _MI_ConstUint16Field
{
    MI_Uint16 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint16Field;

typedef struct _MI_ConstSint32Field
{
    MI_Sint32 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint32Field;

typedef struct _MI_ConstUint32Field
{
    MI_Uint32 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint32Field;

typedef struct _MI_ConstSint64Field
{
    MI_Sint64 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint64Field;

typedef struct _MI_ConstUint64Field
{
    MI_Uint64 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint64Field;

typedef struct _MI_ConstReal32Field
{
    MI_Real32 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstReal32Field;

typedef struct _MI_ConstReal64Field
{
    MI_Real64 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstReal64Field;

typedef struct _MI_ConstChar16Field
{
    MI_Char16 value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstChar16Field;

typedef struct _MI_ConstDatetimeField
{
    MI_Datetime value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstDatetimeField;

typedef struct _MI_ConstStringField
{
    MI_CONST MI_Char* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstStringField;

typedef struct _MI_ConstReferenceField
{
    MI_CONST MI_Instance* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstReferenceField;

typedef struct _MI_ConstInstanceField
{
    MI_CONST MI_Instance* value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstInstanceField;

typedef struct _MI_ConstBooleanAField
{
    MI_ConstBooleanA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstBooleanAField;

typedef struct _MI_ConstUint8AField
{
    MI_ConstUint8A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint8AField;

typedef struct _MI_ConstSint8AField
{
    MI_ConstSint8A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint8AField;

typedef struct _MI_ConstUint16AField
{
    MI_ConstUint16A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint16AField;

typedef struct _MI_ConstSint16AField
{
    MI_ConstSint16A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint16AField;

typedef struct _MI_ConstUint32AField
{
    MI_ConstUint32A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint32AField;

typedef struct _MI_ConstSint32AField
{
    MI_ConstSint32A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint32AField;

typedef struct _MI_ConstUint64AField
{
    MI_ConstUint64A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstUint64AField;

typedef struct _MI_ConstSint64AField
{
    MI_ConstSint64A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstSint64AField;

typedef struct _MI_ConstReal32AField
{
    MI_ConstReal32A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstReal32AField;

typedef struct _MI_ConstReal64AField
{
    MI_ConstReal64A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstReal64AField;

typedef struct _MI_ConstChar16AField
{
    MI_ConstChar16A value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstChar16AField;

typedef struct _MI_ConstDatetimeAField
{
    MI_ConstDatetimeA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstDatetimeAField;

typedef struct _MI_ConstStringAField
{
    MI_ConstStringA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstStringAField;

typedef struct _MI_ConstReferenceAField
{
    MI_ConstReferenceA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstReferenceAField;

typedef struct _MI_ConstInstanceAField
{
    MI_ConstInstanceA value;
    MI_Boolean exists;
    MI_Uint8 flags;
}
MI_ConstInstanceAField;

/*
**==============================================================================
**
** MI_Server
**
**==============================================================================
*/

struct _MI_ServerFT
{
    MI_Result (MI_CALL *GetVersion)(
        MI_Uint32* version);

    MI_Result (MI_CALL *GetSystemName)(
        const MI_Char** systemName);
};

/**
 * This structure defines the global server object. It defines the interface
 * for communicating with the server. It also defines default function tables
 * for all other types (Context, Instance, PropertySet, and Filter).
 *
 */
struct _MI_Server
{
    MI_ServerFT* serverFT;
    MI_ContextFT* contextFT;
    MI_InstanceFT* instanceFT;
    MI_PropertySetFT* propertySetFT;
    MI_FilterFT* filterFT;
};

/**
 * Obtains the value of the MI_VERSION macro used when compiling the server.
 * 
 * @param version contains the version number upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER
 *
 */
MI_Result MI_CALL MI_Server_GetVersion(MI_Uint32* version);

/**
 * Obtains the 'system name' for this server. The system name is used in
 * several standard CIM key properties (e.g., CIM_Fan.SystemName). The name
 * is only known by the server. The provider should never attempt to determine
 * the system name on its own. The system name is typically the hostname
 * for the system but the server may add additional qualification.
 * 
 * @param systemName points to the system name upon return (remains in scope
 *        for the lifetime of the process.
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER
 *
 */
MI_Result MI_CALL MI_Server_GetSystemName(const MI_Char** systemName);

/*
**==============================================================================
**
** MI_Filter
**
**==============================================================================
*/

/**
 * @defgroup MI_FilterFT The MI_Filter Module 
 * @{
 */

/** The MI_FilterFT function table */
struct _MI_FilterFT
{
    MI_Result (MI_CALL *Evaluate)(
        __in const MI_Filter* self, 
        __in const MI_Instance* instance,
        __out MI_Boolean* result);

    MI_Result (MI_CALL *GetExpression)(
        __in const MI_Filter* self, 
        __deref_out_z_opt const MI_Char** queryLang, 
        __deref_out_z_opt const MI_Char** queryExpr);
};

struct _MI_Filter
{
    /* Reserved for future function table */
    MI_FilterFT* ft;

    /* Reserved for future use */
    ptrdiff_t reserved[3];
};

/**
 *
 * Provider calls this function to evaluate an instance against given filter.
 * 
 * @param self pointer to the filter.
 * @param instance a pointer to the instance to evaluate.
 * @param result upon return this result indicates whether the instance 
 *        matched the filter.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_Filter_Evaluate(
        __in const MI_Filter* self, 
        __in const MI_Instance* instance,
        __out MI_Boolean* result)
{
    return self->ft->Evaluate(self, instance, result);
}

/**
 *
 * This function returns filter language and expression.
 * 
 * @param self pointer to the filter.
 * @param queryExpr the query string upon return.
 * @param queryLang the query language upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_Filter_GetExpression(
        __in const MI_Filter* self, 
        __deref_out_z_opt const MI_Char** queryLang, 
        __deref_out_z_opt const MI_Char** queryExpr)
{
    return self->ft->GetExpression(self, queryLang, queryExpr);
}

/* @} */

/*
**==============================================================================
**
** The MI_PropertySet Module
**
**==============================================================================
*/

/**
 * @defgroup MI_PropertySet The MI_PropertySet Module
 * @{
 *
 */

/** The MI_PropertySet function table. */
struct _MI_PropertySetFT
{
    MI_Result (MI_CALL *GetElementCount)(
        __in const MI_PropertySet* self, 
        __out MI_Uint32* count);

    MI_Result (MI_CALL *ContainsElement)(
        __in const MI_PropertySet* self, 
        __in_z const MI_Char* name,
        __out MI_Boolean* flag);

    MI_Result (MI_CALL *AddElement)(
        __in MI_PropertySet* self, 
        __in_z const MI_Char* name);

    MI_Result (MI_CALL *GetElement)(
        __in const MI_PropertySet* self, 
        MI_Uint32 index,
        __out_z const MI_Char** name);

    MI_Result (MI_CALL *Clear)(
        __in MI_PropertySet* self);

    MI_Result (MI_CALL *Destruct)(
        __in MI_PropertySet* self);

    MI_Result (MI_CALL *Delete)(
        __in MI_PropertySet* self);
};

/**
 *  This type implements a set of property names. It supports building of 
 *  property sets and interrogation of property sets. In general, clients
 *  build property sets and providers interrogate them.
 *
 *  Building a property list:
 *
 *  <pre>
 *      MI_Result Foo(MI_PropertySet* list)
 *      {
 *          MI_Result r;
 *          MI_Value v;
 *
 *          v.uint32 = 1;
 *          r = MI_PropertySet_AddElement(list, L"Key", &v, MI_UINT32, 0);
 *          if ((r = != MI_RESULT_OK)
 *              return r;
 *
 *          v.string = "George";
 *          r = MI_PropertySet_AddElement(list, L"First", &v, MI_STRING, 0);
 *          if ((r = != MI_RESULT_OK)
 *              return r;
 *
 *          v.string = "Washington";
 *          r = MI_PropertySet_AddElement(list, L"Last", &v, MI_STRING, 0);
 *          if ((r = != MI_RESULT_OK)
 *              return r;
 *      }
 *  <pre>
 *
 *  Checking property name membership:
 *
 *  <pre>
 *  MI_PropertySet* set;
 *  MI_Boolean found;
 *
 *  r = MI_PropertySet_ContainsElement(set, "Key", &found);
 *  </pre>
 *
 *  Printing a property list:
 *
 *  </pre>
 *      MI_Result PrintNames(const MI_PropertySet* list)
 *      {
 *          MI_Result r;
 *          MI_Uint32 n;
 *          MI_Uint32 i;
 *
 *          r = MI_PropertySet_GetElementCount(list, &n);
 *
 *          if (r != MI_RESULT_OK)
 *              return r;
 *
 *          for (i = 0; i < n; i++)
 *          {
 *              const MI_Char* name;
 *
 *              r = MI_PropertySet_GetElement(list, i, &name);
 *
 *              if (r != MI_RESULT_OK)
 *                  return r;
 *
 *              wprintf(L"%s\n", name);
 *          }
 *      }
 *  </pre>
 *
 *  Releasing a property list (constructed on the stack).
 *
 *  <pre>
 *      MI_Result r;
 *      r = MI_PropertySet_Destruct(list);
 *  </pre>
 *
 */ 
struct _MI_PropertySet
{
    /* Function table */
    MI_PropertySetFT* ft;

    /* Reserved for internal use */
    ptrdiff_t reserved[3];
};

/**
 * Gets the number of properties in the list.
 *
 * @param self the property list
 * @param count the number of properties upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PropertySet_GetElementCount(
    __in const MI_PropertySet* self, 
    __out MI_Uint32* count)
{
    return self->ft->GetElementCount(self, count);
}

/**
 * Determines whether the property list contains the given property.
 *
 * @param self the property list
 * @param name check whether this property is contained in list.
 * @param flag MI_TRUE upon return if property was found. MI_FALSE otherwise.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PropertySet_ContainsElement(
    __in const MI_PropertySet* self, 
    __in_z const MI_Char* name,
    __out MI_Boolean* flag)
{
    return self->ft->ContainsElement(self, name, flag);
}

/**
 * Adds a name to the property list.
 *
 * @param self the property list
 * @param name add this name to the property list.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED, MI_RESULT_ALREADY_EXISTS
 *
 */
MI_INLINE MI_Result MI_CALL MI_PropertySet_AddElement(
    __in MI_PropertySet* self, 
    __in_z const MI_Char* name)
{
    return self->ft->AddElement(self, name);
}

/**
 * Gets the i-th name from the list.
 *
 * @param self the property list.
 * @param index get the name with this index.
 * @param name set this to point to the name (the lifetime of this string
 *        is tied to the property list).
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PropertySet_GetElement(
    __in const MI_PropertySet* self, 
    MI_Uint32 index,
    __out_z const MI_Char** name)
{
    return self->ft->GetElement(self, index, name);
}

/**
 * Remove all names from the property list. Afterwards, the count is zero
 * This allows property lists to be reused (without having to be destructed 
 * and reconstructed).
 *
 * @param self the property list
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PropertySet_Clear(
    __in MI_PropertySet* self)
{
    return self->ft->Clear(self);
}

/**
 * Destructs the property list (releasing memory resources). The property list
 * must have been constructed on the stack and not on the heap.
 *
 * @param self the property list
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PropertySet_Destruct(
    __in MI_PropertySet* self)
{
    return self->ft->Destruct(self);
}

/**
 * Deletes the property list (releasing memory resources). The property list
 * must have been constructed on the heap (not the stack).
 *
 * @param self the property list
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PropertySet_Delete(
    __in MI_PropertySet* self)
{
    return self->ft->Delete(self);
}

/* @} */

/*
**==============================================================================
**
** struct MI_ObjectDecl
**
**     A base type for MI_ClassDecl and MI_PropertyDecl, which allows functions
**     to be written that work on the common fields of these two types.
**
**==============================================================================
*/

typedef struct _MI_ObjectDecl /* extends MI_FeatureDecl */
{
    /* Fields inherited from MI_FeatureDecl */
    MI_Uint32 flags;
    MI_Uint32 code;
    MI_CONST MI_Char* name;
    MI_Qualifier MI_CONST* MI_CONST* qualifiers;
    MI_Uint32 numQualifiers;

    /* The properties or parameters of this object. Note that for methods
     * the type will be MI_ParameterDecl rather than MI_PropertyDecl.
     */
    struct _MI_PropertyDecl MI_CONST* MI_CONST* properties;
    MI_Uint32 numProperties;

    /* Size of structure described by MI_MethodDecl or MI_ClassDecl */
    MI_Uint32 size;
}
MI_ObjectDecl;

/*
**==============================================================================
**
** struct MI_ClassDecl
**
**     Represents a CIM class.
**
**     Flags:
**         MI_FLAG_CLASS
**         MI_FLAG_ASSOCIATION
**         MI_FLAG_INDICATION
**         MI_FLAG_ABSTRACT
**         MI_FLAG_TERMINAL
**
**==============================================================================
*/

struct _MI_ClassDecl /* extends MI_ObjectDecl */
{
    /* Fields inherited from MI_FeatureDecl */
    MI_Uint32 flags;
    MI_Uint32 code;
    MI_CONST MI_Char* name;
    struct _MI_Qualifier MI_CONST* MI_CONST* qualifiers;
    MI_Uint32 numQualifiers;

    /* Fields inherited from MI_ObjectDecl */
    struct _MI_PropertyDecl MI_CONST* MI_CONST* properties;
    MI_Uint32 numProperties;
    MI_Uint32 size;

    /* Name of superclass */
    MI_CONST MI_Char* superClass;

    /* Superclass declaration */
    MI_ClassDecl MI_CONST* superClassDecl;

    /* The methods of this class */
    struct _MI_MethodDecl MI_CONST* MI_CONST* methods;
    MI_Uint32 numMethods;

    /* Pointer to scema this class belongs to */
    struct _MI_SchemaDecl MI_CONST* schema;

    /* Provider functions */
    MI_CONST MI_ProviderFT* providerFT;
};

/*
**==============================================================================
**
** struct MI_InstanceDecl
**
**     Represents an instance declaration (as encountered in MOF).
**
**==============================================================================
*/

typedef struct _MI_InstanceDecl MI_InstanceDecl;

struct _MI_InstanceDecl /* extends MI_ObjectDecl */
{
    /* Fields inherited from MI_FeatureDecl */
    MI_Uint32 flags;
    MI_Uint32 code;
    MI_CONST MI_Char* name; /* name of class of which this is an instance */
    struct _MI_Qualifier MI_CONST* MI_CONST* qualifiers; /* unused */
    MI_Uint32 numQualifiers; /* unused */

    /* Fields inherited from MI_ObjectDecl */
    struct _MI_PropertyDecl MI_CONST* MI_CONST* properties;
    MI_Uint32 numProperties;
    MI_Uint32 size;
};

/*
**==============================================================================
**
** struct MI_FeatureDecl
**
**     This structure functions as a base type for these structures:
**         MI_PropertyDecl 
**         MI_ParameterDecl
**         MI_MethodDecl
**
**==============================================================================
*/

typedef struct _MI_FeatureDecl
{
    /* Flags */
    MI_Uint32 flags;

    /* Hash code: (name[0] << 16) | (name[len-1] << 8) | len */
    MI_Uint32 code;

    /* Name of this feature */
    MI_CONST MI_Char* name;

    /* Qualifiers */
    MI_Qualifier MI_CONST* MI_CONST *  qualifiers;
    MI_Uint32 numQualifiers;
}
MI_FeatureDecl;

/*
**==============================================================================
**
** struct MI_ParameterDecl
**
**     Represents a CIM property (or reference)
**
**     Flags:
**         MI_FLAG_PROPERTY
**         MI_FLAG_KEY
**
**==============================================================================
*/

typedef struct _MI_ParameterDecl /* extends MI_FeatureDecl */
{
    /* Fields inherited from MI_FeatureDecl */
    MI_Uint32 flags;
    MI_Uint32 code;
    MI_CONST MI_Char* name;
    MI_Qualifier MI_CONST* MI_CONST* qualifiers;
    MI_Uint32 numQualifiers;

    /* Type of this field */
    MI_Uint32 type;

    /* Name of reference class */
    MI_CONST MI_Char* className;

    /* Array subscript */
    MI_Uint32 subscript;

    /* Offset of this field within the structure */
    MI_Uint32 offset;
}
MI_ParameterDecl;

/*
**==============================================================================
**
** struct MI_PropertyDecl
**
**     Represents a CIM property (or reference)
**
**     Flags:
**         MI_FLAG_PROPERTY
**         MI_FLAG_KEY
**
**==============================================================================
*/

typedef struct _MI_PropertyDecl /* extends MI_ParameterDecl */
{
    /* Fields inherited from MI_FeatureDecl */
    MI_Uint32 flags;
    MI_Uint32 code;
    MI_CONST MI_Char* name;
    MI_Qualifier MI_CONST* MI_CONST* qualifiers;
    MI_Uint32 numQualifiers;

    /* Fields inherited from MI_ParameterDecl */
    MI_Uint32 type;
    MI_CONST MI_Char* className;
    MI_Uint32 subscript;
    MI_Uint32 offset;

    /* Ancestor class that first defined a property with this name */
    MI_CONST MI_Char* origin;

    /* Ancestor class that last defined a property with this name */
    MI_CONST MI_Char* propagator;

    /* Value of this property */
    MI_CONST  void* value;
}
MI_PropertyDecl;

/*
**==============================================================================
**
** struct MI_MethodDecl
**
**     Represents a CIM method.
**
**     Flags:
**         MI_FLAG_METHOD
**         MI_FLAG_STATIC
**
**==============================================================================
*/

typedef void (MI_CALL *MI_MethodDecl_Invoke)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in_z const MI_Char* methodName,
    __in const MI_Instance* instanceName,
    __in const MI_Instance* parameters);

typedef struct _MI_MethodDecl /* extends MI_ObjectDecl */
{
    /* Fields inherited from MI_FeatureDecl */
    MI_Uint32 flags;
    MI_Uint32 code;
    MI_CONST MI_Char* name;
    struct _MI_Qualifier MI_CONST* MI_CONST* qualifiers;
    MI_Uint32 numQualifiers;

    /* Fields inherited from MI_ObjectDecl */
    struct _MI_ParameterDecl MI_CONST* MI_CONST* parameters;
    MI_Uint32 numParameters;
    MI_Uint32 size;

    /* PostResult type of this method */
    MI_Uint32 returnType;

    /* Ancestor class that first defined a property with this name */
    MI_CONST MI_Char* origin;

    /* Ancestor class that last defined a property with this name */
    MI_CONST MI_Char* propagator;

    /* Pointer to scema this class belongs to */
    struct _MI_SchemaDecl MI_CONST* schema;

    /* Pointer to extrinsic method */
    MI_MethodDecl_Invoke function;
}
MI_MethodDecl;

/*
**==============================================================================
**
** struct MI_QualifierDecl
**
**     Represents a CIM qualifier declaration.
**
**==============================================================================
*/

typedef struct _MI_QualifierDecl
{
    /* Name of this qualifier */
    MI_CONST MI_Char* name;

    /* Type of this qualifier */
    MI_Uint32 type;

    /* Qualifier scope */
    MI_Uint32 scope;

    /* Qualifier flavor */
    MI_Uint32 flavor;

    /* Array subscript (for arrays only) */
    MI_Uint32 subscript;

    /* Pointer to value */
    MI_CONST void* value;
}
MI_QualifierDecl;

/*
**==============================================================================
**
** struct MI_Qualifier
**
**     Represents a CIM qualifier.
**
**==============================================================================
*/

struct _MI_Qualifier
{
    /* Qualifier name */
    MI_CONST MI_Char* name;

    /* Qualifier type */
    MI_Uint32 type;

    /* Qualifier flavor */
    MI_Uint32 flavor;

    /* Pointer to value */
    MI_CONST void* value;
};

/*
**==============================================================================
**
** struct MI_SchemaDecl
**
**     This structure represents the schema objects in a CIM schemas, which
**     include CIM classes and CIM qualifier declarations.
**
**==============================================================================
*/

typedef struct _MI_SchemaDecl
{
    /* Qualifier declarations */
    MI_QualifierDecl MI_CONST* MI_CONST* qualifierDecls;
    MI_Uint32 numQualifierDecls;

    /* Class declarations */
    MI_ClassDecl MI_CONST* MI_CONST* classDecls;
    MI_Uint32 numClassDecls;
}
MI_SchemaDecl;

/*
**==============================================================================
**
** The MI_ProviderFT Module
**
**==============================================================================
*/

/**
 * @defgroup MI_Provider The MI_Provider Module
 * @{
 *
 */

/* The developer may optionally define this structure in module.c */
typedef struct _MI_Module MI_Module_Self;

/**
 * The server invokes this function to initialize the provider, which
 * performs initialization activities. The provider may set the 'self' 
 * parameter to refer to any provider state data (or null if no state data 
 * is required). Whatever value the provider sets for 'self' is passed into 
 * other calls to the provider.
 *
 * @param self the provider may set this to refer to any provider state data 
 *     (or NULL if no state data is required).
 * @param selfModule the 'self' parameter obtained when loading the module.
 * @param context the current request context
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
typedef void (MI_CALL *MI_ProviderFT_Load)(
    __deref_out void** self,
    __in_opt MI_Module_Self* selfModule,
    __in MI_Context* context);

/**
 * The server invokes this function to release any resources held by the 
 * provider. The provider should close any file handles and release any 
 * memory associated with the execution of the provider.
 *
 * The implementation should pass \b MI_RESULT_OK or \b MI_RESULT_FAILED to
 * MI_PostResult.
 * 
 * @param self the provider state data.
 * @param context the request context.
 *
 * @b Post: nothing
 *
 * @return MI_RESULT_OK, MI_RESULT_DO_NOT_UNLOAD, MI_RESULT_FAILED
 *
 */
typedef void (MI_CALL *MI_ProviderFT_Unload)(
    __in_opt void* self,
    __in MI_Context* context);

/**
 *  The server invokes the GetInstance function obtain a single CIM 
 *  instance from the provider. The 'instanceName' property defines the
 *  name of the instance to be retrieved.
 *
 *  If the 'propertySet' parameter is not null, the elements of the set define
 *  zero or more property names. The returned instance shall not include 
 *  elements for properties missing from this set. If the 'propertySet' input 
 *  parameter is an empty set, no properties are included in the response. If 
 *  the 'propertySet' input parameter is null, no properties shall be filtered.
 *
 *  If the provider returns MI_RESULT_NOT_SUPPORTED (via MI_PostResult), the
 *  server attempts to satisfy the request by calling the provider's 
 *  'EnumerateInstances' method. Do not rely on this behavior unless the
 *  number of instances is reasonably small.
 *
 *  If GetInstance is successful, the provider should pass the new instance
 *  to MI_PostInstance(). If GetInstance is unsuccessful, the provider should
 *  return one of the results listed below (via MI_PostResult).
 *
 *  @param self the provider state data
 *  @param context the request context
 *  @param nameSpace the namespace of the request.
 *  @param className the name of the class.
 *  @param instanceName name of the requested instance
 *  @param propertySet list of required properties or NULL for all.
 *
 *  @b Post: resulting instance.
 *
 *  @return
 *      MI_RESULT_OK
 *      MI_RESULT_ACCESS_DENIED
 *      MI_RESULT_INVALID_NAMESPACE
 *      MI_RESULT_INVALID_PARAMETER
 *      MI_RESULT_INVALID_CLASS
 *      MI_RESULT_NOT_FOUND
 *      MI_RESULT_FAILED
 *
 *  @b Example:
 *
 *      <pre>
 *      void MI_CALL President_GetInstance(
 *          President_Self* self,
 *          MI_Context* context,
 *          const MI_Char* nameSpace,
 *          const MI_Char* className,
 *          const President* instanceName,
 *          const MI_PropertySet* propertySet)
 *      {
 *
 *          if (instanceName->Key.value == 1)
 *          {
 *              President inst;
 *              President_Construct(&inst, context);
 *              President_Set_Key(&inst, 1);
 *              President_Set_First(&inst, MI_T("George"));
 *              President_Set_Last(&inst, MI_T("Washington"));
 *              President_Post(&inst, context);
 *              MI_PostResult(context, MI_RESULT_OK);
 *              President_Destruct(&inst);
 *              return;
 *          }
 *      
 *          if (instanceName->Key.value == 2)
 *          {
 *              President inst;
 *              President_Construct(&inst, context);
 *              President_Set_Key(&inst, 2);
 *              President_Set_First(&inst, MI_T("John"));
 *              President_Set_Last(&inst, MI_T("Adams"));
 *              President_Post(&inst, context);
 *              President_Destruct(&inst);
 *              MI_PostResult(context, MI_RESULT_OK);
 *              return;
 *          }
 *      
 *          MI_PostResult(context, MI_RESULT_NOT_FOUND);
 *      }
 *      </pre>
 */
typedef void (MI_CALL *MI_ProviderFT_GetInstance)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in const MI_Instance* instanceName,
    __in_opt const MI_PropertySet* propertySet);

/**
 *  The server calls EnumerateInstances to enumerate instances of a CIM class 
 *  in the target namespace. Note that the enumeration is not polymoprhic; the
 *  implementaiton should provide instances of the exact class given by the 
 *  'className' input parameter, and should not include instances of any
 *  derived classes.
 *
 *  The 'className' input parameter defines the exact class to be enumerated.
 *
 *  If the 'propertySet' parameter is not null, the elements of the set define
 *  zero or more property names. The returned instances shall not include 
 *  elements for properties missing from this set. If the 'propertySet' input 
 *  parameter is an empty set, no properties are included in the response. If 
 *  the 'propertySet' input parameter is null, no properties shall be filtered.
 *
 *  If the 'keysOnly' input parameter is true, then the implementaiton should
 *  provide only key properties.
 *
 *  If not null, the 'filter' input parameter defines a query filter that all 
 *  provided instances must match. If the MI_Module.flags field contains 
 *  MI_MODULE_FLAG_FILTER_SUPPORT (set by the MI_Main() entry point), this
 *  filter may be non-null. Otherwise, the 'filter' input paramerter is null.
 *
 *  If EnumerateInstances is successful, the method returns zero or more 
 *  instances.
 *
 *  @param self the provider state data.
 *  @param context the request context.
 *  @param nameSpace enumerate instances of this namespace.
 *  @param className enumerate instances of this class.
 *  @param propertySet list of required properties or NULL for all.
 *  @param keysOnly true if only key properties are required
 *  @param filter Used to filter instances.
 *
 *  @b Post: zero or more instances.
 *
 *  @return 
 *      MI_RESULT_OK
 *      MI_RESULT_ACCESS_DENIED
 *      MI_RESULT_INVALID_NAMESPACE
 *      MI_RESULT_INVALID_PARAMETER
 *      MI_RESULT_INVALID_CLASS
 *      MI_RESULT_NOT_SUPPORTED
 *      MI_RESULT_FAILED
 *
 *  @b Example:
 *
 *      <pre>
 *      void MI_CALL President_EnumerateInstances(
 *          President_Self* self,
 *          MI_Context* context,
 *          const MI_Char* nameSpace,
 *          const MI_Char* className,
 *          const MI_PropertySet* propertySet,
 *          MI_Boolean keysOnly,
 *          const MI_Filter* filter)
 *      {
 *          President inst;
 *      
 *          President_Construct(&inst, context);
 *          President_Set_Key(&inst, 1);
 *          President_Set_First(&inst, MI_T("George"));
 *          President_Set_Last(&inst, MI_T("Washington"));
 *          President_Post(&inst, context);
 *          President_Destruct(&inst);
 *      
 *          President_Construct(&inst, context);
 *          President_Set_Key(&inst, 2);
 *          President_Set_First(&inst, MI_T("John"));
 *          President_Set_Last(&inst, MI_T("Adams"));
 *          President_Post(&inst, context);
 *          President_Destruct(&inst);
 *      
 *          MI_PostResult(context, MI_RESULT_OK);
 *      }
 *      </pre>
 */
typedef void (MI_CALL *MI_ProviderFT_EnumerateInstances)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in_opt const MI_PropertySet* propertySet,
    MI_Boolean keysOnly,
    __in_opt const MI_Filter* filter);

/**
 *  The server calls the CreateInstance function to create a single CIM 
 *  instance in the target namespace.
 *
 *  The 'newInstance' input parameter defines the properties of the new 
 *  instance. The null properties of this instance are ignored and are not 
 *  part of the new instance.
 *
 *  The 'newInstance' input parameter may define some but not all of the key
 *  properties (leaving some keys null). If so, the implementation must 
 *  allocate values for the undefined keys. This occurs with keys that the
 *  requestor cannot define, since their values are only known by the server.
 *  Typical examples include 'SystemName' and 'SystemCreationClassName'.
 *
 *  If CreateInstance is successful, the implementation should post the
 *  instance name of the new instance and then post MI_RESULT_OK.
 *
 *  If CreateInstance is successful, the implementation should post the
 *  result error code in the return section.
 *
 *  If an instances with the same keys already exists, the implementation
 *  should post MI_RESULT_ALREADY_EXISTS.
 *
 *  @param self the provider state data.
 *  @param context the request context.
 *  @param nameSpace enumerate instances of this namespace.
 *  @param className enumerate instances of this class.
 *  @param newInstance the instance that will be created.
 *
 *  @b Post: a single instance.
 *
 *  @return
 *      MI_RESULT_OK
 *      MI_RESULT_ACCESS_DENIED
 *      MI_RESULT_NOT_SUPPORTED
 *      MI_RESULT_INVALID_NAMESPACE
 *      MI_RESULT_INVALID_PARAMETER
 *      MI_RESULT_INVALID_CLASS
 *      MI_RESULT_ALREADY_EXISTS
 *      MI_RESULT_FAILED
 *
 *  @b Example:
 *  
 *      <pre>
 *      void MI_CALL President_CreateInstance(
 *          President_Self* self,
 *          MI_Context* context,
 *          const MI_Char* nameSpace,
 *          const MI_Char* className,
 *          const President* newInstance)
 *      {
 *          President instanceName;
 *      
 *          if (!newInstance->Key.exists)
 *              MI_PostResult(context, MI_RESULT_INVALID_PARAMETER);
 *      
 *          \.
 *          \.
 *          \.
 *      
 *          President_Construct(\&instanceName, context);
 *          President_Set_Key(\&instanceName, newInstance->Key.value);
 *          President_Post(\&instanceName, context);
 *          President_Destruct(\&instanceName);
 *      
 *          MI_PostResult(context, MI_RESULT_OK);
 *      }
 *      </pre>
 */
typedef void (MI_CALL *MI_ProviderFT_CreateInstance)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in const MI_Instance* newInstance);

/**
 *  The server calls the ModifyInstance function to modify an existing CIM 
 *  instance in the target namespace. The instance must already exist.
 *
 *  The 'modifiedInstance' input parameter identifies the instance that shall
 *  be modified (through its key properties) and provides new property values 
 *  for it.
 *
 *  The set of properties that are modified are determined as follows:
 *
 *  If the propertySet input parameter is not null, the elements of the set
 *  define zero or more property names. Only properties specified in this set
 *  are modified. Properties of the modifiedInstance that are missing from the
 *  set shall be ingored. If the set is empty, no properties are modified. If 
 *  propertySet is null, the set of properties to be modified consists of those
 *  of modifiedInstance that are not null and whose values are different from 
 *  the current values of the instance to be modified.
 *
 *  If propertySet contains invalid property names, the implementation shall
 *  reject the request. If a property cannot be modified because, it is a key,
 *  it is non-writable, or for any other reason, the implementation shall 
 *  reject the request.
 *
 *  If ModifyInstance is successful, all properties to modified are updated
 *  in the specified instance.
 *
 *  If ModifyInstance is unsuccessful, no change is made to the specified
 *  instance and an error is returned.
 *
 *  @param self the provider state data
 *  @param context the request context
 *  @param nameSpace enumerate instances of this namespace.
 *  @param className enumerate instances of this class.
 *  @param modifiedInstance contains the new property values for the instance
 *  @param propertySet specifies which properties to modify or NULL for all.
 *
 *  @return 
 *      MI_RESULT_OK
 *      MI_RESULT_ACCESS_DENIED
 *      MI_RESULT_INVALID_NAMESPACE
 *      MI_RESULT_INVALID_CLASS
 *      MI_RESULT_INVALID_PARAMETER
 *      MI_RESULT_NOT_SUPPORTED
 *      MI_RESULT_NOT_FOUND
 *      MI_RESULT_FAILED
 *
 *  @b Example:
 *
 *      <pre>
 *      void MI_CALL President_ModifyInstance(
 *          President_Self* self,
 *          MI_Context* context,
 *          const MI_Char* nameSpace,
 *          const MI_Char* className,
 *          const President* modifiedInstance,
 *          const MI_PropertySet* propertySet)
 *      {
 *          \.
 *          \.
 *          \.
 *          MI_PostResult(context, MI_RESULT_OK);
 *      }
 *      </pre>
 *
 */
typedef void (MI_CALL *MI_ProviderFT_ModifyInstance)(
    void* self,
    MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    const MI_Instance* modifiedInstance,
    const MI_PropertySet* propertySet);

/**
 *  The server calls the DeleteInstance function to delete a single CIM 
 *  instance from the target namespace.
 *
 *  The instanceName input parameter defines the name (keys) of the instance
 *  to be deleted.
 *
 *  Deleting an instance may cause the automatic deletion of other instances,
 *  such as associations that refer to that instance.
 *
 *  If DeleteInstance is successful, the implementation removes the specified
 *  instance.
 *
 *  If DeleteInstance is unsuccessful, the implementation should return the
 *  appropriate result code.
 *
 *  @param self the provider state data
 *  @param context the request context
 *  @param nameSpace enumerate instances of this namespace.
 *  @param className enumerate instances of this class.
 *  @param instanceName the name of the instance to be deleted
 *
 * @b Post: nothing
 *
 * @return
 *      MI_RESULT_OK
 *      MI_RESULT_ACCESS_DENIED
 *      MI_RESULT_INVALID_NAMESPACE
 *      MI_RESULT_INVALID_CLASS
 *      MI_RESULT_INVALID_PARAMETER
 *      MI_RESULT_NOT_SUPPORTED
 *      MI_RESULT_NOT_FOUND
 *      MI_RESULT_FAILED
 *
 *  @b Example:
 *
 *      <pre>
 *      void MI_CALL President_DeleteInstance(
 *          President_Self* self,
 *          MI_Context* context,
 *          const MI_Char* nameSpace,
 *          const MI_Char* className,
 *          const President* instanceName)
 *      {
 *          \.
 *          \.
 *          \.
 *          MI_PostResult(context, MI_RESULT_NOT_SUPPORTED);
 *      }
 *      </pre>
 */
typedef void (MI_CALL *MI_ProviderFT_DeleteInstance)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in const MI_Instance* instanceName);

/**
 *  The server calls the AssociatorInstances function to find all CIM instances
 *  associated with a particular 'source' CIM instance.
 *
 *  The instanceName input parameter defines the source CIM instance, whose
 *  associated instances shall be returned.
 *
 *  The className input parameter, if not null, is the name of an association
 *  class. It filters the returned set of instances by requiring that each 
 *  returned instance is associated to the source instance through an instance 
 *  of this class or one of its subclasses.
 *
 *  The resultClass input parameter, if not null, is the name of a class.
 *  It filters the returned set of instances by requiring that each returned 
 *  instance is either this class or one of its subclasses. Note that the
 *  resultClass shall not refer to an association class.
 *
 *  The role input parameter, if not null, is a valid property name. It filters
 *  the returned set of instances by requiring that each returned instance be
 *  associated with the source instance through an association that contains
 *  a reference property with this name that refers to the source instance.
 *
 *  The resultRole input parameter, if not null, is a valid property name. It
 *  filters the returned set of instances by requiring that each returned
 *  instance shall be associated to the source instance through an association
 *  that contains a reference property with this name that refers to the
 *  returned instance.
 *
 *  If the propertySet input parameter is not null, the elements of the set
 *  define zero or more property names. Each returned instance shall include
 *  only properties in that set. If propertySet is empty, no properties are
 *  included in each returned instance. If propertySet is null, no additional
 *  filtering is performed.
 *
 *  If the propertySet input parameter contains invalid properties, the
 *  implementation shall reject the request.
 *
 *  If the resultClass input parameter is null, the propertySet shall be null
 *  as well (otherwise the class to which the property names refer, would be
 *  unknown).
 *
 *  If keysOnly is true, only key properties are included in the result
 *  instances.
 *
 *  If AssociatorInstances returns MI_RESULT_NOT_SUPPORTED, the server
 *  attempts to satisfy the request by calling EnumerateInstances. Unless
 *  the number of associators is very small, the AssociatorInstances operation
 *  shall be implemented.
 *
 *  If AssociatorInstances is successful, it returns zero or more CIM instances.
 *  Note that these instances may reside in a different namespace than the
 *  source instance (given by instanceName). The implementation must ensure that
 *  the namespace of the MI_Instance is set correctly.
 *
 *  If AssociatorInstances is unsuccessful, it returns the appropriate result
 *  code.
 *
 *  @param self the provider state data
 *  @param context the request context
 *  @param nameSpace the target namespace
 *  @param className the name of the association class (or NULL)
 *  @param instanceName the source class for the association.
 *  @param resultClass the name of the result class (or NULL)
 *  @param role the property name referring to the source instance.
 *  @param resultRole the property name referring to the result instances.
 *  @param propertySet names of properties to include or NULL for all.
 *  @param keysOnly true if only key properties are requested.
 *  @param filter used to filter the result instances, which could be of
 *        different types.
 *
 *  @b Post: zero or more instances
 *
 *  @return
 *      MI_RESULT_OK
 *      MI_RESULT_ACCESS_DENIED
 *      MI_RESULT_INVALID_NAMESPACE
 *      MI_RESULT_INVALID_PARAMETER
 *      MI_RESULT_NOT_SUPPORTED
 *      MI_RESULT_FAILED
 */
typedef void (MI_CALL *MI_ProviderFT_AssociatorInstances)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in const MI_Instance* instanceName,
    __in_z_opt const MI_Char* resultClass,
    __in_z_opt const MI_Char* role,
    __in_z_opt const MI_Char* resultRole,
    __in_opt const MI_PropertySet* propertySet,
    MI_Boolean keysOnly,
    __in_opt const MI_Filter* filter);

/**
 *  The server calls the ReferenceInstances function to enumerate association 
 *  instances that refer to a particular CIM instance.
 *
 *  The instanceName input parameter defines the target instance whose
 *  referring instances shall be returned.
 *
 *  The resultClass input parameter, if not null, is a CIM class name. It
 *  filters the returned set of association instances by requiring that each
 *  returned instance shall be an instance of this class or one of its 
 *  subclasses.
 *
 *  The role input parameter, if not null, is a CIM property name. It filters
 *  the returned set of association instances by requiring that each returned
 *  instance refers to the target instance through a property with this name.
 *
 *  If the propertySet input parameter is not null, the elements of the set
 *  define zero or more property names. Each returned instance shall include
 *  only properties in that set. If propertySet is empty, no properties are
 *  included in each returned instance. If propertySet is null, no additional
 *  filtering is performed.
 *
 *  If the propertySet input parameter contains invalid properties, the
 *  implementation shall reject the request.
 *
 *  If the className input parameter is null, the propertySet shall be null
 *  as well (otherwise the class to which the property names refer, would be
 *  unknown).
 *
 *  If keysOnly is true, only key properties are included in the result
 *  instances.
 *
 *  If ReferenceInstances returns MI_RESULT_NOT_SUPPORTED, the server
 *  attempts to satisfy the request by calling EnumerateInstances. Unless
 *  the number of associators is very small, the ReferenceInstances operation
 *  shall be implemented.
 *
 *  If ReferenceInstances is successful, the implementation returns zero
 *  or more instances.
 *
 *  If ReferenceInstances is unsuccessful, the implementation returns the
 *  appropriate error result.
 *
 *  @param self the provider state data.
 *  @param context the request context.
 *  @param nameSpace the target namespace.
 *  @param className the name of the result class.
 *  @param instanceName find references of the instance with this name.
 *  @param role the association property name that refers to instanceName.
 *  @param propertySet get these properties or all if NULL.
 *  @param keysOnly get only key properties.
 *  @param filter use to filter instances.
 *
 *  @b Post: zero or more reference instances.
 *
 *  @return
 *      MI_RESULT_OK
 *      MI_RESULT_ACCESS_DENIED
 *      MI_RESULT_INVALID_NAMESPACE
 *      MI_RESULT_INVALID_PARAMETER
 *      MI_RESULT_NOT_SUPPORTED
 *      MI_RESULT_FAILED
 */
typedef void (MI_CALL *MI_ProviderFT_ReferenceInstances)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in const MI_Instance* instanceName,
    __in_z_opt const MI_Char* role,
    __in_opt const MI_PropertySet* propertySet,
    MI_Boolean keysOnly,
    __in_opt const MI_Filter* filter);

/**
 * The server calls this function to enable indications delivery 
 * from the provider. Provider must store context and use it later
 * for posting indications whenever it has new event.
 * Simple implementation of providers may ignore Subscribe and
 * Unsubscribe calls and always post new indications.
 * Advanced providers may analyze filters in subscribe to perform
 * fine filtering of the indications (mostly for performance reasons).
 * Note: that's the only function where provider does not call 
 * PostResult and stores context until DisableIndications call.
 *
 * @param self the provider state data.
 * @param indicationsContext the context for indications delivery
 *
 * @b Post: zero or more indication instances.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
typedef void (MI_CALL *MI_ProviderFT_EnableIndications)(
    __in_opt void* self,
    __in MI_Context* indicationsContext,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className);

/**
 * The server calls this function to disable indications delivery 
 * from the provider. Provider must stop emitting indications and 
 * confirm operations by PostResult(OK) on given context.
 * Server provides the same context pointer as it did in 
 * corresponding EnableIndication call before.
 *
 * @param self the provider state data.
 * @param indicationsContext the context for indications delivery
 *
 * @b Post: nothing
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED.
 *
 */
typedef void (MI_CALL *MI_ProviderFT_DisableIndications)(
    __in_opt void* self,
    __in MI_Context* indicationsContext,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className);

/**
 * The server invokes this function to subscribe to indications. The
 * provider may highjack the calling thread (not recommended) or create a new 
 * thread in order to process indications. As events occur, the provider
 * should create indication instances and pass them to MI_PostInstance(),
 * with context provided by EnableIndications call. See EnableIndications for 
 * details.
 *
 * Subscribe is called between calls to EnableIndications and DisableIndications.
 *
 * @param self the provider state data.
 * @param context the request context used only for request confirmation.
 * @param filter used to filter indications.
 * @param subscriptionID unique id of the subscription.
 *
 * @b Post: nothing
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED, MI_RESULT_ACCESS_DENIED
 *
 */
typedef void (MI_CALL *MI_ProviderFT_Subscribe)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in_opt const MI_Filter* filter,
    __in_z const MI_Char* bookmark,
    MI_Uint64  subscriptionID,
    __deref_out_opt void** subscriptionSelf);

/**
 * The server invokes this function to unsubscribe from indications. 
 * The provider can match subscribe/unsubscribe calls by subscriptionID.
 *
 * Unsubscribe is called between calls to EnableIndications and 
 * DisableIndications.
 *
 * @param self the provider state data.
 * @param context the request context
 * @param subscriptionID unique id of the subscription.
 *
 * @b Post:
 *     MI_PostInstance()
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
typedef void (MI_CALL *MI_ProviderFT_Unsubscribe)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    MI_Uint64  subscriptionID,
    __in_opt void* subscriptionSelf);

/**
 * The server calls this function to carry out a CIM extrinsic method 
 * invocation on behalf of a requestor. The provider receives input parameters,
 * carries out the invoke request, and posts output parameters. 
 * 
 * For static methods, the 'instanceName' parameter is null. For non-static 
 * methods, 'instanceName' defines a target instance (through its keys).
 *
 * Note: the implementation must set the 'MIReturn' output parameter.
 *
 * @param self the provider state data.
 * @param context the request context
 * @param nameSpace the namespace of the request.
 * @param className the name of the class.
 * @param methodName the name of the method.
 * @param instanceName the name of the target instance (null if static method).
 * @param inputParameters the input parameters for the method invocation.
 *
 * @b Post: the output parameters.
 *
 * @return 
 *     MI_RESULT_OK
 *     MI_RESULT_ACCESS_DENIED 
 *     MI_RESULT_NOT_SUPPORTED,
 *     MI_RESULT_INVALID_NAMESPACE
 *     MI_RESULT_NOT_FOUND
 *     MI_RESULT_METHOD_NOT_FOUND
 *     MI_RESULT_METHOD_NOT_AVAILABLE
 *     MI_RESULT_FAILED
 *
 * @b Example:
 *
 * Consider the following MOF definition.
 *
 * <pre>
 *     class Widget
 *     {
 *         [Static] Uint32 Add(
 *             [In] Real32 X, 
 *             [In] Real32 Y, 
 *             [In(false), Out] Real32 Z);
 *     };
 * </pre>
 *
 * The Widget.Add() function adds X and Y and leaves the result in Z. The 
 * provider generator produces the following provider stub.
 * 
 * <pre>
 *     void MI_CALL CIM_Thing_Invoke_Add(
 *         CIM_Thing_Self* self,
 *         MI_Context* context,
 *         const MI_Char* nameSpace,
 *         const MI_Char* className,
 *         const MI_Char* methodName,
 *         const CIM_Thing* instanceName,
 *         const CIM_Thing_Add* in)
 *     {
 *         MI_PostResult(context, MI_RESULT_NOT_SUPPORTED);
 *     }
 * </pre>
 * 
 * The following implements the methods behavior (as described above).
 *
 * <pre>
 *     void MI_CALL CIM_Thing_Invoke_Add(
 *         CIM_Thing_Self* self,
 *         MI_Context* context,
 *         const MI_Char* nameSpace,
 *         const MI_Char* className,
 *         const MI_Char* methodName,
 *         const CIM_Thing* instanceName,
 *         const CIM_Thing_Add* in)
 *     {
 *         MI_Thing_Add out;
 * 
 *         if (!in->X.exists || !in->X.exists)
 *             return MI_RESULT_INVALID_PARAMETER;
 *
 *         CIM_Thing_Add_Construct(&out, context);
 *         CIM_Thing_Add_Set_Z(&out, in->X.value + in->X.value);
 *         CIM_Thing_Add_Set_MIReturn(&out, 0);
 *         MI_PostInstance(context, &out);
 *         CIM_Thing_Add_Destruct(&out);
 *         MI_PostResult(context, MI_RESULT_OK);
 *     }
 * </pre>
 */
typedef void (MI_CALL *MI_ProviderFT_Invoke)(
    __in_opt void* self,
    __in MI_Context* context,
    __in_z const MI_Char* nameSpace,
    __in_z const MI_Char* className,
    __in_z const MI_Char* methodName,
    __in const MI_Instance* instanceName,
    __in const MI_Instance* inputParameters);

/** Defines the function table for providers. */
struct _MI_ProviderFT
{
    MI_ProviderFT_Load Load;
    MI_ProviderFT_Unload Unload;
    MI_ProviderFT_GetInstance GetInstance;
    MI_ProviderFT_EnumerateInstances EnumerateInstances;
    MI_ProviderFT_CreateInstance CreateInstance;
    MI_ProviderFT_ModifyInstance ModifyInstance;
    MI_ProviderFT_DeleteInstance DeleteInstance;
    MI_ProviderFT_AssociatorInstances AssociatorInstances;
    MI_ProviderFT_ReferenceInstances ReferenceInstances;
    MI_ProviderFT_EnableIndications EnableIndications;
    MI_ProviderFT_DisableIndications DisableIndications;
    MI_ProviderFT_Subscribe Subscribe;
    MI_ProviderFT_Unsubscribe Unsubscribe;
    MI_ProviderFT_Invoke Invoke;
};

/** @} */

/*
**==============================================================================
**
** The MI_Module Module
**
**==============================================================================
*/

/**
 * @defgroup MI_Module The MI_Module Module
 * @{
 *
 */

/** Whether standard qualifiers were generated */
#define MI_MODULE_FLAG_STANDARD_QUALIFIERS (1 << 0)

/** Whether description qualifiers were generated */
#define MI_MODULE_FLAG_DESCRIPTIONS (1 << 1)

/** Whether Values and ValueMap qualifiers were generated */
#define MI_MODULE_FLAG_VALUES (1 << 2)

/** Whether the MappingStrings qualifiers were generated */
#define MI_MODULE_FLAG_MAPPING_STRINGS (1 << 3)

/** Whether the boolean qualifiers were generated */
#define MI_MODULE_FLAG_BOOLEANS (1 << 4)

/** Whether C++ extensions were generated */
#define MI_MODULE_FLAG_CPLUSPLUS (1 << 5)

/** Whether translatable qualifiers were localized (and STRING.RC generated) */
#define MI_MODULE_FLAG_LOCALIZED (1 << 6)

/** Whether filters are supported */
#define MI_MODULE_FLAG_FILTER_SUPPORT (1 << 7)


/** 
 * This function is called to load the main provider module. The implementation
 * resides in the file named module.c. The provider developer may define a 
 * suitable MI_Module_Self structure in module.c, for example:
 *
 * <pre>
 *     struct MI_Module_Self
 *     {
 *         int myField;
 *     };
 * </pre>
 *
 * A typical implementation may look like this:
 *
 * <pre>
 *     void MI_CALL Load(MI_Module_Self** self, MI_Context* context)
 *     {
 *         *self = (MI_Module_Self*)malloc(sizeof(MI_Module_Self));
 *         (*self)->myField = 1234;
 *     }
 * </pre>
 *
 * Note: this function is asynchronous.
 * 
 * @param self the module state data.
 * @param context the invocation context.
 *
 * @sa MI_Module_Unload()
 *
 */
typedef void (MI_CALL *MI_Module_Load)(
    __out MI_Module_Self** self,
    __in MI_Context* context);

/** 
 * This function is called to unload the main provider module. The 
 * implementation resides in the file named module.c. A typical implementation 
 * may look like this.
 *
 * <pre>
 *     void MI_CALL Unload(MI_Module_Self* self, MI_Context* context)
 *     {
 *         free(*self);
 *     }
 * </pre>
 * 
 * Note: this function is synchronous.
 *
 * @param self the module state data.
 * @param context the invocation context.
 *
 * @sa MI_Module_Load()
 *
 */
typedef void (MI_CALL *MI_Module_Unload)(
    __in_opt MI_Module_Self* self,
    __in MI_Context* context);

/** This structure is returned by the MI_Main() entry point. It contains
 *  all data needed by the provider manager to manage the providers within this
 *  module. A typical implementation of MI_Main() looks something like this.
 *
 *  <pre>
 *      MI_EXPORT MI_Module* MI_MAIN_CALL MI_Main(MI_Server* server)
 *      {
 *          static MI_Module module;
 *          module.flags |= MI_MODULE_FLAG_STANDARD_QUALIFIERS;
 *          module.charSize = sizeof(MI_Char);
 *          module.version = MI_VERSION;
 *          module.generatorVersion = MI_MAKE_VERSION(1,0,0);
 *          module.schemaDecl = \&schemaDecl;
 *          module.Load = Load;
 *          module.Unload = Unload;
 *          return \&module;
 *      }
 *  </pre>
 *
 *  The module may specify both static and dynamic providers. The provider
 *  manager first attempts to find a static provider function table through
 *  the MI_Module.schemaDecl field. If this fails (or if the field is NULL),
 *  it then uses the MI_Module.dynamicProviderFT (if non-NULL). Static 
 *  providers provides only CIM instances, but dynamic providers may provider
 *  CIM instances, CIM classes, and CIM qualifier declarations.
 *
 */
typedef struct _MI_Module
{
    /** The version the provider was compiled with (MI_VERSION) */
    MI_Uint32 version;

    /** The hex value of MI_VERSION when the generator was compiled */
    MI_Uint32 generatorVersion;

    /** Module flags (see MI_MODULE_FLAG_* enumerations) */
    MI_Uint32 flags;

    /** Size of the MI_Char in bytes */
    MI_Uint32 charSize;

    /** Pointer to generated schema declarations (static providers only). */
    MI_SchemaDecl* schemaDecl;

    /** Library initializer */
    MI_Module_Load Load;

    /** Library cleanup */
    MI_Module_Unload Unload;

    /** The module may implement a single 'dynamic provider' (one that 
     *  provides CIM instances, CIM classes and CIM qualifier declarations). 
     *  The provider manager uses this function table when (1) it is non-null, 
     *  and (2) MI_Module.schemaDecl is null or does not contain an RTTI 
     *  corresponding to the given request.
     */
    MI_ProviderFT* dynamicProviderFT;
}
MI_Module;

/* @} */

/*
**==============================================================================
**
** The MI_Instance Module
**
**==============================================================================
*/

/**
 * @defgroup MI_Instance The MI_Instance Module
 * @{
 *
 */

/** The MI_Instance function table */
struct _MI_InstanceFT
{
    MI_Result (MI_CALL *Clone)(
        __in const MI_Instance* self,
        __deref_out MI_Instance** newInstance);

    MI_Result (MI_CALL *Destruct)(
        __in MI_Instance* self);

    MI_Result (MI_CALL *Delete)(
        __in MI_Instance* self);

    MI_Result (MI_CALL *IsA)(
        __in const MI_Instance* self, 
        __in const MI_ClassDecl* classDecl,
        __out MI_Boolean* flag);

    MI_Result (MI_CALL *GetClassName)(
        __in const MI_Instance* self, 
        __deref_out_z_opt const MI_Char** className);

    MI_Result (MI_CALL *SetNameSpace)(
        __in MI_Instance* self, 
        __in_z const MI_Char* nameSpace);

    MI_Result (MI_CALL *GetNameSpace)(
        __in const MI_Instance* self, 
        __deref_out_z_opt const MI_Char** nameSpace);

    MI_Result (MI_CALL *GetElementCount)(
        __in const MI_Instance* self,
        __out MI_Uint32* count);

    MI_Result (MI_CALL *AddElement)(
        __in MI_Instance* self,
        __in_z const MI_Char* name,
        __in_opt const MI_Value* value,
        MI_Type type,
        MI_Uint32 flags);

    MI_Result (MI_CALL *SetElement)(
        __in MI_Instance* self, 
        __in_z const MI_Char* name,
        __in const MI_Value* value,
        MI_Type type,
        MI_Uint32 flags);

    MI_Result (MI_CALL *SetElementAt)(
        __in MI_Instance* self, 
        MI_Uint32 index,
        __in const MI_Value* value,
        MI_Type type,
        MI_Uint32 flags);

    MI_Result (MI_CALL *GetElement)(
        __in const MI_Instance* self, 
        __in_z const MI_Char* name,
        __deref_out_opt MI_Value* value,
        __out_opt MI_Type* type,
        __out_opt MI_Uint32* flags,
        __out_opt MI_Uint32* index);

    MI_Result (MI_CALL *GetElementAt)(
        __in const MI_Instance* self, 
        MI_Uint32 index,
        __deref_out_z_opt const MI_Char** name,
        __deref_out_opt MI_Value* value,
        __out_opt MI_Type* type,
        __out_opt MI_Uint32* flags);

    MI_Result (MI_CALL *ClearElement)(
        __in MI_Instance* self, 
        __in_z const MI_Char* name);

    MI_Result (MI_CALL *ClearElementAt)(
        __out MI_Instance* self, 
        MI_Uint32 index);

    MI_Result (MI_CALL *Print)(
        __in const MI_Instance* self,
        __in FILE* os,
        MI_Uint32 level);
};

/** 
 * This structure represents a CIM instance. Both dynamic and static instances
 * implement this interface. Static instance structures include this structure
 * as the initial data field.
 *
 *     Knowledge of this structure allows provider to create its own,
 *     server-compatible (for RO operations) instances to improve performance.
 *     
 *     The following restrictions are made for 'reserved' part of MI_Instance:
 *     - when server sends instance to the provider, it has to initialize
 *     ft, classDecl, serverName and namespace (last two may be null).
 *    'reserved' space is server-specific and provider may not 
 *    make any assumptions about its content.
 *
 *      - when provider sends instance to the server (PostResult, PostIndication)
 *      it has to initialize classDecl, serverName (optional) and
 *      namespace (optional). ft and reserved space must be 0-initialized.
 *      Server may perform only read only operations on such instance.

 */
struct _MI_Instance
{
    /** Function table */
    MI_InstanceFT* ft;

    /** The class declaration for this instance */
    const MI_ClassDecl* classDecl;

    /** The server name (optional) */
    const MI_Char* serverName;

    /** The namespace (optional) */
    const MI_Char* nameSpace;

    /** Reserved for internal use */
    ptrdiff_t reserved[4];
};

/**
 *
 * This function creates a copy of the given instance on the heap. Upon
 * a successful return, newInstance points to a newly created instance. The
 * new instance should eventually be passed to MI_Instance_Delete().
 * 
 * @param self pointer to the instance to be cloned.
 * @param newInstance a pointer to the new instance upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 * @sa MI_Instance_Delete()
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_Clone(
    __in const MI_Instance* self,
    __deref_out MI_Instance** newInstance)
{
    return self->ft->Clone(self, newInstance);
}

/**
 *
 * This function releases an instance that was created on the stack. This 
 * function applies to instances constructed on the stack using generated 
 * functions of the form CLASSNAME_Construct().
 * 
 * @param self pointer to the instance to be destructed.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 * @sa MI_Instance_Delete()
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_Destruct(
    __in MI_Instance* self)
{
    return self->ft->Destruct(self);
}

/**
 * 
 * This function releases an instance that was created on the heap. Functions
 * created with MI_Instance_Clone() should eventually be passed to this
 * function.
 *
 * @param self pointer to the instance to be released.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 */
MI_INLINE MI_Result MI_CALL MI_Instance_Delete(
    __in MI_Instance* self)
{
    return self->ft->Delete(self);
}

/**
 * This function checks to see if the instance given by 'self' is an 
 * instance of the class given by 'classDecl'. If so it return 
 * MI_TRUE in flag. Otherwise it returns MI_FALSE in flag out parameter.
 *
 * @param self pointer to an instance.
 * @param classDecl the potential 'super' class declaration.
 * @param flag [out] boolean outcome of the operation.
 *
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_IsA(
    __in const MI_Instance* self,
    __in const MI_ClassDecl* classDecl,
    __out MI_Boolean* flag)
{
    return self->ft->IsA(self, classDecl, flag);
}

/**
 * Gets the className of the given instance.
 *
 * @param self instance whose className will be gotten.
 * @param className the class name of the instance upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_GetClassName(
    __in const MI_Instance* self, 
    __deref_out_z_opt const MI_Char** className)
{
    return self->ft->GetClassName(self, className);
}

/**
 * Sets the namespace name of the given instance. Namespace names must conform
 * to the following productions:
 *
 * <pre>
 *     NAME := IDENT ( '/' IDENT )*
 *     IDENT := [A-Za-z_][A-Za-z0-9_]*
 * </pre>
 *
 * Examples:
 *
 * <pre>
 *     root
 *     root/cimv2
 *     aaa/bbb/ccc
 * </pre>
 *
 * @param self the instance whose property will be gotten.
 * @param nameSpace the new namespace of the instance.
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER, MI_RESULT_FAILED
 *         MI_RESULT_INVALID_PARAMETER
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_SetNameSpace(
    __in MI_Instance* self, 
    __in_z const MI_Char* nameSpace)
{
    return self->ft->SetNameSpace(self, nameSpace);
}

/**
 * Gets the namespace name of the given instance.
 *
 * @param self a pointer to an instance.
 * @param nameSpace the namespace name upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_GetNameSpace(
    __in const MI_Instance* self, 
    __deref_out_z_opt const MI_Char** nameSpace)
{
    return self->ft->GetNameSpace(self, nameSpace);
}

/**
 * Retrieves the number of properties in an instance.
 *
 * @param self the instance
 * @param count the number of properties in the instance upon return
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER, MI_RESULT_INVALID_PARAMETER
 *
 * @sa MI_Instance_GetAt(), MI_Instance_SetAt()
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_GetElementCount(
    __in const MI_Instance* self,
    __out MI_Uint32* count)
{
    return self->ft->GetElementCount(self, count);
}

/**
 * 
 * Adds a new property to a dynamic instance (supported only by dynamic
 * instances whose schema may be extended at run time).
 *
 * @param self the instance
 * @param name the name of the new property
 * @param value the value of the new property
 * @param type the type of the new property
 * @param flags the flags of the new property (MI_FLAG_KEY, MI_FLAG_IN, 
 *        MI_FLAG_OUT)
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER
 */
MI_INLINE MI_Result MI_CALL MI_Instance_AddElement(
    __in MI_Instance* self,
    __in_z const MI_Char* name,
    __in_opt const MI_Value* value,
    MI_Type type,
    MI_Uint32 flags)
{
    return self->ft->AddElement(self, name, value, type, flags);
}

/**
 * Set the value of the property at the given index.
 *
 * @param self a pointer to an instance.
 * @param index the integer position of the property.
 * @param value the new value for the property.
 * @param type the CIM type of the property that will be set.
 * @param flags bit flags indicating memory management policy (MI_FLAG_BORROW).
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED, MI_RESULT_TYPE_MISMATCH
 *     MI_RESULT_INVALID_PARAMETER, MI_RESULT_NOT_FOUND, MI_RESULT_FAILED
 *
 * @sa MI_Instance_GetAt()
 * @sa MI_Instance_GetElementCount()
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_SetElementAt(
    __in MI_Instance* self, 
    MI_Uint32 index,
    __in_opt const MI_Value* value,
    MI_Type type,
    MI_Uint32 flags)
{
    return self->ft->SetElementAt(self, index, value, type, flags);
}

/**
 * Set the value of the property with the given name. By default, all memory
 * referred to by the value parameter is copied. By passing the flag 
 * MI_FLAG_BORROW, memory pointers within the value structure are stored 
 * directly in the instance's property. The caller must guarantee that the
 * memory outlives the instance.
 *
 * @param self a pointer to an instance.
 * @param name the name of the property that will be set.
 * @param value the new value for the property.
 * @param type the CIM type of the property that will be set.
 * @param flags bit flags indicating memory management policy (MI_FLAG_BORROW).
 *
 * @return MI_RESULT_O, KMI_RESULT_TYPE_MISMATCH, MI_RESULT_INVALID_PARAMETER
 *     MI_RESULT_NOT_FOUND, MI_RESULT_FAILED
 * 
 * @sa MI_Instance_GetElement()
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_SetElement(
    __out MI_Instance* self, 
    __in_z const MI_Char* name,
    __in_opt const MI_Value* value,
    MI_Type type,
    MI_Uint32 flags)
{
    return self->ft->SetElement(self, name, value, type, flags);
}

/**
 *
 * Gets the value of the property with the given name.
 *
 * @param self the instance whose property will be gotten.
 * @param name the name of the property that will be gotten.
 * @param value contains the value of the property upon return.
 * @param type contains the type of the property upon return.
 * @param flags the flags associated with property (MI_FLAG_NULL, MI_FLAG_KEY,
 *        MI_FLAG_IN, MI_FLAG_OUT).
 * @param index the index of the named attribute.
 * 
 * @return MI_RESULT_OK, MI_RESULT_TYPE_MISMATCH, MI_RESULT_INVALID_PARAMETER
 *     MI_RESULT_NOT_FOUND, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_GetElement(
    __in const MI_Instance* self, 
    __in_z const MI_Char* name,
    __out_opt MI_Value* value,
    __out_opt MI_Type* type,
    __out_opt MI_Uint32* flags,
    __out_opt MI_Uint32* index)
{
    return self->ft->GetElement(self, name, value, type, flags, index);
}

/**
 *
 * Get the value of the property at the given index.
 *
 * @param self the instance whose property will be gotten.
 * @param index the position of the property that will be set.
 * @param name the name of the property upon return.
 * @param value the value of the property upon return.
 * @param type the type of the property upon return.
 * @param flags the flags of the property upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER, MI_RESULT_FAILED,
 * MI_RESULT_FAILED
 *
 * @sa MI_Instance_SetAt()
 * @sa MI_Instance_GetElementCount()
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_GetElementAt(
    __in const MI_Instance* self,
    MI_Uint32 index,
    __deref_out_z_opt const MI_Char** name,
    __out_opt MI_Value* value,
    __out_opt MI_Type* type,
    __out_opt MI_Uint32* flags)
{
    return self->ft->GetElementAt(self, index, name, value, type, flags);
}

/**
 *
 * Clears the value of the property with the given name. Afterwards, the
 * property has a null value.
 *
 * @param self the instance whose property will be set.
 * @param name the name of the property that will be cleared.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 */
MI_INLINE MI_Result MI_CALL MI_Instance_ClearElement(
    __in MI_Instance* self, 
    __in_z const MI_Char* name)
{
    return self->ft->ClearElement(self, name);
}

/**
 *
 * Clears the value of the property at the given index. Afterwards, the
 * property has a null value.
 *
 * @param self the instance whose property will be cleared.
 * @param index the position of the property that will be cleared.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_ClearElementAt(
    __in MI_Instance* self, 
    MI_Uint32 index)
{
    return self->ft->ClearElementAt(self, index);
}

/**
 * 
 * Prints an instance to the given stream object.
 *
 * @param self the instance
 * @param os the output stream the instance will be printed on.
 * @param level the indentation level
 *
 * @return MI_RESULT_OK, MI_RESULT_INVALID_PARAMETER
 *
 */
MI_INLINE MI_Result MI_CALL MI_Instance_Print(
    __in const MI_Instance* self,
    __in FILE* os,
    MI_Uint32 level)
{
    return self->ft->Print(self, os, level);
}

/** @} */

/*
**==============================================================================
**
** The MI_Context Module
**
**==============================================================================
*/

/**
 * @defgroup MI_Context The MI_Context Module
 * @{
 *
 */

/* The maximum size of a locale string (including zero-terminator). */
#define MI_MAX_LOCALE_SIZE 128

/* Defines the support locale enum tags used by the MI_Context.GetLocale() */
typedef enum _MI_LocaleType
{
    MI_LOCALE_TYPE_REQUESTED_UI,
    MI_LOCALE_TYPE_REQUESTED_DATA,
    MI_LOCALE_TYPE_CLOSEST_UI,
    MI_LOCALE_TYPE_CLOSEST_DATA
}
MI_LocaleType;

typedef enum _MI_CancelationReason
{
    MI_REASON_NONE,
    MI_REASON_TIMEOUT,
    MI_REASON_SHUTDOWN,
    MI_REASON_SERVICESTOP
}
MI_CancelationReason;

typedef void (MI_CALL *MI_CancelCallback)(
    MI_CancelationReason reason,
    __in_opt void* callbackData);

/* Defines the channel numbers for WriteMessage PS semantic callback */
#define MI_WRITEMESSAGE_CHANNEL_WARNING 0
#define MI_WRITEMESSAGE_CHANNEL_VERBOSE 1
#define MI_WRITEMESSAGE_CHANNEL_DEBUG   2

/** Defines the function table used by MI_Context */
struct _MI_ContextFT
{
    /*
    **--------------------------------------------------------------------------
    **
    ** Post Methods
    **
    **--------------------------------------------------------------------------
    */

    MI_Result (MI_CALL *PostResult)(
        __in MI_Context* context,
        MI_Result result);

    MI_Result (MI_CALL *PostResultWithMessage)(
        __in MI_Context* context,
        MI_Result result,
        __in_z const MI_Char* message);

    MI_Result (MI_CALL *PostResultWithError)(
        __in MI_Context* context,
        MI_Result result,
        __in const MI_Instance* error);

    MI_Result (MI_CALL *PostInstance)(
        __in MI_Context* context,
        __in const MI_Instance* instance);

    MI_Result (MI_CALL *PostIndication)(
        __in MI_Context* context,
        __in const MI_Instance* indication,
        MI_Uint32 subscriptionIDCount,
        __in_z_opt const MI_Char* bookmark);

    /*
    **--------------------------------------------------------------------------
    **
    ** Factory Methods
    **
    **--------------------------------------------------------------------------
    */

    MI_Result (MI_CALL *ConstructInstance)(
        __in MI_Context* context,
        __in const MI_ClassDecl* classDecl,
        __out MI_Instance* instance);

    MI_Result (MI_CALL *ConstructParameters)(
        __in MI_Context* context,
        __in const MI_MethodDecl* methodDecl,
        __out MI_Instance* instance);

    MI_Result (MI_CALL *NewInstance)(
        __in MI_Context* context,
        __in const MI_ClassDecl* classDecl,
        __deref_out MI_Instance** instance);

    /* ATTN:NEW */
    MI_Result (MI_CALL *NewDynamicInstance)(
        __in MI_Context* context,
        __in const MI_Char* className,
        MI_Uint32 flags,
        __deref_out MI_Instance** instance);

    MI_Result (MI_CALL *NewParameters)(
        __in MI_Context* context,
        __in const MI_MethodDecl* methodDecl,
        __deref_out MI_Instance** instance);

    /*
    **--------------------------------------------------------------------------
    **
    ** Misc. Methods
    **
    **--------------------------------------------------------------------------
    */

    MI_Result (MI_CALL *Canceled)(
        __in const MI_Context* context,
        __out MI_Boolean* flag);

    MI_Result (MI_CALL *GetLocale)(
        __in const MI_Context* context,
        MI_LocaleType localeType,
        __out_z MI_Char locale[MI_MAX_LOCALE_SIZE]);

    MI_Result (MI_CALL *RegisterCancel)(
        __in MI_Context* context,
        __in MI_CancelCallback callback,
        __in_opt void* callbackData);

    MI_Result (MI_CALL *RequestUnload)(
        __in MI_Context* context);

    MI_Result (MI_CALL *RefuseUnload)(
        __in MI_Context* context);

    MI_Result (MI_CALL *GetLocalSession)(
        __in const MI_Context* context,
        __out MI_Session* session);

    MI_Result (MI_CALL *SetStringOption)(
        __in MI_Context* context,
        __in_z const MI_Char* name,
        __in_z const MI_Char* value);

    MI_Result (MI_CALL *GetStringOption)(
        __in const MI_Context* context,
        __in_z const MI_Char* name,
        __out_z const MI_Char** value);

    /*
    **--------------------------------------------------------------------------
    **
    ** PowerShell Methods
    **
    **--------------------------------------------------------------------------
    */

    MI_Result (MI_CALL *ShouldProcess)(
        __in MI_Context* context,
        __in_z const MI_Char* message,
        __out MI_Boolean* flag);

    MI_Result (MI_CALL *ShouldContinue)(
        __in MI_Context* context,
        __in_z const MI_Char* message,
        __out MI_Boolean* flag);

    MI_Result (MI_CALL *WriteError)(
        __in MI_Context* context,
        MI_Uint32 errorId,
        MI_ErrorCategory errorCategory,
        __in_z const MI_Char* errorMessage,
        __in_z const MI_Char* targetName);

    MI_Result (MI_CALL *WriteMessage)(
        __in MI_Context* context,
        MI_Uint32 channel,
        __in_z const MI_Char* message);

    MI_Result (MI_CALL *WriteProgress)(
        __in MI_Context* context,
        __in_z const MI_Char* activity,
        __in_z const MI_Char* currentOperation,
        __in_z const MI_Char* statusDescription,
        MI_Uint32 percentComplete,
        MI_Uint32 secondsRemaining);

    MI_Result (MI_CALL *WriteCommandDetail)(
        __in MI_Context* context,
        __in_z const MI_Char* message,
        void* reserved);

    MI_Result (MI_CALL *WriteStreamParameter)(
        __in MI_Context* context,
        __in_z const MI_Char* name,
        __in const MI_Value* value,
        __in MI_Type type,
        __in MI_Uint32 flags);
};

/** Operations are defined on this structure for (1) posting results,
 *  (2) posting instances, (3) creating new objects.
 *
 */
struct _MI_Context
{
    /* Function table */
    MI_ContextFT* ft;

    /* Reserved for internal use */
    ptrdiff_t reserved[3];
};

/**
 * Providers call this function to post a return code to the server in
 * response to a request.
 *
 * @param context the request context
 * @param result the result code
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PostResult(
    __in MI_Context* context,
    MI_Result result)
{
    return context->ft->PostResult(context, result);
}

/**
 * Providers call this function to post a return code and an error message
 * to the server in response to a request.
 *
 * @param context the request context.
 * @param result the result code.
 * @param message an error message containing detail about the error.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PostResultWithMessage(
    __in MI_Context* context,
    MI_Result result,
    __in const MI_Char* message)
{
    return context->ft->PostResultWithMessage(context, result, message);
}

/**
 * Providers call this function to post a return code and a CIM error
 * instance to the server in response to a request. The instance is of
 * the standard CIM class 'CIM_Error' (or derived from it), which the provider 
 * developer must generate and include in the provider.
 *
 * @param context the request context.
 * @param result the result code.
 * @param error an instance of the CIM error class
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PostResultWithError(
    __in MI_Context* context,
    MI_Result result,
    __in const MI_Instance* error)
{
    return context->ft->PostResultWithError(context, result, error);
}

/**
 * Providers call this function to post an instance to the server in 
 * response to a request. The server is responsible for copying the
 * instance so the provider is free to dispose of the instance afterwards.
 *
 * @param context the request context
 * @param instance instance to be posted
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PostInstance(
    __in MI_Context* context, 
    __in const MI_Instance* instance)
{
    return context->ft->PostInstance(context, instance);
}

/**
 * Providers call this function to post an indication to the server in 
 * response to a request. The server is responsible for copying the
 * instance so the provider is free to dispose of the instance afterwards.
 *
 * @param context the request context
 * @param indication the indication to be posted
 * @param subscriptionIDCount the number of subscription identifiers.
 * @param bookmark the bookmark for this subscription.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_PostIndication(
    __in MI_Context* context, 
    __in const MI_Instance* indication,
    MI_Uint32 subscriptionIDCount,
    __in_z const MI_Char* bookmark)
{
    /* ATTN:QUESTION what is subscriptionIDCount for? */
    return context->ft->PostIndication(
        context, indication, subscriptionIDCount, bookmark);
}

/**
 * A provider calls this function to initialize an instance. The caller
 * is responsible for reserving the memory for the instance (either on
 * the stack or the heap). The caller should eventually pass
 * the instance to MI_Instance_Destruct().
 *
 * @param context the request context.
 * @param instance the instance to be initialized.
 * @param classDecl the class declaration used to initialize the instance.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_ConstructInstance(
    __in MI_Context* context, 
    __in const MI_ClassDecl* classDecl,
    __out MI_Instance* instance)
{
    return context->ft->ConstructInstance(context, classDecl, instance);
}

/**
 * A provider calls this function to initialize a parameters instance.
 * The caller is responsible for reserving the memory for the instance 
 * (either on the stack or the heap). The caller should eventually pass
 * the instance to MI_Instance_Destruct().
 *
 * @param context the request context.
 * @param instance the instance to be initialized.
 * @param methodDecl the method declaration used to initialize the instance.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_ConstructParameters(
    __in MI_Context* context, 
    __in const MI_MethodDecl* methodDecl,
    __out MI_Instance* instance)
{
    return context->ft->ConstructParameters(context, methodDecl, instance);
}

/**
 * This function creates a new instance of the class given by the classDecl
 * parameter. The caller should eventually pass the instance to
 * MI_Instance_Delete().
 *
 * @param context the request context
 * @param classDecl the class declaration used to initialize the instance.
 * @param instance points to a new instance upon successful return.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_NewInstance(
    __in MI_Context* context,
    __in const MI_ClassDecl* classDecl,
    __deref_out MI_Instance** instance)
{
    return context->ft->NewInstance(context, classDecl, instance);
}

/**
 * This function creates a new dynamic instance of the class whose name is 
 * given by the className parameter. The caller should eventually pass the 
 * instance to MI_Instance_Delete().
 *
 * @param context the request context
 * @param className the name of the new class.
 * @param flags create flags (include class meta type).
 * @param instance points to a new instance upon successful return.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_NewDynamicInstance(
    __in MI_Context* context,
    __in const MI_Char* className,
    MI_Uint32 flags,
    __deref_out MI_Instance** instance)
{
    return context->ft->NewDynamicInstance(context, className, flags, instance);
}

/**
 * This function creates a new instance of the method given by the 
 * methodDecl parameter. The caller should eventually pass the instance to
 * MI_Instance_Delete().
 *
 * @param context the request context
 * @param methodDecl the method declaration used to initialize the instance.
 * @param instance points to a new instance upon successful return.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_NewParameters(
    __in MI_Context* context,
    __in const MI_MethodDecl* methodDecl,
    __deref_out MI_Instance** instance)
{
    return context->ft->NewParameters(context, methodDecl, instance);
}

/**
 * Providers call this function periodically to determine whether the
 * operation has been canceled. If so, the flag parameter is set to
 * MI_TRUE and MI_RESULT_OK is returned.
 *
 * @param context the request context
 * @param flag upon return this flag indicates whether the operation has been
 *        canceled.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_Canceled(
    __in const MI_Context* context,
    __out MI_Boolean* flag)
{
    return context->ft->Canceled(context, flag);
}

/**
 * This function returns the locale of the given type.
 *
 * @param context the request context
 * @param localeType the type of locale to be returned.
 * @param locale the locale upon return.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_GetLocale(
    __in const MI_Context* context,
    MI_LocaleType localeType,
    __out_z MI_Char locale[MI_MAX_LOCALE_SIZE])
{
    return context->ft->GetLocale(context, localeType, locale);
}

/**
 * This function registers a callback that is called when the operation
 * is canceled.
 * 
 * @param context the request context.
 * @param callback call this function on cancel.
 * @param callbackData pass this data to the callback.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_RegisterCancel(
    __in MI_Context* context,
    __in MI_CancelCallback callback,
    __in_opt void* callbackData)
{
    return context->ft->RegisterCancel(context, callback, callbackData);
}

/**
 * This function requests to unload the module or the provider (depending
 * on the location of invocation). Providers should call this function within
 * their load methods. The provider will be unloaded soon after this call.
 * 
 * @param context the request context.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_RequestUnload(
    __in MI_Context* context)
{
    return context->ft->RequestUnload(context);
}

/**
 * By calling this, the provider prevents itself from being unloaded after
 * provider unload timeout (but it does not prevent it from being unloaded
 * during CIM server shutdown). After calling this, the provider manages 
 * its own lifetime. The provider may call MI_RequestUnload() to request an
 * unload at any time. This function should be called with the load method.
 * 
 * @param context the request context.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_RefuseUnload(
    __in MI_Context* context)
{
    return context->ft->RefuseUnload(context);
}

/**
 * Gets the local session (MI_Session), which allows the provider to 
 * communicate with the CIM server. This session is pre-instantiated
 * and has the lifetime of the context (from which the session was
 * obtained. The provider MUST NOT destruct this session, since its
 * lifetime is bound to the context.
 * 
 * @param context the request context.
 * @param session the local session handle.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_GetLocalSession(
    __in const MI_Context* context,
    __out MI_Session* session)
{
    return context->ft->GetLocalSession(context, session);
}

/**
 * Sets context-specific option. It allows the provider to 
 * adjust server's behavior. Typically is server-specific.
 * 
 * @param context the request context.
 * @param name of the option to change.
 * @param value - new value for the option.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_SetStringOption(
    __in MI_Context* context,
    __in_z const MI_Char* name,
    __in_z const MI_Char* value)
{
    return context->ft->SetStringOption(context, name, value);
}

/**
 * Gets context-specific option. 
 * 
 * @param context the request context.
 * @param name of the option to get.
 * @param value [out] of the option.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED
 *
 */
MI_INLINE MI_Result MI_CALL MI_GetStringOption(
    __in const MI_Context* context,
    __in_z const MI_Char* name,
    __out_z const MI_Char** value)
{
    return context->ft->GetStringOption(context, name, value);
}

/**
 * This function implements the ShouldProcess PowerShell operation.
 * 
 * @param context the request context.
 * @param message the message.
 * @param flag MI_TRUE if for 'should process'.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED.
 *
 */
MI_INLINE MI_Result MI_CALL MI_ShouldProcess(
    __in MI_Context* context,
    __in_z const MI_Char* message,
    __out MI_Boolean* flag)
{
    return context->ft->ShouldProcess(context, message, flag);
}

/**
 * This function implements the ShouldContinue PowerShell operation.
 * 
 * @param context the request context.
 * @param message a message.
 * @param flag MI_TRUE if for 'should continue'.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED.
 *
 */
MI_INLINE MI_Result MI_CALL MI_ShouldContinue(
    __in MI_Context* context,
    __in_z const MI_Char* message,
    __out MI_Boolean* flag)
{
    return context->ft->ShouldContinue(context, message, flag);
}

/**
 * This function implements the WriteError PowerShell operation.
 * 
 * @param context the request context.
 * @param errorId
 * @param errorCategory
 * @param errorMessage
 * @param targetName
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED.
 *
 */
MI_INLINE MI_Result MI_CALL MI_WriteError(
    __in MI_Context* context,
    MI_Uint32 errorId,
    MI_ErrorCategory errorCategory,
    __in_z const MI_Char* errorMessage,
    __in_z const MI_Char* targetName)
{
    return context->ft->WriteError(context, errorId, errorCategory, 
        errorMessage, targetName);
}

/**
 * This function implements the WriteMessage PowerShell operation.
 * 
 * @param context the request context.
 * @param channel
 * @param message
 *
 * @return MI_TRUE or MI_FALSE.
 *
 */
MI_INLINE MI_Result MI_CALL MI_WriteMessage(
    __in MI_Context* context,
    MI_Uint32 channel,
    __in_z const MI_Char* message)
{
    return context->ft->WriteMessage(context, channel, message);
}

/**
 * This function implements the WriteProgress PowerShell operation.
 * 
 * @param context the request context.
 * @param activity
 * @param currentOperation
 * @param statusDescription
 * @param percentComplete
 * @param secondsRemaining
 *
 * @return MI_TRUE or MI_FALSE.
 *
 */
MI_INLINE MI_Result MI_CALL MI_WriteProgress(
    __in MI_Context* context,
    __in_z const MI_Char* activity,
    __in_z const MI_Char* currentOperation,
    __in_z const MI_Char* statusDescription,
    MI_Uint32 percentComplete,
    MI_Uint32 secondsRemaining)
{
    return context->ft->WriteProgress(context, activity, currentOperation, 
        statusDescription, percentComplete, secondsRemaining);
}

/**
 * This function implements the CommandDetail PowerShell operation.
 * 
 * @param context the request context.
 * @param message
 * @param reserved
 *
 * @return MI_TRUE or MI_FALSE.
 *
 */
MI_INLINE MI_Result MI_CALL MI_WriteCommandDetail(
    __in MI_Context* context,
    __in_z const MI_Char* message,
    void *reserved)
{
    return context->ft->WriteCommandDetail(context, message, reserved);
}

/**
 * The provider calls this function to send streamed data to the requestor.
 * The value is an array that contains one or more elements of the specified
 * type. Call this function repeatedly to send the entire stream. Once the
 * entire stream has been sent, call it one final, passing MI_FLAG_NULL to
 * the flag parameter (to indicate the end of the stream).
 * 
 * @param self the request context
 * @param name the name of a parameter.
 * @param value an array value with at least one element.
 * @param type the type (must be an array type).
 * @param flags MI_FLAG_NULL on final call.
 *
 * @return MI_RESULT_OK, MI_RESULT_FAILED.
 *
 */
MI_INLINE MI_Result MI_CALL MI_WriteStreamParameter(
    __in MI_Context* self,
    __in_z const MI_Char* name,
    __in const MI_Value* value,
    __in MI_Type type,
    __in MI_Uint32 flags)
{
    return self->ft->WriteStreamParameter(self, name, value, type, flags);
}

/** @} */


/*
**==============================================================================
**
**  MI_InstanceOf
**      converts pointer to concrete instance to pointer to MI_Instance
**
**==============================================================================
*/
#define MI_InstanceOf(inst)  (&(inst)->__instance)

/*
**==============================================================================
**
**  Undo SAL macro (on non-Windows)
**
**==============================================================================
*/
#if !defined(_MSC_VER)
# undef __in
# undef __out
# undef __in_opt
# undef __deref_out
# undef __deref_out_opt
# undef __deref_out_z_opt
# undef __in_z
# undef __out_z
# undef __in_z_opt
# undef __out_opt
# undef __out_ecount_z
# undef __inout_ecount_z
# undef __format_string
#endif


/*
**==============================================================================
**
**  Undo eight-byte packing for structures (on Windows)
**
**==============================================================================
*/

#if defined(_MSC_VER)
# pragma pack(pop)
#endif

#endif /* _MI_h */
