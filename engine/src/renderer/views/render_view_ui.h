#pragma once 
#include "render_view.h"

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
    /*constexpr */RenderViewUI(u16 id, const Config &config);
    ~RenderViewUI();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(class LinearAllocator& FrameAllocator, void* data, Packet& OutPacket) override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) override;

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
