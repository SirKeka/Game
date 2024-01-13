#include "application.hpp"
#include "game_types.hpp"

#include "logger.hpp"

#include "platform/platform.hpp"
#include "core/mmemory.hpp"
#include "core/event.hpp"
#include "input.hpp"
#include <new>

struct ApplicationState {
    // MMemory* mem;
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

static bool initialized = FALSE;
// TODO: Возможно убрать статик для запуска двух окон на разных экранах или придумать другую реализацию
static ApplicationState AppState;

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
        return FALSE;
    }

    AppState.GameInst->OnResize(AppState.width, AppState.height);

    initialized = true;

    return true;
}

bool ApplicationRun() {
    
    MINFO(AppState.GameInst->mem.GetMemoryUsageStr());

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

    AppState.Events->Shutdown();
    AppState.Inputs->~Input(); // ShutDown

    AppState.Window->Close();

    return true;
}
