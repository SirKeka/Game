#pragma once
#include "renderer/render_view.h"
#include "resources/texture.hpp"
#include "resources/mesh.h"

class RenderViewPick
{
public:
    struct PacketData {
    DArray<GeometryRenderData>* WorldMeshData;   // Указатель на динамический массив
    DArray<GeometryRenderData>* TerrainMeshData; // Указатель на динамический массив
    Mesh::PacketData UiMeshData;
    u32 UiGeometryCount;
    // ЗАДАЧА: временно
    u32 TextCount;
    struct Text** texts;
};
private:
    struct ShaderInfo {
        struct Shader* s;
        Renderpass* pass;
        u16 IdColourLocation;
        u16 ModelLocation;
        u16 ProjectionLocation;
        u16 ViewLocation;
        Matrix4D projection;
        Matrix4D view;
        f32 NearClip;
        f32 FarClip;
        f32 fov;
    };
    
    ShaderInfo UiShaderInfo;
    ShaderInfo WorldShaderInfo;
    ShaderInfo TerrainShaderInfo;
    Texture ColoureTargetAttachmentTexture; // Используется как цветовая привязка для обоих проходов рендеринга.
    Texture DepthTargetAttachmentTexture;   // Привязка глубины
    u32 InstanceCount;
    DArray<bool> InstanceUpdate;
    u16 MouseX, MouseY;
    // u32 RenderMode;

public:
    constexpr RenderViewPick() : ColoureTargetAttachmentTexture(), DepthTargetAttachmentTexture(), InstanceCount(), MouseX(), MouseY() {}
    ~RenderViewPick();

    static bool OnRegistered(RenderView* self);
    static void Resize(RenderView* self, u32 width, u32 height);
    static bool BuildPacket(RenderView* self, class LinearAllocator& FrameAllocator, void* data, RenderView::Packet& OutPacket);
    static bool Render(RenderView* self, const RenderView::Packet& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData);
    static bool RegenerateAttachmentTarget(RenderView* self, u32 PassIndex = 0, struct RenderTargetAttachment* attachment = nullptr);

    void GetMatrices(Matrix4D& OutView, Matrix4D& OutProjection);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);

private:
    static bool OnMouseMoved(u16 code, void* sender, void* ListenerInst, EventContext EventData);
    void AcquireShaderInstances();
    void ReleaseShaderInstances();
};
