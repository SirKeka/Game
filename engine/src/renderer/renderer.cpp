#include "renderer.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"

RendererType *Renderer::ptrRenderer;
/*f32 Renderer::NearClip = 0.1f;
f32 Renderer::FarClip = 1000.f;
Matrix4D Renderer::projection = Matrix4::MakeFrustumProjection(Math::DegToRad(45.0f), 1280 / 720.0f, NearClip, FarClip);
Matrix4D Renderer::view = Matrix4::MakeTranslation(Vector3D<f32>{0, 0, -30.f});*/

Renderer::~Renderer()
{
    delete ptrRenderer; //TODO: Unhandled exception at 0x00007FFEADC9B93C (engine.dll) in testbed.exe: 0xC0000005: Access violation reading location 0x0000000000000000.
}

bool Renderer::Initialize(MWindow* window, const char *ApplicationName, ERendererType type)
{
    /*switch (type)
    {
    case RENDERER_TYPE_VULKAN:
        //ptrRenderer = dynamic_cast<VulcanAPI> (ptrRenderer);
        ptrRenderer = new VulcanAPI(ApplicationName);
        break;
    case RENDERER_TYPE_DIRECTX:

        break;
    case RENDERER_TYPE_OPENGL:

        break;
    }*/
    if(type == ERendererType::VULKAN) {
        //ptrRenderer = dynamic_cast<VulkanAPI*> (ptrRenderer);

        ptrRenderer = new VulkanAPI();
        // Возьмите указатель на текстуры по умолчанию для использования в серверной части.
        ptrRenderer->Initialize(window, ApplicationName);
        NearClip = 0.1f;
        FarClip = 1000.f;
        projection = Matrix4::MakeFrustumProjection(Math::DegToRad(45.0f), 1280 / 720.0f, NearClip, FarClip);
        view = Matrix4::MakeTranslation(Vector3D<f32>{0, 0, -30.f});
        view.Inverse();

        return true;
    }
    return false;
}

void Renderer::Shutdown()
{
    //this->~Renderer();
}

bool Renderer::BeginFrame(f32 DeltaTime)
{
    return ptrRenderer->BeginFrame(DeltaTime);
}

bool Renderer::EndFrame(f32 DeltaTime)
{
    bool result = ptrRenderer->EndFrame(DeltaTime);
    ptrRenderer->FrameNumber++;
    return result;
}

void Renderer::OnResized(u16 width, u16 height)
{
    if (ptrRenderer) {
        projection = Matrix4::MakeFrustumProjection(Math::DegToRad(45.0f), width / (f32)height, NearClip, FarClip);
        ptrRenderer->Resized(width, height);
    } else {
        MWARN("Средства визуализации (Renderer) не существует, чтобы принять изменение размера: %i %i", width, height);
    }
}

bool Renderer::DrawFrame(RenderPacket *packet)
{
    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (BeginFrame(packet->DeltaTime)) {
        ptrRenderer->UpdateGlobalState(projection, view, Vector3D<f32>::Zero(), Vector4D<f32>::Zero(), 0);

        u32 count = packet->GeometryCount;
        for (u32 i = 0; i < count; ++i) {
            ptrRenderer->DrawGeometry(packet->geometries[i]);
        }

        // Завершите кадр. Если это не удастся, скорее всего, это будет невозможно восстановить.
        bool result = EndFrame(packet->DeltaTime);

        if (!result) {
            MERROR("Ошибка EndFrame. Приложение закрывается...");
            return FALSE;
        }
    }

    return false;
}

VulkanAPI *Renderer::GetRenderer()
{
    return dynamic_cast<VulkanAPI*>(ptrRenderer);
}

bool Renderer::CreateMaterial(Material *material)
{
    return ptrRenderer->CreateMaterial(material);
}

void Renderer::DestroyMaterial(Material *material)
{
    ptrRenderer->DestroyMaterial(material);
}

bool Renderer::Load(GeometryID* gid, u32 VertexCount, const Vertex3D* vertices, u32 IndexCount, const u32* indices)
{
    return ptrRenderer->Load(gid, VertexCount, vertices, IndexCount, indices);
}

void Renderer::Unload(GeometryID *gid)
{
}

void Renderer::SetView(Matrix4D view)
{
    this->view = view;
}

void *Renderer::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}
