#pragma once

#include "defines.hpp"

#include "memory/linear_allocator.hpp"
#include "containers/stack.hpp"
#include "keymap.hpp"

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
    bool Buttons[static_cast<u32>(Buttons::Max)];
};

class Input
{
private:
    Keyboard KeyboardCurrent;
    Keyboard KeyboardPrevious;
    Mouse MouseCurrent;
    Mouse MousePrevious;

    Stack<Keymap> KeymapStack;

    static Input* input;

    constexpr Input();
public:
    ~Input();

    Input(const Input&) = delete;
    Input& operator= (const Input&) = delete;

    static void Initialize(LinearAllocator& SystemAllocator);
    static void Sutdown();
    MAPI static void Update(f64 DeltaTime);

    // ввод с клавиатуры

    MAPI static bool IsKeyDown(Keys key);
    MAPI static bool IsKeyUp(Keys key);
    MAPI static bool WasKeyDown(Keys key);
    MAPI static bool WasKeyUp(Keys key);

    MAPI static void ProcessKey(Keys key, bool pressed);

    // ввод с помощью мыши
    MAPI static bool IsButtonDown(Buttons button);
    MAPI static bool IsButtonUp(Buttons button);
    MAPI static bool WasButtonDown(Buttons button);
    MAPI static bool WasButtonUp(Buttons button);
    MAPI static void GetMousePosition(i16& x, i16& y);
    MAPI static void GetPreviousMousePosition(i16& x, i16& y);

    MAPI static void ProcessButton(Buttons button, bool pressed);
    MAPI static void ProcessMouseMove(i16 x, i16 y);
    MAPI static void ProcessMouseWheel(i8 Zdelta);

    MAPI static void KeymapPush(const Keymap& map);
    MAPI static void KeymapPop();

    MAPI static Input* Instance();
private:
    static bool CheckModifiers(Keymap::Modifier modifiers);
    static constexpr u16 ToInt(Keys key);
    static constexpr u32 ToInt(Buttons button);
};
