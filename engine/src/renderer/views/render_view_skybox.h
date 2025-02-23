#pragma once
#include "render_view.h"
#include "systems/shader_system.h"
#include "systems/camera_system.hpp"

class RenderViewSkybox : public RenderView
{
private:
    Shader* shader           {nullptr};
    f32 fov                         {};
    f32 NearClip                    {};
    f32 FarClip                     {};
    Matrix4D ProjectionMatrix       {};
    class Camera* WorldCamera{nullptr};
    // Uniform locations
    u16 ProjectionLocation          {};
    u16 ViewLocation                {};
    u16 CubeMapLocation             {};
public:
    RenderViewSkybox(u16 id, const Config &config);
    ~RenderViewSkybox();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(class LinearAllocator& FrameAllocator, void* data, Packet& OutPacket) override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData) override;

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
