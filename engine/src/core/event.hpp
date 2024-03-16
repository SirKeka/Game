#pragma once

#include "defines.hpp"
#include "containers/darray.hpp"

struct EventContext {
    // 128 байт
    union Data {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];
    } data;
};

// Должен возвращать true, если обработано.
typedef bool (*PFN_OnEvent)(u16 code, void* sender, void* ListenerInst, EventContext data);

struct RegisteredEvent {
    void* listener;
    PFN_OnEvent callback;
};

struct EventCodeEntry {
    DArray<RegisteredEvent> events;
};

// Кодов должно быть более чем достаточно...
#define MAX_MESSAGE_CODES 16384

// Структура состояния.
struct EventSystemState {
    // Таблица поиска кодов событий.
    EventCodeEntry registered[MAX_MESSAGE_CODES];
};

class Event
{
using LinearAllocator = WrapLinearAllocator<Event>;

private:
    static bool IsInitialized;
    static EventSystemState state;
    
public:
    Event() = default;
    ~Event();

    bool Initialize();
    void Shutdown();

    /// @brief Зарегистрируйте, следить за отправкой событий с предоставленным кодом. 
    /// События с повторяющимися комбинациями прослушивателя и обратного вызова не будут регистрироваться снова 
    /// и приведут к возврату FALSE.
    /// @param code Код события, который необходимо прослушать.
    /// @param listener Указатель на экземпляр прослушивателя. Может быть 0/NULL.
    /// @param OnEvent Указатель функции обратного вызова, который будет вызываться при запуске кода события.
    /// @return TRUE, если событие успешно зарегистрировано; в противном случае FALSE.
    MAPI static bool Register(u16 code, void* listener, PFN_OnEvent OnEvent);

    /// @brief Отмените регистрацию от прослушивания событий, отправляемых с помощью предоставленного кода.
    /// Если соответствующая регистрация не найдена, эта функция возвращает FALSE.
    /// @param code Код события, для которого необходимо прекратить прослушивание.
    /// @param listener Указатель на экземпляр прослушивателя. Может быть 0/NULL.
    /// @param OnEvent Указатель функции обратного вызова, регистрацию которого необходимо отменить.
    /// @return TRUE, если событие успешно отменено; в противном случае FALSE.
    MAPI static bool Unregister(u16 code, void* listener, PFN_OnEvent OnEvent);

    /// @brief Вызывает событие для слушателей данного кода. Если обработчик событий возвращает TRUE, 
    /// событие считается обработанным и больше не передается прослушивателям.
    /// @param code Код события, которое необходимо активировать.
    /// @param sender Указатель на отправителя. Может быть 0/NULL.
    /// @param data Данные о событии.
    /// @return TRUE, если обработано, иначе FALSE.
    MAPI static bool Fire(u16 code, void* sender, EventContext context);

    void* operator new(u64 size);
};



// Коды внутренних событий системы. Приложение должно использовать коды, превышающие 255.
enum SystemEventCode {
    // Закрывает приложение на следующем кадре.
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Нажата клавиша клавиатуры.
    /* Использование контекста:
     * u16 key_code = data.data.u16[0]; */
    EVENT_CODE_KEY_PRESSED = 0x02,

    // Клавиша клавиатуры отпущена.
    /* Использование контекста:
     * u16 key_code = data.data.u16[0]; */
    EVENT_CODE_KEY_RELEASED = 0x03,

    // Кнопка мыши нажата.
    /* Использование контекста:
     * u16 button = data.data.u16[0]; */
    EVENT_CODE_BUTTON_PRESSED = 0x04,

    // Кнопка мыши отпущена.
    /* Использование контекста:
     * u16 button = data.data.u16[0]; */
    EVENT_CODE_BUTTON_RELEASED = 0x05,

    // Мышь перемещена.
    /* Использование контекста:
     * u16 x = data.data.u16[0];
     * u16 y = data.data.u16[1]; */
    EVENT_CODE_MOUSE_MOVED = 0x06,

    //Мышь перемещена.
    /* Использование контекста:
     * u8 z_delta = data.data.u8[0]; */
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // Изменен размер/разрешение из ОС.
    /* Использование контекста:
     * u16 width = data.data.u16[0];
     * u16 height = data.data.u16[1];   */
    EVENT_CODE_RESIZED = 0x08,

    EVENT_CODE_DEBUG0 = 0x10,
    EVENT_CODE_DEBUG1 = 0x11,
    EVENT_CODE_DEBUG2 = 0x12,
    EVENT_CODE_DEBUG3 = 0x13,
    EVENT_CODE_DEBUG4 = 0x14,

    MAX_EVENT_CODE = 0xFF
};