#include "event.hpp"
#include "memory/linear_allocator.hpp"

Event* Event::event = nullptr;

bool Event::Initialize()
{
    event = new Event();
    if (!event) {
       return false;
    }
    
    return true;
}

void Event::Shutdown()
{
    // Освободите массивы событий. А объекты, на которые указывают, должны уничтожаться самостоятельно.
    /*for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(registered[i].events.Capacity() > 0) {
            registered[i].events.~DArray();
            // state.registered[i].events = 0;
        }
    }*/
    //delete event;
}

bool Event::Register(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!event) {
        return false;
    }

    u64 RegisteredCount = event->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        if(event->registered[code].events[i].listener == listener && event->registered[code].events[i].callback == OnEvent) {
            MWARN("Событие уже зарегистрировано с кодом %hu и обратным вызовом %p", code, OnEvent);
            return false;
        }
    }

    // Если на этом этапе дубликат не найден. Продолжайте регистрацию.
    event->registered[code].events.EmplaceBack(listener, OnEvent);

    return true;
}

bool Event::Unregister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!event) {
        return false;
    }

    // По коду ничего не прописано, загружаемся.
    if(event->registered[code].events.Capacity() == 0) {
        // ЗАДАЧА: warn
        return false;
    }

    u64 RegisteredCount = event->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        const auto& e = event->registered[code].events[i];
        if(e.listener == listener/* && e.callback == OnEvent*/) {
            // Нашёл, удали
            // RegisteredEvent PoppedEvent;
            event->registered[code].events.PopAt(i);
            return true;
        }
    }

    // Не найдено.
    return false;
}

bool Event::Fire(u16 code, void *sender, EventContext context)
{
    if(!event) {
        return false;
    }

    // Если для кода ничего не зарегистрировано, выйдите из системы.
    if(event->registered[code].events.Length() == 0) {
        return false;
    }

    const u64& RegisteredCount = event->registered[code].events.Length();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        const auto& e = event->registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Сообщение обработано, не отправляйте его другим слушателям.
            return true;
        }
    }

    // Не найдено.
    return false;
}
