#include "event.h"
#include "containers/darray.h"
#include "engine.h"
#include <new>

EventSystem* EventSystem::Initialize(u64& MemoryRequirement, void* memory)
{
    MemoryRequirement = sizeof(EventSystem);
    if (!memory) {
        return nullptr;
    }
    
    state = new(memory) EventSystem();
    if (!state) {
       return nullptr;
    }

    Engine::OnEventSystemInitialized();
    
    return state;
}

bool EventSystem::Register(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!state) {
        return false;
    }

    u32 RegisteredCount = state->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        if(state->registered[code].events[i].listener == listener && state->registered[code].events[i].callback == OnEvent) {
            MWARN("Событие уже зарегистрировано с кодом %hu и обратным вызовом %p", code, OnEvent);
            return false;
        }
    }

    // Если на этом этапе дубликат не найден. Продолжайте регистрацию.
    state->registered[code].events.EmplaceBack(listener, OnEvent);

    return true;
}

bool EventSystem::Unregister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!state) {
        return false;
    }

    // По коду ничего не прописано, загружаемся.
    if(state->registered[code].events.Capacity() == 0) {
        // ЗАДАЧА: warn
        return false;
    }

    u32 RegisteredCount = state->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        const auto& e = state->registered[code].events[i];
        if(e.listener == listener/* && e.callback == OnEvent*/) {
            // Нашёл, удали
            // RegisteredEvent PoppedEvent;
            state->registered[code].events.PopAt(i);
            return true;
        }
    }

    // Не найдено.
    return false;
}

bool EventSystem::Fire(u16 code, void *sender, const EventContext& context)
{
    if(!state) {
        return false;
    }

    // Если для кода ничего не зарегистрировано, выйдите из системы.
    if(state->registered[code].events.Length() == 0) {
        return false;
    }

    const u32 RegisteredCount = state->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        const auto& e = state->registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Сообщение обработано, не отправляйте его другим слушателям.
            return true;
        }
    }

    // Не найдено.
    return false;
}
