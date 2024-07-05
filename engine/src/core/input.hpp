#pragma once

#include "defines.hpp"

#include "memory/linear_allocator.hpp"

enum class Buttons {
    Left,
    Right,
    Middle,
    Max
};

//#define DEFINE_KEY(name, code) KEY_##name = code

enum class Keys : u16 {
    BACKSPACE   = 0x08,
    ENTER       = 0x0D,
    TAB         = 0x09,
    SHIFT       = 0x10,
    CONTROL     = 0x11,

    PAUSE       = 0x13,
    CAPITAL     = 0x14,     // Caps Lock

    ESCAPE      = 0x1B,

    CONVERT     = 0x1C,
    NONCONVERT  = 0x1D,
    ACCEPT      = 0x1E,
    MODECHANGE  = 0x1F,

    SPACE       = 0x20,
    PRIOR       = 0x21,
    NEXT        = 0x22,
    END         = 0x23,
    HOME        = 0x24,
    LEFT        = 0x25,
    UP          = 0x26,
    RIGHT       = 0x27,
    DOWN        = 0x28,
    SELECT      = 0x29,
    PRINT       = 0x2A,
    EXECUTE     = 0x2B,     // просто EXECUTE не рабоатет
    SNAPSHOT    = 0x2C,
    INSERT      = 0x2D,
    DELETE      = 0x2E,
    HELP        = 0x2F,

    KEY_0       = 0x30,     // 1
    KEY_1       = 0x31,     // 2
    KEY_2       = 0x32,     // 3
    KEY_3       = 0x33,     // 4
    KEY_4       = 0x34,     // 5
    KEY_5       = 0x35,     // 6
    KEY_6       = 0x36,     // 7
    KEY_7       = 0x37,     // 8
    KEY_8       = 0x38,     // 9
    KEY_9       = 0x39,     // 0

    A           = 0x41,     // Ф
    B           = 0x42,     // И
    C           = 0x43,     // С
    D           = 0x44,     // В
    E           = 0x45,     // У
    F           = 0x46,     // А
    G           = 0x47,     // П
    H           = 0x48,     // Р
    I           = 0x49,     // Ш
    J           = 0x4A,     // О
    K           = 0x4B,     // Л
    L           = 0x4C,     // Д
    M           = 0x4D,     // Ь
    N           = 0x4E,     // Т
    O           = 0x4F,     // Щ
    P           = 0x50,     // З
    Q           = 0x51,     // Й
    R           = 0x52,     // К
    S           = 0x53,     // Ы
    T           = 0x54,     // Е
    U           = 0x55,     // Г
    V           = 0x56,     // М
    W           = 0x57,     // Ц
    X           = 0x58,     // Ч
    Y           = 0x59,     // Н
    Z           = 0x5A,     // Я

    LWIN        = 0x5B,
    RWIN        = 0x5C,
    APPS        = 0x5D,

    SLEEP       = 0x5F,

    NUMPAD0     = 0x60,
    NUMPAD1     = 0x61,
    NUMPAD2     = 0x62,
    NUMPAD3     = 0x63,
    NUMPAD4     = 0x64,
    NUMPAD5     = 0x65,
    NUMPAD6     = 0x66,
    NUMPAD7     = 0x67,
    NUMPAD8     = 0x68,
    NUMPAD9     = 0x69,
    MULTIPLY    = 0x6A,
    ADD         = 0x6B,
    SEPARATOR   = 0x6C,
    SUBTRACT    = 0x6D,
    DECIMAL     = 0x6E,
    DIVIDE      = 0x6F,
    F1          = 0x70,
    F2          = 0x71,
    F3          = 0x72,
    F4          = 0x73,
    F5          = 0x74,
    F6          = 0x75,
    F7          = 0x76,
    F8          = 0x77,
    F9          = 0x78,
    F10         = 0x79,
    F11         = 0x7A,
    F12         = 0x7B,
    F13         = 0x7C,
    F14         = 0x7D,
    F15         = 0x7E,
    F16         = 0x7F,
    F17         = 0x80,
    F18         = 0x81,
    F19         = 0x82,
    F20         = 0x83,
    F21         = 0x84,
    F22         = 0x85,
    F23         = 0x86,
    F24         = 0x87,

    NUMLOCK     = 0x90,
    SCROLL      = 0x91,

    NumpadEqual = 0x92,

    LSHIFT      = 0xA0,
    RSHIFT      = 0xA1,
    LCONTROL    = 0xA2,
    RCONTROL    = 0xA3,
    LALT        = 0xA4,
    RALT        = 0xA5,

    SEMICOLON   = 0xBA,    // Ж ; :
    PLUS        = 0xBB, 
    COMMA       = 0xBC,    // Б , <
    MINUS       = 0xBD,
    PERIOD      = 0xBE,    // Ю .
    SLASH       = 0xBF,    // . , / ?
    GRAVE       = 0xC0,    // Ё ` ~ 

    MaxKeys
};

constexpr bool operator==(Keys l, u16 r)
{
    return static_cast<u16>(l) == r;
}

constexpr bool operator== (u16 l, Keys r)
{
    return r == l;
}

// Параметры клавиатуры
struct Keyboard
{
    bool keys[256];
};
// Параметры мыши
struct Mouse
{
    // ЗАДАЧА: исправить на i32 если мы хотим добавить поддержку нескольких мониторов с высоким разрешением
    i16 PosX;
    i16 PosY;
    u8 Buttons[static_cast<u32>(Buttons::Max)];
};

class MAPI Input
{
private:
    Keyboard KeyboardCurrent;
    Keyboard KeyboardPrevious;
    Mouse MouseCurrent;
    Mouse MousePrevious;

    static Input* input;

    Input() : KeyboardCurrent(), KeyboardPrevious(), MouseCurrent(), MousePrevious() {};
public:
    ~Input();

    Input(const Input&) = delete;
    Input& operator= (const Input&) = delete;

    void Initialize();
    void Sutdown();
    void Update(f64 DeltaTime);

    // ввод с клавиатуры

    bool IsKeyDown(Keys key);
    bool IsKeyUp(Keys key);
    bool WasKeyDown(Keys key);
    bool WasKeyUp(Keys key);

    void ProcessKey(Keys key, bool pressed);

    // ввод с помощью мыши
    bool IsButtonDown(Buttons button);
    bool IsButtonUp(Buttons button);
    bool WasButtonDown(Buttons button);
    bool WasButtonUp(Buttons button);
    void GetMousePosition(i16& x, i16& y);
    void GetPreviousMousePosition(i16& x, i16& y);

    void ProcessButton(Buttons button, bool pressed);
    void ProcessMouseMove(i16 x, i16 y);
    void ProcessMouseWheel(i8 z_delta);

    static Input* Instance();
    void* operator new(u64 size);
private:
    constexpr u16 ToInt(Keys key);
    constexpr u32 ToInt(Buttons button);
};
