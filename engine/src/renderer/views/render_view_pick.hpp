#pragma once
#include "render_view.hpp"
#include "resources/texture.hpp"
#include "resources/mesh.hpp"

class RenderViewPick : public RenderView
{
public:
    struct PacketData {
    Mesh::PacketData WorldMeshData;
    u32 WorldGeometryCount;
    Mesh::PacketData UiMeshData;
    u32 UiGeometryCount;
    // ЗАДАЧА: временно
    u32 TextCount;
    class Text** texts;
};
private:
    struct ShaderInfo {
        class Shader* s;
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
    Texture ColoureTargetAttachmentTexture; // Используется как цветовая привязка для обоих проходов рендеринга.
    Texture DepthTargetAttachmentTexture;   // Привязка глубины
    i32 InstanceCount;
    DArray<bool> InstanceUpdate;
    u16 MouseX, MouseY;
    // u32 RenderMode;

public:
    RenderViewPick(u16 id, const Config &config);
    ~RenderViewPick();

    void Resize(u32 width, u32 height) override;
    bool BuildPacket(class LinearAllocator& FrameAllocator, void* data, Packet& OutPacket) override;
    void DestroyPacket(Packet& packet);
    bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) override;
    bool RegenerateAttachmentTarget(u32 PassIndex = 0, struct RenderTargetAttachment* attachment = nullptr) override;

    void GetMatrices(Matrix4D& OutView, Matrix4D& OutProjection);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);

private:
    static bool OnMouseMoved(u16 code, void* sender, void* ListenerInst, EventContext EventData);
    void AcquireShaderInstances();
    void ReleaseShaderInstances();
};
