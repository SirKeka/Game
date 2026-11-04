#include "rendering_system.h"
#include "render_view.h"
#include "renderer_utils.h"
#include "viewport.h"

#include "core/systems_manager.h"
#include "memory/linear_allocator.h"
#include "platform/platform.hpp"
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.hpp"

RenderBuffer *RenderingSystem::GetRenderbuffer(RenderBufferType type)
{
    switch (type) {
    case RenderBufferType::Vertex:
        return &GeometryVertexBuffer;
    case RenderBufferType::Index:
        return &GeometryIndexBuffer;
    default:
        MERROR("Неподдерживаемый тип буфера %u", type);
        return nullptr;
    }
}

void RenderingSystem::Shutdown()
{
    ptrRenderer->WaitForIdle();
    ptrRenderer->RenderBufferDestroy(GeometryVertexBuffer);
    ptrRenderer->RenderBufferDestroy(GeometryIndexBuffer);
    ptrRenderer->Shutdown();
}

void RenderingSystem::OnResized(u16 width, u16 height)
{
    // Отметить как изменение размера и сохранить изменение, но дождаться повторной генерации.
    FramebufferWidth = width;
    FramebufferHeight = height;
    ptrRenderer->Resized(width, height);
}

bool RenderingSystem::PrepareFrame(FrameData& rFrameData)
{
    // Увеличиваем номер кадра.
    ptrRenderer->FrameNumber++;

    // Сбросить индекс отрисовки для этого кадра.
    ptrRenderer->DrawIndex = 0;

    bool result = ptrRenderer->PrepareFrame(rFrameData);

    // Обновляем данные кадра с учетом информации о рендерере.
    const u8& AttachmentIndex = ptrRenderer->GetWindowAttachmentIndex();

    rFrameData.RendererFrameNumber = ptrRenderer->FrameNumber;
    rFrameData.DrawIndex = ptrRenderer->DrawIndex;
    rFrameData.RenderTargetIndex = AttachmentIndex;

    return result;
}

bool RenderingSystem::End(FrameData &rFrameData)
{
    bool result = ptrRenderer->End(rFrameData);
    // Увеличьте индекс отрисовки для этого кадра.
    ptrRenderer->DrawIndex++;
    // Синхронизируйте с ним данные кадра.
    rFrameData.DrawIndex = ptrRenderer->DrawIndex;
    return result;
}

bool RenderingSystem::Present(const FrameData &rFrameData)
{
    // Завершите кадр. Если это не удастся, восстановление, скорее всего, невозможно.
    bool result = ptrRenderer->Present(rFrameData);

    if (!result) {
        MERROR("Ошибка RenderingSystem::Present. Приложение завершает работу...");
    }
    return result;
}

void RenderingSystem::BeginDebugLabel(const char *LabelText, FVec3 colour)
{
#ifdef _DEBUG
    if (ptrRenderer) {
        ptrRenderer->BeginDebugLabel(LabelText, colour);
    }
#endif
}

void RenderingSystem::EndDebugLabel()
{
#ifdef _DEBUG
    if (ptrRenderer) {
        ptrRenderer->EndDebugLabel();
    }
#endif
}

bool RenderingSystem::Load(Geometry *geometry)
{
    // Проверьте, не повторная ли это загрузка. Если это так, необходимо впоследствии освободить старые данные.
    bool IsReupload = geometry->generation != INVALID::U16ID;
    u64 VertexSize = (u64)(geometry->VertexElementSize * geometry->VertexCount);
    u64 VertexOffset = 0;
    u64 IndexSize = (u64)(geometry->IndexElementSize * geometry->IndexCount);
    u64 IndexOffset = 0;

    // Данные вершин.
    if (!IsReupload) {
        // Выделить место в буфере.
        if (!GeometryVertexBuffer.Allocate(VertexSize, geometry->VertexBufferOffset)) {
            MERROR("VulkanAPI::Load не удалось выделить память из буфера вершин!");
            return false;
        }
    }

    // Загрузите данные.
    // ЗАДАЧА: Передача значения `false` в данном случае приводит к ожиданию в очереди, поэтому эту задачу следует перенести в другую очередь.
    if (!ptrRenderer->RenderBufferLoadRange(GeometryVertexBuffer, geometry->VertexBufferOffset + VertexOffset, VertexSize, (u8*)geometry->vertices + VertexOffset, false)) {
        MERROR("VulkanAPI::Load не удалось загрузить данные в буфер вершин!");
        return false;
    }

    // Данные индексов, если применимо.
    if (geometry->IndexCount && geometry->indices && IndexSize) {
        if (!IsReupload) {
            // Выделите место в буфере.
            if (!GeometryIndexBuffer.Allocate(IndexSize, geometry->IndexBufferOffset)) {
                MERROR("VulkanAPI::Load не удалось выделить данные из буфера индекса!");
                return false;
            }
        }

        // Загрузите данные.
        // ЗАДАЧА: Передача значения `false` в данном случае приводит к ожиданию в очереди, поэтому эту задачу следует перенести в другую очередь.
        if (!ptrRenderer->RenderBufferLoadRange(GeometryIndexBuffer, geometry->IndexBufferOffset + IndexOffset, IndexSize, (u8*)geometry->indices + IndexOffset, false)) {
            MERROR("VulkanAPI::Load не удалось загрузить данные в буфер индекса!");
            return false;
        }
    }

    geometry->generation++;

    return true;
}

void RenderingSystem::GeometryVertexUpdate(Geometry *geometry, u32 offset, bool IncludeInFrameWorkload)
{
    // u32 TotalSize = VertexCount * geometry->VertexElementSize;

    // Загрузить данные.
    if (!ptrRenderer->RenderBufferLoadRange(GeometryVertexBuffer, geometry->VertexBufferOffset + offset, geometry->VertexCount * geometry->VertexElementSize, (u8*)geometry->vertices + offset, IncludeInFrameWorkload)) {
        MERROR("VulkanAPI::GeometryVertexUpdate не удалось загрузить в буфер вершин!");
    }
}

void RenderingSystem::Unload(Geometry *geometry)
{
        if (geometry->generation != INVALID::U16ID) {
        // Бесплатные данные о вершинах
        u64 VertexDataSize = geometry->VertexElementSize * geometry->VertexCount;
        if (VertexDataSize) {
            if (!GeometryVertexBuffer.Free(VertexDataSize, geometry->VertexBufferOffset)) {
                MERROR("RenderingSystem::Unload - не удалось освободить диапазон буфера вершин геометрии.");
            }
        }

        // Бесплатные данные об индексах, если применимо
        u64 IndexDataSize = geometry->IndexElementSize * geometry->IndexCount;
        if (IndexDataSize) {
            if (!GeometryIndexBuffer.Free(IndexDataSize, geometry->IndexBufferOffset)) {
                MERROR("RenderingSystem::Unload  не удалось освободить диапазон буфера индексов геометрии.");
            }
        }

        geometry->generation = INVALID::U16ID;
    }

    if (geometry->vertices) {
        MemorySystem::Free(geometry->vertices, geometry->VertexElementSize * geometry->VertexCount, Memory::Renderer);
    }
    if (geometry->indices) {
        MemorySystem::Free(geometry->indices, geometry->IndexElementSize * geometry->IndexCount, Memory::Renderer);
    }
}

void RenderingSystem::DrawGeometry(const GeometryRenderData &data)
{
    // Игнорировать незагруженные геометрии.
    if (!data.VertexCount) {
        return;
    }

    bool IncludesIndexData = data.IndexCount > 0;
    if (!RenderBufferDraw(GeometryVertexBuffer, data.VertexBufferOffset, data.VertexCount, IncludesIndexData)) {
        MERROR("VulkanAPI::DrawGeometry не удалось отрисовать буфер вершин.");
        return;
    }

    if (IncludesIndexData) {
        if (!RenderBufferDraw(GeometryIndexBuffer, data.IndexBufferOffset, data.IndexCount, !IncludesIndexData)) {
            MERROR("VulkanAPI::DrawGeometry не удалось отрисовать буфер индексов.");
            return;
        }
    }
}

bool RenderingSystem::ShaderCreate(Shader *shader, const ShaderConfig &config, Renderpass *renderpass)
{
    // Получите количество униформ.
    shader->GlobalUniformCount = 0;
    // Количество сэмплеров в шейдере за кадр. НЕ количество необходимых дескрипторов (т.е. это может быть массив).
    shader->GlobalUniformSamplerCount = 0;
    // shader->GlobalSamplerIndices = darray_create(u32);
    shader->InstanceUniformCount = 0;
    // Количество сэмплеров в шейдере на экземпляр за кадр. НЕ количество необходимых дескрипторов (т.е. это может быть массив).
    shader->InstanceUniformSamplerCount = 0;
    // shader->InstanceSamplerIndices = darray_create(u32);
    shader->LocalUniformCount = 0;
    shader->ShaderStageCount = config.StageConfigs.Length();

    // Изучите униформы и определите область видимости, а также количество сэмплеров.
    u32 TotalCount = config.uniforms.Length();
    for (u32 i = 0; i < TotalCount; ++i) {
        const ShaderUniformConfig& uniform = config.uniforms[i];
        switch (uniform.scope) {
            case ShaderScope::Global:
                if (UniformTypeIsSampler(uniform.type)) {
                    shader->GlobalUniformSamplerCount++;
                    shader->GlobalSamplerIndices.PushBack(i);
                } else {
                    shader->GlobalUniformCount++;
                }
                break;
            case ShaderScope::Instance:
                if (UniformTypeIsSampler(uniform.type)) {
                    shader->InstanceUniformSamplerCount++;
                    shader->InstanceSamplerIndices.PushBack(i);
                } else {
                    shader->InstanceUniformCount++;
                }
                break;
            case ShaderScope::Local:
                shader->LocalUniformCount++;
                break;
        }
    }

    // Проанализируйте этапы шейдеров и загрузите исходный код шейдеров по мере необходимости. 
    // Затем этот исходный код передается на бэкэнд-рендерер, который подготавливает необходимые ресурсы шейдерной программы.
    // ЗАДАЧА: Здесь, на этом уровне, следует реализовать директивы #include, чтобы обработка была одинаковой независимо от используемого бэкенда.

    CreateArray(shader->StageConfigs, sizeof(ShaderStageConfig) * shader->ShaderStageCount, true);
    CreateArray(shader->ModuleWatchIDs, sizeof(u32) * shader->ShaderStageCount, true);
    // Каждый этап.
    for (u8 i = 0; i < shader->ShaderStageCount; ++i) {
        ShaderStageConfig& stConfig = shader->StageConfigs[i];
        stConfig.stage = config.StageConfigs[i].stage;
        stConfig.filename = (MString&&)config.StageConfigs[i].filename;
        // Ознакомьтесь с материалом.
        TextResource textResource;
        if (!ResourceSystem::Load(stConfig.filename.c_str(), nullptr, textResource)) {
            MERROR("Не удалось прочитать файл шейдера: %s.", stConfig.filename.c_str());
            return false;
        }
        // Восстановите исходный код и его длину, а затем освободите ресурс.
        stConfig.source = (MString&&)textResource.data;
        // ЗАДАЧА: Здесь, на этом уровне, следует реализовать рекурсивные директивы #include, 
        // чтобы обработка была одинаковой независимо от используемого бэкенда.
        // Это должно рекурсивно заменять директивы #include содержимым файла на месте и 
        // корректировать длину исходного кода по ходу выполнения.
#ifdef _DEBUG
        // Разрешить «горячую» перезагрузку шейдеров в отладочных сборках.
        if (!PlatformWatchFile(textResource.FullPath.c_str(), shader->ModuleWatchIDs[i])) {
            // В случае сбоя следует выводить предупреждение, но аварийное завершение работы не требуется.
            MWARN("Не удалось отследить исходный файл шейдера '%s'.", textResource.FullPath.c_str());
        }
#endif
    }

    return ptrRenderer->ShaderCreate(shader, config, renderpass);
}

void RenderingSystem::ShaderDestroy(Shader *shader)
{
#ifdef _DEBUG
    if (shader->ModuleWatchIDs) {
        // Прекратить отслеживание файлов шейдеров.
        for (u8 i = 0; i < shader->ShaderStageCount; ++i) {
            PlatformUnwatchFile(shader->ModuleWatchIDs[i]);
        }
    }
#endif
    ptrRenderer->ShaderDestroy(shader);
}

bool RenderingSystem::ShaderReload(Shader *shader)
{
    // Изучите этапы работы шейдеров и при необходимости загрузите их исходный код. 
    // Затем этот код передается в бэкенд-компонент рендеринга, 
    // который по мере необходимости создает соответствующие ресурсы шейдерных программ.
    // ЗАДАЧА: Реализовать директивы #include на этом уровне, чтобы их обработка была единообразной независимо от используемого бэкенда.

    // Создайте копию конфигураций этапа на случай, если какой-либо файл не удастся загрузить.
    bool HasError = false;
    ShaderStageConfig* NewStageConfigs = AllocateTemp<ShaderStageConfig>(sizeof(ShaderStageConfig) * shader->ShaderStageCount);
    for (u8 i = 0; i < shader->ShaderStageCount; ++i) {
        ShaderStageConfig& config = shader->StageConfigs[i];
        // Ознакомьтесь с ресурсом.
        TextResource textResource;
        if (!ResourceSystem::Load(config.filename.c_str(), nullptr, textResource)) {
            MERROR("Не удалось прочитать файл шейдера: %s.", config.filename.c_str());
            HasError = true;
            break;
        }

        // Скопируйте источник и длину, а затем освободите ресурс.
        NewStageConfigs[i].source = (MString&&)textResource.data;
        // ЗАДАЧА: Реализуйте здесь, на этом уровне, рекурсивную обработку директив #include, чтобы она выполнялась единообразно независимо от используемого бэкенда.
        // Это должно рекурсивно заменять директивы `#include` содержимым соответствующих файлов непосредственно в тексте и по ходу дела корректировать его длину.

        // ResourceSystem::Unload(textResource);
    }

    for (u8 i = 0; i < shader->ShaderStageCount; ++i) {
        if (HasError) {
            if (NewStageConfigs[i].source) {
                NewStageConfigs[i].source.Destroy();
            }
        } else {
            shader->StageConfigs[i].source = NewStageConfigs[i].source;
        }
    }

    if (HasError) {
        return false;
    }

    return ptrRenderer->ShaderReload(shader);
}

bool RenderingSystem::ShaderSetWireframe(Shader *shader, bool WireframeEnabled)
{
    // Перед внесением изменений убедитесь, что этот шейдер поддерживает режим отображения каркаса (wireframe).
    if (!ptrRenderer->ShaderSupportsWireframe(shader)) {
        // Не поддерживается, не включайте. Можете по этому поводу поныть.
        MWARN("Шейдер не поддерживает каркасный режим: «%s».", shader->name.c_str());
        return false;
    }
    shader->IsWireframe = WireframeEnabled;
    return true;
}

bool RenderingSystem::RenderBufferCreate(RenderBuffer &buffer)
{
    auto pRenderingSystem = SystemsManager::GetRenderingSystem();
    // Создайте внутренний буфер из бэкэнда.
    if (!pRenderingSystem->ptrRenderer->RenderBufferCreate(buffer)) {
        MFATAL("Невозможно создать резервный буфер для рендербуфера. Приложение не может продолжать работу.");
        return false;
    }
    return true;
}

bool RenderingSystem::RenderbufferClear(RenderBuffer &buffer, bool ZeroMemory)
{
    if (buffer.FreelistMemoryRequirement != 0) {
        buffer.BufferFreelist.Clear();
    }

    if (ZeroMemory) {
        // ЗАДАЧА: обнуление памяти
        MFATAL("ЗАДАЧА: Обнуление памяти");
        return false;
    }

    return true;
}

void RenderingSystem::RenderBufferDestroy(RenderBuffer &buffer)
{
    // Освободите внутренние ресурсы.
    ptrRenderer->RenderBufferDestroy(buffer);
    buffer.data = nullptr;
}

bool RenderingSystem::RenderBufferResize(RenderBuffer &buffer, u64 NewTotalSize)
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

void RenderingSystem::SetActiveViewport(Viewport *viewvport)
{
    auto pRenderingSystem = SystemsManager::GetRenderingSystem();
    auto Renderer = pRenderingSystem->ptrRenderer;
    pRenderingSystem->ActiveViewport = viewvport;

    auto& rect = viewvport->rect;
    Renderer->SetViewport(Rect2D(rect.x, rect.y + rect.height, rect.width, -rect.height));

    Renderer->SetScissor(Rect2D(rect.x, rect.y, rect.width, rect.height));
}
