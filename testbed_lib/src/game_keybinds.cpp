#include "game.h"

#include <core/logger.hpp>
#include <core/input.h>
#include <core/console.hpp>
#include <renderer/camera.h>
#include <renderer/rendering_system.h>
#include "debug_console.h"

void GameOnEscapeCallback(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    MDEBUG("GameOnEscapeCallback");
    EventSystem::Fire(EventSystem::ApplicationQuit, nullptr, (EventContext){});
}

void GameOnYaw(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state); 

    f32 f = 0.F;
    if (key == Keys::LEFT || key == Keys::A) {
        f = 1.F;
    } else if (key == Keys::RIGHT || key == Keys::D) {
        f = -1.F;
    }
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->Yaw(f * DeltaTime);
}

void GameOnPitch(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);

    f32 f = 0.F;
    if (key == Keys::UP) {
        f = 1.F;
    } else if (key == Keys::DOWN) {
        f = -1.F;
    }
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->Pitch(f * DeltaTime);
}

void GameOnMoveForward(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveForward(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveBackward(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveBackward(state->BackwardMoveSpeed * DeltaTime);
}

void GameOnMoveLeft(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveLeft(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveRight(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveRight(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveUp(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveUp(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveDown(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveDown(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnConsoleChangeVisibility(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto state = reinterpret_cast<Game*>(UserData);

    bool ConsoleVisible = state->console.Visible();
    ConsoleVisible = !ConsoleVisible;

    state->console.SetVisible(ConsoleVisible);
    if (ConsoleVisible) {
        InputSystem::KeymapPush(state->ConsoleKeymap);
    } else {
        InputSystem::KeymapPop();
    }
}

void GameOnSetRenderModeDefault(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    EventContext data = {};
    data.data.i32[0] = Render::Default;
    EventSystem::Fire(EventSystem::SetRenderMode, /*(Game*)*/UserData, data);
}

void GameOnSetRenderModeLighting(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    EventContext data = {};
    data.data.i32[0] = Render::Lighting;
    EventSystem::Fire(EventSystem::SetRenderMode, /*(Game*)*/UserData, data);
}

void GameOnSetRenderModeNormals(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    EventContext data = {};
    data.data.i32[0] = Render::Normals;
    EventSystem::Fire(EventSystem::SetRenderMode, /*(Game*)*/UserData, data);
}

void GameOnSetGizmoMode(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;

    switch (key) {
        case Keys::KEY_1:
            state->gizmo.mode = Gizmo::Mode::NONE;
            break;
        case Keys::KEY_2:
            state->gizmo.mode = Gizmo::Mode::MOVE;
            break;
        case Keys::KEY_3:
            state->gizmo.mode = Gizmo::Mode::ROTATE;
            break;
        case Keys::KEY_4:
            state->gizmo.mode = Gizmo::Mode::SCALE;
            break;
        default:
            break;
    }
}

void GameOnLoadScene(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    EventSystem::Fire(EventSystem::DEBUG1, /*(Game*)*/UserData, (EventContext){});
}

void GameOnUnloadScene(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    EventSystem::Fire(EventSystem::DEBUG2, UserData, EventContext());
}

void GameOnConsoleScroll(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto console = reinterpret_cast<DebugConsole*>(UserData);
    if (key == Keys::PAGEUP) {
        console->MoveUp();
    } else if (key == Keys::PAGEDOWN) {
        console->MoveDown();
    }
}

void GameOnConsoleScrollHold(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;

    static f32 AccumulatedTime = 0.F;
    AccumulatedTime += DeltaTime;
    if (AccumulatedTime >= 0.1F) {
        if (key == Keys::PAGEUP) {
            state->console.MoveUp();
        } else if (key == Keys::PAGEDOWN) {
            state->console.MoveDown();
        }
        AccumulatedTime = 0.F;
    }
}

void GameOnConsoleChangeHistory(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData)
{
    auto console = reinterpret_cast<DebugConsole*>(UserData);
    if (key == Keys::UP) {
        console->HistoryBack();
    } else if (key == Keys::DOWN) {
        console->HistoryForward();
    }
}

void GameOnDebugTextureSwap(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    MDEBUG("Swapping texture!");
    EventContext context = {};
    EventSystem::Fire(EventSystem::DEBUG0, UserData, context);
}

void GameOnDebugCamPosition(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);

    MDEBUG(
        "Позиция:[%.2F, %.2F, %.2F",
        state->WorldCamera->GetPosition().x,
        state->WorldCamera->GetPosition().y,
        state->WorldCamera->GetPosition().z);
}

void GameOnDebugVsyncToggle(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    char cmd[30];
    MString::Copy(cmd, "mvar_set_int vsync 0", 29);
    bool VsyncEnabled = RenderingSystem::FlagEnabled(VsyncEnabledBit);
    u32 length = MString::Length(cmd);
    cmd[length - 1] = VsyncEnabled ? '1' : '0';
    Console::ExecuteCommand(cmd);
}

void GamePrintMemoryMetrics(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Application*>(UserData);
    auto state = reinterpret_cast<Game*>(GameInst->state);

    auto usage = MemorySystem::GetMemoryUsageStr();
    MINFO(usage.c_str());

    MDEBUG("Распределения: %llu (%llu в этом кадре)", state->AllocCount, state->AllocCount - state->PrevAllocCount);
}

void GameSetupKeymaps(Application* app) {
    // Глобальная раскладка клавиатуры
    Keymap GlobalKeymap;
    GlobalKeymap.BindingAdd(Keys::ESCAPE, Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnEscapeCallback);

    InputSystem::KeymapPush(GlobalKeymap);

    // Тестовая раскладка клавиатуры
    Keymap TestbedKeymap;
    TestbedKeymap.BindingAdd(Keys::A,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnYaw);
    TestbedKeymap.BindingAdd(Keys::LEFT,  Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnYaw);

    TestbedKeymap.BindingAdd(Keys::D,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnYaw);
    TestbedKeymap.BindingAdd(Keys::RIGHT, Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnYaw);

    TestbedKeymap.BindingAdd(Keys::UP,    Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnPitch);
    TestbedKeymap.BindingAdd(Keys::DOWN,  Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnPitch);

    TestbedKeymap.BindingAdd(Keys::GRAVE, Keymap::BindTypePress, Keymap::ModifierNoneBit, app->state, GameOnConsoleChangeVisibility);

    TestbedKeymap.BindingAdd(Keys::W,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnMoveForward);
    TestbedKeymap.BindingAdd(Keys::S,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnMoveBackward);
    TestbedKeymap.BindingAdd(Keys::Q,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnMoveLeft);
    TestbedKeymap.BindingAdd(Keys::E,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnMoveRight);
    TestbedKeymap.BindingAdd(Keys::SPACE, Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnMoveUp);
    TestbedKeymap.BindingAdd(Keys::X,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnMoveDown);

    TestbedKeymap.BindingAdd(Keys::KEY_0, Keymap::BindTypePress, Keymap::ModifierControlBit, app, GameOnSetRenderModeDefault);
    TestbedKeymap.BindingAdd(Keys::KEY_1, Keymap::BindTypePress, Keymap::ModifierControlBit, app, GameOnSetRenderModeLighting);
    TestbedKeymap.BindingAdd(Keys::KEY_2, Keymap::BindTypePress, Keymap::ModifierControlBit, app, GameOnSetRenderModeNormals);

    TestbedKeymap.BindingAdd(Keys::KEY_1, Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnSetGizmoMode);
    TestbedKeymap.BindingAdd(Keys::KEY_2, Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnSetGizmoMode);
    TestbedKeymap.BindingAdd(Keys::KEY_3, Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnSetGizmoMode);
    TestbedKeymap.BindingAdd(Keys::KEY_4, Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnSetGizmoMode);

    TestbedKeymap.BindingAdd(Keys::L,     Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnLoadScene);
    TestbedKeymap.BindingAdd(Keys::U,     Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnUnloadScene);

    TestbedKeymap.BindingAdd(Keys::T,     Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnDebugTextureSwap);
    TestbedKeymap.BindingAdd(Keys::P,     Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnDebugCamPosition);
    TestbedKeymap.BindingAdd(Keys::V,     Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GameOnDebugVsyncToggle);
    TestbedKeymap.BindingAdd(Keys::M,     Keymap::BindTypePress, Keymap::ModifierNoneBit, app, GamePrintMemoryMetrics);

    InputSystem::KeymapPush(TestbedKeymap);

    // Консольная раскладка клавиатуры. По умолчанию не активируется.
    auto state = reinterpret_cast<Game*>(app->state);

    state->ConsoleKeymap.OverridesAll = true;
    state->ConsoleKeymap.BindingAdd(Keys::GRAVE,    Keymap::BindTypePress, Keymap::ModifierNoneBit, state, GameOnConsoleChangeVisibility);
    state->ConsoleKeymap.BindingAdd(Keys::ESCAPE,   Keymap::BindTypePress, Keymap::ModifierNoneBit, state, GameOnConsoleChangeVisibility);

    state->ConsoleKeymap.BindingAdd(Keys::PAGEUP,   Keymap::BindTypePress, Keymap::ModifierNoneBit, &state->console, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(Keys::PAGEDOWN, Keymap::BindTypePress, Keymap::ModifierNoneBit, &state->console, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(Keys::PAGEUP,   Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnConsoleScrollHold);
    state->ConsoleKeymap.BindingAdd(Keys::PAGEDOWN, Keymap::BindTypeHold,  Keymap::ModifierNoneBit, app, GameOnConsoleScrollHold);

    state->ConsoleKeymap.BindingAdd(Keys::UP,       Keymap::BindTypePress, Keymap::ModifierNoneBit, &state->console, GameOnConsoleChangeHistory);
    state->ConsoleKeymap.BindingAdd(Keys::DOWN,     Keymap::BindTypePress, Keymap::ModifierNoneBit, &state->console, GameOnConsoleChangeHistory);

    // Если это было сделано при открытой консоли, нажмите ее раскладку.
    bool ConsoleVisible = state->console.Visible();
    if (ConsoleVisible) {
        InputSystem::KeymapPush(state->ConsoleKeymap);
    }
}

void GameRemoveKeymaps(Application *app)
{
    // Извлечь все раскладки клавиатуры
    while (InputSystem::KeymapPop()) {
    }

    auto state = reinterpret_cast<Game*>(app->state);

    // Удаляем все привязки для раскладки клавиатуры консоли, так как это единственное, что у нас есть.
    state->ConsoleKeymap.Clear();
}
