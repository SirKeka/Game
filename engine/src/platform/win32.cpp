#include "platform.hpp"

#include "core/semaphor.h"

// Уровень платформы Windows.
#if MPLATFORM_WINDOWS

#include "core/event.h"
#include "core/engine.h"
#include "core/input.h"
#include "core/mmutex.h"
#include "core/mthread.h"

#define WIN32_LEAN_AND_MEAN
#include <windowsx.h>  // извлечение входных параметров
#include <windows.h>
#include <stdlib.h>

struct Win32HandleInfo {
    HINSTANCE HInstance;   // Дескриптор экземпляра приложения
    HWND hwnd;             // Дескриптор окна
};

struct Win32FileWatch {
    u32 id;
    MString FilePath;
    FILETIME LastWriteTime;

    Win32FileWatch& operator=(Win32FileWatch&& v) {
        id = v.id;
        FilePath = (MString&&)v.FilePath;
        LastWriteTime = v.LastWriteTime;

        return *this;
    }
};

struct WinPlatform : public PlatformState
{
    Win32HandleInfo handle;
    DArray<Win32FileWatch> watches;
    f32 DevicePixelRatio;

    void Shutdown() override;
};

static WinPlatform* state = nullptr;

// Прототип функции обратного вызова для обработки сообщений
LRESULT CALLBACK Win32MessageProcessor(HWND, u32, WPARAM, LPARAM);
void PlatformUpdateWatches();

// Часы
static f64 ClockFrequency;
static LARGE_INTEGER StartTime;

PlatformState* WindowSystem::Initialize(u64& MemoryRequirement, void* memory, ApplicationConfig& config)
{
    MemoryRequirement = sizeof(WinPlatform);

    if (!memory) {
        return nullptr;
    }
 
    state = new(memory) WinPlatform();
    state->handle.HInstance = GetModuleHandleA(0);

    // Доступно только в обновлении Creators Update для Windows 10+.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    // ПРИМЕЧАНИЕ: В более старых версиях Windows может потребоваться следующее:
    // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    state->DevicePixelRatio = 1.F;

    // Настройка и регистрация класса окна.
    HICON icon = LoadIcon(state->handle.HInstance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;                     // Получайте двойные клики
    wc.lpfnWndProc = Win32MessageProcessor;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state->handle.HInstance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // NULL; // Управление курсором вручную
    wc.hbrBackground = NULL;           // Прозрачный
    wc.lpszClassName = "Moon Window Class";    // Имя класса

    // Регистрация класса окна
    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Регистрация окна не удалась", "Ошибка", MB_ICONEXCLAMATION | MB_OK);
        return nullptr;
    }

    u32 WinStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | COLOR_WINDOW; // Стиль окна
    u32 WinExStyle = WS_EX_APPWINDOW;

    WinStyle |= WS_MAXIMIZEBOX;
    WinStyle |= WS_MINIMIZEBOX;
    WinStyle |= WS_THICKFRAME;

    // Получите размер рамки.
    RECT BorderRect = {0, 0, 0, 0};
    AdjustWindowRectEx(&BorderRect, WinStyle, 0, WinExStyle);

    u32 x = config.StartPosX;
    u32 y = config.StartPosY;
    u32 width  = config.StartWidth;
    u32 height = config.StartHeight;
    // В этом случае прямоугольник рамки имеет отрицательное значение.
    x += BorderRect.left;
    y += BorderRect.top;

    // Увеличиваться на размер рамки операционной системы.
    width += BorderRect.right - BorderRect.left;
    height += BorderRect.bottom - BorderRect.top;

    HWND handle = CreateWindowExA(
        WinExStyle, 
        "Moon Window Class", // Имя класса
        config.name,         // Текст заголовка
        WinStyle,            // Стиль окна
        x,                   // Инициализация позиции х окна
        y,                   // Инициализация позиции y окна
        width,               // Инициализация ширины окна
        height,              // Инициализация высоты окна
        NULL,                // Дескриптор родительского окна
        NULL,                // Дескриптор меню окна
        state->handle.HInstance,    // Дескриптор экземпляра программы
        NULL
    );

    if (handle == 0) {
        MessageBoxA(NULL, "Не удалось создать окно!", "Ошибка!", MB_ICONEXCLAMATION | MB_OK);

        MFATAL("Не удалось создать окно!");
        return nullptr;
    } else {
        state->handle.hwnd = handle;
    }

    // Показывает окно
    b32 ShouldActivate = 1;  // ЗАДАЧА: если окно не должно принимать вводимые данные, это должно быть значение false.
    i32 ShowWindowCommandFlags = ShouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // Если изначально свернуто, используйте SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // Если изначально развернуто, используйте SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(state->handle.hwnd, ShowWindowCommandFlags);
    //UpdateWindow(state->handle.hwnd);

    ClockSetup();

    return state;
}

void WinPlatform::Shutdown()
{
    if (handle.hwnd) {
        DestroyWindow(handle.hwnd);
        handle.hwnd = 0;
    }
}

bool WindowSystem::Messages()
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    PlatformUpdateWatches();
    return true;
}

f64 WindowSystem::PlatformGetAbsoluteTime()
{
    if (!ClockFrequency) {
        ClockSetup();
    }

    LARGE_INTEGER NowTime;
    QueryPerformanceCounter(&NowTime);
    return (f64)NowTime.QuadPart * ClockFrequency;
}

void WindowSystem::ClockSetup()
{
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);          // Тактовая чистота процессора
    ClockFrequency = 1.F / (f64)Frequency.QuadPart;
    QueryPerformanceCounter(&StartTime);
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
    DWORD NumberWritten = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, &NumberWritten, 0);
}

void PlatformConsoleWriteError(const char *message, u8 colour) {
    HANDLE ConsoleHandle = GetStdHandle(STD_ERROR_HANDLE);
    // FATAL,ОШИБКА,ПРЕДУПРЕЖДЕНИЕ,ИНФОРМАЦИЯ,ОТЛАДКА,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(ConsoleHandle, levels[colour]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    DWORD NumberWritten = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, &NumberWritten, 0);
}

f32 PlatformDevicePixelRatio(void)
{
    return state->DevicePixelRatio;
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

void PlatformGetHandleInfo(u64& OutSize, void* memory)
{
    OutSize = sizeof(Win32HandleInfo);
    if (!memory) {
        return;
    }
    MemorySystem::CopyMem(memory, &state->handle, OutSize);
}

bool PlatformDynamicLibraryLoad(const char* name, DynamicLibrary& OutLibrary)
{
    // MemorySystem::ZeroMem(OutLibrary, sizeof(DynamicLibrary));
    if (!name) {
        return false;
    }

    char filename[MAX_PATH]{};
    MString::Format(filename, "%s.dll", name);

    HMODULE library = LoadLibraryA(filename);
    if (!library) {
        return false;
    }

    OutLibrary.name = name;
    OutLibrary.filename = filename;

    OutLibrary.InternalDataSize = sizeof(HMODULE);
    OutLibrary.InternalData = library;

    return true;
}

bool PlatformDynamicLibraryUnload(DynamicLibrary& library)
{
    HMODULE InternalModule = (HMODULE)library.InternalData;
    if (!InternalModule) {
        return false;
    }

    BOOL result = FreeLibrary(InternalModule);
    if (result == 0) {
        return false;
    }

    if (library.name) {
        library.name.Destroy();
    }

    if (library.filename) {
        library.filename.Destroy();
    }

    if (library.functions) {
        library.functions.Clear();
    }

    // MemorySystem::ZeroMem(&library, sizeof(DynamicLibrary));

    return true;
}

bool PlatformDynamicLibraryLoadFunction(const char* name, DynamicLibrary& library)
{
    if (!name) {
        return false;
    }

    if (!library.InternalData) {
        return false;
    }

    FARPROC fAddr = GetProcAddress((HMODULE)library.InternalData, name);
    if (!fAddr) {
        return false;
    }

    auto f = library.functions.PushBack();
    f->pfn = reinterpret_cast<void*>(fAddr);
    f->name = name;

    return true;
}

const char *PlatformDynamicLibraryExtension() {
    return ".dll";
}

const char* PlatformDynamicLibraryPrefix() {
    return "";
}

PlatformError::Code PlatformCopyFile(const char *source, const char *dest, bool OverwriteIfExists)
{
    BOOL result = CopyFileA(source, dest, !OverwriteIfExists);
    if (!result) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND) {
            return PlatformError::FileNotFound;
        } else if (err == ERROR_SHARING_VIOLATION) {
            return PlatformError::FileLocked;
        } else {
            return PlatformError::Unknown;
        }
    }
    return PlatformError::Success;
}

static bool RegisterWatch(const char *FilePath, u32 &OutWatchID) 
{
    if (!state || !FilePath) {
        if (OutWatchID) {
            OutWatchID = INVALID::ID;
        }
        return false;
    }
    OutWatchID = INVALID::ID;

    WIN32_FIND_DATAA data;
    HANDLE FileHandle = FindFirstFileA(FilePath, &data);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    BOOL result = FindClose(FileHandle);
    if (result == 0) {
        return false;
    }

    const u32 count = state->watches.Length();
    for (u32 i = 0; i < count; ++i) {
        auto& w = state->watches[i];
        if (w.id == INVALID::ID) {
            // Найден свободный слот для использования.
            w.id = i;
            w.FilePath = FilePath;
            w.LastWriteTime = data.ftLastWriteTime;
            OutWatchID = i;
            return true;
        }
    }

    // Если свободного места нет, создайте и отправьте новую запись.
    Win32FileWatch w {
        .id = count,
        .FilePath = FilePath,
        .LastWriteTime = data.ftLastWriteTime
    };
    OutWatchID = count;
    state->watches.PushBack((Win32FileWatch&&)w);

    return true;
}

static bool UnregisterWatch(u32 WatchID) 
{
    if (!state || !state->watches) {
        return false;
    }

    const u32 count = state->watches.Length();
    if (count == 0 || WatchID > (count - 1)) {
        return false;
    }

    auto& w = state->watches[WatchID];
    w.id = INVALID::ID;
    w.FilePath.Destroy();
    MemorySystem::ZeroMem(&w.LastWriteTime, sizeof(FILETIME));

    return true;
}

bool PlatformWatchFile(const char *FilePath, u32 &OutWatchID) 
{
    return RegisterWatch(FilePath, OutWatchID);
}

bool PlatformUnwatchFile(u32 WatchID) 
{
    return UnregisterWatch(WatchID);
}

void PlatformUpdateWatches() {
    if (!state || !state->watches) {
        return;
    }

    const u32 count = state->watches.Length();
    for (u32 i = 0; i < count; ++i) {
        auto& f = state->watches[i];
        if (f.id != INVALID::ID) {
            WIN32_FIND_DATAA data;
            HANDLE FileHandle = FindFirstFileA(f.FilePath.c_str(), &data);
            if (FileHandle == INVALID_HANDLE_VALUE) {
                // Это означает, что файл был удален, снимите с наблюдения.
                EventContext context = {0};
                context.data.u32[0] = f.id;
                EventSystem::Fire(EventSystem::Code::WatchedFileDeleted, nullptr, context);
                MINFO("Файл с идентификатором наблюдения %d был удален.", f.id);
                UnregisterWatch(f.id);
                continue;
            }
            BOOL result = FindClose(FileHandle);
            if (result == 0) {
                continue;
            }

            // Проверьте время файла, чтобы увидеть, было ли оно изменено, и обновите/уведомите, если это так.
            if (CompareFileTime(&data.ftLastWriteTime, &f.LastWriteTime) != 0) {
                f.LastWriteTime = data.ftLastWriteTime;
                // Уведомите слушателей.
                EventContext context = {0};
                context.data.u32[0] = f.id;
                EventSystem::Fire(EventSystem::Code::WatchedFileWritten, nullptr, context);
            }
        }
    }
}

// const char* PlatformGetKeyboardLayout()
// {
//     // ЗАДАЧА: изменить.
//     HKL hk = GetKeyboardLayout(0);
//     i32 lang = LOWORD (hk);
// 
//     return nullptr;
// }

// ПРИМЕЧАНИЕ: начало потоков---------------------------------------------------------------------------------------------

bool MThread::Create(PFN_ThreadStart StartFunctionPtr, void *params, bool AutoDetach)
{
    if (!StartFunctionPtr) {
        return false;
    }
    /* Функция из Windows API (заголовок processthreadsapi.h), которая создаёт новый поток внутри текущего процесса.
     *  lpThreadAttributes (опционально). Указатель на структуру SECURITY_ATTRIBUTES. Она определяет дескриптор безопасности и то, 
        может ли дескриптор потока унаследоваться дочерними процессами. Если NULL, поток получит дескриптор безопасности по умолчанию, 
        а дескриптор не будет унаследован.
     *  dwStackSize (опционально). Начальный размер стека в байтах. Система округляет это значение до ближайшей страницы. 
        Если передать 0, новый поток возьмёт размер стека по умолчанию у создающего потока. 
     *  lpStartAddress (обязательно). Указатель на функцию, с которой начнётся выполнение потока. Она должна иметь 
        тип LPTHREAD_START_ROUTINE — это указатель на функцию, которая принимает LPVOID и возвращает DWORD.
     *  lpParameter (опционально). Параметр, который будет передан в функцию потока.
     *  dwCreationFlags. Управляет режимом создания. Часто используют флаг CREATE_SUSPENDED (0x00000004): поток создаётся в приостановленном 
        состоянии и не начинает работу, пока извне не вызовут ResumeThread. Если флаг не указан (или равен 0), поток стартует сразу.
     *  lpThreadId (опционально). Указатель на переменную, в которую функция запишет идентификатор созданного потока. */
    data = CreateThread(
        nullptr,
        0,                                          // Размер стека по умолчанию
        (LPTHREAD_START_ROUTINE)StartFunctionPtr,   // указатель на функцию
        params,                                     // параметр для передачи потоку
        0,
        (DWORD *)&ThreadID);
    MDEBUG("Запуск процесса в потоке с идентификатором: %#x", ThreadID);
    if (!data) {
        return false;
    }
    if (AutoDetach) {
        CloseHandle(data);
    }
    return true;
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

u64 GetThreadID()
{
    return (u64)GetCurrentThreadId();
}

// ПРИМЕЧАНИЕ: конец потоков----------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------
#pragma region мьютекс----------------------------------------------------------------------------------------------------

void *MMutex::Create()
{
    /*  Функция, которая создаёт именованный или безымянный объект mutex.
     *  lpMutexAttributes — указывает на структуру SECURITY_ATTRIBUTES, которая определяет, 
        может ли возвращённый дескриптор быть унаследован дочерними процессами. 
        Если lpMutexAttributes равен NULL, то дескриптор не может быть унаследован.
     *  bInitialOwner — определяет начального владельца объекта mutex. Если это значение равно TRUE 
        и вызывающий создал mutex, то вызывающий поток получает владение объектом. 
     *  lpName — если равен NULL, то объект mutex создаётся без имени. Если lpName совпадает с именем существующего события, 
        семафора, ожидаемого таймера, задания или объекта сопоставления файлов, то функция завершается с ошибкой. */
    data = CreateMutex(nullptr, 0, nullptr);
    if (!data) {
        MERROR("Создать мьютекс не удалось.");
        return nullptr;
    }
    return data;
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
    /*  Функция из Windows API (заголовок synchapi.h), которая позволяет потоку приостановиться и ждать, 
        пока какой-то объект перейдёт в сигнальное состояние. Проще говоря: она заставляет поток «замереть» 
        и не продолжать выполнение, пока не случится определённое событие.
     *  HANDLE hHandle,         -   Дескриптор объекта
     *  DWORD dwMilliseconds    -   Интервал ожидания в миллисекундах */
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

#pragma endregion конец мьютексов

#pragma region Семафор
bool Semaphore::Create(u32 MaxCount, u32 StartCount)
{
    InternalData = CreateSemaphore(0, StartCount, MaxCount, "semaphore");

    return true;
}

void Semaphore::Destroy()
{
    if (InternalData) {
        CloseHandle(InternalData);
        MTRACE("Уничтоженный дескриптор семафора.");
        InternalData = nullptr;
    }
}

bool Semaphore::Signal()
{
    if (!InternalData) {
        return false;
    }
    // W: освобождение/инкремент
    LONG PreviousCount = 0;
    // ПРИМЕЧАНИЕ: освобождать по одному за раз.
    if (!ReleaseSemaphore(InternalData, 1, &PreviousCount)) {
        MERROR("Не удалось освободить семафор.");
        return false;
    }
    return true;
    // L: post/инкремент
}

bool Semaphore::Wait(u64 TimeoutMs)
{
    if (!InternalData) {
        return false;
    }

    DWORD result = WaitForSingleObject(InternalData, TimeoutMs);
    switch (result) {
        case WAIT_ABANDONED:
            MERROR("Указанный объект представляет собой мьютекс, который не был освобожден владевшим им потоком до завершения работы этого потока. Право владения мьютексом переходит к вызывающему потоку, а состояние мьютекса устанавливается как «несигнальное» (nonsignaled). Если мьютекс использовался для защиты данных, отражающих устойчивое состояние системы, следует проверить их на непротиворечивость.");
            return false;
        case WAIT_OBJECT_0:
            // Сигнал от семафора получен.
            return true;
        case WAIT_TIMEOUT:
            MERROR("Истекло время ожидания семафора.");
            return false;
        case WAIT_FAILED:
            MERROR("Функция WaitForSingleObject завершилась с ошибкой.");
            // ЗАДАЧА: Вызвать GetLastError и вывести сообщение.
            return false;
        default:
            MERROR("При ожидании семафора произошла неизвестная ошибка.При ожидании семафора произошла неизвестная ошибка.");
            // ЗАДАЧА: Вызвать GetLastError и вывести сообщение.
            return false;
    }
    // W: ожидание/декремент, блокируется при достижении 0
    // L: ожидание/декремент, блокируется при достижении 0
    return true;
}
#pragma endregion конец семафора

LRESULT CALLBACK Win32MessageProcessor(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        /*case WM_ERASEBKGND:
            // Сообщите ОС, что удаление будет выполняться приложением для предотвращения мерцания.
            return 1;*/
        case WM_CLOSE:
            // ЗАДАЧА: Вызов события для закрытия приложения.
            EventContext data;
            EventSystem::Fire(EventSystem::ApplicationQuit, nullptr, data);
            return 0;
            // Вызывается, когда пользователь закрывает окно
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_DPICHANGED: {
            // Разрешение по осям X и Y всегда одинаковое, поэтому просто выберите один из них.
            i32 xDpi = GET_X_LPARAM(w_param);

            // Сохраните соотношение пикселей устройства.
            state->DevicePixelRatio = (f32)xDpi / USER_DEFAULT_SCREEN_DPI;
            MINFO("Соотношение пикселей устройства отображения: %.2f", state->DevicePixelRatio);

            return 0;
        }
        case WM_SIZE: {
            // Получаем обновленный размер.
            RECT r;
            GetClientRect(hwnd, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;

            {
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

                MONITORINFO MonitorInfo = {0};
                MonitorInfo.cbSize = sizeof(MONITORINFO);
                if (!GetMonitorInfoA(monitor, &MonitorInfo)) {
                    MWARN("Failed to get monitor info. ");
                }

                MINFO("monitor: %u", MonitorInfo.rcMonitor.left);
            }

            // Запускаем событие. Уровень приложения должен уловить это, но не обрабатывать, 
            // поскольку это должно быть видно другим частям приложения.
            EventContext context;
            context.data.u16[0] = (u16)width;
            context.data.u16[1] = (u16)height;
            EventSystem::Fire(EventSystem::Resized, nullptr, context);
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

            // HACK: Это отвратительная дрянь, связанная с сочетаниями клавиш в Windows.
            if (key == VK_OEM_1) {
                key = Keys::SEMICOLON;
            }

            // Перейдите к подсистеме ввода для обработки.
            InputSystem::ProcessKey(key, pressed);

            // Верните 0, чтобы предотвратить поведение окна по умолчанию при некоторых нажатиях клавиш, таких как alt.
            return 0;
        }  
        case WM_MOUSEMOVE: {
            // Mouse move
            i16 xPos = GET_X_LPARAM(l_param);
            i16 yPos = GET_Y_LPARAM(l_param);

            // Переходим к подсистеме ввода.
            InputSystem::ProcessMouseMove(xPos, yPos);
        } break;
        case WM_MOUSEWHEEL: {
            i32 zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (zDelta != 0) {
                // Свести входные данные в независимый от ОС (-1, 1)
                zDelta = (zDelta < 0) ? -1 : 1;
                InputSystem::ProcessMouseWheel(zDelta);
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
                InputSystem::ProcessButton(MouseButtons, pressed);
            }
        } break;
        case WM_INPUTLANGCHANGE:
        {
            HKL hkl = (HKL)l_param;
            // LANGID устарели. По возможности используйте имена локалей BCP 47.
            LANGID langId = LOWORD(HandleToUlong(hkl));

            WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
            LCIDToLocaleName(MAKELCID(langId, SORT_DEFAULT), localeName, LOCALE_NAME_MAX_LENGTH, 0);

            // Получите сокращенное название языка по стандарту ISO (например, «eng»).
            WCHAR lang[9];
            GetLocaleInfoEx(localeName, LOCALE_SISO639LANGNAME2, lang, 9);

            // Получите идентификатор раскладки клавиатуры (например, «00020409» для раскладки клавиатуры «США — международная»).
            WCHAR keyboardLayoutId[KL_NAMELENGTH];
            GetKeyboardLayoutNameW(keyboardLayoutId);
            // LoadKeyboardLayoutW(keyboardLayoutId, KLF_ACTIVATE);
        }
        break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif // MPLATFORM_WINDOWS