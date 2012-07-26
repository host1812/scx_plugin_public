#ifndef _osc_buf_h
#define _osc_buf_h

#include <scxcorelib/nw/config.h>
#include <string.h>
#include <scxcorelib/nw/common.h>

BEGIN_EXTERNC

/* Represents a page of memory */
typedef struct _Page
{
    union
    {
        struct _Page_s
        {
            struct _Page* next;

            /* If non-zero, this page may be independenty released (contains
             * a single block). If so, then passing it to Batch_Put() will
             * pass this block to free(). Otherwise, calling Batch_Put()
             * has no effect.
             */
            unsigned int independent:1;

            /* Size of the allocation */
            unsigned int size:31;
        }
        s;
        char alignment[(sizeof(struct _Page_s) + 7) & ~7];
    }
    u;
}
Page;

/*
**==============================================================================
**
** Buf
**
**==============================================================================
*/
#define BUF_INITIALIZER { NULL, 0, 0, 0 }

typedef struct _Buf
{
    /* The buffer memory (preceded by Page object) */
    void* data;

    /* The current size of the buffer */
    MI_Uint32 size;

    /* The current capacity of the buffer */
    MI_Uint32 capacity;

    /* The current unpacking offset of ths buffer */
    MI_Uint32 offset;
}
Buf;

MI_Result Buf_Init(
    Buf* self,
    MI_Uint32 capacity);

void Buf_Destroy(
    Buf* self);

MI_Result Buf_Reserve(
    Buf* self, 
    MI_Uint32 capacity);

/* Steal the memory page (can be passed to Batch_Attach) */
Page* Buf_StealPage(
    Buf* self);

MI_Result Buf_App(
    Buf* self, 
    const void* data, 
    MI_Uint32 size);

/* Pad buffer out to the next 2-byte boundary */
MI_INLINE MI_Result Buf_Pad16(
    Buf* self)
{
    MI_Uint32 offset = (self->size + 1) & ~1;

    if (offset > self->capacity)
    {
        if (Buf_Reserve(self, offset) != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    self->size = offset;
    return MI_RESULT_OK;
}

/* Pad buffer out to the next 4-byte boundary */
MI_INLINE MI_Result Buf_Pad32(
    Buf* self)
{
    MI_Uint32 offset = (self->size + 3) & ~3;

    if (offset > self->capacity)
    {
        if (Buf_Reserve(self, offset) != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    self->size = offset;
    return MI_RESULT_OK;
}

/* Pad buffer out to the next 8-byte boundary */
MI_INLINE MI_Result Buf_Pad64(
    Buf* self)
{
    MI_Uint32 offset = (self->size + 7) & ~7;

    if (offset > self->capacity)
    {
        if (Buf_Reserve(self, offset) != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    self->size = offset;
    return MI_RESULT_OK;
}

/* Align buffer on the next 2 byte boundary */
MI_INLINE MI_Result Buf_Align16(
    Buf* self)
{
    MI_Uint32 offset = (self->offset + 1) & ~1;

    if (offset > self->size)
        return MI_RESULT_FAILED;

    self->offset = offset;
    return MI_RESULT_OK;
}

/* Align buffer on the next 4 byte boundary */
MI_INLINE MI_Result Buf_Align32(
    Buf* self)
{
    MI_Uint32 offset = (self->offset + 3) & ~3;

    if (offset > self->size)
        return MI_RESULT_FAILED;

    self->offset = offset;
    return MI_RESULT_OK;
}

/* Align buffer on the next 8 byte boundary */
MI_INLINE MI_Result Buf_Align64(
    Buf* self)
{
    MI_Uint32 offset = (self->offset + 7) & ~7;

    if (offset > self->size)
        return MI_RESULT_FAILED;

    self->offset = offset;
    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_PackU8(
    Buf* self,
    MI_Uint8 x)
{
    MI_Uint32 offset = self->size;

    if (offset + sizeof(x) > self->capacity)
    {
        if (Buf_Reserve(self, offset + (MI_Uint32)sizeof(x)) != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    *((MI_Uint8*)((char*)self->data + offset)) = x;
    self->size = offset + (MI_Uint32)sizeof(x);
    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_PackU16(
    Buf* self,
    MI_Uint16 x)
{
    MI_Uint32 offset = (self->size + 1) & ~1;

    if (offset + sizeof(x) > self->capacity)
    {
        if (Buf_Reserve(self, offset + (MI_Uint32)sizeof(x)) != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    *((MI_Uint16*)((char*)self->data + offset)) = x;
    self->size = offset + (MI_Uint32)sizeof(x);
    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_PackU32(
    Buf* self,
    MI_Uint32 x)
{
    MI_Uint32 offset = (self->size + 3) & ~3;

    if (offset + sizeof(x) > self->capacity)
    {
        if (Buf_Reserve(self, offset + (MI_Uint32)sizeof(x)) != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    *((MI_Uint32*)((char*)self->data + offset)) = x;
    self->size = offset + (MI_Uint32)sizeof(x);
    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_PackU64(
    Buf* self,
    MI_Uint64 x)
{
    MI_Uint32 offset = (self->size + 7) & ~7;

    if (offset + sizeof(x) > self->capacity)
    {
        if (Buf_Reserve(self, offset + (MI_Uint32)sizeof(x)) != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    *((MI_Uint64*)((char*)self->data + offset)) = x;
    self->size = offset + (MI_Uint32)sizeof(x);
    return MI_RESULT_OK;
}

MI_Result Buf_PackStr(
    Buf* self,
    const MI_Char* x);

MI_INLINE MI_Result Buf_PackU8A(
    Buf* self,
    MI_Uint8* data,
    MI_Uint32 size)
{
    MI_RETURN_ERR(Buf_PackU32(self, size));

    if (size)
        MI_RETURN_ERR(Buf_App(self, data, size * sizeof(MI_Uint8)));

    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_PackU16A(
    Buf* self,
    MI_Uint16* data,
    MI_Uint32 size)
{
    MI_RETURN_ERR(Buf_PackU32(self, size));

    if (size)
        MI_RETURN_ERR(Buf_App(self, data, size * (MI_Uint32)sizeof(MI_Uint16)));

    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_PackU32A(
    Buf* self,
    MI_Uint32* data,
    MI_Uint32 size)
{
    MI_RETURN_ERR(Buf_PackU32(self, size));

    if (size)
        MI_RETURN_ERR(Buf_App(self, data, size * (MI_Uint32)sizeof(MI_Uint32)));

    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_PackU64A(
    Buf* self,
    MI_Uint64* data,
    MI_Uint32 size)
{
    MI_RETURN_ERR(Buf_PackU32(self, size));

    if (size)
    {
        MI_RETURN_ERR(Buf_Pad64(self));
        MI_RETURN_ERR(Buf_App(self, data, size * (MI_Uint32)sizeof(MI_Uint64)));
    }

    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_UnpackU8(
    Buf* self,
    MI_Uint8* x)
{
    MI_Uint32 offset = self->offset;

    if (offset + sizeof(*x) > self->size)
        return MI_RESULT_FAILED;

    *x = *((MI_Uint8*)((char*)self->data + offset));
    self->offset = offset + (MI_Uint32)sizeof(*x);
    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_UnpackU16(
    Buf* self,
    MI_Uint16* x)
{
    MI_Uint32 offset = (self->offset + 1) & ~1;

    if (offset + sizeof(*x) > self->size)
        return MI_RESULT_FAILED;

    *x = *((MI_Uint16*)((char*)self->data + offset));
    self->offset = offset + (MI_Uint32)sizeof(*x);
    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_UnpackU32(
    Buf* self,
    MI_Uint32* x)
{
    MI_Uint32 offset = (self->offset + 3) & ~3;

    if (offset + sizeof(*x) > self->size)
        return MI_RESULT_FAILED;

    *x = *((MI_Uint32*)((char*)self->data + offset));
    self->offset = offset + (MI_Uint32)sizeof(*x);
    return MI_RESULT_OK;
}

MI_INLINE MI_Result Buf_UnpackU64(
    Buf* self,
    MI_Uint64* x)
{
    MI_Uint32 offset = (self->offset + 7) & ~7;

    if (offset + sizeof(*x) > self->size)
        return MI_RESULT_FAILED;

    *x = *((MI_Uint64*)((char*)self->data + offset));
    self->offset = offset + (MI_Uint32)sizeof(*x);
    return MI_RESULT_OK;
}

MI_Result Buf_UnpackU8A(
    Buf* self,
    const MI_Uint8** data,
    MI_Uint32* size);

MI_Result Buf_UnpackU16A(
    Buf* self,
    const MI_Uint16** data,
    MI_Uint32* size);

MI_Result Buf_UnpackU32A(
    Buf* self,
    const MI_Uint32** data,
    MI_Uint32* size);

MI_Result Buf_UnpackU64A(
    Buf* self,
    const MI_Uint64** data,
    MI_Uint32* size);

MI_Result Buf_UnpackStr(
    Buf* self,
    const MI_Char** x);

MI_Result Buf_PackStrA(
    Buf* self,
    const MI_Char** data,
    MI_Uint32 size);

MI_Result Buf_UnpackStrA(
    Buf* self,
    const MI_Char*** data,
    MI_Uint32* size);

MI_Result Buf_PackDT(
    Buf* self,
    const MI_Datetime* x);

MI_Result Buf_UnpackDT(
    Buf* self,
    MI_Datetime* x);

MI_Result Buf_PackDTA(
    Buf* self,
    const MI_Datetime* data,
    MI_Uint32 size);

MI_Result Buf_UnpackDTA(
    Buf* self,
    const MI_Datetime** dataPtr,
    MI_Uint32* sizePtr);

END_EXTERNC

#endif /* _osc_buf_h */
