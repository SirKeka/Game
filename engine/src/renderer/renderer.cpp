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

#include <new>

RendererType *Renderer::ptrRenderer = nullptr;

Renderer::Renderer(MWindow *window, const char *ApplicationName, ERendererType type, LinearAllocator& SystemAllocator)
:  
WindowRenderTargetCount(),
// Размер буфера кадра по умолчанию. Переопределяется при создании окна.
FramebufferWidth(1280),
FramebufferHeight(720),
resizing(false),
FramesSinceResize()
{    
    RendererConfig RConfig { ApplicationName };
    void* ptrMem = SystemAllocator.Allocate(sizeof(VulkanAPI));
    if (!(ptrRenderer = new(ptrMem) VulkanAPI(window, RConfig, WindowRenderTargetCount))) {
        MERROR("Систему рендеринга не удалось инициализировать. Выключение.");
    }
}

Renderer::~Renderer()
{
    delete ptrRenderer;
    ptrRenderer = nullptr;
}

void Renderer::Shutdown()
{}

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

    // Убедитесь, что окно в данный момент не меняет размер, подождав указанное количество кадров после последней операции изменения размера, прежде чем выполнять внутренние обновления.
    if (resizing) {
        FramesSinceResize++;

        // Если требуемое количество кадров прошло с момента изменения размера, продолжайте и выполните фактические обновления.
        if (FramesSinceResize >= 30) {
            f32 width = FramebufferWidth;
            f32 height = FramebufferHeight;
            RenderViewSystem::OnWindowResize(width, height);
            ptrRenderer->Resized(width, height);
            // Уведомить пользователей об изменении размера.
            RenderViewSystem::OnWindowResize(width, height);
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
            if (!RenderViewSystem::OnRender(packet.views[i].view, packet.views[i], ptrRenderer->FrameNumber, AttachmentIndex)) {
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

void Renderer::TextureReadData(Texture *texture, u32 offset, u32 size, void **OutMemory)
{
    ptrRenderer->TextureReadData(texture, offset, size, OutMemory);
}

void Renderer::TextureReadPixel(Texture *texture, u32 x, u32 y, u8 **OutRgba)
{
    ptrRenderer->TextureReadPixel(texture, x, y, OutRgba);
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

void Renderer::ViewportSet(const FVec4 &rect)
{
    ptrRenderer->ViewportSet(rect);
}

void Renderer::ViewportReset()
{
    ptrRenderer->ViewportReset();
}

void Renderer::ScissorSet(const FVec4 &rect)
{
    ptrRenderer->ScissorSet(rect);
}

void Renderer::ScissorReset()
{
    ptrRenderer->ScissorReset();
}

bool Renderer::RenderpassBegin(Renderpass *pass, RenderTarget &target)
{
    return ptrRenderer->RenderpassBegin(pass, target);
}

bool Renderer::RenderpassEnd(Renderpass *pass)
{
    return ptrRenderer->RenderpassEnd(pass);
}

bool Renderer::Load(Shader *shader, const Shader::Config& config, Renderpass* renderpass, u8 StageCount, const DArray<MString>& StageFilenames, const Shader::Stage *stages)
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

bool Renderer::SetUniform(Shader *shader, Shader::Uniform *uniform, const void *value)
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

void Renderer::RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment *attachments, Renderpass *pass, u32 width, u32 height, RenderTarget &OutTarget)
{
    ptrRenderer->RenderTargetCreate(AttachmentCount, attachments, pass, width, height, OutTarget);
}

void Renderer::RenderTargetDestroy(RenderTarget &target, bool FreeInternalMemory)
{
    ptrRenderer->RenderTargetDestroy(target, FreeInternalMemory);
}

bool Renderer::RenderpassCreate(const RenderpassConfig &config, Renderpass &OutRenderpass)
{
    return ptrRenderer->RenderpassCreate(config, OutRenderpass);
}

void Renderer::RenderpassDestroy(Renderpass *OutRenderpass)
{
    ptrRenderer->RenderpassDestroy(OutRenderpass);
}

Texture *Renderer::WindowAttachmentGet(u8 index)
{
    return ptrRenderer->WindowAttachmentGet(index);
}

Texture *Renderer::DepthAttachmentGet(u8 index)
{
    return ptrRenderer->DepthAttachmentGet(index);
}

u8 Renderer::WindowAttachmentIndexGet()
{
    return ptrRenderer->WindowAttachmentIndexGet();
}

u8 Renderer::WindowAttachmentCountGet()
{
    return ptrRenderer->WindowAttachmentCountGet();
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
