#include "ui_rendergraph.h"
#include "core/frame_data.h"
#include "core/systems_manager.h"
#include "renderer/viewport.h"

bool UiRendergraph::Create(const UiRendergraphConfig *config)
{
    name = "ui_rendergraph";
    // Добавьте глобальные источники.
    if (!AddGlobalSource("colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера цвета.");
        return false;
    }

    if (!AddGlobalSource("depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера глубины.");
        return false;
    }

    uiPass.name = "UI";
    RG_CHECK(AddPass(uiPass));
    RG_CHECK(AddPassSink("UI",        "colourbuffer"));
    RG_CHECK(AddPassSink("UI",        "depthbuffer"));
    RG_CHECK(AddPassSource("UI",      "colourbuffer", RendergraphSourceType::RenderTargetColour,       RendergraphSourceOrigin::Other));
    RG_CHECK(AddPassSource("UI",      "depthbuffer",  RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global));
    RG_CHECK(PassSetSinkLinkage("UI", "colourbuffer", nullptr, "colourbuffer"));
    RG_CHECK(PassSetSinkLinkage("UI", "depthbuffer",  nullptr, "depthbuffer"));

    if (!Finalize()) {
        MERROR("Не удалось завершить построение графа рендеринга. Подробности см. в журнале.");
        return false;
    }
    return true;
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
