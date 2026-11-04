#pragma once

#include "containers/stack.h"
#include "keymap.h"

class InputSystem
{
public:
    InputSystem(InputSystem&)            = delete;
    InputSystem& operator=(InputSystem&) = delete;

    static InputSystem* Initialize(u64& MemoryRequirement, void* memory);
    MAPI static void Update(f64 DeltaTime);

    // Ввод с клавиатуры
    MAPI static bool IsKeyDown(Keys key);
    MAPI bool IsKeyUp(Keys key);
    MAPI static bool WasKeyDown(Keys key);
    MAPI bool WasKeyUp(Keys key);

    MAPI static void ProcessKey(Keys key, bool pressed);

    // Ввод с помощью мыши

    /// @brief Указывает, нажата ли данная кнопка мыши в данный момент.
    /// @param button кнопка для проверки.
    /// @return True, если кнопка нажата в данный момент; в противном случае — false.
    MAPI static bool IsButtonDown(Buttons button);

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
    MAPI static bool IsButtonDragging(Buttons button);

    /// @brief Получает текущее положение мыши.
    /// @param x ссылка для фиксации текущего положения мыши по оси X.
    /// @param y ссылка для фиксации текущего положения мыши по оси Y.
    MAPI static void GetMousePosition(i16& x, i16& y);

    /// @brief Получает предыдущее положение мыши.
    /// @param x ссылка для фиксации текущего положения мыши по оси X.
    /// @param y ссылка для фиксации текущего положения мыши по оси Y.
    MAPI void GetPreviousMousePosition(i16& x, i16& y);

    /// @brief Устанавливает состояние нажатия заданной кнопки мыши.
    /// @param button кнопка мыши, состояние которой необходимо установить.
    /// @param pressed указывает, нажата ли кнопка мыши в данный момент.
    MAPI static void ProcessButton(Buttons button, bool pressed);

    /// @brief Устанавливает текущее положение мыши в соответствии с заданными координатами X и Y. Также предварительно обновляет предыдущее положение.
    MAPI static void ProcessMouseMove(i16 x, i16 y);

    /// @brief Обрабатывает прокрутку колеса мыши.
    /// @param Zdelta Величина прокрутки по оси Z (колесо мыши).
    MAPI static void ProcessMouseWheel(i8 Zdelta);

    /// @brief Включает/отключает повтор нажатия клавиш клавиатуры.
    /// @param enable Указывает, следует ли включить повтор нажатия клавиш.
    MAPI static void RepeatsEnable(bool enable);

    /// @brief Возвращает строковое представление предоставленной клавиши. Например, «tab» для клавиши Tab.
    /// @param key 
    /// @return 
    MAPI const char* KeycodeStr(Keys key);

    MAPI static void KeymapPush(const Keymap& map);
    MAPI static bool KeymapPop();

    static bool CheckModifiers(Keymap::Modifier modifiers);
private:
    /// @brief Параметры клавиатуры
    struct Keyboard {
        bool keys[256];
    };

    /// @brief Параметры мыши
    struct Mouse
    {
        // ЗАДАЧА: исправить на i32 если мы хотим добавить поддержку нескольких мониторов с высоким разрешением
        i16 PosX;
        i16 PosY;
        bool buttons[(u32)Buttons::Max];
        bool dragging[(u32)Buttons::Max];
    };

    Keyboard KeyboardCurrent;
    Keyboard KeyboardPrevious;
    Mouse MouseCurrent;
    Mouse MousePrevious;

    Stack<Keymap> KeymapStack;
    bool AllowKeyRepeats;

    constexpr InputSystem() : KeyboardCurrent(), KeyboardPrevious(), MouseCurrent(), MousePrevious(), KeymapStack(), AllowKeyRepeats() {}

    static inline InputSystem* pInput;
};
