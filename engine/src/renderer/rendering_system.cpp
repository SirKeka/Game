#include "rendering_system.h"
#include "memory/linear_allocator.h"
#include "platform/platform.hpp"
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.hpp"
#include "systems/render_view_system.h"
#include "render_view.h"
#include "viewport.h"
#include "core/mvar.h"
#include "core/systems_manager.hpp"

#include <new>

#include "core/metrics.h"

struct sRenderingSystem
{
    RendererPlugin* ptrRenderer;
    /// @brief Текущая ширина буфера кадра окна.
    u32 FramebufferWidth;
    /// @brief Текущая высота буфера кадра окна.
    u32 FramebufferHeight;
    u8 WindowRenderTargetCount;   // Количество целей рендеринга. Обычно совпадает с количеством изображений swapchain.
    bool resizing;                // Указывает, изменяется ли размер окна в данный момент.
    u8 FramesSinceResize;         // Текущее количество кадров с момента последней операции изменения размера. Устанавливается только если resizing = true. В противном случае 0.
    Viewport* ActiveViewport;

    constexpr sRenderingSystem(RendererPlugin* plugin) : ptrRenderer(plugin), FramebufferWidth(1280), FramebufferHeight(720), WindowRenderTargetCount(), resizing(false), FramesSinceResize(), ActiveViewport(nullptr) {}
};

bool RenderingSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<RenderingSystemConfig*>(config);

    MemoryRequirement = sizeof(sRenderingSystem);
    
    if (!memory) {
        return true;
    }

    auto pRenderingSystem = new(memory) sRenderingSystem(pConfig->plugin);

    RenderingConfig RConfig;
    RConfig.ApplicationName = pConfig->ApplicationName;
    // ЗАДАЧА: предоставить это приложению для настройки.
    RConfig.flags = VsyncEnabledBit | PowerSavingBit;

    // Создайте vsync mvar
    MVar::CreateInt("vsync", (RConfig.flags & VsyncEnabledBit) ? 1 : 0);

    pRenderingSystem->ptrRenderer->Initialize(RConfig, pRenderingSystem->WindowRenderTargetCount);

    if (!pRenderingSystem && !pRenderingSystem->ptrRenderer) {
        MERROR("Систему рендеринга не удалось инициализировать. Выключение.");
        return false;
    }
    return true;
}

void RenderingSystem::Shutdown()
{}

void RenderingSystem::OnResized(u16 width, u16 height)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    if (pRenderingSystem) {
        // Отметить как изменение размера и сохранить изменение, но дождаться повторной генерации.
        pRenderingSystem->resizing = true;
        pRenderingSystem->FramebufferWidth = width;
        pRenderingSystem->FramebufferHeight = height;
        // Также сбросить количество кадров с момента последней операции изменения размера.
        pRenderingSystem->FramesSinceResize = 0;
    } else {
        MWARN("Средства визуализации (Renderer) не существует, чтобы принять изменение размера: %i %i", width, height);
    }
}

bool RenderingSystem::PrepareFrame(FrameData& rFrameData)
{
    Metrics::BeginFunction("RenderingSystem::PrepareFrame");
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    auto& plugin = pRenderingSystem->ptrRenderer;

    // Увеличиваем номер кадра.
    plugin->FrameNumber++;

    // Сбросить индекс отрисовки для этого кадра.
    plugin->DrawIndex = 0;

    // Убедитесь, что окно в данный момент не меняет размер, подождав указанное количество кадров после последней операции изменения размера, прежде чем выполнять внутренние обновления.
    if (pRenderingSystem->resizing) {
        pRenderingSystem->FramesSinceResize++;

        // Если требуемое количество кадров прошло с момента изменения размера, продолжайте и выполните фактические обновления.
        if (pRenderingSystem->FramesSinceResize >= 30) {
            f32 width = pRenderingSystem->FramebufferWidth;
            f32 height = pRenderingSystem->FramebufferHeight;
            RenderViewSystem::OnWindowResize(width, height);
            pRenderingSystem->ptrRenderer->Resized(width, height);
            // Уведомить пользователей об изменении размера.
            RenderViewSystem::OnWindowResize(width, height);
            pRenderingSystem->FramesSinceResize = 0;
            pRenderingSystem->resizing = false;
        } else {
            // Пропустите рендеринг кадра и попробуйте снова в следующий раз.
            // ПРИМЕЧАНИЕ: Имитация «отрисовки» кадра со скоростью 60 кадров в секунду.
            PlatformSleep(16);
            return false;
        }
    }

    bool result = plugin->PrepareFrame(rFrameData);

    // Обновляем данные кадра с учетом информации о рендерере.
    const u8& AttachmentIndex = plugin->WindowAttachmentIndexGet();

    rFrameData.RendererFrameNumber = plugin->FrameNumber;
    rFrameData.DrawIndex = plugin->DrawIndex;
    rFrameData.RenderTargetIndex = AttachmentIndex;

    Metrics::EndFunction("RenderingSystem::PrepareFrame");
    return result;
}

bool RenderingSystem::Begin(const FrameData &rFrameData)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->Begin(rFrameData);
}

bool RenderingSystem::End(FrameData &rFrameData)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    auto plugin = pRenderingSystem->ptrRenderer;
    bool result = plugin->End(rFrameData);
    // Увеличьте индекс отрисовки для этого кадра.
    plugin->DrawIndex++;
    // Синхронизируйте с ним данные кадра.
    rFrameData.DrawIndex = plugin->DrawIndex;
    return result;
}

bool RenderingSystem::Present(const FrameData &rFrameData)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    // Завершите кадр. Если это не удастся, восстановление, скорее всего, невозможно.
    bool result = pRenderingSystem->ptrRenderer->Present(rFrameData);

    if (!result) {
        MERROR("Ошибка RenderingSystem::Present. Приложение завершает работу...");
    }
    return result;
}

void RenderingSystem::Load(const u8 *pixels, Texture *texture)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->Load(pixels, texture);
}

void RenderingSystem::LoadTextureWriteable(Texture *texture)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->LoadTextureWriteable(texture);
}

void RenderingSystem::TextureResize(Texture *texture, u32 NewWidth, u32 NewHeight)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->TextureResize(texture, NewWidth, NewHeight);
}

void RenderingSystem::TextureWriteData(Texture *texture, u32 offset, u32 size, const u8 *pixels)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->TextureWriteData(texture, offset, size, pixels);
}

void RenderingSystem::TextureReadData(Texture *texture, u32 offset, u32 size, void **OutMemory)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->TextureReadData(texture, offset, size, OutMemory);
}

void RenderingSystem::TextureReadPixel(Texture *texture, u32 x, u32 y, u8 **OutRgba)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->TextureReadPixel(texture, x, y, OutRgba);
}

void *RenderingSystem::TextureCopyData(const Texture *texture)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->TextureCopyData(texture);
}

void RenderingSystem::Unload(Texture *texture)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->Unload(texture);
}

bool RenderingSystem::CreateGeometry(Geometry *geometry, u32 VertexSize, u32 VertexCount, const void *vertices, u32 IndexSize, u32 IndexCount, const void *indices)
{
    if (!geometry) {
        MERROR("Для RenderingSystem::Load требуется действительный указатель на геометрию.");
        return false;
    }

    if (!VertexCount || !vertices) {
        MERROR("RenderingSystem::Load требуются данные о вершинах, но они не были предоставлены. VertexCount=%d, vertices=%p", VertexCount, vertices);
        return false;
    }

    geometry->material = nullptr;

    // Сделайте идентификаторы недействительными. ПРИМЕЧАНИЕ: Не делайте идентификатор геометрии недействительным! На этом этапе у него должен быть действительный идентификатор, и его отмена может привести к хаосу.
    geometry->InternalID = INVALID::ID;
    geometry->generation = INVALID::U16ID;

    // Сделайте копию данных вершин.
    geometry->VertexCount = VertexCount;
    geometry->VertexElementSize = VertexSize;
    geometry->vertices = MemorySystem::Allocate(VertexSize * VertexCount, Memory::Renderer);
    MemorySystem::CopyMem(geometry->vertices, vertices, VertexSize * VertexCount);

    geometry->IndexCount = IndexCount;
    geometry->IndexElementSize = IndexSize;
    geometry->indices = nullptr;
    // Если есть, сделайте копию данных индекса.
    if (IndexSize && IndexCount) {
        geometry->indices = MemorySystem::Allocate(IndexSize * IndexCount, Memory::Renderer);
        MemorySystem::CopyMem(geometry->indices, indices, IndexSize * IndexCount);
    }

    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->CreateGeometry(geometry);
}

MAPI bool RenderingSystem::Load(Geometry *geometry)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->Load(geometry, 0, geometry->VertexElementSize * geometry->VertexCount, 0, geometry->IndexElementSize * geometry->IndexCount);
}

void RenderingSystem::GeometryVertexUpdate(Geometry *geometry, u32 offset, u32 VertexCount, void *vertices)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->GeometryVertexUpdate(geometry, offset, VertexCount, vertices);
}

void RenderingSystem::Unload(Geometry *geometry)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->Unload(geometry);
}

void RenderingSystem::DrawGeometry(const GeometryRenderData &data)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->DrawGeometry(data);
}

void RenderingSystem::ViewportSet(const Rect2D &rect)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->SetViewport(rect);
}

void RenderingSystem::ViewportReset()
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->ViewportReset();
}

void RenderingSystem::ScissorSet(const Rect2D &rect)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->SetScissor(rect);
}

void RenderingSystem::ScissorReset()
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->ScissorReset();
}

void RenderingSystem::SetWinding(RendererWinding winding)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->SetWinding(winding);
}

bool RenderingSystem::RenderpassBegin(Renderpass *pass, RenderTarget &target)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderpassBegin(pass, target);
}

bool RenderingSystem::RenderpassEnd(Renderpass *pass)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderpassEnd(pass);
}

bool RenderingSystem::Load(Shader *shader, const ShaderConfig& config, Renderpass* renderpass, const DArray<Shader::Stage>& stages, const DArray<MString>& StageFilenames)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->Load(shader, config, renderpass, stages, StageFilenames);
}

void RenderingSystem::Unload(Shader *shader)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->Unload(shader);
}

bool RenderingSystem::ShaderInitialize(Shader *shader)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->ShaderInitialize(shader);
}

bool RenderingSystem::ShaderUse(Shader *shader)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->ShaderUse(shader);
}

bool RenderingSystem::ShaderApplyGlobals(Shader *shader, bool NeedsUpdate)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->ShaderApplyGlobals(shader, NeedsUpdate);
}

bool RenderingSystem::ShaderApplyInstance(Shader *shader, bool NeedsUpdate)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->ShaderApplyInstance(shader, NeedsUpdate);
}

bool RenderingSystem::ShaderAcquireInstanceResources(Shader *shader, u32 TextureMapCount, TextureMap **maps, u32 &OutInstanceID)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->ShaderAcquireInstanceResources(shader, TextureMapCount, maps, OutInstanceID);
}

bool RenderingSystem::ShaderBindInstance(Shader *shader, u32 InstanceID)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->ShaderBindInstance(shader, InstanceID);
}

bool RenderingSystem::ShaderReleaseInstanceResources(Shader *shader, u32 InstanceID)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->ShaderReleaseInstanceResources(shader, InstanceID);
}

bool RenderingSystem::SetUniform(Shader *shader, Shader::Uniform *uniform, const void *value)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->SetUniform(shader, uniform, value);
}

bool RenderingSystem::TextureMapAcquireResources(TextureMap *map)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->TextureMapAcquireResources(map);
}

void RenderingSystem::TextureMapReleaseResources(TextureMap *map)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->TextureMapReleaseResources(map);
}

void RenderingSystem::RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment *attachments, Renderpass *pass, u32 width, u32 height, RenderTarget &OutTarget)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->RenderTargetCreate(AttachmentCount, attachments, pass, width, height, OutTarget);
}

void RenderingSystem::RenderTargetDestroy(RenderTarget &target, bool FreeInternalMemory)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->RenderTargetDestroy(target, FreeInternalMemory);
}

bool RenderingSystem::RenderpassCreate(RenderpassConfig &config, Renderpass &OutRenderpass, bool copy)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderpassCreate(config, OutRenderpass, copy);
}

void RenderingSystem::RenderpassDestroy(Renderpass *OutRenderpass)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->RenderpassDestroy(OutRenderpass);
}

Texture *RenderingSystem::WindowAttachmentGet(u8 index)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->WindowAttachmentGet(index);
}

Texture *RenderingSystem::DepthAttachmentGet(u8 index)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->DepthAttachmentGet(index);
}

u8 RenderingSystem::WindowAttachmentIndexGet()
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->WindowAttachmentIndexGet();
}

u8 RenderingSystem::WindowAttachmentCountGet()
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->WindowAttachmentCountGet();
}

const bool& RenderingSystem::IsMultithreaded()
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->IsMultithreaded();
}

bool RenderingSystem::FlagEnabled(RendererConfigFlags flag)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->FlagEnabled(flag);
}

void RenderingSystem::FlagSetEnabled(RendererConfigFlags flag, bool enabled)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->FlagSetEnabled(flag, enabled);
}

bool RenderingSystem::RenderBufferCreate(RenderBuffer &buffer)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    // Создайте внутренний буфер из бэкэнда.
    if (!pRenderingSystem->ptrRenderer->RenderBufferCreateInternal(buffer)) {
        MFATAL("Невозможно создать резервный буфер для рендербуфера. Приложение не может продолжать работу.");
        return false;
    }
    return true;
}

bool RenderingSystem::RenderBufferCreate(const char* name, RenderBufferType type, u64 TotalSize, bool UseFreelist, RenderBuffer &buffer)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferCreate(name, type, TotalSize, UseFreelist, buffer);
}

void RenderingSystem::RenderBufferDestroy(RenderBuffer &buffer)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    // Освободите внутренние ресурсы.
    pRenderingSystem->ptrRenderer->RenderBufferDestroyInternal(buffer);
    buffer.data = nullptr;
}

bool RenderingSystem::RenderBufferBind(RenderBuffer &buffer, u64 offset)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferBind(buffer, offset);
}

bool RenderingSystem::RenderBufferUnbind(RenderBuffer &buffer)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferUnbind(buffer);
}

void *RenderingSystem::RenderBufferMapMemory(RenderBuffer &buffer, u64 offset, u64 size)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferMapMemory(buffer, offset, size);
}

void RenderingSystem::RenderBufferUnmapMemory(RenderBuffer &buffer, u64 offset, u64 size)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    pRenderingSystem->ptrRenderer->RenderBufferUnmapMemory(buffer, offset, size);
}

bool RenderingSystem::RenderBufferFlush(RenderBuffer &buffer, u64 offset, u64 size)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferFlush(buffer, offset, size);
}

bool RenderingSystem::RenderBufferRead(RenderBuffer &buffer, u64 offset, u64 size, void **OutMemory)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferRead(buffer, offset, size, OutMemory);
}

bool RenderingSystem::RenderBufferResize(RenderBuffer &buffer, u64 NewTotalSize)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    buffer.Resize(NewTotalSize);

    bool result = pRenderingSystem->ptrRenderer->RenderBufferResize(buffer, NewTotalSize);
    if (result) {
        buffer.TotalSize = NewTotalSize;
    } else {
        MERROR("Не удалось изменить размер внутренних ресурсов буфера визуализации.");
    }
    return result;
}

bool RenderingSystem::RenderBufferLoadRange(RenderBuffer &buffer, u64 offset, u64 size, const void *data)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferLoadRange(buffer, offset, size, data);
}

bool RenderingSystem::RenderBufferCopyRange(RenderBuffer &source, u64 SourceOffset, RenderBuffer &dest, u64 DestOffset, u64 size)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferCopyRange(source, SourceOffset, dest, DestOffset, size);
}

bool RenderingSystem::RenderBufferDraw(RenderBuffer &buffer, u64 offset, u32 ElementCount, bool BindOnly)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ptrRenderer->RenderBufferDraw(buffer, offset, ElementCount, BindOnly);
}

Viewport *RenderingSystem::GetActiveViewport()
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    return pRenderingSystem->ActiveViewport;
}

void RenderingSystem::SetActiveViewport(Viewport *viewvport)
{
    auto pRenderingSystem = reinterpret_cast<sRenderingSystem*>(SystemsManager::GetState(MSystem::Type::Renderer));
    auto Renderer = pRenderingSystem->ptrRenderer;
    pRenderingSystem->ActiveViewport = viewvport;

    auto& rect = viewvport->rect;
    Rect2D ViewportRect {rect.x, rect.y + rect.height, rect.width, -rect.height};
    Renderer->SetViewport(ViewportRect);

    Rect2D ScissorRect {rect.x, rect.y, rect.width, rect.height};
    Renderer->SetScissor(ScissorRect);
}
