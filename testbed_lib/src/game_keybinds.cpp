#include "game.h"

#include <core/event.h>
#include <core/input.h>
#include <core/console.h>
#include <renderer/camera.h>
#include <renderer/rendering_system.h>
#include "debug_console.h"

void GameOnEscapeCallback(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    MDEBUG("GameOnEscapeCallback");
    EventSystem::Fire(EventSystem::ApplicationQuit, nullptr, EventContext());
}

void GameOnYaw(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state; 

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
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;

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
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveForward(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveBackward(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveBackward(state->BackwardMoveSpeed * DeltaTime);
}

void GameOnMoveLeft(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveLeft(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveRight(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveRight(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveUp(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveUp(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnMoveDown(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
    auto DeltaTime = GameInst->engine->GetFrameData().DeltaTime;
    state->WorldCamera->MoveDown(state->ForwardMoveSpeed * DeltaTime);
}

void GameOnConsoleChangeVisibility(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto state = (Game*)UserData;

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
    EventContext data {};
    data.data.i32[0] = Render::Default;
    EventSystem::Fire(EventSystem::SetRenderMode, UserData, data);
}

void GameOnSetRenderModeLighting(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    EventContext data {};
    data.data.i32[0] = Render::Lighting;
    EventSystem::Fire(EventSystem::SetRenderMode, UserData, data);
}

void GameOnSetRenderModeNormals(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    EventContext data {};
    data.data.i32[0] = Render::Normals;
    EventSystem::Fire(EventSystem::SetRenderMode, UserData, data);
}

void GameOnSetRenderModeCascades(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data {};
    data.data.i32[0] = Render::Cascades;
    EventSystem::Fire(EventSystem::SetRenderMode, UserData, data);
}

void GameOnSetRenderModeWireframe(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data {};
    data.data.i32[0] = Render::Wireframe;
    EventSystem::Fire(EventSystem::SetRenderMode, UserData, data);
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

void GameOnGizmoOrientationSet(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
    
    i32 orientation = (i32)state->gizmo.GetOrientation();
    orientation++;
    if (orientation > Gizmo::Orientation::Max) {
        orientation = Gizmo::Orientation::Global;
    }
    state->gizmo.SetOrientation((Gizmo::Orientation)orientation);
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
    auto console = (DebugConsole*)UserData;
    if (key == Keys::PAGEUP) {
        console->MoveUp();
    } else if (key == Keys::PAGEDOWN) {
        console->MoveDown();
    }
}

void GameOnConsoleScrollHold(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) 
{
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;
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
    auto console = (DebugConsole*)UserData;
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
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;

    MINFO("Позиция:[%.2F, %.2F, %.2F",
        state->WorldCamera->GetPosition().x,
        state->WorldCamera->GetPosition().y,
        state->WorldCamera->GetPosition().z
    );
}

void GameOnDebugVsyncToggle(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    char cmd[30];
    MString::Copy(cmd, "mvar_set_int vsync 0", 29);
    bool VsyncEnabled = SystemsManager::GetRenderingSystem()->FlagEnabled(RenderingConfigFlagBits::VsyncEnabledBit);
    u32 length = MString::Length(cmd);
    cmd[length - 1] = VsyncEnabled ? '1' : '0';
    Console::ExecuteCommand(cmd);
}

void GamePrintMemoryMetrics(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = (Application*)UserData;
    auto state = (Game*)GameInst->state;

    MINFO(MemorySystem::GetMemoryUsageStr().c_str());

    MINFO("Распределения: %llu (%llu в этом кадре)", state->AllocCount, state->AllocCount - state->PrevAllocCount);
}

void GameSetupKeymaps(Application* app) {
    using enum Keys;
    // Глобальная раскладка клавиатуры
    Keymap GlobalKeymap;
    GlobalKeymap.BindingAdd(ESCAPE, Keymap::Press, Keymap::NoneBit, app, GameOnEscapeCallback);

    InputSystem::KeymapPush(GlobalKeymap);

    // Тестовая раскладка клавиатуры
    Keymap TestbedKeymap;
    TestbedKeymap.BindingAdd(A,     Keymap::Hold,  Keymap::NoneBit, app, GameOnYaw);
    TestbedKeymap.BindingAdd(LEFT,  Keymap::Hold,  Keymap::NoneBit, app, GameOnYaw);

    TestbedKeymap.BindingAdd(D,     Keymap::Hold,  Keymap::NoneBit, app, GameOnYaw);
    TestbedKeymap.BindingAdd(RIGHT, Keymap::Hold,  Keymap::NoneBit, app, GameOnYaw);

    TestbedKeymap.BindingAdd(UP,    Keymap::Hold,  Keymap::NoneBit, app, GameOnPitch);
    TestbedKeymap.BindingAdd(DOWN,  Keymap::Hold,  Keymap::NoneBit, app, GameOnPitch);

    TestbedKeymap.BindingAdd(GRAVE, Keymap::Press, Keymap::NoneBit, app->state, GameOnConsoleChangeVisibility);

    TestbedKeymap.BindingAdd(W,     Keymap::Hold,  Keymap::NoneBit, app, GameOnMoveForward);
    TestbedKeymap.BindingAdd(S,     Keymap::Hold,  Keymap::NoneBit, app, GameOnMoveBackward);
    TestbedKeymap.BindingAdd(Q,     Keymap::Hold,  Keymap::NoneBit, app, GameOnMoveLeft);
    TestbedKeymap.BindingAdd(E,     Keymap::Hold,  Keymap::NoneBit, app, GameOnMoveRight);
    TestbedKeymap.BindingAdd(SPACE, Keymap::Hold,  Keymap::NoneBit, app, GameOnMoveUp);
    TestbedKeymap.BindingAdd(X,     Keymap::Hold,  Keymap::NoneBit, app, GameOnMoveDown);

    TestbedKeymap.BindingAdd(KEY_0, Keymap::Press, Keymap::ControlBit, app, GameOnSetRenderModeDefault);
    TestbedKeymap.BindingAdd(KEY_1, Keymap::Press, Keymap::ControlBit, app, GameOnSetRenderModeLighting);
    TestbedKeymap.BindingAdd(KEY_2, Keymap::Press, Keymap::ControlBit, app, GameOnSetRenderModeNormals);
    TestbedKeymap.BindingAdd(KEY_3, Keymap::Press, Keymap::ControlBit, app, GameOnSetRenderModeCascades);
    TestbedKeymap.BindingAdd(KEY_4, Keymap::Press, Keymap::ControlBit, app, GameOnSetRenderModeWireframe);

    TestbedKeymap.BindingAdd(KEY_1, Keymap::Press, Keymap::NoneBit, app, GameOnSetGizmoMode);
    TestbedKeymap.BindingAdd(KEY_2, Keymap::Press, Keymap::NoneBit, app, GameOnSetGizmoMode);
    TestbedKeymap.BindingAdd(KEY_3, Keymap::Press, Keymap::NoneBit, app, GameOnSetGizmoMode);
    TestbedKeymap.BindingAdd(KEY_4, Keymap::Press, Keymap::NoneBit, app, GameOnSetGizmoMode);
    TestbedKeymap.BindingAdd(G,     Keymap::Press, Keymap::NoneBit, app, GameOnGizmoOrientationSet);

    TestbedKeymap.BindingAdd(L,     Keymap::Press, Keymap::NoneBit, app, GameOnLoadScene);
    TestbedKeymap.BindingAdd(U,     Keymap::Press, Keymap::NoneBit, app, GameOnUnloadScene);

    TestbedKeymap.BindingAdd(T,     Keymap::Press, Keymap::NoneBit, app, GameOnDebugTextureSwap);
    TestbedKeymap.BindingAdd(P,     Keymap::Press, Keymap::NoneBit, app, GameOnDebugCamPosition);
    TestbedKeymap.BindingAdd(V,     Keymap::Press, Keymap::NoneBit, app, GameOnDebugVsyncToggle);
    TestbedKeymap.BindingAdd(M,     Keymap::Press, Keymap::NoneBit, app, GamePrintMemoryMetrics);

    InputSystem::KeymapPush(TestbedKeymap);

    // Консольная раскладка клавиатуры. По умолчанию не активируется.
    auto state = (Game*)app->state;

    state->ConsoleKeymap.OverridesAll = true;
    state->ConsoleKeymap.BindingAdd(GRAVE,    Keymap::Press, Keymap::NoneBit, state, GameOnConsoleChangeVisibility);
    state->ConsoleKeymap.BindingAdd(ESCAPE,   Keymap::Press, Keymap::NoneBit, state, GameOnConsoleChangeVisibility);

    state->ConsoleKeymap.BindingAdd(PAGEUP,   Keymap::Press, Keymap::NoneBit, &state->console, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(PAGEDOWN, Keymap::Press, Keymap::NoneBit, &state->console, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(PAGEUP,   Keymap::Hold,  Keymap::NoneBit, app, GameOnConsoleScrollHold);
    state->ConsoleKeymap.BindingAdd(PAGEDOWN, Keymap::Hold,  Keymap::NoneBit, app, GameOnConsoleScrollHold);

    state->ConsoleKeymap.BindingAdd(UP,       Keymap::Press, Keymap::NoneBit, &state->console, GameOnConsoleChangeHistory);
    state->ConsoleKeymap.BindingAdd(DOWN,     Keymap::Press, Keymap::NoneBit, &state->console, GameOnConsoleChangeHistory);

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

    auto state = (Game*)app->state;

    // Удаляем все привязки для раскладки клавиатуры консоли, так как это единственное, что у нас есть.
    state->ConsoleKeymap.Clear();
}
