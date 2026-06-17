#pragma once
#include "containers/array.h"
#include "renderer/rendergraph.h"

struct GeometryRenderData;
struct Shader;

struct EditorPass : RendergraphNode
{
    Array<GeometryRenderData> DebugGeometries;

    ~EditorPass();
    void Destroy() override;
    bool Initialize() override;
    bool Render(FrameData& rFrameData) override;  
    
    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
private:
    Shader* shader;
    // Индексы привязки шейдера
    u16 projection;
    u16 view;
    u16 model;
};
    
    