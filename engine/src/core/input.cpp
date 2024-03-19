#include "input.hpp"
#include "application.hpp"

bool Input::initialized = false;
InputState Input::InState {};

Input::Input()
{
    initialized = true;
    MINFO("Подсистема ввода инициализирована.");
}

Input::~Input()
{
    // TODO: Добавить процедуры выключения при необходимости.
    initialized = false;
}

void Input::Update(f64 DeltaTime)
{
    if (!initialized) {
        return;
    }

    // Копировать текущие состояния в предыдущие состояния. TODO: подумать о перемещении, а не копировании
        InState.KeyboardPrevious = InState.KeyboardCurrent;
        InState.MousePrevious = InState.MouseCurrent;
}

bool Input::IsKeyDown(Keys key)
{
    if (!initialized) {
        return false;
    }
    return InState.KeyboardCurrent.keys[key] == true;
}

bool Input::IsKeyUp(Keys key)
{
    if (!initialized) {
        return true;
    }
    return InState.KeyboardCurrent.keys[key] == false;
}

bool Input::WasKeyDown(Keys key)
{
    if (!initialized) {
        return false;
    }
    return InState.KeyboardPrevious.keys[key] == true;
}

bool Input::WasKeyUp(Keys key)
{
    if (!initialized) {
        return true;
    }
    return InState.KeyboardPrevious.keys[key] == false;
}

void Input::ProcessKey(Keys key, bool pressed)
{
    // Обрабатывайте это только в том случае, если состояние действительно изменилось.
    if (InState.KeyboardCurrent.keys[key] != pressed) {
        // Обновить внутреннее состояние.
        InState.KeyboardCurrent.keys[key] = pressed;

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
        Event::Fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

bool Input::IsButtonDown(Buttons button)
{
    if (!initialized) {
        return false;
    }
    return InState.MouseCurrent.Buttons[button] == true;
}

bool Input::IsButtonUp(Buttons button)
{
    if (!initialized) {
        return true;
    }
    return InState.MouseCurrent.Buttons[button] == false;
}

bool Input::WasButtonDown(Buttons button)
{
    if (!initialized) {
        return false;
    }
    return InState.MousePrevious.Buttons[button] == TRUE;
}

bool Input::WasButtonUp(Buttons button)
{
    if (!initialized) {
        return true;
    }
    return InState.MousePrevious.Buttons[button] == false;
}  

void Input::GetMousePosition(i16& x, i16& y)
{
    if (!initialized) {
        x = 0;
        y = 0;
        return;
    }
    x = InState.MouseCurrent.PosX;
    y = InState.MouseCurrent.PosY;
}

void Input::GetPreviousMousePosition(i16& x, i16& y)
{
    if (!initialized) {
        x = 0;
        y = 0;
        return;
    }
    x = InState.MousePrevious.PosX;
    y = InState.MousePrevious.PosY;
}

void Input::ProcessButton(Buttons button, bool pressed)
{
    // Если состояние изменилось, создайте событие.
    if (InState.MouseCurrent.Buttons[button] != pressed) {
        InState.MouseCurrent.Buttons[button] = pressed;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = button;
        Event::Fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void Input::ProcessMouseMove(i16 x, i16 y)
{
    // Обрабатывайте только в том случае, если на самом деле они разные
    if (InState.MouseCurrent.PosX != x || InState.MouseCurrent.PosY != y) {
        // ПРИМЕЧАНИЕ. Включите это при отладке.
        //MDEBUG("Позиция мыши: %i, %i!", x, y);

        // Обновить внутреннее состояние.
        InState.MouseCurrent.PosX = x;
        InState.MouseCurrent.PosY = y;

        // Запустите событие.
        EventContext context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        Event::Fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void Input::ProcessMouseWheel(i8 z_delta)
{
    // ПРИМЕЧАНИЕ. Внутреннее состояние для обновления отсутствует.

    // Запустите событие.
    EventContext context;
    context.data.u8[0] = z_delta;
    Event::Fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

void *Input::operator new(u64 size)
{
    return Application::AllocMemory(size);
}
