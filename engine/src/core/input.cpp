#include "input.hpp"
#include "event.hpp"
#include "memory/linear_allocator.hpp"

Input* Input::input = nullptr;

Input::~Input()
{
    // TODO: Добавить процедуры выключения при необходимости.
    
}

void Input::Initialize()
{
    if (!input) {
        input = new Input();
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

    // Копировать текущие состояния в предыдущие состояния. TODO: подумать о перемещении, а не копировании
        KeyboardPrevious = KeyboardCurrent;
        MousePrevious = MouseCurrent;
}

bool Input::IsKeyDown(Keys key)
{
    if (!input) {
        return false;
    }
    return KeyboardCurrent.keys[key] == true;
}

bool Input::IsKeyUp(Keys key)
{
    if (!input) {
        return true;
    }
    return KeyboardCurrent.keys[key] == false;
}

bool Input::WasKeyDown(Keys key)
{
    if (!input) {
        return false;
    }
    return KeyboardPrevious.keys[key] == true;
}

bool Input::WasKeyUp(Keys key)
{
    if (!input) {
        return true;
    }
    return KeyboardPrevious.keys[key] == false;
}

void Input::ProcessKey(Keys key, bool pressed)
{
    // Обрабатывайте это только в том случае, если состояние действительно изменилось.
    if (KeyboardCurrent.keys[key] != pressed) {
        // Обновить внутреннее состояние.
        KeyboardCurrent.keys[key] = pressed;

        if (key == KEY_LALT) {
            MINFO("Левая клавиша alt %s.", pressed ? "нажата" : "отпущена");
        } else if (key == KEY_RALT) {
            MINFO("Правая клавиша alt %s.", pressed ? "нажата" : "отпущена");
        }

        if (key == KEY_LCONTROL) {
            MINFO("Левая клавиша ctrl %s.", pressed ? "нажата" : "отпущена");
        } else if (key == KEY_RCONTROL) {
            MINFO("Правая клавиша ctrl %s.", pressed ? "нажата" : "отпущена");
        }

        if (key == KEY_LSHIFT) {
            MINFO("Левая клавиша shift %s.", pressed ? "нажата" : "отпущена");
        } else if (key == KEY_RSHIFT) {
            MINFO("Правая клавиша shift %s.", pressed ? "нажата" : "отпущена");
        }

        // Запустите событие для немедленной обработки.
        EventContext context;
        context.data.u16[0] = key;
        Event::GetInstance()->Fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

bool Input::IsButtonDown(Buttons button)
{
    if (!input) {
        return false;
    }
    return MouseCurrent.Buttons[button] == true;
}

bool Input::IsButtonUp(Buttons button)
{
    if (!input) {
        return true;
    }
    return MouseCurrent.Buttons[button] == false;
}

bool Input::WasButtonDown(Buttons button)
{
    if (!input) {
        return false;
    }
    return MousePrevious.Buttons[button] == true;
}

bool Input::WasButtonUp(Buttons button)
{
    if (!input) {
        return true;
    }
    return MousePrevious.Buttons[button] == false;
}  

void Input::GetMousePosition(i16& x, i16& y)
{
    if (!input) {
        x = 0;
        y = 0;
        return;
    }
    x = MouseCurrent.PosX;
    y = MouseCurrent.PosY;
}

void Input::GetPreviousMousePosition(i16& x, i16& y)
{
    if (!input) {
        x = 0;
        y = 0;
        return;
    }
    x = MousePrevious.PosX;
    y = MousePrevious.PosY;
}

void Input::ProcessButton(Buttons button, bool pressed)
{
    // Если состояние изменилось, создайте событие.
    if (MouseCurrent.Buttons[button] != pressed) {
        MouseCurrent.Buttons[button] = pressed;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = button;
        Event::GetInstance()->Fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void Input::ProcessMouseMove(i16 x, i16 y)
{
    // Обрабатывайте только в том случае, если на самом деле они разные
    if (MouseCurrent.PosX != x || MouseCurrent.PosY != y) {
        // ПРИМЕЧАНИЕ. Включите это при отладке.
        //MDEBUG("Позиция мыши: %i, %i!", x, y);

        // Обновить внутреннее состояние.
        MouseCurrent.PosX = x;
        MouseCurrent.PosY = y;

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
    context.data.u8[0] = z_delta;
    Event::GetInstance()->Fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

Input *Input::Instance()
{
    return input;
}

void *Input::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}
