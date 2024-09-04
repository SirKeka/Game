#include "renderer.hpp"
#include "memory/linear_allocator.hpp"
#include "platform/platform.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/render_view_system.hpp"
#include "views/render_view.hpp"

RendererType *Renderer::ptrRenderer = nullptr;

#define CriticalInit(op, message)\
    if (!op) {                   \
        MERROR(message);         \
        return;                  \
    }

Renderer::Renderer(MWindow *window, const char *ApplicationName, ERendererType type)
:  
SkyboxShaderID(),
MaterialShaderID(), 
UIShaderID(), 
WindowRenderTargetCount(),
// Размер буфера кадра по умолчанию. Переопределяется при создании окна.
FramebufferWidth(1280),
FramebufferHeight(720),
SkyboxRenderpass(nullptr),
WorldRenderpass(nullptr),
UiRenderpass(nullptr),
resizing(false),
FramesSinceResize()
{
    const char* SkyboxRenderpassName = "Renderpass.Builtin.Skybox";
    const char* WorldRenderpassName = "Renderpass.Builtin.World";
    const char* UiRenderpassName = "Renderpass.Builtin.UI";
    
    // Проходы рендеринга. ЗАДАЧА: прочитать конфигурацию из файла.
    RenderpassConfig RpConfig[3] = {
        RenderpassConfig(
            SkyboxRenderpassName,
            nullptr,
            WorldRenderpassName,
            FVec4(0, 0, 1280, 720),
            FVec4(0.0f, 0.0f, 0.2f, 1.0f),
            RenderpassClearFlag::ColourBuffer
        ),
        RenderpassConfig(
            WorldRenderpassName, 
            SkyboxRenderpassName, 
            UiRenderpassName, 
            FVec4(0, 0, 1280, 720), 
            FVec4(0.0f, 0.0f, 0.2f, 1.0f),
            RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer
        ),
        RenderpassConfig(
            UiRenderpassName,
            WorldRenderpassName,
            nullptr,
            FVec4(0, 0, 1280, 720), 
            FVec4(0.0f, 0.0f, 0.2f, 1.0f),
            RenderpassClearFlag::None
        )
    };

    RendererConfig RConfig {
        ApplicationName,
        3,
        RpConfig,
        RendererConfig::PFN_Method(this, &Renderer::RegenerateRenderTargets)
    };

    CriticalInit((ptrRenderer = new VulkanAPI(window, RConfig, WindowRenderTargetCount)), "Систему рендеринга не удалось инициализировать. Выключение.");

    // ЗАДАЧА: Узнаем, как их получить, когда определим представления.
    SkyboxRenderpass = ptrRenderer->GetRenderpass(SkyboxRenderpassName);
    SkyboxRenderpass->RenderTargetCount = WindowRenderTargetCount;
    SkyboxRenderpass->targets = MMemory::TAllocate<RenderTarget>(Memory::Array, WindowRenderTargetCount, true);

    WorldRenderpass = ptrRenderer->GetRenderpass(WorldRenderpassName);
    WorldRenderpass->RenderTargetCount = WindowRenderTargetCount;
    WorldRenderpass->targets = MMemory::TAllocate<RenderTarget>(Memory::Array, WindowRenderTargetCount, true);
    
    UiRenderpass = ptrRenderer->GetRenderpass(UiRenderpassName);
    UiRenderpass->RenderTargetCount = WindowRenderTargetCount;
    UiRenderpass->targets = MMemory::TAllocate<RenderTarget>(Memory::Array, WindowRenderTargetCount, true);

    RegenerateRenderTargets();

    // Обновите размеры рендерпасса скайбокса.
    SkyboxRenderpass->RenderArea = FVec4(0, 0, FramebufferWidth, FramebufferHeight);

    // Обновите размеры основного/мирового рендерпасса.
    WorldRenderpass->RenderArea = FVec4(0, 0, FramebufferWidth, FramebufferHeight);

    // Также обновите размеры рендерпасса пользовательского интерфейса.
    UiRenderpass->RenderArea = FVec4(0, 0, FramebufferWidth, FramebufferHeight);

    // Shaders
    Resource ConfigResource;
    ShaderConfig* config = nullptr;

    auto ResourceSystemInst = ResourceSystem::Instance();
    auto ShaderSystemInst = ShaderSystem::GetInstance();
    // Встроенный шейдер скайбокса.
    CriticalInit(
        ResourceSystemInst->Load(BUILTIN_SHADER_NAME_SKYBOX, ResourceType::Shader, nullptr, ConfigResource),
        "Не удалось загрузить встроенный шейдер скайбокса.");
    config = reinterpret_cast<ShaderConfig*>(ConfigResource.data);
    CriticalInit(ShaderSystemInst->Create(config), "Не удалось загрузить встроенный шейдер скайбокса.");
    ResourceSystemInst->Unload(ConfigResource);
    SkyboxShaderID = ShaderSystemInst->GetID(BUILTIN_SHADER_NAME_SKYBOX);

    // Встроенный шейдер материала.
    CriticalInit(
        ResourceSystemInst->Load(BUILTIN_SHADER_NAME_MATERIAL, ResourceType::Shader, nullptr, ConfigResource),
        "Не удалось загрузить встроенный шейдер материала.");
    config = reinterpret_cast<ShaderConfig*>(ConfigResource.data);
    CriticalInit(ShaderSystemInst->Create(config), "Не удалось загрузить встроенный шейдер материала.");
    ResourceSystemInst->Unload(ConfigResource);
    MaterialShaderID = ShaderSystemInst->GetID(BUILTIN_SHADER_NAME_MATERIAL);

    // Встроенный шейдер пользовательского интерфейса.
    CriticalInit(
        ResourceSystemInst->Load(BUILTIN_SHADER_NAME_UI, ResourceType::Shader, nullptr, ConfigResource),
        "Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
    config = reinterpret_cast<ShaderConfig*>(ConfigResource.data);
    CriticalInit(ShaderSystemInst->Create(config), "Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
    ResourceSystemInst->Unload(ConfigResource);
    UIShaderID = ShaderSystemInst->GetID(BUILTIN_SHADER_NAME_UI);
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
        ptrRenderer->RenderTargetDestroy(SkyboxRenderpass->targets[i], true);
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

bool Renderer::DrawFrame(const RenderPacket &packet)
{
    ptrRenderer->FrameNumber++;
    auto RenderViewSystemInst = RenderViewSystem::Instance();

    // Убедитесь, что окно в данный момент не меняет размер, подождав указанное количество кадров после последней операции изменения размера, прежде чем выполнять внутренние обновления.
    if (resizing) {
        FramesSinceResize++;

        // Если требуемое количество кадров прошло с момента изменения размера, продолжайте и выполните фактические обновления.
        if (FramesSinceResize >= 30) {
            f32 width = FramebufferWidth;
            f32 height = FramebufferHeight;
            RenderViewSystemInst->OnWindowResize(width, height);
            ptrRenderer->Resized(width, height);
            FramesSinceResize = 0;
            resizing = false;
        } else {
            // Пропустите рендеринг кадра и попробуйте снова в следующий раз.
            // ПРИМЕЧАНИЕ: Имитация «отрисовки» кадра со скоростью 60 кадров в секунду.
            PlatformSleep(16);
            return true;
        }
    }

    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (ptrRenderer->BeginFrame(packet.DeltaTime)) {
        const u8& AttachmentIndex = ptrRenderer->WindowAttachmentIndexGet();
        
        // Отобразить каждое представление.
        for (u32 i = 0; i < packet.ViewCount; i++) {
            if (!RenderViewSystemInst->OnRender(packet.views[i].view, packet.views[i], ptrRenderer->FrameNumber, AttachmentIndex)) {
                MERROR("Ошибка рендеринга индекса представления %i.", i);
            }
        }

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

void Renderer::DrawGeometry(const GeometryRenderData &data)
{
    ptrRenderer->DrawGeometry(data);
}

bool Renderer::RenderpassBegin(Renderpass *pass, RenderTarget &target)
{
    return ptrRenderer->RenderpassBegin(pass, target);
}

bool Renderer::RenderpassEnd(Renderpass *pass)
{
    return ptrRenderer->RenderpassEnd(pass);
}

Renderpass* Renderer::GetRenderpass(const MString &name)
{
    return ptrRenderer->GetRenderpass(name);
}

bool Renderer::Load(Shader *shader, const ShaderConfig& config, Renderpass* renderpass, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage *stages)
{
    return ptrRenderer->Load(shader, config, renderpass, StageCount, StageFilenames, stages);
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
        ptrRenderer->RenderTargetDestroy(SkyboxRenderpass->targets[i], false);
        ptrRenderer->RenderTargetDestroy(WorldRenderpass->targets[i], false);
        ptrRenderer->RenderTargetDestroy(UiRenderpass->targets[i], false);

        auto WindowTargetTexture = ptrRenderer->WindowAttachmentGet(i);
        auto DepthTargetTexture = ptrRenderer->DepthAttachmentGet();

        // Цели рендеринга скайбокса
        Texture* SkyboxAttachments[1] = {WindowTargetTexture};
        ptrRenderer->RenderTargetCreate(
            1,
            SkyboxAttachments,
            SkyboxRenderpass,
            FramebufferWidth,
            FramebufferHeight,
            SkyboxRenderpass->targets[i]
        );

        // Цели рендеринга мира.
        Texture* attachments[2] = {WindowTargetTexture, DepthTargetTexture};
        ptrRenderer->RenderTargetCreate(
            2,
            attachments,
            WorldRenderpass,
            FramebufferWidth,
            FramebufferHeight,
            WorldRenderpass->targets[i]
        );

        // Цели рендеринга пользовательского интерфейса
        Texture* UiAttachments[1] = {WindowTargetTexture};
        ptrRenderer->RenderTargetCreate(
            1,
            UiAttachments,
            UiRenderpass,
            FramebufferWidth,
            FramebufferHeight,
            UiRenderpass->targets[i]
        );

    }
}

void Renderer::RenderpassCreate(Renderpass *OutRenderpass, f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass)
{
    ptrRenderer->RenderpassCreate(*OutRenderpass, depth, stencil, HasPrevPass, HasNextPass);
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

bool Renderer::IsMultithreaded()
{
    return ptrRenderer->IsMultithreaded();
}

bool Renderer::RenderBufferCreate(RenderBuffer &buffer)
{
    // Создайте внутренний буфер из бэкэнда.
    if (!ptrRenderer->RenderBufferCreateInternal(buffer)) {
        MFATAL("Невозможно создать резервный буфер для рендербуфера. Приложение не может продолжать работу.");
        return false;
    }
    return true;
}

bool Renderer::RenderBufferCreate(RenderBufferType type, u64 TotalSize, bool UseFreelist, RenderBuffer &buffer)
{
    return ptrRenderer->RenderBufferCreate(type, TotalSize, UseFreelist, buffer);
}

void Renderer::RenderBufferDestroy(RenderBuffer &buffer)
{
    // Освободите внутренние ресурсы.
    ptrRenderer->RenderBufferDestroyInternal(buffer);
    buffer.data = nullptr;
}

bool Renderer::RenderBufferBind(RenderBuffer &buffer, u64 offset)
{
    return ptrRenderer->RenderBufferBind(buffer, offset);
}

bool Renderer::RenderBufferUnbind(RenderBuffer &buffer)
{
    return ptrRenderer->RenderBufferUnbind(buffer);
}

void *Renderer::RenderBufferMapMemory(RenderBuffer &buffer, u64 offset, u64 size)
{
    return ptrRenderer->RenderBufferMapMemory(buffer, offset, size);
}

void Renderer::RenderBufferUnmapMemory(RenderBuffer &buffer, u64 offset, u64 size)
{
    ptrRenderer->RenderBufferUnmapMemory(buffer, offset, size);
}

bool Renderer::RenderBufferFlush(RenderBuffer &buffer, u64 offset, u64 size)
{
    return ptrRenderer->RenderBufferFlush(buffer, offset, size);
}

bool Renderer::RenderBufferRead(RenderBuffer &buffer, u64 offset, u64 size, void **OutMemory)
{
    return ptrRenderer->RenderBufferRead(buffer, offset, size, OutMemory);
}

bool Renderer::RenderBufferResize(RenderBuffer &buffer, u64 NewTotalSize)
{
    buffer.Resize(NewTotalSize);

    bool result = ptrRenderer->RenderBufferResize(buffer, NewTotalSize);
    if (result) {
        buffer.TotalSize = NewTotalSize;
    } else {
        MERROR("Не удалось изменить размер внутренних ресурсов буфера визуализации.");
    }
    return result;
}

bool Renderer::RenderBufferLoadRange(RenderBuffer &buffer, u64 offset, u64 size, const void *data)
{
    return ptrRenderer->RenderBufferLoadRange(buffer, offset, size, data);
}

bool Renderer::RenderBufferCopyRange(RenderBuffer &source, u64 SourceOffset, RenderBuffer &dest, u64 DestOffset, u64 size)
{
    return ptrRenderer->RenderBufferCopyRange(source, SourceOffset, dest, DestOffset, size);
}

bool Renderer::RenderBufferDraw(RenderBuffer &buffer, u64 offset, u32 ElementCount, bool BindOnly)
{
    return ptrRenderer->RenderBufferDraw(buffer, offset, ElementCount, BindOnly);
}

void *Renderer::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}
