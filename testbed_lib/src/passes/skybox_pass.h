#pragma once
#include "renderer/rendergraph.h"

struct Shader;

struct SkyboxPass : RendergraphNode
{
    struct Sky* skybox;

    ~SkyboxPass();
    void Destroy() override;
    bool Initialize() override;
    bool Render(FrameData& rFrameData) override;

    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
private:
    Shader* shader;
    // Индексы привязки шейдера
    u16 ProjectionLocation;
    u16 ViewLocation;
    u16 CubeMapLocation;
};