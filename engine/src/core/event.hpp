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

/// @brief Реализация класса инициатора для синхронных действий
class Executor
{
public:
    void CallbackHandler(int eventID);
    void operator() (int eventID);
};

//using ptrCallback = bool(*) (u16, void*, void*, EventContext);
using PFN_OnEventStatic = bool(*) (u16, void*, void*, EventContext);
using PFN_OnEventMethod = bool(Executor::*)(u16, void*, void*, EventContext, Executor*);

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

class MAPI Event
{
private:
    // Таблица поиска кодов событий.
    EventCodeEntry registered[MAX_MESSAGE_CODES];

    static Event* event;
    Event() : registered() {};
public:
    ~Event() = default;

    Event(const Event&) = delete;
    Event& operator= (const Event&) = delete;

    static bool Initialize();
    static void Shutdown();

    /// @brief Зарегистрируйте, следить за отправкой событий с предоставленным кодом. 
    /// События с повторяющимися комбинациями прослушивателя и обратного вызова не будут регистрироваться снова 
    /// и приведут к возврату false.
    /// @param code Код события, который необходимо прослушать.
    /// @param listener Указатель на экземпляр прослушивателя. Может быть 0/NULL.
    /// @param OnEvent Указатель функции обратного вызова, который будет вызываться при запуске кода события.
    /// @return true, если событие успешно зарегистрировано; в противном случае false.
    static bool Register(u16 code, void* listener, PFN_OnEvent OnEvent);

    /// @brief Отмените регистрацию от прослушивания событий, отправляемых с помощью предоставленного кода.
    /// Если соответствующая регистрация не найдена, эта функция возвращает false.
    /// @param code Код события, для которого необходимо прекратить прослушивание.
    /// @param listener Указатель на экземпляр прослушивателя. Может быть 0/NULL.
    /// @param OnEvent Указатель функции обратного вызова, регистрацию которого необходимо отменить.
    /// @return true, если событие успешно отменено; в противном случае false.
    static bool Unregister(u16 code, void* listener, PFN_OnEvent OnEvent);

    /// @brief Вызывает событие для слушателей данного кода. Если обработчик событий возвращает true, 
    /// событие считается обработанным и больше не передается прослушивателям.
    /// @param code Код события, которое необходимо активировать.
    /// @param sender Указатель на отправителя. Может быть 0/NULL.
    /// @param data Данные о событии.
    /// @return true, если обработано, иначе false.
    static bool Fire(u16 code, void* sender, EventContext context);

    static MINLINE Event* GetInstance() {
        /*if (!event) {
            MERROR("Функция Event::GetInstance вызывается перед инициализацией!");
        }*/
        
        return event;
    }

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
     * i8 z_delta = data.data.i8[0]; */
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // Изменен размер/разрешение из ОС.
    /* Использование контекста:
     * u16 width = data.data.u16[0];
     * u16 height = data.data.u16[1];   */
    EVENT_CODE_RESIZED = 0x08,

    // Измените режим рендеринга в целях отладки.
    /* Использование контекста:
     * i32 mode = context.data.i32[0];  */
    EVENT_CODE_SET_RENDER_MODE = 0x0A,

    EVENT_CODE_DEBUG0 = 0x10,   // Специальное событие отладки. Контекст будет меняться со временем.
    EVENT_CODE_DEBUG1 = 0x11,   // Специальное событие отладки. Контекст будет меняться со временем.
    EVENT_CODE_DEBUG2 = 0x12,   // Специальное событие отладки. Контекст будет меняться со временем.
    EVENT_CODE_DEBUG3 = 0x13,   // Специальное событие отладки. Контекст будет меняться со временем.
    EVENT_CODE_DEBUG4 = 0x14,   // Специальное событие отладки. Контекст будет меняться со временем.

    /// @brief Идентификатор наведенного объекта, если он есть.
    /// Использование контекста:
    /// i32 id = context.data.u32[0]; - будет НЕДЕЙСТВИТЕЛЬНЫМ ИДЕНТИФИКАТОРОМ, если ни на что не наведен курсор.
    EVENT_CODE_OBJECT_HOVER_ID_CHANGED = 0x15,
 
    /// @brief Событие, запускаемое бэкэндом рендеринга, чтобы указать, когда необходимо обновить 
    /// любые цели рендеринга, связанные с ресурсами окна по умолчанию (т.е. изменить размер окна).
    EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED = 0x16,

    MAX_EVENT_CODE = 0xFF       // Максимальный код события, который можно использовать внутри.
};

/*
void Run(ptrCallback ptrCallback, void* contextData = nullptr)  // (1)
{
    int eventID = 0;
    ptrCallback(eventID, contextData);
}
void Run(ptrCallbackStatic ptrCallback, Executor* contextData = nullptr)  // (2)
{
    int eventID = 0;
    ptrCallback(eventID, contextData);
}
void Run(Executor* ptrClientCallbackClass, ptrCallbackMethod ptrClientCallbackMethod)  // (3)
{
    int eventID = 0;
    (ptrClientCallbackClass->*ptrClientCallbackMethod)(eventID);
}
void Run(Executor callbackHandler)  // (4)
{
    int eventID = 0;
    callbackHandler(eventID);
}

/// @brief 
/// @tparam CallbackArgument 
/// @param callbackHandler 
template <typename CallbackArgument>
void run(CallbackArgument callbackHandler)
{
    int eventID = 0;
    //Some actions
    callbackHandler(eventID);
}

template<typename Function, typename Context>                                           // (1)
class CallbackConverter                                                                 // (2)
{
public:
    CallbackConverter (Function argFunction = nullptr, Context argContext = nullptr)    // (3)
    {
        ptrFunction = argFunction; context = argContext;
    }
    void operator() (int eventID)                                                       // (4)
    {
        ptrFunction(eventID, context);                                                  // (5)
    }
private:
    Function ptrFunction;                                                               // (6)
    Context context;                                                                    // (7)
};

template<typename ClassName>  // (1)
class CallbackConverter <void(ClassName::*)(), ClassName>   //
{
public:
    using ClassMethod = void(ClassName::*)();               //

    CallbackConverter(ClassMethod MethodPointer = nullptr, ClassName* ClassPointer = nullptr) { //
        ptrClass = ClassPointer; ptrMethod = MetodPointer;
    } // 

    void operator()(int eventID) {
        ptrClass->*ptrMethod)
    }
}
*/