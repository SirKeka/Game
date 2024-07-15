#include "renderer.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/shader_system.hpp"

RendererType *Renderer::ptrRenderer;

constexpr bool CriticalInit(bool op, const MString& message)
{
    if (!op) {
        MERROR(message.c_str());
        return false;
    }
    return true;
}

Renderer::Renderer(MWindow *window, const char *ApplicationName, ERendererType type)
: 
NearClip(0.1f), 
FarClip(1000.f), 
MaterialShaderID(), 
UIShaderID(), 
RenderMode(),
projection(Matrix4D::MakeFrustumProjection(Math::DegToRad(45.0f), 1280 / 720.0f, NearClip, FarClip)), 
view(Matrix4D::MakeTranslation(Vector3D<f32>{0, 0, -30.f})), 
UIProjection(Matrix4D::MakeOrthographicProjection(0, 1280.0f, 720.0f, 0, -100.f, 100.0f)),              // Намеренно перевернуто по оси Y.
UIView(Matrix4D::MakeIdentity()),
AmbientColour(0.25f, 0.25f, 0.25f, 1.0f),
ViewPosition()
{
    if(type == ERendererType::VULKAN) {
        //ptrRenderer = dynamic_cast<VulkanAPI*> (ptrRenderer);

        Event::GetInstance()->Register(EVENT_CODE_SET_RENDER_MODE, this, OnEvent);

        CriticalInit(ptrRenderer = new VulkanAPI(window, ApplicationName), "Систему рендеринга не удалось инициализировать. Выключение.");

        // Shaders
        Resource ConfigResource;
        ShaderConfig* config = nullptr;

        // Встроенный шейдер материала.
        CriticalInit(
            ResourceSystem::Instance()->Load(BUILTIN_SHADER_NAME_MATERIAL, ResourceType::Shader, ConfigResource),
            "Не удалось загрузить встроенный шейдер материала.");
        config = reinterpret_cast<ShaderConfig*>(ConfigResource.data);
        CriticalInit(ShaderSystem::GetInstance()->Create(config), "Не удалось загрузить встроенный шейдер материала.");
        ResourceSystem::Instance()->Unload(ConfigResource);
        MaterialShaderID = ShaderSystem::GetInstance()->GetID(BUILTIN_SHADER_NAME_MATERIAL);

        // Встроенный шейдер пользовательского интерфейса.
        CriticalInit(
            ResourceSystem::Instance()->Load(BUILTIN_SHADER_NAME_UI, ResourceType::Shader, ConfigResource),
            "Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
        config = reinterpret_cast<ShaderConfig*>(ConfigResource.data);
        CriticalInit(ShaderSystem::GetInstance()->Create(config), "Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
        ResourceSystem::Instance()->Unload(ConfigResource);
        UIShaderID = ShaderSystem::GetInstance()->GetID(BUILTIN_SHADER_NAME_UI);

        // ЗАДАЧА: настраиваемое начальное положение камеры.
        view.Inverse();
        UIView.Inverse();
    }
}

Renderer::~Renderer()
{
    delete ptrRenderer;
}
/*
bool Renderer::Initialize(class MWindow* window, const char *ApplicationName, ERendererType type)
{
    switch (type)
    {
    case RENDERER_TYPE_VULKAN:
        //ptrRenderer = dynamic_cast<VulcanAPI> (ptrRenderer);
        ptrRenderer = new VulcanAPI(ApplicationName);
        break;
    case RENDERER_TYPE_DIRECTX:

        break;
    case RENDERER_TYPE_OPENGL:

        break;
    }
    
}
*/
void Renderer::Shutdown()
{
    //this->~Renderer();
}

void Renderer::OnResized(u16 width, u16 height)
{
    if (ptrRenderer) {
        projection = Matrix4D::MakeFrustumProjection(Math::DegToRad(45.0f), width / height, NearClip, FarClip);
        UIProjection = Matrix4D::MakeOrthographicProjection(0, width, height, 0, -100.f, 100.0f);
        ptrRenderer->Resized(width, height);
    } else {
        MWARN("Средства визуализации (Renderer) не существует, чтобы принять изменение размера: %i %i", width, height);
    }
}

bool Renderer::DrawFrame(RenderPacket &packet)
{
    ptrRenderer->FrameNumber++;
    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (ptrRenderer->BeginFrame(packet.DeltaTime)) {
        // Мировой проход рендеринга
        if (!ptrRenderer->BeginRenderpass(static_cast<u8>(BuiltinRenderpass::World))) {
            MERROR("Ошибка Renderer::BeginRenderpass -> BuiltinRenderpass::World. Приложение закрывается...");
            return false;
        }

        if(!ShaderSystem::GetInstance()->Use(MaterialShaderID)) {
            MERROR("Не удалось использовать шейдер материала. Не удалось выполнить рендеринг кадра.");
            return false;
        }

        // Apply globals
        if(!MaterialSystem::Instance()->ApplyGlobal(MaterialShaderID, projection, view, AmbientColour, ViewPosition, RenderMode)) {
            MERROR("Не удалось использовать глобальные переменные для шейдера материала. Не удалось выполнить рендеринг кадра.");
            return false;
        }

        // Отрисовка геометрии
        const u32& count = packet.GeometryCount;
        for (u32 i = 0; i < count; ++i) {
            Material* m = nullptr;
            if (packet.geometries[i].gid->material) {
                m = packet.geometries[i].gid->material;
            } else {
                m = MaterialSystem::Instance()->GetDefaultMaterial();
            }

            // Примените материал, если он еще не был в этом кадре. 
            // Это предотвращает многократное обновление одного и того же материала.
            bool NeedsUpdate = m->RenderFrameNumber != ptrRenderer->FrameNumber;
            if (!MaterialSystem::Instance()->ApplyInstance(m, NeedsUpdate)) {
                MWARN("Не удалось применить материал «%s». Пропуск отрисовки.", m->name);
                continue;
            } else {
                // Синхронизация ноиера кадров.
                m->RenderFrameNumber = ptrRenderer->FrameNumber;
            }

            // Приминение locals
            MaterialSystem::Instance()->ApplyLocal(m, packet.geometries[i].model);

            // Отрисовка.
            ptrRenderer->DrawGeometry(packet.geometries[i]);
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

        // Обновить глобальное состояние пользовательского интерфейса
        if(!ShaderSystem::GetInstance()->Use(UIShaderID)) {
            MERROR("Не удалось использовать шейдер пользовательского интерфейса. Не удалось выполнить рендеринг кадра.");
            return false;
        }

        // Применить глобальные переменные
        if(!MaterialSystem::Instance()->ApplyGlobal(UIShaderID, UIProjection, UIView)) {
            MERROR("Не удалось использовать глобальные переменные для шейдера пользовательского интерфейса. Не удалось выполнить рендеринг кадра.");
            return false;
        }

        // Нарисуйте геометрию пользовательского интерфейса.
        const u32& UIcount = packet.UI_GeometryCount;
        for (u32 i = 0; i < UIcount; ++i) {
            Material* m = nullptr;
            if (packet.UI_Geometries[i].gid->material) {
                m = packet.UI_Geometries[i].gid->material;
            } else {
                m = MaterialSystem::Instance()->GetDefaultMaterial();
            }
            // Приминение материала
            bool NeedsUpdate = m->RenderFrameNumber != ptrRenderer->FrameNumber;
            if (!MaterialSystem::Instance()->ApplyInstance(m, NeedsUpdate)) {
                MWARN("Не удалось применить материал пользовательского интерфейса «%s». Пропуск отрисовки.", m->name);
                continue;
            } else {
                // Синхронизация ноиера кадров.
                m->RenderFrameNumber = ptrRenderer->FrameNumber;
            }

            // Приминение locals
            MaterialSystem::Instance()->ApplyLocal(m, packet.UI_Geometries[i].model);

            // Отрисовка.
            ptrRenderer->DrawGeometry(packet.UI_Geometries[i]);
        }

        if (!ptrRenderer->EndRenderpass(static_cast<u8>(BuiltinRenderpass::UI))) {
            MERROR("Ошибка Renderer::EndRenderpass -> BuiltinRenderpass::UI. Приложение закрывается...");
            return false;
        }
        // Завершить рендеринг пользовательского интерфейса

        // Завершите кадр. Если это не удастся, скорее всего, это будет невозможно восстановить.
        bool result = ptrRenderer->EndFrame(packet.DeltaTime);

        if (!result) {
            MERROR("Ошибка EndFrame. Приложение закрывается...");
            return false;
        }
    }

    return false;
}

bool Renderer::Load(const u8* pixels, Texture *texture)
{
    return ptrRenderer->Load(const u8* pixels, texture);
}

void Renderer::Unload(Texture *texture)
{
    ptrRenderer->Unload(texture)
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
    // ЗАДАЧА: HACK: Нужны динамические проходы рендеринга(renderpass) вместо их жесткого кодирования.
    if (name.Comparei("Renderpass.Builtin.World")) {
        OutRenderpassID = static_cast<u8>(BuiltinRenderpass::World);
        return true;
    } else if (name.Comparei("Renderpass.Builtin.UI")) {
        OutRenderpassID = static_cast<u8>(BuiltinRenderpass::UI);
        return true;
    }

    MERROR("Renderer::RenderpassID: Renderpass не указан '%s'.", name.c_str());
    OutRenderpassID = INVALID::U8ID;
    return false;
}

bool Renderer::Load(Shader *shader, u8 RenderpassID, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage *stages)
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

bool Renderer::ShaderApplyInstance(Shader *shader, bool NeedsUpdate)
{
    return ptrRenderer->ShaderApplyInstance(shader, NeedsUpdate);
}

bool Renderer::ShaderAcquireInstanceResources(Shader *shader, TextureMap **maps, u32 &OutInstanceID)
{
    return ptrRenderer->ShaderAcquireInstanceResources(shader, maps, OutInstanceID);
}

bool Renderer::ShaderReleaseInstanceResources(Shader *shader, u32 InstanceID)
{
    return ptrRenderer->ShaderReleaseInstanceResources(shader, InstanceID);
}

bool Renderer::SetUniform(Shader *shader, ShaderUniform *uniform, const void *value)
{
    return ptrRenderer->SetUniform(shader, uniform, value);
}

bool Renderer::TextureMapAcquireResources(TextureMap *map)
{
    return ptrRenderer->TextureMapAcquireResources(map);
}

void Renderer::TextureMapReleaseResources(TextureMap *map)
{
    ptrRenderer->TextureMapReleaseResources(map);
}

void Renderer::SetView(const Matrix4D& view, const Vector3D<f32>& ViewPosition)
{
    this->view = view;
    this->ViewPosition = ViewPosition;
}

void *Renderer::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}

bool Renderer::OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    switch (code) {
        case EVENT_CODE_SET_RENDER_MODE: {
            Renderer* state = reinterpret_cast<Renderer*>(ListenerInst);
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case RendererDebugViewMode::Default:
                    MDEBUG("Режим рендеринга установлен по умолчанию.");
                    state->RenderMode = RendererDebugViewMode::Default;
                    break;
                case RendererDebugViewMode::Lighting:
                    MDEBUG("Режим рендерера установлен на освещение.");
                    state->RenderMode = RendererDebugViewMode::Lighting;
                    break;
                case RendererDebugViewMode::Normals:
                    MDEBUG("Режим рендерера установлен на нормальный.");
                    state->RenderMode = RendererDebugViewMode::Normals;
                    break;
            }
            return true;
        }
    }

    return false;
}
