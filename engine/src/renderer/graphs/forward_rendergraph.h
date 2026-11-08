#pragma once

#include "renderer/passes/skybox_pass.h"
#include "renderer/passes/shadow_map_pass.h"
#include "renderer/passes/scene_pass.h"

struct FrameData;
class Camera;
struct Viewport;
struct SimpleScene;

struct ForwardRendergraphConfig {
    u16 ShadowmapResolution;
};

struct MAPI ForwardRendergraph : public Rendergraph
{
    // Rendergraph internal_graph;
    u16 ShadowmapResolution;
    SkyboxPass skyboxPass;
    ShadowmapPass shadowmapPass;
    ScenePass scenePass;

    constexpr ForwardRendergraph(bool& result, ForwardRendergraphConfig* config = nullptr);
    
    bool Initialize();
    bool Update(FrameData& rFrameData);
    bool FramePrepare(FrameData& rFrameData, Camera& CurrentCamera, Viewport& CurrentViewport, SimpleScene& scene, u32 RenderMode);
};
