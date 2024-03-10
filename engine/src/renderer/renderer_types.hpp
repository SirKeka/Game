#pragma once

#include "defines.hpp"
#include "math/matrix4d.hpp"

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

/*размер данной струтуры для карт Nvidia должен быть равен 256 байт*/
struct GlobalUniformObject
{
    Matrix4D projection;  // 64 байта
    Matrix4D view;        // 64 байта
    Matrix4D mReserved0;  // 64 байта, зарезервированные для будущего использования
    Matrix4D mReserved1;  // 64 байта, зарезервированные для будущего использования
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
    virtual void UpdateGlobalState(const Matrix4D& projection, const Matrix4D& view, const Vector3D<f32>& ViewPosition, const Vector4D<f32>& AmbientColour, i32 mode) = 0;
    virtual bool EndFrame(f32 DeltaTime) = 0;
};
