#include "render_view_wireframe.h"
#include <core/systems_manager.h>
#include <memory/linear_allocator.h>
#include <renderer/camera.h>
#include <renderer/rendering_system.h>
#include <renderer/renderpass.h>
#include <renderer/viewport.h>
#include <resources/shader.h>
#include <systems/resource_system.h>
#include <systems/shader_system.h>

struct WireframeShaderLocations {
    u16 projection;
    u16 view;
    u16 model;
    u16 colour;
};

struct WireframeColoureInstance
{
    u32 id;
    u64 FrameNumber;
    u8 DrawIndex;
    FVec4 colour;
};

struct WireframeShaderInfo {
    Shader* shader;
    WireframeShaderLocations locations;
    // Выбран один экземпляр каждого цвета.
    WireframeColoureInstance NormalInstance;
    WireframeColoureInstance SelectedInstance;
};

struct sRenderViewWireframe
{
    u32 SelectedID{INVALID::ID};
    WireframeShaderInfo MeshShader{};
    WireframeShaderInfo TerrainShader{};

    void* operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Renderer);
    }
    void operator delete(void* ptr, u64 size) {
        MemorySystem::Free(ptr, size, Memory::Renderer);
    }
};

static bool WireframeOnEvent(u16 code, void* sender, void* ListenerInst, const EventContext& context) {
    auto self = (RenderView*)ListenerInst;
    if (!self) {
        return false;
    }
    auto data = (sRenderViewWireframe*)self->data;
    if (!data) {
        return false;
    }

    switch (code) {
        case EventSystem::DefaultRendertargetRefreshRequired:
            // RenderViewSystem::RegenerateRenderTargets(self);
            // Это должно быть использовано другими представлениями, поэтому считайте это _не_ обработанным.
            return false;
    }

    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}

bool RenderViewWireframe::OnRegistered(RenderView *self)
{
    if (!self) {
        return false;
    }

    // Настройка внутренних данных.
    auto data = new sRenderViewWireframe();
    self->data = data;

    const char* ShaderNames[2] = {"Shader.Builtin.Wireframe", "Shader.Builtin.WireframeTerrain"};
    WireframeShaderInfo* ShaderInfos[2] = {&data->MeshShader, &data->TerrainShader};
    FVec4 NormalColours[2] = {FVec4(0.5F, 0.8F, 0.8F, 1.F), FVec4(0.8F, 0.8F, 0.5F, 1.F)};

    for (u32 s = 0; s < 2; ++s) {
        auto info = ShaderInfos[s];
        // Загрузите каркасный шейдер и его местоположение.
        ShaderResource WireframeShaderConfigResource;
        if (!ResourceSystem::Load(ShaderNames[s], nullptr, WireframeShaderConfigResource)) {
            MERROR("Не удалось загрузить встроенный каркасный шейдер.");
            return false;
        }
        auto& WireframeShaderConfig = WireframeShaderConfigResource.data;
        if (!ShaderSystem::CreateShader(self->passes[0], WireframeShaderConfig)) {
            MERROR("Не удалось загрузить встроенный каркасный шейдер.");
            return false;
        }
        // ResourceSystem::Unload(WireframeShaderConfigResource);

        auto shader = info->shader = ShaderSystem::GetShader(ShaderNames[s]);
        info->locations.projection = shader->UniformLocation("projection");
        info->locations.view       = shader->UniformLocation("view");
        info->locations.model      = shader->UniformLocation("model");
        info->locations.colour     = shader->UniformLocation("colour");

        // Приобретите ресурсы экземпляра шейдера.
        // info->NormalInstance = WireframeColoureInstance();
        info->NormalInstance.colour = NormalColours[s];
        auto renderer = SystemsManager::GetRenderingSystem();
        ShaderInstanceResourceConfig InstanceResourceConfig{
        .UniformConfigCount = 0,  // ПРИМЕЧАНИЕ: текстур нет, поэтому это не имеет значения.
        .UniformConfigs = nullptr
        };

        if (!renderer->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, info->NormalInstance.id)) {
            MERROR("Не удалось получить ресурсы экземпляра геометрического шейдера из каркасного шейдера.");
            return false;
        }

        // info->SelectedInstance = (WireframeColoureInstance){0};
        info->SelectedInstance.colour = FVec4(0.F, 1.F, 0.F, 1.F);
        if (!renderer->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, info->SelectedInstance.id)) {
            MERROR("Не удалось получить выбранные ресурсы экземпляра шейдера из каркасного шейдера.");
            return false;
        }
    }

    // Регистрация союытия.
    if (!EventSystem::Register(EventSystem::DefaultRendertargetRefreshRequired, self, WireframeOnEvent)) {
        MERROR("Не удалось прослушивать требуемое событие обновления. Создание не удалось.");
        return false;
    }

    return true;
}

void RenderViewWireframe::Destroy(RenderView *self)
{
    if (!self) {
        return;
    }

    auto data = (sRenderViewWireframe*)self->data;

    // Отменить регистрацию в событии.
    // EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, self, EditorWorldOnEvent);

    // Освободить ресурсы экземпляра шейдера.
    auto renderer = SystemsManager::GetRenderingSystem();
    renderer->ShaderReleaseInstanceResources(data->MeshShader.shader,    data->MeshShader.NormalInstance.id);
    renderer->ShaderReleaseInstanceResources(data->MeshShader.shader,    data->MeshShader.SelectedInstance.id);
    renderer->ShaderReleaseInstanceResources(data->TerrainShader.shader, data->TerrainShader.NormalInstance.id);
    renderer->ShaderReleaseInstanceResources(data->TerrainShader.shader, data->TerrainShader.SelectedInstance.id);

    // Освободить внутреннюю структуру данных.
    delete data;
}

void RenderViewWireframe::Resize(RenderView *self, u32 width, u32 height)
{
    if (self) {
        if (width != self->width || height != self->height) {
            self->width = width;
            self->height = height;
        }
    }
}

bool RenderViewWireframe::BuildPacket(RenderView *self, FrameData &rFrameData, Viewport &viewport, Camera *camera, void *data, RenderViewPacket &OutPacket)
{
    if (!self || !data) {
        MWARN("Для render_view_wireframe_on_packet_build требуется действительный указатель на представление, пакет и данные.");
        return false;
    }

    auto WorldData = (RenderViewWireframeData*)data;
    auto InternalData = (sRenderViewWireframe*)self->data;

    OutPacket.view = self;
    OutPacket.viewport = &viewport;

    // Установить матрицы и т.д.
    OutPacket.ProjectionMatrix = viewport.projection;
    OutPacket.ViewMatrix = camera->GetView();
    OutPacket.ViewPosition = camera->GetPosition();

    // Запомнить текущий выбранный объект.
    InternalData->SelectedID = WorldData->SelectedID;

    // Сбросить индексы отрисовки.
    InternalData->MeshShader.NormalInstance.DrawIndex    = InternalData->MeshShader.SelectedInstance.DrawIndex    = 0;
    InternalData->TerrainShader.NormalInstance.DrawIndex = InternalData->TerrainShader.SelectedInstance.DrawIndex = 0;

    // Геометрия.
    if (*WorldData->WorldGeometries) {
        const u32 GeometryDataCount = WorldData->WorldGeometries->Length();
        // Для этого представления отрисовать всё предоставленное.
        OutPacket.geometries = DArray<GeometryRenderData>(GeometryDataCount, false, rFrameData.FrameAllocator);
        OutPacket.geometries.Resize(GeometryDataCount);
        OutPacket.geometries = *WorldData->WorldGeometries;
    }
    // Ландшафт
    if (*WorldData->TerrainGeometries) {
        const u32 TerrainDataCount = WorldData->TerrainGeometries->Length();
        // Для этого представления отрисовать всё предоставленное.
        OutPacket.TerrainGeometries = DArray<GeometryRenderData>(TerrainDataCount, false, rFrameData.FrameAllocator);
        OutPacket.TerrainGeometries.Resize(TerrainDataCount);
        OutPacket.TerrainGeometries = *WorldData->TerrainGeometries;
    }

    return true;
}

bool RenderViewWireframe::Render(const RenderView *self, RenderViewPacket &packet, const FrameData &rFrameData)
{
    if (self) {
        auto data = (sRenderViewWireframe*)self->data;
        auto renderer = SystemsManager::GetRenderingSystem();

        // Привязать область просмотра
        renderer->SetActiveViewport(packet.viewport);

        // ПРИМЕЧАНИЕ: первый проход рендеринга — единственный.
        auto pass = &self->passes[0]; 

        if (!renderer->RenderpassBegin(pass, pass->targets[rFrameData.RenderTargetIndex])) {
            MERROR("RenderViewWireframe::Render Не удалось запустить проход рендеринга.");
            return false;
        }

        auto render { [data, &packet, &rFrameData, renderer](WireframeShaderInfo& info, DArray<GeometryRenderData>& array) {
            ShaderSystem::Use(info.shader->id);

            // Установить глобальные униформы
            info.shader->BindGlobals();
            if (!ShaderSystem::UniformSet(info.locations.projection, &packet.ProjectionMatrix)) {
                MERROR("Не удалось установить униформу матрицы проекции для каркасного шейдера.");
                return false;
            }
            if (!ShaderSystem::UniformSet(info.locations.view, &packet.ViewMatrix)) {
                MERROR("Не удалось установить униформу матрицы вида для каркасного шейдера.");
                return false;
            }
            ShaderSystem::ApplyGlobal(true, rFrameData);

            if (array) {
                const u32 ArraySize = array.Length();
                for (u32 i = 0; i < ArraySize; ++i) {
                    // Установить униформы экземпляра.
                    auto& geometry = array[i];
                    // Выбор экземпляра позволяет легко менять цвет.
                    WireframeColoureInstance* inst = nullptr;
                    if (geometry.UniqueID == data->SelectedID) {
                        inst = &info.SelectedInstance;
                    } else {
                        inst = &info.NormalInstance;
                    }

                    ShaderSystem::BindInstance(inst->id);

                    bool NeedsUpdate = inst->FrameNumber != rFrameData.RendererFrameNumber || inst->DrawIndex != rFrameData.DrawIndex;
                    if (NeedsUpdate) {
                        if (!ShaderSystem::UniformSet(info.locations.colour, &inst->colour)) {
                            MERROR("Не удалось установить униформу матрицы проекции для каркасного шейдера.");
                            return false;
                        }
                    }

                    ShaderSystem::ApplyInstance(NeedsUpdate, rFrameData);

                    // Синхронизировать номер кадра и индекс отрисовки.
                    inst->FrameNumber = rFrameData.RendererFrameNumber;
                    inst->DrawIndex = rFrameData.DrawIndex;

                    // Локальные переменные.
                    if (!ShaderSystem::UniformSet(info.locations.model, &geometry.model)) {
                        MERROR("Не удалось применить униформу матрицы модели для каркасного шейдера.");
                        return false;
                    }

                    // Нарисовать его.
                    renderer->DrawGeometry(geometry);
                }
            }
            return true;
        } };

        if (!render(data->MeshShader, packet.geometries)) {
            return false;
        }
        
        if (!render(data->TerrainShader, packet.TerrainGeometries)) {
            return false;
        }

        if (!renderer->RenderpassEnd(pass)) {
            MERROR("RenderViewWireframe::Render Не удалось завершить проход рендеринга.");
            return false;
        }
    }

    return true;
}
