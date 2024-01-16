#pragma once

#include "defines.hpp"

enum RendererBackendType {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
};

class RendererBackend
{
private:
    f32 DeltaTime;
public:
    RendererBackend();
    ~RendererBackend();

    void ShutDown();
    void Resize(u16 width, u16 height);
    bool BeginFrame(f32 Deltatime);
    bool EndFrame(f32 DeltaTime);
};
