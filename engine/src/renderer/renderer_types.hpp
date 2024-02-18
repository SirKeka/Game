#pragma once

#include "defines.hpp"

struct StaticMeshData;
class MWindow;

enum ERendererType {
    RENDERER_TYPE_VULKAN,
    RENDERER_TYPE_OPENGL,
    RENDERER_TYPE_DIRECTX
};

struct RenderPacket
{
    f32 DeltaTime;
};

class RendererType
{
public:
    u64 FrameNumber;
public:
    //RendererType();
    virtual ~RendererType() = default;

    virtual bool Initialize(MWindow* window, const char* ApplicationName) = 0;
    virtual void ShutDown() = 0;
    virtual void Resized(u16 width, u16 height) = 0;
    virtual bool BeginFrame(f32 Deltatime) = 0;
    virtual bool EndFrame(f32 DeltaTime) = 0;
};
