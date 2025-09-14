#include "input.hpp"
#include "event.h"
#include "memory/linear_allocator.hpp"
#include "keymap.h"

#include <new>

/// @brief Параметры клавиатуры
struct Keyboard
{
    bool keys[256];
};

/// @brief Параметры мыши
struct Mouse
{
    // ЗАДАЧА: исправить на i32 если мы хотим добавить поддержку нескольких мониторов с высоким разрешением
    i16 PosX;
    i16 PosY;
    bool Buttons[static_cast<u32>(Buttons::Max)];
};

struct Input
{
    Keyboard KeyboardCurrent;
    Keyboard KeyboardPrevious;
    Mouse MouseCurrent;
    Mouse MousePrevious;

    Stack<Keymap> KeymapStack;

    constexpr Input() : KeyboardCurrent(), KeyboardPrevious(), MouseCurrent(), MousePrevious(), KeymapStack() {}
};

static Input* pInput = nullptr;

bool InputSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    MemoryRequirement = sizeof(Input);
    if (!memory) {
        return true;
    }
    
    if (!pInput) {
        pInput = new(memory) Input();
    };

    if (!pInput) {
        return false;
    }
    
    MINFO("Подсистема ввода инициализирована.");
    return true;
}

void InputSystem::Shutdown()
{
    pInput = nullptr;
}

void InputSystem::Update(f64 DeltaTime)
{
    if (!pInput) {
        return;
    }

    // Обрабатывать привязки удержания.
    for (u32 k = 0; k < ToInt(Keys::MaxKeys); ++k) {
        Keys key = static_cast<Keys>(k);
        if (IsKeyDown(key) && WasKeyDown(key)) {
            u32 MapCount = pInput->KeymapStack.ElementCount;
            auto maps = pInput->KeymapStack.data;
            for (i32 m = MapCount - 1; m >= 0; --m) {
                auto& map = maps[m];
                auto binding = &map.entries[k].bindings[0];
                bool unset = false;
                while (binding) {
                    // Если обнаружена неустановка, остановить обработку.
                    if (binding->type == Keymap::BindTypeUnset) {
                        unset = true;
                        break;
                    } else if (binding->type == Keymap::BindTypeHold) {
                        if (binding->callback && CheckModifiers(binding->modifiers)) {
                            binding->callback(key, binding->type, binding->modifiers, binding->UserData);
                        }
                    }

                    binding = binding->next;
                }
                // Если обнаружена неустановка или карта отмечена для переопределения всех, остановить обработку.
                if (unset || map.OverridesAll) {
                    break;
                }
            }
        }
    }

    // Копировать текущие состояния в предыдущие состояния. ЗАДАЧА: подумать о перемещении, а не копировании
    pInput->KeyboardPrevious = pInput->KeyboardCurrent;
    pInput->MousePrevious = pInput->MouseCurrent;
}

bool InputSystem::IsKeyDown(Keys key)
{
    if (!pInput) {
        return false;
    }
    return pInput->KeyboardCurrent.keys[ToInt(key)] == true;
}

bool InputSystem::IsKeyUp(Keys key)
{
    if (!pInput) {
        return true;
    }
    return pInput->KeyboardCurrent.keys[ToInt(key)] == false;
}

bool InputSystem::WasKeyDown(Keys key)
{
    if (!pInput) {
        return false;
    }
    return pInput->KeyboardPrevious.keys[ToInt(key)] == true;
}

bool InputSystem::WasKeyUp(Keys key)
{
    if (!pInput) {
        return true;
    }
    return pInput->KeyboardPrevious.keys[ToInt(key)] == false;
}

void InputSystem::ProcessKey(Keys key, bool pressed)
{
    // Обрабатывайте это только в том случае, если состояние действительно изменилось.
    if (pInput->KeyboardCurrent.keys[ToInt(key)] != pressed) {
        // Обновить внутреннее состояние.
        pInput->KeyboardCurrent.keys[ToInt(key)] = pressed;

        // if (key == Keys::LALT) {
        //     MINFO("Левая клавиша alt %s.", pressed ? "нажата" : "отпущена");
        // } else if (key == Keys::RALT) {
        //     MINFO("Правая клавиша alt %s.", pressed ? "нажата" : "отпущена");
        // }

        // if (key == Keys::LCONTROL) {
        //     MINFO("Левая клавиша ctrl %s.", pressed ? "нажата" : "отпущена");
        // } else if (key == Keys::RCONTROL) {
        //     MINFO("Правая клавиша ctrl %s.", pressed ? "нажата" : "отпущена");
        // }

        // if (key == Keys::LSHIFT) {
        //     MINFO("Левая клавиша shift %s.", pressed ? "нажата" : "отпущена");
        // } else if (key == Keys::RSHIFT) {
        //     MINFO("Правая клавиша shift %s.", pressed ? "нажата" : "отпущена");
        // }

        // Проверка привязок клавиш
        // Итерация сопоставлений клавиш сверху вниз по стеку.
        u32 MapCount = pInput->KeymapStack.ElementCount;
        auto maps = pInput->KeymapStack.data;
        for (i32 m = MapCount - 1; m >= 0; --m) {
            auto& map = maps[m];
            auto binding = &map.entries[ToInt(key)].bindings[0];
            bool unset = false;
            while (binding) {
                // Если обнаружена неустановка, остановите обработку.
                if (binding->type == Keymap::BindTypeUnset) {
                    unset = true;
                    break;
                } else if (pressed && binding->type == Keymap::BindTypePress) {
                    if (binding->callback && CheckModifiers(binding->modifiers)) {
                        binding->callback(key, binding->type, binding->modifiers, binding->UserData);
                    }
                } else if (!pressed && binding->type == Keymap::BindTypeRelease) {
                    if (binding->callback && CheckModifiers(binding->modifiers)) {
                        binding->callback(key, binding->type, binding->modifiers, binding->UserData);
                    }
                }

                binding = binding->next;
            }
            // Если обнаружена неустановка или сопоставление отмечено как переопределяющее все, остановите обработку.
            if (unset || map.OverridesAll) {
                break;
            }
        }

        // Запустите событие для немедленной обработки.
        EventContext context;
        context.data.u16[0] = ToInt(key);
        EventSystem::Fire(pressed ? EventSystem::KeyPressed : EventSystem::KeyReleased, 0, context);
    }
}

bool InputSystem::IsButtonDown(Buttons button)
{
    if (!pInput) {
        return false;
    }
    return pInput->MouseCurrent.Buttons[ToInt(button)] == true;
}

bool InputSystem::IsButtonUp(Buttons button)
{
    if (!pInput) {
        return true;
    }
    return pInput->MouseCurrent.Buttons[ToInt(button)] == false;
}

bool InputSystem::WasButtonDown(Buttons button)
{
    if (!pInput) {
        return false;
    }
    return pInput->MousePrevious.Buttons[ToInt(button)] == true;
}

bool InputSystem::WasButtonUp(Buttons button)
{
    if (!pInput) {
        return true;
    }
    return pInput->MousePrevious.Buttons[ToInt(button)] == false;
}  

void InputSystem::GetMousePosition(i16& x, i16& y)
{
    if (!pInput) {
        x = 0;
        y = 0;
        return;
    }
    x = pInput->MouseCurrent.PosX;
    y = pInput->MouseCurrent.PosY;
}

void InputSystem::GetPreviousMousePosition(i16& x, i16& y)
{
    if (!pInput) {
        x = 0;
        y = 0;
        return;
    }
    x = pInput->MousePrevious.PosX;
    y = pInput->MousePrevious.PosY;
}

void InputSystem::ProcessButton(Buttons button, bool pressed)
{
    // Если состояние изменилось, создайте событие.
    if (pInput->MouseCurrent.Buttons[ToInt(button)] != pressed) {
        pInput->MouseCurrent.Buttons[ToInt(button)] = pressed;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = ToInt(button);
        context.data.i16[1] = pInput->MouseCurrent.PosX;
        context.data.i16[2] = pInput->MouseCurrent.PosY;
        EventSystem::Fire(pressed ? EventSystem::ButtonPressed : EventSystem::ButtonReleased, nullptr, context);
    }
}

void InputSystem::ProcessMouseMove(i16 x, i16 y)
{
    // Обрабатывайте только в том случае, если на самом деле они разные
    if (pInput->MouseCurrent.PosX != x || pInput->MouseCurrent.PosY != y) {
        // ПРИМЕЧАНИЕ. Включите это при отладке.
        //MDEBUG("Позиция мыши: %i, %i!", x, y);

        // Обновить внутреннее состояние.
        pInput->MouseCurrent.PosX = x;
        pInput->MouseCurrent.PosY = y;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        EventSystem::Fire(EventSystem::MouseMoved, nullptr, context);
    }
}

void InputSystem::ProcessMouseWheel(i8 z_delta)
{
    // ПРИМЕЧАНИЕ. Внутреннее состояние для обновления отсутствует.

    // Запустите событие.
    EventContext context;
    context.data.i8[0] = z_delta;
    EventSystem::Fire(EventSystem::MouseWheel, nullptr, context);
}

void InputSystem::KeymapPush(const Keymap &map)
{
    if (pInput) {
        // Добавьте раскладку в стек, затем примените ее.
        if (!pInput->KeymapStack.Push(map)) {
            MERROR("Не удалось отправить раскладку клавиатуры!");
            return;
        }
    }
}

bool InputSystem::KeymapPop()
{
    if (pInput) {
        // Извлеките раскладку из стека, затем повторно примените стек.
        Keymap popped;
        if (!pInput->KeymapStack.Pop(popped)) {
            MERROR("Не удалось извлечь раскладку клавиатуры!");
            return false;
        }
        return true;
    }

    return false;
}

bool InputSystem::CheckModifiers(Keymap::Modifier modifiers)
{
    if (modifiers & Keymap::ModifierShiftBit) {
        if (!IsKeyDown(Keys::SHIFT) && !IsKeyDown(Keys::LSHIFT) && !IsKeyDown(Keys::RSHIFT)) {
            return false;
        }
    }
    if (modifiers & Keymap::ModifierControlBit) {
        if (!IsKeyDown(Keys::CONTROL) && !IsKeyDown(Keys::LCONTROL) && !IsKeyDown(Keys::RCONTROL)) {
            return false;
        }
    }
    if (modifiers & Keymap::ModifierAltBit) {
        if (!IsKeyDown(Keys::LALT) && !IsKeyDown(Keys::RALT)) {
            return false;
        }
    }

    return true;
}

constexpr u16 InputSystem::ToInt(Keys key)
{
    return (u16)key;
}

constexpr u32 InputSystem::ToInt(Buttons button)
{
    return (u32)button;
}
