#include "stringlib.h"
#include <string.h>

#ifndef thread_local
    #ifdef __GNUC__
        #define thread_local __thread
    #elif __STDC_VERSION__ >= 201112L
        #define thread_local _Thread_local
    #elif defined(_MSC_VER)
        #define thread_local __declspec(thread)
    #else
        #warning Could not determine your compilers version of thread_local expect stringlib errors to cause problems
        #define thread_local
    #endif
#endif // !thread_local

// NOTE: STR_REALLOC must free when n is 0
#ifndef STR_REALLOC
    #include <stdlib.h>
    #define STR_REALLOC(ptr, n) realloc((ptr), (n))
#endif // !STR_REALLOC

#ifndef STR_INITIAL_CAPACITY
    #define STR_INITIAL_CAPACITY 32
#endif // !STR_INITIAL_CAPACITY

#ifndef STR_SCRATCH_CAPACITY
    #define STR_SCRATCH_CAPACITY (1024*8)
#endif // !STR_SCRATCH_CAPACITY


#define STR_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define STR_MAX(a, b) ((a) >= (b) ? (a) : (b))

static thread_local struct {
    StrError error;
    char scratch[STR_SCRATCH_CAPACITY];
    size_t scratch_index;

} STRLIB = {0};



// Internal functions
size_t CStrlen(const char* str);
void* CMemcpy(void *restrict dest, const void *restrict src, size_t n);
StrError StrExpand(String* str);
StrError StrExpandTo(String* str, size_t capacity);

StrError StrGetLastError()
{
    return STRLIB.error;
}

const char* StrGetErrorCstr()
{
    switch (STRLIB.error) {
        case STR_ERR_SUCCESS:
            return "No Error";
        case STR_ERR_CANT_ALLOC:
            return "Could not allocate memory";
        case STR_ERR_OUT_OF_SPACE:
            return "String out of capacity";
        case STR_ERR_CANT_MODIFY_CONST:
            return "Cannot modify const string";
        case STR_ERR_CANT_FREE_STATIC:
            return "Cannot free static string";
        case STR_ERR_CANT_REALLOC_FIXED_SIZE:
            return "Cannot free static string";
    }
    return "UNKNOWN ERROR";
}


void StrFree(String* str)
{
    if (str->flags & STR_FLAG_STATIC)
    {
        STRLIB.error = STR_ERR_CANT_FREE_STATIC;
        return;
    }

    str->buffer = STR_REALLOC(str->buffer, 0);
    str->buffer = NULL;
    str->capacity = 0;
    str->length = 0;
    str->flags = 0;
}


String Str(const char* cstr)
{
    size_t length = CStrlen(cstr);
    return (String) {
        .buffer = (char*)cstr, // NOTE: We're recording that this is const in the flags and will not write to it
        .capacity = length + 1,
        .length = length,
        .flags = STR_FLAG_STATIC | STR_FLAG_CONST,
    };
}

String StrMakeStatic(char* buffer, size_t capacity)
{
    return (String) {
        .buffer = buffer,
        .capacity = capacity,
        .flags = STR_FLAG_STATIC,
    };
}

String StrMakeWindow(String self, size_t start, size_t length)
{
    if (start >= self.length)
    {
        return (String){ .flags = STR_FLAG_WINDOW };
    }

    length = STR_MIN(length, self.length - start);

    return (String) {
        .buffer = self.buffer + start,
        .length = length,
        .capacity = length,
        .flags = STR_FLAG_WINDOW,
    };
}


bool StrIsStatic(String self)
{
    return self.flags & STR_FLAG_STATIC;
}

bool StrIsConst(String self)
{
    return self.flags & STR_FLAG_CONST;
}

bool StrHasError(String self)
{
    return self.flags & STR_FLAG_ERROR;
}

bool StrEqual(String self, String other)
{
    if (self.length != other.length)
    {
        return false;
    }
    
    return StrCompare(self, other) == 0;
}


int StrCompare(String self, String other)
{
    return memcmp(self.buffer, other.buffer, STR_MIN(self.length, other.length));
}


void StrAppend(String* self, String other)
{
    if (self->flags & STR_FLAG_CONST)
    {
        STRLIB.error = STR_ERR_CANT_MODIFY_CONST;
        return;
    }

    size_t required_capacity = self->length + other.length;
    if (required_capacity > self->capacity)
    {
        if (self->flags & STR_FLAG_STATIC)
        {
            self->flags |= STR_FLAG_ERROR;
            STRLIB.error = STR_ERR_OUT_OF_SPACE;
            return;
        }
        
        StrError error = StrExpandTo(self, required_capacity);
        if (error != STR_ERR_SUCCESS)
        {
            self->flags |= STR_FLAG_ERROR;
            STRLIB.error = error;
            return;
        }
    }

    CMemcpy(self->buffer + self->length, other.buffer, other.length);
    self->length += other.length;
}

void StrAppendNull(String* self)
{
    if (self->flags & STR_FLAG_CONST)
    {
        self->flags |= STR_FLAG_ERROR;
        STRLIB.error = STR_ERR_CANT_MODIFY_CONST;
        return;
    }

    if (self->length + 1 >= self->capacity)
    {
        if (self->flags & STR_FLAG_STATIC)
        {
            self->flags |= STR_FLAG_ERROR;
            STRLIB.error = STR_ERR_OUT_OF_SPACE;
            return;
        }
        
        StrError error = StrExpandTo(self, self->length + 1);
        if (error != STR_ERR_SUCCESS)
        {
            self->flags |= STR_FLAG_ERROR;
            STRLIB.error = error;
            return;
        }
    }
    
    // NOTE: We are not incrementing length here as we consider the null terminator not part of the string
    self->buffer[self->length] = 0;
}


void StrClear(String* self)
{
    self->length = 0;
}

String StrCopy(String self)
{
    String str = {0};
    StrError error = StrExpandTo(&str, self.length);
    if (error != STR_ERR_SUCCESS)
    {
        return (String){ .flags = STR_FLAG_ERROR | STR_FLAG_CONST | STR_FLAG_STATIC };
    }

    CMemcpy(str.buffer, self.buffer, self.length);
    return str;
}

const char* StrTempCstr(String self)
{
    if (STRLIB.scratch_index + self.length >= STR_SCRATCH_CAPACITY)
    {
        STRLIB.scratch_index = 0;
    }

    char* destination = STRLIB.scratch + STRLIB.scratch_index;
    CMemcpy(destination, self.buffer, self.length);
    STRLIB.scratch_index += self.length;
    STRLIB.scratch[STRLIB.scratch_index] = 0;
    STRLIB.scratch_index++;

    return destination;
}

size_t StrFind(String self, String other)
{
    return StrFindFrom(self, other, 0);
}

size_t StrFindFrom(String self, String other, size_t from)
{
    void* found = memmem(self.buffer + from, self.length - from, other.buffer, other.length);
    if (found == NULL)
    {
        return StrInvalidIndex;
    }
    return (char*)found - self.buffer;
}


size_t StrRFind(String self, String other)
{
    if (other.length == 0)
    {
        return StrInvalidIndex;
    }

    size_t n = self.length - other.length;
    char c = other.buffer[0];
    while (n--)
    {
        // Check one character at a time for the beginning of the "needle"
        if (self.buffer[n]==c)
        {
            // Check the entire "needle" only if found the first character
            if (other.length == 1 || memcmp(self.buffer + n, other.buffer, other.length) == 0)
            {
                return n;
            }
        }
    }

    return StrInvalidIndex;
}

bool StrBeginsWith(String self, String target)
{
    for (size_t i = 0; i < target.length; i++)
    {
        if (i >= self.length || self.buffer[i] != target.buffer[i])
        {
            return false;
        }
    }
    
    return true;
}

bool StrEndsWith(String self, String target)
{
    for (size_t i = 0; i < target.length; i++)
    {
        size_t target_idx = target.length - 1 - i;
        size_t self_idx   =   self.length - 1 - i;

        if (self_idx >= self.length || self.buffer[self_idx] != target.buffer[target_idx])
        {
            return false;
        }
    }

    return true;
}


void StrPathAppend(String* self, String other)
{
    if (other.length == 0)
    {
        return;
    }

    String delimiter = Str(PATH_SEPARATOR);
    bool needs_delimiter = self->length > 0 && !StrEndsWith(*self, delimiter);

    if (StrBeginsWith(other, delimiter) && self->length > 0)
    {
        other = StrMakeWindow(other, delimiter.length, other.length - delimiter.length);
    }

    size_t delimiter_length = needs_delimiter ? delimiter.length : 0;

    size_t required_capacity = self->length + other.length + delimiter_length;

    if (required_capacity > self->capacity)
    {
        if (self->flags & STR_FLAG_STATIC)
        {
            self->flags |= STR_FLAG_ERROR;
            STRLIB.error = STR_ERR_OUT_OF_SPACE;
            return;
        }
        
        StrError error = StrExpandTo(self, required_capacity);
        if (error != STR_ERR_SUCCESS)
        {
            self->flags |= STR_FLAG_ERROR;
            STRLIB.error = error;
            return;
        }
    }


    if (needs_delimiter)
    {
        StrAppend(self, delimiter);
    }
    StrAppend(self, other);
}

String StrFileExtension(String self)
{
    size_t extension_begin = StrRFind(self, Str("."));
    if (extension_begin == StrInvalidIndex)
    {
        return (String){ .flags = STR_FLAG_WINDOW };
    }

    return StrMakeWindow(self, extension_begin, self.length - extension_begin);
}

String StrFileName(String self)
{
    size_t filename_begin = StrRFind(self, Str(PATH_SEPARATOR));
    if (filename_begin == StrInvalidIndex)
    {
        return (String){ .flags = STR_FLAG_WINDOW };
    }

    return StrMakeWindow(self, filename_begin, self.length - filename_begin);
}


// Helpers

size_t CStrlen(const char* str)
{
    const char* begin = str;
    for (; *str; str++);
	return str-begin;
}

void* CMemcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = dest;
	const unsigned char *s = src;
	for (; n; n--) *d++ = *s++;
    return dest;
}


StrError StrExpand(String* str)
{
    return StrExpandTo(str, 0);
}

StrError StrExpandTo(String* str, size_t capacity)
{
    if (str->capacity >= capacity)
    {
        return STR_ERR_SUCCESS;
    }

    if (str->flags & STR_FLAG_FIXED_SIZE)
    {
        return STR_ERR_CANT_REALLOC_FIXED_SIZE;
    }

    size_t new_capacity = str->capacity;
    while(new_capacity < capacity)
    {
        new_capacity = (new_capacity == 0) ? (STR_INITIAL_CAPACITY) : (new_capacity * 2);
    }


    char* old_buffer = (str->flags & STR_FLAG_STATIC) ? NULL : str->buffer;
    char* new_buffer = realloc(old_buffer, new_capacity);
    if (new_buffer == NULL)
    {
        return STR_ERR_CANT_ALLOC;
    }

    str->buffer = new_buffer;
    str->capacity = new_capacity;
    // NOTE: If we managed to allocate, then we replaced our static buffer
    str->flags &= ~STR_FLAG_STATIC; 
    return STR_ERR_SUCCESS;
}
