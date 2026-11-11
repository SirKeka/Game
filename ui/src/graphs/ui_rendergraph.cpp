#include "ui_rendergraph.h"
#include "core/frame_data.h"
#include "core/systems_manager.h"
#include "renderer/viewport.h"

bool UiRendergraph::Create(UiRendergraphConfig *config)
{
    name = "ui_rendergraph";
    // Добавление глобальных источников.
    if (!AddGlobalSource("colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера цвета (colourbuffer).");
        return false;
    }

    if (!AddGlobalSource("depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера глубины (depthbuffer).");
        return false;
    }

    // UI pass
    uiPass.name = "UI";
    RG_CHECK(AddPass(uiPass));
    RG_CHECK(AddPassSink("UI", "colourbuffer"));
    RG_CHECK(AddPassSink("UI", "depthbuffer"));
    RG_CHECK(AddPassSource("UI", "colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Other));
    RG_CHECK(AddPassSource("UI", "depthbuffer",  RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Global));
    RG_CHECK(PassSetSinkLinkage("UI", "colourbuffer", nullptr, "colourbuffer"));
    RG_CHECK(PassSetSinkLinkage("UI", "depthbuffer", nullptr, "colourbuffer"));

    if (!Finalize()) {
        MERROR("Не удалось завершить построение графа рендеринга. Подробности см. в журнале.");
        return false;
    }

    return true;
}

bool UiRendergraph::Initialize()
{
    return LoadResource();
}

bool UiRendergraph::FramePrepare(FrameData &rFrameData, Camera &CurrentCamera, Viewport &CurrentViewport, SimpleScene &scene, u32 RenderMode)
{
    RendergraphPassData& data = uiPass.PassData;
    data.viewport = &CurrentViewport;
    data.ViewMatrix.Identity();
    data.ProjectionMatrix = CurrentViewport.projection;
    data.DoExecute = true;

    // Renderables.
    uiPass.RenderData.renderables = DArray<UiRenderable>(rFrameData.FrameAllocator);
    auto uiSys = SystemsManager::GetUiSystem(); 
    if (!uiSys->Render(nullptr, rFrameData, uiPass.RenderData)) {
        MERROR("Стандартной системе пользовательского интерфейса не удалось выполнить отрисовку.");
    }
    
    return true;
}
