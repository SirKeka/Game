#include "simple_scene.h"
#include "resources/skybox.h"
#include "resources/debug/debug_box3d.h"
#include "resources/debug/debug_line3d.h"
#include "renderer/renderer_types.h"
#include "systems/render_view_system.h"
#include "systems/camera_system.hpp"
#include "systems/light_system.h"
#include "systems/resource_system.h"
#include "core/frame_data.h"
#include "math/frustrum.h"
#include "math/geometry_utils.h"
#include "game.h"

struct SimpleSceneDebugData {
    DebugBox3D box;
    DebugLine3D line;
};

bool SimpleScene::Create(SimpleSceneConfig *config)
{
    enabled = false;
    state = State::Uninitialized;
    SceneTransform = Transform();
    id = ++GlobalSceneID;

    // Внутренние «списки» визуализируемых объектов.
    DirLight = nullptr;
    skybox = nullptr;

    if (config) {
        this->config = new SimpleSceneConfig((SimpleSceneConfig&&)*config); // MemorySystem::Allocate(sizeof(SimpleSceneConfig), Memory::Scene);
        //MemorySystem::CopyMem(this->config, config, sizeof(SimpleSceneConfig));
    }

    // ПРИМЕЧАНИЕ: Начните с достаточно большого числа, чтобы избежать перераспределения памяти в начале.
    WorldData.WorldGeometries.Reserve(512);

    DebugGrid::Config GridConfig{};
    GridConfig.orientation = DebugGrid::Orientation::XZ;
    GridConfig.TileCountDim0 = 100;
    GridConfig.TileCountDim1 = 100;
    GridConfig.TileScale = 1.F;
    GridConfig.name = "debug_grid";
    GridConfig.UseThirdAxis = true;

    grid = DebugGrid(GridConfig);

    return true;
}

bool SimpleScene::Destroy()
{
    return true;
}

bool SimpleScene::Initialize()
{
    // Конфигурация процесса и иерархия настройки.
    if (config) {
        if (config->name) {
            name = (MString&&)config->name;
        }
        if (config->description) {
            description = (MString&&)config->description;
        }

        // Скайбокс можно настроить только если указаны имя и имя кубической карты. В противном случае его нет.
        if (config->SkyboxConfig.name && config->SkyboxConfig.CubemapName) {
            Skybox::Config SbConfig = {0};
            SbConfig.CubemapName = (MString&&)config->SkyboxConfig.CubemapName;
            skybox = new Skybox();
            if (!skybox->Create(SbConfig)) {
                MWARN("Не удалось создать скайбокс.");
                delete skybox;
                skybox = nullptr;
            }
        }

        // Если имя не назначено, предполагается отсутствие направленного света.
        if (config->DirectionalLightConfig.name) {
            DirLight = new DirectionalLight();
            DirLight->name = (MString&&)config->DirectionalLightConfig.name;
            DirLight->data.colour = config->DirectionalLightConfig.colour;
            DirLight->data.direction = config->DirectionalLightConfig.direction;

            // Добавьте отладочные данные и инициализируйте их.
            DirLight->DebugData = MemorySystem::Allocate(sizeof(SimpleSceneDebugData), Memory::Resource, true);
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);

            // Создайте точки линии на основе направления света.
            // Первая точка всегда будет в начале координат сцены.
            FVec3 Point0{};
            FVec3 Point1 = FVec3(DirLight->data.direction).Normalize() * -1.F;

            if (!debug->line.Create(Point0, Point1)) {
                MERROR("Не удалось создать отладочную линию для направленного света.");
            }
        }

        // Точечные источники света.
        const u64& PointLightCount = config->PointLights.Length();
        PointLights.Resize(PointLightCount);
        for (u32 i = 0; i < PointLightCount; ++i) {
            // PointLight NewLight = {0};
            PointLights[i].name = (MString&&)config->PointLights[i].name;
            PointLights[i].data.colour = config->PointLights[i].colour;
            PointLights[i].data.ConstantF = config->PointLights[i].ConstantF;
            PointLights[i].data.linear = config->PointLights[i].linear;
            PointLights[i].data.position = config->PointLights[i].position;
            PointLights[i].data.quadratic = config->PointLights[i].quadratic;

            // Добавьте отладочные данные и инициализируйте их.
            PointLights[i].DebugData = MemorySystem::Allocate(sizeof(SimpleSceneDebugData), Memory::Resource, true);
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(PointLights[i].DebugData);
            
            if (!(debug->box = DebugBox3D(0.2F, nullptr))) {
                MERROR("Не удалось создать отладочное поле для направленного света.");
            } else {
                debug->box.xform.SetPosition(PointLights[i].data.position);
            }

            // PointLights.PushBack(NewLight);
        }

        // Meshes
        const u64& MeshConfigCount = config->meshes.Length();
        for (u32 i = 0; i < MeshConfigCount; ++i) {
            if (!config->meshes[i].name || !config->meshes[i].ResourceName) {
                MWARN("Неверная конфигурация сетки, требуется имя и имя ресурса.");
                continue;
            }
            Mesh::Config NewMeshConfig = {0};
            NewMeshConfig.name = (MString&&)config->meshes[i].name;
            NewMeshConfig.ResourceName = (MString&&)config->meshes[i].ResourceName;
            if (config->meshes[i].ParentName) {
                NewMeshConfig.ParentName = (MString&&)config->meshes[i].ParentName;
            }
            Mesh NewMesh;
            if (!NewMesh.Create(NewMeshConfig)) {
                MERROR("Не удалось создать новую сетку в простой сцене.");
                NewMeshConfig.name.Clear();
                NewMeshConfig.ResourceName.Clear();
                if (NewMeshConfig.ParentName) {
                    NewMeshConfig.ParentName.Clear();
                }
                continue;
            }
            NewMesh.transform = config->meshes[i].transform;

            meshes.PushBack((Mesh&&)NewMesh);
        }

        // Ландшафты
        u32 TerrainConfigCount = config->terrains.Length();
        for (u32 i = 0; i < TerrainConfigCount; ++i) {
            if (!config->terrains[i].name ||
                !config->terrains[i].ResourceName) {
                MWARN("Неверная конфигурация ландшафта, требуется имя и имя ресурса.");
                continue;
            }
            /*Terrain::Config NewTerrainConfig = {0};
            NewTerrainConfig.name = config->terrains[i].name;
            // ЗАДАЧА: Скопировать имя ресурса, загрузить из ресурса.
            NewTerrainConfig.TileCountX = 100;
            NewTerrainConfig.TileCountЯ = 100;
            NewTerrainConfig.TileScaleX = 1.F;
            NewTerrainConfig.TileScaleЯ = 1.F;
            NewTerrainConfig.MaterialCount = 0;
            NewTerrainConfig.MaterialNames = 0;*/
            TerrainResource terrainResource;
            if (!ResourceSystem::Load(config->terrains[i].ResourceName.c_str(), eResource::Type::Terrain, nullptr, terrainResource)) {
                MWARN("Не удалось загрузить ресурс ландшафта.");
                continue;
            }

            Terrain::Config& ParsedConfig = terrainResource.data;
            ParsedConfig.xform = config->terrains[i].xform;

            Terrain NewTerrain{};
            // ЗАДАЧА: Мы действительно хотим это копировать?
            if (!NewTerrain.Create(ParsedConfig)) {
                MWARN("Не удалось загрузить ландшафт.");
                continue;
            }

            // ResourceSystem::Unload(terrainResource);

            terrains.PushBack(NewTerrain);
        }
    }

    if (!grid.Initialize()) {
        return false;
    }

    // Обработать линии отладки направленного света
    if (DirLight && DirLight->DebugData) {
        auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);
        if (!debug->line.Initialize()) {
            MERROR("не удалось инициализировать отладочный блок.");
            MemorySystem::Free(DirLight->DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
            DirLight->DebugData = nullptr;
            return false;
        }
    }

    // Обработать отладочные блоки точечного света
    const u32& PointLightCount = PointLights.Length();
    for (u32 i = 0; i < PointLightCount; ++i) {
        if (PointLights[i].DebugData) {
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(PointLights[i].DebugData);
            if (!debug->box.Initialize()) {
                MERROR("не удалось инициализировать отладочный блок.");
                MemorySystem::Free(PointLights[i].DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                PointLights[i].DebugData = nullptr;
                return false;
            }
        }
    }

    // Теперь обрабатывайте иерархию.
    // ПРИМЕЧАНИЕ: в настоящее время поддерживается только иерархия сеток.
    const u64& MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (meshes[i].config.ParentName) {
            auto parent = GetMesh(meshes[i].config.ParentName);
            if (!parent) {
                MWARN("Сетка '%s' настроена на наличие родителя с именем '%s', но родителя не существует.", meshes[i].config.name.c_str(), meshes[i].config.ParentName.c_str());
                continue;
            }

            meshes[i].transform.SetParent(&parent->transform);
        }
    }

    if (skybox) {
        if (!skybox->Initialize()) {
            MERROR("Skybox не удалось инициализировать.");
            skybox = nullptr;
            // return false;
        }
    }

    for (u32 i = 0; i < MeshCount; ++i) {
        if (!meshes[i].Initialize()) {
            MERROR("Сетку не удалось инициализировать.");
            // return false;
        }
    }

    const u32& TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        if (!terrains[i].Initialize()) {
            MERROR("Не удалось инициализировать ландшафт.");
            // return false;
        }
    }

    // Обновите состояние, чтобы показать, что сцена инициализирована.
    state = State::Initialized;

    return true;
}

bool SimpleScene::Load()
{
    // Обновите состояние, чтобы показать, что сцена в данный момент загружается.
    state = State::Loading;

    if (skybox) {
        if (skybox->InstanceID == INVALID::ID) {
            if (!skybox->Load()) {
                MERROR("Не удалось загрузить Skybox.");
                skybox = nullptr;
                return false;
            }
        }
    }

    const u64& MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (!meshes[i].Load()) {
            MERROR("Не удалось загрузить Mesh.");
            return false;
        }
    }

    const u32& TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        if (!terrains[i].Load()) {
            MERROR("Не удалось загрузить ландшафт.");
            // return false; // Возвращать false в случае неудачи?
        }
    }

    // Отладочная сетка.
    if (!grid.Load()) {
        return false;
    }

    if (DirLight) {
        if (!LightSystem::AddDirectional(DirLight)) {
            MWARN("Не удалось добавить направленный свет в систему освещения.");
        } else {
            if (DirLight->DebugData) {
                auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);
                if (!debug->line.Load()) {
                    MERROR("Не удалозь загрузить отладочные линии.");
                    MemorySystem::Free(DirLight->DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                    DirLight->DebugData = nullptr;
                }
            }
        }
    }

    const u64& PointLightCount = PointLights.Length();
    for (u32 i = 0; i < PointLightCount; ++i) {
        if (!LightSystem::AddPoint(&PointLights[i])) {
            MWARN("Не удалось добавить точечный источник света в систему освещения.");
        } else {
            // Загрузите отладочные данные, если они были настроены.
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(PointLights[i].DebugData);
            if (!debug->box.Load()) {
                MERROR("Не удалось загрузить отладочное поле.");
                MemorySystem::Free(PointLights[i].DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                PointLights[i].DebugData = 0;
            }
        }
    }

    // Обновите состояние, чтобы показать, что сцена полностью загружена.
    state = State::Loaded;

    return true;
}

bool SimpleScene::Unload(bool immediate)
{
    if (immediate) {
        state = State::Unloading;
        ActualUnload();
        return true;
    }

    // Обновите состояние, чтобы показать, что сцена в данный момент выгружается.
    state = State::Unloading;
    return true;
}

bool SimpleScene::Update(const FrameData &rFrameData)
{
    if (state >= SimpleScene::State::Loaded) {
        // ЗАДАЧА: Обновите направленный свет, если он изменился.
        if (DirLight && DirLight->DebugData) {
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);
            if (debug->line.geometry.generation != INVALID::U16ID) {
                // Обновите цвет. ПРИМЕЧАНИЕ: выполнение этого в каждом кадре может быть затратным, если нам придется постоянно перезагружать геометрию.
                // ЗАДАЧА: Возможно, есть другой способ сделать это, например, шейдер, который использует униформу для цвета?
                debug->line.SetColour(DirLight->data.colour);
            }
        }

        // Обновите отладочные блоки точечного света.
        const u32& PointLightCount = PointLights.Length();
        for (u32 i = 0; i < PointLightCount; ++i) {

            auto& pLight = PointLights[i];

            if (pLight.DebugData) {
                auto debug = reinterpret_cast<SimpleSceneDebugData*>(pLight.DebugData);
                if (debug->box.geometry.generation != INVALID::U16ID) {
            
                    // Обновите преобразование.
                    debug->box.xform.SetPosition(pLight.data.position);

                    // Обновите цвет. ПРИМЕЧАНИЕ: выполнение этого в каждом кадре может быть затратным, если нам придется постоянно перезагружать геометрию.
                    // ЗАДАЧА: Возможно, есть другой способ сделать это, например, шейдер, который использует униформу для цвета?
                    debug->box.SetColour(pLight.data.colour);
                }
            }
        }

        // Проверьте сетки, чтобы узнать, есть ли у них отладочные данные. Если нет, добавьте их здесь и инициализируйте/загрузите их.
        // Это делается здесь, потому что загрузка сеток многопоточная и может быть еще недоступна, даже если объект присутствует в сцене.
        const u32& MeshCount = meshes.Length();
        for (u32 i = 0; i < MeshCount; ++i) {
            auto& mesh = meshes[i];
            if (mesh.generation == INVALID::U8ID) {
                continue;
            }
            if (!mesh.DebugData) {
                mesh.DebugData = MemorySystem::Allocate(sizeof(SimpleSceneDebugData), Memory::Resource, true);
                auto debug = reinterpret_cast<SimpleSceneDebugData*>(mesh.DebugData);

                if (!(debug->box = DebugBox3D(0.2F, nullptr))) {
                    MERROR("Не удалось создать отладочный блок для сетки '%s'.", mesh.config.name.c_str());
                } else {
                    debug->box.xform.SetParent(&mesh.transform);

                    if (!debug->box.Initialize()) {
                        MERROR("Не удалось инициализировать отладочный блок.");
                        MemorySystem::Free(mesh.DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                        mesh.DebugData = nullptr;
                        continue;
                    }

                    if (!debug->box.Load()) {
                        MERROR("Не удалось загрузить отладочный блок.");
                        MemorySystem::Free(mesh.DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                        mesh.DebugData = nullptr;
                    }

                    // Обновите экстенты.
                    debug->box.SetColour(FVec4(0.F, 1.F, 0.F, 1.F));
                    debug->box.SetExtents(mesh.extents);
                }
            }
        }
    }

    if (state == State::Unloading) {
        ActualUnload();
    }

    return true;
}

bool SimpleScene::PopulateRenderPacket(Camera *CurrentCamera, Viewport& viewport, FrameData &rFrameData, RenderPacket &packet)
{

    // Мир
    {
        auto& ViewPacket = packet.views[Testbed::PacketViews::World];
        const auto view = ViewPacket.view;
        // Обязательно очистите массив геометрии мира.
        WorldData.WorldGeometries.Clear();
        WorldData.TerrainGeometries.Clear();
        WorldData.DebugGeometries.Clear();

        // Skybox
        WorldData.SkyboxData.skybox = skybox;

        // Обновить усеченную пирамиду
        auto forward = CurrentCamera->Forward();
        auto right = CurrentCamera->Right();
        auto up = CurrentCamera->Up();

        // ЗАДАЧА: получите поле зрения камеры, аспект и т. д.
        auto& rect = viewport.rect;
        Frustum f;
        f.Create(CurrentCamera->GetPosition(), forward, right, up, (f32)rect.width / rect.height, viewport.FOV, viewport.NearClip, viewport.FarClip);

        rFrameData.DrawnMeshCount = 0;
        
        const u64& MeshCount = meshes.Length();
        for (u32 i = 0; i < MeshCount; ++i) {
            auto& m = meshes[i];
            if (m.generation != INVALID::U8ID) {
                auto model = m.transform.GetWorld();
                bool WindingInverted = m.transform.determinant < 0;

                for (u32 j = 0; j < m.GeometryCount; ++j) {
                    auto g = m.geometries[j];

                    // Расчет ограничивающей сферы.
                    // {
                    //     // Переместите/масштабируйте экстенты.
                    //     auto ExtentsMin = g->extents.MinSize * model;
                    //     auto ExtentsMax = g->extents.MaxSize * model;

                    //     f32 min = MMIN(MMIN(ExtentsMin.x, ExtentsMin.y), ExtentsMin.z);
                    //     f32 max = MMAX(MMAX(ExtentsMax.x, ExtentsMax.y), ExtentsMax.z);
                    //     f32 diff = Math::abs(max - min);
                    //     f32 radius = diff * 0.5f;

                    //     // Переместите/масштабируйте центр.
                    //     auto center = g->center * model;

                    //     if (state->CameraFrustrum.IntersectsSphere(center, radius)) {
                    //         // Добавьте его в список для рендеринга.
                    //         GeometryRenderData data = {};
                    //         data.model = model;
                    //         data.gid = g;
                    //         data.UniqueID = m.UniqueID;
                    //         WorldGeometries.PushBack(data);

                    //         rFrameData.DrawnMeshCount++;
                    //     }
                    // }

                    // Расчет AABB
                    {
                        // Переместите/масштабируйте экстенты.
                        // auto ExtentsMin = g->extents.min * model;
                        auto ExtentsMax = g->extents.max * model;

                        // Переместить/масштабировать центр.
                        auto center = g->center * model;
                        FVec3 HalfExtents{
                            Math::abs(ExtentsMax.x - center.x),
                            Math::abs(ExtentsMax.y - center.y),
                            Math::abs(ExtentsMax.z - center.z),
                        };

                        if (f.IntersectsAABB(center, HalfExtents)) {
                            // Добавьте его в список для рендеринга.
                            GeometryRenderData data = {};
                            data.model = model;
                            data.geometry = g;
                            data.UniqueID = m.UniqueID;
                            data.WindingInverted = WindingInverted;
                            WorldData.WorldGeometries.PushBack(data);

                            rFrameData.DrawnMeshCount++;
                        }
                    }
                }
            }
        }

        // ЗАДАЧА: добавить ландшафт(ы)
        const u32& TerrainCount = terrains.Length();
        for (u32 i = 0; i < TerrainCount; ++i) {
            // ЗАДАЧА: Проверить генерацию ландшафта
            // ЗАДАЧА: Отбраковка усеченных пирамид
            //
            GeometryRenderData data = {};
            data.model = terrains[i].xform.GetWorld();
            data.geometry = &terrains[i].geo;
            data.UniqueID = terrains[i].UniqueID;

            WorldData.TerrainGeometries.PushBack(data);

            // ЗАДАЧА: Счетчик для геометрии ландшафта.
            rFrameData.DrawnMeshCount++;
        }

        // Геометрия отладки

        // Сетка.
        {
            GeometryRenderData data {};
            data.model = Matrix4D::MakeIdentity();
            data.geometry = &grid.geometry;
            data.UniqueID = INVALID::ID;
            WorldData.DebugGeometries.PushBack(data);
        }

        // Направленный свет.
        {
            if (DirLight && DirLight->DebugData) {
                auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);

                // Линия отладки 3d
                GeometryRenderData data{};
                data.model = debug->line.xform.GetWorld();
                data.geometry = &debug->line.geometry;
                data.UniqueID = debug->line.UniqueID;
                WorldData.DebugGeometries.PushBack(data);
            }
        }

        // Точечные источники света
        {
            const u32& PointLightCount = PointLights.Length();
            for (u32 i = 0; i < PointLightCount; ++i) {
                if (PointLights[i].DebugData) {
                    auto debug = reinterpret_cast<SimpleSceneDebugData*>(PointLights[i].DebugData);

                    // Отладочное поле 3d
                    GeometryRenderData data{};
                    data.model = debug->box.xform.GetWorld();
                    data.geometry = &debug->box.geometry;
                    data.UniqueID = debug->box.UniqueID;
                    WorldData.DebugGeometries.PushBack(data);
                }
            }
        }

        // Сетка отладочных форм
        {
            const u32& MeshCount = meshes.Length();
            for (u32 i = 0; i < MeshCount; ++i) {
                if (meshes[i].DebugData) {
                    auto debug = reinterpret_cast<SimpleSceneDebugData*>(meshes[i].DebugData);

                    // Отладочное поле 3d
                    GeometryRenderData data{};
                    data.model = debug->box.xform.GetWorld();
                    data.geometry = &debug->box.geometry;
                    data.UniqueID = debug->box.UniqueID;
                    WorldData.DebugGeometries.PushBack(data);
                }
            }
        }

        // Мир
        if (!RenderViewSystem::BuildPacket(view, rFrameData, viewport, &WorldData.WorldGeometries, ViewPacket)) {
            MERROR("Не удалось создать пакет для представления «мира».");
            return false;
        }
    }
    
    return true;
}

bool SimpleScene::AddDirectionalLight(const char* name, DirectionalLight &light)
{
    if (DirLight) {
        // ЗАДАЧА: Выполните необходимую выгрузку ресурсов.
        LightSystem::RemoveDirectional(DirLight);

        if (DirLight->DebugData) {
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);

            debug->line.Unload();
            debug->line.Destroy();

            // ПРИМЕЧАНИЕ: здесь нельзя освобождать, если нет света, так как он будет снова использован ниже.
            // if (!light) {
                // MemorySystem::Free(DirLight->DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                // DirLight->DebugData = nullptr;
            // }
        }
    }

    DirLight = &light;

    if (DirLight) {
        if (!LightSystem::AddDirectional(&light)) {
            MERROR("SimpleScene::AddDirectionalLight - не удалось добавить направленный свет в систему освещения.");
            return false;
        }

        // Добавьте линии, указывающие направление света.
        auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);

        // Создайте точки линии на основе направления света.
        // Первая точка всегда будет в начале координат сцены.
        FVec3 Point0 {};
        FVec3 Point1 = FVec3(DirLight->data.direction).Normalize() * -1.F;

        if (!(debug->line = DebugLine3D(Point0, Point1, nullptr))) {
            MERROR("Не удалось создать линию отладки для направленного света.");
        } else {
            if (state > State::Initialized) {
                if (!debug->line.Initialize()) {
                    MERROR("Не удалось инициализировать линию отладки.");
                    MemorySystem::Free(light.DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                    light.DebugData = nullptr;
                    return false;
                }
            }

            if (state >=  State::Loaded) {
                if (!debug->line.Load()) {
                    MERROR("Неудалось загрузить отладочную линию.");
                    MemorySystem::Free(light.DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                    light.DebugData = nullptr;
                }
            }
        }
    }

    

    return true;
}

bool SimpleScene::AddPointLight(const char* name, PointLight &light)
{
    if (!LightSystem::AddPoint(&light)) {
        MERROR("Не удалось добавить точечный источник света в сцену (сбой добавления системы освещения, проверьте журналы).");
        return false;
    }

    light.DebugData = MemorySystem::Allocate(sizeof(SimpleSceneDebugData), Memory::Resource, true);
    auto debug = reinterpret_cast<SimpleSceneDebugData*>(light.DebugData);

    if (!(debug->box = DebugBox3D(0.2F, nullptr))) {
        MERROR("Не удалось создать отладочное поле для направленного света.");
    } else {
        debug->box.xform.SetPosition(light.data.position);

        if (state > State::Initialized) {
            if (!debug->box.Initialize()) {
                MERROR("не удалось инициализировать отладочное поле.");
                MemorySystem::Free(light.DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                light.DebugData = nullptr;
                return false;
            }
        }

        if (state >= State::Loaded) {
            if (!debug->box.Load()) {
                MERROR("не удалось загрузить отладочное поле.");
                MemorySystem::Free(light.DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                light.DebugData = nullptr;
            }
        }
    }

    PointLights.PushBack(light);

    return true;
}

bool SimpleScene::AddMesh(const char* name, Mesh &m)
{
    if (state > State::Initialized) {
        if (!m.Initialize()) {
            MERROR("Сетка не инициализировалась.");
            return false;
        }
    }

    if (state >= State::Loaded) {
        if (!m.Initialize()) {
            MERROR("Сетка не загрузилась.");
            return false;
        }
    }

    meshes.PushBack(m);

    return true;
}

bool SimpleScene::AddSkybox(const char* name, Skybox &sb)
{
    // ЗАДАЧА: если он уже существует, нужно ли что-то с ним делать?
    this->skybox = &sb;
    if (state > State::Initialized) {
        if (!sb.Initialize()) {
            MERROR("Skybox не удалось инициализировать.");
            this->skybox = nullptr;
            return false;
        }
    }

    if (state >= State::Loaded) {
        if (!sb.Load()) {
            MERROR("Skybox не удалось загрузить.");
            this->skybox = nullptr;
            return false;
        }
    }

    return true;
}

bool SimpleScene::AddTerrain(const char *name, Terrain &terrain)
{

    if (state > SimpleScene::State::Initialized) {
        if (!terrain.Initialize()) {
            MERROR("Не удалось инициализировать ландшафт.");
            return false;
        }
    }

    if (state >= SimpleScene::State::Loaded) {
        if (!terrain.Load()) {
            MERROR("Не удалось загрузить ландшафт.");
            return false;
        }
    }

    terrains.PushBack(terrain);

    return true;
}

DirectionalLight *SimpleScene::GetDirectionalLight(const char *name)
{
    return DirLight;
}

PointLight *SimpleScene::GetPointLight(const char *name)
{
    for (u32 i = 0; i < PointLights.Length(); i++) {
        if (PointLights[i].name == name) {
            return &PointLights[i];
        }
    }

    MWARN("Простая сцена не содержит точечный источник света с именем «%s».", name);
    return nullptr;
}

Mesh *SimpleScene::GetMesh(const MString &name)
{
    for (u32 i = 0; i < meshes.Length(); i++) {
        if (meshes[i].config.name == name) {
            return &meshes[i];
        }
    }
    
    MWARN("Простая сцена не содержит сетку с именем «%s».", name.c_str());
    return nullptr;
}

Skybox *SimpleScene::GetSkybox(const char *name)
{
    return skybox;
}

Terrain *SimpleScene::GetTerrain(const char *name)
{
    if (!name)
        return nullptr;

    const u32& length = terrains.Length();
    for (u32 i = 0; i < length; ++i) {
        if (name == terrains[i].name) {
            return &terrains[i];
        }
    }

    MWARN("Простая сцена не содержит ландшафт под названием «%s».", name);
    return nullptr;
}

Transform *SimpleScene::GetTransform(u32 UniqueID)
{
    const u32& MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        auto& mesh = meshes[i];
        if (mesh.UniqueID == UniqueID) {
            return &mesh.transform;
        }
    }

    u32 TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        auto& terrain = terrains[i];
        if (terrain.UniqueID == UniqueID) {
            return &terrain.xform;
        }
    }

    return nullptr;
}

bool SimpleScene::RemoveDirectionalLight()
{
    // if (!name) {
    //     return false;
    // }
    
    // if (light != DirLight) {
    //     MWARN("Невозможно удалить направленный свет из сцены, который не является частью сцены.");
    //     return false;
    // }

    if (!LightSystem::RemoveDirectional(DirLight)) {
        MERROR("Не удалось удалить направленный свет из системы освещения.");
        return false;
    } else {
        // Выгрузите отладку направленного света, если она существует.
        if (DirLight->DebugData) {
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);

            debug->line.Unload();
            debug->line.Destroy();

            MemorySystem::Free(DirLight->DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
            DirLight->DebugData = nullptr;
        }
    }

    DirLight = nullptr;

    return true;
}

bool SimpleScene::RemovePointLight(const MString& name, PointLight& light)
{
    if (!name) {
        return false;
    }

    const u64& LightCount = PointLights.Length();
    for (u32 i = 0; i < LightCount; ++i) {
        if (PointLights[i].name == light.name) {
            if (!LightSystem::RemovePoint(&light)) {
                MERROR("Не удалось удалить точечный источник света из системы освещения.");
                return false;
            } else {
                // Уничтожьте отладочные данные, если они существуют.
                if (PointLights[i].DebugData) {
                    auto debug = reinterpret_cast<SimpleSceneDebugData*>(PointLights[i].DebugData);
                    debug->box.Unload();
                    debug->box.Destroy();
                    MemorySystem::Free(PointLights[i].DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                    PointLights[i].DebugData = nullptr;
                }
            }

            PointLights.PopAt(i);

            return true;
        }
    }

    MERROR("Невозможно удалить точечный источник света из сцены, частью которой он не является.");
    return false;
}

bool SimpleScene::RemoveMesh(const char* name)
{
    if (!name) {
        return false;
    }

    const u64& MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (meshes[i].config.name == name) {
            // Выгрузите все отладочные данные.
            if (meshes[i].DebugData) {
                auto debug = reinterpret_cast<SimpleSceneDebugData*>(meshes[i].DebugData);
            
                debug->box.Unload();
                debug->box.Destroy();
            
                MemorySystem::Free(meshes[i].DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                meshes[i].DebugData = nullptr;
            }

            // Выгрузите саму сетку.
            if (!meshes[i].Unload()) {
                MERROR("Не удалось выгрузить сетку");
                return false;
            }

            meshes.PopAt(i);

            return true;
        }
    }

    MERROR("Невозможно удалить точечный источник света из сцены, частью которой он не является.");
    return false;
}

bool SimpleScene::RemoveSkybox(const char* name)
{
    if (!name) {
        return false;
    }

    // ЗАДАЧА: имя?
    if (!skybox) {
        MWARN("Невозможно удалить скайбокс из сцены, частью которой он не является.");
        return false;
    }

    skybox = nullptr;

    return true;
}

bool SimpleScene::RemoveTerrain(const char *name)
{
    if (!name)
        return false;

    const u32& TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        if (terrains[i].name == name) {
            if (!terrains[i].Unload()) {
                MERROR("Не удалось выгрузить ландшафт");
                return false;
            }

            Terrain rubbish;
            terrains.PopAt(i);

            return true;
        }
    }

    MERROR("Невозможно удалить ландшафт из сцены, частью которой он не является.");
    return false;
}

bool SimpleScene::Raycast(const Ray &ray, RaycastResult &OutResult)
{
    if (state < State::Loaded) {
        return false;
    }

    // Создавайте только при необходимости.
    OutResult.hits = 0;

    // Перебирайте сетки в сцене.
    // ЗАДАЧА: Это необходимо оптимизировать. Для ускорения процесса нам понадобится какое-то пространственное разбиение.
    // В противном случае сцена с тысячами объектов будет очень медленной!
    const u32& MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        auto& mesh = meshes[i];
        Matrix4D model = mesh.transform.GetWorld();
        f32 dist;
        if (Math::RaycastOrientedExtents(mesh.extents, model, ray, dist)) {
            // Hit

            RaycastHit hit;
            hit.distance = dist;
            hit.type = RaycastHit::Type::OBB;
            hit.position = ray.origin + ray.direction * hit.distance;
            hit.UniqueID = mesh.UniqueID;

            OutResult.hits.PushBack(hit);
        }
    }

    // Сортируем результаты по расстоянию.
    if (OutResult.hits) {
        bool swapped;
        const u32& length = OutResult.hits.Length();
        for (u32 i = 0; i < length - 1; ++i) {
            swapped = false;
            for (u32 j = 0; j < length - 1; ++j) {
                if (OutResult.hits[j].distance > OutResult.hits[j + 1].distance) {
                    MSWAP(RaycastHit, OutResult.hits[j], OutResult.hits[j + 1]);
                    swapped = true;
                }
            }

            // Если ни один из элементов не был перепутан, сортировка завершена.
            if (!swapped) {
                break;
            }
        }
    }

    return bool(OutResult.hits);
}

void SimpleScene::ActualUnload()
{
    if (skybox) {
        if (!skybox->Unload()) {
            MERROR("Не удалось выгрузить скайбокс");
        }
        skybox->Destroy();
        skybox = nullptr;
    }

    const u64& MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (meshes[i].generation != INVALID::U8ID) {
            // Выгрузите все отладочные данные.
            if (meshes[i].DebugData) {
                auto debug = reinterpret_cast<SimpleSceneDebugData*>(meshes[i].DebugData);

                debug->box.Unload();
                debug->box.Destroy();

                MemorySystem::Free(meshes[i].DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                meshes[i].DebugData = nullptr;
            }

            // Выгрузите саму сетку
            if (!meshes[i].Unload()) {
                MERROR("Не удалось выгрузить сетку.");
            }
            meshes[i].Destroy();
        }
    }

    const u32& TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        if (!terrains[i].Unload()) {
            MERROR("Не удалось выгрузить местность.");
        }
        terrains[i].Destroy();
    }

    // Отладочная сетка.
    grid.Unload() ? grid.Destroy() : MWARN("Не удалось выгрузить отладочную сетку.");

    if (DirLight) {
        if (!RemoveDirectionalLight()) {
            MERROR("Не удалось выгрузить/удалить направленный свет.");
        }

        if (DirLight && DirLight->DebugData) {
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(DirLight->DebugData);
            // Выгрузить данные о направленной световой линии.
            debug->line.Unload();
            debug->line.Destroy();
            MemorySystem::Free(DirLight->DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
            DirLight->DebugData = nullptr;
        }
    }
    

    const u64& pLightCount = PointLights.Length();
    for (u32 i = 0; i < pLightCount; ++i) {
        // ЗАДАЧА: Если есть ресурс для выгрузки, это должно быть сделано до следующей строки. Пример: поле, представляющее позицию/цвет
        if (!LightSystem::RemovePoint(&PointLights[i])) {
            MWARN("Не удалось выгрузить/удалить точечный свет.");
        }

        // Уничтожьте отладочные данные, если они существуют.
        if (PointLights[i].DebugData) {
            auto debug = reinterpret_cast<SimpleSceneDebugData*>(PointLights[i].DebugData);
            debug->box.Unload();
            debug->box.Destroy();
            MemorySystem::Free(PointLights[i].DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
            PointLights[i].DebugData = nullptr;
        }
    }

    // Обновите состояние, чтобы показать, что сцена инициализирована.
    state = State::Unloaded;

    // Также уничтожьте сцену.
    DirLight = nullptr;
    skybox = nullptr;

    if (pLightCount > 0) {
        PointLights.Destroy();
    }

    if (MeshCount > 0) {
        meshes.Destroy();
    }

    if (terrains) {
        terrains.Destroy();
    }

    if (WorldData.WorldGeometries) {
        WorldData.WorldGeometries.Destroy();
    }

    if (WorldData.TerrainGeometries) {
        WorldData.TerrainGeometries.Destroy();
    }

    if (WorldData.DebugGeometries) {
        WorldData.DebugGeometries.Destroy();
    }

    // MemorySystem::ZeroMem(this, sizeof(SimpleScene));
}
