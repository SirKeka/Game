#include "logger.hpp"
#include "asserts.hpp"
#include "platform/platform.hpp"

// TODO: временное
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
    if (!Filesystem::Open("console.log", FILE_MODE_WRITE, false, &LogFileHandle)) {
        PlatformConsoleWriteError("ОШИБКА: Не удается открыть console.log для записи.", LOG_LEVEL_ERROR);
        return false;
    }

    // TODO: Удали это
    MFATAL("Тестовое сообщение: %f", 3.14f);
    MERROR("Тестовое сообщение: %f", 3.14f);
    MWARN("Тестовое сообщение: %f", 3.14f);
    MINFO("Тестовое сообщение: %f", 3.14f);
    MDEBUG("Тестовое сообщение: %f", 3.14f);
    MTRACE("Тестовое сообщение: %f", 3.14f);

    return true;
}

void Logger::Shutdown() 
{
    // TODO: очистка журнала/запись записей в очереди.
    this->~Logger();
}

void *Logger::operator new(u64 size)
{
    return LinearAllocator::Allocate(size);
}

void Logger::AppendToLogFile(MString message)
{
    if (LogFileHandle.IsValid) {
        // Поскольку сообщение уже содержит '\n', просто запишите байты напрямую.
        u64 length = message.Length();
        u64 written = 0;
        if (!Filesystem::Write(&LogFileHandle, length, message.c_str(), &written)) {
            PlatformConsoleWriteError("ОШИБКА записи в console.log.", LOG_LEVEL_ERROR);
        }
    }
}

void Logger::Output(LogLevel level, MString message, ...)
{
    // TODO: Все эти строковые операции выполняются довольно медленно. В конечном 
    // итоге это необходимо переместить в другой поток вместе с записью файла, 
    // чтобы избежать замедления процесса во время попытки запуска движка.
    const char* LevelStrings[6] = {"[FATAL]: ", "[ОШИБКА]: ", "[ПРЕДУПРЕЖДЕНИЕ]:  ", "[ИНФО]:  ", "[ОТЛАДКА]: ", "[TRACE]: "};
    bool IsError = LOG_LEVEL_WARN;

    // Технически накладывает ограничение на длину одной записи журнала в 32 тыс. символов, но...
    // НЕ ДЕЛАЙТЕ ЭТОГО!
    char OutMessage[32000] {};

    // Отформатируйте исходное сообщение.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    StringFormatV(OutMessage, message.c_str(), arg_ptr);
    va_end(arg_ptr);

    // Добавить уровень журнала к сообщению.
    StringFormat(OutMessage, "%s%s\n", LevelStrings[level], OutMessage);

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
    Logger::Output(LOG_LEVEL_FATAL, "Ошибка утверждения: %s, сообщение: '%s', в файле: %s, строка: %d\n", expression, message, file, line);
}
