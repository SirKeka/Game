#pragma once

// Беззнаковые типы int.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Знаковые типы int.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Типы с плавающей запятой
typedef float f32;
typedef double f64;

// Логические типы
typedef int b32;
typedef char b8;

// Правильно определять статические утверждения.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Убедитесь, что все типы имеют правильный размер..
STATIC_ASSERT(sizeof(u8) == 1, "Ожидаемый u8 будет 1 байт.");
STATIC_ASSERT(sizeof(u16) == 2, "Ожидаемый размер u16 составит 2 байта.");
STATIC_ASSERT(sizeof(u32) == 4, "Ожидаемый размер u32 составит 4 байта.");
STATIC_ASSERT(sizeof(u64) == 8, "Ожидаемый размер u64 составит 8 байт.");

STATIC_ASSERT(sizeof(i8) == 1, "Ожидаемый i8 будет 1 байт.");
STATIC_ASSERT(sizeof(i16) == 2, "Ожидаемый размер i16 составит 2 байта.");
STATIC_ASSERT(sizeof(i32) == 4, "Ожидаемый i32 будет 4 байта.");
STATIC_ASSERT(sizeof(i64) == 8, "Ожидаемый i64 будет 8 байт.");

STATIC_ASSERT(sizeof(f32) == 4, "Ожидается, что f32 будет иметь размер 4 байта.");
STATIC_ASSERT(sizeof(f64) == 8, "Ожидается, что f64 будет иметь размер 8 байт.");

#define TRUE 1
#define FALSE 0

/* Любой установленный для этого идентификатор следует считать 
 * недействительным и фактически не указывающим на реальный объект. */
#define INVALID_ID 4294967295U

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

#define MCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max \
                                                                      : value;

// Встраивание
#ifdef _MSC_VER
#define MINLINE __forceinline
#define MNOINLINE __declspec(noinline)
#else
#define MINLINE static inline
#define MNOINLINE
#endif