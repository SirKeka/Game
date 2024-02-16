#include "platform.hpp"

// Уровень платформы Windows.
#if MPLATFORM_WINDOWS

#include "core/logger.hpp"
#include "core/application.hpp"
#include "core/input.hpp"
#include "containers/darray.hpp"

#include <windowsx.h>  // извлечение входных параметров
#include <stdlib.h>

// Прототип функции обратного вызова для обработки сообщений
LRESULT CALLBACK Win32MessageProcessor(HWND, u32, WPARAM, LPARAM);

f64 MWindow::ClockFrequency = 0;
LARGE_INTEGER MWindow::StartTime;

MWindow::MWindow(const char * name, i32 x, i32 y, i32 width, i32 height)
{
    this->name = name;
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
}

bool MWindow::Create()
{
// Настройка и регистрация класса окна.
    HICON icon = LoadIcon(HInstance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;                     // Получайте двойные клики
    wc.lpfnWndProc = Win32MessageProcessor;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = HInstance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // NULL; // Управление курсором вручную
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);           // Прозрачный
    wc.lpszClassName = "Moon Window Class";    // Имя класса

    // Регистрация класса окна
    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Регистрация окна не удалась", "Ошибка", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    u32 WinStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | COLOR_WINDOW; // Стиль окна
    u32 WinExStyle = WS_EX_APPWINDOW;

    WinStyle |= WS_MAXIMIZEBOX;
    WinStyle |= WS_MINIMIZEBOX;
    WinStyle |= WS_THICKFRAME;

    // Получите размер рамки.
    RECT BorderRect = {0, 0, 0, 0};
    AdjustWindowRectEx(&BorderRect, WinStyle, 0, WinExStyle);

    // В этом случае прямоугольник рамки имеет отрицательное значение.
    x += BorderRect.left;
    y += BorderRect.top;

    // Увеличиваться на размер рамки операционной системы.
    width += BorderRect.right - BorderRect.left;
    height += BorderRect.bottom - BorderRect.top;

    HWND handle = CreateWindowExA(
        WinExStyle, 
        "Moon Window Class", // Имя класса
        name,                // Текст заголовка
        WinStyle,            // Стиль окна
        x,                   // Инициализация позиции х окна
        y,                   // Инициализация позиции y окна
        width,               // Инициализация ширины окна
        height,              // Инициализация высоты окна
        NULL,                // Дескриптор родительского окна
        NULL,                // Дескриптор меню окна
        HInstance,           // Дескриптор экземпляра программы
        NULL);

        if (handle == 0) {
        MessageBoxA(NULL, "Не удалось создать окно!", "Ошибка!", MB_ICONEXCLAMATION | MB_OK);

        MFATAL("Не удалось создать окно!");
        return FALSE;
    } else {
        hwnd = handle;
    }

    // Показывает окно
    b32 ShouldActivate = 1;  // TODO: если окно не должно принимать вводимые данные, это должно быть значение false.
    i32 ShowWindowCommandFlags = ShouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // Если изначально свернуто, используйте SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // Если изначально развернуто, используйте SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(hwnd, ShowWindowCommandFlags);
    //UpdateWindow(state->hwnd);

    // Настройка часов
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);          // Тактовая чистота процессора
    ClockFrequency = 1.0 / (f64)Frequency.QuadPart;
    QueryPerformanceCounter(&StartTime);

    return true;

}

void MWindow::Close()
{
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = 0;
    }
}

bool MWindow::Messages()
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return true;
}

f64 MWindow::PlatformGetAbsoluteTime()
{
    LARGE_INTEGER NowTime;
    QueryPerformanceCounter(&NowTime);
    return (f64)NowTime.QuadPart * ClockFrequency;
}

void *PlatformAllocate(u64 size, bool aligned) {
    return malloc(size);
}

void PlatformFree(void *block, bool aligned) {
    free(block);
}

void *PlatformZeroMemory(void *block, u64 size) {
    return memset(block, 0, size);
}

void *PlatformCopyMemory(void *dest, const void *source, u64 size) {
    return memcpy(dest, source, size);
}

void *PlatformSetMemory(void *dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

void PlatformConsoleWrite(const char *message, u8 colour) {
    HANDLE ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    // FATAL,ОШИБКА,ПРЕДУПРЕЖДЕНИЕ,ИНФОРМАЦИЯ,ОТЛАДКА,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(ConsoleHandle, levels[colour]);
    OutputDebugStringA(message);  // Позволяет вывести сообщение в консоль отладки vs code
    u64 length = strlen(message);
    LPDWORD NumberWritten = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, NumberWritten, 0);
}

void PlatformConsoleWriteError(const char *message, u8 colour) {
    HANDLE ConsoleHandle = GetStdHandle(STD_ERROR_HANDLE);
    // FATAL,ОШИБКА,ПРЕДУПРЕЖДЕНИЕ,ИНФОРМАЦИЯ,ОТЛАДКА,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(ConsoleHandle, levels[colour]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD NumberWritten = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, NumberWritten, 0);
}

void PlatformSleep(u64 ms) {
    Sleep(ms);
}

void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray)
{
    NameDarray.PushBack("VK_KHR_win32_surface");
}

LRESULT CALLBACK Win32MessageProcessor(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        /*case WM_ERASEBKGND:
            // Сообщите ОС, что удаление будет выполняться приложением для предотвращения мерцания.
            return 1;*/
        case WM_CLOSE:
            // TODO: Вызов события для закрытия приложения.
            return 0;
            // Вызывается, когда пользователь закрывает окно
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            // Получаем обновленный размер.
            // RECT r;
            // GetClientRect(hwnd, &r);
            // u32 width = r.right - r.left;
            // u32 height = r.bottom - r.top;

            // TODO: Fire an event for window resize.
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            // Клавиша нажата/отпущена
            bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            Keys key = static_cast<Keys> (w_param);

            // Перейдите к подсистеме ввода для обработки.
            Input::InputProcessKey(key, pressed);
        } break;
        case WM_MOUSEMOVE: {
            // Mouse move
            i16 xPos = GET_X_LPARAM(l_param);
            i16 yPos = GET_Y_LPARAM(l_param);

            // Переходим к подсистеме ввода.
            Input::InputProcessMouseMove(xPos, yPos);
        } break;
        case WM_MOUSEWHEEL: {
            i32 zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (zDelta != 0) {
                // Свести входные данные в независимый от ОС (-1, 1)
                zDelta = (zDelta < 0) ? -1 : 1;
                Input::InputProcessMouseWheel(zDelta);
            }
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            bool pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
            Buttons MouseButtons = BUTTON_MAX_BUTTONS;
            switch (msg)
            {
            case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    MouseButtons = BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    MouseButtons = BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    MouseButtons = BUTTON_RIGHT;
                    break;
            }
            // Переходим к подсистеме ввода.
            if (MouseButtons != BUTTON_MAX_BUTTONS) {
                Input::InputProcessButton(MouseButtons, pressed);
            }
        } break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif // MPLATFORM_WINDOWS