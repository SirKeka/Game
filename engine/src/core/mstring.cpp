#include "mstring.hpp"
#include "core/mmemory.hpp"

#include <string.h>

MString::MString()
{
    str = nullptr;
}

MString::MString(const char *s)
{
    u64 lenght = Length(s) + 1;
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

    u64 lenght = Length(s.str) + 1;
    this->str = reinterpret_cast<char*>(MMemory::Allocate(lenght, MEMORY_TAG_STRING));
    MMemory::CopyMemory(this->str, s.str, lenght);
    
    return *this;
}

MString &MString::operator=(const char *s)
{
    if (this->str != nullptr) Destroy();

    u64 lenght = Length(s) + 1;
    this->str = reinterpret_cast<char*>(MMemory::Allocate(lenght, MEMORY_TAG_STRING));
    MMemory::CopyMemory(this->str, s, lenght);
    
    return *this;
}

u64 MString::Length(const char *s)
{
    return strlen(str);
}

void MString::Destroy()
{
    u64 lenght = strlen(str);
    MMemory::Free(reinterpret_cast<void*>(str), lenght, MEMORY_TAG_STRING);
}
