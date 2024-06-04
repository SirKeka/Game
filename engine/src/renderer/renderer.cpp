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

        ptrRenderer = new VulkanAPI(); // При выделении памяти линейным аллокатором происходит сбой
        // Возьмите указатель на текстуры по умолчанию для использования в серверной части.
        ptrRenderer->Initialize(window, ApplicationName);

        // Проекция мира/вид
        NearClip = 0.1f;
        FarClip = 1000.f;
        projection = Matrix4D::MakeFrustumProjection(Math::DegToRad(45.0f), 1280 / 720.0f, NearClip, FarClip);
        // TODO: настраиваемое начальное положение камеры.
        view = Matrix4D::MakeTranslation(Vector3D<f32>{0, 0, -30.f});
        view.Inverse();
        // Проекция пользовательского интерфейса/вид
        UI_Projection = Matrix4D::MakeOrthographicProjection(0, 1280.0f, 720.0f, 0, -100.f, 100.0f); // Намеренно перевернуто по оси Y.
        UI_View = Matrix4D::MakeIdentity();
        UI_View.Inverse();

        return true;
    }
    return false;
}

void Renderer::Shutdown()
{
    //this->~Renderer();
}

void Renderer::OnResized(u16 width, u16 height)
{
    if (ptrRenderer) {
        projection = Matrix4D::MakeFrustumProjection(Math::DegToRad(45.0f), width / height, NearClip, FarClip);
        UI_Projection = Matrix4D::MakeOrthographicProjection(0, width, height, 0, -100.f, 100.0f);
        ptrRenderer->Resized(width, height);
    } else {
        MWARN("Средства визуализации (Renderer) не существует, чтобы принять изменение размера: %i %i", width, height);
    }
}

bool Renderer::DrawFrame(RenderPacket *packet)
{
    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (ptrRenderer->BeginFrame(packet->DeltaTime)) {
        // Мировой проход рендеринга
        if (!ptrRenderer->BeginRenderpass(static_cast<u8>(BuiltinRenderpass::World))) {
            MERROR("Ошибка Renderer::BeginRenderpass -> BuiltinRenderpass::World. Приложение закрывается...");
            return false;
        }

        // Отрисовка геометрии
        u32 count = packet->GeometryCount;
        for (u32 i = 0; i < count; ++i) {
            ptrRenderer->DrawGeometry(packet->geometries[i]);
        }

        if (!ptrRenderer->EndRenderpass(static_cast<u8>(BuiltinRenderpass::World))) {
            MERROR("Ошибка Renderer::EndRenderpass -> BuiltinRenderpass::World. Приложение закрывается...");
            return false;
        }
        // Конец рендеринга мира

         // UI renderpass
        if (!ptrRenderer->BeginRenderpass(static_cast<u8>(BuiltinRenderpass::UI))) {
            MERROR("Ошибка Renderer::BeginRenderpass -> BuiltinRenderpass::UI. Приложение закрывается...");
            return false;
        }

        // Нарисуйте геометрию пользовательского интерфейса.
        count = packet->UI_GeometryCount;
        for (u32 i = 0; i < count; ++i) {
            ptrRenderer->DrawGeometry(packet->UI_Geometries[i]);
        }

        if (!ptrRenderer->EndRenderpass(static_cast<u8>(BuiltinRenderpass::UI))) {
            MERROR("Ошибка Renderer::EndRenderpass -> BuiltinRenderpass::UI. Приложение закрывается...");
            return false;
        }
        // Завершить рендеринг пользовательского интерфейса

        // Завершите кадр. Если это не удастся, скорее всего, это будет невозможно восстановить.
        bool result = ptrRenderer->EndFrame(packet->DeltaTime);
        ptrRenderer->FrameNumber++;

        if (!result) {
            MERROR("Ошибка EndFrame. Приложение закрывается...");
            return false;
        }
    }

    return false;
}

VulkanAPI *Renderer::GetRenderer()
{
    return dynamic_cast<VulkanAPI*>(ptrRenderer);
}

bool Renderer::Load(GeometryID *gid, u32 VertexSize, u32 VertexCount, const void *vertices, u32 IndexSize, u32 IndexCount, const void *indices)
{
    return ptrRenderer->Load(gid, VertexSize, VertexCount, vertices, IndexSize, IndexCount, indices);
}

void Renderer::Unload(GeometryID *gid)
{
    return ptrRenderer->Unload(gid);
}

bool Renderer::RenderpassID(const MString &name, u8 &OutRenderpassID)
{
    // СДЕЛАТЬ: HACK: Нужны динамические проходы рендеринга(renderpass) вместо их жесткого кодирования.
    if (name.Cmpi("Renderpass.Builtin.World")) {
        OutRenderpassID = static_cast<u8>(BuiltinRenderpass::World);
        return true;
    } else if (name.Cmpi("Renderpass.Builtin.UI")) {
        OutRenderpassID = static_cast<u8>(BuiltinRenderpass::UI);
        return true;
    }

    MERROR("Renderer::RenderpassID: Renderpass не указан '%s'.", name.c_str());
    OutRenderpassID = INVALID::U8ID;
    return false;
}

bool Renderer::Load(Shader *shader, u8 RenderpassID, u8 StageCount, DArray<MString> StageFilenames, const ShaderStage *stages)
{
    return ptrRenderer->Load(shader, RenderpassID, StageCount, StageFilenames, stages);
}

void Renderer::Unload(Shader *shader)
{
    return ptrRenderer->Unload(shader);
}

bool Renderer::ShaderInitialize(Shader *shader)
{
    return ptrRenderer->ShaderInitialize(shader);
}

bool Renderer::ShaderUse(Shader *shader)
{
    return ptrRenderer->ShaderUse(shader);
}

bool Renderer::ShaderApplyGlobals(Shader *shader)
{
    return ptrRenderer->ShaderApplyGlobals(shader);
}

bool Renderer::ShaderApplyInstance(Shader *shader)
{
    return ptrRenderer->ShaderApplyInstance(shader);
}

bool Renderer::ShaderAcquireInstanceResources(Shader *shader, u32 &OutInstanceID)
{
    return ptrRenderer->ShaderAcquireInstanceResources(shader, OutInstanceID);
}

bool Renderer::ShaderReleaseInstanceResources(Shader *shader, u32 InstanceID)
{
    return ptrRenderer->ShaderReleaseInstanceResources(shader, InstanceID);
}

bool Renderer::SetUniform(Shader *shader, ShaderUniform *uniform, const void *value)
{
    return ptrRenderer->SetUniform(shader, uniform, value);
}

void Renderer::SetView(Matrix4D view)
{
    this->view = view;
}

void *Renderer::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}
