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
    MAPI bool operator== (const char* rhs);
    //MAPI bool operator== (const string& lhs, const char*   rhs);

    /// @param str константная строка
    /// @return длину(количество символов в строке)
    MAPI u64 Length();
    //MAPI char* Copy(const char* s);

    /// @return строку типа си
    MAPI const char* c_str() const noexcept;

private:
    void Destroy();

};

// Сравнение строк с учетом регистра. True, если совпадает, в противном случае false.
MAPI bool StringsEqual(const char* str0, const char* str1);
// Определение длины строк
MAPI u64 StringLenght(const char* str);
