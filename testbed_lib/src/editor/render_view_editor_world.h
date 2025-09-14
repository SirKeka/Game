#pragma once

#include "defines.h"

struct RenderView;
struct LinearAllocator;
struct FrameData;
struct Gizmo;
struct RenderViewPacket;

struct EditorWorldPacketData {
    Gizmo* gizmo;
};

namespace RenderViewEditorWorld
{
    bool OnRegistered(RenderView* self);
    void Destroy(RenderView* self);
    void Resize(RenderView* self, u32 width, u32 height);
    bool BuildPacket(RenderView* self, LinearAllocator& FrameAllocator, void* data, RenderViewPacket& OutPacket);
    bool Render(const RenderView* self, const RenderViewPacket& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& pFrameData);
} // namespace RenderViewEditorWorld
