#pragma once
#include "render_view.hpp"

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
    constexpr RenderViewWorld();
    ~RenderViewWorld();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(void* data, Packet* OutPacket) override;
    bool Render(const Packet* packet, u64 FrameNumber, u64 RenderTargetIndex) override;
private:
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
};
