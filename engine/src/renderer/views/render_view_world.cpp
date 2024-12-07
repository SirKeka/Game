#include "render_view_world.hpp"
// #include "memory/linear_allocator.hpp"
#include "renderer/rendering_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/material_system.hpp"
#include "systems/render_view_system.hpp"
#include "systems/resource_system.hpp"
#include "renderer/renderpass.hpp"
#include "resources/mesh.hpp"
#include "resources/geometry.hpp"

/// @brief Частная структура, используемая для сортировки геометрии по расстоянию от камеры.
struct GeometryDistance {
    GeometryRenderData g;   // Данные рендеринга геометрии.
    f32 distance;           // Расстояние от камеры.
};

bool RenderViewWorld::OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context) {
    auto self = reinterpret_cast<RenderViewWorld*>(ListenerInst);
    if (!self) {
        return false;
    }

    switch (code) {
        case EventSystem::SetRenderMode: {
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case Render::Default:
                    MDEBUG("Режим рендеринга установлен на значение по умолчанию.");
                    self->RenderMode = Render::Default;
                    break;
                case Render::Lighting:
                    MDEBUG("Режим рендеринга установлен на освещение.");
                    self->RenderMode = Render::Lighting;
                    break;
                case Render::Normals:
                    MDEBUG("Режим рендеринга установлен на нормали.");
                    self->RenderMode = Render::Normals;
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

/*constexpr */RenderViewWorld::RenderViewWorld(u16 id, const Config &config)
:
RenderView(id, config),
shader(),
fov(Math::DegToRad(45.F)),
NearClip(0.1F),
FarClip(1000.F),
ProjectionMatrix(Matrix4D::MakeFrustumProjection(fov, 1280 / 720.f, NearClip, FarClip)), // Поумолчанию
WorldCamera(CameraSystem::Instance()->GetDefault()),
AmbientColour(0.25F, 0.25F, 0.25F, 1.F),
RenderMode()
{
    const char* ShaderName = "Shader.Builtin.Material";
    ShaderResource ConfigResource;
    if (!ResourceSystem::Load(ShaderName, eResource::Shader, nullptr, ConfigResource)) {
        MERROR("Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
        return;
    }

    // ПРИМЕЧАНИЕ: Предположим, что это первый проход, так как это все, что есть в этом представлении.
    if (!ShaderSystem::Create(passes[0], ConfigResource.data)) {
        MERROR("Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
        return;
    }
    // ResourceSystem::Unload(ConfigResource);

    // Получите либо переопределение пользовательского шейдера, либо заданное значение по умолчанию.
    shader = ShaderSystem::GetShader(CustomShaderName ? CustomShaderName : ShaderName);

    // Следите за изменениями режима.
    if (!EventSystem::Register(EventSystem::SetRenderMode, this, OnEvent)) {
        MERROR("Не удалось прослушать событие установки режима рендеринга, создание не удалось.");
        return;
    }

    if (!EventSystem::Register(EventSystem::DefaultRendertargetRefreshRequired, this, OnEvent)) {
        MERROR("Не удалось прослушать событие установки режима рендеринга, создание не удалось.");
        return;
    }
}

RenderViewWorld::~RenderViewWorld()
{
    EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, this, OnEvent);
    EventSystem::Unregister(EventSystem::SetRenderMode, this, OnEvent);
}

void RenderViewWorld::Resize(u32 width, u32 height)
{
    // Проверьте, отличается ли. Если да, пересоздайте матрицу проекции.
    if (width != this->width || height != this->height) {

        this->width = width;
        this->height = height;
        f32 aspect = (f32)this->width / this->height;
        ProjectionMatrix = Matrix4D::MakeFrustumProjection(fov, aspect, NearClip, FarClip);

        for (u32 i = 0; i < RenderpassCount; ++i) {
            passes[i].RenderArea = FVec4(0, 0, width, height);
        }
    }
}

bool RenderViewWorld::BuildPacket(class LinearAllocator& FrameAllocator, void *data, Packet &OutPacket)
{
    if (!data) {
        MWARN("RenderViewWorld::BuildPacket требует действительный указатель на вид, пакет и данные.");
        return false;
    }

    auto& GeometryData = *reinterpret_cast<DArray<GeometryRenderData>*>(data);
    
    OutPacket.view = this;

    // Установить матрицы и т. д.
    OutPacket.ProjectionMatrix = ProjectionMatrix;
    OutPacket.ViewMatrix = WorldCamera->GetView();
    OutPacket.ViewPosition = WorldCamera->GetPosition();
    OutPacket.AmbientColour = AmbientColour;

    // ЗАДАЧА: перенести сортировку в динамический массив.
    // Получить все геометрии из текущей сцены.
    DArray<GeometryDistance> GeometryDistances;
    
    for (u32 i = 0; i < GeometryData.Length(); ++i) {
        auto& gData = GeometryData[i];
        if (!gData.gid) {
            continue;
        }

        // ЗАДАЧА: Добавить что-то к материалу для проверки прозрачности.
        if ((gData.gid->material->DiffuseMap.texture->flags & Texture::Flag::HasTransparency) == 0) {
            OutPacket.geometries.PushBack(gData);
            OutPacket.GeometryCount++;
        } else {
            // Для сеток _с_ прозрачностью добавьте их в отдельный список, чтобы позже отсортировать по расстоянию.
            // Получите центр, извлеките глобальную позицию из матрицы модели и добавьте ее в центр, 
            // затем вычислите расстояние между ней и камерой и, наконец, сохраните ее в списке для сортировки.
            // ПРИМЕЧАНИЕ: это не идеально для полупрозрачных сеток, которые пересекаются, но для наших целей сейчас достаточно.
            auto center = FVec3::Transform(gData.gid->center, gData.model);
            auto distance = Distance(center, WorldCamera->GetPosition());
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
        OutPacket.GeometryCount++;
    }

    return true;
}

bool RenderViewWorld::Render(const Packet &packet, u64 FrameNumber, u64 RenderTargetIndex)
{
    const auto& ShaderID = shader->id;
    for (u32 p = 0; p < RenderpassCount; ++p) {
        auto pass = &passes[p];
        if (!RenderingSystem::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
            MERROR("RenderViewWorld::Render pass index %u не удалось запустить.", p);
            return false;
        }

        if (!ShaderSystem::Use(ShaderID)) {
            MERROR("Не удалось использовать шейдер материала. Не удалось отрисовать кадр.");
            return false;
        }

        // Применить глобальные переменные
        // ЗАДАЧА: Найти общий способ запроса данных, таких как окружающий цвет (который должен быть из сцены) и режим (из рендерера)
        if (!MaterialSystem::ApplyGlobal(ShaderID, FrameNumber, packet.ProjectionMatrix, packet.ViewMatrix, packet.AmbientColour, packet.ViewPosition, RenderMode)) {
            MERROR("Не удалось использовать применить глобальные переменные для шейдера материала. Не удалось отрисовать кадр.");
            return false;
        }

        // Нарисовать геометрию.
        auto& count = packet.GeometryCount;
        for (u32 i = 0; i < count; ++i) {
            Material* m = nullptr;
            if (packet.geometries[i].gid->material) {
                m = packet.geometries[i].gid->material;
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

        if (!RenderingSystem::RenderpassEnd(pass)) {
            MERROR("Не удалось завершить проход RenderViewWorld::Render индекс %u.", p);
            return false;
        }
    }

    return true;
}

void *RenderViewWorld::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void RenderViewWorld::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
