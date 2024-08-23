#include "mstring.hpp"
#include "darray.hpp"
#include "core/mmemory.hpp"

#include <string>
#include <stdarg.h>

constexpr MString::MString(u16 length) 
: length(length + 1), str(MMemory::TAllocate<char>(Memory::String, this->length)) {}

constexpr MString::MString(const char *str1, const char *str2)
: length(Length(str1) + Length(str2) + 1), str(Concat(str1, str2, length)) {}

constexpr MString::MString(const MString &str1, const MString &str2) 
: length(str1.length + str2.length - 1), str(Concat(str1.str, str2.str, length)) {}

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
        str = MMemory::TAllocate<char>(Memory::String, length);
    } 

    nCopy(s, length);
    return *this;
}

MString &MString::operator=(MString &&s)
{
    if (str && length != s.length) {
        Clear();
    }
    if (length != s.length) {
        length = s.length;
    } 

    str = s.str;
    s.length = 0;
    s.str = nullptr;

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
        str = MMemory::TAllocate<char>(Memory::String, length);
    }
    
    Copy(this->str, s);
    
    return *this;
}

MString &MString::operator+=(const MString &s)
{
    char* NewString = MMemory::TAllocate<char>(Memory::String, length + s.length - 1);
    for (u64 i = 0, j = 0; i < length + s.length - 1; i++) {
        if (i < length - 1) {
           NewString[i] = str[i];
        } else {
            NewString[i] = s.str[j];
            j++;
        }
    }
    if (str) {
        Clear();
    }
    str = NewString;
    length = length + s.length - 1;

    return *this;
}

MString &MString::operator+=(i64 n)
{
    return *this += IntToString(n);
}

MString &MString::Append(const char* source, f32 f)
{
    sprintf(str, "%s%f", source, f);
    return *this;
}

MString &MString::operator+=(bool b)
{
    char* NewString = nullptr;
    const char* bl = nullptr;
    if (b) {
        bl = "true";
        NewString = MMemory::TAllocate<char>(Memory::String, length + 4);
    } else {
        bl = "false";
        NewString = MMemory::TAllocate<char>(Memory::String, length + 5);
    }
    for (u64 i = 0, j = 0; i < length + 5; i++) {
        if(str[i]) {
            NewString[i] = str[i];
        } else {
            NewString[i] = bl[j];
            j++;
        }
    }
    
    return *this;
}

MString &MString::operator+=(char c)
{
    // ЗАДАЧА: вставьте здесь оператор return
}

void MString::DirectoryFromPath(char* dest, const char *path)
{
    u64 length = Length(path);
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            nCopy(dest, path, i + 1);
            return;
        }
    }
}

MString &MString::FilenameFromPath(const char *path)
{
    u64 length = Length(path);
    if (this->length < length) {
        Clear();
    }
    this->length = length + 1;
    str = MMemory::TAllocate<char>(Memory::String, this->length);
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            Copy(str, path + i + 1);
            return *this;
        }
    }
    return *this;
}

void MString::FilenameNoExtensionFromPath(char* dest, const char *path)
{
    u64 length = Length(path);
    u64 start = 0;
    u64 end = 0;
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (end == 0 && c == '.') {
            end = i;
        }
        if (start == 0 && (c == '/' || c == '\\')) {
            start = i + 1;
            break;
        }
    }

    Mid(dest, path, start, end - start);
}

MString::operator bool() const
{
    if (str != nullptr) {
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

const bool MString::nCompare(const MString &string, u64 lenght) const
{
    if (!str && !string) {
        return true;
    } else {
        return false;
    }
    
    if (this->length < lenght && string.length < lenght) {
        MERROR("MString::Necompare: длина сравнения больше длины строки!")
        return false;
    }
    
    for (u64 i = 0; i < lenght; i++) {
        if(!(str[i] == string[i])) {
            return false;
        } 
    }
    return true;
}

const bool MString::nCompare(const char *string, u64 lenght) const
{
    if (!str && !string) {
        return true;
    } else {
        return false;
    }

    if (this->length < lenght && Length(string) < lenght) {
        MERROR("MString::Necompare: длина сравнения больше длины строки!")
        return false;
    }

    for (u64 i = 0; i < lenght; i++) {
        if(!(str[i] == string[i])) {
            return false;
        } 
    }
    return true;
}

const bool MString::nComparei(const MString &string, u64 length) const
{
    return nComparei(str, string.str, length);
}

const bool MString::nComparei(const char *string, u64 length) const
{
    return nComparei(str, string, length);
}

const bool MString::nComparei(const char *string1, const char *string2, u64 length)
{
#if defined(__GNUC__)
    return strncasecmp(str0, str1, length) == 0;
#elif (defined _MSC_VER)
    return _strnicmp(string1, string2, length) == 0;
#endif
}

char *MString::Duplicate(const char *s)
{
    u64 length = Length(s);
    char* copy = MMemory::TAllocate<char>(Memory::String, length + 1);
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

MString& MString::IntToString(i64 n)
{
    u8 buf[20]{}; // 10 для i30 и u32
    u16 length = 0;
    bool minus = false;

    for (u32 i = 1; i < 21; i++) {
        if (n < 0) {
			n *= -1;
			minus = true;
		}
        buf[20 - i] = '0' + (n % 10);
        n /= 10;
        if (!n) {
            if (minus) {
				length = i + 1;
				buf[20 - i - 1] = '-';
				break;
			} 
            else length = i;
            break;
        }
    }

    if (str) {
        Clear();
    }
    this->length = length + 1;
    str = MMemory::TAllocate<char>(Memory::String, this->length);
    str[length] = '\0';
    

    for (u32 i = 20 - length, j = 0; i < 20; i++) {
        str[j] = buf[i];
        j++;
    }
    return *this;
}

i64 MString::StringToI64(const char *s)
{
    i64 num = 0;
    bool sign = false;
    if (*s == '-') {
        sign = true;
        s++;
    }
    while (s) {
        num += *s - '0';
        num *= 10;
    }
    return sign ? num / 10 * -1 : num/10;
}

bool MString::StringToF32(const char* s, f32& fn1, f32* fn2, f32* fn3, f32* fn4)
{
    // ЗАДАЧА: Убрать лишние проверки
    u8 count = 1;
    if (fn2) {
        *fn2 = 0;
    }
    if (fn3) {
        *fn3 = 0;
    }
    if (fn4) {
        *fn4 = 0;
    }
    f64  buffer   = 0; 
    u32  factor   = 10;
    bool sign     = false;
    bool mantissa = false;
    auto end      = [&]() {
        buffer /= factor;
        if (sign) {
            buffer *= -1.F;
        }
        if (count == 1) {
            fn1 = buffer;
            return true;
        }
        if (count == 2 && fn2) {
            *fn2 = buffer;
            return true;
        }
        if (count == 3 && fn3) {
            *fn3 = buffer;
            return true;
        }
        if (count == 4 && fn4) {
            *fn4 = buffer;
            return true;
        }
        return false;
    };
    fn1 = 0;
    while (*s != '\0') {
        switch (*s)
        {
        case '-': {
            sign = true;
            s++;
        } break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            buffer += *s - '0';
            buffer *= 10;
            if (mantissa) {
                factor *= 10;
            }
            s++;
        } break;
        case '.': case ',': {
            mantissa = true;
            s++;
        } break;
        case ' ': case '\0': {
            s++;
            end();
            count++;
            buffer   = 0;
            factor   = 10;
            sign     = false;
            mantissa = false;

        } break;
        default: {
            goto ExitLoop;
        }
               break;
        }
    }
ExitLoop:
    return end();
}

void MString::Copy(char *dest, const char *source)
{
    if(dest && source) {
        while (*source) {
            if(*source != '\n' && *source != '\t') {
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
    for (u64 i = 0; i < Length; i++) {
        if (dest && source) {
            dest[i] = source[i];
            if (!source[i]) {
                dest[i] = '\0';
                return;
            }
        }    
    }
    dest[Length] = '\0';
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
        str = MMemory::TAllocate<char>(Memory::String, length); 
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
    str = MMemory::TAllocate<char>(Memory::String, length); 
    nCopy(source, length);
    return str;
}

constexpr char* MString::Concat(const char *str1, const char *str2, u64 length)
{
    if(!str) {
        str = MMemory::TAllocate<char>(Memory::String, length);
    }
    u64 j = 0;
    for (u64 i = 0; i < length; i++) {
        if (!str1[i]) {
            str[i] = str1[i];
        }
        else {
            str[i] = str2[j];
            j++;
        }
    }
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

/*void MString::Zero(char *string)
{
    if (string) {
        while (*string) {
            *string = '\0';
            string++;
        }
    }
}*/

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

bool MString::ToVector(char *s, FVec4 &OutVector)
{
    if (!s) {
        return false;
    }

    i32 result = sscanf(s, "%f %f %f %f", &OutVector.x, &OutVector.y, &OutVector.z, &OutVector.w);
    return result != -1;
}

bool MString::ToVector(FVec4 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.x, &OutVector.y, &OutVector.z, &OutVector.w)) {
        return false;
    }

    return true;
}

bool MString::ToVector(char *str, FVec3 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.x, &OutVector.y, &OutVector.z)) {
        return false;
    }

    return true;
}

bool MString::ToVector(FVec3 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.x, &OutVector.y, &OutVector.z)) {
        return false;
    }

    return true;
}

bool MString::ToVector(char *str, FVec2 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.x, &OutVector.y)) {
        return false;
    }

    return true;
}

bool MString::ToFloat(f32 &f)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, f)) {
        return false;
    }

    return true;
}

bool MString::ToFloat(f64 &f)
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
    b = *this == "1" || Equali(str, "true");
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
                    darray.EmplaceBack();
                } else {
                    // MString entry{result};
                    darray.EmplaceBack(std::move(result));
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
            darray.EmplaceBack();
        } else {
            //MString entry{result};
            darray.EmplaceBack(std::move(result));
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
        MMemory::Free(str, length, Memory::String);  
        length = 0;
        str = nullptr;
    }
}
/*
void *MString::operator new(u64 size)
{
    return MMemory::Allocate(size, Memory::String);
}

void MString::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::String);
}

void *MString::operator new[](u64 size)
{
    return MMemory::Allocate(size, Memory::String);
}

void MString::operator delete[](void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::String);
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

MString operator+(const MString &ls, const MString &rs)
{
    return MString(ls, rs);
}

MString operator+(const MString &ls, const char *rs)
{
    return MString(ls, rs);
}
