#pragma once 
#include "render_view.hpp"

class RenderViewUI : public RenderView
{
private:
    class Shader* shader     {};
    f32 NearClip             {};
    f32 FarClip              {};
    Matrix4D ProjectionMatrix{};
    Matrix4D ViewMatrix      {};
    u16 DiffuseMapLocation   {};
    u16 DiffuseColourLocation{};
    u16 ModelLocation        {};
    // u32 RenderMode;
public:
    // constexpr RenderViewUI();
    /*constexpr */RenderViewUI(u16 id, MString&& name, KnownType type, u8 RenderpassCount, const char* CustomShaderName, RenderpassConfig* PassConfig);
    ~RenderViewUI();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(void* data, Packet& OutPacket) const override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) const override;

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
