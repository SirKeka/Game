#include "render_view_ui.hpp"
#include "memory/linear_allocator.hpp"
#include "systems/shader_system.hpp"
#include "renderer/renderpass.hpp"
#include "resources/font_resource.hpp"
#include "resources/mesh.hpp"
#include "renderer/rendering_system.hpp"
#include "renderer/renderpass.hpp"
#include "systems/material_system.hpp"
#include "systems/resource_system.hpp"
#include "resources/geometry.hpp"

RenderViewUI::RenderViewUI(u16 id, const Config &config)
:
RenderView(id, config), 
shader(),
NearClip(-100.F), 
FarClip(100.F), 
ProjectionMatrix(Matrix4D::MakeOrthographicProjection(0.F, 1280.F, 720.F, 0.F, NearClip, FarClip)), 
ViewMatrix(Matrix4D::MakeIdentity()),
DiffuseMapLocation(),
DiffuseColourLocation(),
ModelLocation()
/*RenderMode(),*/ 
{
    const char* ShaderName = "Shader.Builtin.UI";
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

    DiffuseMapLocation    = ShaderSystem::UniformIndex(shader, "diffuse_texture");
    DiffuseColourLocation = ShaderSystem::UniformIndex(shader, "diffuse_colour");
    ModelLocation         = ShaderSystem::UniformIndex(shader, "model");

    if(!EventSystem::Register(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, this, RenderViewOnEvent)) {
        MERROR("Не удалось прослушать событие, требующее обновления, создание не удалось.");
        return;
    }
}

RenderViewUI::~RenderViewUI()
{
    // Отменить регистрацию на мероприятии.
    EventSystem::Unregister(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, this, RenderViewOnEvent);
}

void RenderViewUI::Resize(u32 width, u32 height)
{
    // Проверьте, отличается ли. Если да, пересоздайте матрицу проекции.
    if (width != this->width || height != this->height) {

        this->width = width;
        this->height = height;
        ProjectionMatrix = Matrix4D::MakeOrthographicProjection(0.0f, (f32)this->width, (f32)this->height, 0.0f, NearClip, FarClip);

        for (u32 i = 0; i < RenderpassCount; ++i) {
            passes[i].RenderArea = FVec4(0, 0, width, height);
        }
    }
}

bool RenderViewUI::BuildPacket(LinearAllocator& FrameAllocator, void *data, Packet &OutPacket)
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
    OutPacket.ExtendedData = FrameAllocator.Allocate(sizeof(UiPacketData));
    MemorySystem::CopyMem(OutPacket.ExtendedData, PacketData, sizeof(UiPacketData));

    // Получить все геометрии из текущей сцены.
    // Итерировать все сетки и добавить их в коллекцию геометрий пакета
    for (u32 i = 0; i < PacketData->MeshData.MeshCount; ++i) {
        auto* m = PacketData->MeshData.meshes[i];
        for (u32 j = 0; j < m->GeometryCount; ++j) {
            GeometryRenderData RenderData;
            RenderData.gid = m->geometries[j];
            RenderData.model = m->transform.GetWorld();
            OutPacket.geometries.PushBack(RenderData);
            OutPacket.GeometryCount++;
        }
    }
    return true;
}

bool RenderViewUI::Render(const Packet &packet, u64 FrameNumber, u64 RenderTargetIndex)
{
    auto MaterialSystemInst = MaterialSystem::Instance();
    const auto& ShaderID = shader->id;
    for (u32 p = 0; p < RenderpassCount; ++p) {
        auto pass = &passes[p];
        if (!RenderingSystem::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
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
            RenderingSystem::DrawGeometry(packet.geometries[i]);
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

        if (!RenderingSystem::RenderpassEnd(pass)) {
            MERROR("RenderViewUI::Render Проход рендеринга с индексом %u не завершился.", p);
            return false;
        }
    }

    return true;
}

void *RenderViewUI::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void RenderViewUI::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
