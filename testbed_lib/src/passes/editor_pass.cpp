#include "editor_pass.h"
#include <core/frame_data.h>
#include <core/systems_manager.h>
#include <systems/shader_system.h>
#include <renderer/rendering_system.h>
#include <resources/shader.h>

EditorPass::~EditorPass()
{
    Destroy();
}

void EditorPass::Destroy()
{
    SystemsManager::GetRenderingSystem()->RenderpassDestroy(&pass);
}

bool EditorPass::Initialize()
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Конфигурация подпрохода рендеринга
    pass.name = "Renderpass.Testbed.EditorWorld";
    pass.ClearColour = FVec4(0.F, 0.F, 0.F, 1.F);
    pass.ClearFlags = RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer;
    pass.RenderTargetCount = renderer->GetWindowAttachmentCount();
    pass.targets = nul_alloc(RenderTarget, pass.RenderTargetCount, Memory::Array);
    RenderpassConfig ePass{};
    ePass.depth = 1.F;
    ePass.target.AttachmentCount = 2;
    ePass.target.attachments = (RenderTargetAttachmentConfig*)MemorySystem::Allocate(sizeof(RenderTargetAttachmentConfig) * ePass.target.AttachmentCount, Memory::Array);

    // Привязка цвета
    auto& eTargetColour = ePass.target.attachments[0];
    eTargetColour.type = RenderTargetAttachmentType::Colour;
    eTargetColour.source = RenderTargetAttachmentSource::Default;
    eTargetColour.LoadOperation = RenderTargetAttachmentLoadOperation::Load;
    eTargetColour.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    eTargetColour.PresentAfter = false;

    // Привязка глубины
    auto& eTargetDepth = ePass.target.attachments[1];
    eTargetDepth.type = RenderTargetAttachmentType::Depth;
    eTargetDepth.source = RenderTargetAttachmentSource::Default;;
    eTargetDepth.LoadOperation = RenderTargetAttachmentLoadOperation::DontCare;
    eTargetDepth.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    eTargetDepth.PresentAfter = false;

    if (!renderer->RenderpassCreate(ePass, pass, false)) {
        MERROR("EditorPass::Initialize — Не удалось создать проход рендеринга «%s»", pass.name.c_str());
        return false;
    }

    // Загрузите отладочный шейдер Colour3D и получите унифицированные местоположения шейдера.
    const char* Colour3dShaderName = "Shader.Builtin.ColourShader3D";
    if (!(shader = ShaderSystem::GetShader(Colour3dShaderName))) {
        MERROR("Не удалось получить шейдер Colour3D!");
        return false;
    }
    projection = shader->UniformLocation("projection");
    view       = shader->UniformLocation("view");
    model      = shader->UniformLocation("model");

    return true;
}

bool EditorPass::Render(FrameData& rFrameData)
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Привязать область просмотра
    renderer->SetActiveViewport(PassData.viewport);

    if (!renderer->RenderpassBegin(&pass, pass.targets[rFrameData.RenderTargetIndex])) {
        MERROR("Не удалось запустить проход рендеринга редактора.");
        return false;
    }

    ShaderSystem::Use(shader->id);

    shader->BindGlobals();
    // Глобальные переменные
    bool NeedsUpdate = rFrameData.RendererFrameNumber != shader->RenderFrameNumber || shader->DrawIndex != rFrameData.DrawIndex;
    if (NeedsUpdate) {
        ShaderSystem::UniformSet(projection, &PassData.ProjectionMatrix);
        ShaderSystem::UniformSet(view, &PassData.ViewMatrix);
    }
    ShaderSystem::ApplyGlobal(NeedsUpdate, rFrameData);

    // Синхронизировать номер кадра и индекс отрисовки.
    shader->RenderFrameNumber = rFrameData.RendererFrameNumber;
    shader->DrawIndex = rFrameData.DrawIndex;

    const u32 DebugGeometryCount = DebugGeometries.Size();
    for (u32 i = 0; i < DebugGeometryCount; ++i) {
        // ПРИМЕЧАНИЕ: Нет униформных переменных уровня экземпляра, которые нужно задать.
        auto& RenderData = DebugGeometries[i];

        // Установить матрицу модели.
        ShaderSystem::BindLocal();
        ShaderSystem::UniformSet(model, &RenderData.model);
        ShaderSystem::ApplyLocal(rFrameData);

        // Нарисовать её.
        renderer->DrawGeometry(RenderData);
    }

    if (!renderer->RenderpassEnd(&pass)) {
        MERROR("Не удалось завершить проход рендеринга редактора.");
        return false;
    }

    return true;
}
