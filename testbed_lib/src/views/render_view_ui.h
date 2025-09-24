#pragma once 
#include "renderer/render_view.h"

class RenderViewUI
{
private:
    struct Shader* shader;
    Matrix4D ViewMatrix;
    u16 DiffuseMapLocation;
    u16 PropertiesLocation;
    u16 ModelLocation;
    // u32 RenderMode;
public:
    constexpr RenderViewUI() : shader(), ViewMatrix(Matrix4D::MakeIdentity()), DiffuseMapLocation(), PropertiesLocation(), ModelLocation() /*RenderMode(),*/ {}
    ~RenderViewUI();

    static bool OnRegistered(RenderView* self);
    static void Destroy(RenderView* self);
    static void Resize(RenderView* self, u32 width, u32 height);
    static bool BuildPacket(RenderView* self, FrameData& rFrameData, Viewport& viewport, void* data, RenderViewPacket& OutPacket);
    static bool Render(const RenderView* self, const RenderViewPacket& packet, const FrameData& rFrameData);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
