#include "render_view_world.hpp"
#include "renderer/renderer.hpp"
#include "systems/shader_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/material_system.hpp"
#include "renderer/renderpass.hpp"
#include "resources/mesh.hpp"

bool RenderViewWorld::OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context) {
    RenderViewWorld* self = (RenderViewWorld*)ListenerInst;
    if (!self) {
        return false;
    }

    switch (code) {
        case EVENT_CODE_SET_RENDER_MODE: {
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case Renderer::Default:
                    MDEBUG("Режим рендеринга установлен на значение по умолчанию.");
                    self->RenderMode = Renderer::Default;
                    break;
                case Renderer::Lighting:
                    MDEBUG("Режим рендеринга установлен на освещение.");
                    self->RenderMode = Renderer::Lighting;
                    break;
                case Renderer::Normals:
                    MDEBUG("Режим рендеринга установлен на нормали.");
                    self->RenderMode = Renderer::Normals;
                    break;
            }
            return true;
        }
    }

    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}

constexpr RenderViewWorld::RenderViewWorld()
:
    RenderView(),
    // Получите либо переопределение пользовательского шейдера, либо заданное значение по умолчанию.
    ShaderID(ShaderSystem::GetInstance()->GetID(CustomShaderName ? CustomShaderName : "Shader.Builtin.Material")),
    fov(Math::DegToRad(45.F)),
    NearClip(0.1F),
    FarClip(1000.F),
    ProjectionMatrix(Matrix4D::MakeFrustumProjection(fov, 1280 / 720.f, NearClip, FarClip)), // Поумолчанию
    WorldCamera(CameraSystem::Instance()->GetDefault()),
    AmbientColour(0.25F, 0.25F, 0.25F, 1.F),
    RenderMode()
{
    // Следите за изменениями режима.
    if (!Event::GetInstance()->Register(EVENT_CODE_SET_RENDER_MODE, this, OnEvent)) {
        MERROR("Не удалось прослушать событие установки режима рендеринга, создание не удалось.");
        return;
    }
}

RenderViewWorld::~RenderViewWorld()
{
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
            passes[i]->RenderArea.x = 0;
            passes[i]->RenderArea.y = 0;
            passes[i]->RenderArea.z = width;
            passes[i]->RenderArea.w = height;
        }
    }
}

bool RenderViewWorld::BuildPacket(void *data, Packet *OutPacket)
{
    if (!data || !OutPacket) {
        MWARN("RenderViewWorld::BuildPacket требует действительный указатель на вид, пакет и данные.");
        return false;
    }

    Mesh::PacketData* MeshData = (Mesh::PacketData*)data;
    
    OutPacket->view = this;

    // Установить матрицы и т. д.
    OutPacket->ProjectionMatrix = ProjectionMatrix;
    OutPacket->ViewMatrix = WorldCamera->GetView();
    OutPacket->ViewPosition = WorldCamera->GetPosition();
    OutPacket->AmbientColour = AmbientColour;

    // Получить все геометрии из текущей сцены.
    // Перебрать все сетки и добавить их в коллекцию геометрий пакета
    for (u32 i = 0; i < mesh_data->mesh_count; ++i) {
        Mesh* m = &MeshData->meshes[i];
        for (u32 j = 0; j < m->GeometryCount; ++j) {
            // Добавлять только сетки с _отсутствием_ прозрачности.
            // ЗАДАЧА: Добавить что-то к материалу для проверки прозрачности.
            if ((m->geometries[j]->material->diffuse_map.texture->flags & TEXTURE_FLAG_HAS_TRANSPARENCY) == 0) {
                OutPacket->geometries.EmplaceBack(m->transform.GetWorld(), m->geometries[j]);
                OutPacket->GeometryCount++;
            }
        }
    }

    return true;
}

bool RenderViewWorld::Render(const Packet *packet, u64 FrameNumber, u64 RenderTargetIndex)
{
    for (u32 p = 0; p < RenderpassCount; ++p) {
        Renderpass* pass = passes[p];
        if (!Renderer::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
            MERROR("RenderViewWorld::Render pass index %u не удалось запустить.", p);
            return false;
        }

        if (!ShaderSystem::GetInstance()->Use(ShaderID)) {
            MERROR("Не удалось использовать шейдер материала. Не удалось отрисовать кадр.");
            return false;
        }

        // Применить глобальные переменные
        // ЗАДАЧА: Найти общий способ запроса данных, таких как окружающий цвет (который должен быть из сцены) и режим (из рендерера)
        if (!MaterialSystem::Instance()->ApplyGlobal(ShaderID, packet->ProjectionMatrix, packet->ViewMatrix, packet->AmbientColour, packet->ViewPosition, RenderMode)) {
            MERROR("Не удалось использовать применить глобальные переменные для шейдера материала. Не удалось отрисовать кадр.");
            return false;
        }

        // Нарисовать геометрию.
        auto& count = packet->GeometryCount;
        for (u32 i = 0; i < count; ++i) {
            Material* m = 0;
            if (packet->geometries[i].geometry->material) {
                m = packet->geometries[i].geometry->material;
            } else {
                m = MaterialSystem::Instance()->GetDefaultMaterial();
            }

            // Обновите материал, если он еще не был в этом кадре. 
            // Это предотвращает многократное обновление одного и того же материала. 
            // Его все равно нужно привязать в любом случае, поэтому этот результат проверки передается на бэкэнд, 
            // который либо обновляет внутренние привязки шейдера и привязывает их, либо только привязывает их.
            bool NeedsUpdate = m->RenderFrameNumber != FrameNumber;
            if (!MaterialSystem::Instance()->ApplyInstance(m, NeedsUpdate)) {
                MWARN("Не удалось применить материал '%s'. Пропуск отрисовки.", m->name);
                continue;
            } else {
                // Синхронизируйте номер кадра.
                m->RenderFrameNumber = FrameNumber;
            }

            // Примените локальные переменные
            MaterialSystem::Instance()->ApplyLocal(m, packet->geometries[i].model);

            // Нарисуйте его.
            Renderer::DrawGeometry(packet->geometries[i]);
        }

        if (!Renderer::RenderpassEnd(pass)) {
            MERROR("Не удалось завершить проход RenderViewWorld::Render индекс %u.", p);
            return false;
        }
    }

    return true;
}
