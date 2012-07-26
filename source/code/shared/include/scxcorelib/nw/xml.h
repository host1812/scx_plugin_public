#ifndef _oscar_xml_h
#define _oscar_xml_h

#include <stddef.h>

/* The maximum number of nested XML elements */
#define XML_MAX_NESTED 64

/* The maximum number of XML namespaces */
#define XML_MAX_NAMESPACES 32

/* The maximum number of registered XML namespaces */
#define XML_MAX_REGISTERED_NAMESPACES 32

/* The maximum number of attributes in a start tag */
#define XML_MAX_ATTRIBUTES 32

/* Represents case where tag has no namespace */
#define XML_NAMESPACE_NONE 0

#if defined(__cplusplus)
extern "C" {
#endif

/* Represents an XML name */
typedef struct _XML_Name
{
    /* Pointer to name */
    char* data;
    /* Size of name (excluding zero-terminator) */
    size_t size;
}
XML_Name;

/* Represents an XML namespace as registered by the client */
typedef struct _XML_RegisteredNameSpace
{
    /* URI for this namespace */
    const char* uri;

    /* Hash code for uri */
    unsigned int uriCode;

    /* Single character namespace name expected by client */
    char id;
}
XML_RegisteredNameSpace;

/* Represents an XML namespace as encountered during parsing */
typedef struct _XML_NameSpace
{
    /* Namespace name */
    const char* name;

    /* Hash code for name */
    unsigned int nameCode;

    /* URI for this namespace */
    const char* uri;

    /* Single character namespace name expected by client */
    char id;

    /* Depth at which this definition was encountered */
    size_t depth;
}
XML_NameSpace;

void XML_NameSpace_Dump(
    XML_NameSpace* self);

/* Represents an XML attributes */
typedef struct _XML_Attr
{
    const char* name;
    const char* value;
}
XML_Attr;

/* XML element type tag */
typedef enum _XML_Type
{
    XML_NONE,
    XML_START,
    XML_END,
    XML_INSTRUCTION,
    XML_CHARS,
    XML_COMMENT
}
XML_Type;

#if 0
/* Attribute info (for structure mappings) */
typedef struct _XML_AttrDecl
{
    /* Name of this attribute */
    const char* name;

    /* 'S'=String 'L'=Long 'D'=Double */
    char type;

    /* Offset of the field in destination structure */
    size_t offset;

    /* Whether this attribute is required */
    int required;

    /* Pointer to value (char*, long*, or double*) */
    void* value;
}
XML_AttrDecl;
#endif

/* Represents one XML element */
typedef struct _XML_Elem
{
    /* Type of this XML object */
    XML_Type type;

    /* Character data or tag name */
    const char* data;
    size_t size;

    /* Attributes */
    XML_Attr attrs[XML_MAX_ATTRIBUTES];
    size_t attrsSize;
}
XML_Elem;

const char* XML_Elem_GetAttr(
    XML_Elem* self,
    const char* name);

void XML_Elem_Dump(
    const XML_Elem* self);

typedef struct _XML
{
    /* Points to first text character zero-terminated text */
    char* text;

    /* Pointer to current character */
    char* ptr;

    /* Line number */
    size_t line;

    /* Status: 0=Okay, 1=Done, 2=Failed */
    int status;

    /* Error message */
    char message[256];
    
    /* Stack of open tags (used to match closing tags) */
    XML_Name stack[XML_MAX_NESTED];
    size_t stackSize;

    /* Current nesting level */
    size_t nesting;

    /* Stack of dummy elements generated for empty tags and PutBack calls */
    XML_Elem elemStack[XML_MAX_NESTED];
    size_t elemStackSize;

    /* Array of namespaces */
    XML_NameSpace nameSpaces[XML_MAX_NAMESPACES];
    size_t nameSpacesSize;

    /* Index of last namespace lookup from nameSpaces[] array */
    size_t nameSpacesCacheIndex;

    /* Predefined namespaces */
    XML_RegisteredNameSpace registeredNameSpaces[XML_MAX_NAMESPACES];
    size_t registeredNameSpacesSize;

    /* Internal parser state */
    int state;

    /* Whether XML root element has been encountered */
    int foundRoot;
}
XML;

void XML_Init(
    XML* self);

void XML_SetText(
    XML* self,
    char* text);

int XML_Next(
    XML* self,
    XML_Elem* elem);

int XML_Expect(
    XML* self,
    XML_Elem* elem,
    XML_Type type,
    const char* name);

int XML_Skip(
    XML* self);

int XML_RegisterNameSpace(
    XML* self,
    char id,
    const char* uri);

int XML_PutBack(
    XML* self,
    const XML_Elem* elem);

void XML_Dump(
    XML* self);

void XML_PutError(XML* self);

void XML_Raise(XML* self, const char* format, ...);

void XML_FormatError(XML* self, char* format, size_t size);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _oscar_xml_h */
