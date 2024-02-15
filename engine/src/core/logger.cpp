#include "logger.hpp"
#include "asserts.hpp"
#include "platform/platform.hpp"

// TODO: временное
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

bool InitializeLogging() 
{
    // TODO: создать файл журнала.
    return TRUE;
}

void ShutdownLogging() 
{
    // TODO: очистка журнала/запись записей в очереди.
}

void LogOutput(LogLevel level, const char* message, ...) 
{
    const char* LevelStrings[6] = {"[FATAL]: ", "[ОШИБКА]: ", "[ПРЕДУПРЕЖДЕНИЕ]:  ", "[ИНФО]:  ", "[ОТЛАДКА]: ", "[TRACE]: "};
    b8 is_error = LOG_LEVEL_WARN;

    // Технически накладывает ограничение на длину одной записи журнала в 32 тыс. символов, но...
    // НЕ ДЕЛАЙТЕ ЭТОГО!
    const i32 MsgLength = 32000;
    char* OutMessage = new char[MsgLength]();

    // Отформатируйте исходное сообщение.
    // ПРИМЕЧАНИЕ. Как ни странно, заголовки MS в некоторых случаях переопределяют тип va_list GCC/Clang 
    // с помощью «typedef char* va_list», и в результате здесь выдается странная ошибка. Обходной путь 
    // на данный момент — просто использовать __builtin_va_list, который является типом, ожидаемым va_start GCC/Clang.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(OutMessage, MsgLength, message, arg_ptr);
    va_end(arg_ptr);

    char OutMessage2[MsgLength];
    sprintf(OutMessage2, "%s%s\n", LevelStrings[level], OutMessage);

    // Platform-specific output.
    if (is_error) {
        PlatformConsoleWriteError(OutMessage2, level);
    } else {
        PlatformConsoleWrite(OutMessage2, level);
    }
}

void LogOutput(LogLevel level, MString message, ...)
{
    const char* LevelStrings[6] = {"[FATAL]: ", "[ОШИБКА]: ", "[ПРЕДУПРЕЖДЕНИЕ]:  ", "[ИНФО]:  ", "[ОТЛАДКА]: ", "[TRACE]: "};
    b8 is_error = LOG_LEVEL_WARN;

    // Технически накладывает ограничение на длину одной записи журнала в 32 тыс. символов, но...
    // НЕ ДЕЛАЙТЕ ЭТОГО!
    const i32 MsgLength = 32000;
    char* OutMessage = new char[MsgLength]();

    // Отформатируйте исходное сообщение.
    // ПРИМЕЧАНИЕ. Как ни странно, заголовки MS в некоторых случаях переопределяют тип va_list GCC/Clang 
    // с помощью «typedef char* va_list», и в результате здесь выдается странная ошибка. Обходной путь 
    // на данный момент — просто использовать __builtin_va_list, который является типом, ожидаемым va_start GCC/Clang.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(OutMessage, MsgLength, message.c_str(), arg_ptr);
    va_end(arg_ptr);

    char OutMessage2[MsgLength];
    sprintf(OutMessage2, "%s%s\n", LevelStrings[level], OutMessage);

    // Platform-specific output.
    if (is_error) {
        PlatformConsoleWriteError(OutMessage2, level);
    } else {
        PlatformConsoleWrite(OutMessage2, level);
    }
}

void ReportAssertionFailure(const char* expression, const char* message, const char* file, i32 line) 
{
    LogOutput(LOG_LEVEL_FATAL, "Ошибка утверждения: %s, сообщение: '%s', в файле: %s, строка: %d\n", expression, message, file, line);
}