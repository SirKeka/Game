#pragma once

#include "containers/darray.h"
#include "elements/label.h"
#include "elements/panel.h"
#include "elements/textbox.h"

using CommandHistoryEntry = MString;

// struct ComandHistoryEntry
// {
//     MString command;
// };

class MAPI DebugConsole 
{
public:
    DebugConsole();
    ~DebugConsole() = default;

    bool Load();
    void Update();

    void OnLibLoad(bool UpdateConsumer);
    void OnLibUnload();

    Label& GetText() { return TextControl; }
    Textbox& GetEntryText() { return EntryTextbox; }

    bool Visible() { return visible; }
    void SetVisible(bool visible);

    void MoveUp();
    void MoveDown();
    void MoveToTop();
    void MoveToBottom();

    DArray<MString>& History() { return history; }
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
    static bool OnResize(u16 code, void* sender, void* ListenerInst, const EventContext& context);

    Panel panel;
    Label TextControl;
    Textbox EntryTextbox;
    
    DArray<MString> lines;
    DArray<CommandHistoryEntry> history;
    
    u32 LineDisplayCount;   // Количество строк, отображаемых одновременно.
    u32 LineOffset;         // Количество строк, смещенных от низа списка.
    u32 HistoryOffset;
    u8 ConsoleConsumerID;
    
    bool loaded;
    bool dirty;
    bool visible;
};
