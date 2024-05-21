#include "mstring.hpp"
#include "core/mmemory.hpp"

#include <string>
#include <stdarg.h>

MString::MString() : str(nullptr), lenght() {}

MString::MString(u64 lenght)
{
    this->lenght = lenght + 1;
    this->str = MMemory::TAllocate<char>(lenght, MemoryTag::String);
    this->str[lenght] = '\0';
}

MString::MString(const char *s)
{
    lenght = strlen(s) + 1;
    this->str = MMemory::TAllocate<char>(lenght, MemoryTag::String);
    MMemory::CopyMem(this->str, s, lenght);
}

MString::MString(const MString &s)
{
    lenght = s.lenght;
    this->str = MMemory::TAllocate<char>(lenght, MemoryTag::String);
    MMemory::CopyMem(this->str, s.str, lenght);
}

MString::MString(MString &&s) : str(s.str), lenght(s.lenght)
{
    s.str = nullptr;
    s.lenght = 0;
}

MString::~MString()
{
    if (str) {
        MMemory::Free(str, lenght, MemoryTag::String);
        lenght = 0;
    }
}

MString &MString::operator=(const MString &s)
{
    if (str) {
        Destroy();
    }
    lenght = s.lenght + 1;
    this->str = MMemory::TAllocate<char>(lenght, MemoryTag::String);

    MMemory::CopyMem(this->str, s.str, lenght);
    
    return *this;
}

MString &MString::operator=(const char *s)
{
    if(str) {
        Destroy();
    }
    lenght = strlen(s) + 1;
    str = reinterpret_cast<char*>(MMemory::Allocate(lenght, MemoryTag::String));
    //this->str = MMemory::TAllocate<char>(lenght, MemoryTag::String);

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

u64 MString::Length(const char *s)
{
    return strlen(s);
}

const char *MString::c_str() const noexcept
{
    return str;
}

i32 MString::Format(char *dest, const char *format, ...)
{
    if (dest) {
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, format);
        i32 written = FormatV(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }
    return -1;
}

i32 MString::FormatV(char *dest, const char *format, char *va_list)
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

    return *this;
}

char *MString::Trim(char *s)
{
    while (isspace((unsigned char)*s)) {
        s++;
    }
    if (*s) {
        char* p = s;
        while (*p) {
            p++;
        }
        while (isspace((unsigned char)*(--p)))
            ;

        p[1] = '\0';
    }

    return s;
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

bool MString::ToVector4D(char *s, Vector4D<f32> *OutVector)
{
    if (!s) {
        return false;
    }

    MMemory::ZeroMem(OutVector, sizeof(Vector4D<f32>));
    i32 result = sscanf(s, "%f %f %f %f", &OutVector->x, &OutVector->y, &OutVector->z, &OutVector->w);
    return result != -1;
}

bool MString::ToVector3D(char *str, Vector3D<f32> *OutVector)
{
    if (!str) {
        return false;
    }

    MMemory::ZeroMem(OutVector, sizeof(Vector3D<f32>));
    i32 result = sscanf(str, "%f %f %f", &OutVector->x, &OutVector->y, &OutVector->z);
    return result != -1;
}

bool MString::ToVector2D(char *str, Vector2D<f32> *OutVector)
{
    if (!str) {
        return false;
    }

    MMemory::ZeroMem(OutVector, sizeof(Vector2D<f32>));
    i32 result = sscanf(str, "%f %f", &OutVector->x, &OutVector->y);
    return result != -1;
}

bool MString::ToFloat(char *str, f32 *f)
{
    if (!str) {
        return false;
    }

    *f = 0;
    i32 result = sscanf(str, "%f", f);
    return result != -1;
}

bool MString::ToFloat(char *str, f64 *f)
{
    if (!str) {
        return false;
    }

    *f = 0;
    i32 result = sscanf(str, "%lf", f);
    return result != -1;
}

bool MString::ToInt(char *str, i8 *i)
{
    if (!str) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%hhi", i);
    return result != -1;
}

bool MString::ToInt(char *str, i16 *i)
{
    if (!str) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%hi", i);
    return result != -1;
}

bool MString::ToInt(char *str, i32 *i)
{
    if (!str) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%i", i);
    return result != -1;
}

bool MString::ToInt(char *str, i64 *i)
{
    if (!str) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%lli", i);
    return result != -1;
}

bool MString::ToUInt(char *str, u8 *u)
{
    if (!str) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%hhu", u);
    return result != -1;
}

bool MString::ToUInt(char *str, u16 *u)
{
    if (!str) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%hu", u);
    return result != -1;
}

bool MString::ToUInt(char *str, u32 *u)
{
    if (!str) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%u", u);
    return result != -1;
}

bool MString::ToUInt(char *str, u64 *u)
{
    if (!str) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%llu", u);
    return result != -1;
}

bool MString::ToBool(char *str, bool *b)
{
    if (!str) {
        return false;
    }

    return MString::Equal(str, "1") || MString::Equali(str, "true");
}

void MString::Destroy()
{
    this->~MString();
}

bool MString::Equal(const char *strL, const char *strR)
{
    return strcmp(strL, strR);
}

bool MString::Equali(const char *str0, const char *str1)
{
#if defined(__GNUC__)
    return strcasecmp(str0, str1) == 0;
#elif (defined _MSC_VER)
    return _strcmpi(str0, str1) == 0;
#endif
}
