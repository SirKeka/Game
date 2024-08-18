#include "platform.hpp"

// Уровень платформы Windows.
#if MPLATFORM_WINDOWS

#include "core/logger.hpp"
#include "core/application.hpp"
#include "core/input.hpp"
#include "core/mthread.hpp"
#include "core/mmutex.hpp"

#include <windowsx.h>  // извлечение входных параметров
#include <windows.h>
#include <stdlib.h>

// Для создания поверхности
#include <renderer/vulkan/vulkan_api.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


struct PlatformState
{
    HINSTANCE HInstance;   // Дескриптор экземпляра приложения
    HWND hwnd;             // Дескриптор окна
    VkSurfaceKHR surface;

    PlatformState() : HInstance(0), hwnd(0), surface(0) { };
    void* operator new(u64 size) { return LinearAllocator::Instance().Allocate(size); }
};

// Прототип функции обратного вызова для обработки сообщений
LRESULT CALLBACK Win32MessageProcessor(HWND, u32, WPARAM, LPARAM);

// Часы
static f64 ClockFrequency;
static LARGE_INTEGER StartTime;

MWindow::MWindow(const char * name, i32 x, i32 y, i32 width, i32 height)
{
    this->name = name;
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
}

MWindow::~MWindow()
{
    PlatformState *state = reinterpret_cast<PlatformState*>(InternalState);
    delete state;
}

bool MWindow::Create()
{
    InternalState = new PlatformState();
    PlatformState *state = reinterpret_cast<PlatformState*>(InternalState);
    state->HInstance = GetModuleHandleA(0);

    // Настройка и регистрация класса окна.
    HICON icon = LoadIcon(state->HInstance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;                     // Получайте двойные клики
    wc.lpfnWndProc = Win32MessageProcessor;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state->HInstance;
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
        state->HInstance,    // Дескриптор экземпляра программы
        NULL
    );

    if (handle == 0) {
    MessageBoxA(NULL, "Не удалось создать окно!", "Ошибка!", MB_ICONEXCLAMATION | MB_OK);

    MFATAL("Не удалось создать окно!");
    return false;
    } else {
    state->hwnd = handle;
    }

    // Показывает окно
    b32 ShouldActivate = 1;  // ЗАДАЧА: если окно не должно принимать вводимые данные, это должно быть значение false.
    i32 ShowWindowCommandFlags = ShouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // Если изначально свернуто, используйте SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // Если изначально развернуто, используйте SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(state->hwnd, ShowWindowCommandFlags);
    //UpdateWindow(state->hwnd);

    ClockSetup();

    return true;

}

void MWindow::Close()
{
    PlatformState *state = reinterpret_cast<PlatformState*>(InternalState);
    if (state->hwnd) {
        DestroyWindow(state->hwnd);
        state->hwnd = 0;
    }
    //this->~MWindow();
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
    if (!ClockFrequency) {
        ClockSetup();
    }

    LARGE_INTEGER NowTime;
    QueryPerformanceCounter(&NowTime);
    return (f64)NowTime.QuadPart * ClockFrequency;
}

void MWindow::ClockSetup()
{
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);          // Тактовая чистота процессора
    ClockFrequency = 1.0 / (f64)Frequency.QuadPart;
    QueryPerformanceCounter(&StartTime);
}

void *MWindow::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
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
    return memmove(dest, source, size);
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

i32 PlatformGetProcessorCount()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    MINFO("Обнаружено %i ядер процессора.", sysinfo.dwNumberOfProcessors);
    return sysinfo.dwNumberOfProcessors;
}

// ПРИМЕЧАНИЕ: начало потоков---------------------------------------------------------------------------------------------

constexpr MThread::MThread(PFN_ThreadStart StartFunctionPtr, void *params, bool AutoDetach) : data(), ThreadID()
{
    if (!StartFunctionPtr) {
        return;
    }

    data = CreateThread(
        0,
        0,                                          // Размер стека по умолчанию
        (LPTHREAD_START_ROUTINE)StartFunctionPtr,   // указатель на функцию
        params,                                     // параметр для передачи потоку
        0,
        (DWORD *)&ThreadID);
    MDEBUG("Запуск процесса в потоке с идентификатором: %#x", ThreadID);
    if (!data) {
        return;
    }
    if (StartFunctionPtr) {
        CloseHandle(data);
    }
}

MThread::~MThread()
{
    if (data) {
        DWORD ExitCode;
        GetExitCodeThread(data, &ExitCode);
        // if (exit_code == STILL_ACTIVE) {
        //     TerminateThread(data, 0);  // 0 = failure
        // }
        CloseHandle((HANDLE)data);
        data = nullptr;
        ThreadID = 0;
    }
}

void MThread::Detach()
{
    if (data) {
        CloseHandle(data);
        data = nullptr;
    }
}

void MThread::Cancel()
{
    if (data) {
        TerminateThread(data, 0);
        data = nullptr;
    }
}

bool MThread::IsActive()
{
    if (data) {
        DWORD ExitCode = WaitForSingleObject(data, 0);
        if (ExitCode == WAIT_TIMEOUT) {
            return true;
        }
    }
    return false;
}

void MThread::Sleep(u64 ms)
{
    PlatformSleep(ms);
}

u64 MThread::GetThreadID()
{
    return (u64)GetCurrentThreadId();
}

// ПРИМЕЧАНИЕ: конец потоков----------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------
// ПРИМЕЧАНИЕ: начало мьютксов--------------------------------------------------------------------------------------------

MMutex::MMutex() : data (CreateMutex(0, 0, 0))
{
    if (!data) {
        MERROR("Невозможно создать мьютекс.");
        return;
    }
    // MTRACE("Создан мьютекс.");
}

MMutex::~MMutex()
{
    if (data) {
        CloseHandle(data);
        // MTRACE("Мьютекс уничтожен.");
        data = nullptr;
    }
}

bool MMutex::Lock()
{
    DWORD result = WaitForSingleObject(data, INFINITE);
    switch (result) {
        // Поток получил право собственности на мьютекс
        case WAIT_OBJECT_0:
            // MTRACE("Мьютекс заблокирован.");
            return true;

            // Поток получил право собственности на заброшенный мьютекс.
        case WAIT_ABANDONED:
            MERROR("Блокировка мьютекса не удалась.");
            return false;
    }
    // MTRACE("Мьютекс заблокирован.");
    return true;
}

bool MMutex::Unlock()
{
    if (!data) {
        return false;
    }
    i32 result = ReleaseMutex(data);
    // MTRACE("Мьютекс разблокирован.");
    return result != 0;  // 0 — это неудача
}

// ПРИМЕЧАНИЕ: конец мьютексов

void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray)
{
    NameDarray.PushBack("VK_KHR_win32_surface");
}

// Создание поверхности для Vulkan
bool PlatformCreateVulkanSurface(MWindow *window, VulkanAPI *VkAPI) {
    PlatformState *state = reinterpret_cast<PlatformState*>(window->InternalState);

    VkWin32SurfaceCreateInfoKHR CreateInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    CreateInfo.hinstance = state->HInstance;
    CreateInfo.hwnd = state->hwnd;

    VkResult result = vkCreateWin32SurfaceKHR(VkAPI->instance, &CreateInfo, VkAPI->allocator, &state->surface);
    if (result != VK_SUCCESS) {
        MFATAL("Не удалось создать поверхность Вулкана.");
        return false;
    }

    VkAPI->surface = state->surface;
    return true;
}

LRESULT CALLBACK Win32MessageProcessor(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        /*case WM_ERASEBKGND:
            // Сообщите ОС, что удаление будет выполняться приложением для предотвращения мерцания.
            return 1;*/
        case WM_CLOSE:
            // ЗАДАЧА: Вызов события для закрытия приложения.
            EventContext data;
            Event::GetInstance()->Fire(EVENT_CODE_APPLICATION_QUIT, nullptr, data);
            return 0;
            // Вызывается, когда пользователь закрывает окно
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            // Получаем обновленный размер.
            RECT r;
            GetClientRect(hwnd, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;

            // Запускаем событие. Уровень приложения должен уловить это, но не обрабатывать, 
            // поскольку это должно быть видно другим частям приложения.
            EventContext context;
            context.data.u16[0] = (u16)width;
            context.data.u16[1] = (u16)height;
            Event::GetInstance()->Fire(EVENT_CODE_RESIZED, nullptr, context);
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            // Клавиша нажата/отпущена
            bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            Keys key = static_cast<Keys> (w_param);

            // Проверьте наличие расширенного кода сканирования.
            bool IsExtended = (HIWORD(l_param) & KF_EXTENDED) == KF_EXTENDED;
            if (w_param == VK_MENU) {
                key = IsExtended ? Keys::RALT : Keys::LALT;
            } else if (w_param == VK_SHIFT) {
                // Досадно, что KF_EXTENDED не установлен для клавиш shift.
                u32 LeftShift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);
                u32 scancode = ((l_param & (0xFF << 16)) >> 16);
                key = scancode == LeftShift ? Keys::LSHIFT : Keys::RSHIFT;
            } else if (w_param == VK_CONTROL) {
                key = IsExtended ? Keys::RCONTROL : Keys::LCONTROL;
            }

            // Перейдите к подсистеме ввода для обработки.
            Input::Instance()->ProcessKey(key, pressed);

            // Верните 0, чтобы предотвратить поведение окна по умолчанию при некоторых нажатиях клавиш, таких как alt.
            return 0;
        }  
        case WM_MOUSEMOVE: {
            // Mouse move
            i16 xPos = GET_X_LPARAM(l_param);
            i16 yPos = GET_Y_LPARAM(l_param);

            // Переходим к подсистеме ввода.
            Input::Instance()->ProcessMouseMove(xPos, yPos);
        } break;
        case WM_MOUSEWHEEL: {
            i32 zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (zDelta != 0) {
                // Свести входные данные в независимый от ОС (-1, 1)
                zDelta = (zDelta < 0) ? -1 : 1;
                Input::Instance()->ProcessMouseWheel(zDelta);
            }
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            bool pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
            Buttons MouseButtons = Buttons::Max;
            switch (msg)
            {
            case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    MouseButtons = Buttons::Left;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    MouseButtons = Buttons::Middle;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    MouseButtons = Buttons::Right;
                    break;
            }
            // Переходим к подсистеме ввода.
            if (MouseButtons != Buttons::Max) {
                Input::Instance()->ProcessButton(MouseButtons, pressed);
            }
        } break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif // MPLATFORM_WINDOWS