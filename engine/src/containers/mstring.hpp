#pragma once

#include "defines.hpp"


class MString
{
private:
    char* str;

    u64 lenght;

public:
    MAPI MString();
    MAPI MString(const char* s);
    ~MString();

    MAPI MString& operator= (const MString& s);
    MAPI MString& operator= (const char* s);
    //MAPI MString& operator= (char c);
    //MAPI MString& operator= (MString&& s) noexcept;

    MAPI bool operator== (const MString& rhs);
    //MAPI bool operator== (const char*   lhs, const string& rhs);
    //MAPI bool operator== (const string& lhs, const char*   rhs);

    /// @param str константная строка
    /// @return длину(количество символов в строке)
    MAPI u64 Length(const char* s);
    //MAPI char* Copy(const char* s);

    /// @return строку типа си
    MAPI const char* c_str() const noexcept;

private:
    void Destroy();

};

bool StringsEqual(const char* strL, const char* strR);

// Выполняет форматирование строки для определения заданной строки формата и параметров.
MAPI i32 StringFormat(char* dest, const char* format, ...);

/// @brief Выполняет форматирование переменной строки для определения заданной строки формата и va_list.
/// @param dest определяет место назначения для отформатированной строки.
/// @param format отформатируйте строку, которая должна быть отформатирована.
/// @param va_list cписок переменных аргументов.
/// @return размер записываемых данных.
MAPI i32 StringFormatV(char* dest, const char* format, char* va_listp);