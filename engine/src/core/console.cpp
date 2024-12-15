#include "console.hpp"
#include "memory/linear_allocator.hpp"
#include "asserts.hpp"
#include <new>

struct Consumer {
    PFN_ConsoleConsumerWrite callback;
    void* instance;
};

struct Command {
    MString name;
    u8 ArgCount;
    PFN_ConsoleCommand func;
};

struct sConsole
{
    u8 ConsumerCount;
    Consumer* consumers;

    DArray<Command> RegisteredCommands;

    constexpr sConsole(Consumer* consumers) : ConsumerCount(), consumers(consumers), RegisteredCommands() {}
};

constexpr u32 MAX_CONSUMER_COUNT = 10;

static sConsole* pConsole = nullptr;

bool Console::Initialize(u64& MemoryRequirement, void* memory, void* config) // LinearAllocator &SystemAllocator
{
    MemoryRequirement = sizeof(sConsole) + sizeof(Consumer) * MAX_CONSUMER_COUNT;
    
    if (!memory) {
        return true;
    }

    auto ConsumerMem = reinterpret_cast<Consumer*>(reinterpret_cast<u8*>(memory) + sizeof(sConsole));

    pConsole = new(memory) sConsole(ConsumerMem);
    if (!pConsole) {
        return false;
    }
    
    return true;
}

void Console::Shutdown()
{
    if (pConsole) {
        pConsole->RegisteredCommands.~DArray();

        MemorySystem::ZeroMem(pConsole, sizeof(sConsole) + (sizeof(Consumer) * MAX_CONSUMER_COUNT));
    }

    pConsole = nullptr;
}

void Console::RegisterConsumer(void *inst, PFN_ConsoleConsumerWrite callback, u8& OutConsumerID)
{
    if (pConsole) {
        MASSERT_MSG(pConsole->ConsumerCount + 1 < MAX_CONSUMER_COUNT, "Достигнуто максимальное количество потребителей консолей.");

        auto& consumer = pConsole->consumers[pConsole->ConsumerCount];
        consumer.instance = inst;
        consumer.callback = callback;
        OutConsumerID = pConsole->ConsumerCount;
        pConsole->ConsumerCount++;
    }
}

void Console::UpdateConsumer(u8 ConsumerID, void *inst, PFN_ConsoleConsumerWrite callback)
{
    if (pConsole) {
        MASSERT_MSG(ConsumerID < pConsole->ConsumerCount, "Идентификатор потребителя недействителен.");

        auto& consumer = pConsole->consumers[ConsumerID];
        consumer.instance = inst;
        consumer.callback = callback;
    }
}

void Console::WriteLine(Log::Level level, const char *message)
{
    if (pConsole) {
        // Сообщите каждому потребителю, что строка была добавлена.
        for (u8 i = 0; i < pConsole->ConsumerCount; ++i) {
            auto& consumer = pConsole->consumers[i];
            if (consumer.callback) {
                consumer.callback(consumer.instance, level, message);
            }
        }
    }
}

bool Console::RegisterCommand(const char *command, u8 ArgCount, PFN_ConsoleCommand func)
{
    MASSERT_MSG(pConsole && command, "Console::RegisterCommand требует состояние и допустимую команду");

    // Убедитесь, что она еще не существует.
    const u64& CommandCount = pConsole->RegisteredCommands.Length();
    for (u64 i = 0; i < CommandCount; ++i) {
        if (pConsole->RegisteredCommands[i].name.Comparei(command)) {
            MERROR("Команда уже зарегистрирована: %s", command);
            return false;
        }
    }

    Command NewCommand;
    NewCommand.ArgCount = ArgCount;
    NewCommand.func = func;
    NewCommand.name = command;
    pConsole->RegisteredCommands.PushBack(static_cast<Command&&>(NewCommand));

    return true;
}

bool Console::UnregisterCommand(const char *command)
{
    MASSERT_MSG(pConsole && command, "Console::UnregisterCommand требует состояния и допустимой команды");

    // Убедитесь, что его еще нет.
    auto& CommandCount = pConsole->RegisteredCommands.Length();
    for (u32 i = 0; i < CommandCount; ++i) {
        if (pConsole->RegisteredCommands[i].name.Comparei(command)) {
            // Команда найдена, удалите ее.
            Command PoppedCommand;
            pConsole->RegisteredCommands.PopAt(i, &PoppedCommand);
            return true;
        }
    }

    return false;
}

bool Console::ExecuteCommand(const MString &command)
{
    if (!command) {
        return false;
    }
    DArray<MString> parts;
    // ЗАДАЧА: Если строки когда-либо будут использоваться в качестве аргументов, это приведет к неправильному разделению.
    u32 PartCount = command.Split(' ', parts, true, false);
    if (PartCount < 1) {
        return false;
    }

    // Запишите строку обратно в консоль для справки.
    char temp[512] = {};
    MString::Format(temp, "-->%s", command.c_str());
    WriteLine(Log::Level::Info, temp);

    // Да, строки медленные. Но это консоль. Она не должна быть молниеносной...
    bool HasError = false;
    bool CommandFound = false;
    const u64& CommandCount = pConsole->RegisteredCommands.Length();
    // Просмотрите зарегистрированные команды на предмет совпадения.
    for (u32 i = 0; i < CommandCount; ++i) {
        auto& cmd = pConsole->RegisteredCommands[i];
        if (parts[0].Comparei(cmd.name)) {
            CommandFound = true;
            u8 ArgCount = PartCount - 1;
            // Указанное количество аргументов должно соответствовать ожидаемому количеству аргументов для команды.
            if (pConsole->RegisteredCommands[i].ArgCount != ArgCount) {
                MERROR("Консольная команда '%s' требует %u аргументов, но %u были предоставлены.", cmd.name.c_str(), cmd.ArgCount, ArgCount);
                HasError = true;
            } else {
                // Выполните ее, передав аргументы, если необходимо.
                ConsoleCommandContext context = {};
                context.ArgumentCount = cmd.ArgCount;
                if (context.ArgumentCount > 0) {
                    context.arguments = MemorySystem::TAllocate<ConsoleCommandArgument>(Memory::Array, cmd.ArgCount);
                    for (u8 j = 0; j < cmd.ArgCount; ++j) {
                        context.arguments[j].value = parts[j + 1].c_str();
                    }
                }

                cmd.func(context);

                if (context.arguments) {
                    MemorySystem::Free(context.arguments, sizeof(ConsoleCommandArgument) * cmd.ArgCount, Memory::Array);
                }
            }
            break;
        }
    }

    if (!CommandFound) {
        MERROR("Команда '%s' не существует.", parts[0].c_str());
        HasError = true;
    }

    return !HasError;
}
