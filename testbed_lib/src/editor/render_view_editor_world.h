#pragma once

#include "defines.h"

struct RenderView;
struct LinearAllocator;
struct FrameData;
struct Gizmo;
struct Viewport;
struct RenderViewPacket;
class Camera;

struct EditorWorldPacketData {
    Gizmo* gizmo;
};

namespace RenderViewEditorWorld
{
    bool OnRegistered(RenderView* self);
    void Destroy(RenderView* self);
    void Resize(RenderView* self, u32 width, u32 height);
    bool BuildPacket(RenderView* self, FrameData& pFrameData, Viewport& viewport, Camera* camera, void* data, RenderViewPacket& OutPacket);
    bool Render(const RenderView* self, const RenderViewPacket& packet, const FrameData& rFrameData);
} // namespace RenderViewEditorWorld
