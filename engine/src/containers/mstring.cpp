#include "mstring.hpp"
#include "core/mmemory.hpp"

#include <string>

MString::MString() : str(nullptr), lenght(0){   }

MString::MString(const char *s)
{
    lenght = strlen(s) + 1;
    this->str = MMemory::TAllocate<char>(lenght, MEMORY_TAG_STRING);
    MMemory::CopyMem(this->str, s, lenght);
    str[lenght] = '\0';
}

MString::~MString()
{
    Destroy();
}

MString &MString::operator=(const MString &s)
{
    if (this->str != nullptr) Destroy();

    lenght = strlen(s.str) + 1;
    this->str = MMemory::TAllocate<char>(lenght, MEMORY_TAG_STRING);
    MMemory::CopyMem(this->str, s.str, lenght);
    str[lenght] = '\0';
    
    return *this;
}

MString &MString::operator=(const char *s)
{
    if (this->str != nullptr) Destroy();

    lenght = strlen(s) + 1;
    this->str = MMemory::TAllocate<char>(lenght, MEMORY_TAG_STRING);
    MMemory::CopyMem(this->str, s, lenght);
    str[lenght] = '\0';
    
    return *this;
}

bool MString::operator==(const MString &rhs)
{
    if (lenght != rhs.lenght) {
        return false;
    }
    for (u64 i = 0; i < lenght; i++) {
        if (str[i] != rhs.str[i]) {
            return false;
        }
        
    }
    
    return true;
}

MAPI bool MString::operator==(const char *rhs)
{
    for (u64 i = 0; i < lenght; i++) {
        if (str[i] != rhs[i]) return false;
    }
    
    return true;
}

u64 MString::Length()
{
    return lenght;
}

const char *MString::c_str() const noexcept
{
    return str;
}

void MString::Destroy()
{
    lenght = 0;
    MMemory::Free(reinterpret_cast<void*>(str), lenght, MEMORY_TAG_STRING);
}

bool StringsEqual(const char *str0, const char *str1)
{
    return strcmp(str0, str1) == 0;
}

u64 StringLenght(const char *str)
{
    return strlen(str);
}
