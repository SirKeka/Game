#pragma once

#include "renderer/rendergraph.h"

struct GeometryRenderData;

#define MAX_CASCADE_COUNT 4

struct ShadowMapCascadeData {
    Matrix4D projection;
    Matrix4D view;
    f32 SplitDepth;
    i32 CascadeIndex;
};

struct MAPI ShadowmapPass : public RendergraphNode {
    struct Impl;
    
    struct Data {
        struct DirectionalLight* light;
        // Данные по каждому каскаду.
        ShadowMapCascadeData cascades[MAX_CASCADE_COUNT];
        DArray<GeometryRenderData> TerrainGeometries;
        DArray<GeometryRenderData> geometries;
    }* ExtendedData;
    
    bool Create(u16 resolution);
    void Destroy() override;

    bool Initialize() override;
    static bool LoadResources(RendergraphNode* self);
    bool Render(FrameData& rFrameData) override;
    
    static bool AttachmentPopulate(RendergraphNode* self, RenderTargetAttachment& attachment);
    static bool SourcePopulate(RendergraphNode* self, RendergraphSource& source);
    
    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
    
private:
    struct Impl* pImpl;
};
