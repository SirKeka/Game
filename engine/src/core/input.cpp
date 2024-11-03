#include "input.hpp"
#include "event.hpp"
#include "memory/linear_allocator.hpp"
#include "keymap.hpp"

#include <new>

Input* Input::input = nullptr;

constexpr Input::Input() : KeyboardCurrent(), KeyboardPrevious(), MouseCurrent(), MousePrevious(), KeymapStack() {}

Input::~Input()
{
    // ЗАДАЧА: Добавить процедуры выключения при необходимости.
    
}

void Input::Initialize(LinearAllocator& SystemAllocator)
{
    if (!input) {
        void* ptrMem = SystemAllocator.Allocate(sizeof(Input));
        input = new(ptrMem) Input();
    };
    
    MINFO("Подсистема ввода инициализирована.");
}

void Input::Sutdown()
{
    //delete input;
}

void Input::Update(f64 DeltaTime)
{
    if (!input) {
        return;
    }

    // Обрабатывать привязки удержания.
    for (u32 k = 0; k < ToInt(Keys::MaxKeys); ++k) {
        Keys key = static_cast<Keys>(k);
        if (IsKeyDown(key) && WasKeyDown(key)) {
            u32 MapCount = input->KeymapStack.ElementCount;
            auto maps = input->KeymapStack.data;
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
    input->KeyboardPrevious = input->KeyboardCurrent;
    input->MousePrevious = input->MouseCurrent;
}

bool Input::IsKeyDown(Keys key)
{
    if (!input) {
        return false;
    }
    return input->KeyboardCurrent.keys[ToInt(key)] == true;
}

bool Input::IsKeyUp(Keys key)
{
    if (!input) {
        return true;
    }
    return input->KeyboardCurrent.keys[ToInt(key)] == false;
}

bool Input::WasKeyDown(Keys key)
{
    if (!input) {
        return false;
    }
    return input->KeyboardPrevious.keys[ToInt(key)] == true;
}

bool Input::WasKeyUp(Keys key)
{
    if (!input) {
        return true;
    }
    return input->KeyboardPrevious.keys[ToInt(key)] == false;
}

void Input::ProcessKey(Keys key, bool pressed)
{
    // Обрабатывайте это только в том случае, если состояние действительно изменилось.
    if (input->KeyboardCurrent.keys[ToInt(key)] != pressed) {
        // Обновить внутреннее состояние.
        input->KeyboardCurrent.keys[ToInt(key)] = pressed;

        if (key == Keys::LALT) {
            MINFO("Левая клавиша alt %s.", pressed ? "нажата" : "отпущена");
        } else if (key == Keys::RALT) {
            MINFO("Правая клавиша alt %s.", pressed ? "нажата" : "отпущена");
        }

        if (key == Keys::LCONTROL) {
            MINFO("Левая клавиша ctrl %s.", pressed ? "нажата" : "отпущена");
        } else if (key == Keys::RCONTROL) {
            MINFO("Правая клавиша ctrl %s.", pressed ? "нажата" : "отпущена");
        }

        if (key == Keys::LSHIFT) {
            MINFO("Левая клавиша shift %s.", pressed ? "нажата" : "отпущена");
        } else if (key == Keys::RSHIFT) {
            MINFO("Правая клавиша shift %s.", pressed ? "нажата" : "отпущена");
        }

        // Проверка привязок клавиш
        // Итерация сопоставлений клавиш сверху вниз по стеку.
        u32 MapCount = input->KeymapStack.ElementCount;
        auto maps = input->KeymapStack.data;
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
        Event::GetInstance()->Fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

bool Input::IsButtonDown(Buttons button)
{
    if (!input) {
        return false;
    }
    return input->MouseCurrent.Buttons[ToInt(button)] == true;
}

bool Input::IsButtonUp(Buttons button)
{
    if (!input) {
        return true;
    }
    return input->MouseCurrent.Buttons[ToInt(button)] == false;
}

bool Input::WasButtonDown(Buttons button)
{
    if (!input) {
        return false;
    }
    return input->MousePrevious.Buttons[ToInt(button)] == true;
}

bool Input::WasButtonUp(Buttons button)
{
    if (!input) {
        return true;
    }
    return input->MousePrevious.Buttons[ToInt(button)] == false;
}  

void Input::GetMousePosition(i16& x, i16& y)
{
    if (!input) {
        x = 0;
        y = 0;
        return;
    }
    x = input->MouseCurrent.PosX;
    y = input->MouseCurrent.PosY;
}

void Input::GetPreviousMousePosition(i16& x, i16& y)
{
    if (!input) {
        x = 0;
        y = 0;
        return;
    }
    x = input->MousePrevious.PosX;
    y = input->MousePrevious.PosY;
}

void Input::ProcessButton(Buttons button, bool pressed)
{
    // Если состояние изменилось, создайте событие.
    if (input->MouseCurrent.Buttons[ToInt(button)] != pressed) {
        input->MouseCurrent.Buttons[ToInt(button)] = pressed;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = ToInt(button);
        Event::GetInstance()->Fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void Input::ProcessMouseMove(i16 x, i16 y)
{
    // Обрабатывайте только в том случае, если на самом деле они разные
    if (input->MouseCurrent.PosX != x || input->MouseCurrent.PosY != y) {
        // ПРИМЕЧАНИЕ. Включите это при отладке.
        //MDEBUG("Позиция мыши: %i, %i!", x, y);

        // Обновить внутреннее состояние.
        input->MouseCurrent.PosX = x;
        input->MouseCurrent.PosY = y;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        Event::GetInstance()->Fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void Input::ProcessMouseWheel(i8 z_delta)
{
    // ПРИМЕЧАНИЕ. Внутреннее состояние для обновления отсутствует.

    // Запустите событие.
    EventContext context;
    context.data.i8[0] = z_delta;
    Event::GetInstance()->Fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

void Input::KeymapPush(const Keymap &map)
{
    if (input) {
        // Добавьте раскладку в стек, затем примените ее.
        if (!input->KeymapStack.Push(map)) {
            MERROR("Не удалось отправить раскладку клавиатуры!");
            return;
        }
    }
}

void Input::KeymapPop()
{
    if (input) {
        // Извлеките раскладку из стека, затем повторно примените стек.
        Keymap popped;
        if (!input->KeymapStack.Pop(popped)) {
            MERROR("Не удалось извлечь раскладку клавиатуры!");
            return;
        }
    }
}

Input *Input::Instance()
{
    return input;
}

bool Input::CheckModifiers(Keymap::Modifier modifiers)
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

constexpr u16 Input::ToInt(Keys key)
{
    return (u16)key;
}

constexpr u32 Input::ToInt(Buttons button)
{
    return (u32)button;
}
