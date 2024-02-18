#pragma once

#include "defines.hpp"

#include "renderer_types.hpp"
#include "renderer/vulkan/vulcan_api.hpp"

struct StaticMeshData;
struct PlatformState;

class Renderer
{
private:
    static RendererType* ptrRenderer;
public:
    Renderer() = default;
    ~Renderer();

    static bool Initialize(MWindow* window, const char *ApplicationName, ERendererType type);
    void Shutdown();

    bool BeginFrame(f32 DeltaTime);

    bool EndFrame(f32 DeltaTime);

    void OnResized(u16 width, u16 height);

    bool DrawFrame(RenderPacket* packet);

    void* operator new(u64 size);
    void operator delete(void* ptr);
};

