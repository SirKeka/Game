#include "event.hpp"

// #include "mmemory.hpp"
#include "containers/darray.hpp"

struct RegisteredEvent {
    void* listener;
    PFN_OnEvent callback;
};

struct EventCodeEntry {
    RegisteredEvent* events;
};

// Кодов должно быть более чем достаточно...
#define MAX_MESSAGE_CODES 16384

// Структура состояния.
struct EventSystemState {
    // Таблица поиска кодов событий.
    EventCodeEntry registered[MAX_MESSAGE_CODES];
};

// Внутреннее состояние системы событий.
static bool IsInitialized = false;
static EventSystemState state;

bool EventRegister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(IsInitialized == false) {
        return false;
    }

    if(state.registered[code].events == 0) {
        state.registered[code].events = MArrayCreate(RegisteredEvent);
    }

    DArray<u64> RegisteredCount;
    u64 registered_count = MArrayLength(state.registered[code].events);
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

bool EventUnregister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    return MAPI b8();
}

bool EventFire(u16 code, void *sender, EventContext context)
{
    return MAPI b8();
}

bool Event::Initialize()
{
    if (IsInitialized == true) {
        return false;
    }
    IsInitialized = false;
    MZeroMemory(&state, sizeof(state));

    IsInitialized = true;

    return true;
}

void Event::Shutdown()
{
    // Освободите массивы событий. А объекты, на которые указывают, должны уничтожаться самостоятельно.
    for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(state.registered[i].events != 0) {
            MArrayDestroy(state.registered[i].events);
            state.registered[i].events = 0;
        }
    }
}
