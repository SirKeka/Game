#pragma once 
#include "renderer/render_view.h"

class RenderViewUI
{
private:
    struct Shader* shader;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    Matrix4D ViewMatrix;
    u16 DiffuseMapLocation;
    u16 PropertiesLocation;
    u16 ModelLocation;
    // u32 RenderMode;
public:
    constexpr RenderViewUI() : shader(), NearClip(-100.F), FarClip(100.F), ProjectionMatrix(Matrix4D::MakeOrthographicProjection(0.F, 1280.F, 720.F, 0.F, NearClip, FarClip)), 
    ViewMatrix(Matrix4D::MakeIdentity()), DiffuseMapLocation(), PropertiesLocation(), ModelLocation() /*RenderMode(),*/ {}
    ~RenderViewUI();

    static bool OnRegistered(RenderView* self);
    static void Destroy(RenderView* self);
    static void Resize(RenderView* self, u32 width, u32 height);
    static bool BuildPacket(RenderView* self, struct LinearAllocator& FrameAllocator, void* data, RenderViewPacket& OutPacket);
    static bool Render(const RenderView* self, const RenderViewPacket& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
