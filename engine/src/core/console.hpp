#pragma once
#include "defines.hpp"
#include "logger.hpp"
#include "containers/darray.hpp"

class LinearAllocator;

/// @brief Функция записи потребителя консоли, которая вызывается каждый раз, когда происходит событие регистрации. Потребители должны реализовать это и обрабатывать ввод таким образом.
using PFN_ConsoleConsumerWrite = bool(*)(void*, Log::Level, const char*);

/// @brief Представляет значение аргумента одной консольной команды. Всегда представлено в виде строки, функция консольной команды должна интерпретировать и преобразовывать его в требуемый тип во время обработки.
struct ConsoleCommandArgument {
    const char* value;
};

/// @brief Контекст, который должен передаваться вместе с выполняемой консольной командой (т. е. аргументы команды).
struct ConsoleCommandContext {
    /// @brief Количество переданных аргументов.
    u8 ArgumentCount;
    /// @brief Массив аргументов.
    ConsoleCommandArgument* arguments;
};

/// @brief Указатель функции, который представляет зарегистрированную консольную команду и вызывается при срабатывании какого-либо средства консольного ввода (обычно консоли отладки).
using PFN_ConsoleCommand = void(*)(ConsoleCommandContext);

namespace Console
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config); // LinearAllocator& SystemAllocator
    void Shutdown();

    MAPI void RegisterConsumer(void* inst, PFN_ConsoleConsumerWrite callback, u8& OutConsumerID);

    /// @brief Обновляет экземпляр и обратный вызов для потребителя с заданным идентификатором.
    /// @param ConsumerID идентификатор потребителя для обновления.
    /// @param inst экземпляр потребителя.
    /// @param callback новая функция обратного вызова.
    MAPI void UpdateConsumer(u8 ConsumerID, void* inst, PFN_ConsoleConsumerWrite callback);

    void WriteLine(Log::Level level, const char* message);

    MAPI bool RegisterCommand(const char* command, u8 ArgCount, PFN_ConsoleCommand func);
    
    /// @brief Отменяет регистрацию данной команды.
    /// @param command имя команды, регистрация которой отменяется.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool UnregisterCommand(const char* command);

    MAPI bool ExecuteCommand(const MString& command);
};

