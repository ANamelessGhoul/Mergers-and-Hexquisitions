#pragma once
#ifndef MSTRING_H
#define MSTRING_H

#include <alloca.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef PATH_SEPARATOR
    #if defined(_WIN32)
        #define PATH_SEPARATOR "\\"
    #else
        #define PATH_SEPARATOR "/"
    #endif 
#endif // !PATH_SEPARATOR


#define StrMakeOnStack(n) StrMakeStatic(alloca(n), n)
#define StrInvalidIndex (size_t)(-1)

#define STR_BIT(n) (1 << n)
enum StringFlags{
    STR_FLAG_ERROR  = STR_BIT(0),
    STR_FLAG_STATIC = STR_BIT(1), // NOTE: We're using static to mean it doesn't need to be freed, and it's okay if we lose the pointer
    STR_FLAG_CONST  = STR_BIT(2),
    STR_FLAG_FIXED_SIZE = STR_BIT(3),
    STR_FLAG_WINDOW = STR_FLAG_STATIC | STR_FLAG_CONST | STR_FLAG_FIXED_SIZE,
};

enum StrError{
    STR_ERR_SUCCESS,
    STR_ERR_CANT_ALLOC,
    STR_ERR_OUT_OF_SPACE,
    STR_ERR_CANT_MODIFY_CONST,
    STR_ERR_CANT_FREE_STATIC,
    STR_ERR_CANT_REALLOC_FIXED_SIZE,
};
typedef enum StrError StrError;

struct String{
    char* buffer;
    size_t capacity;
    size_t length;
    int flags;
};
typedef struct String String;


StrError StrGetLastError();
const char* StrGetErrorCstr();


void StrFree(String* str);

String Str(const char* cstr);
String StrMakeStatic(char* buffer, size_t capacity);
String StrMakeWindow(String self, size_t start, size_t length);

bool StrIsStatic(String self);
bool StrIsConst(String self);
bool StrHasError(String self);

bool StrEqual(String self, String other);
int StrCompare(String self, String other);

void StrAppend(String* self, String other);
void StrAppendNull(String* self);
void StrClear(String* self);
String StrCopy(String self);

const char* StrTempCstr(String self);

size_t StrFind(String self, String other);
size_t StrFindFrom(String self, String other, size_t from);
bool StrBeginsWith(String self, String target);
bool StrEndsWith(String self, String target);

void StrPathAppend(String* self, String other);
String StrFileExtension(String self);
String StrFileName(String self);

#endif // MSTRING_H
