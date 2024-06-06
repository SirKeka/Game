#pragma once

#include "defines.hpp"
#include "containers/mstring.hpp"
#include "platform/filesystem.hpp"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// Отключите ведение журнала отладки и трассировки для сборок выпуска.
#if MRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

enum class LogLevel : u8 { Fatal, Error, Warn, Info, Debug, Trace };

class Logger
{
private:
    static FileHandle LogFileHandle;
public:
    Logger();
    ~Logger() = default;

    bool Initialize();
    void Shutdown();
    
    MAPI static void Output(LogLevel level, const char* message, ...);

    MAPI void* operator new(u64 size);
    //MAPI void operator delete(void* ptr);
private:
    static void AppendToLogFile(const MString& message);
};

// Регистрирует сообщение критического уровня.
#define MFATAL(message, ...) Logger::Output(LogLevel::Fatal, message, ##__VA_ARGS__);

#ifndef MERROR
/// @brief Регистрирует сообщение об уровне ошибки.
#define MERROR(message, ...) Logger::Output(LogLevel::Error, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
// Регистрирует сообщение уровня предупреждения.
#define MWARN(message, ...) Logger::Output(LogLevel::Warn, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_WARN_ENABLED != 1
#define MWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
// Регистрирует сообщение информационного уровня.
#define MINFO(message, ...) Logger::Output(LogLevel::Info, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define MINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Регистрирует сообщение уровня отладки.
#define MDEBUG(message, ...) Logger::Output(LogLevel::Debug, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_DEBUG_ENABLED != 1
#define MDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// Регистрирует сообщение уровня трассировки.
#define MTRACE(message, ...) Logger::Output(LogLevel::Trace, message, ##__VA_ARGS__);
#else
// Ничего не делает, если LOG_TRACE_ENABLED != 1
#define MTRACE(message, ...)
#endif
