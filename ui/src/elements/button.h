#pragma once

#include "../ui_system.h"
#include "math/nine_slice.h"

class MAPI Button : public UiElement 
{
public:
    Point size;
    FVec4 colour;
    NineSlice nSlice;
    // union {
    //     NineSlice* nslice;
    //     Geometry* geometry;
    // };
    
    constexpr Button(UiElement* parent = nullptr) : UiElement(parent), size(), colour(), nSlice() { InitFunctions(); }
    constexpr Button(const char* name, UiElement* parent = nullptr)  : UiElement(name, parent), size(200, 50), colour(FVec4::One()) { InitFunctions();}
    void Destroy() override;
    bool SetHeight(i32 height);
    
    bool Load() override;
    void Unload() override;
    
    bool Update(FrameData& rFrameData) override;
    void RenderPrepare(const FrameData& rFrameData) override;
    bool Render(FrameData& rFrameData, UiRenderData& RenderData) override;
    
    static void OnMouseDown(UiElement* self, UiMouseEvent event);
    static void OnMouseUp  (UiElement* self, UiMouseEvent event);
    static void OnMouseOver(UiElement* self, UiMouseEvent event);
    static void OnMouseOut (UiElement* self, UiMouseEvent event);

    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::UI); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::UI); }
private:
    void InitFunctions();
    };
