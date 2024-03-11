#pragma once

#include "defines.hpp"

#include "renderer_types.hpp"
#include "memory/linear_allocator.hpp"

struct StaticMeshData;
struct PlatformState;

class Renderer
{
using LinearAllocator = WrapLinearAllocator<Renderer>;

private:
    static RendererType* ptrRenderer;
    static Matrix4D projection;
    static Matrix4D view;
    static f32 NearClip;
    static f32 FarClip;
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
    // void operator delete(void* ptr);
};

