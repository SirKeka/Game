#include "render_view_skybox.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/renderpass.hpp"
#include "renderer/camera.hpp"
#include "renderer/rendering_system.hpp"
#include "systems/render_view_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/shader_system.hpp"
#include "resources/skybox.hpp"

RenderViewSkybox::RenderViewSkybox(u16 id, const Config &config)
: RenderView(id, config),
shader(),
fov(Math::DegToRad(45.F)), NearClip(0.1F), FarClip(1000.F), 
ProjectionMatrix(Matrix4D::MakeFrustumProjection(fov, 1280 / 720.F, NearClip, FarClip)), 
WorldCamera(CameraSystem::Instance()->GetDefault()), 
// locations(),
ProjectionLocation(), ViewLocation(), CubeMapLocation() 
{
    // Встроенный шейдер скабокса.
    const char* ShaderName = "Shader.Builtin.Skybox";
    ShaderResource ConfigResource;
    if (!ResourceSystem::Load(ShaderName, eResource::Shader, nullptr, ConfigResource)) {
        MERROR("Не удалось загрузить встроенный шейдер скайбокса.");
        return;
    }

    // ПРИМЕЧАНИЕ: предполагается первый проход, так как это все, что есть в этом представлении.
    if (!ShaderSystem::Create(passes[0],ConfigResource.data)) {
        MERROR("Не удалось загрузить встроенный шейдер скайбокса.");
        return;
    }

    // ResourceSystem::Unload(ConfigResource);

    // Получить указатель на шейдер.
    shader = ShaderSystem::GetShader(CustomShaderName ? CustomShaderName : ShaderName);

    ProjectionLocation = ShaderSystem::UniformIndex(shader, "projection");
    ViewLocation = ShaderSystem::UniformIndex(shader, "view");
    CubeMapLocation = ShaderSystem::UniformIndex(shader, "cube_texture");

    if(!EventSystem::Register(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, this, RenderViewOnEvent)) {
        MERROR("Не удалось прослушать событие, требующее обновления, создание не удалось.");
        return;
    }
}

RenderViewSkybox::~RenderViewSkybox()
{
    // Отменить регистрацию на мероприятии.
    EventSystem::Unregister(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, this, RenderViewOnEvent);
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
            passes[i].RenderArea = FVec4(0, 0, width, height);
        }
    }
}

bool RenderViewSkybox::BuildPacket(class LinearAllocator& FrameAllocator, void *data, Packet &OutPacket)
{
    auto SkyboxData = reinterpret_cast<SkyboxPacketData*>(data);

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
    auto SkyboxData = reinterpret_cast<SkyboxPacketData*>(packet.ExtendedData);
    const auto& ShaderID = shader->id;

    for (u32 p = 0; p < RenderpassCount; ++p) {
        auto& pass = passes[p];
        if (!RenderingSystem::RenderpassBegin(&pass, pass.targets[RenderTargetIndex])) {
            MERROR("RenderViewSkybox::Render индекс прохода %u ошибка запуска.", p);
            return false;
        }

        if (!ShaderSystem::Use(ShaderID)) {
            MERROR("Не удалось использовать шейдер скайбокса. Не удалось отрисовать кадр.");
            return false;
        }

        // Получить матрицу вида, но обнулить позицию, чтобы скайбокс остался на экране.
        auto ViewMatrix = WorldCamera->GetView();
        ViewMatrix(12) = ViewMatrix(13) = ViewMatrix(14) = 0.f;

        // Применить глобальные переменные
        // ЗАДАЧА: Это ужасно. Нужно привязать по идентификатору.
        ShaderSystem::GetShader(ShaderID)->BindGlobals();
        if (!ShaderSystem::UniformSet(ProjectionLocation, &packet.ProjectionMatrix)) {
            MERROR("Не удалось применить единообразие проекции скайбокса.");
            return false;
        }
        if (!ShaderSystem::UniformSet(ViewLocation, &ViewMatrix)) {
            MERROR("Не удалось применить единообразие вида скайбокса.");
            return false;
        }
        ShaderSystem::ApplyGlobal();

        // Экземпляр
        ShaderSystem::BindInstance(SkyboxData->sb->InstanceID);
        if (!ShaderSystem::UniformSet(CubeMapLocation, &SkyboxData->sb->cubemap)) {
            MERROR("Не удалось применить единообразие кубической карты скайбокса.");
            return false;
        }
        bool NeedsUpdate = SkyboxData->sb->RenderFrameNumber != FrameNumber;
        ShaderSystem::ApplyInstance(NeedsUpdate);

        // Синхронизировать номер кадра.
        SkyboxData->sb->RenderFrameNumber = FrameNumber;

        // Нарисовать его.
        GeometryRenderData RenderData = {};
        RenderData.gid = SkyboxData->sb->g;
        RenderingSystem::DrawGeometry(RenderData);

        if (!RenderingSystem::RenderpassEnd(&pass)) {
            MERROR("RenderViewSkybox::Render проход под индексом %u не завершился.", p);
            return false;
        }
    }

    return true;
}

void *RenderViewSkybox::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void RenderViewSkybox::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
