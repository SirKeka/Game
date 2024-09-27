#pragma once
#include "render_view.hpp"
#include "systems/shader_system.hpp"
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
    RenderViewSkybox(u16 id, MString&& name, KnownType type, u8 RenderpassCount, const char* CustomShaderName, RenderpassConfig* PassConfig);
    ~RenderViewSkybox();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(void* data, Packet& OutPacket) const override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) const override;

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
