#pragma once

#include "defines.hpp"

// Отключите утверждения, закомментировав строку ниже.
#define MASSERTIONS_ENABLED

#ifdef MASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

MAPI void ReportAssertionFailure(const char* expression, const char* message, const char* file, i32 line);

#define MASSERT(expr)                                                \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            ReportAssertionFailure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }

#define MASSERT_MSG(expr, message)                                        \
    {                                                                     \
        if (expr) {                                                       \
        } else {                                                          \
            ReportAssertionFailure(#expr, message, __FILE__, __LINE__); \
            debugBreak();                                                 \
        }                                                                 \
    }

#ifdef _DEBUG
#define MASSERT_DEBUG(expr)                                          \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            ReportAssertionFailure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }
#else
#define MASSERT_DEBUG(expr)  // Ничего не делает вообще
#endif

#else
#define MASSERT(expr)               // Ничего не делает вообще
#define MSSERT_MSG(expr, message)  // Ничего не делает вообще
#define MASSERT_DEBUG(expr)         // Ничего не делает вообще
#endif