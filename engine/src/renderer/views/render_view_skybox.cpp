#include "render_view_skybox.hpp"
#include "renderer/renderpass.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "systems/shader_system.hpp"
#include "resources/skybox.hpp"

RenderViewSkybox::~RenderViewSkybox()
{
}

void RenderViewSkybox::Resize(u32 width, u32 height)
{
    // Проверьте, отличается ли. Если да, пересоздайте матрицу проекции.
    if (width != this->width || height != this->height) {

        this->width = width;
        this->height = height;
        f32 aspect = (f32)this->width / this->height;
        ProjectionMatrix = Matrix4D::MakeFrustumProjection(fov, aspect, NearClip, FarClip);

        for (u32 i = 0; i < RenderpassCount; ++i) {
            passes[i]->RenderArea = FVec4(0, 0, width, height);
        }
    }
}

bool RenderViewSkybox::BuildPacket(void *data, Packet &OutPacket)
{
    SkyboxPacketData* SkyboxData = reinterpret_cast<SkyboxPacketData*>(data);

    OutPacket.view = this;

    // Матрицы множеств и т.д.
    OutPacket.ProjectionMatrix = ProjectionMatrix;
    OutPacket.ViewMatrix = WorldCamera->GetView();
    OutPacket.ViewPosition = WorldCamera->GetPosition();

    // Просто установите расширенные данные для данных скайбокса
    OutPacket.ExtendedData = SkyboxData;
    return true;
}

bool RenderViewSkybox::Render(const Packet &packet, u64 FrameNumber, u64 RenderTargetIndex)
{

    SkyboxPacketData* SkyboxData = reinterpret_cast<SkyboxPacketData*>(packet.ExtendedData);

    for (u32 p = 0; p < RenderpassCount; ++p) {
        Renderpass* pass = passes[p];
        if (!Renderer::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
            MERROR("RenderViewSkybox::Render индекс прохода %u ошибка запуска.", p);
            return false;
        }

        if (!ShaderSystem::GetInstance()->Use(ShaderID)) {
            MERROR("Не удалось использовать шейдер скайбокса. Не удалось отрисовать кадр.");
            return false;
        }

        // Получить матрицу вида, но обнулить позицию, чтобы скайбокс остался на экране.
        Matrix4D ViewMatrix = WorldCamera->GetView();
        ViewMatrix.data[12] = 0.0f;
        ViewMatrix.data[13] = 0.0f;
        ViewMatrix.data[14] = 0.0f;

        // Применить глобальные переменные
        // TODO: Это ужасно. Нужно привязать по идентификатору.
        ShaderSystem::GetInstance()->GetShader(ShaderID)->BindGlobals();
        if (!ShaderSystem::GetInstance()->UniformSet(ProjectionLocation, &packet.ProjectionMatrix)) {
            MERROR("Не удалось применить единообразие проекции скайбокса.");
            return false;
        }
        if (!ShaderSystem::GetInstance()->UniformSet(ViewLocation, &ViewMatrix)) {
            MERROR("Не удалось применить единообразие вида скайбокса.");
            return false;
        }
        ShaderSystem::GetInstance()->ApplyGlobal();

        // Экземпляр
        ShaderSystem::GetInstance()->BindInstance(SkyboxData->sb->InstanceID);
        if (!ShaderSystem::GetInstance()->UniformSet(CubeMapLocation, &SkyboxData->sb->cubemap)) {
            MERROR("Не удалось применить единообразие кубической карты скайбокса.");
            return false;
        }
        bool NeedsUpdate = SkyboxData->sb->RenderFrameNumber != FrameNumber;
        ShaderSystem::GetInstance()->ApplyInstance(NeedsUpdate);

        // Синхронизировать номер кадра.
        SkyboxData->sb->RenderFrameNumber = FrameNumber;

        // Нарисовать его.
        GeometryRenderData RenderData = {};
        RenderData.geometry = SkyboxData->sb->g;
        Renderer::DrawGeometry(RenderData);

        if (!Renderer::RenderpassEnd(pass)) {
            MERROR("RenderViewSkybox::Render проход под индексом %u не завершился.", p);
            return false;
        }
    }

    return true;
}
