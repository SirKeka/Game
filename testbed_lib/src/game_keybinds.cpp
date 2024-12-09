#include "game.hpp"

#include <core/event.hpp>
#include <core/logger.hpp>
#include <core/input.hpp>
#include <core/console.hpp>
#include <renderer/camera.hpp>
#include <renderer/rendering_system.hpp>
#include "debug_console.hpp"

void GameOnEscapeCallback(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    MDEBUG("GameOnEscapeCallback");
    EventSystem::Fire(EventSystem::ApplicationQuit, nullptr, (EventContext){});
}

void GameOnYaw(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state); 

    f32 f = 0.F;
    if (key == Keys::LEFT || key == Keys::A) {
        f = 1.F;
    } else if (key == Keys::RIGHT || key == Keys::D) {
        f = -1.F;
    }
    state->WorldCamera->Yaw(f * state->DeltaTime);
}

void GameOnPitch(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);

    f32 f = 0.F;
    if (key == Keys::UP) {
        f = 1.F;
    } else if (key == Keys::DOWN) {
        f = -1.F;
    }
    state->WorldCamera->Pitch(f * state->DeltaTime);
}

void GameOnMoveForward(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
    static const f32 TempMoveSpeed = 50.F;
    state->WorldCamera->MoveForward(TempMoveSpeed * state->DeltaTime);
}

void GameOnMoveBackward(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
    static const f32 TempMoveSpeed = 50.F;
    state->WorldCamera->MoveBackward(TempMoveSpeed * state->DeltaTime);
}

void GameOnMoveLeft(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
    static const f32 TempMoveSpeed = 50.F;
    state->WorldCamera->MoveLeft(TempMoveSpeed * state->DeltaTime);
}

void GameOnMoveRight(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
    static const f32 TempMoveSpeed = 50.0f;
    state->WorldCamera->MoveRight(TempMoveSpeed * state->DeltaTime);
}

void GameOnMoveUp(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
    static const f32 TempMoveSpeed = 50.0f;
    state->WorldCamera->MoveUp(TempMoveSpeed * state->DeltaTime);
}

void GameOnMoveDown(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
    static const f32 TempMoveSpeed = 50.0f;
    state->WorldCamera->MoveDown(TempMoveSpeed * state->DeltaTime);
}

void GameOnConsoleChangeVisibility(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);

    bool ConsoleVisible = DebugConsole::Visible();
    ConsoleVisible = !ConsoleVisible;

    DebugConsole::VisibleSet(ConsoleVisible);
    if (ConsoleVisible) {
        InputSystem::KeymapPush(state->ConsoleKeymap);
    } else {
        InputSystem::KeymapPop();
    }
}

void GameOnSetRenderModeDefault(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data = {};
    data.data.i32[0] = Render::Default;
    EventSystem::Fire(EventSystem::SetRenderMode, /*(Game*)*/UserData, data);
}

void GameOnSetRenderModeLighting(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data = {};
    data.data.i32[0] = Render::Lighting;
    EventSystem::Fire(EventSystem::SetRenderMode, /*(Game*)*/UserData, data);
}

void GameOnSetRenderModeNormals(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data = {};
    data.data.i32[0] = Render::Normals;
    EventSystem::Fire(EventSystem::SetRenderMode, /*(Game*)*/UserData, data);
}

void GameOnLoadScene(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventSystem::Fire(EventSystem::DEBUG1, /*(Game*)*/UserData, (EventContext){});
}

void GameOnConsoleScroll(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    if (key == Keys::PAGEUP) {
        DebugConsole::MoveUp();
    } else if (key == Keys::PAGEDOWN) {
        DebugConsole::MoveDown();
    }
}

void GameOnConsoleScrollHold(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);

    static f32 AccumulatedTime = 0.F;
    AccumulatedTime += state->DeltaTime;
    if (AccumulatedTime >= 0.1F) {
        if (key == Keys::PAGEUP) {
            DebugConsole::MoveUp();
        } else if (key == Keys::PAGEDOWN) {
            DebugConsole::MoveDown();
        }
        AccumulatedTime = 0.F;
    }
}

void GameOnConsoleChangeHistory(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData)
{
    if (key == Keys::UP) {
        DebugConsole::HistoryBack();
    } else if (key == Keys::DOWN) {
        DebugConsole::HistoryForward();
    }
}

void GameOnDebugTextureSwap(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    MDEBUG("Swapping texture!");
    EventContext context = {};
    EventSystem::Fire(EventSystem::DEBUG0, UserData, context);
}

void GameOnDebugCamPosition(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);

    MDEBUG(
        "Позиция:[%.2F, %.2F, %.2F",
        state->WorldCamera->GetPosition().x,
        state->WorldCamera->GetPosition().y,
        state->WorldCamera->GetPosition().z);
}

static void ToggleVsync() {
    bool VsyncEnabled = RenderingSystem::FlagEnabled(RenderingConfigFlagBits::VsyncEnabledBit);
    VsyncEnabled = !VsyncEnabled;
    RenderingSystem::FlagSetEnabled(RenderingConfigFlagBits::VsyncEnabledBit, VsyncEnabled);
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
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);

    auto usage = MemorySystem::GetMemoryUsageStr();
    MINFO(usage.c_str());
    MDEBUG("Распределения: %llu (%llu в этом кадре)", state->AllocCount, state->AllocCount - state->PrevAllocCount);
}

static bool GameOnMVarChanged(u16 code, void* sender, void* ListenerInst, EventContext data) {
    if (code == EventSystem::MVarChanged && MString::Equali(data.data.c, "vsync")) {
        ToggleVsync();
    }
    return false;
}

void Game::SetupKeymaps() {
    EventSystem::Register(EventSystem::MVarChanged, nullptr, GameOnMVarChanged);
    
    // Глобальная раскладка клавиатуры
    Keymap GlobalKeymap;
    GlobalKeymap.BindingAdd(Keys::ESCAPE, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnEscapeCallback);

    InputSystem::KeymapPush(GlobalKeymap);

    // Тестовая раскладка клавиатуры
    Keymap TestbedKeymap;
    TestbedKeymap.BindingAdd(Keys::A,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnYaw);
    TestbedKeymap.BindingAdd(Keys::LEFT,  Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnYaw);

    TestbedKeymap.BindingAdd(Keys::D,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnYaw);
    TestbedKeymap.BindingAdd(Keys::RIGHT, Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnYaw);

    TestbedKeymap.BindingAdd(Keys::UP,    Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnPitch);
    TestbedKeymap.BindingAdd(Keys::DOWN,  Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnPitch);

    TestbedKeymap.BindingAdd(Keys::GRAVE, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleChangeVisibility);

    TestbedKeymap.BindingAdd(Keys::W,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnMoveForward);
    TestbedKeymap.BindingAdd(Keys::S,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnMoveBackward);
    TestbedKeymap.BindingAdd(Keys::Q,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnMoveLeft);
    TestbedKeymap.BindingAdd(Keys::E,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnMoveRight);
    TestbedKeymap.BindingAdd(Keys::SPACE, Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnMoveUp);
    TestbedKeymap.BindingAdd(Keys::X,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnMoveDown);

    TestbedKeymap.BindingAdd(Keys::KEY_0, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnSetRenderModeDefault);
    TestbedKeymap.BindingAdd(Keys::KEY_1, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnSetRenderModeLighting);
    TestbedKeymap.BindingAdd(Keys::KEY_2, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnSetRenderModeNormals);

    TestbedKeymap.BindingAdd(Keys::L,     Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnLoadScene);

    TestbedKeymap.BindingAdd(Keys::T,     Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnDebugTextureSwap);
    TestbedKeymap.BindingAdd(Keys::P,     Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnDebugCamPosition);
    TestbedKeymap.BindingAdd(Keys::V,     Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnDebugVsyncToggle);
    TestbedKeymap.BindingAdd(Keys::M,     Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GamePrintMemoryMetrics);

    InputSystem::KeymapPush(TestbedKeymap);

    // Консольная раскладка клавиатуры. По умолчанию не активируется.
    auto state = reinterpret_cast<State*>(this->state);

    state->ConsoleKeymap.OverridesAll = true;
    state->ConsoleKeymap.BindingAdd(Keys::GRAVE,    Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleChangeVisibility);
    state->ConsoleKeymap.BindingAdd(Keys::ESCAPE,   Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleChangeVisibility);

    state->ConsoleKeymap.BindingAdd(Keys::PAGEUP,   Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(Keys::PAGEDOWN, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(Keys::PAGEUP,   Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnConsoleScrollHold);
    state->ConsoleKeymap.BindingAdd(Keys::PAGEDOWN, Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnConsoleScrollHold);

    state->ConsoleKeymap.BindingAdd(Keys::UP,       Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleChangeHistory);
    state->ConsoleKeymap.BindingAdd(Keys::DOWN,     Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleChangeHistory);
}
