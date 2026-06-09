#pragma once

#include "../ui_system.h"
#include "resources/font_resource.hpp"

class MAPI Label : public UiElement  {
public:
    Point size;
    FVec4 colour;
    UiGeometry geometry;
    MString text;
    FontData* data;
    FontType type;
    u32 MaxTextLength;
    // u32 CachedUt8Length;
    
    constexpr Label();
    constexpr Label(const char* name, FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);
    constexpr Label(MString&& name,   FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);

    bool Create(const char* name, FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);

    bool Load() override;
    void Unload() override;
    bool Render(FrameData& rFrameData, UiRenderData& RenderData) override;
    
    void ClearText() { text.Destroy(); }

    /// @brief Устанавливает текст для заданного объекта метки.
    /// @param text Текст, который будет установлен.
    void SetText(const char* text);

    /// @brief Устанавливает текст для заданного объекта метки.
    /// @param text текст, который будет установлен.
    /// @param copy указывает нужно ли скопировать текст или переместить
    void SetText(MString& text, bool copy = true);
    
    const MString& GetText() { return text; }
private:
    void RegenerateGeometry();
};


