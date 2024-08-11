#pragma once
#include "render_view.hpp"
#include "core/event.hpp"

class RenderViewWorld : public RenderView
{
private:
    u32 ShaderID;
    f32 fov;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    class Camera* WorldCamera;
    FVec4 AmbientColour;
    u32 RenderMode;
public:
    RenderViewWorld();
    RenderViewWorld(u16 id, MString& name, KnownType type, u8 RenderpassCount, const char* CustomShaderName);
    ~RenderViewWorld();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(void* data, Packet& OutPacket) const override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) const override;
private:
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
};
