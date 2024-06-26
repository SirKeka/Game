#include "mstring.hpp"
#include "core/mmemory.hpp"
#include "darray.hpp"

#include <string>
#include <stdarg.h>

constexpr MString::MString() : length(), str(nullptr) {}

// constexpr MString::MString(u16 length) : length(length), str(MMemory::TAllocate<char>(length + 1, MemoryTag::String)) {}

constexpr MString::MString(const char *s) : length(Len(s)), str(Copy(s, length)) {}

constexpr MString::MString(const MString &s) : length(s.length), str(Copy(s)) {}

constexpr MString::MString(MString &&s) : length(s.length), str(s.str) 
{
    s.str = nullptr;
    s.length = 0;
}

MString::~MString()
{
    Clear();
}
/*
char MString::operator[](u64 i)
{
    if (!str || i >= lenght) {
        return '\0';
    } else {
        return str[i];
    }
}
*/
const char MString::operator[](u16 i) const
{
    if (!str || i >= length) {
        return '\0';
    } else {
        return str[i];
    }
}

MString &MString::operator=(const MString &s)
{
    if (str && length != s.length) {
        Clear();
    }
    if (length != s.length) {
        length = s.length;
        str = MMemory::TAllocate<char>(MemoryTag::String, length);
    } 
    nCopy(s, length);
    return *this;
}

MString &MString::operator=(const char *s)
{
    u16 slength = 0;
    if (s && *s != '\0') {
        slength = Length(s) + 1;
    }
    if(str && slength != length) {
        Clear();
    }
    length = slength;
    if (!str) {
        str = MMemory::TAllocate<char>(MemoryTag::String, length);
    }
    
    Copy(this->str, s);
    
    return *this;
}

MString::operator bool() const
{
    if (str != nullptr && length != 0) {
        return true;
    }
    return false;
}

bool MString::operator==(const MString &rhs) const
{
    if (length != rhs.length) {
        return false;
    }
    for (u64 i = 0; i < length; i++) {
        if (str[i] != rhs.str[i]) {
            return false;
        }
        
    }
    
    return true;
}

bool MString::operator==(const char *s) const
{
    if (length - 1 != Length(s)) {
        return false;
    }
    for (u64 i = 0; i < length - 1; i++) {
        if (str[i] != s[i]) {
            return false;
        }
    }
    
    return true;
}

constexpr u16 MString::Length() const noexcept
{
    if (length) {
        return length - 1;
    }
    return length;
}

const u16 MString::Length(const char *s)
{
    if (!s) {
        return 0;
    }
    
    u64 len = 0;
    while (*s) {
        if(*s != '\n') {
            len++;
        }
        s++;
    }
    
    return len;
}

u16 MString::Len(const char *s)
{
    u16 len = Length(s);
    return len ? len + 1 : 0;
}

const char *MString::c_str() const noexcept
{
    if (str) {
        return str;
    }
    const char* s = "";
    return s;
}

const bool MString::Comparei(const MString &string) const
{
    return MString::Equali(str, string.str);
}

const bool MString::Comparei(const char *string) const
{
    return MString::Equali(str, string);
}

char *MString::Duplicate(const char *s)
{
    u64 length = Length(s);
    char* copy = MMemory::TAllocate<char>(MemoryTag::String, length + 1);
    nCopy(copy, s, length);
    copy[length] = '\0';
    return copy;
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
        char buffer[32000]{};
        i32 written = vsnprintf(buffer, 32000, format, va_list);
        buffer[written] = '\0';
        MMemory::CopyMem(dest, buffer, written + 1);

        return written;
    }
    return -1;
}

void MString::Copy(char *dest, const char *source)
{
    if(dest && source) {
        while (*source) {
            if(*source != '\n') {
                *dest = *source;
                dest++;
            }
            source++;
        }
        *dest = '\0';
    }
}

void MString::Copy(char *dest, const MString &source)
{
    Copy(dest, source.str);
}

void MString::nCopy(const MString& source, u64 length)
{
        if(str) {
            for (u64 i = 0; i < length; i++) {
            str[i] = source.str[i];
            }
        }
}

void MString::nCopy(char *dest, const char *source, u64 Length)
{
    if (dest && source) {
        for (u64 i = 0; i < Length; i++) {
            dest[i] = source[i];
            if (!dest[i] || !source[i]) {
                break;
            }
        }
    }
}

void MString::nCopy(char *dest, const MString &source, u64 length)
{
    if (dest && source.str) {
        for (u64 i = 0; i < length; i++) {
            dest[i] = source[i];
            if (!dest[i] || !source[i]) {
                break;
            }
        }
    }
}

char* MString::Copy(const char *source, u64 lenght)
{
    if(source && lenght) {
        str = MMemory::TAllocate<char>(MemoryTag::String, length); 
        Copy(str, source);
        return str;
    }
    return nullptr;
}

char *MString::Copy(const MString &source)
{   
    if (!source) {
        return nullptr;
    }
    str = MMemory::TAllocate<char>(MemoryTag::String, length); 
    nCopy(source, length);
    return str;
}

void MString::Trim()
{
    if (str) {
        str = MString::Trim(str);
    }
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

void MString::Zero(char *string)
{
    if (string) {
        while (*string) {
            *string = '\0';
            string++;
        }
    }
}

void MString::Mid(char *dest, const MString &source, i32 start, i32 length)
{
    if (length == 0) {
        return;
    }
    const u64& srcLength = source.Length();
    if (start >= srcLength) {
        dest[0] = '\0';
        return;
    }
    if (length > 0) {
        for (u64 i = start, j = 0; j < length && source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + length] = '\0';
    } else {
        // Если передано отрицательное значение, перейдите к концу строки.
        u64 j = 0;
        for (u64 i = start; source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + j] = '\0';
    }
}

i32 MString::IndexOf(char *str, char c)
{
    if (!str) {
        return -1;
    }
    u32 length = Length(str);
    if (length > 0) {
        for (u32 i = 0; i < length; ++i) {
            if (str[i] == c) {
                return i;
            }
        }
    }

    return -1;
}

i32 MString::IndexOf(char c)
{
    return MString::IndexOf(str, c);
}

bool MString::ToVector4D(char *s, Vector4D<f32> &OutVector)
{
    if (!s) {
        return false;
    }

    i32 result = sscanf(s, "%f %f %f %f", &OutVector.x, &OutVector.y, &OutVector.z, &OutVector.w);
    return result != -1;
}

bool MString::ToVector4D(Vector4D<f32> &OutVector)
{
    return MString::ToVector4D(str, OutVector);
}

bool MString::ToVector3D(char *str, Vector3D<f32> &OutVector)
{
    if (!str) {
        return false;
    }

    i32 result = sscanf(str, "%f %f %f", &OutVector.x, &OutVector.y, &OutVector.z);
    return result != -1;
}

bool MString::ToVector2D(char *str, Vector2D<f32> &OutVector)
{
    if (!str) {
        return false;
    }

    i32 result = sscanf(str, "%f %f", &OutVector.x, &OutVector.y);
    return result != -1;
}

bool MString::ToFloat(char *str, f32 &f)
{
    if (!str) {
        return false;
    }

    f = 0;
    i32 result = sscanf(str, "%f", &f);
    return result != -1;
}

bool MString::ToFloat(char *str, f64 &f)
{
    if (!str) {
        return false;
    }

    f = 0;
    i32 result = sscanf(str, "%lf", &f);
    return result != -1;
}

bool MString::ToInt(char *str, i8 &i)
{
    if (!str) {
        return false;
    }

    i = 0;
    i32 result = sscanf(str, "%hhi", &i);
    return result != -1;
}

bool MString::ToInt(char *str, i16 &i)
{
    if (!str) {
        return false;
    }

    i = 0;
    i32 result = sscanf(str, "%hi", &i);
    return result != -1;
}

bool MString::ToInt(char *str, i32 &i)
{
    if (!str) {
        return false;
    }

    i = 0;
    i32 result = sscanf(str, "%i", &i);
    return result != -1;
}

bool MString::ToInt(char *str, i64 &i)
{
    if (!str) {
        return false;
    }

    i = 0;
    i32 result = sscanf(str, "%lli", &i);
    return result != -1;
}

bool MString::ToUInt(char *str, u8 &u)
{
    if (!str) {
        return false;
    }

    u = 0;
    i32 result = sscanf(str, "%hhu", &u);
    return result != -1;
}

bool MString::ToUInt(char *str, u16 &u)
{
    if (!str) {
        return false;
    }

    u = 0;
    i32 result = sscanf(str, "%hu", &u);
    return result != -1;
}

bool MString::ToUInt(char *str, u32 &u)
{
    if (!str) {
        return false;
    }

    u = 0;
    i32 result = sscanf(str, "%u", &u);
    return result != -1;
}

bool MString::ToUInt(char *str, u64 &u)
{
    if (!str) {
        return false;
    }

    u = 0;
    i32 result = sscanf(str, "%llu", &u);
    return result != -1;
}

bool MString::ToBool(/*char *str, */bool &b)
{
    if (!str) {
        MERROR("MString::ToBool: нулевая строка")
        return false;
    }
    b = MString::Equal(str, "1") || MString::Equali(str, "true");
    return true;
}

u32 MString::Split(const char *str, char delimiter, DArray<MString> &darray, bool TrimEntries, bool IncludeEmpty)
{
    if (!str) {
        return 0;
    }

    MString result;
    //u32 TrimmedLength = 0;
    u32 EntryCount = 0;
    u32 length = Length(str);
    char buffer[16384];  // Если одна запись выходит за рамки этого, что ж... просто не делайте этого.
    u32 CurrentLength = 0;
    // Повторяйте каждый символ, пока не будет достигнут разделитель.
    for (u32 i = 0; i < length; ++i) {
        char c = str[i];

        // Найден разделитель, финализируйте строку.
        if (c == delimiter) {
            buffer[CurrentLength] = '\0';
            result = buffer;
            
            // Добавить новую запись
            if (IncludeEmpty || result) {
                if (!result) {
                    darray.PushBack(MString());
                } else {
                    // MString entry{result};
                    darray.PushBack(result);
                }
                EntryCount++;
            } 

            // Очистка буфера.
            MMemory::ZeroMem(buffer, sizeof(char) * 16384);
            CurrentLength = 0;
            continue;
        }

        buffer[CurrentLength] = c;
        CurrentLength++;
    }

    // В конце строки. Если какие-либо символы поставлены в очередь, прочитайте их.
    result = buffer;
    
    // Добавить новую запись
    if (IncludeEmpty || result) {
        if (!result) {
            darray.PushBack(MString());
        } else {
            //MString entry{result};
            darray.PushBack(result);
        }
        EntryCount++;
    }

    return EntryCount;
}

u32 MString::Split(char delimiter, DArray<MString> &darray, bool TrimEntries, bool IncludeEmpty)
{
    return MString::Split(str, delimiter, darray, TrimEntries, IncludeEmpty);
}

void MString::Clear()
{
    if (str) {
        MMemory::Free(str, length, MemoryTag::String);  
        length = 0;
        str = nullptr;
    }
}
/*
void *MString::operator new(u64 size)
{
    return MMemory::Allocate(size, MemoryTag::String);
}

void MString::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::String);
}

void *MString::operator new[](u64 size)
{
    return MMemory::Allocate(size, MemoryTag::String);
}

void MString::operator delete[](void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::String);
}
*/
bool MString::Equal(const char *strL, const char *strR)
{
    return strcmp(strL, strR) == 0;
}

bool MString::Equali(const char *str0, const char *str1)
{
#if defined(__GNUC__)
    return strcasecmp(str0, str1) == 0;
#elif (defined _MSC_VER)
    return _strcmpi(str0, str1) == 0;
#endif
}
