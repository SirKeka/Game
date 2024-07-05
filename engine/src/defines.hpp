#pragma once

// Беззнаковые типы int.
typedef unsigned char u8;       // Беззнаковое  8-битное целое число
typedef unsigned short u16;     // Беззнаковое 16-битное целое число
typedef unsigned int u32;       // Беззнаковое 32-битное целое число
typedef unsigned long long u64; // Беззнаковое 64-битное целое число

// Знаковые типы int.
typedef signed char i8;         //  8-битное целое число со знаком
typedef signed short i16;       // 16-битное целое число со знаком
typedef signed int i32;         // 32-битное целое число со знаком
typedef signed long long i64;   // 64-битное целое число со знаком

// Типы с плавающей запятой
typedef float f32;              // 32-битное число с плавающей запятой
typedef double f64;             // 64-битное число с плавающей запятой

// Логические типы
typedef int b32;                // 32-битный логический тип, используемый для API, которые этого требуют.

// Правильно определять статические утверждения.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Убедитесь, что все типы имеют правильный размер..
STATIC_ASSERT(sizeof(u8) == 1, "Ожидаемый u8 будет 1 байт.");                       // Утверждение, что  u8 имеет размер 1 байт.
STATIC_ASSERT(sizeof(u16) == 2, "Ожидаемый размер u16 составит 2 байта.");          // Утверждение, что u16 имеет размер 2 байта.
STATIC_ASSERT(sizeof(u32) == 4, "Ожидаемый размер u32 составит 4 байта.");          // Утверждение, что u32 имеет размер 4 байта.
STATIC_ASSERT(sizeof(u64) == 8, "Ожидаемый размер u64 составит 8 байт.");           // Утверждение, что u64 имеет размер 8 байт.

STATIC_ASSERT(sizeof(i8) == 1, "Ожидаемый i8 будет 1 байт.");                       // Утверждение, что i8  имеет размер 1 байт.
STATIC_ASSERT(sizeof(i16) == 2, "Ожидаемый размер i16 составит 2 байта.");          // Утверждение, что i16 имеет размер 2 байта.
STATIC_ASSERT(sizeof(i32) == 4, "Ожидаемый i32 будет 4 байта.");                    // Утверждение, что i32 имеет размер 4 байта.
STATIC_ASSERT(sizeof(i64) == 8, "Ожидаемый i64 будет 8 байт.");                     // Утверждение, что i64 имеет размер 8 байт.

STATIC_ASSERT(sizeof(f32) == 4, "Ожидается, что f32 будет иметь размер 4 байта.");  // Утверждение, что f32 имеет размер 4 байта.
STATIC_ASSERT(sizeof(f64) == 8, "Ожидается, что f64 будет иметь размер 8 байт.");   // Утверждение, что f64 имеет размер 8 байта.

// Любой установленный для этого идентификатор следует считать 
// недействительным и фактически не указывающим на реальный объект.
namespace INVALID
{   
    constexpr u8  U8ID = 255u;
    constexpr u16 U16ID = 65535u;
    constexpr u32 ID = 4294967295u;
} // namespace INVALID

//#define INVALID_ID 4294967295U
//#define INVALID_ID_U16 65535U
//#define INVALID_ID_U8 255U

// Обнаружение платформы
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define MPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "Для Windows требуется 64-битная версия!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define MPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define MPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Поймайте все, что не поймано вышеперечисленным.
#define MPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define MPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define MPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define MPLATFORM_IOS 1
#define MPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define MPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Неизвестная платформа Apple"
#endif
#else
#error "Неизвестная платформа!"
#endif

#ifdef MEXPORT
// Exports
#ifdef _MSC_VER
#define MAPI __declspec(dllexport)
#else
#define MAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define MAPI __declspec(dllimport)
#else
#define MAPI
#endif
#endif

#define MCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;

// Встраивание
#ifdef _MSC_VER
#define MINLINE __forceinline
#define MNOINLINE __declspec(noinline)
#else
#define MINLINE static inline
#define MNOINLINE
#endif

#define GIBIBYTES(amount) amount * 1024 * 1024 * 1024   // Получает количество байтов из количества гибибайтов (GiB) (1024*1024*1024)
#define MEBIBYTES(amount) amount * 1024 * 1024          // Получает количество байтов из количества мебибайтов (MiB) (1024*1024)
#define KIBIBYTES(amount) amount * 1024                 // Получает количество байтов из количества кибибайтов (KiB) (1024)

#define GIGABYTES(amount) amount * 1000 * 1000 * 1000   // Получает количество байтов из количества гигабайт    (GB) (1000*1000*1000)
#define MEGABYTES(amount) amount * 1000 * 1000          // Получает количество байтов из количества мегабайт    (MB) (1000*1000)
#define KILOBYTES(amount) amount * 1000                 // Получает количество байтов из количества килобайт    (KB) (1000)

#define VARIABLE_TO_STRING(Variable) (void(Variable),#Variable)   // Проверяет существует ли переменная, если да, то преобразует в строку.
#define TYPE_TO_STRING(Type) (void(sizeof(Type)),#Type)           // Проверяет существует ли тип, если да, то преобразует в строку.

//ЗАДАЧА: возможно здесь этому не место
/// @brief Диапазон, обычной памяти
struct Range {
    u64 offset; // Смещение в байтах
    u64 size;   // Размер в байтах.

    constexpr Range() : offset(), size() {}
    constexpr Range(u64 offset, u64 size) : offset(offset), size(size) {}
    constexpr Range(u64 offset, u64 size, u64 granularity) : offset(GetAligned(offset, granularity)), size(GetAligned(size, granularity)) {}

    static MINLINE u64 GetAligned(u64 operand, u64 granularity) {
        return ((operand + (granularity - 1)) & ~(granularity - 1));
    }

    static MINLINE Range GetAlignedRange(u64 offset, u64 size, u64 granularity) {
        return Range{GetAligned(offset, granularity), GetAligned(size, granularity)};
    }
};