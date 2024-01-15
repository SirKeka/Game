#include "application.hpp"
#include "game_types.hpp"

#include "logger.hpp"

#include "platform/platform.hpp"
#include "core/mmemory.hpp"
#include "core/event.hpp"
#include "input.hpp"
#include <new>

/*Application::Application()
{
}

Application::~Application()
{
}*/


struct ApplicationState {
    //MMemory* mem;
    Input* Inputs;
    Event* Events;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    Game* GameInst;
    
    i16 width;
    i16 height;
    f64 LastTime;
};

// static Application App;

static bool initialized = FALSE;
// TODO: Возможно убрать статик для запуска двух окон на разных экранах или придумать другую реализацию
static ApplicationState AppState;

// Обработчики событий
bool ApplicationOnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
bool ApplicationOnKey(u16 code, void* sender, void* ListenerInst, EventContext context);

bool ApplicationCreate(Game* GameInst) {
    if (initialized) {
        MERROR("ApplicationCreate вызывался более одного раза.");
        return FALSE;
    }

    AppState.GameInst = GameInst;

    // Инициализируйте подсистемы.
    InitializeLogging();
    AppState.Inputs = new Input();

    // TODO: Удали это
    MFATAL("Тестовое сообщение: %f", 3.14f);
    MERROR("Тестовое сообщение: %f", 3.14f);
    MWARN("Тестовое сообщение: %f", 3.14f);
    MINFO("Тестовое сообщение: %f", 3.14f);
    MDEBUG("Тестовое сообщение: %f", 3.14f);
    MTRACE("Тестовое сообщение: %f", 3.14f);

    AppState.IsRunning = true;
    AppState.IsSuspended = false;

    AppState.Events = new Event();
    if (!AppState.Events->Initialize()) {
        MERROR("Система событий не смогла инициализироваться. Приложение не может быть продолжено.");
        return false;
    }

    Event::Register(EVENT_CODE_APPLICATION_QUIT, 0, ApplicationOnEvent);
    Event::Register(EVENT_CODE_KEY_PRESSED, 0, ApplicationOnKey);
    Event::Register(EVENT_CODE_KEY_RELEASED, 0, ApplicationOnKey);
    
    AppState.Window = new MWindow(GameInst->AppConfig.name,
                                  GameInst->AppConfig.StartPosX, 
                                  GameInst->AppConfig.StartPosY, 
                                  GameInst->AppConfig.StartHeight, 
                                  GameInst->AppConfig.StartWidth);

    if (!AppState.Window)
    {
        return false;
    }
    else AppState.Window->Create();

    // Инициализируйте игру.
    if (!AppState.GameInst->Initialize()) {
        MFATAL("Не удалось инициализировать игру.");
        return false;
    }

    AppState.GameInst->OnResize(AppState.width, AppState.height);

    initialized = true;

    return true;
}

bool ApplicationRun() {
    
    MINFO(MMemory::GetMemoryUsageStr());

    while (AppState.IsRunning) {
        if(!AppState.Window->Messages()) {
            AppState.IsRunning = false;
        }

        if(!AppState.IsSuspended) {
            if (!AppState.GameInst->Update(0.0f)) {
                MFATAL("Ошибка обновления игры, выключение.");
                AppState.IsRunning = false;
                break;
            }

            // Вызовите процедуру рендеринга игры.
            if (!AppState.GameInst->Render(0.0f)) {
                MFATAL("Ошибка рендеринга игры, выключение.");
                AppState.IsRunning = false;
                break;
            }
        }

        // ПРИМЕЧАНИЕ. Обновление/копирование состояния ввода всегда
        // должно выполняться после записи любого ввода; т.е. перед этой
        // строкой. В целях безопасности входные данные обновляются в
        // последнюю очередь перед завершением этого кадра.
        AppState.Inputs->InputUpdate(0);
    }

    AppState.IsRunning = false;

    // Отключение системы событий.
    Event::Unregister(EVENT_CODE_APPLICATION_QUIT, 0, ApplicationOnEvent);
    Event::Unregister(EVENT_CODE_KEY_PRESSED, 0, ApplicationOnKey);
    Event::Unregister(EVENT_CODE_KEY_RELEASED, 0, ApplicationOnKey);
    AppState.Events->Shutdown();
    AppState.Inputs->~Input(); // ShutDown

    AppState.Window->Close();

    return true;
}

bool ApplicationOnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            MINFO("Получено событие EVENT_CODE_APPLICATION_QUIT, завершение работы.\n");
            AppState.IsRunning = false;
            return true;
        }
    }

    return false;
}

bool ApplicationOnKey(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 KeyCode = context.data.u16[0];
        if (KeyCode == KEY_ESCAPE) {
            // ПРИМЕЧАНИЕ. Технически событие генерируется само по себе, но могут быть и другие прослушиватели.
            EventContext data = {};
            Event::Fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Заблокируйте что-либо еще от обработки этого.
            return true;
        } else if (KeyCode == KEY_A) {
            // Пример проверки ключа
            MDEBUG("Явно — клавиша A нажата!");
        } else {
            MDEBUG("'%c' нажатие клавиши в окне.", KeyCode);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 KeyCode = context.data.u16[0];
        if (KeyCode == KEY_B) {
            // Пример проверки ключа
            MDEBUG("Явное — клавиша B отущена!");
        } else {
            MDEBUG("'%c' клавиша отпущен в окне.", KeyCode);
        }
    }

    return false;
}
