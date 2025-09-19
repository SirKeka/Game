#pragma once

#include "defines.h"

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
using PFN_OnEvent = bool(*)(u16, void*, void*, EventContext);

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

namespace EventSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();

    /// @brief Зарегистрируйте, следить за отправкой событий с предоставленным кодом. 
    /// События с повторяющимися комбинациями прослушивателя и обратного вызова не будут регистрироваться снова 
    /// и приведут к возврату false.
    /// @param code Код события, который необходимо прослушать.
    /// @param listener Указатель на экземпляр прослушивателя. Может быть 0/NULL.
    /// @param OnEvent Указатель функции обратного вызова, который будет вызываться при запуске кода события.
    /// @return true, если событие успешно зарегистрировано; в противном случае false.
    MAPI bool Register(u16 code, void* listener, PFN_OnEvent OnEvent);

    /// @brief Отмените регистрацию от прослушивания событий, отправляемых с помощью предоставленного кода.
    /// Если соответствующая регистрация не найдена, эта функция возвращает false.
    /// @param code Код события, для которого необходимо прекратить прослушивание.
    /// @param listener Указатель на экземпляр прослушивателя. Может быть 0/NULL.
    /// @param OnEvent Указатель функции обратного вызова, регистрацию которого необходимо отменить.
    /// @return true, если событие успешно отменено; в противном случае false.
    MAPI bool Unregister(u16 code, void* listener, PFN_OnEvent OnEvent);

    /// @brief Вызывает событие для слушателей данного кода. Если обработчик событий возвращает true, 
    /// событие считается обработанным и больше не передается прослушивателям.
    /// @param code Код события, которое необходимо активировать.
    /// @param sender Указатель на отправителя. Может быть 0/NULL.
    /// @param data Данные о событии.
    /// @return true, если обработано, иначе false.
    MAPI bool Fire(u16 code, void* sender, EventContext context);

    // Коды внутренних событий системы. Приложение должно использовать коды, превышающие 255.
    enum Code {
        // Закрывает приложение на следующем кадре.
        ApplicationQuit = 0x01,

        // Нажата клавиша клавиатуры.
        /* Использование контекста:
         * u16 key_code = data.data.u16[0]; */
        KeyPressed = 0x02,

        // Клавиша клавиатуры отпущена.
        /* Использование контекста:
         * u16 key_code = data.data.u16[0]; */
        KeyReleased = 0x03,

        // Кнопка мыши нажата.
        /* Использование контекста:
         * u16 button = data.data.u16[0]; */
        ButtonPressed = 0x04,

        // Кнопка мыши отпущена.
        /* Использование контекста:
         * u16 button = data.data.u16[0]; */
        ButtonReleased = 0x05,

        // Мышь перемещена.
        /* Использование контекста:
         * u16 x = data.data.u16[0];
         * u16 y = data.data.u16[1]; */
        MouseMoved = 0x06,

        //Мышь перемещена.
        /* Использование контекста:
         * i8 z_delta = data.data.i8[0]; */
        MouseWheel = 0x07,

        // Изменен размер/разрешение из ОС.
        /* Использование контекста:
         * u16 width = data.data.u16[0];
         * u16 height = data.data.u16[1];   */
        Resized = 0x08,

        // Измените режим рендеринга в целях отладки.
        /* Использование контекста:
         * i32 mode = context.data.i32[0];  */
        SetRenderMode = 0x0A,

        DEBUG0 = 0x10,   // Специальное событие отладки. Контекст будет меняться со временем.
        DEBUG1 = 0x11,   // Специальное событие отладки. Контекст будет меняться со временем.
        DEBUG2 = 0x12,   // Специальное событие отладки. Контекст будет меняться со временем.
        DEBUG3 = 0x13,   // Специальное событие отладки. Контекст будет меняться со временем.
        DEBUG4 = 0x14,   // Специальное событие отладки. Контекст будет меняться со временем.

        /// @brief Идентификатор наведенного объекта, если он есть.
        /// Использование контекста:
        /// i32 id = context.data.u32[0]; - будет НЕДЕЙСТВИТЕЛЬНЫМ ИДЕНТИФИКАТОРОМ, если ни на что не наведен курсор.
        OojectHoverIdChanged = 0x15,
    
        /// @brief Событие, запускаемое бэкэндом рендеринга, чтобы указать, когда необходимо обновить 
        /// любые цели рендеринга, связанные с ресурсами окна по умолчанию (т.е. изменить размер окна).
        DefaultRendertargetRefreshRequired = 0x16,

        /// @brief Событие, запускаемое системой MVar при обновлении MVar.
        MVarChanged = 0x17,

        /// @brief Событие, срабатывающее при записи в отслеживаемый файл.
        /// Контекст использования:
        /// u32 WatchID = context.data.u32[0];
        WatchedFileWritten = 0x18,

        /// @brief Событие, срабатывающее при удалении отслеживаемого файла.
        /// Контекст использования:
        /// u32 WatchID = context.data.u32[0];
        WatchedFileDeleted = 0x19,

        /// @brief Событие, возникающее при удерживании кнопки и перемещении мыши.
        // Контекст использования:
        // i16 x = context.data.i16[0]
        // i16 y = context.data.i16[1]
        // u16 button = context.data.u16[2]
       MouseDragged = 0x20,

        /// @brief Событие, возникающее при нажатии кнопки и перемещении мыши во время нажатия.
        // Контекст использования:
        // i16 x = context.data.i16[0]
        // i16 y = context.data.i16[1]
        // u16 button = context.data.u16[2]
        MouseDragBegin = 0x21,

        /// @brief Событие, возникающее при отпускании кнопки, ранее называлось перетаскиванием.
        // Контекст использования:
        // i16 x = context.data.i16[0]
        // i16 y = context.data.i16[1]
        // u16 button = context.data.u16[2]
        MouseDragEnd = 0x22,

        MaxEventCode = 0xFF       // Максимальный код события, который можно использовать внутри.
    };
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