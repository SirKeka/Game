#include "logger.hpp"
#include "asserts.hpp"
#include "platform/platform.hpp"
#include "memory/linear_allocator.hpp"

// ЗАДАЧА: временное
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

FileHandle Logger::LogFileHandle;

Logger::Logger()
{
    
}

bool Logger::Initialize()
{
    // Создайте новый/сотрите существующий файл журнала, затем откройте его.
    if (!Filesystem::Open("console.log", FileModes::Write, false, LogFileHandle)) {
        PlatformConsoleWriteError("ОШИБКА: Не удается открыть console.log для записи.", static_cast<u8>(LogLevel::Error));
        return false;
    }

    return true;
}

void Logger::Shutdown() 
{
    // ЗАДАЧА: очистка журнала/запись записей в очереди.
    //this->~Logger();
}

void Logger::AppendToLogFile(const char *message)
{
    if (LogFileHandle.IsValid) {
        // Поскольку сообщение уже содержит '\n', просто запишите байты напрямую.
        u64 length = MString::Length(message);
        u64 written = 0;
        if (!Filesystem::Write(LogFileHandle, length, message, written)) {
            PlatformConsoleWriteError("ОШИБКА записи в console.log.", static_cast<u8>(LogLevel::Error));
        }
    }
}

void Logger::Output(LogLevel level, const char *message, ...)
{
    // ЗАДАЧА: Все эти строковые операции выполняются довольно медленно. В конечном 
    // итоге это необходимо переместить в другой поток вместе с записью файла, 
    // чтобы избежать замедления процесса во время попытки запуска движка.
    const char* LevelStrings[6] = {"[FATAL]: ", "[ОШИБКА]: ", "[ПРЕДУПРЕЖДЕНИЕ]:  ", "[ИНФО]:  ", "[ОТЛАДКА]: ", "[TRACE]: "};
    bool IsError = static_cast<bool>(LogLevel::Warn);

    // Технически накладывает ограничение на длину одной записи журнала в 32 тыс. символов, но...
    // НЕ ДЕЛАЙТЕ ЭТОГО!
    char OutMessage[32000] {};

    // Отформатируйте исходное сообщение.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    MString::FormatV(OutMessage, message, arg_ptr);
    va_end(arg_ptr);

    const u8& lvl = static_cast<u8>(level);
    // Добавить уровень журнала к сообщению.
    MString::Format(OutMessage, "%s%s\n", LevelStrings[lvl], OutMessage);

    if (IsError) {
        PlatformConsoleWriteError(OutMessage, lvl);
    } else {
        PlatformConsoleWrite(OutMessage, lvl);
    }

    // Поставьте копию в очередь для записи в файл журнала.
    AppendToLogFile(OutMessage);
}

void ReportAssertionFailure(const char* expression, const char* message, const char* file, i32 line) 
{
    Logger::Output(LogLevel::Fatal, "Ошибка утверждения: %s, сообщение: '%s', в файле: %s, строка: %d\n", expression, message, file, line);
}
