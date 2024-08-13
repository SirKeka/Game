#include "render_view_skybox.hpp"
#include "renderer/renderpass.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "systems/shader_system.hpp"
#include "resources/skybox.hpp"

RenderViewSkybox::RenderViewSkybox(u16 id, MString &name, KnownType type, u8 RenderpassCount, const char *CustomShaderName)
: RenderView(id, name, type, RenderpassCount, CustomShaderName), 
ShaderID(ShaderSystem::GetInstance()->GetID(CustomShaderName ? CustomShaderName : "Shader.Builtin.Skybox")),
fov(Math::DegToRad(45.F)), NearClip(0.1F), FarClip(1000.F), 
ProjectionMatrix(Matrix4D::MakeFrustumProjection(fov, 1280 / 720.F, NearClip, FarClip)), 
WorldCamera(CameraSystem::Instance()->GetDefault()), 
// locations(),
ProjectionLocation(), ViewLocation(), CubeMapLocation() 
{
    ShaderSystem* ShaderSystemInst = ShaderSystem::GetInstance();
    Shader* SkyboxShader = ShaderSystemInst->GetShader(CustomShaderName ? CustomShaderName : "Shader.Builtin.Skybox");
    ProjectionLocation = ShaderSystemInst->UniformIndex(SkyboxShader, "projection");
    ViewLocation = ShaderSystemInst->UniformIndex(SkyboxShader, "view");
    CubeMapLocation = ShaderSystemInst->UniformIndex(SkyboxShader, "cube_texture");
}

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

bool RenderViewSkybox::BuildPacket(void *data, Packet &OutPacket) const
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

bool RenderViewSkybox::Render(const Packet &packet, u64 FrameNumber, u64 RenderTargetIndex) const
{

    SkyboxPacketData* SkyboxData = reinterpret_cast<SkyboxPacketData*>(packet.ExtendedData);
    ShaderSystem* ShaderSystemInst = ShaderSystem::GetInstance();

    for (u32 p = 0; p < RenderpassCount; ++p) {
        Renderpass* pass = passes[p];
        if (!Renderer::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
            MERROR("RenderViewSkybox::Render индекс прохода %u ошибка запуска.", p);
            return false;
        }

        if (!ShaderSystemInst->Use(ShaderID)) {
            MERROR("Не удалось использовать шейдер скайбокса. Не удалось отрисовать кадр.");
            return false;
        }

        // Получить матрицу вида, но обнулить позицию, чтобы скайбокс остался на экране.
        Matrix4D ViewMatrix = WorldCamera->GetView();
        ViewMatrix(12) = ViewMatrix(13) = ViewMatrix(14) = 0.f;

        // Применить глобальные переменные
        // TODO: Это ужасно. Нужно привязать по идентификатору.
        ShaderSystemInst->GetShader(ShaderID)->BindGlobals();
        if (!ShaderSystemInst->UniformSet(ProjectionLocation, &packet.ProjectionMatrix)) {
            MERROR("Не удалось применить единообразие проекции скайбокса.");
            return false;
        }
        if (!ShaderSystemInst->UniformSet(ViewLocation, &ViewMatrix)) {
            MERROR("Не удалось применить единообразие вида скайбокса.");
            return false;
        }
        ShaderSystemInst->ApplyGlobal();

        // Экземпляр
        ShaderSystemInst->BindInstance(SkyboxData->sb->InstanceID);
        if (!ShaderSystemInst->UniformSet(CubeMapLocation, &SkyboxData->sb->cubemap)) {
            MERROR("Не удалось применить единообразие кубической карты скайбокса.");
            return false;
        }
        bool NeedsUpdate = SkyboxData->sb->RenderFrameNumber != FrameNumber;
        ShaderSystemInst->ApplyInstance(NeedsUpdate);

        // Синхронизировать номер кадра.
        SkyboxData->sb->RenderFrameNumber = FrameNumber;

        // Нарисовать его.
        GeometryRenderData RenderData = {};
        RenderData.gid = SkyboxData->sb->g;
        Renderer::DrawGeometry(RenderData);

        if (!Renderer::RenderpassEnd(pass)) {
            MERROR("RenderViewSkybox::Render проход под индексом %u не завершился.", p);
            return false;
        }
    }

    return true;
}
