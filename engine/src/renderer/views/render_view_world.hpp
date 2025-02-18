#pragma once
#include "render_view.hpp"
#include "core/event.hpp"

struct RenderViewWorldData {
    DArray<GeometryRenderData> WorldGeometries;
    DArray<GeometryRenderData> TerrainGeometries;
};

class RenderViewWorld : public RenderView
{
private:
    class Shader* shader;
    f32 fov;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    class Camera* WorldCamera;
    FVec4 AmbientColour;
    u32 RenderMode;
public:
    // RenderViewWorld();
    /*constexpr */RenderViewWorld(u16 id, const Config &config);
    ~RenderViewWorld();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(class LinearAllocator& FrameAllocator, void* data, Packet& OutPacket) override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) override;

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
private:
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
};
