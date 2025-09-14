#pragma once
#include "renderer/render_view.h"

struct RenderViewWorldData {
    DArray<GeometryRenderData> WorldGeometries;
    DArray<GeometryRenderData> TerrainGeometries;
    DArray<GeometryRenderData> DebugGeometries;
};

class RenderViewWorld
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
    constexpr RenderViewWorld() : shader(), fov(Math::DegToRad(45.F)), NearClip(0.1F), FarClip(4000.F), 
    ProjectionMatrix(Matrix4D::MakeFrustumProjection(fov, 1280 / 720.F, NearClip, FarClip)), // Поумолчанию
    WorldCamera(), AmbientColour(0.25F, 0.25F, 0.25F, 1.F), RenderMode() {}

    static bool OnRegistered(RenderView* self);
    static void Destroy(RenderView* self);
    static void Resize(RenderView* self, u32 width, u32 height);
    static bool BuildPacket(RenderView* self, LinearAllocator& FrameAllocator, void* data, RenderViewPacket& OutPacket);
    static bool Render(const RenderView* self, const RenderViewPacket& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
private:
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
};
