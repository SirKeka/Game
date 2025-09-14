#pragma once

#include "memory/linear_allocator.hpp"
#include "containers/stack.hpp"
#include "keymap.h"

namespace InputSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();
    MAPI void Update(f64 DeltaTime);

    // Ввод с клавиатуры
    MAPI bool IsKeyDown(Keys key);
    MAPI bool IsKeyUp(Keys key);
    MAPI bool WasKeyDown(Keys key);
    MAPI bool WasKeyUp(Keys key);

    MAPI void ProcessKey(Keys key, bool pressed);

    // Ввод с помощью мыши
    MAPI bool IsButtonDown(Buttons button);
    MAPI bool IsButtonUp(Buttons button);
    MAPI bool WasButtonDown(Buttons button);
    MAPI bool WasButtonUp(Buttons button);
    MAPI void GetMousePosition(i16& x, i16& y);
    MAPI void GetPreviousMousePosition(i16& x, i16& y);

    MAPI void ProcessButton(Buttons button, bool pressed);
    MAPI void ProcessMouseMove(i16 x, i16 y);
    MAPI void ProcessMouseWheel(i8 Zdelta);

    MAPI void KeymapPush(const Keymap& map);
    MAPI bool KeymapPop();

    bool CheckModifiers(Keymap::Modifier modifiers);
    constexpr u16 ToInt(Keys key);
    constexpr u32 ToInt(Buttons button);
};
