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

MString::MString(MString &&s) : str(s.str), lenght(s.lenght)
{
    s.str = nullptr;
    s.lenght = 0;
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

char *MString::Copy(char *dest, const char *source)
{
    return strcpy(dest, source);
}

char *MString::nCopy(char *dest, const char *source, i64 length)
{
    return strncpy(dest, source, length);
}

MString &MString::Trim()
{
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str) {
        char* p = str;
        while (*p) {
            p++;
        }
        while (isspace((unsigned char)*(--p)))
            ;

        p[1] = '\0';
    }

    return str;
}

void MString::Mid(char *dest, const char *source, i32 start, i32 length)
{
    if (length == 0) {
        return;
    }
    u64 srcLength = strlen(source);
    if (start >= srcLength) {
        dest[0] = 0;
        return;
    }
    if (length > 0) {
        for (u64 i = start, j = 0; j < length && source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + length] = 0;
    } else {
        // Если передано отрицательное значение, перейдите к концу строки.
        u64 j = 0;
        for (u64 i = start; source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + j] = 0;
    }
}

i32 MString::IndexOf(char *str, char c)
{
    if (!str) {
        return -1;
    }
    u32 length = strlen(str);
    if (length > 0) {
        for (u32 i = 0; i < length; ++i) {
            if (str[i] == c) {
                return i;
            }
        }
    }

    return -1;
}

bool MString::ToVector4D(char *str, Vector4D *OutVector)
{
    if (!str) {
        return false;
    }

    MMemory::ZeroMem(OutVector, sizeof(Vector4D));
    i32 result = sscanf(str, "%f %f %f %f", &OutVector->x, &OutVector->y, &OutVector->z, &OutVector->w);
    return result != -1;
}

bool MString::ToVector3D(char *str, Vector3D *OutVector)
{
    if (!str) {
        return false;
    }

    MMemory::ZeroMem(OutVector, sizeof(Vector3D));
    i32 result = sscanf(str, "%f %f %f", &OutVector->x, &OutVector->y, &OutVector->z);
    return result != -1;
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
