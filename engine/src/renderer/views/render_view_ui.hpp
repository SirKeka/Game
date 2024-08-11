#pragma once 
#include "render_view.hpp"

class RenderViewUI : public RenderView
{
private:
    u32 ShaderID;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    Matrix4D ViewMatrix;
    // u32 RenderMode;
public:
    RenderViewUI();
    RenderViewUI(u16 id, MString& name, KnownType type, u8 RenderpassCount, const char* CustomShaderName);
    ~RenderViewUI();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(void* data, Packet& OutPacket) const override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) const override;
};
