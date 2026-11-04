#include "logger.h"
#include "asserts.h"
#include "platform/platform.hpp"
#include "console.h"
#include "containers/mstring.h"

#include <new>

// ЗАДАЧА: временное
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

Log* Log::Initialize(u64& MemoryRequirement, void* memory)
{
    MemoryRequirement = sizeof(Log);

    if (!memory) {
        return nullptr;
    }
    
    logger = new(memory) Log();

    if (!logger) {
        return nullptr;
    }
    

    // Создайте новый/сотрите существующий файл журнала, затем откройте его.
    if (!Filesystem::Open("console.log", FileModes::Write, false, logger->LogFileHandle)) {
        PlatformConsoleWriteError("ОШИБКА: Не удается открыть console.log для записи.", Log::Level::Error);
        return nullptr;
    }

    return logger;
}

/*void Log::Shutdown() 
{
    // ЗАДАЧА: очистка журнала/запись записей в очереди.
    logger = nullptr;
}*/

void Log::AppendToLogFile(const char *message)
{
    if (logger && logger->LogFileHandle.IsValid) {
        // Поскольку сообщение уже содержит '\n', просто запишите байты напрямую.
        u64 length = MString::Length(message);
        u64 written = 0;
        if (!Filesystem::Write(logger->LogFileHandle, length, message, written)) {
            PlatformConsoleWriteError("ОШИБКА записи в console.log.", Log::Level::Error);
        }
    }
}

void Log::Output(Level level, const char *message, ...)
{
    // ЗАДАЧА: Все эти строковые операции выполняются довольно медленно. В конечном 
    // итоге это необходимо переместить в другой поток вместе с записью файла, 
    // чтобы избежать замедления процесса во время попытки запуска движка.
    const char* LevelStrings[6] = {"[FATAL]: ", "[ОШИБКА]: ", "[ПРЕДУПРЕЖДЕНИЕ]: ", "[ИНФО]: ", "[ОТЛАДКА]: ", "[TRACE]: "};
    bool IsError = level < Level::Warn;

    // Технически накладывает ограничение на длину одной записи журнала в 32 тыс. символов, но...
    // НЕ ДЕЛАЙТЕ ЭТОГО!
    char OutMessage[32000] {};

    // Отформатируйте исходное сообщение.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    MString::FormatV(OutMessage, message, arg_ptr);
    va_end(arg_ptr);

    // Добавить уровень журнала к сообщению.
    MString::Format(OutMessage, "%s%s\n", LevelStrings[level], OutMessage);

    // Передать сообщение в консоль.
    Console::WriteLine(level, OutMessage);

    if (IsError) {
        PlatformConsoleWriteError(OutMessage, level);
    } else {
        PlatformConsoleWrite(OutMessage, level);
    }

    // Поставьте копию в очередь для записи в файл журнала.
    AppendToLogFile(OutMessage);
}

void ReportAssertionFailure(const char* expression, const char* message, const char* file, i32 line) 
{
    Log::Output(Log::Level::Fatal, "Ошибка утверждения: %s, сообщение: '%s', в файле: %s, строка: %d\n", expression, message, file, line);
}
