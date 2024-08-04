#include "renderer.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/shader_system.hpp"

RendererType *Renderer::ptrRenderer;

void regenerate_render_targets();

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
ViewPosition(),
WindowRenderTargetCount(),
// Размер буфера кадра по умолчанию. Переопределяется при создании окна.
FramebufferWidth(1280),
FramebufferHeight(720),
WorldRenderpass(ptrRenderer->GetRenderpass("Renderpass.Builtin.World")),
UiRenderpass(ptrRenderer->GetRenderpass("Renderpass.Builtin.UI")),
resizing(false),
FramesSinceResize()
{
    if(type == ERendererType::VULKAN) {
        //ptrRenderer = dynamic_cast<VulkanAPI*> (ptrRenderer);

        Event::GetInstance()->Register(EVENT_CODE_SET_RENDER_MODE, this, OnEvent);

        // Проходы рендеринга. ЗАДАЧА: прочитать конфигурацию из файла.
        RenderpassConfig RpConfig[2] = {
            RenderpassConfig(
                "Renderpass.Builtin.World", 
                nullptr, 
                "Renderpass.Builtin.UI", 
                FVec4(0, 0, 1280, 720), 
                FVec4(0.0f, 0.0f, 0.2f, 1.0f),
                RenderpassClearFlag::ColourBuffer | RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer
            ),
            RenderpassConfig(
                "Renderpass.Builtin.UI",
                "Renderpass.Builtin.World",
                nullptr,
                FVec4(0, 0, 1280, 720), 
                FVec4(0.0f, 0.0f, 0.2f, 1.0f),
                RenderpassClearFlag::None
            )
        };

        RendererConfig RConfig {
            ApplicationName,
            2,
            RpConfig,
            RendererConfig::PFN_Method(this, &Renderer::RegenerateRenderTargets)
        };

        CriticalInit(ptrRenderer = new VulkanAPI(window, RConfig, WindowRenderTargetCount), "Систему рендеринга не удалось инициализировать. Выключение.");

        // ЗАДАЧА: Узнаем, как их получить, когда определим представления.
        WorldRenderpass->RenderTargetCount = WindowRenderTargetCount;
        WorldRenderpass->targets = new RenderTarget[WindowRenderTargetCount];

        UiRenderpass->RenderTargetCount = WindowRenderTargetCount;
        UiRenderpass->targets = new RenderTarget[WindowRenderTargetCount];

        RegenerateRenderTargets();

        // Обновите размеры основного/мирового рендерпасса.
        WorldRenderpass->RenderArea.x = 0;
        WorldRenderpass->RenderArea.y = 0;
        WorldRenderpass->RenderArea.z = FramebufferWidth;
        WorldRenderpass->RenderArea.w = FramebufferHeight;

        // Также обновите размеры рендерпасса пользовательского интерфейса.
        UiRenderpass->RenderArea.x = 0;
        UiRenderpass->RenderArea.y = 0;
        UiRenderpass->RenderArea.z = FramebufferWidth;
        UiRenderpass->RenderArea.w = FramebufferHeight;

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
    ptrRenderer = nullptr;
}

void Renderer::Shutdown()
{
    // Уничтожить цели рендеринга.
    for (u8 i = 0; i < WindowRenderTargetCount; ++i) {
        ptrRenderer->RenderTargetDestroy(WorldRenderpass->targets[i], true);
        ptrRenderer->RenderTargetDestroy(UiRenderpass->targets[i], true);
    }
}

void Renderer::OnResized(u16 width, u16 height)
{
    if (ptrRenderer) {
        // Отметить как изменение размера и сохранить изменение, но дождаться повторной генерации.
        resizing = true;
        FramebufferWidth = width;
        FramebufferHeight = height;
        // Также сбросить количество кадров с момента последней операции изменения размера.
        FramesSinceResize = 0;
    } else {
        MWARN("Средства визуализации (Renderer) не существует, чтобы принять изменение размера: %i %i", width, height);
    }
}

bool Renderer::DrawFrame(RenderPacket &packet)
{
    ptrRenderer->FrameNumber++;

    // Убедитесь, что окно в данный момент не меняет размер, подождав указанное количество кадров после последней операции изменения размера, прежде чем выполнять внутренние обновления.
    if (resizing) {
        FramesSinceResize++;

        // Если требуемое количество кадров прошло с момента изменения размера, продолжайте и выполните фактические обновления.
        if (FramesSinceResize >= 30) {
            f32 width = FramebufferWidth;
            f32 height = FramebufferHeight;
            projection = Matrix4D::MakeFrustumProjection(Math::DegToRad(45.F), width / height, NearClip, FarClip);
            UIProjection = Matrix4D::MakeOrthographicProjection(0, width, height, 0, -100.f, 100.0f);  // Намеренно перевернуто по оси Y.
            ptrRenderer->Resized(width, height);

            FramesSinceResize = 0;
            resizing = false;
        } else {
            // Пропустите рендеринг кадра и попробуйте снова в следующий раз.
            return true;
        }
    }

    // ЗАДАЧА: представления
    // Обновите размеры основного/мирового рендерпасса.
    WorldRenderpass->RenderArea.z = FramebufferWidth;
    WorldRenderpass->RenderArea.w = FramebufferHeight;

    // Также обновите размеры рендерпасса пользовательского интерфейса.
    UiRenderpass->RenderArea.z = FramebufferWidth;
    UiRenderpass->RenderArea.w = FramebufferHeight;

    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (ptrRenderer->BeginFrame(packet.DeltaTime)) {
        const u8& AttachmentIndex = ptrRenderer->WindowAttachmentIndexGet();
        // Мировой проход рендеринга
        // ЗАДАЧА: только рендерпас
        if (!ptrRenderer->BeginRenderpass(WorldRenderpass, WorldRenderpass->targets[AttachmentIndex])) {
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

        if (!ptrRenderer->EndRenderpass(WorldRenderpass)) {
            MERROR("Ошибка Renderer::EndRenderpass -> WorldRenderpass. Приложение закрывается...");
            return false;
        }
        // Конец рендеринга мира

         // UI renderpass
        if (!ptrRenderer->BeginRenderpass(UiRenderpass, UiRenderpass->targets[AttachmentIndex])) {
            MERROR("Ошибка Renderer::BeginRenderpass -> UiRenderpass. Приложение закрывается...");
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

        if (!ptrRenderer->EndRenderpass(UiRenderpass)) {
            MERROR("Ошибка Renderer::EndRenderpass -> UiRenderpass. Приложение закрывается...");
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

void Renderer::Load(const u8* pixels, Texture *texture)
{
    return ptrRenderer->Load(pixels, texture);
}

void Renderer::LoadTextureWriteable(Texture *texture)
{
    ptrRenderer->LoadTextureWriteable(texture);
}

void Renderer::TextureResize(Texture *texture, u32 NewWidth, u32 NewHeight)
{
    ptrRenderer->TextureResize(texture, NewWidth, NewHeight);
}

void Renderer::TextureWriteData(Texture *texture, u32 offset, u32 size, const u8 *pixels)
{
    ptrRenderer->TextureWriteData(texture, offset, size, pixels);
}

void Renderer::Unload(Texture *texture)
{
    ptrRenderer->Unload(texture);
}

bool Renderer::Load(GeometryID *gid, u32 VertexSize, u32 VertexCount, const void *vertices, u32 IndexSize, u32 IndexCount, const void *indices)
{
    return ptrRenderer->Load(gid, VertexSize, VertexCount, vertices, IndexSize, IndexCount, indices);
}

void Renderer::Unload(GeometryID *gid)
{
    return ptrRenderer->Unload(gid);
}

Renderpass* Renderer::GetRenderpass(const MString &name)
{
    return ptrRenderer->GetRenderpass(name);
}

bool Renderer::Load(Shader *shader, Renderpass* renderpass, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage *stages)
{
    return ptrRenderer->Load(shader, renderpass, StageCount, StageFilenames, stages);
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

void Renderer::RenderTargetCreate(u8 AttachmentCount, Texture **attachments, Renderpass *pass, u32 width, u32 height, RenderTarget &OutTarget)
{
    ptrRenderer->RenderTargetCreate(AttachmentCount, attachments, pass, width, height, OutTarget);
}

void Renderer::RenderTargetDestroy(RenderTarget &target, bool FreeInternalMemory)
{
    ptrRenderer->RenderTargetDestroy(target, FreeInternalMemory);
}

void Renderer::RegenerateRenderTargets() {
    // Создайте цели рендеринга для каждого. ЗАДАЧА: Должны быть настраиваемыми.
    for (u8 i = 0; i < WindowRenderTargetCount; ++i) {
        // Сначала уничтожьте старые, если они существуют.
        ptrRenderer->RenderTargetDestroy(WorldRenderpass->targets[i], false);
        ptrRenderer->RenderTargetDestroy(UiRenderpass->targets[i], false);

        Texture* WindowTargetTexture = ptrRenderer->WindowAttachmentGet(i);
        Texture* DepthTargetTexture = ptrRenderer->DepthAttachmentGet();

        // Цели рендеринга мира.
        Texture* attachments[2] = {WindowTargetTexture, DepthTargetTexture};
        ptrRenderer->RenderTargetCreate(
            2,
            attachments,
            WorldRenderpass,
            FramebufferWidth,
            FramebufferHeight,
            WorldRenderpass->targets[i]);

        // Цели рендеринга пользовательского интерфейса
        Texture* UiAttachments[1] = {WindowTargetTexture};
        ptrRenderer->RenderTargetCreate(
            1,
            UiAttachments,
            UiRenderpass,
            FramebufferWidth,
            FramebufferHeight,
            UiRenderpass->targets[i]);

    }
}

void Renderer::RenderpassCreate(Renderpass *OutRenderpass, f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass)
{
    ptrRenderer->RenderpassCreate(OutRenderpass, depth, stencil, HasPrevPass, HasNextPass);
}

void Renderer::RenderpassDestroy(Renderpass *OutRenderpass)
{
    ptrRenderer->RenderpassDestroy(OutRenderpass);
}

Texture *Renderer::WindowAttachmentGet(u8 index)
{
    return ptrRenderer->WindowAttachmentGet(index);
}

Texture *Renderer::DepthAttachmentGet()
{
    return ptrRenderer->DepthAttachmentGet();
}

u8 Renderer::WindowAttachmentIndexGet()
{
    return ptrRenderer->WindowAttachmentIndexGet();
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
