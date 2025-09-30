#include "render_view_wireframe.h"
#include "memory/linear_allocator.h"
#include "renderer/camera.h"
#include "renderer/rendering_system.h"
#include "renderer/renderpass.h"
#include "renderer/viewport.h"
#include "resources/shader.h"
#include "systems/render_view_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

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

struct sRenderViewWireframe
{
    Shader* shader{nullptr};
    WireframeShaderLocations WireframeLocations;
    u32 SelectedID{INVALID::ID};
    // Один экземпляр на каждый нарисованный цвет.
    WireframeColoureInstance GeometrtyInstance{};
    WireframeColoureInstance TerrainInstance{};
    WireframeColoureInstance SelectedInstance{};

    void* operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Renderer);
    }
    void operator delete(void* ptr, u64 size) {
        MemorySystem::Free(ptr, size, Memory::Renderer);
    }
};

static bool WireframeOnEvent(u16 code, void* sender, void* ListenerInst, EventContext context) {
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
            RenderViewSystem::RegenerateRenderTargets(self);
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

    // Загрузите каркасный шейдер и его местоположение.
    const char* WireframeShaderName = "Shader.Builtin.Wireframe";
    ShaderResource WireframeShaderConfigResource;
    if (!ResourceSystem::Load(WireframeShaderName, eResource::Shader, nullptr, WireframeShaderConfigResource)) {
        MERROR("Не удалось загрузить встроенный каркасный шейдер.");
        return false;
    }
    auto& WireframeShaderConfig = WireframeShaderConfigResource.data;
    if (!ShaderSystem::CreateShader(self->passes[0], WireframeShaderConfig)) {
        MERROR("Не удалось загрузить встроенный каркасный шейдер.");
        return false;
    }
    // ResourceSystem::Unload(WireframeShaderConfigResource);

    data->shader = ShaderSystem::GetShader(WireframeShaderName);
    data->WireframeLocations.projection = ShaderSystem::UniformIndex(data->shader, "projection");
    data->WireframeLocations.view = ShaderSystem::UniformIndex(data->shader, "view");
    data->WireframeLocations.model = ShaderSystem::UniformIndex(data->shader, "model");
    data->WireframeLocations.colour = ShaderSystem::UniformIndex(data->shader, "colour");

    // Приобретите ресурсы экземпляра шейдера.
    // data->GeometrtyInstance = (WireframeColoureInstance){0};
    data->GeometrtyInstance.colour = FVec4(0.5F, 0.8F, 0.8F, 1.F);
    if (!RenderingSystem::ShaderAcquireInstanceResources(data->shader, 0, nullptr, data->GeometrtyInstance.id)) {
        MERROR("Не удалось получить ресурсы экземпляра геометрического шейдера из каркасного шейдера.");
        return false;
    }

    // data->TerrainInstance = (WireframeColoureInstance){0};
    data->TerrainInstance.colour = FVec4(0.8F, 0.8F, 0.5F, 1.F);
    if (!RenderingSystem::ShaderAcquireInstanceResources(data->shader, 0, nullptr, data->TerrainInstance.id)) {
        MERROR("Не удалось получить ресурсы экземпляра шейдера рельефа из каркасного шейдера.");
        return false;
    }

    // data->SelectedInstance = (WireframeColoureInstance){0};
    data->SelectedInstance.colour = FVec4(0.F, 1.F, 0.F, 1.F);
    if (!RenderingSystem::ShaderAcquireInstanceResources(data->shader, 0, nullptr, data->SelectedInstance.id)) {
        MERROR("Не удалось получить выбранные ресурсы экземпляра шейдера из каркасного шейдера.");
        return false;
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
    EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, self, EditorWorldOnEvent);

    // Освободить ресурсы экземпляра шейдера.
    RenderingSystem::ShaderReleaseInstanceResources(data->shader, data->GeometrtyInstance.id);
    RenderingSystem::ShaderReleaseInstanceResources(data->shader, data->TerrainInstance.id);
    RenderingSystem::ShaderReleaseInstanceResources(data->shader, data->SelectedInstance.id);

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

    // Сбросить индексы отрисовки.
    InternalData->GeometrtyInstance.DrawIndex = InternalData->TerrainInstance.DrawIndex = InternalData->SelectedInstance.DrawIndex = 0;

    // Запомнить текущий выбранный объект.
    InternalData->SelectedID = WorldData->SelectedID;

    // Геометрия.
    if (WorldData->WorldGeometries) {
        const u32& GeometryDataCount = WorldData->WorldGeometries->Length();
        // OutPacket.geometries.Resize(GeometryDataCount);
        if (GeometryDataCount) {
            // Для этого представления отрисовать всё предоставленное.
            OutPacket.geometries = DArray<GeometryRenderData>(GeometryDataCount, GeometryDataCount, true, rFrameData.FrameAllocator->Allocate(sizeof(GeometryRenderData) * GeometryDataCount));
            /*for (u32 i = 0; i < GeometryDataCount; ++i) {
                auto& RenderData = OutPacket.geometries[i];
                auto& WorldGeometry = WorldData->WorldGeometries[i];
                RenderData.UniqueID = WorldGeometry.UniqueID;
                RenderData.geometry = WorldGeometry.geometry;
                RenderData.model = WorldGeometry.model;
                RenderData.WindingInverted = WorldGeometry.WindingInverted;
            }*/
           OutPacket.geometries = *WorldData->WorldGeometries;
        }
    }

    return true;
}

bool RenderViewWireframe::Render(const RenderView *self, const RenderViewPacket &packet, const FrameData &rFrameData)
{
    if (self) {
        auto data = (sRenderViewWireframe*)self->data;

        // Привязать область просмотра
        RenderingSystem::SetActiveViewport(packet.viewport);

        // ПРИМЕЧАНИЕ: первый проход рендеринга — единственный.
        auto pass = &self->passes[0]; 

        if (!RenderingSystem::RenderpassBegin(pass, pass->targets[rFrameData.RenderTargetIndex])) {
            MERROR("RenderViewWireframe::Render Не удалось запустить проход рендеринга.");
            return false;
        }

        ShaderSystem::Use(data->shader->id);

        // Установить глобальные униформы
        data->shader->BindGlobals();
        if (!ShaderSystem::UniformSet(data->WireframeLocations.projection, &packet.ProjectionMatrix)) {
            MERROR("Не удалось установить униформу матрицы проекции для каркасного шейдера.");
            return false;
        }
        if (!ShaderSystem::UniformSet(data->WireframeLocations.view, &packet.ViewMatrix)) {
            MERROR("Не удалось установить униформу матрицы вида для каркасного шейдера.");
            return false;
        }
        ShaderSystem::ApplyGlobal(true);

        // Геометрии
        if (packet.geometries) {
            for (u32 i = 0; i < packet.geometries.Length(); ++i) {
                // Установить униформы экземпляра.
                auto& geometry = packet.geometries[i];
                // Выбор экземпляра позволяет легко менять цвет.
                WireframeColoureInstance* inst = nullptr;
                if (geometry.UniqueID == data->SelectedID) {
                    inst = &data->SelectedInstance;
                } else {
                    inst = &data->GeometrtyInstance;
                }

                ShaderSystem::BindInstance(inst->id);

                bool NeedsUpdate = inst->FrameNumber != rFrameData.RendererFrameNumber || inst->DrawIndex != rFrameData.DrawIndex;
                if (NeedsUpdate) {
                    if (!ShaderSystem::UniformSet(data->WireframeLocations.colour, &inst->colour)) {
                        MERROR("Не удалось установить униформу матрицы проекции для каркасного шейдера.");
                        return false;
                    }
                }

                ShaderSystem::ApplyInstance(NeedsUpdate);

                // Синхронизировать номер кадра и индекс отрисовки.
                inst->FrameNumber = rFrameData.RendererFrameNumber;
                inst->DrawIndex = rFrameData.DrawIndex;

                // Локальные переменные.
                if (!ShaderSystem::UniformSet(data->WireframeLocations.model, &geometry.model)) {
                    MERROR("Не удалось применить униформу матрицы модели для каркасного шейдера.");
                    return false;
                }

                // Нарисовать его.
                RenderingSystem::DrawGeometry(geometry);
            }
        }

        // ЗАДАЧА: ландшафты.

        if (!RenderingSystem::RenderpassEnd(pass)) {
            MERROR("RenderViewWireframe::Render Не удалось завершить проход рендеринга.");
            return false;
        }
    }

    return true;
}
