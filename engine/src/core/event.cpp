#include "event.hpp"

#include "core/application.hpp"

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
    for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(registered[i].events.Capacity() > 0) {
            registered[i].events.~DArray();
            // state.registered[i].events = 0;
        }
    }
    delete event;
    this->~Event();
}

bool Event::Register(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!event) {
        return false;
    }

    DArray<u64> RegisteredCount;
    u64 registered_count = registered[code].events.Lenght();
    for(u64 i = 0; i < registered_count; ++i) {
        if(registered[code].events[i].listener == listener) {
            // TODO: warn
            return false;
        }
    }

    // Если на этом этапе дубликат не найден. Продолжайте регистрацию.
    RegisteredEvent event;
    event.listener = listener;
    event.callback = OnEvent;
    registered[code].events.PushBack(event);

    return true;
}

bool Event::Unregister(u16 code, void *listener, PFN_OnEvent OnEvent)
{
    if(!event) {
        return false;
    }

    // По коду ничего не прописано, загружаемся.
    if(registered[code].events.Capacity() == 0) {
        // TODO: warn
        return false;
    }

    u64 RegisteredCount = registered[code].events.Lenght();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        RegisteredEvent e = registered[code].events[i];
        if(e.listener == listener/* && e.callback == OnEvent*/) {
            // Нашёл, удали
            // RegisteredEvent PoppedEvent;
            registered[code].events.PopAt(i);
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
    if(registered[code].events.Capacity() == 0) {
        return false;
    }

    u64 RegisteredCount = registered[code].events.Lenght();
    for(u64 i = 0; i < RegisteredCount; ++i) {
        RegisteredEvent e = registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Сообщение обработано, не отправляйте его другим слушателям.
            return true;
        }
    }

    // Not found.
    return false;
}

Event *Event::GetInstance()
{
    return event;
}

void *Event::operator new(u64 size)
{
    return Application::AllocMemory(size);
}
