#pragma once

#include "containers/darray.h"

enum class jcf_token_type {
    Unknown,
    Whitespace,
    Comment,
    Identifier,
    OperatorEqual,
    OperatorMinus,
    OperatorPlus,
    OperatorSlash,
    OperatorAsterisk,
    OperatorDot,
    StringLiteral,
    NumericLiteral,
    Boolean,
    CurlyBraceOpen,
    CurlyBraceClose,
    BracketOpen,
    BracketClose,
    Newline,
    EOF
};

struct jcf_token 
{
    jcf_token_type type;
    u32 start;
    u32 end;
#ifdef MOON_DEBUG
    const char* content;
#endif
};

struct jcf_parser 
{
    const char* FileContent;
    u32 position;

    // darray
    DArray<jcf_token> tokens;
}

enum class jcf_property_type 
{
    // ЗАДАЧА: Хотим ли мы поддерживать типы undefined/null? Если да, то выберите один из них и используйте только его — не нужно определять оба.
    UNKNOWN,
    INT,
    FLOAT,
    STRING,
    OBJECT,
    ARRAY,
    BOOLEAN,
}

enum class jcf_object_type 
{
    OBJECT,
    ARRAY
}