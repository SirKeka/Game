#include "mstring.h"
#include "darray.h"
#include "core/memory_system.h"

#include "math/transform.h"

#include <string>
#include <stdarg.h>

constexpr MString::MString(u32 capacity) : size(), capacity(capacity < hb_size ? hb_size : capacity), heapbuf()
{
    if (capacity >= hb_size) heapbuf.str = (char*)MemorySystem::Allocate((capacity) * sizeof(char), Memory::String, true);
}

constexpr MString::MString(const char *str1, const char *str2) : size(), capacity(hb_size), heapbuf()
{
    u32 size1 = 0, size2 = 0; 
    u32 len1 = Len(str1, size1);
    u32 len2 = Len(str2, size2);
    size = size1 + size2 - 1;
    capacity = size < capacity ? capacity : size;
    heapbuf.length = size > hb_size ? len1 + len2 : 0;
    Concat(str1, str2, size1 - 1, size2 - 1);
}

constexpr MString::MString(const MString &str1, const MString &str2) 
: size(str1.Size() + str2.Size() - 1), capacity(size < hb_size ? hb_size : size), heapbuf()
{
    Concat(str1.Data(), str2.Data(), str1.Length(), str2.Length());
}

constexpr MString::MString(const char *s, bool trim) : size(), capacity(hb_size), heapbuf()
{
    u32 ssize = 0;
    u32 len = Len(s, ssize, trim);
    Copy(s, ssize, len);
}

constexpr MString::MString(const char *s, u32 length) : size(), capacity(hb_size), heapbuf()
{
    u32 l = MString::Length(s, true, length);
    Copy(s, length, l);
}

constexpr MString::MString(const MString &s) : size(), capacity(hb_size), heapbuf() 
{
    Copy(s.Data(), s.Size(), s.Length());
}

constexpr MString::MString(MString &&s) : size(s.size), capacity(s.capacity), heapbuf(s.heapbuf) 
{
    s.Null();
}

constexpr MString::MString(const Substring &ss) : size(), capacity(hb_size), heapbuf()
{
    if (ss.data) {
        Copy(ss.data, ss.size > hb_size ? ss.size + 1 : ss.size);
    }
}

MString::~MString()
{
    Destroy();
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

char MString::operator[](u32 i) const
{
    return capacity < hb_size ? stackbuf[i] : heapbuf.str[i];
}

MString &MString::operator=(const MString &s)
{
    // if (size <= hb_size && s.size <= hb_size) {
    //     size = s.size;
    //     Copy(stackbuf, s.stackbuf);
    //     return *this;
    // }

    // if (s.size > hb_size) {
    //     if (!size) {
    //         size = s.heapbuf.length + 1;
    //         heapbuf.str = (char*)MemorySystem::Allocate(size  * sizeof(char), Memory::String);
    //     } else if (size && size <= s.size) {
    //         //str = (char*)MemorySystem::Realloc(str, size, s.size, Memory::String, false);
    //         MemorySystem::Free(heapbuf.str, size, Memory::String);
    //         size = s.heapbuf.length + 1;
    //         heapbuf.str = (char*)MemorySystem::Allocate(size  * sizeof(char), Memory::String);
    //     }

    //     heapbuf.length = s.heapbuf.length;
    //     Copy(heapbuf.str, s.heapbuf.str, size);
    // }
    capacity = !capacity ? hb_size : capacity;
    Copy(s.Data(), s.Size(), s.Length());
    return *this;
}

MString &MString::operator=(MString &&s)
{
    capacity = !capacity ? hb_size : capacity;
    if (this == &s) {
        return *this;
    } 
    
    if (size >= hb_size) {
        Destroy();
    }
    size     = s.size;
    capacity = s.capacity;
    heapbuf  = s.heapbuf;

    MemorySystem::ZeroMem(&s, sizeof(MString));

    return *this;
}

MString &MString::operator=(const char *s)
{
    capacity = !capacity ? hb_size : capacity;
    // if (s && *s != 0) {
    //     u32 ssize = 0;
    //     u16 slength = Len(s, ssize);

    //     if (ssize < hb_size) {
    //         size = slength;
    //         Copy(stackbuf, s);
    //         return *this;
    //     }
    //     
    //     if (!size) {
    //         heapbuf.str = (char*)MemorySystem::Allocate(ssize  * sizeof(char), Memory::String);
    //     } else if (size <= ssize) {
    //         // str = reinterpret_cast<char*>(MemorySystem::Realloc(str, size, ssize, Memory::String)); ЗАДАЧА: Отладить realloc
    //         MemorySystem::Free(heapbuf.str, size, Memory::String);
    //         heapbuf.str = (char*)MemorySystem::Allocate(ssize  * sizeof(char), Memory::String);
    //     }

    //     size = ssize;
    //     heapbuf.length = slength;

    //     Copy(heapbuf.str, s);
    // }

    u32 ssize = 0;
    u32 len = Len(s, ssize);
    Copy(s, ssize, len);
    return *this;
}

MString &MString::operator=(Substring sb)
{
    capacity = !capacity ? hb_size : capacity;
    if (sb.size > 0 && sb.size == sb.length) { sb.size++; }
    Copy(sb.data, sb.size, sb.length ? sb.length : Length(sb.data, true, sb.size));
    return *this;
}

MString &MString::operator+=(const char *s)
{
    if (!s || *s == 0) { return *this; }
    
    u32 c_size = 0;
    u32 c_length = Len(s, c_size);

    if (size + c_length < hb_size) {
        Copy(stackbuf + size, s);
        size = c_length;
        return *this;
    }

    u32 NewSize = size + c_size - 1;
    heapbuf.str = (char*)MemorySystem::Realloc(heapbuf.str, size, NewSize, Memory::String);
    for (u64 i = heapbuf.length, j = 0; i < c_size; i++, j++) {
        heapbuf.str[i] = s[j];
    }

    heapbuf.length += c_length;
    size = NewSize;

    return *this;
}

MString &MString::operator+=(const MString &s)
{
    if (!s) { return *this; }

    if (size + s.size < hb_size) {
        Copy(stackbuf + size, s.stackbuf);
        size += s.size;
        return *this;
    }

    u32 nlen = heapbuf.length + s.Length();
    if (nlen < size) {
        Copy(heapbuf.str + heapbuf.length, s.c_str());
        heapbuf.length = nlen;
        return *this;
    }
    
    u32 NewSize = size + s.size - 1;
    heapbuf.str = (char*)MemorySystem::Realloc(heapbuf.str, size, NewSize, Memory::String);
    for (u64 i = size, j = 0; i < s.size; i++, j++) {
        heapbuf.str[i] = s.heapbuf.str[j];
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
    if (size < hb_size) {
        sprintf(stackbuf, "%s%f", source, f);
    } else { sprintf(heapbuf.str, "%s%f", source, f); }
    
    return *this;
}

MString &MString::operator+=(bool b)
{
    char* NewString = nullptr;
    const char* bl = nullptr;
    u64 i = 0;
    u32 nSize = 0;

    auto compare = [&](u8 num) {
        if ((nSize = size + num) < hb_size) {
            i = size; NewString = stackbuf;
        } else NewString = heapbuf.str = (char*)MemorySystem::Realloc(heapbuf.str, size, nSize, Memory::String);
    };

    if (b) {
        bl = "true"; compare(4);
    } else {
        bl = "false"; compare(5);
    }

    for (u64 j = 0; i < nSize; i++, j++) {
        NewString[i] = bl[j];
    }

    size = nSize;
    
    return *this;
}

MString &MString::operator+=(char c)
{
    if (c != 0) { 
        // u32 CharSize = CheckSymbol(c);  // Получаем размер символа ПРИМЕЧАНИЕ: для UTF8 может быть больше 1го байта
        u32 len = size - 1;  // размер строки без терминальнго нуля

        if (size < hb_size && size + 1 < hb_size) {
            stackbuf[size++] = c;
            return *this;
        } else if (size + 1 == hb_size) {
            char* NewStr = (char*)MemorySystem::Allocate(size  * sizeof(char), Memory::String);
            Copy(NewStr, stackbuf, len);
            NewStr[size++] = c;
            MemorySystem::ZeroMem(&heapbuf, hb_size);
            heapbuf.str = NewStr;
            heapbuf.length = len;
            return *this;
        }

        if (heapbuf.length == len) {
            // heapbuf.str = (char*)MemorySystem::Realloc(heapbuf.str, size - CharSize, size, Memory::String);
            char* NewStr = (char*)MemorySystem::Allocate((++size) * sizeof(char), Memory::String);
            Copy(NewStr, heapbuf.str, heapbuf.length);
            MemorySystem::Free(heapbuf.str, size, Memory::String);
            heapbuf.str = NewStr;
            heapbuf.str[size - 1] = c;
            heapbuf.str[size] = '\0';
        } else {
            heapbuf.str[heapbuf.length] = c;
        }

        heapbuf.length++;
    }

    return *this;
}

void MString::Create(char *str, u64 length)
{
    heapbuf.str = str;
    capacity = size = length + 1;
    heapbuf.length = length;
}

void MString::Clear()
{
    capacity > hb_size ? heapbuf.length = 0 : size = 0;
}

const char *MString::Data(u32 *size) const
{   
    if (size) { *size = this->size; }
    
    return capacity > hb_size ? heapbuf.str : stackbuf;
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
    if (heapbuf.length < length) { Destroy(); }
    heapbuf.length = length + 1;
    heapbuf.str = (char*)MemorySystem::Allocate(sizeof(char) * heapbuf.length, Memory::String);
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            Copy(heapbuf.str, path + i + 1);
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

// constexpr u32 MString::UTF8Length()
// {
//     /*if (size > hb_size && heapbuf.UTF8Length == 0) {
//         return heapbuf.UTF8Length = Length(heapbuf.str, true);
//     } else */return size > hb_size ? heapbuf.UTF8Length : Length(stackbuf, true);
// }

constexpr u32 MString::Length() const
{
    return size < hb_size ? size : heapbuf.length;
}

MString::operator bool() const
{
    if (size) { return true;
    } else return false;
}

bool MString::operator==(const MString &rhs) const
{
    if (size != rhs.size && heapbuf.length != rhs.heapbuf.length) {
        return false;
    }

    if (this == &rhs) {
        return true;
    }

    const char* strL = nullptr;
    const char* strR = rhs.c_str();
    u32 len = 0;

    if (size < hb_size) {
        strL = stackbuf; len = size;
    } else { strL = heapbuf.str; len = heapbuf.length; }

    for (u64 i = 0; i < len; i++) {
        if (strL[i] != strR[i]) {
            return false;
        }
    }
    
    return true;
}

bool MString::operator==(const char *s) const
{
    if (!size) { return false; }
    
    const char* str = nullptr;

    if (size < hb_size) {
        str = stackbuf;
    } else { str = heapbuf.str; }
    
    for (u64 i = 0; i < size; i++) {
        if (str[i] != s[i]) {
            return false;
        }
    }
    
    return true;
}

bool MString::operator<(const MString &rs) const
{
    if(size > 0 && rs.size > 0) {
        const char* strL = c_str();
        const char* strR = rs.c_str();
        u8 al = 0, ar = 0;
        i32 cpl = 0, cpr = 0;
        for (u32 l = 0, r = 0; l < size; l += al, r += ar) {
            BytesToCodepoint(strL, l, cpl, al);
            BytesToCodepoint(strR, r, cpr, ar);
            if (cpl == cpr) {
                continue;
            } else if (cpl > cpr) {
                return false;
            } else return true;
        }
        return false;
    } else return false;
}

bool MString::operator>(const MString &rs) const
{
    return rs < *this;
}

/* u32 MString::nChar()
{ 
    if (size > hb_size) {
        return size - 1;
    } else if (hb_size) {
        return size - 1;
    } else return size;
} */

u32 MString::Length(const char *s, bool UTF8, u32 size)
{
    if (!s) { return 0; }
    
    u64 len = 0;
    if (UTF8) {
        for (u64 i = 0; size > 0 ? i < size : s[i] != 0; len++) {
            i += CheckSymbol(s[i]);
        }
    } else { 
        for (u64 i = 0; size > 0 ? i < size : s[i] != 0; i++, len++) { }
    }
    
    return len;
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

bool MString::BytesToCodepoint(u32 offset, i32 &OutCodepoint, u8 &OutAdvance) const
{
    if (size < hb_size) {
        return BytesToCodepoint(stackbuf, offset, OutCodepoint, OutAdvance);
    } else return BytesToCodepoint(heapbuf.str, offset, OutCodepoint, OutAdvance);
}

constexpr u32 MString::Len(const char *s, u32& size, bool DelCon)
{   
    u32 l = 0;
    u8 CharSize = 0;
    if (!DelCon) {
        for (; *s; l++) {
            CharSize = CheckSymbol(*s);
            size += CharSize;
            s += CharSize;
        }
    } else {
        while (*s) {
            CharSize = CheckSymbol(*s);

            if (*s != '\a' && *s != '\b' && *s != '\t' && 
                *s != '\n' && *s != '\v' && *s != '\f' && *s != '\r') {
                l++;
                size += CharSize;
            }
            
            s += CharSize;
        }
    }

    if (size) { 
        size++;
    }
    
    return l;
}

bool MString::Compare(const char *strL, const char *strR, u64 length)
{
    if (strL && strR && *strL && *strR) {
        if (length == 0) {
            while (*strL && *strR) {
                if (*strL != *strR) {
                    return false;
                }
                strL++; strR++;
            }
            return true;
        } else {
            for (u64 i = 0; i < length; i++) {
                if (strL[i] != strR[i]) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool MString::Comparei(const char *cstringL, const char *cstringR, u64 length)
{
    if (!cstringL || !cstringR) {
        return false;
    }

    if (length == 0) {
        length = UINT64_MAX;
    }

    for (size_t i = 0; i < length; i++) {
        char ch1 = cstringL[i], ch2 = cstringR[i];
        if (!ch1 || !ch2) {
            if (ch1 == ch2) {
                return true;
            } else return false;
        } else if (ch1 != ch2) {
            if (ch1 + 32 != ch2 && ch1 - 32 != ch2) {
                return false;
            }
        }
    }
    return true;
}

bool MString::Compare(const char *cstring, u64 length) const
{
    if (!size || !cstring) { return false; }

    const char* str = nullptr;
    u32 len = 0;

    if (size < hb_size) {
        str = stackbuf; len = size;
    } else { str = heapbuf.str; len = heapbuf.length; }

    if (len < length && Length(cstring) < length) {
        MERROR("MString::Necompare: длина сравнения больше длины строки!")
        return false;
    }

    for (u64 i = 0; i < length; i++) {
        if(!(str[i] == cstring[i])) {
            return false;
        } 
    }
    return true;
}

bool MString::Compare(const MString &string, u64 length) const
{
    if (!size || !string) {
        return false;
    }

    const char* str = nullptr;
    u32 len = 0;

    if (size < hb_size) {
        str = stackbuf; len = size;
    } else { str = heapbuf.str; len = heapbuf.length; }
    
    if (len < length && string.Length() < length) {
        MERROR("MString::Necompare: длина сравнения больше длины строки!")
        return false;
    }
    
    for (u64 i = 0; i < length; i++) {
        if(!(str[i] == string[i])) {
            return false;
        } 
    }
    return true;
}

bool MString::Comparei(const MString &string) const
{
    if (size < hb_size) { return MString::Comparei(stackbuf, string.c_str());
    } else return MString::Comparei(heapbuf.str, string.c_str());
}

bool MString::Comparei(const char *string, u64 length) const
{
    return Comparei(c_str(), string, length);
}

char *MString::Duplicate(const char *s)
{
    u64 length = Length(s);
    char* copy = (char*)MemorySystem::Allocate(sizeof(char) * (length + 1), Memory::String);
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
    u8 buf[20]{};   // 10 для i32 и u32
    u16 length = 0; // длина добавляемого числа к строке
    u32 nLen = 0;   // новая длина строки
    bool minus = false;
    char* str = nullptr; // Data(&len);

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

    if (size < hb_size && (nLen = size + length) < hb_size) {
        str = stackbuf; size += length;
    } else if (size < hb_size && nLen > hb_size) {
        size = nLen + 1;
        str = (char*)MemorySystem::Allocate(sizeof(char) * size, Memory::String);
        Copy(str, stackbuf);
        heapbuf.length = nLen;
    } else if (size > hb_size && (nLen = heapbuf.length + length) > size) {
        str = (char*)MemorySystem::Allocate(sizeof(char) * (nLen + 1), Memory::String);
        MemorySystem::Free(heapbuf.str, size, Memory::String);
        size = nLen + 1; 
        heapbuf.length = nLen; 
        heapbuf.str = str;
    } else if(size > hb_size && nLen < size) {
        str = heapbuf.str; 
        heapbuf.length = nLen;
    }

    str[nLen] = '\0';

    for (u32 i = 20 - length, j = 0; i < 20; i++) {
        str[j] = buf[i];
        j++;
    }
    return *this;
}

static bool StringToInt(const char *s, i64& num, u32 length)
{

    if (!s) { return false; }
    
    num = 0;
    bool sign = false;
    if (*s == '-') {
        sign = true;
        s++;
    }

    for (u32 i = 0; length == 0 ? s[i] != '\0' : i < length; i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            num += s[i] - '0';
            num *= 10;
        }
    }
        
    sign ? num /= -10 : num /= 10;
    return true;
}

static bool StringToInt(const char *s, u64 &num, u32 length)
{
    if (!s) { return false; }
    
    num = 0;
    u16 n = 0; // число цифр
    // bool condition = length == 0 ? 
    
    for (u32 i = 0; length == 0 ? s[i] != '\0' : i < length; i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            num += s[i] - '0';
            num *= 10; n++;
        }
    }

    num /= 10;
    return n;
}

bool MString::ToInt(const char *s, i64 &num, u32 length)
{
    return StringToInt(s, num, length);
}

bool MString::ToInt(const char *s, i32 &num, u32 length)
{
    i64 val;
    if (StringToInt(s, val, length) && val <= INT32_MAX) {
        num = val; return true;
    }
    
    return false;
}

bool MString::ToInt(const char *str, u32 &num, u32 length)
{
    u64 val;
    if (StringToInt(str, val, length) && val <= UINT32_MAX) {
        num = val; return true;
    }
    
    return false;
}

bool MString::ToF32(const char *str, f32 &num, u32 length)
{
    return StringToF32(str, num, length);
}

bool MString::ToInt(const char *s, u64 &num, u32 length)
{
    return StringToInt(s, num, length);
}

bool MString::StringToF32(const char* string, f32(&fArray)[] , u32 aSize, u32 length)
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

    for (u32 i = 0; length == 0 ? string[i] != '\0' : i < length; i++) {
        switch (string[i]) {
            case '-': {
                sign = true;
            } break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                buffer += string[i] - '0';
                buffer *= 10;
                if (mantissa) {
                    factor *= 10;
                }
            } break;
            case '.': case ',': {
                mantissa = true;
            } break;
            case ' ': case '\0': {
                end();
                buffer   = 0;
                factor   = 10;
                sign     = false;
                mantissa = false;
            } break;
            default: {
            } break;
        }
    }
    
    return end();
}

bool MString::StringToF32(const char* s, f32& num, u32 length)
{
    if(*s != '-' && (*s < '0' || *s > '9' )) {
        return false;
    }
    
    // ЗАДАЧА: Убрать лишние проверки
    u8 count = 1;
    f64  buffer   = 0; 
    u32  factor   = 10;
    bool sign     = false;
    bool mantissa = false;

    auto end      = [&]() {
        buffer /= factor;
        if (sign) { buffer *= -1.F; }
        if (count == 1) {
            num = buffer;
            return true;
        }
        return false;
    };

    num = 0;
    while (*s != '\0') {
        switch (*s) {
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
                dest[i] = '\0';
                return dest;
            } break;
            default: {
                dest[i] = source[i];
            } break;
            }
        }
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
    u32 nlen = 0;
    char* str = nullptr;

    if(size < hb_size && (nlen = size + source.Length()) < hb_size) {
        str = stackbuf;
    } else if (size < hb_size && nlen > hb_size) {
        str = heapbuf.str = (char*)MemorySystem::Allocate(sizeof(char) * (size = nlen + 1), Memory::String);
        heapbuf.length = nlen;
    } else if (size > hb_size && (nlen = size + source.Length()) > size) {
        MemorySystem::Free(heapbuf.str, size, Memory::String);
        str = heapbuf.str = (char*)MemorySystem::Allocate(sizeof(char) * (size = nlen + 1), Memory::String);
        heapbuf.length = nlen;
    } else if (size > hb_size && nlen < size) {
        str = heapbuf.str;
        heapbuf.length = nlen;
    }

    Copy(str, source.c_str(), length);
}

void MString::Trim()
{
    char* data = pData();
    data = MString::Trim(data);
}

char *MString::Trim(char *s, u64* length)
{
    /* while (isspace((unsigned char)*s)) {
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
    } */

    u8 shift = 0;
    u32 i = 0; 

    for (; s[i] != '\0'; i++) {
        if ((s[i] == '\a' || s[i] == '\b' || s[i] == '\t' || s[i] == '\n' || 
             s[i] == '\v' || s[i] == '\f' || s[i] == '\r')) {
            shift++;
        } else if (shift) {
            s[i - shift] = s[i];
        }
    }

    s[i - shift] = s[i];
    if (length) {
        *length = i - shift;
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

Substring MString::Mid(u32 start, u32 length)
{      
    u32 sbsize = 0;
    char* s = pData() + start;

    length = length == 0 ? Length() - start : length;

    for (u32 i = 0, charSize = 1; i < length; i++, sbsize += charSize) {
        charSize = CheckSymbol(s[sbsize]);
    }
    
    return Substring(sbsize, length, s);
}

void MString::Mid(const MString &source, u32 start, i32 length)
{
    Mid(pData(), source, start, length);
}

void MString::Mid(char *dest, const MString &source, u32 start, i32 length)
{
    if (length == 0) { return; }

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
    if (size < hb_size) {
        MWARN("Строку мньше %d можно только скопировать!", hb_size);
        return nullptr;
    }
       
    heapbuf.length = size = 0;
    char* NewStr = heapbuf.str;
    heapbuf.str = nullptr;
    return NewStr;
}

i32 MString::IndexOf(const char *str, char c, u32 length)
{
    if (!str) { return -1; }

    if (length == 0) {
        length = Length(str);
    }
    
    if (length > 0) {
        for (u32 i = 0; i < length; ++i) {
            if (str[i] == c) {
                return i;
            }
        }
    }

    return -1;
}

bool MString::ParseArrayLength(u32 &ArrayLength)
{
    char* str = pData();
    i32 OpenIndex = IndexOf(str, '[');
    i32 CloseIndex = IndexOf(str, ']');
    if (OpenIndex == -1 || CloseIndex == -1) {
        return false;
    }

    // Извлеките текст из скобок.
    char NumString[20]{};
    Mid(NumString, str, OpenIndex + 1, CloseIndex - OpenIndex);

    return ToInt(NumString, ArrayLength);
}

bool MString::ToInt(u32 &value)
{
    u64 val;
    if (StringToInt(c_str(), val, 0) && val <= UINT32_MAX) {
        value = val; return true;
    }
    
    return false;
}

bool MString::ToVector(char *s, FVec4 &OutVector)
{
    if (!s) {
        return false;
    }

    i32 result = sscanf(s, "%f %f %f %f", &OutVector.x, &OutVector.y, &OutVector.z, &OutVector.w);
    return result != -1;
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

bool MString::ToBool(bool &b)
{
    b = (*this == "1") || (*this == "true");
    return true;
}

u32 MString::Split(const char *str, char delimiter, DArray<MString> &darray, bool TrimEntries, bool IncludeEmpty)
{
    if (!str) { return 0; }

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
                    darray.PushBack((MString&&)result);
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

u32 MString::Split(char delimiter, DArray<MString> &darray, bool TrimEntries, bool IncludeEmpty)
{
    u32 EntryCount = 0;
    u32 l = Length();
    const char* s = Data();

    u32 start = 0;

    for (u32 i = 0; i <= l; i++) {
        if (s[i] == delimiter || s[i] == 0) {
            MString ns { Mid(start, i) };
            // Добавить новую запись
            if (!IncludeEmpty && !ns) {        
                continue;
            } 
            darray.PushBack((MString&&)ns);
            start = i + 1;
            EntryCount++;
        }
    }

    return EntryCount;
}

u32 MString::Split(char delimiter, DArray<MString> &darray, bool TrimEntries, bool IncludeEmpty) const
{
    u32 EntryCount = 0;
    u32 l = Length();
    const char* s = Data();

    u32 start = 0;

    for (u32 i = 0; i <= l; i++) {
        if (s[i] == delimiter || s[i] == 0) {
            MString ns { s + start, i - start };
            // Добавить новую запись
            if (!IncludeEmpty && !ns) {        
                continue;
            } 
            darray.PushBack((MString&&)ns);
            start = i + 1;
            EntryCount++;
        }
    }

    return EntryCount;
}

u32 MString::Split(char delimiter, DArray<Substring>& darray, bool TrimEntries, bool IncludeEmpty)
{
    u32 EntryCount = 0;
    u32 l = 0;
    char* s = pData();

    u32 start = 0;

    for (u32 i = 0; i <= size; i++) {
        if (s[i] == delimiter || s[i] == 0) {
            Substring ns { 
                .size = i - start, 
                .length = l,
                .data = s + start 
            };
            // Добавить новую запись
            if (!IncludeEmpty && !ns) {        
                continue;
            } 
            darray.PushBack(ns);
            start = i + 1;
            EntryCount++;
        }
        l += CheckSymbol(s[i]);
    }

    return EntryCount;
}

void MString::Destroy()
{
    if (size > hb_size) {
        MemorySystem::Free(heapbuf.str, size, Memory::String);  
        heapbuf.length = size = capacity = 0;
        heapbuf.str = nullptr;
    } else size = capacity = 0;
}

MString MString::CopyPart(u32 position, u32 length) const
{
    u32 remaining = size - position - length;
    MString string {position + length + 1};
    //const char* str = nullptr; 

    // s - размер, ch - номер символа
    // for (u32 s = 0, u32 ch = 0; i < position; s++, ch++) {
    //     str = 
    // }
    

    if (position > 0) {
        MemorySystem::CopyMem(string.pData(), c_str(), sizeof(u8) * position);
    }

    if (position < size) {
        MemorySystem::CopyMem(string.pData() + position, c_str() + position + length, sizeof(u8) * remaining);
    }

    return string;
}

void MString::InsertChar(u32 position, char ch)
{
    u32& length = capacity < hb_size ? size : heapbuf.length;
    char* str = pData();
    
    u32 remaining = 0, pos = 0;
    for (u32 i = 0, ii = 0; str[ii]; i++) {
        u8 CharSize = CheckSymbol(str[ii]);
        ii += CharSize;
        if (i < position) {
            pos += CharSize;
        } else {
            remaining += CharSize;
        }
    }

    if (position < length) {
        MemorySystem::CopyMem(str + pos + 1, str + pos, sizeof(char) * remaining);
    }

    str[pos] = ch;
    length++;
}

void MString::RemoveFirstChar(u32 count)
{
    char* str = pData();
    u32 size = 0; // размер сиволов которые нужно удалить

    for (u32 i = 0; i < count; i++) {
        u8 cs = CheckSymbol(str[i]);
        size += cs;
    }

    u32 len = Length();
    for (u32 i = 0; i < len + 1; i++) {
        str[i] = str[size];
    }
    
}

void MString::RemoveLastChar(u32 count)
{
    u32 len = this->Length() - 1;
    char* str = pData();
    
    if (size > hb_size) {
        heapbuf.length = len;
    }
    
    u32 i = 0;
    for (u8 cs = 0; i < len; i+=cs) {
        cs = CheckSymbol(str[i]);
    }

    str[i] = '\0';
    
}

void MString::RemovePart(u32 position, u32 length)
{
    u32& len = capacity < hb_size ? size : heapbuf.length;
    char* str = pData();
    
    if (len < length) {
        len = 0;
        *str = '\0';
        return;
    }

    u32 ActualPosition = 0, del = 0; 
    for (u32 i = 0, count = position + length; i < count; i++) {
        u8 cs = CheckSymbol(str[del]);
        ActualPosition = i < position ? ActualPosition + cs : ActualPosition;
        del += cs; // Позиция первого символа после удаления
    }
    
    u32 remaining = size - del; // Количество байт которые нужно скопировать

    if (position < size) {
        MemorySystem::CopyMem(&str[ActualPosition], &str[del], sizeof(char) * remaining);
    }
    
    len -= length;
}

constexpr void MString::Copy(const char *source, u64 size, u64 length, bool DelCon)
{ // size < this->size {  } и size > this->size { }
    if(!source || !size) { return; }

    if (capacity >= size) {
        this->size = size;
        if (capacity <= hb_size) {
            Copy(stackbuf, source, size, DelCon);
        } else {
            heapbuf.length = length;
            Copy(heapbuf.str, source, size, DelCon);
        }
    } else { // capacity < size
        if (size < hb_size) {
            this->size = length;
            Copy(stackbuf, source, size, DelCon);
        } else {
            if (capacity <= hb_size) {
                this->size = capacity = size;
                heapbuf.length = length;
                heapbuf.str = (char*)MemorySystem::Allocate(size * sizeof(char), Memory::String); 
                Copy(heapbuf.str, source, size, DelCon);
            } else {
                MemorySystem::Free(heapbuf.str, capacity * sizeof(char), Memory::String);
                heapbuf.str = (char*)MemorySystem::Allocate(size * sizeof(char), Memory::String);
                Copy(heapbuf.str, source, size, DelCon);
                heapbuf.length = length;
                this->size = capacity = size;
            }
        }
    }
    

    // if (size < hb_size) {
    //     if (this->size < hb_size) {
    //         this->size = length;
    //         Copy(stackbuf, source, length, DelCon);
    //     } else {
    //         heapbuf.length = length;
    //         Copy(heapbuf.str, source, length, DelCon);
    //     }
    // } else {
    //     if (this->size < hb_size) {
    //         this->size = size;
    //         heapbuf.length = length;
    //         heapbuf.str = (char*)MemorySystem::Allocate(size * sizeof(char), Memory::String); 
    //         Copy(heapbuf.str, source, size, DelCon);
    //     } else {
    //         if (this->size >= size) {
    //             heapbuf.length = length;
    //             Copy(heapbuf.str, source, size, DelCon);
    //             // heapbuf.str[size - 1] = '\0';
    //         } else {
    //             MemorySystem::Free(heapbuf.str, this->size * sizeof(char), Memory::String);
    //             heapbuf.str = (char*)MemorySystem::Allocate(size * sizeof(char), Memory::String);
    //             Copy(heapbuf.str, source, size, DelCon);
    //             heapbuf.length = length;
    //             this->size = size;
    //         }
    //     }
    // }

    // if (this->size > hb_size) {
    //     if (size < this->size) {
    //         heapbuf.length = length;
    //         Copy(heapbuf.str, source, size, DelCon);
    //         heapbuf.str[size] = '\0';
    //     } else {
    //         MemorySystem::Free(heapbuf.str, this->size * sizeof(char), Memory::String);
    //             heapbuf.str = (char*)MemorySystem::Allocate(size * sizeof(char), Memory::String);
    //             Copy(heapbuf.str, source, size, DelCon);
    //             heapbuf.length = length;
    //             this->size = size;
    //     }
    //     
    // }
}

// constexpr char *MString::Copy(const MString &source)
// {   
//     if (!source) { return nullptr; }
// 
//     if (source.size < hb_size) {
//         return heapbuf.str;
//     } else {
//         char* str = (char*)MemorySystem::Allocate(size * sizeof(char), Memory::String); 
//         Copy(str, source.heapbuf.str, size);
//         return str;
//     }
// }

constexpr char* MString::Concat(const char *str1, const char *str2, u32 length1, u32 length2)
{
    char* str = nullptr;
    if(!heapbuf.str && size > hb_size) {
        str = heapbuf.str = (char*)MemorySystem::Allocate(size, Memory::String);
    } else str = stackbuf;
    
    u64 count = length1 + length2;
    i32 j = 0 - length1;
    for (u64 i = 0; i < count; i++, j++) {
        str[i] = j < 0 ? str1[i] : str2[j];
    }
    str[count] = '\0';
    
    return str;
}

constexpr char *MString::pData(u32 *length)
{
    if (size < 0) { return nullptr; }
    bool b = size < hb_size;
    if (length) { *length = b ? size : heapbuf.length; }
    return b ? stackbuf : heapbuf.str;
}

u32 MString::CheckSymbol(const char &c)
{
    i32 ch = (i32)c;
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


void *MString::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::String);
}

void *MString::operator new[](u64 size)
{
    return MemorySystem::Allocate(size, Memory::String);
}

void MString::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::String);
}

void MString::operator delete[](void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::String);
}

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
    return ToTransform(pData(), transform);
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

MString operator+(const MString &ls, const char *rs)
{
    return MString(ls, rs);
}

MString operator+(const char *ls, const MString &rs)
{
    u32 lsLen = MString::Length(ls);
    MString result {lsLen + rs.Length() + 1};
    result = ls;
    result += rs;
    return result;
}

Substring::operator bool() const
{
    if (data) { return true;
    } else return false;
}

Substring Substring::Mid(u32 start, u32 length) const
{
    return Substring(length, length, data + start);
}

bool Substring::ParseArrayLength(u32 &ArrayLength)
{
    i32 OpenIndex = MString::IndexOf(data, '[', size);
    i32 CloseIndex = MString::IndexOf(data, ']', size);
    if (OpenIndex == -1 || CloseIndex == -1) {
        return false;
    }

    OpenIndex++;
    Substring temp = Mid(OpenIndex, CloseIndex - OpenIndex);

    return temp.ToInt(ArrayLength);
}

u32 Substring::Split(char delimiter, DArray<Substring> &darray, bool TrimEntries, bool IncludeEmpty)
{
    u32 EntryCount = 0;
    u32 l = size;

    u32 start = 0;
    u32 len = 0;

    for (u32 i = 0; i <= l; i++, len++) {
        if (data[i] == delimiter || data[i] == 0) {
            Substring ns { Mid(start, len) };
            len = 0;
            // Добавить новую запись
            if (!IncludeEmpty && !ns.data) {        
                continue;
            } 
            ns.size++;
            darray.PushBack(ns);
            EntryCount++;
            data[i] = '\0';
            i++;
            start = i;
        }
    }

    return EntryCount;
}

bool Substring::ToBool(bool &b)
{
    b = MString::Compare(data, "1", size) || MString::Compare(data, "true", size);
    return true;
}

bool Substring::ToInt(i8 &value)
{
    i64 val;
    if (StringToInt(data, val, length) && val <= INT8_MAX) {
        value = val; return true;
    }
    
    return false;
}

bool Substring::ToInt(u8 &value)
{
    u64 val;
    if (StringToInt(data, val, length) && val <= UINT8_MAX) {
        value = val; return true;
    }
    
    return false;
}

bool Substring::ToInt(i16 &value)
{
    i64 val;
    if (StringToInt(data, val, length) && val <= INT16_MAX) {
        value = val; return true;
    }
    
    return false;
}

bool Substring::ToInt(u16 &value)
{
    u64 val;
    if (StringToInt(data, val, length) && val <= UINT16_MAX) {
        value = val; return true;
    }
    
    return false;
}

bool Substring::ToInt(i32 &value)
{
    i64 val;
    if (StringToInt(data, val, length) && val <= INT32_MAX) {
        value = val; return true;
    }
    
    return false;
}

bool Substring::ToMatrix(Matrix4D &value)
{
    // ЗАДАЧА: вставить код
    return false;
}
