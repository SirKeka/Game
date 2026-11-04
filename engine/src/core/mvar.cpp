#include "mvar.h"

#include "console.h"
#include "event.h"

MVar *MVar::Initialize(u64 &MemoryRequirement, void *memory)
{
    MemoryRequirement = sizeof(MVar);

    if (!memory) {
        return nullptr;
    }

    state = new(memory) MVar();

    if (!state) {
        return nullptr;
    }

    Console::RegisterCommand("mvar_create_int", 2, MVar::ConsoleCommandCreateInt);
    Console::RegisterCommand("mvar_print_int", 1, MVar::ConsoleCommandPrintInt);
    Console::RegisterCommand("mvar_set_int", 2, MVar::ConsoleCommandSetInt);
    Console::RegisterCommand("mvar_print_all", 0, MVar::ConsoleCommandPrintAll);

    return state;
}

void MVar::Shutdown()
{
    if (state) {
        MemorySystem::ZeroMem(state, sizeof(MVar));
    }
}

MAPI bool MVar::CreateInt(const char *name, i32 value)
{
    if (!state || !name) {
        return false;
    }

    for (u32 i = 0; i < MVAR_INT_MAX_COUNT; ++i) {
        auto& entry = state->ints[i];
        if (!entry.name && entry.name.Comparei(name)) {
            MERROR("Целая переменная с именем '%s' уже существует.", name);
            return false;
        }
    }

    for (u32 i = 0; i < MVAR_INT_MAX_COUNT; ++i) {
        auto& entry = state->ints[i];
        if (!entry.name) {
            entry.name = name;
            entry.value = value;
            return true;
        }
    }

    MERROR("MVar::CreateInt не удалось найти свободный слот для сохранения записи.");
    return false;
}

MAPI bool MVar::GetInt(const char *name, i32 &OutValue)
{
    if (!state || !name) {
        return false;
    }

    for (u32 i = 0; i < MVAR_INT_MAX_COUNT; ++i) {
        auto& entry = state->ints[i];
        if (entry.name && entry.name.Comparei(name)) {
            OutValue = entry.value;
            return true;
        }
    }

    MERROR("MVar::GetInt не удалось найти MVar с именем '%s'.", name);
    return false;
}

bool MVar::SetInt(const char *name, i32 value)
{
    if (!state || !name) {
        return false;
    }

    for (u32 i = 0; i < MVAR_INT_MAX_COUNT; ++i) {
        auto& entry = state->ints[i];
        if (entry.name && entry.name.Comparei(name)) {
            entry.value = value;
            // ЗАДАЧА: also pass type?
            EventContext context = {0};
            MString::Copy(context.data.c, name, 16);
            EventSystem::Fire(EventSystem::MVarChanged, nullptr, context);
            return true;
        }
    }

    MERROR("MVar::SetInt не удалось найти MVar с именем '%s'.", name);
    return false;
}

void MVar::ConsoleCommandCreateInt(ConsoleCommandContext &context) {
    if (context.ArgumentCount != 2) {
        MERROR("Для MVar::ConsoleCommandCreateInt требуется количество аргументов контекста, равное 2.");
        return;
    }

    const char* name = context.arguments[0].value;
    const char* ValString = context.arguments[1].value;
    i64 value = 0;
    if (!MString::ToInt(ValString, value, 0)) {
        MERROR("Не удалось преобразовать аргумент 1 в i32: '%s'.", ValString);
        return;
    }

    if (!MVar::CreateInt(name, value)) {
        MERROR("Не удалось создать int MVar.");
    }
}

void MVar::ConsoleCommandPrintInt(ConsoleCommandContext &context) {
    if (context.ArgumentCount != 1) {
        MERROR("MVar::ConsoleCommandPrintInt требует количество аргументов контекста 1.");
        return;
    }

    const char* name = context.arguments[0].value;
    i32 value = 0;
    if (!MVar::GetInt(name, value)) {
        MERROR("Не удалось найти MVar с именем '%s'.", name);
        return;
    }

    char ValString[50] = {0};
    MString::Format(ValString, "%i", value);

    Console::WriteLine(Log::Level::Info, ValString);
}

void MVar::ConsoleCommandSetInt(ConsoleCommandContext &context) {
    if (context.ArgumentCount != 2) {
        MERROR("MVar::ConsoleCommandSetInt требует количество аргументов контекста 2.");
        return;
    }

    const char* name = context.arguments[0].value;
    const char* ValString = context.arguments[1].value;
    i64 value = 0;
    if (!MString::ToInt(ValString, value)) {
        MERROR("Не удалось преобразовать аргумент 1 в i32: '%s'.", ValString);
        return;
    }

    if (!MVar::SetInt(name, value)) {
        MERROR("Не удалось задать int MVar с именем '%s', так как он не существует.", name);
    }

    char out_str[500] = {0};
    MString::Format(out_str, "%s = %i", name, value);
    Console::WriteLine(Log::Level::Info, ValString);
}

void MVar::ConsoleCommandPrintAll(ConsoleCommandContext &context) {
    // Int mvars
    for (u32 i = 0; i < MVAR_INT_MAX_COUNT; ++i) {
        auto& entry = state->ints[i];
        if (entry.name) {
            char ValString[500] = {0};
            MString::Format(ValString, "%s = %i", entry.name.c_str(), entry.value);
            Console::WriteLine(Log::Level::Info, ValString);
        }
    }

    // ЗАДАЧА: Другие типы переменных.
}
