#pragma once

#include "defines.hpp"
#include <containers/darray.hpp>
#include <core/event.hpp>
#include <resources/ui_text.hpp>

// struct ComandHistoryEntry
// {
//     MString command;
// };

using CommandHistoryEntry = MString;

class DebugConsole 
{
    u8 ConsoleConsumerID;
    u32 LineDisplayCount;   // Количество строк, отображаемых одновременно.
    u32 LineOffset;         // Количество строк, смещенных от низа списка.
    DArray<MString> lines;

    DArray<CommandHistoryEntry> history;
    u32 HistoryOffset;

    bool dirty;
    bool visible;

    Text TextControl;
    Text EntryControl;
public:
    DebugConsole();
    ~DebugConsole() = default;

    bool Load();
    void Update();

    void OnLibLoad(bool UpdateConsumer);
    void OnLibUnload();

    Text& GetText();
    Text& GetEntryText();

    bool Visible();
    void SetVisible(bool visible);

    void MoveUp();
    void MoveDown();
    void MoveToTop();
    void MoveToBottom();

    void HistoryBack();
    void HistoryForward();

    void* operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Game);
    }
    void operator delete(void* ptr, u64 size){
        MemorySystem::Free(ptr, size, Memory::Game);
    }
private:
    static bool DebugConsoleConsumerWrite(void* inst, Log::Level level, const char* message);
    static bool OnKey(u16 code, void* sender, void* ListenerInst, EventContext context);
};


