#include "skybox_pass.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "resources/skybox.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

SkyboxPass::~SkyboxPass()
{
    Destroy();
}

void SkyboxPass::Destroy()
{
    SystemsManager::GetRenderingSystem()->RenderpassDestroy(&pass);
}

bool SkyboxPass::Initialize()
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Цветовое вложение(Colour attachment)
    RenderTargetAttachmentConfig SkyboxTargetColour {
        .type           = RenderTargetAttachmentType::Colour,
        .source         = RenderTargetAttachmentSource::Default,
        .LoadOperation  = RenderTargetAttachmentLoadOperation::DontCare,
        .StoreOperation = RenderTargetAttachmentStoreOperation::Store,
        .PresentAfter   = false
    };

    // Конфигурация подпрохода рендеринга
    RenderpassConfig SkyboxPass;
    SkyboxPass.depth                  = 1.F;
    SkyboxPass.stencil                = 0;
    SkyboxPass.target.AttachmentCount = 1;
    SkyboxPass.target.attachments     = &SkyboxTargetColour;
    
    pass.ClearColour = FVec4(0.F, 0.F, 0.2F, 1.F);
    pass.ClearFlags = RenderpassClearFlag::ColourBuffer;
    pass.name = "Renderpass.Bultin.Skybox";
    pass.RenderTargetCount = renderer->GetWindowAttachmentCount();
    pass.targets = nul_alloc(RenderTarget, pass.RenderTargetCount, Memory::Array);

    if (!SystemsManager::GetRenderingSystem()->RenderpassCreate(SkyboxPass, pass, false)) { // 
        MERROR("SkyboxPass::Initialize — Не удалось создать проход рендеринга «%.*s»",  pass.name.Length(), pass.name.Data());
        return false;
    }

    // Загрузка шейдера скайбокса.
    const char* SkyboxShaderName = "Shader.Builtin.Skybox";
    ShaderResource SkyboxShaderConfigResource;
    if (!ResourceSystem::Load(SkyboxShaderName, nullptr, SkyboxShaderConfigResource)) {
        MERROR("Не удалось загрузить ресурс шейдера скайбокса.");
        return false;
    }
    auto& SkyboxShaderConfig = SkyboxShaderConfigResource.data;
    if (!(shader = ShaderSystem::CreateShader(pass, SkyboxShaderConfig))) {
        MERROR("Не удалось создать шейдер скайбокса.");
        return false;
    }

    ResourceSystem::Unload(SkyboxShaderConfigResource);

    ProjectionLocation = shader->UniformLocation("projection");
    ViewLocation       = shader->UniformLocation("view");
    CubeMapLocation    = shader->UniformLocation("cube_texture");

    return true;
}

bool SkyboxPass::Render(FrameData &rFrameData)
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Привязать область просмотра
    renderer->SetActiveViewport(PassData.viewport);

    if (!renderer->RenderpassBegin(&pass, pass.targets[rFrameData.RenderTargetIndex])) {
        MERROR("Не удалось запустить проход скайбокса.");
        return false;
    }

    if (skybox) {
        ShaderSystem::Use(shader->id);

        // Получить матрицу вида, но обнулить положение, чтобы скайбокс остался на экране.
        auto ViewMatrix = PassData.ViewMatrix;
        ViewMatrix.data[12] = 0.F;
        ViewMatrix.data[13] = 0.F;
        ViewMatrix.data[14] = 0.F;

        // Применить глобальные переменные
        shader->BindGlobals();
        if (!ShaderSystem::UniformSet(ProjectionLocation, &PassData.ProjectionMatrix)) {
            MERROR("Не удалось применить единообразие проекции скайбокса.");
            return false;
        }
        if (!ShaderSystem::UniformSet(ViewLocation, &ViewMatrix)) {
            MERROR("Не удалось применить единообразие вида скайбокса.");
            return false;
        }
        ShaderSystem::ApplyGlobal(true, rFrameData);

        // Экземпляр
        ShaderSystem::BindInstance(skybox->InstanceID);
        if (!ShaderSystem::UniformSet(CubeMapLocation, &skybox->cubemap)) {
            MERROR("Не удалось применить единообразие кубической карты скайбокса.");
            return false;
        }
        bool NeedsUpdate = skybox->RenderFrameNumber != rFrameData.RendererFrameNumber || skybox->DrawIndex != rFrameData.DrawIndex;
        ShaderSystem::ApplyInstance(NeedsUpdate, rFrameData);

        // Синхронизировать номер кадра и индекс отрисовки.
        skybox->RenderFrameNumber = rFrameData.RendererFrameNumber;
        skybox->DrawIndex = rFrameData.DrawIndex;

        // Нарисовать его.
        GeometryRenderData RenderData{};
        RenderData.geometry = skybox->geometry;
        renderer->DrawGeometry(RenderData);
    }

    if (!renderer->RenderpassEnd(&pass)) {
        MERROR("Не удалось завершить проход скайбокса.");
        return false;
    }

    return true;
}
