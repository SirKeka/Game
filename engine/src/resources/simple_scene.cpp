#include "simple_scene.h"
#include "core/frame_data.h"
#include "core/systems_manager.h"
#include "math/frustrum.h"
#include "math/geometry_utils.h"
#include "resources/skybox.h"
#include "resources/debug/debug_box3d.h"
#include "resources/debug/debug_line3d.h"
#include "renderer/renderer_types.h"
#include "systems/camera_system.hpp"
#include "systems/light_system.h"
#include "systems/resource_system.h"
#include "utils/sort.h"

struct SimpleSceneDebugData {
    DebugBox3D box;
    DebugLine3D line;

    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Resource); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Resource); }
};

/// @brief Частная структура, используемая для сортировки геометрии по расстоянию от камеры.
struct GeometryDistance {
    /// @brief Данные рендеринга геометрии.
    GeometryRenderData g;
    /// @brief Расстояние от камеры.
    f32 distance;
};

static i32 GeometryRenderDataCompare(void* a, void* b) {
    auto _a = (GeometryRenderData*)a;
    auto _b = (GeometryRenderData*)b;
    if (!_a->material || !_b->material) {
        return 0;  // Не сортируйте недопустимые записи.
    }
    return _a->material->id - _b->material->id;
}

static i32 GeometryDistanceCompare(void* a, void* b) {
    auto _a = (GeometryDistance*)a;
    auto _b = (GeometryDistance*)b;
    if (_a->distance > _b->distance) {
        return 1;
    } else if (_a->distance < _b->distance) {
        return -1;
    }
    return 0;
}

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
        this->config = new SimpleSceneConfig((SimpleSceneConfig&&)*config);
    }

    DebugGrid::Config GridConfig{
        .name = "debug_grid",
        .orientation = DebugGrid::Orientation::XZ,
        .TileCountDim0 = 100,
        .TileCountDim1 = 100,
        .TileScale = 1.F,
        .UseThirdAxis = true
    };

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
            Skybox::Config SbConfig{(MString&&)config->SkyboxConfig.CubemapName};
            skybox = new Skybox();
            if (!skybox->Create(SbConfig)) {
                MWARN("Не удалось создать скайбокс.");
                delete skybox;
                skybox = nullptr;
            }
        }

        // Если имя не назначено, предполагается отсутствие направленного света.
        if (config->DirectionalLightConfig.name) {
            DirectionalLightSimpleSceneConfig& conf = config->DirectionalLightConfig;
            DirLight = new DirectionalLight();
            DirLight->name = (MString&&)conf.name;
            DirectionalLight::Data& data = DirLight->data;
            data.colour             = conf.colour;
            data.direction          = conf.direction;
            data.ShadowDistance     = conf.ShadowDistance;
            data.ShadowFadeDistance = conf.ShadowFadeDistance;
            data.ShadowSplitMult    = conf.ShadowSplitMult;

            // Добавьте отладочные данные и инициализируйте их.
            auto debug = new SimpleSceneDebugData();
            DirLight->DebugData = debug;

            // Создайте точки линии на основе направления света.
            // Первая точка всегда будет в начале координат сцены.
            FVec3 Point0{};
            FVec3 Point1 = FVec3(DirLight->data.direction).Normalize() * -1.F;

            if (!debug->line.Create(Point0, Point1)) {
                MERROR("Не удалось создать отладочную линию для направленного света.");
            }
        }

        // Точечные источники света.
        const u32 PointLightCount = config->PointLights.Length();
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
            auto debug = new SimpleSceneDebugData();
            PointLights[i].DebugData = debug;
            
            if (!(debug->box = DebugBox3D(0.2F, nullptr))) {
                MERROR("Не удалось создать отладочное поле для направленного света.");
            } else {
                debug->box.xform.SetPosition(PointLights[i].data.position);
            }
        }

        // Meshes
        const u32 MeshConfigCount = config->meshes.Length();
        for (u32 i = 0; i < MeshConfigCount; ++i) {
            Mesh* NewMesh = meshes.PushBack();
            MeshConfig& MeshConfig = config->meshes[i];
            if (!MeshConfig.name || !MeshConfig.ResourceName) {
                MWARN("Неверная конфигурация сетки, требуется имя и имя ресурса.");
                continue;
            }

            if (!NewMesh->Create(MeshConfig)) {
                MERROR("Не удалось создать новую сетку в простой сцене.");
                continue;
            }
        }

        // Ландшафты
        u32 TerrainConfigCount = config->terrains.Length();
        for (u32 i = 0; i < TerrainConfigCount; ++i) {
            TerrainSimpleSceneConfig& tConfig = config->terrains[i];
            if (!tConfig.name ||
                !tConfig.ResourceName) {
                MWARN("Неверная конфигурация ландшафта, требуется имя и имя ресурса.");
                continue;
            }
            // Terrain::Config NewTerrainConfig = {0};
            // NewTerrainConfig.name = tConfig.name;
            // // ЗАДАЧА: Скопировать имя ресурса, загрузить из ресурса.
            // NewTerrainConfig.TileCountX = 100;
            // NewTerrainConfig.TileCountЯ = 100;
            // NewTerrainConfig.TileScaleX = 1.F;
            // NewTerrainConfig.TileScaleЯ = 1.F;
            // NewTerrainConfig.MaterialCount = 0;
            // NewTerrainConfig.MaterialNames = 0;
            TerrainResource terrainResource;
            if (!ResourceSystem::Load(tConfig.ResourceName.c_str(), nullptr, terrainResource)) {
                MWARN("Не удалось загрузить ресурс ландшафта.");
                continue;
            }

            Terrain::Config& ParsedConfig = terrainResource.data;
            ParsedConfig.xform = tConfig.xform;

            Terrain* NewTerrain = terrains.PushBack();
            // ЗАДАЧА: Мы действительно хотим это копировать?
            if (!NewTerrain->Create(ParsedConfig)) {
                MWARN("Не удалось загрузить ландшафт.");
                continue;
            }

            // ResourceSystem::Unload(terrainResource);
        }
    }

    if (!grid.Initialize()) {
        return false;
    }

    // Обработать линии отладки направленного света
    if (DirLight && DirLight->DebugData) {
        auto debug = (SimpleSceneDebugData*)DirLight->DebugData;
        if (!debug->line.Initialize()) {
            MERROR("не удалось инициализировать отладочный блок.");
            delete debug;
            DirLight->DebugData = nullptr;
            return false;
        }
    }

    // Обработать отладочные блоки точечного света
    const u32 PointLightCount = PointLights.Length();
    for (u32 i = 0; i < PointLightCount; ++i) {
        if (PointLights[i].DebugData) {
            auto debug = (SimpleSceneDebugData*)PointLights[i].DebugData;
            if (!debug->box.Initialize()) {
                MERROR("не удалось инициализировать отладочный блок.");
                delete debug;
                PointLights[i].DebugData = nullptr;
                return false;
            }
        }
    }

    // Теперь обрабатывайте иерархию.
    // ПРИМЕЧАНИЕ: в настоящее время поддерживается только иерархия сеток.
    const u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (meshes[i].ParentName) {
            auto parent = GetMesh(meshes[i].ParentName);
            if (!parent) {
                MWARN("Сетка '%s' настроена на наличие родителя с именем '%s', но родителя не существует.", meshes[i].name.c_str(), meshes[i].ParentName.c_str());
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

    // for (u32 i = 0; i < MeshCount; ++i) {
    //     if (!meshes[i].Initialize()) {
    //         MERROR("Сетку не удалось инициализировать.");
    //         // return false;
    //     }
    // }

    const u32 TerrainCount = terrains.Length();
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

    const u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (!meshes[i].Load()) {
            MERROR("Не удалось загрузить Mesh.");
            return false;
        }
    }

    const u32 TerrainCount = terrains.Length();
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

    auto lightSys = (LightSystem*)SystemsManager::GetState(SystemsManager::Light);
    if (DirLight) {
        if (!lightSys->AddDirectional(DirLight)) {
            MWARN("Не удалось добавить направленный свет в систему освещения.");
        } else {
            if (DirLight->DebugData) {
                auto debug = (SimpleSceneDebugData*)DirLight->DebugData;
                if (!debug->line.Load()) {
                    MERROR("Не удалозь загрузить отладочные линии.");
                    MemorySystem::Free(DirLight->DebugData, sizeof(SimpleSceneDebugData), Memory::Resource);
                    DirLight->DebugData = nullptr;
                }
            }
        }
    }

    const u32 PointLightCount = PointLights.Length();
    for (u32 i = 0; i < PointLightCount; ++i) {
        if (!lightSys->AddPoint(&PointLights[i])) {
            MWARN("Не удалось добавить точечный источник света в систему освещения.");
        } else {
            // Загрузите отладочные данные, если они были настроены.
            auto debug = (SimpleSceneDebugData*)PointLights[i].DebugData;
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
    if (state == State::Unloading) {
        ActualUnload();
        return true;
    }

    if (state >= State::Loaded) {
        // ЗАДАЧА: Обновите направленный свет, если он изменился.
        if (DirLight && DirLight->DebugData) {
            auto debug = (SimpleSceneDebugData*)DirLight->DebugData;
            if (debug->line.geometry.generation != INVALID::U16ID) {
                // Обновите цвет. ПРИМЕЧАНИЕ: выполнение этого в каждом кадре может быть затратным, если нам придется постоянно перезагружать геометрию.
                // ЗАДАЧА: Возможно, есть другой способ сделать это, например, шейдер, который использует униформу для цвета?
                debug->line.SetColour(DirLight->data.colour);
            }
        }

        // Обновите отладочные блоки точечного света.
        const u32 PointLightCount = PointLights.Length();
        for (u32 i = 0; i < PointLightCount; ++i) {

            auto& pLight = PointLights[i];

            if (pLight.DebugData) {
                auto debug = (SimpleSceneDebugData*)pLight.DebugData;
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
        const u32 MeshCount = meshes.Length();
        for (u32 i = 0; i < MeshCount; ++i) {
            Mesh& mesh = meshes[i];
            if (mesh.generation == INVALID::U8ID) {
                continue;
            }
            if (!mesh.DebugData) {
                SimpleSceneDebugData* debug = new SimpleSceneDebugData();
                mesh.DebugData = debug;

                if (!(debug->box = DebugBox3D(0.2F, nullptr))) {
                    MERROR("Не удалось создать отладочный блок для сетки '%s'.", mesh.name.c_str());
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

    return true;
}

void SimpleScene::RenderFramePrepare(const FrameData &rFrameData)
{
    if (state >= State::Loaded) {
        if (DirLight && DirLight->DebugData) {
            auto debug = (SimpleSceneDebugData*)DirLight->DebugData;
            debug->line.RenderFramePrepare(rFrameData);
        }

        // Обновить отладочные рамки точечных источников света.
        u32 PointLightCount = PointLights.Length();
        for (u32 i = 0; i < PointLightCount; ++i) {
            if (PointLights[i].DebugData) {
                auto debug = (SimpleSceneDebugData*)PointLights[i].DebugData;
                debug->box.RenderFramePrepare(rFrameData);
            }
        }

        // Проверьте меши на наличие отладочных данных.
        u32 MeshCount = meshes.Length();
        for (u32 i = 0; i < MeshCount; ++i) {
            Mesh& mesh = meshes[i];
            if (mesh.generation == INVALID::U8ID) {
                continue;
            }
            if (mesh.DebugData) {
                auto debug = (SimpleSceneDebugData*)mesh.DebugData;
                debug->box.RenderFramePrepare(rFrameData);
            }
        }
    }
}

void SimpleScene::UpdateLODfromViewPosition(FVec3 ViewPosition, f32 NearClip, f32 FarClip, const FrameData &rFrameData)
{
    if (state >= State::Loaded) {
    u32 TerrainCount = terrains.Length();
        for (u32 i = 0; i < TerrainCount; ++i) {
            Terrain& terrain = terrains[i];

            Matrix4D model = terrain.xform.GetWorld();

            // Рассчитайте разбиения LOD на основе диапазона отсечения.
            f32 range = FarClip - NearClip;

            // Расстояние первого разделения всегда равно 0.
            f32 *splits = (f32*)rFrameData.FrameAllocator->Allocate(sizeof(f32) * (terrain.LODcount + 1));
            splits[0] = 0.F;
            for (u32 l = 0; l < terrain.LODcount; ++l) {
                f32 pct = (l + 1) / (f32)terrain.LODcount;
                // Just do linear splits for now.
                splits[l + 1] = (NearClip + range) * pct;
            }

            // Рассчитайте уровни детализации (LOD) для фрагментов (чанков) в зависимости от расстояния до позиции камеры.
            for (u32 c = 0; c < terrain.ChunkCount; c++) {
                Terrain::Chunk& chunk = terrain.chunks[c];

                // Выполните перенос/масштабирование центра.
                FVec3 gCenter = chunk.center * model;

                // Проверьте расстояние до фрагмента.
                f32 DistToChunk = Distance(ViewPosition, gCenter);
                u8 lod = INVALID::U8ID;
                for (u8 l = 0; l < terrain.LODcount; ++l) {
                    // Если речь идёт об интервале между этим и следующим разбиением, то следует использовать именно этот LOD.
                    if (DistToChunk >= splits[l] && DistToChunk <= splits[l + 1]) {
                        lod = l;
                        break;
                    }
                }
                // Предусмотрите обработку чанков, находящихся за пределами усеченной пирамиды видимости.
                if (lod == INVALID::U8ID) {
                    lod = terrain.LODcount - 1;
                }

                chunk.CurrentLOD = lod;
            }
        }
    }
}

bool SimpleScene::AddDirectionalLight(const char* name, DirectionalLight &light)
{
    auto lightSys = (LightSystem*)SystemsManager::GetState(SystemsManager::Light);
    if (DirLight) {
        // ЗАДАЧА: Выполните необходимую выгрузку ресурсов.
        lightSys->RemoveDirectional(DirLight);

        if (DirLight->DebugData) {
            auto debug = (SimpleSceneDebugData*)DirLight->DebugData;

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
        if (!lightSys->AddDirectional(&light)) {
            MERROR("SimpleScene::AddDirectionalLight - не удалось добавить направленный свет в систему освещения.");
            return false;
        }

        // Добавьте линии, указывающие направление света.
        auto debug = (SimpleSceneDebugData*)DirLight->DebugData;

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
    if (!((LightSystem*)SystemsManager::GetState(SystemsManager::Light))->AddPoint(&light)) {
        MERROR("Не удалось добавить точечный источник света в сцену (сбой добавления системы освещения, проверьте журналы).");
        return false;
    }

    light.DebugData = MemorySystem::Allocate(sizeof(SimpleSceneDebugData), Memory::Resource, true);
    auto debug = (SimpleSceneDebugData*)light.DebugData;

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
    const u32 pLightCount = PointLights.Length();
    for (u32 i = 0; i < pLightCount; i++) {
        if (PointLights[i].name == name) {
            return &PointLights[i];
        }
    }

    MWARN("Простая сцена не содержит точечный источник света с именем «%s».", name);
    return nullptr;
}

Mesh *SimpleScene::GetMesh(const MString &name)
{
    const u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; i++) {
        if (meshes[i].name == name) {
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

    const u32 length = terrains.Length();
    for (u32 i = 0; i < length; ++i) {
        if (name == terrains[i].name) {
            return &terrains[i];
        }
    }

    MWARN("Простая сцена не содержит ландшафт под названием «%s».", name);
    return nullptr;
}

Transform *SimpleScene::GetTransform(Identifier UniqueID)
{
    const u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        auto& mesh = meshes[i];
        if (mesh.id == UniqueID) {
            return &mesh.transform;
        }
    }

    const u32 TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        auto& terrain = terrains[i];
        if (terrain.id == UniqueID) {
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

    if (!((LightSystem*)SystemsManager::GetState(SystemsManager::Light))->RemoveDirectional(DirLight)) {
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

    auto lightSys = (LightSystem*)SystemsManager::GetState(SystemsManager::Light);
    const u32 LightCount = PointLights.Length();
    for (u32 i = 0; i < LightCount; ++i) {
        if (PointLights[i].name == light.name) {
            if (!lightSys->RemovePoint(&light)) {
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

    const u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (meshes[i].name == name) {
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

    const u32 TerrainCount = terrains.Length();
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

    // Перебирайте сетки в сцене.
    // ЗАДАЧА: Это необходимо оптимизировать. Для ускорения процесса нам понадобится какое-то пространственное разбиение.
    // В противном случае сцена с тысячами объектов будет очень медленной!
    const u32 MeshCount = meshes.Length();
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
            hit.UniqueID = mesh.id;

            OutResult.hits.PushBack(hit);
        }
    }

    // Сортируем результаты по расстоянию.
    if (OutResult.hits) {
        bool swapped;
        const u32 length = OutResult.hits.Length();
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

bool SimpleScene::DebugRenderDataQuery(/*u32 &DataCount, */DArray<GeometryRenderData> *DebugGeometries)
{
    if (!DebugGeometries) {
        return false;
    }
    
    // ЗАДАЧА: Проверьте наличие сетки.
    {
        GeometryRenderData* dg = (*DebugGeometries).PushBack();
        dg->model    = Matrix4D::MakeIdentity();
        dg->CopyData(&grid.geometry);
        dg->UniqueID = INVALID::ID;
    }

    #pragma region Направленный свет.
    {
        if (DirLight && DirLight->DebugData) {
            auto debug = (SimpleSceneDebugData*)DirLight->DebugData;

            // Линия отладки 3D
            GeometryRenderData* dg = (*DebugGeometries).PushBack();
            dg->model    = debug->line.xform.GetWorld();
            dg->CopyData(&debug->line.geometry);
            dg->UniqueID = debug->line.id.UniqueID;
        }
    }
    #pragma endregion
    #pragma region Точечные источники света
    {
        const u32 PointLightCount = PointLights.Length();
        for (u32 i = 0; i < PointLightCount; ++i) {
            PointLight& PointLight = PointLights[i]; 
            if (PointLight.DebugData) {
                auto debug = (SimpleSceneDebugData*)PointLight.DebugData;

                // Поле отладки 3D
                GeometryRenderData* dg = (*DebugGeometries).PushBack();
                dg->model = debug->box.xform.GetWorld();
                dg->CopyData(&debug->box.geometry);
                dg->UniqueID = debug->box.id.UniqueID;
            }
        }
    }
    #pragma endregion
    #pragma region Фигуры отладки сетки
    {
        const u32 MeshCount = meshes.Length();
        for (u32 i = 0; i < MeshCount; ++i) {
            Mesh& mesh = meshes[i];
            if (mesh.DebugData) {
                auto debug = (SimpleSceneDebugData*)mesh.DebugData;

                // Поле отладки 3D
                GeometryRenderData* dg = (*DebugGeometries).PushBack();
                dg->model = debug->box.xform.GetWorld();
                dg->CopyData(&debug->box.geometry);
                dg->UniqueID = debug->box.id.UniqueID;
            }
        }
    }
    #pragma endregion
    return true;
}

bool SimpleScene::MeshRenderDataQuery(const Frustum &f, FVec3 center, FrameData &rFrameData, DArray<GeometryRenderData> &OutGeometries)
{
    DArray<GeometryDistance> TransparentGeometries { true };
    OutGeometries = DArray<GeometryRenderData>(rFrameData.FrameAllocator);

    u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        Mesh& mesh = meshes[i];
        if (mesh.generation != INVALID::U8ID) {
            Matrix4D model = mesh.transform.GetWorld();
            bool WindingInverted = mesh.transform.determinant < 0;

            for (u32 j = 0; j < mesh.GeometryCount; ++j) {
                Geometry* g = mesh.geometries[j]; 

                // ЗАДАЧА: Определение расстояния от линии для каждого объекта (например, направление света и положение центра, затем проверка расстояния от этой линии).
                //
                // // Расчет ограничивающей сферы.
                // {
                //     // Перемещение/масштабирование границ.
                //     FVec3 ExtentsMin = g->extents.min * model;
                //     FVec3 ExtentsMax = g->extents.max * model;

                //     f32 min = MMIN(MMIN(ExtentsMin.x, ExtentsMin.y), ExtentsMin.z); 
                //     f32 max = MMAX(MMAX(ExtentsMax.x, ExtentsMax.y), ExtentsMax.z); 
                //     f32 diff = Math::abs(max - min);
                //     f32 radius = diff * 0.5F;

                //     // Перемещение/масштабирование центра.
                //     FVec3 center = g->center * model;

                //     if (frustum_intersects_sphere(&state->camera_frustum, &center, radius)) {
                //         // Добавьте его в список для отображения.
                //         GeometryRenderData data {
                //           .model = model,
                //           .geometry = g,
                //           .UniqueID = mesh.id.UniqueID,
                //         };

                //         darray_push(game_inst->frame_data.world_geometries,
                //         data);

                //         draw_count++;
                //     }
                // }

                // Расчет AABB
                {
                    // Перемещение/масштабирование границ.
                    // FVec3 ExtentsMin = g->extents.min * model;
                    FVec3 ExtentsMax = g->extents.max * model;

                    // Переместите/масштабируйте центр.
                    FVec3 center = g->center * model;
                    FVec3 HalfExtents = {
                        Math::abs(ExtentsMax.x - center.x),
                        Math::abs(ExtentsMax.y - center.y),
                        Math::abs(ExtentsMax.z - center.z),
                    };

                    if (f.IntersectsAABB(center, HalfExtents)) {
                        // Добавьте его в список для рендеринга.
                        GeometryRenderData data {
                            .model              = model,
                            .material           = g->material,
                            .UniqueID           = mesh.id.UniqueID,
                            .VertexCount        = g->VertexCount,
                            .VertexElementSize  = g->VertexElementSize,
                            .VertexBufferOffset = g->VertexBufferOffset,
                            .IndexCount         = g->IndexCount,
                            .IndexElementSize   = g->IndexElementSize,
                            .IndexBufferOffset  = g->IndexBufferOffset,
                            .WindingInverted    = WindingInverted
                        };

                        // Проверьте, прозрачен ли объект. Если да, поместите его в отдельный временный массив, 
                        // который будет отсортирован по расстоянию от камеры. В противном случае, 
                        // поместите данные непосредственно в массив OutGeometries.
                        bool HasTransparency = false;
                        if (g->material->type == Material::Type::PBR) {
                            // Проверьте диффузную карту (слот 0).
                            HasTransparency = ((g->material->maps[0].texture->flags & Texture::HasTransparency) != 0);
                        }

                        if (HasTransparency) {
                            // Для сеток с прозрачностью добавьте их в отдельный список, который будет отсортирован по расстоянию позже.
                            // Получите центр, извлеките глобальную позицию из матрицы модели и добавьте её к центру, 
                            // затем вычислите расстояние между ним и камерой и, наконец, сохраните результат в список для сортировки.
                            // ПРИМЕЧАНИЕ: Это не идеально подходит для полупрозрачных сеток, которые пересекаются, но для наших целей этого достаточно.
                            FVec3 GeometryCenter = VectorTransform(g->center, 1.F, model);
                            f32 distance = Distance(GeometryCenter, center);

                            TransparentGeometries.EmplaceBack(data, Math::abs(distance));
                        } else {
                            OutGeometries.PushBack(data);
                        }
                        rFrameData.DrawnMeshCount++;
                    }
                }
            }
        }
    }

    // Отсортируйте непрозрачные объекты по материалу.
    Moon::QuickSort(sizeof(GeometryRenderData), OutGeometries.data, 0, OutGeometries.Length() - 1, GeometryRenderDataCompare);

    // Отсортируйте прозрачные объекты, затем добавьте их в массив OutGeometries.
    u32 TransparentGeometryCount = TransparentGeometries.Length();
    Moon::QuickSort(sizeof(GeometryDistance), TransparentGeometries.Data(), 0, TransparentGeometryCount - 1, GeometryDistanceCompare);
    for (u32 i = 0; i < TransparentGeometryCount; ++i) {
        OutGeometries.PushBack(TransparentGeometries[i].g);
    }

    return true;
}

bool SimpleScene::MeshRenderDataQueryFromLine(FVec3 direction, FVec3 center, f32 radius, FrameData &rFrameData, DArray<GeometryRenderData> &OutGeometries)
{
    DArray<GeometryDistance> TransparentGeometries{ true };
    OutGeometries = DArray<GeometryRenderData>(rFrameData.FrameAllocator);

    u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        Mesh& mesh = meshes[i];
        if (mesh.generation != INVALID::U8ID) {
            Matrix4D model = mesh.transform.GetWorld();
            bool WindingInverted = mesh.transform.determinant < 0;

            for (u32 j = 0; j < mesh.GeometryCount; ++j) {
                Geometry* geometry = mesh.geometries[j];

                // ЗАДАЧА: сохранить это где-нибудь...
                //
                // Перевести/масштабировать границы.
                FVec3 ExtentsMin = geometry->extents.min * model;
                FVec3 ExtentsMax = geometry->extents.max * model;
                // Переместите/масштабируйте центр.
                FVec3 TransformedCenter = geometry->center * model;
                // Найдите объект, наиболее удаленный от центра.
                f32 MeshRadius = MMAX(Distance(ExtentsMin, TransformedCenter), Distance(ExtentsMax, TransformedCenter));

                f32 DistToLine = DistanceToLine(TransformedCenter, center, direction);

                // Находится на расстоянии, поэтому включите его.
                if ((DistToLine - MeshRadius) <= radius) {
                    // Добавьте его в список для отображения.
                    GeometryRenderData data {
                        .model              = model,
                        .material           = geometry->material,
                        .UniqueID           = mesh.id.UniqueID,
                        .VertexCount        = geometry->VertexCount,
                        .VertexElementSize  = geometry->VertexElementSize,
                        .VertexBufferOffset = geometry->VertexBufferOffset,
                        .IndexCount         = geometry->IndexCount,
                        .IndexElementSize   = geometry->IndexElementSize,
                        .IndexBufferOffset  = geometry->IndexBufferOffset,
                        .WindingInverted = WindingInverted
                    };

                    // Проверьте, прозрачен ли объект. Если да, поместите его в отдельный временный массив, 
                    // который будет отсортирован по расстоянию от камеры. В противном случае, 
                    // поместите его непосредственно в массив ext_data->geometries.
                    bool HasTransparency = false;
                    if (geometry->material->type == Material::Type::PBR) {
                        // Проверьте диффузную карту (слот 0).
                        HasTransparency = ((geometry->material->maps[0].texture->flags & Texture::HasTransparency) != 0);
                    }

                    if (HasTransparency) {
                        // Для сеток с прозрачностью добавьте их в отдельный список, который будет отсортирован по расстоянию позже.
                        // Получите центр, извлеките глобальную позицию из матрицы модели и добавьте её к центру, затем вычислите расстояние между ним и камерой и, наконец, сохраните результат в список для сортировки.
                        // ПРИМЕЧАНИЕ: Это не идеально подходит для полупрозрачных сеток, которые пересекаются, но для наших целей этого достаточно.
                        FVec3 GeometryCenter = VectorTransform(geometry->center, 1.F, model);
                        f32 distance = Distance(GeometryCenter, center);

                        TransparentGeometries.EmplaceBack(data, Math::abs(distance));
                    } else {
                        OutGeometries.PushBack(data);
                    }
                    rFrameData.DrawnMeshCount++;
                }
            }
        }
    }

    // Отсортируйте непрозрачные объекты по материалу.
    Moon::QuickSort(sizeof(GeometryRenderData), OutGeometries.data, 0, OutGeometries.Length() - 1, GeometryRenderDataCompare);

    // Отсортируйте прозрачные объекты, затем добавьте их в массив ext_data->geometries.
    u32 TransparentGeometryCount = TransparentGeometries.Length();
    Moon::QuickSort(sizeof(GeometryDistance), TransparentGeometries.Data(), 0, TransparentGeometryCount - 1, GeometryDistanceCompare);
    for (u32 i = 0; i < TransparentGeometryCount; ++i) {
        OutGeometries.PushBack(TransparentGeometries[i].g);
    }

    return true;
}

bool SimpleScene::TerrainRenderDataQuery(const Frustum &f, FVec3 center, FrameData &rFrameData, DArray<GeometryRenderData> &OutTerrainGeometries)
{
    OutTerrainGeometries = DArray<GeometryRenderData>(rFrameData.FrameAllocator);
    u32 TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        Terrain& terrain = terrains[i];
        Matrix4D model = terrain.xform.GetWorld();
        bool WindingInverted = terrain.xform.determinant < 0;

        // Проверьте каждый фрагмент, чтобы убедиться, что он находится в поле зрения.
        for (u32 c = 0; c < terrain.ChunkCount; c++) {
            Terrain::Chunk& chunk = terrain.chunks[c];

            if (chunk.generation != INVALID::U16ID) {
                // Расчет AABB
                FVec3 gCenter, HalfExtents;

                // ЗАДАЧА: сохранить это где-нибудь...
                //
                // Переместить/масштабировать границы.
                // FVec3 ExtentsMin = g->extents.min * model;
                FVec3 ExtentsMax = chunk.extents.max * model;

                // Переместить/масштабировать центр.
                gCenter = chunk.center * model;
                HalfExtents = FVec3(
                    Math::abs(ExtentsMax.x - gCenter.x),
                    Math::abs(ExtentsMax.y - gCenter.y),
                    Math::abs(ExtentsMax.z - gCenter.z)
                );

                // ЗАДАЧА: переделать
                if (f.IntersectsAABB(gCenter, HalfExtents)) {
                    GeometryRenderData* grd = OutTerrainGeometries.PushBack();
                    grd->model = model;
                    grd->material = chunk.material;
                    grd->UniqueID = terrain.id.UniqueID;
                    grd->VertexCount = chunk.TotalVertexCount;
                    grd->VertexElementSize = sizeof(TerrainVertex);
                    grd->VertexBufferOffset = chunk.VertexBufferOffset;
                    // Использовать индексы для текущего уровня детализации.
                    grd->IndexCount = chunk.lods[chunk.CurrentLOD].TotalIndexCount;
                    grd->IndexElementSize = sizeof(u32);
                    grd->IndexBufferOffset = chunk.lods[chunk.CurrentLOD].IndexBufferOffset;
                    grd->WindingInverted = WindingInverted;
                }
            }
        }
    }

    return true;
}

bool SimpleScene::TerrainRenderDataQueryFromLine(FVec3 direction, FVec3 center, f32 radius, FrameData &rFrameData, DArray<GeometryRenderData> &OutTerrainGeometries)
{
    OutTerrainGeometries = DArray<GeometryRenderData>(rFrameData.FrameAllocator);
    u32 TerrainCount = terrains.Length();
    for (u32 i = 0; i < TerrainCount; ++i) {
        Terrain& terrain = terrains[i];
        Matrix4D model = terrain.xform.GetWorld();
        bool WindingInverted = terrain.xform.determinant < 0;

        // Проверьте каждый фрагмент, чтобы узнать, находится ли он в поле зрения.
        for (u32 c = 0; c < terrain.ChunkCount; ++c) {
            Terrain::Chunk& chunk = terrain.chunks[c];

            if (chunk.generation != INVALID::U16ID) {
                // ЗАДАЧА: где-нибудь это закэшировать...
                //
                // Выполнить перенос/масштабирование границ.
                FVec3 ExtentsMin = chunk.extents.min * model;
                FVec3 ExtentsMax = chunk.extents.max * model;
                // Выполнить перенос/масштабирование центра.
                FVec3 TransformedCenter = chunk.center * model;
                // Найти точку, наиболее удаленную от центра.
                f32 MeshRadius = MMAX(Distance(ExtentsMin, TransformedCenter), Distance(ExtentsMax, TransformedCenter));

                f32 DistToLine = DistanceToLine(TransformedCenter, center, direction);

                // Находится на расстоянии, поэтому включите его
                if ((DistToLine - MeshRadius) <= radius) {
                    // Добавьте его в список для рендеринга.
                    GeometryRenderData* grd = OutTerrainGeometries.PushBack();
                    grd->model = model;
                    grd->material = chunk.material;
                    grd->VertexCount = chunk.TotalVertexCount;
                    grd->VertexBufferOffset = chunk.VertexBufferOffset;

                    // Используйте индексы для текущего уровня детализации.
                    grd->IndexCount = chunk.lods[chunk.CurrentLOD].TotalIndexCount;
                    grd->IndexBufferOffset = chunk.lods[chunk.CurrentLOD].IndexBufferOffset;
                    grd->IndexElementSize = sizeof(u32);
                    grd->UniqueID = terrain.id.UniqueID;
                    grd->WindingInverted = WindingInverted;
                }
            }
        }
    }

    return true;
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

    const u32 MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (meshes[i].generation != INVALID::U8ID) {
            // Выгрузите все отладочные данные.
            if (meshes[i].DebugData) {
                auto debug = (SimpleSceneDebugData*)meshes[i].DebugData;

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

    const u32 TerrainCount = terrains.Length();
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
    

    auto lightSys = (LightSystem*)SystemsManager::GetState(SystemsManager::Light);
    const u32 pLightCount = PointLights.Length();
    for (u32 i = 0; i < pLightCount; ++i) {
        // ЗАДАЧА: Если есть ресурс для выгрузки, это должно быть сделано до следующей строки. Пример: поле, представляющее позицию/цвет
        if (!lightSys->RemovePoint(&PointLights[i])) {
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

    // MemorySystem::ZeroMem(this, sizeof(SimpleScene));
}
