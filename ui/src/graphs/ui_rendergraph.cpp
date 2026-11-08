#include "ui_rendergraph.h"
#include "core/frame_data.h"
#include "core/systems_manager.h"
#include "renderer/viewport.h"

constexpr UiRendergraph::UiRendergraph(bool& result, const UiRendergraphConfig *config)  : Rendergraph("standard_ui_rendergraph"), uiPass("UI")
{
    // Добавьте глобальные источники.
    if (!(result = AddGlobalSource("colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Global))) {
        MERROR("Не удалось добавить глобальный источник буфера цвета.");
        return;
    }
    if (!(result = AddGlobalSource("depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global))) {
        MERROR("Не удалось добавить глобальный источник буфера глубины.");
        return;
    }

    RG_CHECK_RET(AddPass(uiPass), result);
    RG_CHECK_RET(AddPassSink("ui", "colourbuffer"), result);
    RG_CHECK_RET(AddPassSink("ui", "depthbuffer"), result)
    RG_CHECK_RET(AddPassSource("ui", "colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Global), result);
    RG_CHECK_RET(AddPassSource("ui", "depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global), result)
    RG_CHECK_RET(PassSetSinkLinkage("ui", "colourbuffer", nullptr, "colourbuffer"), result);
    RG_CHECK_RET(PassSetSinkLinkage("ui", "depthbuffer", nullptr, "depthbuffer"), result);

    if (!(result = Finalize())) {
        MERROR("Не удалось завершить построение графа рендеринга. Подробности см. в журнале.");
        return;
    }
    
}

bool UiRendergraph::Initialize()
{
    if (!LoadResource()) {
        MERROR("Не удалось загрузить ресурсы графа рендеринга.");
        return false;
    }
    
    return true;
}

bool UiRendergraph::FramePrepare(FrameData &rFrameData, Camera &CurrentCamera, Viewport &CurrentViewport, SimpleScene &scene, u32 RenderMode)
{
    RendergraphPassData& ExtData = uiPass.PassData;
    ExtData.viewport = &CurrentViewport;
    ExtData.ViewMatrix = Matrix4D::MakeIdentity();
    ExtData.ProjectionMatrix = CurrentViewport.projection;
    ExtData.DoExecute = true;

    // Визуализируемые объекты.
    uiPass.RenderData.renderables = DArray<UiRenderable>(rFrameData.FrameAllocator);
    UiSystem* UiSys = SystemsManager::GetUiSystem();
    if (!UiSys->Render(nullptr, rFrameData, uiPass.RenderData)) {
        MERROR("Стандартной системе пользовательского интерфейса не удалось выполнить отрисовку.");
    }
    return true;
}
