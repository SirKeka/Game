#pragma once
#include "renderer/render_view.h"

struct Shader;

struct RenderViewWorldData {
    DArray<GeometryRenderData> WorldGeometries;
    DArray<GeometryRenderData> TerrainGeometries;
    DArray<GeometryRenderData> DebugGeometries;
    SkyboxPacketData SkyboxData;
};

struct SkyboxShaderLocation
{
    u16 ProjectionLocation;
    u16 ViewLocation;
    u16 CubeMapLocation;
};


class RenderViewWorld
{
private:
    Shader* MaterialShader;
    Shader* SkyboxShader;
    Shader* TerrainShader;
    Shader* ColourShader;

    FVec4 AmbientColour;
    u32 RenderMode;
    struct DebugColourShaderLocations {
        u16 projection;
        u16 view;
        u16 model;
    } DebugLocations;
    SkyboxShaderLocation SkyboxLocation;

public:
    constexpr RenderViewWorld() : MaterialShader(nullptr), SkyboxShader(nullptr), TerrainShader(nullptr), ColourShader(nullptr),  AmbientColour(0.25F, 0.25F, 0.25F, 1.F), RenderMode(), DebugLocations(), SkyboxLocation() {}

    static bool OnRegistered(RenderView* self);
    static void Destroy(RenderView* self);
    static void Resize(RenderView* self, u32 width, u32 height);
    static bool BuildPacket(RenderView* self, FrameData& rFrameData, Viewport& viewport, Camera* camera, void* data, RenderViewPacket& OutPacket);
    static bool Render(const RenderView* self, const RenderViewPacket& packet, const FrameData& rFrameData);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
private:
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
};
