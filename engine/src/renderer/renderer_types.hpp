#pragma once

#include "defines.hpp"
#include "math/matrix4d.hpp"
#include "math/vector4d.hpp"
#include "math/vertex.hpp"

struct StaticMeshData;

enum ERendererType 
{
    RENDERER_TYPE_VULKAN,
    RENDERER_TYPE_OPENGL,
    RENDERER_TYPE_DIRECTX
};

struct RenderPacket
{
    f32 DeltaTime;

    u32 GeometryCount;
    struct GeometryRenderData* geometries;
};

/*размер данной струтуры для карт Nvidia должен быть равен 256 байт*/
struct GlobalUniformObject
{
    Matrix4D projection;  // 64 байта
    Matrix4D view;        // 64 байта
    Matrix4D mReserved0;  // 64 байта, зарезервированные для будущего использования
    Matrix4D mReserved1;  // 64 байта, зарезервированные для будущего использования
};

struct MaterialUniformObject 
{
    Vector4D<f32> DiffuseColor; // 16 байт
    Vector4D<f32> vReserved0;   // 16 байт, зарезервировано для будущего использования.
    Vector4D<f32> vReserved1;   // 16 байт, зарезервировано для будущего использования.
    Vector4D<f32> vReserved2;   // 16 байт, зарезервировано для будущего использования.
};

struct GeometryRenderData 
{
    Matrix4D model;
    struct GeometryID* gid;
};

class RendererType
{
public:
    u64 FrameNumber;

    // Указатель на текстуру по умолчанию.
    class Texture* DefaultDiffuse;
public:
    //RendererType();
    virtual ~RendererType() = default;

    virtual bool Initialize(class MWindow* window, const char* ApplicationName) = 0;
    virtual void ShutDown() = 0;
    virtual void Resized(u16 width, u16 height) = 0;
    virtual bool BeginFrame(f32 Deltatime) = 0;
    virtual void UpdateGlobalState(const Matrix4D& projection, const Matrix4D& view, const Vector3D<f32>& ViewPosition, const Vector4D<f32>& AmbientColour, i32 mode) = 0;
    virtual bool EndFrame(f32 DeltaTime) = 0;
    virtual void DrawGeometry(const GeometryRenderData& data) = 0;
    virtual bool Load(class Material* material) = 0;
    virtual void Unload(class Material* material) = 0;
    virtual bool Load(struct GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices) = 0;
    virtual void Unload(struct GeometryID* gid) = 0;
};
