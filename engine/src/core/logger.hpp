#pragma once

#include "defines.hpp"
#include "containers/mstring.hpp"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// Отключите ведение журнала отладки и трассировки для сборок выпуска.
#if MRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

enum LogLevel {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
};

bool InitializeLogging();
void ShutdownLogging();

MAPI void LogOutput(LogLevel level, const char* message, ...);
MAPI void LogOutput(LogLevel level, MString message, ...);

// Регистрирует сообщение критического уровня.
#define MFATAL(message, ...) LogOutput(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);

#ifndef MERROR
// Регистрирует сообщение об уровне ошибки.
#define MERROR(message, ...) LogOutput(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
// Регистрирует сообщение уровня предупреждения.
#define MWARN(message, ...) LogOutput(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_WARN_ENABLED != 1
#define MWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
// Регистрирует сообщение информационного уровня.
#define MINFO(message, ...) LogOutput(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define MINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Регистрирует сообщение уровня отладки.
#define MDEBUG(message, ...) LogOutput(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_DEBUG_ENABLED != 1
#define MDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// Регистрирует сообщение уровня трассировки.
#define MTRACE(message, ...) LogOutput(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_TRACE_ENABLED != 1
#define MTRACE(message, ...)
#endif