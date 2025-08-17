#include "event.hpp"
#include "containers/darray.hpp"
#include "engine.h"
#include <new>

struct RegisteredEvent {
    void* listener;
    PFN_OnEvent callback;
    constexpr RegisteredEvent(void* listener, PFN_OnEvent callback) : listener(listener), callback(callback) {}
};

struct EventCodeEntry {
    DArray<RegisteredEvent> events;
};

// Кодов должно быть более чем достаточно...
constexpr u16 MAX_MESSAGE_CODES = 16384;

struct Events
{
    // Таблица поиска кодов событий.
    EventCodeEntry registered[MAX_MESSAGE_CODES];

    Events() : registered() {}
};


static Events* pEvents = nullptr;

bool EventSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    MemoryRequirement = sizeof(Events);
    if (!memory) {
        return true;
    }
    
    pEvents = new(memory) Events();
    if (!pEvents) {
       return false;
    }

    Engine::OnEventSystemInitialized();
    
    return true;
}

void EventSystem::Shutdown()
{
    // Освободите массивы событий. А объекты, на которые указывают, должны уничтожаться самостоятельно.
    /*for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(registered[i].events.Capacity() > 0) {
            registered[i].events.~DArray();
            // state.registered[i].events = 0;
        }
    }*/
    pEvents = nullptr;
}

bool EventSystem::Register(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!pEvents) {
        return false;
    }

    u64 RegisteredCount = pEvents->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        if(pEvents->registered[code].events[i].listener == listener && pEvents->registered[code].events[i].callback == OnEvent) {
            MWARN("Событие уже зарегистрировано с кодом %hu и обратным вызовом %p", code, OnEvent);
            return false;
        }
    }

    // Если на этом этапе дубликат не найден. Продолжайте регистрацию.
    pEvents->registered[code].events.EmplaceBack(listener, OnEvent);

    return true;
}

bool EventSystem::Unregister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!pEvents) {
        return false;
    }

    // По коду ничего не прописано, загружаемся.
    if(pEvents->registered[code].events.Capacity() == 0) {
        // ЗАДАЧА: warn
        return false;
    }

    u64 RegisteredCount = pEvents->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        const auto& e = pEvents->registered[code].events[i];
        if(e.listener == listener/* && e.callback == OnEvent*/) {
            // Нашёл, удали
            // RegisteredEvent PoppedEvent;
            pEvents->registered[code].events.PopAt(i);
            return true;
        }
    }

    // Не найдено.
    return false;
}

bool EventSystem::Fire(u16 code, void *sender, EventContext context)
{
    if(!pEvents) {
        return false;
    }

    // Если для кода ничего не зарегистрировано, выйдите из системы.
    if(pEvents->registered[code].events.Length() == 0) {
        return false;
    }

    const u64& RegisteredCount = pEvents->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        const auto& e = pEvents->registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Сообщение обработано, не отправляйте его другим слушателям.
            return true;
        }
    }

    // Не найдено.
    return false;
}
