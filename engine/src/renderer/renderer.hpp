#pragma once

#include "defines.hpp"

#include "renderer_types.hpp"

// TODO: временно
#include "containers/mstring.hpp"
#include "core/event.hpp"
// TODO: временно

struct StaticMeshData;
struct PlatformState;
class Texture;
class VulkanAPI;

class Renderer
{
private:
    static RendererType* ptrRenderer;
    Matrix4D projection;
    Matrix4D view;
    f32 NearClip;
    f32 FarClip;

    //TODO: временно
    static Material* TestMaterial; 
    //TODO: временно
public:
    Renderer() : projection(), view(), NearClip(0.f), FarClip(0.f) {}
    ~Renderer();

    bool Initialize(MWindow* window, const char *ApplicationName, ERendererType type);
    void Shutdown();
    bool BeginFrame(f32 DeltaTime);
    bool EndFrame(f32 DeltaTime);
    void OnResized(u16 width, u16 height);
    bool DrawFrame(RenderPacket* packet);

    static VulkanAPI* GetRendererType();
    static bool CreateMaterial(class Material* material);
    static void DestroyMaterial(class Material* material);

    // ВЗЛОМ: это не должно быть выставлено за пределы движка.
    MAPI void SetView(Matrix4D view);

    void* operator new(u64 size);
    // void operator delete(void* ptr);
private:
    //static void CreateTexture(Texture* t);

    //TODO: Временно
    static bool EventOnDebugEvent(u16 code, void* sender, void* ListenerInst, EventContext data);
    //TODO: Временно
};

