#pragma once

#include "defines.hpp"
#include <core/event.hpp>
#include <resources/ui_text.hpp>

namespace DebugConsole 
{
    void Create();
    bool Load();
    void Unload();
    void Update();

    Text* GetText();
    Text* GetEntryText();

    bool Visible();
    void VisibleSet(bool visible);

    void MoveUp();
    void MoveDown();
    void MoveToTop();
    void MoveToBottom();
}; // namespace DebugConsole


