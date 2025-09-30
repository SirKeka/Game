#include "mstring.hpp"
#include "darray.h"
#include "core/memory_system.h"

#include "math/transform.h"

#include <string>
#include <stdarg.h>
// #include <locale.h>

constexpr MString::MString(const char *str1, const char *str2)
{
    length = Len(str1, size);
    length += Len(str2, size);
    size--;
    Concat(str1, str2, size);
}

constexpr MString::MString(const MString &str1, const MString &str2) 
: length(str1.length + str2.length), size(str1.size + str2.size - 1), str(Concat(str1.str, str2.str, size)) {}

constexpr MString::MString(const char *s, bool trim)
{
    if (s && *s != 0) {
        length = Len(s, size, trim);
        str = Copy(s, size, trim);
    }
}

constexpr MString::MString(const MString &s) : length(s.length), size(s.size), str(Copy(s)) {}

constexpr MString::MString(MString &&s) : length(s.length), size(s.size), str(s.str) 
{
    s.str = nullptr;
    s.size = s.length = 0;
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
char MString::operator[](u16 i) const
{
    if (!str || i >= size) {
        return '\0';
    } else {
        return str[i];
    }
}

MString &MString::operator=(const MString &s)
{
    if (s.str) {
        if (size == 0) {
            str = reinterpret_cast<char*>(MemorySystem::Allocate(s.size, Memory::String));
        } else if (size != 0 && size != s.size) {
            str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size, s.size, Memory::String, false));
        }

        length = s.length;
        size = s.size;
             
        Copy(str, s.str, size);
    }
    return *this;
}

MString &MString::operator=(MString &&s)
{
    if (s.str) {
        if (str) {
            Clear();
        }

        length = s.length;
        size = s.size;

        str = s.str;
        s.length = s.size = 0; s.str = nullptr;
    }

    return *this;
}

MString &MString::operator=(const char *s)
{
    if (s && *s != 0) {
        u32 ssize = 0;
        u16 slength = Len(s, ssize);

        if (!str) {
            str = reinterpret_cast<char*>(MemorySystem::Allocate(ssize, Memory::String));
        } else if (size != ssize) {
            // str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size, ssize, Memory::String)); ЗАДАЧА: Отдадить realloc
            MemorySystem::Free(str, size, Memory::String);
            str = reinterpret_cast<char*>(MemorySystem::Allocate(ssize, Memory::String));
        }

        length = slength;
        size = ssize;

        Copy(str, s);
    }

    return *this;
}

MString &MString::operator+=(const char *s)
{
    u32 c_size = 0;
    u32 c_length = Len(s, c_size);
    u32 NewSize = size + c_size - 1;
    str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size, NewSize, Memory::String));
    for (u64 i = length, j = 0; i < c_size; i++, j++) {
        str[i] = s[j];
    }

    length += c_length;
    size = NewSize;

    return *this;
}

MString &MString::operator+=(const MString &s)
{
    length += s.length;
    u32 NewSize = size + s.size - 1;
    str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size, NewSize, Memory::String));
    for (u64 i = size, j = 0; i < s.size; i++, j++) {
        str[i] = s.str[j];
    }

    size = NewSize;

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
    // char* NewString = nullptr;
    const char* bl = nullptr;
    if (b) {
        bl = "true";
        str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size, size + 4, Memory::String)); // NewString = MMemory::TAllocate<char>(Memory::String, size + 4);
    } else {
        bl = "false";
        str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size, size + 5, Memory::String)); // NewString = MMemory::TAllocate<char>(Memory::String, size + 5);
    }
    for (u64 i = length, j = 0; i < size + 5; i++, j++) {
        if(str[i]) {
            str[i] = bl[j];
        } else {
            str[i] = bl[j];
        }
    }
    
    return *this;
}

MString &MString::operator+=(char c)
{
    if (c != 0) {
        u32 CharSize = CheckSymbol(c);  // Получаем размер символа ПРИМЕЧАНИЕ: для UTF8 может быть больше 1го байта

        if (!length) {
            size = CharSize + 1;        // Размер данной строки равен развмеру символа + '\0'
        } else {
            size += CharSize;
        }
        length++;
        
        if (!str) {
            str = reinterpret_cast<char*>(MemorySystem::Allocate(size, Memory::String));
        } else {
            str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size - CharSize, size, Memory::String));
        }
        
        u32 n = size - 1;               // Номер последней ячейки памяти где должен быть '\0'.
        str[n - CharSize] = c;
        str[n] = '\0';
    }

    return *this;
}

void MString::Create(char *str, u64 length)
{
    this->str = str;

    this->length = length;
}

void MString::DirectoryFromPath(char* dest, const char *path)
{
    u64 length = Length(path);
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            Copy(dest, path, i + 1);
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
    str = MemorySystem::TAllocate<char>(Memory::String, this->length);
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

    for (u64 i = 0; i < size; i++) {
        if (str[i] != rhs.str[i]) {
            return false;
        }
    }
    
    return true;
}

bool MString::operator==(const char *s) const
{
    for (u64 i = 0; i < size; i++) {
        if (str[i] != s[i]) {
            return false;
        }
    }
    
    return true;
}

constexpr const u32& MString::Length() const noexcept
{
    return length;
}

constexpr const u32 &MString::Size() const noexcept
{
    return size;
}

u32 MString::nChar()
{
    return size ? size : size - 1;
}

u32 MString::Length(const char *s)
{
    if (!s) {
        return 0;
    }
    
    u64 len = 0;
    while (*s) {
        len++;
        s++;
    }
    
    return len;
}

u32 MString::UTF8Length(const char *str)
{
    u32 length = 0;

    for (u32 i = 0; i < __UINT32_MAX__; ++length) {
        if (str[i] == '\0') {
            break;
        }
        i += CheckSymbol(str[i]);
    }

    return length;
}

bool MString::BytesToCodepoint(const char *bytes, u32 offset, i32 &OutCodepoint, u8 &OutAdvance)
{
    i32 codepoint = (i32)bytes[offset];
    if (codepoint >= 0 && codepoint < 0x7F) {
        // Обычный однобайтовый символ ascii.
        OutAdvance = 1;
        OutCodepoint = codepoint;
        return true;
    } else if ((codepoint & 0xE0) == 0xC0) {
        // Двухбайтовый символ
        codepoint = ((bytes[offset + 0] & 0b00011111) << 6) +
                    (bytes[offset + 1] & 0b00111111);
        OutAdvance = 2;
        OutCodepoint = codepoint;
        return true;
    } else if ((codepoint & 0xF0) == 0xE0) {
        // Трехбайтовый символ
        codepoint = ((bytes[offset + 0] & 0b00001111) << 12) +
                    ((bytes[offset + 1] & 0b00111111) << 6) +
                    (bytes[offset + 2] & 0b00111111);
        OutAdvance = 3;
        OutCodepoint = codepoint;
        return true;
    } else if ((codepoint & 0xF8) == 0xF0) {
        // 4-байтовый символ
        codepoint = ((bytes[offset + 0] & 0b00000111) << 18) +
                    ((bytes[offset + 1] & 0b00111111) << 12) +
                    ((bytes[offset + 2] & 0b00111111) << 6) +
                    (bytes[offset + 3] & 0b00111111);
        OutAdvance = 4;
        OutCodepoint = codepoint;
        return true;
    } else {
        // ПРИМЕЧАНИЕ: Не поддерживаются 5- и 6-байтовые символы; возвращается как недопустимый UTF-8.
        OutAdvance = 0;
        OutCodepoint = 0;
        MERROR("MString::BytesToCodepoint() — не поддерживает 5- и 6-байтовые символы; недопустимый UTF-8.");
        return false;
    }
}

bool MString::BytesToCodepoint(u32 offset, i32 &OutCodepoint, u8 &OutAdvance)
{
    return BytesToCodepoint(str, offset, OutCodepoint, OutAdvance);
}

constexpr u32 MString::Len(const char *s, u32& size, bool DelCon)
{   
    u32 len = 0;
    u8 CharSize = 0;
    if (!DelCon) {
        while (*s) {
            CharSize = CheckSymbol(*s);
            size += CharSize;
            len++;
            s += CharSize;
        }
        size++;
        
    } else {
        while (*s) {
            CharSize = CheckSymbol(*s);

            if (*s != '\a' && *s != '\b' && *s != '\t' && 
                *s != '\n' && *s != '\v' && *s != '\f' && *s != '\r') {
                len++;
                size += CharSize;
            }
            
            s += CharSize;
        }
        if (size) { // size = size ? size++ : size;
            size++;
        }
         
    }
    return len;
}

const char *MString::c_str() const noexcept
{
    if (str) {
        return str;
    }
    const char* s = "";
    return s;
}

bool MString::Compare(const char *strL, const char *strR)
{
    if (strL && strR) {
        while (*strL && *strR) {
            if (*strL != *strR) {
                return false;
            }
            strL++; strR++;
        }
        return true;
    }
    return false;
}

bool MString::Comparei(const MString &string) const
{
    return MString::Equali(str, string.str);
}

bool MString::Comparei(const char *string, u64 length) const
{
    if (!length)
        return MString::Equali(str, string);
    else
        return nComparei(str, string, length);
}

bool MString::nCompare(const MString &string, u64 lenght) const
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

bool MString::nCompare(const char *string, u64 lenght) const
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

bool MString::nComparei(const MString &string, u64 length) const
{
    return nComparei(str, string.str, length);
}

bool MString::nComparei(const char *string, u64 length) const
{
    return nComparei(str, string, length);
}

bool MString::nComparei(const char *string1, const char *string2, u64 length)
{
#if defined(__GNUC__)
    return strncasecmp(str0, str1, length) == 0;
#elif (defined _MSC_VER)
    return _strnicmp(string1, string2, length) == 0;
#endif

/*if (length == 0) {
        length = UINT64_MAX;
    }
     
    for (u64 i = 0; i < length; i++) {
        
    }
*/
}

char *MString::Duplicate(const char *s)
{
    u64 length = Length(s);
    char* copy = MemorySystem::TAllocate<char>(Memory::String, length + 1);
    Copy(copy, s, length);
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
        MemorySystem::CopyMem(dest, buffer, written + 1);

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
    str = MemorySystem::TAllocate<char>(Memory::String, this->length);
    str[length] = '\0';
    

    for (u32 i = 20 - length, j = 0; i < 20; i++) {
        str[j] = buf[i];
        j++;
    }
    return *this;
}

i64 MString::ToInt(const char *s)
{
    i64 num = 0;
    bool sign = false;
    if (*s == '-') {
        sign = true;
        s++;
    }
    while (*s) {
        if (*s >= '0' || *s <= '9') {
            num += *s - '0';
            num *= 10;
        }
        s++;
    }
    return sign ? num / 10 * -1 : num/10;
}

u64 MString::ToUInt(const char *s)
{
    u64 num = 0;
    while (*s) {
        switch (*s) {
        case '0' ... '9':
            num += *s - '0';
            num *= 10;
        
        default:
            s++;
            break;
        }
    }
    return num/10;
}

bool MString::StringToF32(const char* string, f32(&fArray)[] , u32 aSize)
{
    if (!string) {
        return false;
    }
    
    while (*string != '-' && (*string < '0' || *string > '9' )) {
        string++;
    }
    
    // ЗАДАЧА: Убрать лишние проверки
    u32  count    =     0;
    f64  buffer   =     0; 
    u32  factor   =    10;
    bool sign     = false;
    bool mantissa = false;
    auto end      = [&]() {
        buffer /= factor;
        if (sign) {
            buffer *= -1.F;
        }
        if (count < aSize) {
            fArray[count++] = buffer;
        }
        return true;
    };
    while (*string != '\0') {
        switch (*string) {
            case '-': {
                sign = true;
                string++;
            } break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                buffer += *string - '0';
                buffer *= 10;
                if (mantissa) {
                    factor *= 10;
                }
                string++;
            } break;
            case '.': case ',': {
                mantissa = true;
                string++;
            } break;
            case ' ': case '\0': {
                end();
                buffer   = 0;
                factor   = 10;
                sign     = false;
                mantissa = false;
                string++;
            } break;
            default: {
                string++;
            } break;
        }
    }
    return end();
}

bool MString::StringToF32(const char *s, f32 &fn1, f32 *fn2, f32 *fn3, f32 *fn4)
{
    while (*s != '-' && (*s < '0' || *s > '9' )) {
        s++;
    }
    
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
            if (buffer) {
                end();
            }
            count++;
            buffer   = 0;
            factor   = 10;
            sign     = false;
            mantissa = false;
            s++;
        } break;
        /*case '\0': {
            goto ExitLoop;
        } break;*/
        default: {
            s++;
        } break;
        }
    }
//ExitLoop:
    return end();
}

char* MString::Copy(char *dest, const char *source, u64 length, bool DelCon)
{
    if(dest && source) {
        u64 count = length ? length : UINT64_MAX;
        for (u64 i = 0; i < count; i++) {
            switch (source[i])
            {
            case '\a': case '\b': case '\t': 
            case '\n': case '\v': case '\f': case '\r': {
                if (!DelCon && i < count - 1) {
                    dest[i] = source[i];
                } else if (DelCon && i < count - 1) {
                    
                } else {
                    dest[i] = 0;
                }
            } break;
            case '\0': {
                dest[i] = 0;
                goto ExitLoop;
            } break;
            default: {
                dest[i] = source[i];
            } break;
            }
        }
        ExitLoop:
        return dest;
    }
    return nullptr;
}

// char* MString::Copy(char *dest, const MString &source, u64 length, bool DelCon)
// {
//     return Copy(dest, source.str, length, DelCon);
// }

void MString::Copy(const MString& source, u64 length)
{
    if(str) {
        for (u64 i = 0; i < length; i++) {
        str[i] = source.str[i];
        }
    }
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

void MString::Mid(char *dest, const MString &source, u32 start, i32 length)
{
    if (length == 0) {
        return;
    }
    const u32& srcLength = source.Length();
    if (start >= srcLength) {
        dest[0] = '\0';
        return;
    }
    if (length > 0) {
        for (i64 i = start, j = 0; j < length && source[i]; ++i, ++j) {
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

char *MString::Move()
{
    length = size = 0;
    char* NewStr = str;
    str = nullptr;
    return NewStr;
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

bool MString::ToFVector(FVec4 &OutVector)
{
    if (!str) {
        return false;
    }
    if (!StringToF32(str, OutVector.elements, 4)) {
        return false;
    }

    return true;
}

bool MString::ToFVector(char *str, FVec3 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.elements, 3)) {
        return false;
    }

    return true;
}

bool MString::ToFVector(FVec3 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.elements, 3)) {
        return false;
    }

    return true;
}

bool MString::ToFVector(char *str, FVec2 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.elements, 2)) {
        return false;
    }

    return true;
}

bool MString::ToFVector(FVec2 &OutVector)
{
    if (!str) {
        return false;
    }

    if (!StringToF32(str, OutVector.elements, 2)) {
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

bool MString::ToInt(i8 &value)
{
    if (str) {
        i64 v = ToInt(str);
        if (v <= INT8_MAX || v >= INT8_MIN) {
            value = v;
            return true;
        } else {
            return false;
        }
        
    } else {
        return false;
    }
}

bool MString::ToInt(u8 &value)
{
    if (str) {
        u64 v = ToUInt(str);
        if (v <= UINT8_MAX) {
            value = v;
            return true;
        } else {
            return false;
        }
        
    } else {
        return false;
    }
}

bool MString::ToInt(i16 &value)
{
    if (str) {
        i64 v = ToInt(str);
        if (v <= INT16_MAX || v >= INT16_MIN) {
            value = v;
            return true;
        } else {
            return false;
        }
        
    } else {
        return false;
    }
}

bool MString::ToInt(u16 &value)
{
    if (str) {
        u64 v = ToUInt(str);
        if (v <= UINT16_MAX) {
            value = v;
            return true;
        } else {
            return false;
        }
        
    } else {
        return false;
    }
}

bool MString::ToInt(i32 &value)
{
    if (str) {
        i64 v = ToInt(str);
        if (v <= INT32_MAX || v >= INT32_MIN) {
            value = v;
            return true;
        } else {
            return false;
        }
        
    } else {
        return false;
    }
}

bool MString::ToInt(u32 &value)
{
    if (str) {
        u64 v = ToUInt(str);
        if (v <= UINT32_MAX) {
            value = v;
            return true;
        } else {
            return false;
        }
        
    } else {
        return false;
    }
}

bool MString::ToMatrix(Matrix4D &value)
{
    // ЗАДАЧА: Вставить код
    return false;
}

bool MString::ToBool(bool &b)
{
    if (!str) {
        MERROR("MString::ToBool: нулевая строка")
        return false;
    }
    b = (*this == "1") || (*this == "true");
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
                    darray.PushBack(std::move(result));
                }
                EntryCount++;
            } 

            // Очистка буфера.
            MemorySystem::ZeroMem(buffer, sizeof(char) * 16384);
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
            darray.PushBack(std::move(result));
        }
        EntryCount++;
    }

    return EntryCount;
}

u32 MString::Split(char delimiter, DArray<MString> &darray, bool TrimEntries, bool IncludeEmpty) const
{
    return MString::Split(str, delimiter, darray, TrimEntries, IncludeEmpty);
}

void MString::Clear()
{
    if (str) {
        MemorySystem::Free(str, size, Memory::String);  
        length = size = 0;
        str = nullptr;
    }
}

void MString::DeleteLastChar()
{
    if (str && length == 1) {
        Clear();
        return;
    }
    
    if (str && length) {
        u32 i = 0;
        //u32 len = 0; 
        u8 CharSize = 0;
        while (i < size - 1) {
            CharSize = CheckSymbol(str[i]);
            i += CharSize;
            //len++;
        }
        length--;
        size -= CharSize;
        MemorySystem::Free(&str[size], CharSize, Memory::String);
        
        str[size - 1] = '\0';
    }    
}

constexpr char *MString::Copy(const char *source, u64 length, bool DelCon)
{
    if(source && length) {
        str = reinterpret_cast<char*>(MemorySystem::Allocate(length, Memory::String)); 
        return Copy(str, source, length, DelCon);
    }
    return nullptr;
}

constexpr char *MString::Copy(const MString &source)
{   
    if (!source) {
        return nullptr;
    }
    str = reinterpret_cast<char*>(MemorySystem::Allocate(length + 1, Memory::String)); 
    Copy(source, length + 1);
    return str;
}

constexpr char* MString::Concat(const char *str1, const char *str2, u64 length)
{
    if(!str) {
        str = reinterpret_cast<char*>(MemorySystem::Allocate(length, Memory::String));
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

u32 MString::CheckSymbol(const char &c)
{
    i32 ch = static_cast<i32>(c);
    if (ch >= 0 && ch < 127) {
        // Обычный символ ascii, не увеличивать снова.
        return 1;// i += 0; // В основном это так.
    } else if ((ch & 0xE0) == 0xC0) {
        // Двухбайтовый символ, увеличить еще раз.
        return 2;
    } else if ((ch & 0xF0) == 0xE0) {
        // Трехбайтовый символ, увеличить еще вдвое.
        return 3;
    } else if ((ch & 0xF8) == 0xF0) {
        // 4-байтовый символ, увеличить еще втрое.
        return 4;
    } else {
        // ПРИМЕЧАНИЕ: Не поддерживаются 5- и 6-байтовые символы; возвращается как недопустимый UTF-8.
        MERROR("MString::CheckSymbol - Не поддерживаются 5- и 6-байтовые символы; Недопустимый UTF-8.");
        return 0;
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
bool MString::ToTransform(const char *str, Transform &transform)
{
    if (!str) {
        return false;
    }

    // MemorySystem::ZeroMem(&transform, sizeof(Transform));
    f32 values[7]{};

    i32 count = sscanf(
        str,
        "%f %f %f %f %f %f %f %f %f %f",
        &transform.position.x, &transform.position.y, &transform.position.z,
        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &values[6]);

    if (count == 10) {
        // Обрабатывать как кватернион, загружать напрямую.
        transform.rotation.x = values[0];
        transform.rotation.y = values[1];
        transform.rotation.z = values[2];
        transform.rotation.w = values[3];

        // Установить масштаб
        transform.scale.x = values[4];
        transform.scale.y = values[5];
        transform.scale.z = values[6];
    } else if (count == 9) {
        Quaternion xRot{FVec3(1.F, 0, 0), Math::DegToRad(values[0]), true};
        Quaternion yRot{FVec3(0, 1.F, 0), Math::DegToRad(values[1]), true};
        Quaternion zRot{FVec3(0, 0, 1.F), Math::DegToRad(values[2]), true};
        transform.rotation = xRot * (yRot * zRot);

        // Установить масштаб
        transform.scale.x = values[4];
        transform.scale.y = values[5];
        transform.scale.z = values[6];
    } else {
        MWARN("Ошибка формата: предоставлено недопустимое преобразование. Будет использовано преобразование идентичности.");
        transform = Transform();
        return false;
    }

    transform.IsDirty = true;

    return true;
}

bool MString::ToTransform(Transform &transform)
{
    return ToTransform(str, transform);
}

bool MString::Equal(const char *strL, const char *strR)
{
    return strcmp(strL, strR) == 0;
}

bool MString::Equali(const char *string0, const char *string1)
{
    if (!string0 || !string1) {
        return false;
    }
    
// #if defined(__GNUC__)
//     return strcasecmp(str0, str1) == 0;
// #elif (defined _MSC_VER)
//     return _strcmpi(str0, str1) == 0;
// #endif

    i8 a = 'a' - 'A';

    while (*string0 && *string1) {
        if (*string0 == *string1) {
            string0++; string1++;
        } else if (*string0 >= 'A' && *string0 <= 'Z' && *string1 == (a + *string0)) {
            string0++; string1++;
        } else if (*string0 >= 'a' && *string0 <= 'z' && *string1 == (*string0 - a)) {
            string0++; string1++;
        } else return false;
    }

    return *string0 == *string1;

}

MString operator+(const MString &ls, const MString &rs)
{
    return MString(ls, rs);
}

MString operator+(const MString &ls, const char *rs)
{
    return MString(ls, rs);
}
