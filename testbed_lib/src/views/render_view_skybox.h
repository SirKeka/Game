#pragma once
#include "renderer/render_view.h"

struct RenderViewSkybox
{
    struct Shader* shader;
    f32 fov;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    class Camera* WorldCamera;
    // Uniform locations
    u16 ProjectionLocation;
    u16 ViewLocation;
    u16 CubeMapLocation;

    constexpr RenderViewSkybox() : shader(), fov(Math::DegToRad(45.F)), NearClip(0.1F), FarClip(1000.F), 
    ProjectionMatrix(Matrix4D::MakeFrustumProjection(fov, 1280 / 720.F, NearClip, FarClip)), 
    WorldCamera(), 
    // locations(),
    ProjectionLocation(), ViewLocation(), CubeMapLocation() {}
    ~RenderViewSkybox();

    static bool OnRegistered(RenderView* self);
    static void Resize(RenderView* self, u32 width, u32 height);
    static bool BuildPacket(RenderView* self, class LinearAllocator& FrameAllocator, void* data, RenderView::Packet& OutPacket);
    static bool Render(RenderView* self, const RenderView::Packet& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
