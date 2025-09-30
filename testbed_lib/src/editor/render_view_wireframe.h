#pragma once
#include "renderer/render_view.h"

struct RenderViewWireframeData
{
    DArray<GeometryRenderData>* WorldGeometries;
    DArray<GeometryRenderData>* TerrainGeometries;
    DArray<GeometryRenderData>* DebugGeometries;
    u32 SelectedID;
};

namespace RenderViewWireframe
{
    bool OnRegistered(RenderView* self);
    void Destroy(RenderView* self);
    void Resize(RenderView* self, u32 width, u32 height);
    bool BuildPacket(RenderView* self, FrameData& rFrameData, Viewport& viewport, Camera* camera, void* data, RenderViewPacket& OutPacket);
    bool Render(const RenderView* self, const RenderViewPacket& packet, const FrameData& rFrameData);
} // namespace RenderViewWireframe

