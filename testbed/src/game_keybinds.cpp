#include "game.hpp"

#include <core/event.hpp>
#include <core/logger.hpp>
#include <renderer/camera.hpp>
#include <renderer/renderer.hpp>
#include "debug_console.hpp"

void GameOnEscapeCallback(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    MDEBUG("GameOnEscapeCallback");
    Event::Fire(EVENT_CODE_APPLICATION_QUIT, nullptr, (EventContext){});
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
        Input::KeymapPush(state->ConsoleKeymap);
    } else {
        Input::KeymapPop();
    }
}

void GameOnSetRenderModeDefault(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data = {};
    data.data.i32[0] = Render::Default;
    Event::Fire(EVENT_CODE_SET_RENDER_MODE, /*(Game*)*/UserData, data);
}

void GameOnSetRenderModeLighting(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data = {};
    data.data.i32[0] = Render::Lighting;
    Event::Fire(EVENT_CODE_SET_RENDER_MODE, /*(Game*)*/UserData, data);
}

void GameOnSetRenderModeNormals(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    EventContext data = {};
    data.data.i32[0] = Render::Normals;
    Event::Fire(EVENT_CODE_SET_RENDER_MODE, /*(Game*)*/UserData, data);
}

void GameOnLoadScene(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    Event::Fire(EVENT_CODE_DEBUG1, /*(Game*)*/UserData, (EventContext){});
}

void GameOnConsoleScroll(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    if (key == Keys::UP) {
        DebugConsole::MoveUp();
    } else if (key == Keys::DOWN) {
        DebugConsole::MoveDown();
    }
}

void GameOnConsoleScrollHold(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);

    static f32 AccumulatedTime = 0.F;
    AccumulatedTime += state->DeltaTime;
    if (AccumulatedTime >= 0.1F) {
        if (key == Keys::UP) {
            DebugConsole::MoveUp();
        } else if (key == Keys::DOWN) {
            DebugConsole::MoveDown();
        }
        AccumulatedTime = 0.F;
    }
}

void GameOnDebugTextureSwap(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    MDEBUG("Swapping texture!");
    EventContext context = {};
    Event::Fire(EVENT_CODE_DEBUG0, UserData, context);
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

void GameOnDebugVsyncToggle(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    bool VsyncEnabled = Renderer::FlagEnabled(RendererConfigFlagBits::VsyncEnabledBit);
    VsyncEnabled = !VsyncEnabled;
    Renderer::FlagSetEnabled(RendererConfigFlagBits::VsyncEnabledBit, VsyncEnabled);
}

void GamePrintMemoryMetrics(Keys key, Keymap::EntryBindType type, Keymap::Modifier modifiers, void* UserData) {
    auto GameInst = reinterpret_cast<Game*>(UserData);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);

    auto usage = MMemory::GetMemoryUsageStr();
    MINFO(usage.c_str());
    MDEBUG("Распределения: %llu (%llu в этом кадре)", state->AllocCount, state->AllocCount - state->PrevAllocCount);
}

void Game::SetupKeymaps() {
    // Глобальная раскладка клавиатуры
    Keymap GlobalKeymap;
    GlobalKeymap.BindingAdd(Keys::ESCAPE, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnEscapeCallback);

    Input::KeymapPush(GlobalKeymap);

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

    Input::KeymapPush(TestbedKeymap);

    // Консольная раскладка клавиатуры. По умолчанию не активируется.
    auto state = reinterpret_cast<State*>(this->state);
    state->ConsoleKeymap = Keymap();
    state->ConsoleKeymap.OverridesAll = true;
    state->ConsoleKeymap.BindingAdd(Keys::GRAVE,  Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleChangeVisibility);
    state->ConsoleKeymap.BindingAdd(Keys::ESCAPE, Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleChangeVisibility);

    state->ConsoleKeymap.BindingAdd(Keys::UP,     Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(Keys::DOWN,   Keymap::BindTypePress, Keymap::ModifierNoneBit, this, GameOnConsoleScroll);
    state->ConsoleKeymap.BindingAdd(Keys::UP,     Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnConsoleScrollHold);
    state->ConsoleKeymap.BindingAdd(Keys::DOWN,   Keymap::BindTypeHold,  Keymap::ModifierNoneBit, this, GameOnConsoleScrollHold);
}
