#include "editor_rendergraph.h"
#include "editor/gizmo.h"
#include "renderer/camera.h"
#include "renderer/viewport.h"
#include "resources/simple_scene.h"

bool EditorRendergraph::Create(EditorRendergraphConfig *config)
{
    name = "editor_rendergraph";
    // Добавление глобальных источников.
    if (!AddGlobalSource("colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера цвета (colourbuffer).");
        return false;
    }

    if (!AddGlobalSource("depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера глубины (depthbuffer).");
        return false;
    }

    editorPass.name = "editor";
    RG_CHECK(AddPass(editorPass));
    RG_CHECK(AddPassSink("editor", "colourbuffer"));
    RG_CHECK(AddPassSink("editor", "depthbuffer"));
    RG_CHECK(AddPassSource("editor", "colourbuffer", RendergraphSourceType::RenderTargetColour,       RendergraphSourceOrigin::Other));
    RG_CHECK(AddPassSource("editor", "depthbuffer",  RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global));
    RG_CHECK(PassSetSinkLinkage("editor", "colourbuffer", nullptr, "colourbuffer"));
    RG_CHECK(PassSetSinkLinkage("editor", "depthbuffer",  nullptr, "depthbuffer"));

    if (!Finalize()) {
        MERROR("Не удалось завершить построение графа рендеринга. Подробности см. в журнале.");
        return false;
    }
    return true;
}

bool EditorRendergraph::Initialize()
{
    return LoadResource();
}

bool EditorRendergraph::FramePrepare(FrameData &rFrameData, Camera &CurrentCamera, Viewport &CurrentViewport, SimpleScene &scene, u32 RenderMode)
{
    if (scene.state == SimpleScene::State::Loaded) {
        if (gizmo) {
            gizmo->PrepareRenderFrame(rFrameData);
        }

        // Редакторский проход
        {
            // Включить этот проход для текущего кадра.
            RendergraphPassData& data = editorPass.PassData;
            data.DoExecute = true;
            data.viewport = &CurrentViewport;
            data.ViewMatrix = CurrentCamera.GetView();
            data.ViewPosition = CurrentCamera.GetPosition();
            data.ProjectionMatrix = CurrentViewport.projection;

            Geometry& geometry = gizmo->modeData[gizmo->mode].geometry;

            
            // FVec3 CameraPos = CurrentCamera.GetPosition();
            // FVec3 GizmoPos = gizmo->xform.GetPosition();
            // ЗАДАЧА: Следует получать это из камеры/видового экрана.
            // f32 fov = Math::DegToRad(45.F);
            // f32 dist = Distance(CameraPos, GizmoPos);
            
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
                Geometry& g = gizmo->PlaneNormalLine.geometry;
                PlaneNormalRenderData->material = nullptr;
                PlaneNormalRenderData->material = g.material;
                PlaneNormalRenderData->VertexCount = g.VertexCount;
                PlaneNormalRenderData->VertexBufferOffset = g.VertexBufferOffset;
                PlaneNormalRenderData->IndexCount = g.IndexCount;
                PlaneNormalRenderData->IndexBufferOffset = g.IndexBufferOffset;
                PlaneNormalRenderData->UniqueID = INVALID::U64ID;
            }
#endif
        }
    } else {
        // Не запускайте эти проходы, если сцена не загружена.
        editorPass.PassData.DoExecute = false;
    }

    return true;
}
