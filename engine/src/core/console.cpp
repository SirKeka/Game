#include "console.hpp"
#include "memory/linear_allocator.hpp"
#include "asserts.hpp"
#include <new>

constexpr u32 MAX_CONSUMER_COUNT = 10;

Console *Console::pConsole = nullptr;

constexpr Console::Console(Consumer* consumers) : ConsumerCount(), consumers(consumers), RegisteredCommands() {}

Console::~Console()
{
}

void Console::Initialize(LinearAllocator &SystemAllocator)
{
    u64 MemoryRequirement = sizeof(Console) + sizeof(Consumer) * MAX_CONSUMER_COUNT;
    void* pRawMem = SystemAllocator.Allocate(MemoryRequirement);
    auto ConsumerMem = reinterpret_cast<Consumer*>(reinterpret_cast<u8*>(pRawMem) + sizeof(Console));

    pConsole = new(pRawMem) Console(ConsumerMem);
}

void Console::Shutdown()
{
    if (pConsole) {
        pConsole->RegisteredCommands.~DArray();

        MMemory::ZeroMem(pConsole, sizeof(Console) + (sizeof(Consumer) * MAX_CONSUMER_COUNT));
    }

    pConsole = nullptr;
}

void Console::RegisterConsumer(void *inst, PFN_ConsoleConsumerWrite callback)
{
    if (pConsole) {
        MASSERT_MSG(pConsole->ConsumerCount + 1 < MAX_CONSUMER_COUNT, "Достигнуто максимальное количество потребителей консолей.");

        auto& consumer = pConsole->consumers[pConsole->ConsumerCount];
        consumer.instance = inst;
        consumer.callback = callback;
        pConsole->ConsumerCount++;
    }
}

void Console::WriteLine(LogLevel level, const char *message)
{
    if (pConsole) {
        // Сообщите каждому потребителю, что строка была добавлена.
        for (u8 i = 0; i < pConsole->ConsumerCount; ++i) {
            auto& consumer = pConsole->consumers[i];
            consumer.callback(consumer.instance, level, message);
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
    WriteLine(LogLevel::Info, temp);

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
                    context.arguments = MMemory::TAllocate<ConsoleCommandArgument>(Memory::Array, cmd.ArgCount);
                    for (u8 j = 0; j < cmd.ArgCount; ++j) {
                        context.arguments[j].value = parts[j + 1].c_str();
                    }
                }

                cmd.func(context);

                if (context.arguments) {
                    MMemory::Free(context.arguments, sizeof(ConsoleCommandArgument) * cmd.ArgCount, Memory::Array);
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
