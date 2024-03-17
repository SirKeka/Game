#pragma once

#include "defines.hpp"

class MAPI MString
{
private:
    char* str;

    u64 lenght;

public:
    MString();
    MString(const char* s);
    ~MString();

    MString& operator= (const MString& s);
    MString& operator= (const char* s);
    //MString& operator= (char c);
    //MString& operator= (MString&& s) noexcept;

    explicit operator bool() const;

    bool operator== (const MString& rhs);

    /// @param str константная строка
    /// @return длину(количество символов в строке)
    u64 Length();
    //char* Copy(const char* s);

    /// @return строку типа си
    const char* c_str() const noexcept;

private:
    void Destroy();

};

//MAPI bool operator== (const char*   lhs, const string& rhs);
//MAPI bool operator== (const string& lhs, const char*   rhs);

// Сравнение строк с учетом регистра. True, если совпадает, в противном случае false.
MAPI bool StringsEqual(const char* strL, const char* strR);

// Сравнение строк без учета регистра. True, если совпадает, в противном случае false.
MAPI bool StringsEquali(const char* str0, const char* str1);

// Выполняет форматирование строки для определения заданной строки формата и параметров.
MAPI i32 StringFormat(char* dest, const char* format, ...);

/// @brief Выполняет форматирование переменной строки для определения заданной строки формата и va_list.
/// @param dest определяет место назначения для отформатированной строки.
/// @param format отформатируйте строку, которая должна быть отформатирована.
/// @param va_list cписок переменных аргументов.
/// @return размер записываемых данных.
MAPI i32 StringFormatV(char* dest, const char* format, char* va_list);
