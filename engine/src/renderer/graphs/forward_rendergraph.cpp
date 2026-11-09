#include "forward_rendergraph.h"

#include "math/frustrum.h"
#include "renderer/camera.h"
#include "renderer/viewport.h"
#include "resources/simple_scene.h"
#include "resources/skybox.h"

bool ForwardRendergraph::Create(ForwardRendergraphConfig *config)
{
    name = "forward_rendergraph";
    ShadowmapResolution = config ? config->ShadowmapResolution : 2048;
    // Добавить глобальные источники.
    if (!AddGlobalSource("colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера цвета.");
        return false;
    }

    if (!AddGlobalSource("depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global)) {
        MERROR("Не удалось добавить глобальный источник буфера глубины.");
        return false;
    }

    // Проход скайбокса
    skyboxPass.name = "skybox";
    RG_CHECK(AddPass(skyboxPass));
    RG_CHECK(AddPassSink("skybox", "colourbuffer"));
    RG_CHECK(AddPassSource("skybox", "colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Other));
    RG_CHECK(PassSetSinkLinkage("skybox", "colourbuffer", nullptr, "colourbuffer"));

    // Проход карт теней
    RG_CHECK(shadowmapPass.Create("shadowmap_pass", ShadowmapResolution));
    RG_CHECK(AddPass(shadowmapPass));
    RG_CHECK(AddPassSource("shadowmap_pass", "depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Self));

    // Проход сцены
    scenePass.name = "scene";
    RG_CHECK(AddPass(scenePass));
    RG_CHECK(AddPassSink("scene", "colourbuffer"));
    RG_CHECK(AddPassSink("scene", "depthbuffer"));
    RG_CHECK(AddPassSink("scene", "shadowmap"));
    RG_CHECK(AddPassSource("scene", "colourbuffer", RendergraphSourceType::RenderTargetColour, RendergraphSourceOrigin::Other));
    RG_CHECK(AddPassSource("scene", "depthbuffer", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global));
    RG_CHECK(PassSetSinkLinkage("scene", "colourbuffer", "skybox", "colourbuffer"));
    RG_CHECK(PassSetSinkLinkage("scene", "depthbuffer", nullptr, "depthbuffer"));
    RG_CHECK(PassSetSinkLinkage("scene", "shadowmap", "shadowmap_pass", "depthbuffer"));

    if (!Finalize()) {
        MERROR("Не удалось завершить построение графа рендеринга. Подробности см. в журнале.");
        return false;
    }
    return true;
}

bool ForwardRendergraph::Initialize()
{
    if (!LoadResource()) {
        MERROR("Не удалось загрузить ресурсы графа рендеринга.");
        return false;
    }
    
    return true;
}

bool ForwardRendergraph::Update(FrameData &rFrameData)
{
    return true;
}

bool ForwardRendergraph::FramePrepare(FrameData &rFrameData, Camera &CurrentCamera, Viewport &CurrentViewport, SimpleScene &scene, u32 RenderMode)
{
    const Matrix4D& CamView = CurrentCamera.GetView();
    const FVec3 CamPos = CurrentCamera.GetPosition();

    // Проход отрисовки скайбокса (skybox pass). ПРИМЕЧАНИЕ: Этот проход должен выполняться всегда, поскольку именно он очищает экран.
    skyboxPass.PassData.viewport = &CurrentViewport;
    skyboxPass.PassData.ViewMatrix = CamView;
    skyboxPass.PassData.ViewPosition = CamPos;
    skyboxPass.PassData.ProjectionMatrix = CurrentViewport.projection;
    skyboxPass.PassData.DoExecute = true;

    // Дайте указание нашей сцене сформировать соответствующие пакеты данных. ПРИМЕЧАНИЕ: генерируются пакеты для скайбокса (skybox) и окружающего мира (world).
    if (scene.state == SimpleScene::State::Loaded) {
        scene.RenderFramePrepare(rFrameData);

        DirectionalLight* DirLight = scene.DirLight;

        // Глобальные настройки
        f32 near = CurrentViewport.NearClip;
        f32 far = DirLight ? DirLight->data.ShadowFadeDistance + DirLight->data.ShadowFadeDistance : 0;
        f32 ClipRange = far - near;

        f32 min_z = near;
        f32 max_z = near + ClipRange;
        f32 range = max_z - min_z;
        f32 ratio = max_z / min_z;

        f32 CascadeSplitMultiplier = DirLight ? DirLight->data.ShadowSplitMult : 0.95F;

        // Рассчитайте границы разделения на основе усеченной пирамиды видимости камеры.
        FVec4 splits;
        for (u32 c = 0; c < MAX_CASCADE_COUNT; c++) {
            f32 p = (c + 1) / (f32)MAX_CASCADE_COUNT;
            f32 log = min_z * Math::pow(ratio, p);
            f32 uniform = min_z + range * p;
            f32 d = CascadeSplitMultiplier * (log - uniform) + uniform;
            splits.elements[c] = (d - near) / ClipRange;
        }

        // Значения по умолчанию, используемые в том случае, если направленный источник света отсутствует.
        // Они необходимы, поскольку требуются для прохода отрисовки сцены.
        Matrix4D ShadowCameraLookats[MAX_CASCADE_COUNT];
        Matrix4D ShadowCameraProjections[MAX_CASCADE_COUNT];
        FVec3 ShadowCameraPositions[MAX_CASCADE_COUNT];
        for (u32 i = 0; i < MAX_CASCADE_COUNT; ++i) {
            ShadowCameraLookats[i] = Matrix4D::MakeIdentity();
            ShadowCameraProjections[i] = Matrix4D::MakeIdentity();
            ShadowCameraPositions[i] = FVec3();
        }

        // Skybox pass
        {
            skyboxPass.skybox = scene.skybox;
        }

        // Проход для построения карты теней — выполняется только при наличии направленного источника света.
        if (DirLight) {
            f32 LastSplitDist = 0.F;

            // Пометьте этот проход как исполняемый.
            shadowmapPass.PassData.DoExecute = true;

            // Получите направление света.
            FVec3 LightDir = FVec3(DirLight->data.direction).Normalize();

            // Подготовьте расширенные данные для прохода.
            ShadowmapPass::Data* ExtData = shadowmapPass.ExtendedData;
            ExtData->light = DirLight;

            // Frustum CullingFrustum;
            FVec3 CullingCenter;
            f32 CullingRadius;

            // Получите матрицу вида-проекции.
            Matrix4D ShadowDistProjection = Matrix4D::MakeFrustumProjection(
                CurrentViewport.FOV,
                CurrentViewport.rect.width / CurrentViewport.rect.height,
                near,
                far);
            Matrix4D CamViewProj = Matrix4D::MakeTransposed(CamView * ShadowDistProjection);

            for (u32 c = 0; c < MAX_CASCADE_COUNT; c++) {
                ShadowMapCascadeData& cascade = ExtData->cascades[c];
                cascade.CascadeIndex = c;

                // ПРИМЕЧАНИЕ: Для каждого прохода при работе с каскадами необходимо выполнять следующую процедуру.
                // Единственное существенное различие заключается в том, что для каждого прохода корректируются значения ближней и дальней плоскостей отсечения.

                // Определите координаты углов усеченной пирамиды видимости (view frustum) в мировом пространстве.
                FVec4 corners[8]{};
                Frustum::CornerPointsWorldSpace(CamViewProj, corners);

                // Скорректируйте положение этих углов, сдвигая ближнюю и дальнюю плоскости отсечения в соответствии с текущим разделением (каскадом).
                f32 SplitDist = splits.elements[c];
                for (u32 i = 0; i < 4; ++i) {
                    // дальний — ближний
                    FVec4 dist = corners[i + 4] - corners[i];
                    corners[i + 4] = corners[i] + (dist * SplitDist);
                    corners[i] = corners[i] + (dist * LastSplitDist);
                }

                // Вычислите центр усеченной пирамиды видимости камеры, усреднив координаты точек.
                // Эта точка также используется в качестве целевой точки (look-at point) для «камеры», отвечающей за тени.
                FVec3 center{};
                for (u32 i = 0; i < 8; ++i) {
                    center = center + FVec3(corners[i]);
                }
                center = center / 8.F;  // размер
                if (c == MAX_CASCADE_COUNT - 1) {
                    CullingCenter = center;
                }

                // Найдите точку, наиболее удаленную от центра, и используйте ее для определения границ (размеров) области.
                f32 radius = 0.F;
                for (u32 i = 0; i < 8; ++i) {
                    f32 distance = Distance(FVec3(corners[i]), center);
                    radius = MMAX(radius, distance);
                }
                if (c == MAX_CASCADE_COUNT - 1) {
                    CullingRadius = radius;
                }

                // Рассчитайте границы, опираясь на полученный ранее радиус.
                Extents3D extents;
                extents.max = FVec3(radius, radius, radius);
                extents.min = extents.max * -1.F;

                // Сдвиньте минимальное значение (min) внутрь, а максимальное (max) — наружу по оси Z; это позволит захватить объекты, 
                // отбрасывающие тени, даже если они находятся за пределами области видимости (например, деревья над игроком).
                // ЗАДАЧА: Это следует настраивать/регулировать для каждой сцены.
                f32 z_multiplier = 10.F;
                if (extents.min.z < 0) {
                    extents.min.z *= z_multiplier;
                } else {
                    extents.min.z /= z_multiplier;
                }

                if (extents.max.z < 0) {
                    extents.max.z /= z_multiplier;
                } else {
                    extents.max.z *= z_multiplier;
                }

                // Сформируйте вектор направления взгляда (look-at), сместившись в направлении, 
                // противоположном направлению направленного источника света, на минимально необходимое расстояние. 
                // Знак при этом меняется на противоположный, поскольку направленный источник света светит «вниз», а камера должна располагаться «вверху».
                ShadowCameraPositions[c] = center - (LightDir * -extents.min.z);
                ShadowCameraLookats[c] = Matrix4D::MakeLookAt(ShadowCameraPositions[c], center, FVec3::Up());

                // Сгенерировать ортогональную проекцию на основе границ (extents).
                ShadowCameraProjections[c] = Matrix4D::MakeOrthographicProjection(extents.min.x, extents.max.x, extents.min.y, extents.max.y, extents.min.z, extents.max.z - extents.min.z);

                // Сохранить полученные данные в составе данных прохода (pass data).
                cascade.view = ShadowCameraLookats[c];
                cascade.projection = ShadowCameraProjections[c];

                // Записать значение глубины разделения (split depth) в данные прохода.
                cascade.SplitDepth = (near + SplitDist * ClipRange) * 1.F;

                LastSplitDist = SplitDist;
            }

            // Соберите геометрические объекты, подлежащие отрисовке.
            // Обратите внимание, что это нужно сделать лишь один раз: 
            // все объекты, видимые в рамках самого дальнего каскада, 
            // должны быть отрисованы и в рамках ближайшего каскада — 
            // это необходимо для того, чтобы объекты, находящиеся вне области видимости, 
            // корректно отбрасывали тени внутрь этой области.
            if (!scene.MeshRenderDataQueryFromLine(
                    LightDir,
                    CullingCenter,
                    CullingRadius,
                    rFrameData,
                    ExtData->geometries)) {
                MERROR("Не удалось запросить меши для прохода построения теневой карты.");
            }
            // Отслеживайте количество мешей, отрисовываемых при проходе для построения теней.
            rFrameData.DrawnShadowMeshCount = ExtData->geometries.Length();

            // Соберите геометрию ландшафта.
            if (!scene.TerrainRenderDataQueryFromLine(
                    LightDir,
                    CullingCenter,
                    CullingRadius,
                    rFrameData,
                    ExtData->TerrainGeometries)) {
                MERROR("Не удалось запросить геометрию ландшафта для прохода построения карты теней.");
            }

            // ЗАДАЧА: Счётчик для геометрии ландшафта.
            rFrameData.DrawnShadowMeshCount += ExtData->TerrainGeometries.Length();
        }

        // Проход отрисовки сцены.
        {
            // Включите этот проход для текущего кадра.
            RendergraphPassData& spData = scenePass.PassData;
            spData.DoExecute = true;
            spData.viewport = &CurrentViewport;
            Matrix4D CameraProjection = CurrentViewport.projection;

            spData.ViewMatrix = CamView;
            spData.ViewPosition = CamPos;
            spData.ProjectionMatrix = CameraProjection;

            // Передайте матрицы вида и проекции «камеры» для карты теней (по одной на каждый каскад).
            for (u32 c = 0; c < MAX_CASCADE_COUNT; c++) {
                scenePass.DirectionalLightViews[c] = ShadowCameraLookats[c];
                scenePass.DirectionalLightProjections[c] = ShadowCameraProjections[c];

                ShadowmapPass::Data* spExtData = shadowmapPass.ExtendedData;
                
                scenePass.CascadeSplits.elements[c] = spExtData->cascades[c].SplitDepth;
            }
            scenePass.RenderMode = RenderMode;
            // ХАК: пока что используем кубмап скайбокса в качестве текстуры освещенности (irradiance texture).
            scenePass.IrradianceCubeTexture = scene.skybox->cubemap.texture;

            // Отсечение по пирамиде видимости камеры и подсчет объектов
            Frustum CameraFrustum = Frustum::Create(&CurrentCamera, &CurrentViewport);

            rFrameData.DrawnMeshCount = 0;

            // Выполнение запроса к сцене для поиска статических мешей, попадающих в пирамиду видимости камеры.
            if (!scene.MeshRenderDataQuery(CameraFrustum, CamPos, rFrameData, scenePass.geometries)) {
                MERROR("Не удалось запросить меши для прохода отрисовки сцены.");
            }

            // Отслеживание количества мешей, отрисовываемых при проходе для построения теней.
            rFrameData.DrawnMeshCount = scenePass.geometries.Length();

            // Добавление ландшафта (террейна)
            // Выполнение запроса к сцене для поиска мешей ландшафта, попадающих в пирамиду видимости камеры.
            if (!scene.TerrainRenderDataQuery(CameraFrustum, CamPos, rFrameData, scenePass.TerrainGeometries)) {
                MERROR("Не удалось запросить геометрию ландшафта для прохода отрисовки сцены.");
            }

            // ЗАДАЧА: Добавить счетчик для геометрии ландшафта.
            rFrameData.DrawnMeshCount += scenePass.TerrainGeometries.Length();

            // Отладочная геометрия
            if (!scene.DebugRenderDataQuery(&scenePass.DebugGeometries)) {
                MERROR("Не удалось получить количество объектов отладочного рендеринга.");
                return false;
            }

            
            // ЗАДАЧА: Перенести этот функционал в класс сцены.
            /* // HACK: Внедрение отладочной геометрии (для визуализации трассировки лучей) в данные прохода отрисовки сцены.
             *
            const u32 LineCount = state->TestLines.Length();
            for (u32 i = 0; i < LineCount; ++i) {
                DebugLine3D& tl = state->TestLines[i];
                GeometryRenderData* rd = ScenePass.DebugGeometries.PushBack();
                rd->model = tl.xform.GetWorld();
                rd->CopyData(&tl.geometry);
                rd->UniqueID = INVALID::ID;
            }

            const u32 BoxCount = state->TestBoxes.Length();
            for (u32 i = 0; i < BoxCount; ++i) {
                DebugBox3D& TestBox = state->TestBoxes[i];
                GeometryRenderData* rd = ScenePass.DebugGeometries.PushBack();
                rd->model = TestBox.xform.GetWorld();
                
                rd->CopyData(&TestBox.geometry);
                rd->UniqueID = INVALID::ID;
            } */
        }
    } else {
        // Сцена не загружена.

        // Не выполняйте эти проходы, если сцена не загружена.
        scenePass.PassData.DoExecute = false;
        shadowmapPass.PassData.DoExecute = false;
    }

    return true;
}
