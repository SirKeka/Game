#pragma once
#include "render_view.hpp"

class RenderViewSkybox : public RenderView
{
private:
    u32 ShaderID;
    f32 fov;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    class Camera* WorldCamera;
    // Uniform locations
    u16 ProjectionLocation;
    u16 ViewLocation;
    u16 CubeMapLocation;
public:
    constexpr RenderViewSkybox() : RenderView(), ShaderID(), fov(), NearClip(), FarClip(), ProjectionMatrix(), WorldCamera(nullptr), /*locations(),*/ ProjectionLocation(), ViewLocation(), CubeMapLocation() {}
    constexpr RenderViewSkybox(u16 id, MString& name, KnownType type, u8 RenderpassCount, const char* CustomShaderName) 
    : RenderView(id, name, type, RenderpassCount, CustomShaderName),  {}
    ~RenderViewSkybox();

    //void render_view_skybox_on_destroy(struct render_view* self);
    void Resize(u32 width, u32 height) override;
    bool BuildPacket(void* data, Packet& OutPacket) override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) override;
};
