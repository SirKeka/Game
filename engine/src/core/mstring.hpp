#pragma once

#include "defines.hpp"


class MString
{
private:
    char* str;

public:
    MAPI MString();
    MAPI MString(const char* s);
    ~MString();

    MAPI MString& operator= (const MString& s);
    MAPI MString& operator= (const char* s);
    //MAPI MString& operator= (char c);
    //MAPI MString& operator= (MString&& s) noexcept;

    /// @brief 
    /// @param str константная строка
    /// @return длину(количество символов в строке)
    MAPI u64 Length(const char* s);
    //MAPI char* Copy(const char* s);

private:
    void Destroy();

};
