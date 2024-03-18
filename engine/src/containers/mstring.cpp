#include "mstring.hpp"
#include "core/mmemory.hpp"

#include <string>
#include <stdarg.h>

MString::MString() : str(nullptr), lenght(0){   }

MString::MString(const char *s)
{
    lenght = strlen(s) + 1;
    this->str = MMemory::TAllocate<char>(lenght, MEMORY_TAG_STRING);
    MMemory::CopyMem(this->str, s, lenght);
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
    
    return *this;
}

MString &MString::operator=(const char *s)
{
    if (this->str != nullptr) Destroy();

    lenght = strlen(s) + 1;
    this->str = MMemory::TAllocate<char>(lenght, MEMORY_TAG_STRING);
    MMemory::CopyMem(this->str, s, lenght);
    
    return *this;
}

MString::operator bool() const
{
    if (this->str != nullptr && this->lenght != 0) {
        return true;
    }
    return false;
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
    MMemory::TFree<char>(str, lenght, MEMORY_TAG_STRING);
    lenght = 0;
}

bool StringsEqual(const char *strL, const char *strR)
{
    return strcmp(strL, strR);
}

bool StringsEquali(const char *str0, const char *str1)
{
#if defined(__GNUC__)
    return strcasecmp(str0, str1) == 0;
#elif (defined _MSC_VER)
    return _strcmpi(str0, str1) == 0;
#endif
}

MAPI i32 StringFormat(char *dest, const char *format, ...)
{
    if (dest) {
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, format);
        i32 written = StringFormatV(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }
    return -1;
}

MAPI i32 StringFormatV(char *dest, const char *format, char* va_list)
{
    if (dest) {
        // Большой, но может поместиться в стопке.
        char buffer[32000];
        i32 written = vsnprintf(buffer, 32000, format, va_list);
        buffer[written] = 0;
        MMemory::CopyMem(dest, buffer, written + 1);

        return written;
    }
    return -1;
}
