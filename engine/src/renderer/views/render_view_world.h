#pragma once
#include "render_view.h"
#include "core/event.hpp"

struct RenderViewWorldData {
    DArray<GeometryRenderData> WorldGeometries;
    DArray<GeometryRenderData> TerrainGeometries;
    DArray<GeometryRenderData> DebugGeometries;
};

class RenderViewWorld : public RenderView
{
private:
    struct Shader* shader;
    f32 fov;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    class Camera* WorldCamera;
    FVec4 AmbientColour;
    u32 RenderMode;
    struct DebugColourShaderLocations {
        u16 projection;
        u16 view;
        u16 model;
    } DebugLocations;

public:
    // RenderViewWorld();
    /*constexpr */RenderViewWorld(u16 id, const Config &config);
    ~RenderViewWorld();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(class LinearAllocator& FrameAllocator, void* data, Packet& OutPacket) override;
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData) override;

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
private:
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
};
