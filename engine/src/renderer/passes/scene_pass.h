#pragma once
#include "shadow_map_pass.h"
#include "renderer/renderer_types.h"

struct Shader;
struct TextureMap;

struct ScenePass : RendergraphNode
{
    u32 RenderMode;
    FVec4 CascadeSplits;
    DArray<GeometryRenderData> geometries;
    DArray<GeometryRenderData> TerrainGeometries;
    DArray<GeometryRenderData> DebugGeometries;
    Texture* IrradianceCubeTexture;

    Matrix4D DirectionalLightViews[MAX_CASCADE_COUNT];
    Matrix4D DirectionalLightProjections[MAX_CASCADE_COUNT];

    constexpr ScenePass(const char* name) : RendergraphNode(name) {}
   ~ScenePass();
    void Destroy() override;
    bool Initialize() override;
    bool Render(FrameData& rFrameData) override;

    static bool LoadResources(RendergraphNode* self);

    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
private:
    Shader* MaterialShader;
    Shader* TerrainShader;
    Shader* ColourShader;
    RendergraphSource* ShadowmapSource;
    // Один на кадр.
    TextureMap* ShadowMaps;
    // Один на кадр.
    u32 FrameCount;
    // Индексы привязки шейдера
    u16 projection;
    u16 view;
    u16 model;

};
