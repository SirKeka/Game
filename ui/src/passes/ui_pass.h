#pragma once
#include "ui_system.h"
#include <renderer/rendergraph.h>

struct Shader;

struct MAPI UiPass : RendergraphNode
{
    // pImpl?
    UiRenderData RenderData;

    ~UiPass() { Destroy(); }
    void Destroy() override;
    bool Initialize() override;
    bool Render(FrameData& rFrameData) override;

    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
private:
    Shader* shader;
    // Индексы привязки шейдера
    // struct UI_ShaderLocation { // стандартный интерфейс // ЗАДАЧА: другой проход рендеринга?
        u16 projection;
        u16 view;
        u16 model;
        u16 properties;
        u16 DiffuseMap;
    // } location1;
};

    