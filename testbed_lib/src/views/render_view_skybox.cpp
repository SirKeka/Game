#include "render_view_skybox.h"
#include "memory/linear_allocator.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderpass.h" 
#include "systems/camera_system.hpp"
#include "systems/render_view_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "resources/skybox.h"

bool RenderViewSkybox::OnRegistered(RenderView* self)
{
   if (self) {
        auto SkyboxData = new RenderViewSkybox();
        SkyboxData->WorldCamera = CameraSystem::Instance()->GetDefault();
        self->data = SkyboxData;

        // Встроенный шейдер скабокса.
        const char* ShaderName = "Shader.Builtin.Skybox";
        ShaderResource ConfigResource;
        if (!ResourceSystem::Load(ShaderName, eResource::Shader, nullptr, ConfigResource)) {
            MERROR("Не удалось загрузить встроенный шейдер скайбокса.");
            return false;
        }

        // ПРИМЕЧАНИЕ: предполагается первый проход, так как это все, что есть в этом представлении.
        if (!ShaderSystem::Create(self->passes[0],ConfigResource.data)) {
            MERROR("Не удалось загрузить встроенный шейдер скайбокса.");
            return false;
        }

        // ResourceSystem::Unload(ConfigResource);

        // Получить указатель на шейдер.
        SkyboxData->shader = ShaderSystem::GetShader(self->CustomShaderName ? self->CustomShaderName : ShaderName);

        SkyboxData->ProjectionLocation = ShaderSystem::UniformIndex(SkyboxData->shader, "projection");
        SkyboxData->ViewLocation = ShaderSystem::UniformIndex(SkyboxData->shader, "view");
        SkyboxData->CubeMapLocation = ShaderSystem::UniformIndex(SkyboxData->shader, "cube_texture");

        if(!EventSystem::Register(EventSystem::DefaultRendertargetRefreshRequired, self, RenderViewOnEvent)) {
            MERROR("Не удалось прослушать событие, требующее обновления, создание не удалось.");
            return false;
        }

        return true;
   }
   
    return false;
}

RenderViewSkybox::~RenderViewSkybox()
{
    // Отменить регистрацию на мероприятии.
    EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, this, RenderViewOnEvent);
}

void RenderViewSkybox::Resize(RenderView* self, u32 width, u32 height)
{
    if (self) {
        auto SkyboxData = reinterpret_cast<RenderViewSkybox*>(self->data);
        // Проверьте, отличается ли. Если да, пересоздайте матрицу проекции.
        if (width != self->width || height != self->height) {

            self->width = width;
            self->height = height;
            f32 aspect = (f32)self->width / self->height;
            SkyboxData->ProjectionMatrix = Matrix4D::MakeFrustumProjection(SkyboxData->fov, aspect, SkyboxData->NearClip, SkyboxData->FarClip);

            for (u32 i = 0; i < self->RenderpassCount; ++i) {
                self->passes[i].RenderArea = FVec4(0, 0, width, height);
            }
        }
    }
}

bool RenderViewSkybox::BuildPacket(RenderView* self, class LinearAllocator& FrameAllocator, void *data, RenderView::Packet &OutPacket)
{
    if (self) {
        auto SkyboxData = reinterpret_cast<RenderViewSkybox*>(self->data);
        OutPacket.view = self;

        // Матрицы множеств и т.д.
        OutPacket.ProjectionMatrix = SkyboxData->ProjectionMatrix;
        OutPacket.ViewMatrix = SkyboxData->WorldCamera->GetView();
        OutPacket.ViewPosition = SkyboxData->WorldCamera->GetPosition();

        // Просто установите расширенные данные для данных скайбокса
        u64 size = sizeof(SkyboxPacketData);
        OutPacket.ExtendedData = FrameAllocator.Allocate(size);
        MemorySystem::CopyMem(OutPacket.ExtendedData, data, size);
        return true;
    }

    return false;
}

bool RenderViewSkybox::Render(RenderView* self, const RenderView::Packet &packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData)
{
    if (self) {
        auto data = reinterpret_cast<RenderViewSkybox*>(self->data);
        auto sPacketData = reinterpret_cast<SkyboxPacketData*>(packet.ExtendedData);
        const auto& ShaderID = data->shader->id;

        for (u32 p = 0; p < self->RenderpassCount; ++p) {
            auto& pass = self->passes[p];
            if (!RenderingSystem::RenderpassBegin(&pass, pass.targets[RenderTargetIndex])) {
                MERROR("RenderViewSkybox::Render индекс прохода %u ошибка запуска.", p);
                return false;
            }
            if (sPacketData && sPacketData->sb) {
                if (!ShaderSystem::Use(ShaderID)) {
                    MERROR("Не удалось использовать шейдер скайбокса. Не удалось отрисовать кадр.");
                    return false;
                }

                // Получить матрицу вида, но обнулить позицию, чтобы скайбокс остался на экране.
                auto ViewMatrix = data->WorldCamera->GetView();
                ViewMatrix(12) = ViewMatrix(13) = ViewMatrix(14) = 0.F;

                // Применить глобальные переменные
                // ЗАДАЧА: Это ужасно. Нужно привязать по идентификатору.
                ShaderSystem::GetShader(ShaderID)->BindGlobals();
                if (!ShaderSystem::UniformSet(data->ProjectionLocation, &packet.ProjectionMatrix)) {
                    MERROR("Не удалось применить единообразие проекции скайбокса.");
                    return false;
                }
                if (!ShaderSystem::UniformSet(data->ViewLocation, &ViewMatrix)) {
                    MERROR("Не удалось применить единообразие вида скайбокса.");
                    return false;
                }
                ShaderSystem::ApplyGlobal();

                // Экземпляр
                ShaderSystem::BindInstance(sPacketData->sb->InstanceID);
                if (!ShaderSystem::UniformSet(data->CubeMapLocation, &sPacketData->sb->cubemap)) {
                    MERROR("Не удалось применить единообразие кубической карты скайбокса.");
                    return false;
                }
                bool NeedsUpdate = sPacketData->sb->RenderFrameNumber != FrameNumber;
                ShaderSystem::ApplyInstance(NeedsUpdate);

                // Синхронизировать номер кадра.
                sPacketData->sb->RenderFrameNumber = FrameNumber;

                // Нарисовать его.
                GeometryRenderData RenderData = {};
                RenderData.geometry = sPacketData->sb->g;
                RenderingSystem::DrawGeometry(RenderData);
            }

            if (!RenderingSystem::RenderpassEnd(&pass)) {
                MERROR("RenderViewSkybox::Render проход под индексом %u не завершился.", p);
                return false;
            }
        }

        return true;
    }
    
    return false;
}

void *RenderViewSkybox::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void RenderViewSkybox::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
