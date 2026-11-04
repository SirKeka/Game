#pragma once

#include "../ui_system.h"

class MAPI Panel : public UiElement {
public:
    FVec4 rect;
    FVec4 colour;
    Geometry* geometry{};
    bool IsDirty{true};
    
    constexpr Panel(UiElement* parent = nullptr) : UiElement(parent), rect(), colour(), geometry(nullptr), IsDirty() {}
    constexpr Panel(const char* name, FVec2 size, FVec4 colour, UiElement* parent = nullptr) 
    : UiElement(name, parent),            rect(0, 0, size.width, size.height), colour(colour) {}
    constexpr Panel(MString&& name,   FVec2 size, FVec4 colour, UiElement* parent = nullptr) 
    : UiElement((MString&&)name, parent), rect(0, 0, size.width, size.height), colour(colour) {}

    constexpr bool Create(MString& name,   FVec2 size, FVec4 colour, UiElement* parent = nullptr); 
    
    // bool Create(const char* name, FVec2 size, FVec4 colour, UiElement* parent = nullptr);
    bool Load() override;
    // void Unload() override;
    
    // bool Update(FrameData& rFrameData) override;
    void RenderPrepare(const FrameData& rFrameData) override;
    bool Render(FrameData& rFrameData, UiRenderData& RenderData) override;
    
    FVec2 Size();
    bool Resize(FVec2 NewSize);
};
