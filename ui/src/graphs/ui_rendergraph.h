#pragma once

#include "passes/ui_pass.h"

struct FrameData;
class Camera;
struct Viewport;
struct SimpleScene;

struct UiRendergraphConfig {
    u16 dummy;
};

struct MAPI UiRendergraph : public Rendergraph 
{
    UiPass uiPass;

    constexpr UiRendergraph(bool& result, const UiRendergraphConfig* config = nullptr);

    bool Initialize();
    bool Update(FrameData& rFrameData) { return true; }
    bool FramePrepare(FrameData& rFrameData, Camera& CurrentCamera, Viewport& CurrentViewport, SimpleScene& scene, u32 RenderMode);
};

