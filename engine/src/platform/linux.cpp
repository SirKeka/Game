#include "platform.hpp"

// Уровень платформы Linux.
#if MPLATFORM_LINUX

#include "core/logger.hpp"
#include "core/event.hpp"
#include "core/input.hpp"
#include "core/mthread.hpp"
#include "core/mmutex.hpp"
#include "core/mmemory.hpp"

#include "containers/darray.hpp"

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>  // sudo apt-get install libx11-dev
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>  // sudo apt-get install libxkbcommon-x11-dev
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // наносон
#else
#include <unistd.h>  // usleep
#endif
#include <pthread.h>
#include <errno.h>        // Для сообщения об ошибках
#include <sys/sysinfo.h>  // Информация о процессоре

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Для создания поверхности
struct linux_handle_info 
{
    xcb_connection_t* connection;
    xcb_window_t window;
};


struct PlatformState {
    Display* display;
    linux_handle_info handle;
    xcb_screen_t* screen;
    xcb_atom_t wmProtocols;
    xcb_atom_t wmDeleteWin;

    // constexpr PlatformState() : display(nullptr), connection(nullptr), window(), screen(nullptr), wmProtocols(), wmDeleteWin(), surface() {}
};

static PlatformState* pState = nullptr;

// Перевод ключа
Keys TranslateKeycode(u32 x_keycode);

bool WindowSystem::Initialize(u64& MemoryRequirement, void* memory, void* config) {
    MemoryRequirement = sizeof(PlatformState);

    if (pState) {
        return true;
    }

    auto pConfig = reinterpret_cast<ApplicationConfig*>(config);

    pState = new(memory) PlatformState();

    // Подключиться к X
    pState->display = XOpenDisplay(NULL);

    // Отключите повтор клавиш.
    XAutoRepeatOff(pState->display);

    // Retrieve the connection from the display. Извлеките соединение с дисплея.
    pState->handle.connection = XGetXCBConnection(pState->display);

    if (xcb_connection_has_error(pState->handle.connection)) {
        MFATAL("Не удалось подключиться к X-серверу через XCB.");
        return false;
    }

    // Получить данные с X-сервера
    const struct xcb_setup_t* setup = xcb_get_setup(pState->connection);

    // Перебор экранов с помощью итератора
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int screen_p = 0;
    for (i32 s = screen_p; s > 0; s--) {
        xcb_screen_next(&it);
    }

    // После того, как экраны будут перебраны, назначьте его.
    pState->screen = it.data;

    // Выделите XID для создаваемого окна.
    pState->handle.window = xcb_generate_id(pState->connection);

    // Регистрация типов событий.
    // XCB_CW_BACK_PIXEL = заполнение фона окна одним цветом
    // Требуется XCB_CW_EVENT_MASK.
    u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    // Слушайте кнопки клавиатуры и мыши
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                       XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // Значения для отправки через XCB (цвет фона, события)
    u32 value_list[] = {pState->screen->black_pixel, event_values};

    // Создайте окно
    xcb_create_window(
        pState->handle.connection,
        XCB_COPY_FROM_PARENT,           // глубина
        pState->handle.window,
        pState->screen->root,           // родитель
        pConfig->StartPosX,             // x
        pConfig->StartPosY,             // y
        pConfig->StartWidth,            // ширина
        pConfig->StartHeight,           // высота
        0,                              // Без рамки
        XCB_WINDOW_CLASS_INPUT_OUTPUT,  // класс
        pState->screen->root_visual,
        event_mask,
        value_list);

    // Изменить заголовок
    xcb_change_property(
        pState->handle.connection,
        XCB_PROP_MODE_REPLACE,
        pState->handle.window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,  // данные следует просматривать по 8 бит за раз
        strlen(pConfig->name),
        pConfig->name);

    // Скажите серверу, чтобы он уведомлял, 
    // когда оконный менеджер пытается уничтожить окно.
    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(
        pState->handle.connection,
        0,
        strlen("WM_DELETE_WINDOW"),
        "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
        pState->handle.connection,
        0,
        strlen("WM_PROTOCOLS"),
        "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(
        pState->handle.connection,
        wm_delete_cookie,
        NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(
        pState->handle.connection,
        wm_protocols_cookie,
        NULL);
    pState->wmDeleteWin = wm_delete_reply->atom;
    pState->wmProtocols = wm_protocols_reply->atom;

    xcb_change_property(
        pState->handle.connection,
        XCB_PROP_MODE_REPLACE,
        pState->handle.window,
        wm_protocols_reply->atom,
        4,
        32,
        1,
        &wm_delete_reply->atom);

    // Сопоставьте окно с экраном
    xcb_map_window(pState->handle.connection, pState->handle.window);

    // Очистка потока
    i32 stream_result = xcb_flush(pState->handle.connection);
    if (stream_result <= 0) {
        MFATAL("Произошла ошибка при очистке потока: %d", stream_result);
        return false;
    }

    return true;
}

void WindowSystem::Shutdown() {
    if (pState) {

        // Повторное включение клавиши, поскольку это глобально для ОС... просто... вау.
        XAutoRepeatOn(pState->display);

        xcb_destroy_window(pState->handle.connection, pState->handle.window);
    }
}

bool Messages() {
    if (pState) {

        xcb_generic_event_t* event;
        xcb_client_message_event_t* cm;

        bool quit_flagged = false;

        // Опрос событий до тех пор, пока не будет возвращено значение null.
        while (event != 0) {
            event = xcb_poll_for_event(pState->connection);
            if (event == 0) {
                break;
            }
            //while ((event = xcb_poll_for_event(pState->connection))) вместо конструкции выше

             // Входные события
            switch (event->response_type & ~0x80) {
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE: {
                    // Событие нажатия клавиши - xcb_key_press_event_t и xcb_key_release_event_t совпадают
                    xcb_key_press_event_t *kb_event = (xcb_key_press_event_t *)event;
                    bool pressed = event->response_type == XCB_KEY_PRESS;
                    xcb_keycode_t code = kb_event->detail;
                    KeySym key_sym = XkbKeycodeToKeysym(
                        pState->display,
                        (KeyCode)code,  //event.xkey.keycode,
                        0,
                        0/*code & ShiftMask ? 1 : 0*/);

                    Keys key = translate_keycode(key_sym);

                    // Передайте в подсистему ввода для обработки.
                    InputSystem::InputProcessKey(key, pressed);
                } break;
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE: {
                    xcb_button_press_event_t *mouse_event = (xcb_button_press_event_t *)event;
                    bool pressed = event->response_type == XCB_BUTTON_PRESS;
                    buttons mouse_button = BUTTON_MAX_BUTTONS;
                    switch (mouse_event->detail) {
                        case XCB_BUTTON_INDEX_1:
                            mouse_button = BUTTON_LEFT;
                            break;
                        case XCB_BUTTON_INDEX_2:
                            mouse_button = BUTTON_MIDDLE;
                            break;
                        case XCB_BUTTON_INDEX_3:
                            mouse_button = BUTTON_RIGHT;
                            break;
                    }

                    // Переходим к подсистеме ввода.
                    if (mouse_button != BUTTON_MAX_BUTTONS) {
                        InputSystem::ProcessButton(mouse_button, pressed);
                    }
                }
                case XCB_MOTION_NOTIFY:
                    // Движение мыши
                     xcb_motion_notify_event_t *move_event = (xcb_motion_notify_event_t *)event;

                    // Переходим к подсистеме ввода.
                    InputSystem::InputProcessMouseMove(move_event->event_x, move_event->event_y);
                    break;

                case XCB_CONFIGURE_NOTIFY: {
                    // Изменение размера — обратите внимание, что это также запускается при перемещении окна, но в любом случае
                    // должно быть передано, поскольку изменение x/y может означать изменение размера в верхнем левом углу.
                    // Уровень приложения может решить, что с этим делать.
                    xcb_configure_notify_event_t *configure_event = (xcb_configure_notify_event_t *)event;

                    // Запустите событие. Уровень приложения должен уловить это, но не обрабатывать, 
                    // поскольку это должно быть видно другим частям приложения.
                    EventContext context;
                    context.data.u16[0] = configure_event->width;
                    context.data.u16[1] = configure_event->height;
                    Event::Fire(EVENT_CODE_RESIZED, nullptr, context);
                }

                case XCB_CLIENT_MESSAGE: {
                    cm = (xcb_client_message_event_t*)event;

                    // Закрытие окна
                    if (cm->data.data32[0] == pState->wmDeleteWin) {
                        quit_flagged = true;
                    }
                } break;
                default:
                    // Что-то еще
                    break;
            }

            free(event);
        }
    }
    return !quit_flagged;
}

void* PlatformAllocate(u64 size, b8 aligned) {
    return malloc(size);
}
void PlatformFree(void* block, b8 aligned) {
    free(block);
}
void* PlatformZeroMemory(void* block, u64 size) {
    return memset(block, 0, size);
}
void* PlatformCopyMemory(void* dest, const void* source, u64 size) {
    return memcpy(dest, source, size);
}
void* PlatformSetMemory(void* dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

void PlatformConsoleWrite(const char* message, u8 colour) {
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char* colour_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}
void PlatformConsoleWriteError(const char* message, u8 colour) {
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char* colour_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

f64 PlatformGetAbsoluteTime() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void PlatformSleep(u64 ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

i32 PlatformGetProcessorCount()
{
    // Загрузить информацию о процессоре.
    i32 ProcessorCount = get_nprocs_conf();
    i32 ProcessorsAvailable = get_nprocs();
    MINFO("Обнаружено %i ядер процессора, доступно %i ядер.", ProcessorCount, ProcessorsAvailable);
    return ProcessorsAvailable;
}

void PlatformGetHandleInfo(u64 &OutSize, void *memory)
{
    OutSize = sizeof(linux_handle_info);

    if (!memory) {
        return;
    }

    MemorySystem::CopyMem(memory, &pState->handle, OutSize);
}

// ПРИМЕЧАНИЕ: Начало потоков.

constexpr MThread::MThread(PFN_ThreadStart StartFunctionPtr, void *params, bool AutoDetach)
{
    if (!StartFunctionPtr) {
        return;
    }

    // MThread::MThread использует указатель на функцию, которая возвращает void*, поэтому выполняется холодное приведение к этому типу.
    i32 result = pthread_create((pthread_t*)&out_thread->thread_id, 0, (void* (*)(void*))StartFunctionPtr, params);
    if (result != 0) {
        switch (result) {
            case EAGAIN:
                MERROR("Не удалось создать поток: недостаточно ресурсов для создания другого потока.");
                return;
            case EINVAL:
                MERROR("Не удалось создать поток: в атрибутах переданы недопустимые настройки.");
                return;
            default:
                MERROR("Не удалось создать поток: произошла необработанная ошибка. errno=%i", result);
                return;
        }
    }
    MDEBUG("Запуск процесса на идентификаторе потока: %#x", ThreadID);

    // Сохраняйте только вне дескриптора, если не выполняется автоматическое отсоединение.
    if (!AutoDetach) {
        data = PlatformAllocate(sizeof(u64),);
        *(u64*)data = ThreadID;
    } else {
        // Если выполняется немедленное отсоединение, убедитесь, что операция прошла успешно.
        result = pthread_detach(ThreadID);
        if (result != 0) {
            switch (result) {
                case EINVAL:
                    MERROR("Не удалось отсоединить недавно созданный поток: поток не является присоединяемым потоком.");
                    return;
                case ESRCH:
                    MERROR("Не удалось отсоединить недавно созданный поток: поток с идентификатором %#x не найден.", ThreadID);
                    return;
                default:
                    MERROR("Не удалось отсоединить недавно созданный поток: произошла неизвестная ошибка. errno=%i", result);
                    return;
            }
        }
    }

    return true;
}

MThread::~MThread()
{
    this->Cancel();
}

void MThread::Detach()
{
    if (data) {
        i32 result = pthread_detach(*(pthread_t*)data);
        if (result != 0) {
            switch (result) {
                case EINVAL:
                    MERROR("Не удалось отсоединить поток: поток не является присоединяемым потоком.");
                    break;
                case ESRCH:
                    MERROR("Не удалось отсоединить поток: поток с идентификатором %#x не найден.", ThreadID);
                    break;
                default:
                    MERROR("Не удалось отсоединить поток: произошла неизвестная ошибка. errno=%i", result);
                    break;
            }
        }
        PlatformFree(data, false);
        data = nullptr;
    }
}

void MThread::Cancel()
{
    if (data) {
        i32 result = pthread_cancel(*(pthread_t*)data);
        if (result != 0) {
            switch (result) {
                case ESRCH:
                    MERROR("Не удалось отменить поток: поток с идентификатором %#x не найден.", ThreadID);
                    break;
                default:
                    MERROR("Не удалось отменить поток: произошла неизвестная ошибка. errno=%i", result);
                    break;
            }
        }
        PlatformFree(data, false);
        data = 0;
        ThreadID = 0;
    }
}

bool MThread::IsActive()
{
    // ЗАДАЧА: Найдите лучший способ проверить это.
    return data != 0;
}

void MThread::Sleep(u64 ms)
{
    PlatformSleep(ms);
}

u64 MThread::GetThreadID()
{
    return (u64)pthread_self();
}

// Конец потоков
// Начало мьютекс

constexpr MMutex::MMutex()
{
    // Инициализация
    pthread_mutex_t mutex;
    i32 result = pthread_mutex_init(&mutex, 0);
    if (result != 0) {
        MERROR("Ошибка создания мьютекса!");
        return;
    }
    
    // Сохраните дескриптор мьютекса.
    data = PlatformAllocate(sizeof(pthread_mutex_t), false);
    *(pthread_mutex_t*)data = mutex;

    return true;
}

MMutex::~MMutex()
{
    if (data) {
        i32 result = pthread_mutex_destroy((pthread_mutex_t*)data);
        switch (result) {
            case 0:
                // MTRACE("Mutex destroyed.");
                break;
            case EBUSY:
                MERROR("Невозможно уничтожить мьютекс: мьютекс заблокирован или на него есть ссылка.");
                break;
            case EINVAL:
                MERROR("Невозможно уничтожить мьютекс: значение, указанное мьютексом, недопустимо.");
                break;
            default:
                MERROR("При уничтожении мьютекса произошла обработанная ошибка: errno=%i", result);
                break;
        }

        PlatformFree(data, false);
        data = nullptr;
    }
}

bool MMutex::Lock()
{
    // Lock
    i32 result = pthread_mutex_lock((pthread_mutex_t*)data);
    switch (result) {
        case 0:
            // Успех, все остальное — неудача.
            // MTRACE("Получена блокировка мьютекса.");
            return true;
        case EOWNERDEAD:
            MERROR("Владеющий поток завершен, пока мьютекс все еще активен.");
            return false;
        case EAGAIN:
            MERROR("Невозможно получить блокировку мьютекса: достигнуто максимальное количество рекурсивных блокировок мьютекса.");
            return false;
        case EBUSY:
            MERROR("Невозможно получить блокировку мьютекса: блокировка мьютекса уже существует.");
            return false;
        case EDEADLK:
            MERROR("Невозможно получить блокировку мьютекса: обнаружена взаимоблокировка мьютекса.");
            return false;
        default:
            MERROR("Произошла обработанная ошибка при получении блокировки мьютекса: errno=%i", result);
            return false;
    }
}

bool MMutex::Unlock()
{
    if (data) {
        i32 result = pthread_mutex_unlock((pthread_mutex_t*)data);
        switch (result) {
            case 0:
                // MTRACE("Освобождена блокировка мьютекса.");
                return true;
            case EOWNERDEAD:
                MERROR("Невозможно разблокировать мьютекс: поток-владелец завершен, пока мьютекс все еще активен.");
                return false;
            case EPERM:
                MERROR("Невозможно разблокировать мьютекс: мьютекс не принадлежит текущему потоку.");
                return false;
            default:
                MERROR("Произошла обработанная ошибка при разблокировке блокировки мьютекса: errno=%i", result);
                return false;
        }
    }

    return false;
}

// ПРИМЕЧАНИЕ: Конец мьютекса

// Key translation
Keys translate_keycode(u32 x_keycode) {
    switch (x_keycode) {
        case XK_BackSpace:
            return KEY_BACKSPACE;
        case XK_Return:
            return KEY_ENTER;
        case XK_Tab:
            return KEY_TAB;
            //case XK_Shift: return KEY_SHIFT;
            //case XK_Control: return KEY_CONTROL;

        case XK_Pause:
            return KEY_PAUSE;
        case XK_Caps_Lock:
            return KEY_CAPITAL;

        case XK_Escape:
            return KEY_ESCAPE;

            // Not supported
            // case : return KEY_CONVERT;
            // case : return KEY_NONCONVERT;
            // case : return KEY_ACCEPT;

        case XK_Mode_switch:
            return KEY_MODECHANGE;

        case XK_space:
            return KEY_SPACE;
        case XK_Prior:
            return KEY_PRIOR;
        case XK_Next:
            return KEY_NEXT;
        case XK_End:
            return KEY_END;
        case XK_Home:
            return KEY_HOME;
        case XK_Left:
            return KEY_LEFT;
        case XK_Up:
            return KEY_UP;
        case XK_Right:
            return KEY_RIGHT;
        case XK_Down:
            return KEY_DOWN;
        case XK_Select:
            return KEY_SELECT;
        case XK_Print:
            return KEY_PRINT;
        case XK_Execute:
            return KEY_EXECUTE;
        // case XK_snapshot: return KEY_SNAPSHOT; // not supported
        case XK_Insert:
            return KEY_INSERT;
        case XK_Delete:
            return KEY_DELETE;
        case XK_Help:
            return KEY_HELP;
        /*XK_Meta_L и XK_Meta_R следует изменить на
         XK_Super_L и XK_Super_R из-за того, 
         что мета-ключи не всегда являются 
         ключами Windows (из-за разных оконных менеджеров)*/
        case XK_Meta_L:
        return KEY_LWIN;  // ЗАДАЧА: не уверен, что это правильно
    case XK_Meta_R:
            return KEY_RWIN;
            // case XK_apps: return KEY_APPS; // not supported

            // case XK_sleep: return KEY_SLEEP; //not supported

        case XK_KP_0:
            return KEY_NUMPAD0;
        case XK_KP_1:
            return KEY_NUMPAD1;
        case XK_KP_2:
            return KEY_NUMPAD2;
        case XK_KP_3:
            return KEY_NUMPAD3;
        case XK_KP_4:
            return KEY_NUMPAD4;
        case XK_KP_5:
            return KEY_NUMPAD5;
        case XK_KP_6:
            return KEY_NUMPAD6;
        case XK_KP_7:
            return KEY_NUMPAD7;
        case XK_KP_8:
            return KEY_NUMPAD8;
        case XK_KP_9:
            return KEY_NUMPAD9;
        case XK_multiply:
            return KEY_MULTIPLY;
        case XK_KP_Add:
            return KEY_ADD;
        case XK_KP_Separator:
            return KEY_SEPARATOR;
        case XK_KP_Subtract:
            return KEY_SUBTRACT;
        case XK_KP_Decimal:
            return KEY_DECIMAL;
        case XK_KP_Divide:
            return KEY_DIVIDE;
        case XK_F1:
            return KEY_F1;
        case XK_F2:
            return KEY_F2;
        case XK_F3:
            return KEY_F3;
        case XK_F4:
            return KEY_F4;
        case XK_F5:
            return KEY_F5;
        case XK_F6:
            return KEY_F6;
        case XK_F7:
            return KEY_F7;
        case XK_F8:
            return KEY_F8;
        case XK_F9:
            return KEY_F9;
        case XK_F10:
            return KEY_F10;
        case XK_F11:
            return KEY_F11;
        case XK_F12:
            return KEY_F12;
        case XK_F13:
            return KEY_F13;
        case XK_F14:
            return KEY_F14;
        case XK_F15:
            return KEY_F15;
        case XK_F16:
            return KEY_F16;
        case XK_F17:
            return KEY_F17;
        case XK_F18:
            return KEY_F18;
        case XK_F19:
            return KEY_F19;
        case XK_F20:
            return KEY_F20;
        case XK_F21:
            return KEY_F21;
        case XK_F22:
            return KEY_F22;
        case XK_F23:
            return KEY_F23;
        case XK_F24:
            return KEY_F24;

        case XK_Num_Lock:
            return KEY_NUMLOCK;
        case XK_Scroll_Lock:
            return KEY_SCROLL;

        case XK_KP_Equal:
            return KEY_NUMPAD_EQUAL;

        case XK_Shift_L:
            return KEY_LSHIFT;
        case XK_Shift_R:
            return KEY_RSHIFT;
        case XK_Control_L:
            return KEY_LCONTROL;
        case XK_Control_R:
            return KEY_RCONTROL;
        case XK_Alt_L:
            return KEY_LALT;
        case XK_Alt_R:
            return KEY_RALT;

        case XK_semicolon:
            return KEY_SEMICOLON;
        case XK_plus:
            return KEY_PLUS;
        case XK_comma:
            return KEY_COMMA;
        case XK_minus:
            return KEY_MINUS;
        case XK_period:
            return KEY_PERIOD;
        case XK_slash:
            return KEY_SLASH;
        case XK_grave:
            return KEY_GRAVE;

        case XK_0:
            return KEY_0;
        case XK_1:
            return KEY_1;
        case XK_2:
            return KEY_2;
        case XK_3:
            return KEY_3;
        case XK_4:
            return KEY_4;
        case XK_5:
            return KEY_5;
        case XK_6:
            return KEY_6;
        case XK_7:
            return KEY_7;
        case XK_8:
            return KEY_8;
        case XK_9:
            return KEY_9;

        case XK_a:
        case XK_A:
            return KEY_A;
        case XK_b:
        case XK_B:
            return KEY_B;
        case XK_c:
        case XK_C:
            return KEY_C;
        case XK_d:
        case XK_D:
            return KEY_D;
        case XK_e:
        case XK_E:
            return KEY_E;
        case XK_f:
        case XK_F:
            return KEY_F;
        case XK_g:
        case XK_G:
            return KEY_G;
        case XK_h:
        case XK_H:
            return KEY_H;
        case XK_i:
        case XK_I:
            return KEY_I;
        case XK_j:
        case XK_J:
            return KEY_J;
        case XK_k:
        case XK_K:
            return KEY_K;
        case XK_l:
        case XK_L:
            return KEY_L;
        case XK_m:
        case XK_M:
            return KEY_M;
        case XK_n:
        case XK_N:
            return KEY_N;
        case XK_o:
        case XK_O:
            return KEY_O;
        case XK_p:
        case XK_P:
            return KEY_P;
        case XK_q:
        case XK_Q:
            return KEY_Q;
        case XK_r:
        case XK_R:
            return KEY_R;
        case XK_s:
        case XK_S:
            return KEY_S;
        case XK_t:
        case XK_T:
            return KEY_T;
        case XK_u:
        case XK_U:
            return KEY_U;
        case XK_v:
        case XK_V:
            return KEY_V;
        case XK_w:
        case XK_W:
            return KEY_W;
        case XK_x:
        case XK_X:
            return KEY_X;
        case XK_y:
        case XK_Y:
            return KEY_Y;
        case XK_z:
        case XK_Z:
            return KEY_Z;

        default:
            return 0;
    }
}
#endif