#include "ui_pass.h"
// #include "ui_system.h"
#include <core/systems_manager.h>
#include <renderer/rendergraph.h>
#include <renderer/rendering_system.h>
#include <resources/ui_text.h>
#include <systems/material_system.h>
#include <systems/resource_system.h>
#include <systems/shader_system.h>

void UiPass::Destroy()
{
    // Уничтожение прохода.
    SystemsManager::GetRenderingSystem()->RenderpassDestroy(&pass);
}

bool UiPass::Initialize()
{
    auto rSystem = SystemsManager::GetRenderingSystem();
    #pragma region Конфигурация подпрохода рендеринга
    RenderTargetAttachmentConfig attachments[2];
    // Привязка цвета
    auto& ColourAttachment = attachments[0];
    ColourAttachment.type           = RenderTargetAttachmentType::Colour;
    ColourAttachment.source         = RenderTargetAttachmentSource::Default;
    ColourAttachment.LoadOperation  = RenderTargetAttachmentLoadOperation::Load;
    ColourAttachment.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    ColourAttachment.PresentAfter   = true;
    
    // Глубина/привязка трафарета.
    auto& DepthAttachment = attachments[1];
    DepthAttachment.type           = RenderTargetAttachmentType::Depth | RenderTargetAttachmentType::Stencil;
    DepthAttachment.source         = RenderTargetAttachmentSource::Default;
    DepthAttachment.LoadOperation  = RenderTargetAttachmentLoadOperation::DontCare;
    DepthAttachment.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    DepthAttachment.PresentAfter   = false;
    
    RenderpassConfig UiPass {
        .depth   = 1.F,
        .stencil = 0,
        .target  = { 2, attachments }
    };
    
    pass.name                     = "Renderpass.Bultin.UI";
    pass.ClearFlags               = RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer;
    pass.RenderTargetCount        = rSystem->GetWindowAttachmentCount();
    pass.targets                  = nul_alloc(RenderTarget, pass.RenderTargetCount, Memory::Array);
    pass.ClearColour.z = 0.2F; pass.ClearColour.w = 1.F;

    if (!rSystem->RenderpassCreate(UiPass, pass, false)) {
        MERROR("UI pass — Не удалось создать проход рендеринга «%s»", pass.name.c_str());
        return false;
    };
    #pragma endregion

    // Загрузка шейдера.
    const char* suiShaderName = "Shader.StandardUI";
    ShaderResource confres;
    if (!ResourceSystem::Load(suiShaderName, nullptr, confres)) {
        MERROR("Не удалось загрузить ресурс шейдера StandardUI.");
        return false;
    }

    // ПРИМЕЧАНИЕ: Предполагается первый проход, так как это всё, что есть в этом представлении.
    if (!(shader = ShaderSystem::CreateShader(pass, confres.data))) {
        MERROR("Не удалось создать шейдер StandardUI.");
        return false;
    }

    // Получите либо переопределение пользовательского шейдера, либо заданный по умолчанию.
    projection = shader->UniformLocation("projection");
    view       = shader->UniformLocation("view");
    DiffuseMap = shader->UniformLocation("diffuse_texture");
    properties = shader->UniformLocation("properties");
    model      = shader->UniformLocation("model");

    return true;
}

bool UiPass::Render(FrameData &rFrameData)
{
    auto rSystem = SystemsManager::GetRenderingSystem();
    // Привязка области просмотра
    rSystem->SetActiveViewport(PassData.viewport);
    rSystem->SetDepthTestEnabled(false);

    if (!rSystem->RenderpassBegin(&pass, pass.targets[rFrameData.RenderTargetIndex])) {
        MERROR("Не удалось запустить проход рендеринга пользовательского интерфейса.");
        return false;
    }

    if (!ShaderSystem::Use(shader->id)) {
        MERROR("Не удалось использовать шейдер. Не удалось отрисовать кадр.");
        return false;
    }

    // Применить глобальные переменные
    ShaderSystem::UniformSet(projection, &PassData.ProjectionMatrix);
    ShaderSystem::UniformSet(view, &PassData.ViewMatrix);
    ShaderSystem::ApplyGlobal(true, rFrameData);

    // Синхронизируйте номер кадра.
    shader->RenderFrameNumber = rFrameData.RendererFrameNumber;

    // Отрисовка геометрий.
    const u32 rCount = RenderData.renderables.Length();
    for (u32 i = 0; i < rCount; ++i) {
        const auto& renderable = RenderData.renderables[i];

        // Отобразить геометрию маски отсечения, если она существует.
        if (renderable.ClipMaskRenderData) {
            // Включить запись, отключить проверку.
            rSystem->SetDepthTestEnabled(false);
            rSystem->SetStencilReference((u32)renderable.ClipMaskRenderData->UniqueID);
            rSystem->SetStencilTestEnabled(true);
            rSystem->SetStencilWriteMask(0xFF);
            rSystem->SetStencilOp(RendererStencilOp::Replace, RendererStencilOp::Replace, RendererStencilOp::Replace, RendererCompareOp::Always);

            ShaderSystem::BindLocal();
            ShaderSystem::UniformSet(model, &renderable.ClipMaskRenderData->model);
            ShaderSystem::ApplyLocal(rFrameData);
            // Нарисуйте геометрию маски обрезки.
            rSystem->DrawGeometry(*renderable.ClipMaskRenderData);

            // Отключить запись, включить проверку.
            rSystem->SetStencilWriteMask(0x00);
            rSystem->SetStencilTestEnabled(true);
            rSystem->SetStencilCompareMask(0xFF);
            rSystem->SetStencilOp(RendererStencilOp::Keep, RendererStencilOp::Replace, RendererStencilOp::Keep, RendererCompareOp::Equal);
        } else {
            rSystem->SetStencilWriteMask(0x00);
            rSystem->SetStencilTestEnabled(false);
        }

        // Применить экземпляр
        bool NeedsUpdate = *renderable.FrameNumber != rFrameData.RendererFrameNumber;
        ShaderSystem::BindInstance(*renderable.InstanceID);
        // ПРИМЕЧАНИЕ: При необходимости расширьте это до структуры.
        ShaderSystem::UniformSet(properties, &renderable.RenderData.DiffuseColour);
        auto atlas = renderable.AtlasOverride ? renderable.AtlasOverride : RenderData.UiAtlas;
        ShaderSystem::UniformSet(DiffuseMap, atlas);
        ShaderSystem::ApplyInstance(NeedsUpdate, rFrameData);

        // Примените локальные переменные.
        ShaderSystem::BindLocal();
        ShaderSystem::UniformSet(model, &renderable.RenderData.model);
        ShaderSystem::ApplyLocal(rFrameData);

        // Нарисуйте его.
        rSystem->DrawGeometry(renderable.RenderData);

        // Отключите проверку трафарета, если она была включена.
        if (renderable.ClipMaskRenderData) {
            // Отключите проверку трафарета.
            rSystem->SetStencilTestEnabled(false);
            rSystem->SetStencilOp(RendererStencilOp::Keep, RendererStencilOp::Keep, RendererStencilOp::Keep, RendererCompareOp::Always);
        }

        // Синхронизируйте номер кадра.
        *renderable.FrameNumber = rFrameData.RendererFrameNumber;
        *renderable.DrawIndex = rFrameData.DrawIndex;
    }

    if (!rSystem->RenderpassEnd(&pass)) {
        MERROR("Не удалось завершить проход рендеринга пользовательского интерфейса.");
        return false;
    }

    return true;
}
