#include "input.h"
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
    bool buttons[static_cast<u32>(Buttons::Max)];
    bool dragging[static_cast<u32>(Buttons::Max)];
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
    return pInput->MouseCurrent.buttons[ToInt(button)] == true;
}

bool InputSystem::IsButtonUp(Buttons button)
{
    if (!pInput) {
        return true;
    }
    return pInput->MouseCurrent.buttons[ToInt(button)] == false;
}

bool InputSystem::WasButtonDown(Buttons button)
{
    if (!pInput) {
        return false;
    }
    return pInput->MousePrevious.buttons[ToInt(button)] == true;
}

bool InputSystem::WasButtonUp(Buttons button)
{
    if (!pInput) {
        return true;
    }
    return pInput->MousePrevious.buttons[ToInt(button)] == false;
}

bool InputSystem::IsButtonDragging(Buttons button)
{
    if (!pInput) {
        return false;
    }
    return pInput->MouseCurrent.dragging[ToInt(button)];
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
    if (pInput->MouseCurrent.buttons[ToInt(button)] != pressed) {
        pInput->MouseCurrent.buttons[ToInt(button)] = pressed;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = ToInt(button);
        context.data.i16[1] = pInput->MouseCurrent.PosX;
        context.data.i16[2] = pInput->MouseCurrent.PosY;
        EventSystem::Fire(pressed ? EventSystem::ButtonPressed : EventSystem::ButtonReleased, nullptr, context);
    }
    // Проверьте, не было ли перетаскивания прекращено.
    if (!pressed && pInput->MouseCurrent.dragging[ToInt(button)]) {
        // Вызовите событие завершения перетаскивания.

        pInput->MouseCurrent.dragging[ToInt(button)] = false;
        // MTRACE("перетаскивание мыши завершено в точке: x:%hi, y:%hi, кнопка: %hu", pInput->MouseCurrent.x, pInput->MouseCurrent.y, button);

        EventContext context;
        context.data.i16[0] = pInput->MouseCurrent.PosX;
        context.data.i16[1] = pInput->MouseCurrent.PosY;
        context.data.u16[2] = ToInt(button);
        EventSystem::Fire(EventSystem::MouseDragEnd, nullptr, context);
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

        for (u16 i = 0; i < static_cast<u32>(Buttons::Max); ++i) {
            // Сначала проверьте, нажата ли кнопка.
            if (pInput->MouseCurrent.buttons[i]) {
                if (!pInput->MousePrevious.dragging[i] && !pInput->MouseCurrent.dragging[i]) {
                    // Начните перетаскивание этой кнопки.

                    pInput->MouseCurrent.dragging[i] = true;

                    EventContext DragContext;
                    DragContext.data.i16[0] = pInput->MouseCurrent.PosX;
                    DragContext.data.i16[1] = pInput->MouseCurrent.PosY;
                    DragContext.data.u16[2] = i;
                    EventSystem::Fire(EventSystem::MouseDragBegin, nullptr, DragContext);
                    // MTRACE("перетаскивание мыши началось в точке: x:%hi, y:%hi, кнопка: %hu", pInput->MouseCurrent.x, pInput->MouseCurrent.y, i);
                } else if (pInput->MouseCurrent.dragging[i]) {
                    // Выдайте команду на продолжение операции перетаскивания.
                    EventContext DragContext;
                    DragContext.data.i16[0] = pInput->MouseCurrent.PosX;
                    DragContext.data.i16[1] = pInput->MouseCurrent.PosY;
                    DragContext.data.u16[2] = i;
                    EventSystem::Fire(EventSystem::MouseDragged, nullptr, DragContext);
                    // MTRACE("перетаскивание мыши продолжилось в точке: x:%hi, y:%hi, кнопка: %hu", pInput->MouseCurrent.x, pInput->MouseCurrent.y, i);
                }
            }
        }
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

const char *InputSystem::KeycodeStr(Keys key)
{
    switch (key) {
        case Keys::BACKSPACE:
            return "backspace";
        case Keys::ENTER:
            return "enter";
        case Keys::TAB:
            return "tab";
        case Keys::SHIFT:
            return "shift";
        case Keys::CONTROL:
            return "ctrl";
        case Keys::PAUSE:
            return "pause";
        case Keys::CAPITAL:
            return "capslock";
        case Keys::ESCAPE:
            return "esc";

        case Keys::CONVERT:
            return "ime_convert";
        case Keys::NONCONVERT:
            return "ime_noconvert";
        case Keys::ACCEPT:
            return "ime_accept";
        case Keys::MODECHANGE:
            return "ime_modechange";

        case Keys::SPACE:
            return "space";
        case Keys::PAGEUP:
            return "pageup";
        case Keys::PAGEDOWN:
            return "pagedown";
        case Keys::END:
            return "end";
        case Keys::HOME:
            return "home";
        case Keys::LEFT:
            return "left";
        case Keys::UP:
            return "up";
        case Keys::RIGHT:
            return "right";
        case Keys::DOWN:
            return "down";
        case Keys::SELECT:
            return "select";
        case Keys::PRINT:
            return "print";
        case Keys::EXECUTE:
            return "execute";
        case Keys::PRINTSCREEN:
            return "printscreen";
        case Keys::INSERT:
            return "insert";
        case Keys::DELETE:
            return "delete";
        case Keys::HELP:
            return "help";

        case Keys::KEY_0:
            return "0";
        case Keys::KEY_1:
            return "1";
        case Keys::KEY_2:
            return "2";
        case Keys::KEY_3:
            return "3";
        case Keys::KEY_4:
            return "4";
        case Keys::KEY_5:
            return "5";
        case Keys::KEY_6:
            return "6";
        case Keys::KEY_7:
            return "7";
        case Keys::KEY_8:
            return "8";
        case Keys::KEY_9:
            return "9";

        case Keys::A:
            return "a";
        case Keys::B:
            return "b";
        case Keys::C:
            return "c";
        case Keys::D:
            return "d";
        case Keys::E:
            return "e";
        case Keys::F:
            return "f";
        case Keys::G:
            return "g";
        case Keys::H:
            return "h";
        case Keys::I:
            return "i";
        case Keys::J:
            return "j";
        case Keys::K:
            return "k";
        case Keys::L:
            return "l";
        case Keys::M:
            return "m";
        case Keys::N:
            return "n";
        case Keys::O:
            return "o";
        case Keys::P:
            return "p";
        case Keys::Q:
            return "q";
        case Keys::R:
            return "r";
        case Keys::S:
            return "s";
        case Keys::T:
            return "t";
        case Keys::U:
            return "u";
        case Keys::V:
            return "v";
        case Keys::W:
            return "w";
        case Keys::X:
            return "x";
        case Keys::Y:
            return "y";
        case Keys::Z:
            return "z";

        case Keys::LWIN: // SUPER
            return "l_win";
        case Keys::RWIN: // SUPER
            return "r_win";
        case Keys::APPS:
            return "apps";

        case Keys::SLEEP:
            return "sleep";

            // Numberpad keys
        case Keys::NUMPAD0:
            return "numpad_0";
        case Keys::NUMPAD1:
            return "numpad_1";
        case Keys::NUMPAD2:
            return "numpad_2";
        case Keys::NUMPAD3:
            return "numpad_3";
        case Keys::NUMPAD4:
            return "numpad_4";
        case Keys::NUMPAD5:
            return "numpad_5";
        case Keys::NUMPAD6:
            return "numpad_6";
        case Keys::NUMPAD7:
            return "numpad_7";
        case Keys::NUMPAD8:
            return "numpad_8";
        case Keys::NUMPAD9:
            return "numpad_9";
        case Keys::MULTIPLY:
            return "numpad_mult";
        case Keys::ADD:
            return "numpad_add";
        case Keys::SEPARATOR:
            return "numpad_sep";
        case Keys::SUBTRACT:
            return "numpad_sub";
        case Keys::DECIMAL:
            return "numpad_decimal";
        case Keys::DIVIDE:
            return "numpad_div";

        case Keys::F1:
            return "f1";
        case Keys::F2:
            return "f2";
        case Keys::F3:
            return "f3";
        case Keys::F4:
            return "f4";
        case Keys::F5:
            return "f5";
        case Keys::F6:
            return "f6";
        case Keys::F7:
            return "f7";
        case Keys::F8:
            return "f8";
        case Keys::F9:
            return "f9";
        case Keys::F10:
            return "f10";
        case Keys::F11:
            return "f11";
        case Keys::F12:
            return "f12";
        case Keys::F13:
            return "f13";
        case Keys::F14:
            return "f14";
        case Keys::F15:
            return "f15";
        case Keys::F16:
            return "f16";
        case Keys::F17:
            return "f17";
        case Keys::F18:
            return "f18";
        case Keys::F19:
            return "f19";
        case Keys::F20:
            return "f20";
        case Keys::F21:
            return "f21";
        case Keys::F22:
            return "f22";
        case Keys::F23:
            return "f23";
        case Keys::F24:
            return "f24";

        case Keys::NUMLOCK:
            return "num_lock";
        case Keys::SCROLL:
            return "scroll_lock";
        case Keys::NumpadEqual:
            return "numpad_equal";

        case Keys::LSHIFT:
            return "l_shift";
        case Keys::RSHIFT:
            return "r_shift";
        case Keys::LCONTROL:
            return "l_ctrl";
        case Keys::RCONTROL:
            return "r_ctrl";
        case Keys::LALT:
            return "l_alt";
        case Keys::RALT:
            return "r_alt";

        case Keys::SEMICOLON:
            return ";";

        case Keys::APOSTROPHE:
            return "'";
        case Keys::Equal:
            return "=";
        case Keys::COMMA:
            return ",";
        case Keys::MINUS:
            return "-";
        case Keys::PERIOD:
            return ".";
        case Keys::SLASH:
            return "/";

        case Keys::GRAVE:
            return "`";

        case Keys::LBRACKET:
            return "[";
        case Keys::PIPE:
            return "\\";
        case Keys::RBRACKET:
            return "]";

        default:
            return "undefined";
    }
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
