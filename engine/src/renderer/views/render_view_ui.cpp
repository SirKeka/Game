#include "render_view_ui.hpp"
#include "systems/shader_system.hpp"
#include "renderer/renderpass.hpp"
#include "resources/font_resource.hpp"
#include "resources/mesh.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderpass.hpp"
#include "systems/material_system.hpp"
#include "resources/geometry.hpp"


RenderViewUI::RenderViewUI()
    : 
    RenderView(), 
    ShaderID(ShaderSystem::GetID(CustomShaderName ? CustomShaderName : "Shader.Builtin.UI")), 
    shader(ShaderSystem::GetShader(ShaderID)),
    NearClip(-100.F), 
    FarClip(100.F), 
    ProjectionMatrix(Matrix4D::MakeOrthographicProjection(0.F, 1280.F, 720.F, 0.F, NearClip, FarClip)), 
    ViewMatrix(Matrix4D::MakeIdentity()),
    DiffuseMapLocation(ShaderSystem::UniformIndex(shader, "diffuse_texture")),
    DiffuseColourLocation(ShaderSystem::UniformIndex(shader, "diffuse_colour")),
    ModelLocation(ShaderSystem::UniformIndex(shader, "model"))
    /*RenderMode(),*/ 
{}

RenderViewUI::RenderViewUI(u16 id, MString& name, KnownType type, u8 RenderpassCount, const char* CustomShaderName)
:
RenderView(id, name, type, RenderpassCount, CustomShaderName),
ShaderID(ShaderSystem::GetID(CustomShaderName ? CustomShaderName : "Shader.Builtin.UI")), 
shader(ShaderSystem::GetShader(ShaderID)),
NearClip(-100.F), 
FarClip(100.F), 
ProjectionMatrix(Matrix4D::MakeOrthographicProjection(0.F, 1280.F, 720.F, 0.F, NearClip, FarClip)), 
ViewMatrix(Matrix4D::MakeIdentity()),
DiffuseMapLocation(ShaderSystem::UniformIndex(shader, "diffuse_texture")),
DiffuseColourLocation(ShaderSystem::UniformIndex(shader, "diffuse_colour")),
ModelLocation(ShaderSystem::UniformIndex(shader, "model"))
/*RenderMode(),*/ 
{}

RenderViewUI::~RenderViewUI()
{
}

void RenderViewUI::Resize(u32 width, u32 height)
{
    // Проверьте, отличается ли. Если да, пересоздайте матрицу проекции.
    if (width != this->width || height != this->height) {

        this->width = width;
        this->height = height;
        ProjectionMatrix = Matrix4D::MakeOrthographicProjection(0.0f, (f32)this->width, (f32)this->height, 0.0f, NearClip, FarClip);

        for (u32 i = 0; i < RenderpassCount; ++i) {
            passes[i]->RenderArea = FVec4(0, 0, width, height);
        }
    }
}

bool RenderViewUI::BuildPacket(void *data, Packet &OutPacket) const
{
    if (!data) {
        MWARN("RenderViewUI::BuildPacket требует действительный указатель на представление, пакет и данные.");
        return false;
    }

    auto PacketData = reinterpret_cast<UiPacketData*>(data);

    OutPacket.view = this;

    // Установить матрицы и т. д.
    OutPacket.ProjectionMatrix = ProjectionMatrix;
    OutPacket.ViewMatrix = ViewMatrix;

    // ЗАДАЧА: временно установить расширенные данные для тестовых текстовых объектов на данный момент.
    OutPacket.ExtendedData = data;

    // Получить все геометрии из текущей сцены.
    // Итерировать все сетки и добавить их в коллекцию геометрий пакета
    for (u32 i = 0; i < PacketData->MeshData.MeshCount; ++i) {
        auto* m = PacketData->MeshData.meshes[i];
        for (u32 j = 0; j < m->GeometryCount; ++j) {
            OutPacket.geometries.EmplaceBack(m->transform.GetWorld(), m->geometries[j]);
            OutPacket.GeometryCount++;
        }
    }
    return true;
}

bool RenderViewUI::Render(const Packet &packet, u64 FrameNumber, u64 RenderTargetIndex) const
{
    auto MaterialSystemInst = MaterialSystem::Instance();
    for (u32 p = 0; p < RenderpassCount; ++p) {
        auto pass = passes[p];
        if (!Renderer::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
            MERROR("RenderViewUI::Render pass index %u не удалось запустить.", p);
            return false;
        }

        if (!ShaderSystem::Use(ShaderID)) {
            MERROR("Не удалось использовать шейдер материала. Не удалось отрисовать кадр.");
            return false;
        }

        // Применить глобальные переменные
        if (!MaterialSystemInst->ApplyGlobal(ShaderID, FrameNumber, packet.ProjectionMatrix, packet.ViewMatrix)) {
            MERROR("Не удалось использовать применение глобальных переменных для шейдера материала. Не удалось отрисовать кадр.");
            return false;
        }

        // Нарисовать геометрию.
        u32 count = packet.GeometryCount;
        for (u32 i = 0; i < count; ++i) {
            Material* m = nullptr;
            if (packet.geometries[i].gid->material) {
                m = packet.geometries[i].gid->material;
            } else {
                m = MaterialSystemInst->GetDefaultMaterial();
            }

            // Обновить материал, если он еще не был в этом кадре. 
            // Это предотвращает многократное обновление одного и того же материала. 
            // Его все равно нужно привязать в любом случае, 
            // поэтому результат этой проверки передается на бэкэнд, 
            // который либо обновляет внутренние привязки шейдера и привязывает их, либо только привязывает их.
            bool NeedsUpdate = m->RenderFrameNumber != FrameNumber;
            if (!MaterialSystemInst->ApplyInstance(m, NeedsUpdate)) {
                MWARN("Не удалось применить материал '%s'. Пропуск рисования.", m->name);
                continue;
            } else {
                // Синхронизируйте номер кадра.
                m->RenderFrameNumber = FrameNumber;
            }

            // Примените локальные
            MaterialSystemInst->ApplyLocal(m, packet.geometries[i].model);

            // Нарисуйте его.
            Renderer::DrawGeometry(packet.geometries[i]);
        }

        // Нарисовать растровый текст
        auto PacketData = reinterpret_cast<UiPacketData*>(packet.ExtendedData);  // массив текстов
        for (u32 i = 0; i < PacketData->TextCount; ++i) {
            auto text = PacketData->texts[i];
            ShaderSystem::BindInstance(text->InstanceID);

            if (!ShaderSystem::UniformSet(DiffuseMapLocation, &text->data->atlas)) {
                MERROR("Не удалось применить диффузную карту растрового шрифта.");
                return false;
            }

            // ЗАДАЧА: цвет текста.
            static FVec4 WhiteColour {1.F, 1.F, 1.F, 1.F};  // белый
            if (!ShaderSystem::UniformSet(DiffuseColourLocation, &WhiteColour)) {
                MERROR("Не удалось применить диффузную цветовую форму растрового шрифта.");
                return false;
            }
            bool NeedsUpdate = text->RenderFrameNumber != FrameNumber;
            ShaderSystem::ApplyInstance(NeedsUpdate);

            // Синхронизируйте номер кадра.
            text->RenderFrameNumber = FrameNumber;

            // Применить локальные переменные
            Matrix4D model = text->transform.GetWorld();
            if(!ShaderSystem::UniformSet(ModelLocation, &model)) {
                MERROR("Не удалось применить матрицу модели для текста");
            }

            text->Draw();
        }

        if (!Renderer::RenderpassEnd(pass)) {
            MERROR("RenderViewUI::Render Проход рендеринга с индексом %u не завершился.", p);
            return false;
        }
    }

    return true;
}

void *RenderViewUI::operator new(u64 size)
{
    return MMemory::Allocate(size, Memory::Renderer);
}

void RenderViewUI::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::Renderer);
}
