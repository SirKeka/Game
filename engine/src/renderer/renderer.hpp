#pragma once

#include "defines.hpp"

#include "renderer_types.hpp"
#include "math/vertex.hpp"

// TODO: временно
#include "containers/mstring.hpp"
#include "core/event.hpp"
// TODO: временно

struct StaticMeshData;
struct PlatformState;
class VulkanAPI;

class Renderer
{
private:
    static RendererType* ptrRenderer;
    Matrix4D projection;
    Matrix4D view;
    f32 NearClip;
    f32 FarClip;

public:
    Renderer() : projection(), view(), NearClip(0.f), FarClip(0.f) {}
    ~Renderer();

    bool Initialize(MWindow* window, const char *ApplicationName, ERendererType type);
    void Shutdown();
    bool BeginFrame(f32 DeltaTime);
    bool EndFrame(f32 DeltaTime);
    void OnResized(u16 width, u16 height);
    bool DrawFrame(RenderPacket* packet);

    static VulkanAPI* GetRenderer();
    static bool CreateMaterial(class Material* material);
    static void DestroyMaterial(class Material* material);

    static bool Load(GeometryID* gid, u32 VertexCount, const Vertex3D* vertices, u32 IndexCount, const u32* indices);
    static void Unload(GeometryID* gid);

    // ВЗЛОМ: это не должно быть выставлено за пределы движка.
    MAPI void SetView(Matrix4D view);

    void* operator new(u64 size);
    // void operator delete(void* ptr);
private:
    //static void CreateTexture(class Texture* t);
};

