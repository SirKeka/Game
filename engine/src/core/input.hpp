#pragma once

#include "defines.hpp"

enum Buttons {
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_MIDDLE,
    BUTTON_MAX_BUTTONS
};

#define DEFINE_KEY(name, code) KEY_##name = code

enum Keys {
    DEFINE_KEY(BACKSPACE, 0x08),
    DEFINE_KEY(ENTER, 0x0D),
    DEFINE_KEY(TAB, 0x09),
    DEFINE_KEY(SHIFT, 0x10),
    DEFINE_KEY(CONTROL, 0x11),

    DEFINE_KEY(PAUSE, 0x13),
    DEFINE_KEY(CAPITAL, 0x14), // Caps Lock

    DEFINE_KEY(ESCAPE, 0x1B),

    DEFINE_KEY(CONVERT, 0x1C),
    DEFINE_KEY(NONCONVERT, 0x1D),
    DEFINE_KEY(ACCEPT, 0x1E),
    DEFINE_KEY(MODECHANGE, 0x1F),

    DEFINE_KEY(SPACE, 0x20),
    DEFINE_KEY(PRIOR, 0x21),
    DEFINE_KEY(NEXT, 0x22),
    DEFINE_KEY(END, 0x23),
    DEFINE_KEY(HOME, 0x24),
    DEFINE_KEY(LEFT, 0x25),
    DEFINE_KEY(UP, 0x26),
    DEFINE_KEY(RIGHT, 0x27),
    DEFINE_KEY(DOWN, 0x28),
    DEFINE_KEY(SELECT, 0x29),
    DEFINE_KEY(PRINT, 0x2A),
    DEFINE_KEY(_EXECUTE, 0x2B),
    DEFINE_KEY(SNAPSHOT, 0x2C),
    DEFINE_KEY(INSERT, 0x2D),
    DEFINE_KEY(DELETE, 0x2E),
    DEFINE_KEY(HELP, 0x2F),

    DEFINE_KEY(A, 0x41), // Ф
    DEFINE_KEY(B, 0x42), // И
    DEFINE_KEY(C, 0x43), // С
    DEFINE_KEY(D, 0x44), // В
    DEFINE_KEY(E, 0x45), // У
    DEFINE_KEY(F, 0x46), // А
    DEFINE_KEY(G, 0x47), // П
    DEFINE_KEY(H, 0x48), // Р
    DEFINE_KEY(I, 0x49), // Ш
    DEFINE_KEY(J, 0x4A), // О
    DEFINE_KEY(K, 0x4B), // Л
    DEFINE_KEY(L, 0x4C), // Д
    DEFINE_KEY(M, 0x4D), // Ь
    DEFINE_KEY(N, 0x4E), // Т
    DEFINE_KEY(O, 0x4F), // Щ
    DEFINE_KEY(P, 0x50), // З
    DEFINE_KEY(Q, 0x51), // Й
    DEFINE_KEY(R, 0x52), // К
    DEFINE_KEY(S, 0x53), // Ы
    DEFINE_KEY(T, 0x54), // Е
    DEFINE_KEY(U, 0x55), // Г
    DEFINE_KEY(V, 0x56), // М
    DEFINE_KEY(W, 0x57), // Ц
    DEFINE_KEY(X, 0x58), // Ч
    DEFINE_KEY(Y, 0x59), // Н
    DEFINE_KEY(Z, 0x5A), // Я

    DEFINE_KEY(LWIN, 0x5B),
    DEFINE_KEY(RWIN, 0x5C),
    DEFINE_KEY(APPS, 0x5D),

    DEFINE_KEY(SLEEP, 0x5F),

    DEFINE_KEY(NUMPAD0, 0x60),
    DEFINE_KEY(NUMPAD1, 0x61),
    DEFINE_KEY(NUMPAD2, 0x62),
    DEFINE_KEY(NUMPAD3, 0x63),
    DEFINE_KEY(NUMPAD4, 0x64),
    DEFINE_KEY(NUMPAD5, 0x65),
    DEFINE_KEY(NUMPAD6, 0x66),
    DEFINE_KEY(NUMPAD7, 0x67),
    DEFINE_KEY(NUMPAD8, 0x68),
    DEFINE_KEY(NUMPAD9, 0x69),
    DEFINE_KEY(MULTIPLY, 0x6A),
    DEFINE_KEY(ADD, 0x6B),
    DEFINE_KEY(SEPARATOR, 0x6C),
    DEFINE_KEY(SUBTRACT, 0x6D),
    DEFINE_KEY(DECIMAL, 0x6E),
    DEFINE_KEY(DIVIDE, 0x6F),
    DEFINE_KEY(F1, 0x70),
    DEFINE_KEY(F2, 0x71),
    DEFINE_KEY(F3, 0x72),
    DEFINE_KEY(F4, 0x73),
    DEFINE_KEY(F5, 0x74),
    DEFINE_KEY(F6, 0x75),
    DEFINE_KEY(F7, 0x76),
    DEFINE_KEY(F8, 0x77),
    DEFINE_KEY(F9, 0x78),
    DEFINE_KEY(F10, 0x79),
    DEFINE_KEY(F11, 0x7A),
    DEFINE_KEY(F12, 0x7B),
    DEFINE_KEY(F13, 0x7C),
    DEFINE_KEY(F14, 0x7D),
    DEFINE_KEY(F15, 0x7E),
    DEFINE_KEY(F16, 0x7F),
    DEFINE_KEY(F17, 0x80),
    DEFINE_KEY(F18, 0x81),
    DEFINE_KEY(F19, 0x82),
    DEFINE_KEY(F20, 0x83),
    DEFINE_KEY(F21, 0x84),
    DEFINE_KEY(F22, 0x85),
    DEFINE_KEY(F23, 0x86),
    DEFINE_KEY(F24, 0x87),

    DEFINE_KEY(NUMLOCK, 0x90),
    DEFINE_KEY(SCROLL, 0x91),

    DEFINE_KEY(NUMPAD_EQUAL, 0x92),

    DEFINE_KEY(LSHIFT, 0xA0),
    DEFINE_KEY(RSHIFT, 0xA1),
    DEFINE_KEY(LCONTROL, 0xA2),
    DEFINE_KEY(RCONTROL, 0xA3),
    DEFINE_KEY(LMENU, 0xA4),
    DEFINE_KEY(RMENU, 0xA5),

    DEFINE_KEY(SEMICOLON, 0xBA), // Ж ; :
    DEFINE_KEY(PLUS, 0xBB), 
    DEFINE_KEY(COMMA, 0xBC),     // Б , <
    DEFINE_KEY(MINUS, 0xBD),
    DEFINE_KEY(PERIOD, 0xBE),    // Ю .
    DEFINE_KEY(SLASH, 0xBF),     // . , / ?
    DEFINE_KEY(GRAVE, 0xC0),     // Ё ` ~ 

    KEYS_MAX_KEYS
};

// Параметры клавиатуры
struct Keyboard
{
    bool keys[256];
};
// Параметры мыши
struct Mouse
{
    // TODO: исправить на i32 если мы хотим добавить поддержку нескольких мониторов с высоким разрешением
    i16 PosX;
    i16 PosY;
    u8 Buttons[BUTTON_MAX_BUTTONS];
};
struct InputState {
    Keyboard KeyboardCurrent;
    Keyboard KeyboardPrevious;
    Mouse MouseCurrent;
    Mouse MousePrevious;
};

class Input
{
private:
    // struct InputState;
    static bool initialized;
    static InputState InState;

public:
    Input(/* args */);
    ~Input();

//void input_initialize();
//void input_shutdown();
void InputUpdate(f64 DeltaTime);

// ввод с клавиатуры

MAPI bool InputIsKeyDown(const Keys& key);
MAPI bool InputIsKeyUp(const Keys& key);
MAPI bool InputWasKeyDown(const Keys& key);
MAPI bool InputWasKeyUp(const Keys& key);

void InputProcessKey(const Keys& key, bool pressed);

// ввод с помощью мыши
MAPI bool InputIsButtonDown(const Buttons& button);
MAPI bool InputIsButtonUp(const Buttons& button);
MAPI bool InputWasButtonDown(const Buttons& button);
MAPI bool InputWasButtonUp(const Buttons& button);
MAPI void InputGetMousePosition(i16& x, i16& y);
MAPI void InputGetPreviousMousePosition(i16& x, i16& y);

void InputProcessButton(Buttons button, bool pressed);
void InputProcessMouseMove(const i16& x, const i16& y);
void InputProcessMouseWheel(i8 z_delta);
};
