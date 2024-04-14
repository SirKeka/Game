#pragma once

#include "defines.hpp"
#include "math/matrix4d.hpp"
#include "math/vector4d.hpp"
#include "math/vertex.hpp"

struct StaticMeshData;

enum class ERendererType 
{
    VULKAN,
    OPENGL,
    DIRECTX
};
/* f32 DeltaTime
 * u32 GeometryCount
 * struct GeometryRenderData* geometries
 * u32 UI_GeometryCount
 * struct GeometryRenderData* UI_Geometries*/
struct RenderPacket
{
    f32 DeltaTime;

    u32 GeometryCount;
    struct GeometryRenderData* geometries;

    u32 UI_GeometryCount;
    struct GeometryRenderData* UI_Geometries;
};

/*размер данной струтуры для карт Nvidia должен быть равен 256 байт*/
struct VulkanMaterialShaderGlobalUniformObject
{
    Matrix4D projection;  // 64 байта
    Matrix4D view;        // 64 байта
    Matrix4D mReserved0;  // 64 байта, зарезервированные для будущего использования
    Matrix4D mReserved1;  // 64 байта, зарезервированные для будущего использования
};

struct VulkanMaterialShaderInstanceUniformObject 
{
    Vector4D<f32> DiffuseColor; // 16 байт
    Vector4D<f32> vReserved0;   // 16 байт, зарезервировано для будущего использования.
    Vector4D<f32> vReserved1;   // 16 байт, зарезервировано для будущего использования.
    Vector4D<f32> vReserved2;   // 16 байт, зарезервировано для будущего использования.
};

/// @brief 
struct VulkanUI_ShaderGlobalUniformObject {
    Matrix4D projection;  // 64 bytes
    Matrix4D view;        // 64 bytes
    Matrix4D mReserved0;  // 64 bytes, зарезервировано для будущего использования.
    Matrix4D mReserved1;  // 64 bytes, зарезервировано для будущего использования.
};

/// @brief Объект универсального буфера экземпляра материала пользовательского интерфейса, специфичный для Vulkan, для шейдера пользовательского интерфейса.
struct VulkanUI_ShaderInstanceUniformObject {
    Vector4D<f32> DiffuseColor; // 16 bytes
    Vector4D<f32> vReserved0;   // 16 bytes, зарезервировано для будущего использования.
    Vector4D<f32> vReserved1;   // 16 bytes, зарезервировано для будущего использования.
    Vector4D<f32> vReserved2;   // 16 bytes, зарезервировано для будущего использования.
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
    //class Texture* DefaultDiffuse;
public:
    //RendererType();
    virtual ~RendererType() = default;

    virtual bool Initialize(class MWindow* window, const char* ApplicationName) = 0;
    virtual void ShutDown() = 0;
    virtual void Resized(u16 width, u16 height) = 0;
    virtual bool BeginFrame(f32 Deltatime) = 0;
    virtual void UpdateGlobalWorldState(const Matrix4D& projection, const Matrix4D& view, const Vector3D<f32>& ViewPosition, const Vector4D<f32>& AmbientColour, i32 mode) = 0;
    virtual void UpdateGlobalUIState(const Matrix4D& projection, const Matrix4D& view, i32 mode) = 0;
    virtual bool EndFrame(f32 DeltaTime) = 0;
    virtual bool BeginRenderpass(u8 RenderpassID) = 0;
    virtual bool EndRenderpass(u8 RenderpassID) = 0;
    virtual void DrawGeometry(const GeometryRenderData& data) = 0;
    virtual bool CreateMaterial(class Material* material) = 0;
    virtual void DestroyMaterial(class Material* material) = 0;
    virtual bool Load(struct GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices) = 0;
    virtual void Unload(struct GeometryID* gid) = 0;
};
