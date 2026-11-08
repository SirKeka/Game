#include "editor_rendergraph.h"
#include "editor/gizmo.h"
#include "renderer/camera.h"
#include "renderer/viewport.h"
#include "resources/simple_scene.h"

constexpr EditorRendergraph::EditorRendergraph(bool& result, EditorRendergraphConfig* config) : Rendergraph("editor_rendergraph"), gizmo(), editorPass("editor")
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

    // Проход редактора
    RG_CHECK_RET(AddPass(editorPass), result);
    RG_CHECK_RET(AddPassSink("editor", "colourbuffer"), result);
    RG_CHECK_RET(AddPassSink("editor", "depthbuffer"),  result);
    RG_CHECK_RET(AddPassSource("editor", "colourbuffer", RendergraphSourceType::RenderTargetColour,       RendergraphSourceOrigin::Other), result);
    RG_CHECK_RET(AddPassSource("editor", "depthbuffer",  RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Other), result);
    RG_CHECK_RET(PassSetSinkLinkage("editor", "colourbuffer", nullptr, "colourbuffer"), result);
    RG_CHECK_RET(PassSetSinkLinkage("editor", "depthbuffer",  nullptr, "depthbuffer"),  result);

    Refresh();

    if (!(result = Finalize())) {
        MERROR("Failed to finalize rendergraph. See log for details.");
    }
}

bool EditorRendergraph::Initialize()
{
    if (!LoadResource()) {
        MERROR("Не удалось загрузить ресурсы графа рендеринга.");
        return false;
    }
    
    return true;
}

bool EditorRendergraph::FramePrepare(FrameData &rFrameData, Camera &CurrentCamera, Viewport &CurrentViewport, SimpleScene &scene, u32 RenderMode)
{
    if (scene.state == SimpleScene::State::Loaded) {
        if (gizmo) {
            gizmo->PrepareRenderFrame(rFrameData);
        }

        // Проход редактора
        // Включить этот проход для данного кадра.
        editorPass.PassData.DoExecute = true;
        editorPass.PassData.viewport = &CurrentViewport;
        editorPass.PassData.ViewMatrix = CurrentCamera.GetView();
        editorPass.PassData.ViewPosition = CurrentCamera.GetPosition();
        editorPass.PassData.ProjectionMatrix = CurrentViewport.projection;

        Geometry& geometry = gizmo->modeData[gizmo->mode].geometry;

        Matrix4D model = gizmo->xform.GetWorld();
        // f32 FixedSize = 0.1F;                            // ЗАДАЧА: Сделать размер гизмо настраиваемым параметром.
        f32 ScaleScalar = 1.F;                    // ((2.F * Math::tan(fov * 0.5F)) * dist) * FixedSize;
        gizmo->ScaleScalar = ScaleScalar;  // Сохранить копию для определения попаданий (hit detection).
        Matrix4D scale = Matrix4D::MakeScale(FVec3(ScaleScalar));
        model = model * scale;

        editorPass.DebugGeometries = DArray<GeometryRenderData>(rFrameData.FrameAllocator);

        GeometryRenderData* RenderData = editorPass.DebugGeometries.PushBack();
        RenderData->model = model;
        RenderData->material = geometry.material;
        RenderData->VertexCount = geometry.VertexCount;
        RenderData->VertexBufferOffset = geometry.VertexBufferOffset;
        RenderData->IndexCount = geometry.IndexCount;
        RenderData->IndexBufferOffset = geometry.IndexBufferOffset;
        RenderData->UniqueID = INVALID::U64ID;

#ifdef _DEBUG
        {
            GeometryRenderData* PlaneNormalRenderData = editorPass.DebugGeometries.PushBack();
            PlaneNormalRenderData->model = gizmo->PlaneNormalLine.xform.GetWorld();
            Geometry& geometry = gizmo->PlaneNormalLine.geometry;
            PlaneNormalRenderData->material = nullptr;
            PlaneNormalRenderData->material = geometry.material;
            PlaneNormalRenderData->VertexCount = geometry.VertexCount;
            PlaneNormalRenderData->VertexBufferOffset = geometry.VertexBufferOffset;
            PlaneNormalRenderData->IndexCount = geometry.IndexCount;
            PlaneNormalRenderData->IndexBufferOffset = geometry.IndexBufferOffset;
            PlaneNormalRenderData->UniqueID = INVALID::U64ID;
        }
#endif
    } else {
        // Не запускайте эти проходы, если сцена не загружена.
        editorPass.PassData.DoExecute = false;
    }

    return true;
}
