#pragma once

#include "defines.hpp"

enum class Buttons { Left, Right, Middle, Max };

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
    PAGEUP      = 0x21,
    PAGEDOWN    = 0x22,
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
    Equal       = 0xBB, 
    COMMA       = 0xBC,    // Б , <
    MINUS       = 0xBD,
    PERIOD      = 0xBE,    // Ю .
    SLASH       = 0xBF,    // . , / ?
    GRAVE       = 0xC0,    // Ё ` ~ 

    /// @brief Клавиша левой (квадратной) скобки, например [{
    LBRACKET = 0xDB,
    /// @brief Клавиша вертикальной черты/обратной косой черты
    PIPE = 0xDC,
    /// @brief Псевдоним для клавиши вертикальной черты/обратной косой черты
    BACKSLASH = PIPE,
    /// @brief Клавиша правой (квадратной) скобки, например ]}
    RBRACKET = 0xDD,
    /// @brief Клавиша апострофа/одинарной кавычки
    APOSTROPHE = 0xDE,
    /// @brief Псевдоним для клавиши APOSTROPHE, апострофа/одинарной кавычки
    QUOTE = APOSTROPHE,

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

class MAPI Keymap 
{
public:
    enum ModifierBits {
        ModifierNoneBit    = 0x0,
        ModifierShiftBit   = 0x1,
        ModifierControlBit = 0x2,
        ModifierAltBit     = 0x4,
    };

    using Modifier = u32;

    enum EntryBindType {
        /// @brief Неопределенное сопоставление, которое можно переопределить.
        BindTypeUndefined = 0x0,
        /// @brief Обратный вызов выполняется при первоначальном нажатии клавиши.
        BindTypePress     = 0x1,
        /// @brief Обратный вызов выполняется при отпускании клавиши.
        BindTypeRelease   = 0x2,
        /// @brief Обратный вызов выполняется непрерывно, пока клавиша удерживается.
        BindTypeHold      = 0x4,
        /// @brief Используется для отключения привязки клавиш на карте более низкого уровня.
        BindTypeUnset     = 0x8
    };

    using Callback = void(*)(Keys, EntryBindType, Modifier, void*);

    struct Binding {
        EntryBindType type;
        ModifierBits modifiers;
        Callback callback;
        void* UserData;
        struct Binding* next;
    };

    struct Entry {
        Keys key;
        // Связанный список привязок.
        Binding* bindings;
    };

    bool OverridesAll;
    Entry entries[static_cast<u16>(Keys::MaxKeys)];

    constexpr Keymap() : OverridesAll(false), entries() {
        for (u16 i = 0; i < static_cast<u16>(Keys::MaxKeys); i++) {
            entries->key = static_cast<Keys>(i);
        }
    } 
    ~Keymap();

    void BindingAdd(Keys key, EntryBindType type, Modifier modifiers, void* UserData, Callback callback);
    void BindingRemove(Keys key, EntryBindType type, Modifier modifiers, Callback callback);
    void Clear();
};

// ЗАДАЧА: Раскладки клавиш заменят существующие проверки состояний клавиш, 
// поскольку вместо этого они будут вызывать функции обратного вызова. 
// Раскладки будут добавлены в стек, где привязки будут заменены по ходу дела. 
// Например, если базовая раскладка клавиш определяет клавишу "escape" как выход из приложения, 
// то другая добавленная раскладка клавиш переопределяет клавишу на "ничего" при добавлении новой привязки для "a", 
// тогда привязка "a" будет работать, а "escape" ничего не сделает. Если "escape" не определен во второй раскладке клавиш, 
// исходная привязка останется неизменной. Раскладки вставляются/извлекаются, как и ожидалось, в стеке.