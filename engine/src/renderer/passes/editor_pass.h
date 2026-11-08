#pragma once

#include "renderer/rendergraph.h"
#include "renderer/renderer_types.h"

struct Shader;

struct EditorPass : RendergraphNode
{
    DArray<GeometryRenderData> DebugGeometries;

    constexpr EditorPass(const char* name) : RendergraphNode(name), DebugGeometries() {}
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
    
    