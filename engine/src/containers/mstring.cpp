#include "mstring.hpp"
#include "core/mmemory.hpp"

#include <string>

MString::MString() : str(nullptr), lenght(0){   }

MString::MString(const char *s)
{
    lenght = strlen(s) + 1;
    this->str = reinterpret_cast<char*>(MMemory::Allocate(lenght, MEMORY_TAG_STRING));
    MMemory::CopyMemory(this->str, s, lenght);
}

MString::~MString()
{
    Destroy();
}

MString &MString::operator=(const MString &s)
{
    if (this->str != nullptr) Destroy();

    lenght = strlen(s.str) + 1;
    this->str = reinterpret_cast<char*>(MMemory::Allocate(lenght, MEMORY_TAG_STRING));
    MMemory::CopyMemory(this->str, s.str, lenght);
    
    return *this;
}

MString &MString::operator=(const char *s)
{
    if (this->str != nullptr) Destroy();

    lenght = strlen(s) + 1;
    this->str = reinterpret_cast<char*>(MMemory::Allocate(lenght, MEMORY_TAG_STRING));
    MMemory::CopyMemory(this->str, s, lenght);
    
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

u64 MString::Length(const char *s)
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

bool operator==(const MString &lhs, const MString &rhs)
{
    return false;
}
