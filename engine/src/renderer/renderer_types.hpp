#pragma once

#include "defines.hpp"

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
    virtual ~RendererType();

    virtual bool Initialize();
    virtual void ShutDown();
    virtual void Resized(u16 width, u16 height);
    virtual bool BeginFrame(f32 Deltatime);
    virtual bool EndFrame(f32 DeltaTime);
};
