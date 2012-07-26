#include <scxcorelib/nw/base/buf.h>
#include <scxcorelib/nw/zlen.h>
#include <strings.h>

#define _MIN_CAPACITY 256

/*
**==============================================================================
**
** Local definitions
**
**==============================================================================
*/

/* Round x up to the nearest power of 2 */
static MI_Uint32 _RoundPow2(MI_Uint32 x)
{
    MI_Uint32 r = x - 1;
    r |= (r >> 1);
    r |= (r >> 2);
    r |= (r >> 4);
    r |= (r >> 8);
    r |= (r >> 16);
    return r + 1;
}

/*
**==============================================================================
**
** Public definitions
**
**==============================================================================
*/

MI_Result Buf_Init(
    Buf* self,
    MI_Uint32 capacity)
{
    Page* page;

    /* Adjust capacity if too small */
    if (capacity < _MIN_CAPACITY)
        capacity = _MIN_CAPACITY;

    /* Allocate data buffer */
    page = (Page*)malloc(sizeof(Page) + capacity);

    if (!page)
        return MI_RESULT_FAILED;

    // Always zero your memory kids.
    bzero(page, sizeof(Page) + capacity);

    page->u.s.size = capacity;
    page->u.s.next = 0;

    /* Set fields */
    self->data = page + 1;
    self->size = 0;
    self->capacity = capacity;
    self->offset = 0;

#ifdef CONFIG_ENABLE_DEBUG
    memset(self->data,0xAA,self->capacity);
#endif

    return MI_RESULT_OK;
}

void Buf_Destroy(
    Buf* self)
{
    // JWF -- 4/13/2011 (Friday the 13th is on a Tuesday this month)
    // This is code that was here when I got here and really was disturbing when I put in a NULL.  
    // I do not fully understand what he's doing here myself, but I believe he's freeing not only 
    // the target memory, but the target memory length, which is in the struct right before the 
    // data pointer.  In fact, they were, I beleive, both created from the same malloc and assignment 
    // when the buffer was set.  I am not absolutely sure, though.  I do know that messing with 
    // this code breaks things badly.
    if (self->data)
    {
        free((Page*)self->data - 1);
    }
}

MI_Result Buf_Reserve(
    Buf* self, 
    MI_Uint32 capacity)
{
    /* Expand allocation if we need more space */
    if (capacity > self->capacity)
    {
        Page* page;

        capacity = _RoundPow2(capacity);

        if (self->data)
        {
            page = (Page*)self->data - 1;
            page = (Page*)realloc(page, sizeof(Page) + capacity);

            // Zero the new memory
            memset(((char*)(page+1)) + self->capacity, 0, capacity - self->capacity);

#ifdef CONFIG_ENABLE_DEBUG
            memset(((char*)(page+1)) + self->capacity,0xAA,capacity - self->capacity);
#endif
        }
        else
        {
            page = (Page*)malloc(sizeof(Page) + capacity);
            bzero(page, sizeof(Page) + capacity);

#ifdef CONFIG_ENABLE_DEBUG
            memset(page,0xAA,sizeof(Page) + capacity);
#endif
        }

        if (!page)
            return MI_RESULT_FAILED;

        page->u.s.size = capacity;
        self->data = page + 1;
        self->capacity = capacity;
    }

    return MI_RESULT_OK;
}

MI_Result Buf_App(
    Buf* self, 
    const void* data, 
    MI_Uint32 size)
{
    /* Calculate the new size */
    MI_Uint32 newSize = self->size + size;

    /* Expand allocation if we need more space */
    if (newSize > self->capacity)
    {
        MI_Result r = Buf_Reserve(self, newSize);

        if (r != MI_RESULT_OK)
            return MI_RESULT_FAILED;
    }

    /* Copy in the new data */
    memcpy((char*)self->data + self->size, data, size);
    self->size += size;

    return MI_RESULT_OK;
}

MI_Result Buf_PackStr(
    Buf* self,
    const MI_Char* x)
{
    MI_Uint32 size;

    /* Pack null strings as 0 */
    if (!x)
        return Buf_PackU32(self, 0);
    
    /* Pack the size of the string (size including null terminator) */
    size = (MI_Uint32)Zlen(x) + 1;
    MI_RETURN_ERR(Buf_PackU32(self, size));

    /* Pack the characters (including the null terminator) */
    MI_RETURN_ERR(Buf_App(self, x, size * sizeof(MI_Char)));

    return MI_RESULT_OK;
}

MI_Result Buf_UnpackU8A(
    Buf* self,
    const MI_Uint8** data,
    MI_Uint32* size)
{
    /* Unpack size */
    MI_RETURN_ERR(Buf_UnpackU32(self, size));

    if (*size == 0)
    {
        *data = NULL;
        return MI_RESULT_OK;
    }
    
    /* Check whether there are enough bytes left */
    if (self->offset + *size * sizeof(MI_Uint8) > self->size)
        return MI_RESULT_FAILED;

    /* Store pointer to array */
    *data = (const MI_Uint8*)((char*)self->data + self->offset);
    self->offset += *size * sizeof(MI_Uint8);

    return MI_RESULT_OK;
}

MI_Result Buf_UnpackU16A(
    Buf* self,
    const MI_Uint16** data,
    MI_Uint32* size)
{
    /* Unpack size */
    MI_RETURN_ERR(Buf_UnpackU32(self, size));

    if (*size == 0)
    {
        *data = NULL;
        return MI_RESULT_OK;
    }
    
    /* Check whether there are enough bytes left */
    if (self->offset + *size * sizeof(MI_Uint16) > self->size)
        return MI_RESULT_FAILED;

    /* Store pointer to array */
    *data = (const MI_Uint16*)((char*)self->data + self->offset);
    self->offset += *size * sizeof(MI_Uint16);

    return MI_RESULT_OK;
}

MI_Result Buf_UnpackU32A(
    Buf* self,
    const MI_Uint32** data,
    MI_Uint32* size)
{
    /* Unpack size */
    MI_RETURN_ERR(Buf_UnpackU32(self, size));

    if (*size == 0)
    {
        *data = NULL;
        return MI_RESULT_OK;
    }
    
    /* Check whether there are enough bytes left */
    if (self->offset + *size * sizeof(MI_Uint32) > self->size)
        return MI_RESULT_FAILED;

    /* Store pointer to array */
    *data = (const MI_Uint32*)((char*)self->data + self->offset);
    self->offset += *size * sizeof(MI_Uint32);

    return MI_RESULT_OK;
}

MI_Result Buf_UnpackU64A(
    Buf* self,
    const MI_Uint64** data,
    MI_Uint32* size)
{
    /* Unpack size */
    MI_RETURN_ERR(Buf_UnpackU32(self, size));

    if (*size == 0)
    {
        *data = NULL;
        return MI_RESULT_OK;
    }

    /* Align buffer on 8 byte boundary */
    MI_RETURN_ERR(Buf_Align64(self));
    
    /* Check whether there are enough bytes left */
    if (self->offset + *size * sizeof(MI_Uint64) > self->size)
        return MI_RESULT_FAILED;

    /* Store pointer to array */
    *data = (const MI_Uint64*)((char*)self->data + self->offset);
    self->offset += *size * sizeof(MI_Uint64);

    return MI_RESULT_OK;
}

MI_Result Buf_UnpackStr(
    Buf* self,
    const MI_Char** x)
{
    MI_Uint32 size;

    /* Unpack size */
    MI_RETURN_ERR(Buf_UnpackU32(self, &size));

    if (size == 0)
    {
        *x = NULL;
        return MI_RESULT_OK;
    }
    
    /* Check whether there are enough bytes left */
    if (self->offset + size * sizeof(MI_Char) > self->size)
        return MI_RESULT_FAILED;

    /* Store pointer to array */
    *x = (const MI_Char*)((char*)self->data + self->offset);
    self->offset += size * sizeof(MI_Char);

    return MI_RESULT_OK;
}

MI_Result Buf_PackStrA(
    Buf* self,
    const MI_Char** data,
    MI_Uint32 size)
{
    MI_Uint32 i;

    /* Pack the array size (the number of strings) */
    MI_RETURN_ERR(Buf_PackU32(self, size));

    if (size)
    {
        MI_Uint32 sizes[64];

        if (!data)
            return MI_RESULT_FAILED;

        /* Put sizes of all strings first. Each size is encoded as a 64-bit
         * integer. The unpack function replaces this integer with a pointer
         * to the corresponding string. This avoids having to allocate memory
         * for the pointer array while unpacking.
         */
        for (i = 0; i < size; i++)
        {
            MI_Uint32 n;

            if (!data[i])
                return MI_RESULT_FAILED;

            n = (MI_Uint32)Zlen(data[i]) + 1;

            /* Save size so that it will not have to be recalculated by the
             * next loop using strlen.
             */
            if (i < MI_COUNT(sizes))
                sizes[i] = n;

            MI_RETURN_ERR(Buf_PackU64(self, (MI_Uint64)n));
        }

        /* Pack strings one after the other. */
        for (i = 0; i < size; i++)
        {
            MI_Uint32 n;
            
            if (i < MI_COUNT(sizes))
                n = sizes[i];
            else 
                n = (MI_Uint32)Zlen(data[i]) + 1;

            MI_RETURN_ERR(Buf_App(self, data[i], n * sizeof(MI_Char)));
        }
    }

    return MI_RESULT_OK;
}

MI_Result Buf_UnpackStrA(
    Buf* self,
    const MI_Char*** dataOut,
    MI_Uint32* sizeOut)
{
    const MI_Char** data;
    MI_Uint32 size;
    MI_Uint32 i;
    MI_Uint32 offset;

    /* Unpack the size of the array */
    MI_RETURN_ERR(Buf_UnpackU32(self, &size));

    /* Handle zero-size array case */
    if (size == 0)
    {
        *dataOut = NULL;
        *sizeOut = 0;
        return MI_RESULT_OK;
    }

    /* Align to read uint64 sizes */
    MI_RETURN_ERR(Buf_Align64(self));

    /* Set pointer data array */
    data = (const MI_Char**)((char*)self->data + self->offset);

    /* Calculate offset to first string in array (data[0]) */
    offset = self->offset + (size * sizeof(MI_Uint64));

    /* Fail if offset is beyond end of buffer */
    if (offset > self->size)
        return MI_RESULT_FAILED;

    /* Unpack the string sizes and covert to string pointers */
    for (i = 0; i < size; i++)
    {
        MI_Uint64 tmp;
        MI_Uint32 n;

        /* Unpack size of next string in array */
        MI_RETURN_ERR(Buf_UnpackU64(self, &tmp));
        n = (MI_Uint32)tmp;

        /* Fail if not enough room left in buffer for string */
        if (offset + n * sizeof(MI_Char) > self->size)
            return MI_RESULT_FAILED;

        /* Add string to array */
        data[i] = (MI_Char*)((char*)self->data + offset);
        offset += n * sizeof(MI_Char);
    }

    /* Update the offset */
    self->offset = offset;

    /* Set the output parameters */
    *dataOut = data;
    *sizeOut = size;

    return MI_RESULT_OK;
}

MI_Result Buf_PackDT(
    Buf* self,
    const MI_Datetime* x)
{
    MI_RETURN_ERR(Buf_Pad32(self));
    MI_RETURN_ERR(Buf_App(self, x, sizeof(MI_Datetime)));
    return MI_RESULT_OK;
}

MI_Result Buf_UnpackDT(
    Buf* self,
    MI_Datetime* x)
{
    MI_Uint32 offset;

    MI_RETURN_ERR(Buf_Align32(self));

    /* Find ending offset of datetime structure */
    offset = self->offset + sizeof(MI_Datetime);

    if (offset > self->size)
        return MI_RESULT_FAILED;

    memcpy(x, (char*)self->data + self->offset, sizeof(MI_Datetime));
    self->offset = offset;

    return MI_RESULT_OK;
}

MI_Result Buf_PackDTA(
    Buf* self,
    const MI_Datetime* data,
    MI_Uint32 size)
{
    MI_RETURN_ERR(Buf_PackU32(self, size));
    MI_RETURN_ERR(Buf_App(self, data, size * sizeof(MI_Datetime)));
    return MI_RESULT_OK;
}

MI_Result Buf_UnpackDTA(
    Buf* self,
    const MI_Datetime** dataPtr,
    MI_Uint32* sizePtr)
{
    MI_Uint32 offset;

    /* Unpack the size */
    MI_RETURN_ERR(Buf_UnpackU32(self, sizePtr));

    /* Handle zero-sized array (null data pointer) */
    if (*sizePtr == 0)
    {
        *dataPtr = NULL;
        return MI_RESULT_OK;
    }

    /* Find ending offset of datetime array */
    offset = self->offset + *sizePtr * sizeof(MI_Datetime);

    /* Set pointer to data array */
    *dataPtr = (const MI_Datetime*)((char*)self->data + self->offset);

    /* Advance offset beyond array */
    self->offset = offset;

    return MI_RESULT_OK;
}

Page* Buf_StealPage(
    Buf* self)
{
    if (self->data)
    {
        Page* page = (Page*)self->data - 1;
        self->data = NULL;
        return page;
    }

    return NULL;
}
