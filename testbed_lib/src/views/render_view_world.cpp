#include "render_view_world.h"
#include "renderer/renderpass.h"
#include "resources/geometry.h"
#include "resources/mesh.h"
#include "systems/camera_system.hpp"
#include "systems/material_system.h"
#include "systems/render_view_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

/// @brief Частная структура, используемая для сортировки геометрии по расстоянию от камеры.
struct GeometryDistance {
    GeometryRenderData g;   // Данные рендеринга геометрии.
    f32 distance;           // Расстояние от камеры.
};

struct MaterialInfo {
    FVec4 DiffuseColour;
    f32 specular;
    FVec3 padding;
};

bool RenderViewWorld::OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context) {
    
    if (!ListenerInst) {
        return false;
    }

    auto self = reinterpret_cast<RenderView*>(ListenerInst);
    auto data = reinterpret_cast<RenderViewWorld*>(self->data);

    switch (code) {
        case EventSystem::SetRenderMode: {
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case Render::Default:
                    MDEBUG("Режим рендеринга установлен на значение по умолчанию.");
                    data->RenderMode = Render::Default;
                    break;
                case Render::Lighting:
                    MDEBUG("Режим рендеринга установлен на освещение.");
                    data->RenderMode = Render::Lighting;
                    break;
                case Render::Normals:
                    MDEBUG("Режим рендеринга установлен на нормали.");
                    data->RenderMode = Render::Normals;
                    break;
            }
            return true;
        }
        case EventSystem::DefaultRendertargetRefreshRequired: {
            RenderViewSystem::RegenerateRenderTargets(self);
            // Это должно быть использовано другими представлениями, поэтому считайте, что это _не_ обработано.
            return false;
        }
    }

    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}

bool RenderViewWorld::OnRegistered(RenderView* self)
{
    if (self) {
        auto data = new RenderViewWorld();
        self->data = data; 

        data->WorldCamera = CameraSystem::Instance()->GetDefault();
        const char* ShaderName = "Shader.Builtin.Material";
        ShaderResource ConfigResource;
        if (!ResourceSystem::Load(ShaderName, eResource::Shader, nullptr, ConfigResource)) {
            MERROR("Не удалось загрузить ресурс встроенного шейдера материала.");
            return false;
        }

        // ПРИМЕЧАНИЕ: Предположим, что это первый проход, так как это все, что есть в этом представлении.
        if (!ShaderSystem::Create(self->passes[0], ConfigResource.data)) {
            MERROR("Не удалось загрузить встроенный шейдер материала.");
            return false;
        }
        ResourceSystem::Unload(ConfigResource);

        // Загрузка шейдера ландшафта.
        const char* TerrainShaderName = "Shader.Builtin.Terrain";
        ShaderResource TerrainShaderConfigResource;
        if (!ResourceSystem::Load(TerrainShaderName, eResource::Type::Shader, nullptr, TerrainShaderConfigResource)) {
            MERROR("Не удалось загрузить встроенный ресурс шейдера ландшафта.");
            return false;
        }
        auto& TerrainShaderConfig = TerrainShaderConfigResource.data;
        // ПРИМЕЧАНИЕ: Предположим, что это первый проход, поскольку это все, что есть в этом представлении.
        if (!ShaderSystem::Create(self->passes[0], TerrainShaderConfig)) {
            MERROR("Не удалось загрузить встроенный шейдер ландшафта.");
            return false;
        }
        // ResourceSystem::Unload(TerrainShaderConfigResource);

        // Загрузить отладочный шейдер colour3d.
        // ЗАДАЧА: переместить встроенные шейдеры в саму систему шейдеров.
        const char* Colour3dShaderName = "Shader.Builtin.ColourShader3D";
        if (!ResourceSystem::Load(Colour3dShaderName, eResource::Type::Shader, nullptr, ConfigResource)) {
            MERROR("Не удалось загрузить встроенный ресурс шейдера colour3d.");
            return false;
        }
        auto& Colour3dShaderConfig = ConfigResource.data;
        // ПРИМЕЧАНИЕ: предполагается первый проход, так как это все, что есть в этом представлении.
        if (!ShaderSystem::Create(self->passes[0], Colour3dShaderConfig)) {
            MERROR("Не удалось загрузить встроенный шейдер colour3d.");
            return false;
        }
        ResourceSystem::Unload(ConfigResource);

        // Получить однородные расположения шейдера colour3d.
        {
            auto shader = ShaderSystem::GetShader(Colour3dShaderName);
            if (!shader) {
                MERROR("Не удалось получить шейдер colour3d!");
                return false;
            }
            data->DebugLocations.projection = ShaderSystem::UniformIndex(shader, "projection");
            data->DebugLocations.view = ShaderSystem::UniformIndex(shader, "view");
            data->DebugLocations.model = ShaderSystem::UniformIndex(shader, "model");
        }

        // Получите либо переопределение пользовательского шейдера, либо заданное значение по умолчанию.
        data->shader = ShaderSystem::GetShader(self->CustomShaderName ? self->CustomShaderName : ShaderName);

        // Следите за изменениями режима.
        if (!EventSystem::Register(EventSystem::SetRenderMode, self, OnEvent)) {
            MERROR("Не удалось прослушать событие установки режима рендеринга, создание не удалось.");
            return false;
        }

        if (!EventSystem::Register(EventSystem::DefaultRendertargetRefreshRequired, self, OnEvent)) {
            MERROR("Не удалось прослушать событие установки режима рендеринга, создание не удалось.");
            return false;
        }

        return true;
    }
    
    return false;
}

void RenderViewWorld::Destroy(RenderView *self)
{
    EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, self->data, OnEvent);
    EventSystem::Unregister(EventSystem::SetRenderMode, self->data, OnEvent);

    MemorySystem::Free(self->data, sizeof(RenderViewWorld), Memory::Renderer);
    self->data = nullptr;
}

void RenderViewWorld::Resize(RenderView* self, u32 width, u32 height)
{
    if (self) {
        auto data = reinterpret_cast<RenderViewWorld*>(self->data);

        // Проверьте, отличается ли. Если да, пересоздайте матрицу проекции.
        if (width != self->width || height != self->height) {

            self->width = width;
            self->height = height;
            f32 aspect = (f32)self->width / self->height;
            data->ProjectionMatrix = Matrix4D::MakeFrustumProjection(data->fov, aspect, data->NearClip, data->FarClip);

            for (u32 i = 0; i < self->RenderpassCount; ++i) {
                self->passes[i].RenderArea = FVec4(0, 0, width, height);
            }
        }
    }
    
}

bool RenderViewWorld::BuildPacket(RenderView* self, LinearAllocator& FrameAllocator, void *data, RenderViewPacket &OutPacket)
{
    if (!data) {
        MWARN("RenderViewWorld::BuildPacket требует действительный указатель на вид, пакет и данные.");
        return false;
    }

    if (self) {
        auto rwwData = reinterpret_cast<RenderViewWorld*>(self->data);
        auto& WorldData = *reinterpret_cast<RenderViewWorldData*>(data);
    
        OutPacket.view = self;

        // Установить матрицы и т. д.
        OutPacket.ProjectionMatrix = rwwData->ProjectionMatrix;
        OutPacket.ViewMatrix       = rwwData->WorldCamera->GetView();
        OutPacket.ViewPosition     = rwwData->WorldCamera->GetPosition();
        OutPacket.AmbientColour     = rwwData->AmbientColour;

        // ЗАДАЧА: перенести сортировку в динамический массив.
        // Получить все геометрии из текущей сцены.
        DArray<GeometryDistance> GeometryDistances;
        
        for (u32 i = 0; i < WorldData.WorldGeometries.Length(); ++i) {
            auto& gData = WorldData.WorldGeometries[i];
            if (!gData.geometry) {
                continue;
            }

            // ЗАДАЧА: Добавить что-то к материалу для проверки прозрачности.
            bool HasTransparancy = false;
            if ((gData.geometry->material->type == Material::Type::Phong) == 0) {
                HasTransparancy = (gData.geometry->material->maps[0].texture->flags & Texture::Flag::HasTransparency) == 0;
            }
            if (HasTransparancy) {
                OutPacket.geometries.PushBack(gData);
            } else {
                // Для сеток _с_ прозрачностью добавьте их в отдельный список, чтобы позже отсортировать по расстоянию.
                // Получите центр, извлеките глобальную позицию из матрицы модели и добавьте ее в центр, 
                // затем вычислите расстояние между ней и камерой и, наконец, сохраните ее в списке для сортировки.
                // ПРИМЕЧАНИЕ: это не идеально для полупрозрачных сеток, которые пересекаются, но для наших целей сейчас достаточно.
                auto center = gData.geometry->center * gData.model; // transform
                auto distance = Distance(center, rwwData->WorldCamera->GetPosition());
                GeometryDistance GeoDist;
                GeoDist.g = gData;
                GeoDist.distance = Math::abs(distance);
                GeometryDistances.PushBack(GeoDist);
            }
        }

        // Сортировать расстояния
        u32 GeometryCount = GeometryDistances.Length();
        QuickSort(GeometryDistances.Data(), 0, GeometryCount - 1, false);

        // Добавьте их в геометрию пакета.
        for (u32 i = 0; i < GeometryCount; ++i) {
            OutPacket.geometries.PushBack(GeometryDistances[i].g);
            // OutPacket.GeometryCount++;
        }

        const u32& TerrainCount = WorldData.TerrainGeometries.Length();
        for (u32 i = 0; i < TerrainCount; ++i) {
            OutPacket.TerrainGeometries.PushBack(WorldData.TerrainGeometries[i]);
            // OutPacket.TerrainGeometryCount++;
        }

        // Отлладочная геометрия.
        OutPacket.DebugGeometries = WorldData.DebugGeometries;

        GeometryDistances.Destroy();

        return true;
    }
    
    return false;
}

bool RenderViewWorld::Render(const RenderView* self, const RenderViewPacket &packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData)
{
    if (self) {
        auto data = reinterpret_cast<RenderViewWorld*>(self->data);

        const auto& ShaderID = data->shader->id;
        for (u32 p = 0; p < self->RenderpassCount; ++p) {
            auto pass = &self->passes[p];
            if (!RenderingSystem::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
                MERROR("RenderViewWorld::Render pass index %u не удалось запустить.", p);
                return false;
            }

            // Используйте соответствующий шейдер и примените глобальные униформы.
            const u32& TerrainCount = packet.TerrainGeometries.Length();
            if (TerrainCount > 0) {
                // ЗАДАЧА: Если используется пользовательский шейдер, это приведет к ошибке. Необходимо также обработать их.
                auto shader = ShaderSystem::GetShader("Shader.Builtin.Terrain");
                if (!shader) {
                    MERROR("Невозможно получить шейдер ландшафта.");
                    return false;
                }
                ShaderSystem::Use(shader->id);

                // Применить глобальные переменные
                // ЗАДАЧА: Найдите общий способ запроса данных, таких как цвет окружения (который должен быть из сцены) и режим (из рендерера)
                if (!MaterialSystem::ApplyGlobal(shader->id, FrameNumber, packet.ProjectionMatrix, packet.ViewMatrix, packet.AmbientColour, packet.ViewPosition,  data->RenderMode)) {
                    MERROR("Не удалось применить глобальные переменные для шейдера ландшафта. Кадр рендеринга не удалось.");
                    return false;
                }

                for (u32 i = 0; i < TerrainCount; ++i) {
                    Material* material = nullptr;
                    if (packet.TerrainGeometries[i].geometry->material) {
                        material = packet.TerrainGeometries[i].geometry->material;
                    } else {
                        material = MaterialSystem::GetDefaultTerrainMaterial();
                    }

                    // Обновите материал, если он еще не был в этом кадре. 
                    // Это предотвращает многократное обновление одного и того же материала. 
                    // Его все равно нужно привязать в любом случае, поэтому этот результат проверки передается на бэкэнд, 
                    // который либо обновляет внутренние привязки шейдера и привязывает их, либо только привязывает их.
                    bool NeedsUpdate = material->RenderFrameNumber != FrameNumber;
                    if (!MaterialSystem::ApplyInstance(material, NeedsUpdate)) {
                        MWARN("Не удалось применить материал ландшафта '%s'. Пропуск отрисовки.", material->name);
                        continue;
                    } else {
                        // Синхронизируйте номер кадра.
                        material->RenderFrameNumber = FrameNumber;
                    }

                    // Примените локальные
                    MaterialSystem::ApplyLocal(material, packet.TerrainGeometries[i].model);

                    // Нарисуйте его.
                    RenderingSystem::DrawGeometry(packet.TerrainGeometries[i]);
                }
            }

            // Статичные геометрии.
            const u32& GeometryCount = packet.geometries.Length();
            if (GeometryCount > 0) {
                if (!ShaderSystem::Use(ShaderID)) {
                    MERROR("Не удалось использовать шейдер материала. Не удалось отрисовать кадр.");
                    return false;
                }

                // Применить глобальные переменные
                // ЗАДАЧА: Найти общий способ запроса данных, таких как окружающий цвет (который должен быть из сцены) и режим (из рендерера)
                if (!MaterialSystem::ApplyGlobal(ShaderID, FrameNumber, packet.ProjectionMatrix, packet.ViewMatrix, packet.AmbientColour, packet.ViewPosition,  data->RenderMode)) {
                    MERROR("Не удалось использовать применить глобальные переменные для шейдера материала. Не удалось отрисовать кадр.");
                    return false;
                }

                // Нарисовать геометрию.
                const auto& count = packet.geometries.Length();
                for (u32 i = 0; i < count; ++i) {
                    Material* m = nullptr;
                    if (packet.geometries[i].geometry->material) {
                        m = packet.geometries[i].geometry->material;
                    } else {
                        m = MaterialSystem::GetDefaultMaterial();
                    }

                    // Обновите материал, если он еще не был в этом кадре. 
                    // Это предотвращает многократное обновление одного и того же материала. 
                    // Его все равно нужно привязать в любом случае, поэтому этот результат проверки передается на бэкэнд, 
                    // который либо обновляет внутренние привязки шейдера и привязывает их, либо только привязывает их.
                    bool NeedsUpdate = m->RenderFrameNumber != FrameNumber;
                    if (!MaterialSystem::ApplyInstance(m, NeedsUpdate)) {
                        MWARN("Не удалось применить материал '%s'. Пропуск отрисовки.", m->name);
                        continue;
                    } else {
                        // Синхронизируйте номер кадра.
                        m->RenderFrameNumber = FrameNumber;
                    }

                    // Примените локальные переменные
                    MaterialSystem::ApplyLocal(m, packet.geometries[i].model);

                    // Нарисуйте его.
                    RenderingSystem::DrawGeometry(packet.geometries[i]);
                }
            }

            // ЗАДАЧА: Отладка геометрии (т. е. сеток, линий, блоков, гизмо и т. д.)
            // Это должно проходить через ту же геометрическую систему, что и все остальное.
            const u32& DebugGeometryCount = packet.DebugGeometries.Length();
            if (DebugGeometryCount > 0) {
                auto shader = ShaderSystem::GetShader("Shader.Builtin.ColourShader3D");
                if (!shader) {
                    MERROR("Невозможно получить шейдер colour3d.");
                    return false;
                }
                ShaderSystem::Use(shader->id);

                // Глобальные
                ShaderSystem::UniformSet(data->DebugLocations.projection, &packet.ProjectionMatrix);
                ShaderSystem::UniformSet(data->DebugLocations.view, &packet.ViewMatrix);

                ShaderSystem::ApplyGlobal(true);

                // Каждая геометрия.
                for (u32 i = 0; i < DebugGeometryCount; ++i) {
                    // ПРИМЕЧАНИЕ: не нужно устанавливать униформы на уровне экземпляра.

                    // Локальные
                    ShaderSystem::UniformSet(data->DebugLocations.model, &packet.DebugGeometries[i].model);

                    // Нарисуйте ее.
                    RenderingSystem::DrawGeometry(packet.DebugGeometries[i]);
                }
                // HACK: Это должно каким-то образом обрабатываться в каждом кадре шейдерной системой.
                shader->RenderFrameNumber = FrameNumber;
            }

            if (!RenderingSystem::RenderpassEnd(pass)) {
                MERROR("Не удалось завершить проход RenderViewWorld::Render индекс %u.", p);
                return false;
            }
        }

        return true;
    }
    
    return false;
}

void *RenderViewWorld::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void RenderViewWorld::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
