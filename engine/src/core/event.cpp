#include "event.hpp"

// #include "mmemory.hpp"
#include "containers/darray.hpp"

struct RegisteredEvent {
    void* listener;
    PFN_OnEvent callback;
};

struct EventCodeEntry {
    DArray<RegisteredEvent> events;
    // RegisteredEvent* events;
};

// Кодов должно быть более чем достаточно...
#define MAX_MESSAGE_CODES 16384

// Структура состояния.
struct EventSystemState {
    // Таблица поиска кодов событий.
    EventCodeEntry registered[MAX_MESSAGE_CODES];
};

// Внутреннее состояние системы событий.
bool Event::IsInitialized = false;
static EventSystemState state;

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
    u64 registered_count = state.registered[code].events.lenght;
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
    MArrayPush(state.registered[code].events, event);

    return true;
}

bool Event::Unregister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(IsInitialized == false) {
        return false;
    }

    // По коду ничего не прописано, загружаемся.
    if(state.registered[code].events == 0) {
        // TODO: warn
        return false;
    }

    u64 RegisteredCount = darray_length(state.registered[code].events);
    for(u64 i = 0; i < RegisteredCount; ++i) {
        RegisteredEvent e = state.registered[code].events[i];
        if(e.listener == listener && e.callback == OnEvent) {
            // Нашёл, удали
            RegisteredEvent popped_event;
            darray_pop_at(state.registered[code].events, i, &popped_event);
            return true;
        }
    }

    // Not found.
    return false;
}

bool Event::Fire(u16 code, void *sender, EventContext context)
{
    return MAPI bool();
}
