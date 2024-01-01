//#include "platform.hpp"

// Уровень платформы Linux.
#if MPLATFORM_LINUX

#include "core/logger.h"

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct InternalState {
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wmProtocols;
    xcb_atom_t wmDeleteWin;
} InternalState;

b8 PlatformStartup(
    PlatformState* PlatState,
    const char* ApplicationName,
    i32 x,
    i32 y,
    i32 width,
    i32 height) {
    // Создание внутреннего состояния.
    PlatState->InternalState = malloc(sizeof(InternalState));
    InternalState* state = (InternalState*)PlatState->InternalState;

    // Подключиться к X
    state->display = XOpenDisplay(NULL);

    // Отключите повтор клавиш.
    XAutoRepeatOff(state->display);

    // Retrieve the connection from the display. Извлеките соединение с дисплея.
    state->connection = XGetXCBConnection(state->display);

    if (xcb_connection_has_error(state->connection)) {
        MFATAL("Не удалось подключиться к X-серверу через XCB.");
        return FALSE;
    }

    // Получить данные с X-сервера
    const struct xcb_setup_t* setup = xcb_get_setup(state->connection);

    // Перебор экранов с помощью итератора
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int screen_p = 0;
    for (i32 s = screen_p; s > 0; s--) {
        xcb_screen_next(&it);
    }

    // После того, как экраны будут перебраны, назначьте его.
    state->screen = it.data;

    // Выделите XID для создаваемого окна.
    state->window = xcb_generate_id(state->connection);

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
    u32 value_list[] = {state->screen->black_pixel, event_values};

    // Создайте окно
    xcb_void_cookie_t cookie = xcb_create_window(
        state->connection,
        XCB_COPY_FROM_PARENT,           // глубина
        state->window,
        state->screen->root,            // родитель
        x,                              // x
        y,                              // y
        width,                          // ширина
        height,                         // высота
        0,                              // Без рамки
        XCB_WINDOW_CLASS_INPUT_OUTPUT,  // класс
        state->screen->root_visual,
        event_mask,
        value_list);

    // Изменить заголовок
    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        state->window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,  // данные следует просматривать по 8 бит за раз
        strlen(ApplicationName),
        ApplicationName);

    // Скажите серверу, чтобы он уведомлял, 
    // когда оконный менеджер пытается уничтожить окно.
    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(
        state->connection,
        0,
        strlen("WM_DELETE_WINDOW"),
        "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
        state->connection,
        0,
        strlen("WM_PROTOCOLS"),
        "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(
        state->connection,
        wm_delete_cookie,
        NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(
        state->connection,
        wm_protocols_cookie,
        NULL);
    state->wmDeleteWin = wm_delete_reply->atom;
    state->wmProtocols = wm_protocols_reply->atom;

    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        state->window,
        wm_protocols_reply->atom,
        4,
        32,
        1,
        &wm_delete_reply->atom);

    // Сопоставьте окно с экраном
    xcb_map_window(state->connection, state->window);

    // Очистка потока
    i32 stream_result = xcb_flush(state->connection);
    if (stream_result <= 0) {
        MFATAL("Произошла ошибка при очистке потока: %d", stream_result);
        return FALSE;
    }

    return TRUE;
}

void PlatformShutdown(PlatformState* PlatState) {
    // Simply cold-cast to the known type.
    InternalState* state = (InternalState*)PlatState->InternalState;

    // Повторное включение клавиши, поскольку это глобально для ОС... просто... вау.
    XAutoRepeatOn(state->display);

    xcb_destroy_window(state->connection, state->window);
}

b8 PlatformPumpMessages(PlatformState* PlatState) {
    // Простое легкое приведение к известному типу.
    InternalState* state = (InternalState*)PlatState->InternalState;

    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    b8 quit_flagged = FALSE;

    // Опрос событий до тех пор, пока не будет возвращено значение null.
    while (event != 0) {
        event = xcb_poll_for_event(state->connection);
        if (event == 0) {
            break;
        }
        //while ((event = xcb_poll_for_event(state->connection))) вместо конструкции выше

         // Входные события
        switch (event->response_type & ~0x80) {
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE: {
                // TODO: Нажатия и отпускания клавиш
            } break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE: {
                // TODO: Кнопка мыши нажимается и отпускается
            }
            case XCB_MOTION_NOTIFY:
                // TODO: движение мыши
                break;

            case XCB_CONFIGURE_NOTIFY: {
                // TODO: Изменение размера
            }

            case XCB_CLIENT_MESSAGE: {
                cm = (xcb_client_message_event_t*)event;

                // Закрытие окна
                if (cm->data.data32[0] == state->wmDeleteWin) {
                    quit_flagged = TRUE;
                }
            } break;
            default:
                // Что-то еще
                break;
        }

        free(event);
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
    clock_gettime(CLOCK_MONOTONIC, &now);
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

#endif