#pragma once

#include "renderer/passes/editor_pass.h"

class Camera;
struct FrameData;
struct Gizmo;
struct SimpleScene;
struct Viewport;

struct EditorRendergraphConfig {
    u16 dummy;
};

struct MAPI EditorRendergraph : public Rendergraph 
{
    Gizmo* gizmo;
    EditorPass editorPass;

    bool Create(EditorRendergraphConfig* config = nullptr);

    bool Initialize();
    bool Update(FrameData& rFrameData) { return true; }
    bool FramePrepare(FrameData& rFrameData, Camera& CurrentCamera, Viewport& CurrentViewport, SimpleScene& scene, u32 RenderMode);

    void SetGizmo(Gizmo& gizmo) { this->gizmo = &gizmo; }
};