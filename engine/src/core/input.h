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

    /// @brief Указывает, нажата ли данная кнопка мыши в данный момент.
    /// @param button кнопка для проверки.
    /// @return True, если кнопка нажата в данный момент; в противном случае — false.
    MAPI bool IsButtonDown(Buttons button);

    /// @brief Указывает, отпущена ли данная кнопка мыши.
    /// @param button кнопка для проверки.
    /// @return True, если кнопка отпущена; в противном случае — false.
    MAPI bool IsButtonUp(Buttons button);

    /// @brief Указывает, была ли данная кнопка мыши нажата ранее в последнем кадре.
    /// @param button кнопка для проверки.
    /// @return True, если кнопка была нажата ранее; в противном случае — false.
    MAPI bool WasButtonDown(Buttons button);

    /// @brief Указывает, была ли данная кнопка мыши отпущена в последнем кадре.
    /// @param button кнопка для проверки.
    /// @return True, если кнопка мыши была отпущена ранее; в противном случае — false.
    MAPI bool WasButtonUp(Buttons button);

    /// @brief Указывает, перетаскивается ли в данный момент мышь с нажатой соответствующей кнопкой.
    /// @param button кнопка для проверки.
    /// @return True при перетаскивании; в противном случае false.
    MAPI bool IsButtonDragging(Buttons button);

    /// @brief Получает текущее положение мыши.
    /// @param x ссылка для фиксации текущего положения мыши по оси X.
    /// @param y ссылка для фиксации текущего положения мыши по оси Y.
    MAPI void GetMousePosition(i16& x, i16& y);

    /// @brief Получает предыдущее положение мыши.
    /// @param x ссылка для фиксации текущего положения мыши по оси X.
    /// @param y ссылка для фиксации текущего положения мыши по оси Y.
    MAPI void GetPreviousMousePosition(i16& x, i16& y);

    /// @brief Устанавливает состояние нажатия заданной кнопки мыши.
    /// @param button кнопка мыши, состояние которой необходимо установить.
    /// @param pressed указывает, нажата ли кнопка мыши в данный момент.
    MAPI void ProcessButton(Buttons button, bool pressed);

    /// @brief Устанавливает текущее положение мыши в соответствии с заданными координатами X и Y. Также предварительно обновляет предыдущее положение.
    MAPI void ProcessMouseMove(i16 x, i16 y);

    /// @brief Обрабатывает прокрутку колеса мыши.
    /// @param Zdelta Величина прокрутки по оси Z (колесо мыши).
    MAPI void ProcessMouseWheel(i8 Zdelta);

    /// @brief Возвращает строковое представление предоставленной клавиши. Например, «tab» для клавиши Tab.
    /// @param key 
    /// @return 
    MAPI const char* KeycodeStr(Keys key);

    MAPI void KeymapPush(const Keymap& map);
    MAPI bool KeymapPop();

    bool CheckModifiers(Keymap::Modifier modifiers);
    constexpr u16 ToInt(Keys key);
    constexpr u32 ToInt(Buttons button);
};
