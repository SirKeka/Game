#pragma once

#include "defines.hpp"
#include "platform/filesystem.hpp"

#define LOG_WARN_ENABLED  1
#define LOG_INFO_ENABLED  1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// Отключите ведение журнала отладки и трассировки для сборок выпуска.
#if MRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

namespace Log {
    enum Level { Fatal, Error, Warn, Info, Debug, Trace };

    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();
    
    MAPI void Output(Log::Level level, const char* message, ...);

    void AppendToLogFile(const char* message);
}; // namespace Log

// Регистрирует сообщение критического уровня.
#define MFATAL(message, ...) Log::Output(Log::Level::Fatal, message, ##__VA_ARGS__);

#ifndef MERROR
/// @brief Регистрирует сообщение об уровне ошибки.
#define MERROR(message, ...) Log::Output(Log::Level::Error, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
// Регистрирует сообщение уровня предупреждения.
#define MWARN(message, ...) Log::Output(Log::Level::Warn, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_WARN_ENABLED != 1
#define MWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
// Регистрирует сообщение информационного уровня.
#define MINFO(message, ...) Log::Output(Log::Level::Info, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define MINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Регистрирует сообщение уровня отладки.
#define MDEBUG(message, ...) Log::Output(Log::Level::Debug, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_DEBUG_ENABLED != 1
#define MDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// Регистрирует сообщение уровня трассировки.
#define MTRACE(message, ...) Log::Output(Log::Level::Trace, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_TRACE_ENABLED != 1
#define MTRACE(message, ...)
#endif
