#include "render_view_ui.h"
#include "memory/linear_allocator.hpp"
#include "systems/shader_system.h"
#include "renderer/renderpass.h"
#include "resources/font_resource.hpp"
#include "resources/mesh.h"
#include "systems/material_system.h"
#include "systems/resource_system.h"
#include "resources/geometry.h"

bool RenderViewUI::OnRegistered(RenderView* self) 
{
    if (self) {
        auto data = new RenderViewUI();
        self->data = data;

        const char* ShaderName = "Shader.Builtin.UI";
        ShaderResource ConfigResource;
        if (!ResourceSystem::Load(ShaderName, eResource::Shader, nullptr, ConfigResource)) {
            MERROR("Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
            return false;
        }

        // ПРИМЕЧАНИЕ: Предположим, что это первый проход, так как это все, что есть в этом представлении.
        if (!ShaderSystem::Create(self->passes[0], ConfigResource.data)) {
            MERROR("Не удалось загрузить встроенный шейдер пользовательского интерфейса.");
            return false;
        }
        // ResourceSystem::Unload(ConfigResource);

        // Получите либо переопределение пользовательского шейдера, либо заданное значение по умолчанию.
        data->shader = ShaderSystem::GetShader(self->CustomShaderName ? self->CustomShaderName : ShaderName);

        data->DiffuseMapLocation  = ShaderSystem::UniformIndex(data->shader, "diffuse_texture");
        data->PropertiesLocation  = ShaderSystem::UniformIndex(data->shader, "properties");
        data->ModelLocation       = ShaderSystem::UniformIndex(data->shader, "model");

        if(!EventSystem::Register(EventSystem::DefaultRendertargetRefreshRequired, self, RenderViewOnEvent)) {
            MERROR("Не удалось прослушать событие, требующее обновления, создание не удалось.");
            return false;
        }
        
        return true;
    }

    return false;
}

void RenderViewUI::Destroy(RenderView *self)
{
    // Отменить регистрацию на мероприятии.
    EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, self->data, RenderViewOnEvent);
    MemorySystem::Free(self->data, sizeof(RenderViewUI), Memory::Renderer);
    self->data = nullptr;
}

void RenderViewUI::Resize(RenderView* self, u32 width, u32 height)
{
    if (self) {
        auto UiData = reinterpret_cast<RenderViewUI*>(self->data);
        // Проверьте, отличается ли. Если да, пересоздайте матрицу проекции.
        if (width != self->width || height != self->height) {

            self->width = width;
            self->height = height;
            UiData->ProjectionMatrix = Matrix4D::MakeOrthographicProjection(0.0f, (f32)self->width, (f32)self->height, 0.0f, UiData->NearClip, UiData->FarClip);

            for (u32 i = 0; i < self->RenderpassCount; ++i) {
                self->passes[i].RenderArea = FVec4(0, 0, width, height);
            }
        }
    }
}

bool RenderViewUI::BuildPacket(RenderView* self, LinearAllocator& FrameAllocator, void *data, RenderViewPacket &OutPacket)
{
    if (!data) {
        MWARN("RenderViewUI::BuildPacket требует действительный указатель на представление, пакет и данные.");
        return false;
    }

    if (self) {
        auto RenderViewUiData = reinterpret_cast<RenderViewUI*>(self->data);
        auto PacketData = reinterpret_cast<UiPacketData*>(data);

        OutPacket.view = self;

        // Установить матрицы и т. д.
        OutPacket.ProjectionMatrix = RenderViewUiData->ProjectionMatrix;
        OutPacket.ViewMatrix = RenderViewUiData->ViewMatrix;

        // ЗАДАЧА: временно установить расширенные данные для тестовых текстовых объектов на данный момент.
        OutPacket.ExtendedData = FrameAllocator.Allocate(sizeof(UiPacketData));
        MemorySystem::CopyMem(OutPacket.ExtendedData, PacketData, sizeof(UiPacketData));

        // Получить все геометрии из текущей сцены.
        // Итерировать все сетки и добавить их в коллекцию геометрий пакета
        for (u32 i = 0; i < PacketData->MeshData.MeshCount; ++i) {
            auto* m = PacketData->MeshData.meshes[i];
            for (u32 j = 0; j < m->GeometryCount; ++j) {
                GeometryRenderData RenderData;
                RenderData.geometry = m->geometries[j];
                RenderData.model = m->transform.GetWorld();
                OutPacket.geometries.PushBack(RenderData);
                // OutPacket.GeometryCount++;
            }
        }
        return true;
    }

    return false;
}

bool RenderViewUI::Render(const RenderView* self, const RenderViewPacket &packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData)
{
    if (self) {
        auto data = reinterpret_cast<RenderViewUI*>(self->data);

        const auto& ShaderID = data->shader->id;
        for (u32 p = 0; p < self->RenderpassCount; ++p) {
            auto pass = &self->passes[p];
            if (!RenderingSystem::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
                MERROR("RenderViewUI::Render pass index %u не удалось запустить.", p);
                return false;
            }

            if (!ShaderSystem::Use(ShaderID)) {
                MERROR("Не удалось использовать шейдер материала. Не удалось отрисовать кадр.");
                return false;
            }

            // Применить глобальные переменные
            if (!MaterialSystem::ApplyGlobal(ShaderID, FrameNumber, packet.ProjectionMatrix, packet.ViewMatrix)) {
                MERROR("Не удалось использовать применение глобальных переменных для шейдера материала. Не удалось отрисовать кадр.");
                return false;
            }

            // Нарисовать геометрию.
            const u64& count = packet.geometries.Length();
            for (u32 i = 0; i < count; ++i) {
                Material* material = nullptr;
                if (packet.geometries[i].geometry->material) {
                    material = packet.geometries[i].geometry->material;
                } else {
                    material = MaterialSystem::GetDefaultUiMaterial();
                }

                // Обновить материал, если он еще не был в этом кадре. 
                // Это предотвращает многократное обновление одного и того же материала. 
                // Его все равно нужно привязать в любом случае, 
                // поэтому результат этой проверки передается на бэкэнд, 
                // который либо обновляет внутренние привязки шейдера и привязывает их, либо только привязывает их.
                bool NeedsUpdate = material->RenderFrameNumber != FrameNumber;
                if (!MaterialSystem::ApplyInstance(material, NeedsUpdate)) {
                    MWARN("Не удалось применить материал '%s'. Пропуск рисования.", material->name);
                    continue;
                } else {
                    // Синхронизируйте номер кадра.
                    material->RenderFrameNumber = FrameNumber;
                }

                // Примените локальные
                MaterialSystem::ApplyLocal(material, packet.geometries[i].model);

                // Нарисуйте его.
                RenderingSystem::DrawGeometry(packet.geometries[i]);
            }

            // Нарисовать растровый текст
            auto PacketData = reinterpret_cast<UiPacketData*>(packet.ExtendedData);  // массив текстов
            for (u32 i = 0; i < PacketData->TextCount; ++i) {
                auto text = PacketData->texts[i];
                ShaderSystem::BindInstance(text->InstanceID);

                if (!ShaderSystem::UniformSet(data->DiffuseMapLocation, &text->data->atlas)) {
                    MERROR("Не удалось применить диффузную карту растрового шрифта.");
                    return false;
                }

                // ЗАДАЧА: цвет текста.
                static FVec4 WhiteColour {1.F, 1.F, 1.F, 1.F};  // белый
                if (!ShaderSystem::UniformSet(data->PropertiesLocation, &WhiteColour)) {
                    MERROR("Не удалось применить диффузную цветовую форму растрового шрифта.");
                    return false;
                }
                bool NeedsUpdate = text->RenderFrameNumber != FrameNumber;
                ShaderSystem::ApplyInstance(NeedsUpdate);

                // Синхронизируйте номер кадра.
                text->RenderFrameNumber = FrameNumber;

                // Применить локальные переменные
                Matrix4D model = text->transform.GetWorld();
                if(!ShaderSystem::UniformSet(data->ModelLocation, &model)) {
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
    
    return false;
}

void *RenderViewUI::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void RenderViewUI::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
