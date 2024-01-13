#include "event.hpp"

// #include "mmemory.hpp"

// Внутреннее состояние системы событий.
bool Event::IsInitialized = false;
EventSystemState Event::state;

bool Event::Initialize()
{
    if (IsInitialized == true) {
        return false;
    }
    IsInitialized = false;
    // MZeroMemory(&state, sizeof(state));

    IsInitialized = true;

    return true;
}

void Event::Shutdown()
{
    // Освободите массивы событий. А объекты, на которые указывают, должны уничтожаться самостоятельно.
    for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(state.registered[i].events.capacity > 0) {
            state.registered[i].events.~DArray();
            // state.registered[i].events = 0;
        }
    }
}

bool Event::Register(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(IsInitialized == false) {
        return false;
    }

    /*if(state.registered[code].events.capacity == 0) {
        state.registered[code].events = DArray<RegisteredEvent>();
    }*/

    DArray<u64> RegisteredCount;
    u64 registered_count = state.registered[code].events.size;
    for(u64 i = 0; i < registered_count; ++i) {
        if(state.registered[code].events[i].listener == listener) {
            // TODO: warn
            return false;
        }
    }

    // Если на этом этапе дубликат не найден. Продолжайте регистрацию.
    RegisteredEvent event;
    event.listener = listener;
    event.callback = OnEvent;
    state.registered[code].events.PushBack(event);

    return true;
}

bool Event::Unregister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(IsInitialized == false) {
        return false;
    }

    // По коду ничего не прописано, загружаемся.
    if(state.registered[code].events.capacity == 0) {
        // TODO: warn
        return false;
    }

    u64 RegisteredCount = state.registered[code].events.size;
    for(u64 i = 0; i < RegisteredCount; ++i) {
        RegisteredEvent e = state.registered[code].events[i];
        if(e.listener == listener && e.callback == OnEvent) {
            // Нашёл, удали
            // RegisteredEvent PoppedEvent;
            state.registered[code].events.PopAt(i);
            return true;
        }
    }

    // Не найдено.
    return false;
}

bool Event::Fire(u16 code, void *sender, EventContext context)
{
    if(IsInitialized == false) {
        return false;
    }

    // Если для кода ничего не зарегистрировано, выйдите из системы.
    if(state.registered[code].events.capacity == 0) {
        return false;
    }

    u64 RegisteredCount = state.registered[code].events.size;
    for(u64 i = 0; i < RegisteredCount; ++i) {
        RegisteredEvent e = state.registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Сообщение обработано, не отправляйте его другим слушателям.
            return true;
        }
    }

    // Not found.
    return false;
}
