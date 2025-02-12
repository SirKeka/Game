#include "simple_scene.hpp"
#include "skybox.hpp"
#include "renderer/renderer_types.hpp"
#include "systems/render_view_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/light_system.hpp"
#include "core/frame_data.h"
#include "math/frustrum.hpp"

bool SimpleScene::Create(SimpleSceneConfig *config)
{
    enabled = false;
    state = State::Uninitialized;
    SceneTransform = Transform();
    id = ++GlobalSceneID;

    // Внутренние «списки» визуализируемых объектов.
    DirLight = nullptr;
    sb = nullptr;

    if (config) {
        this->config = new SimpleSceneConfig(static_cast<SimpleSceneConfig&&>(*config)); // MemorySystem::Allocate(sizeof(SimpleSceneConfig), Memory::Scene);
        //MemorySystem::CopyMem(this->config, config, sizeof(SimpleSceneConfig));
    }

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
            name = static_cast<MString&&>(config->name);
        }
        if (config->description) {
            description = static_cast<MString&&>(config->description);
        }

        // Скайбокс можно настроить только если указаны имя и имя кубической карты. В противном случае его нет.
        if (config->SkyboxConfig.name && config->SkyboxConfig.CubemapName) {
            Skybox::Config SbConfig = {0};
            SbConfig.CubemapName = static_cast<MString&&>(config->SkyboxConfig.CubemapName);
            sb = new Skybox();
            if (!sb->Create(SbConfig)) {
                MWARN("Не удалось создать скайбокс.");
                delete sb;
                sb = nullptr;
            }
        }

        // Если имя не назначено, предполагается отсутствие направленного света.
        if (config->DirectionalLightConfig.name) {
            DirLight = new DirectionalLight();
            DirLight->name = static_cast<MString&&>(config->DirectionalLightConfig.name);
            DirLight->data.colour = config->DirectionalLightConfig.colour;
            DirLight->data.direction = config->DirectionalLightConfig.direction;
        }

        // Точечные источники света.
        const u64& PointLightCount = config->PointLights.Length();
        PointLights.Resize(PointLightCount);
        for (u32 i = 0; i < PointLightCount; ++i) {
            // PointLight NewLight = {0};
            PointLights[i].name = static_cast<MString&&>(config->PointLights[i].name);
            PointLights[i].data.colour = config->PointLights[i].colour;
            PointLights[i].data.ConstantF = config->PointLights[i].ConstantF;
            PointLights[i].data.linear = config->PointLights[i].linear;
            PointLights[i].data.position = config->PointLights[i].position;
            PointLights[i].data.quadratic = config->PointLights[i].quadratic;

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
            NewMeshConfig.name = static_cast<MString&&>(config->meshes[i].name);
            NewMeshConfig.ResourceName = static_cast<MString&&>(config->meshes[i].ResourceName);
            if (config->meshes[i].ParentName) {
                NewMeshConfig.ParentName = static_cast<MString&&>(config->meshes[i].ParentName);
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

            meshes.PushBack(static_cast<Mesh&&>(NewMesh));
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

    if (sb) {
        if (!sb->Initialize()) {
            MERROR("Skybox не удалось инициализировать.");
            sb = nullptr;
            return false;
        }
    }

    for (u32 i = 0; i < MeshCount; ++i) {
        if (!meshes[i].Initialize()) {
            MERROR("Сетка не удалось инициализировать.");
            return false;
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

    if (sb) {
        if (sb->InstanceID == INVALID::ID) {
            if (!sb->Load()) {
                MERROR("Не удалось загрузить Skybox.");
                sb = nullptr;
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

    if (DirLight) {
        if (!LightSystem::AddDirectional(DirLight)) {
            MWARN("Не удалось добавить направленный свет в систему освещения.");
        }
    }

    const u64& PointLightCount = PointLights.Length();
    for (u32 i = 0; i < PointLightCount; ++i) {
        if (!LightSystem::AddPoint(&PointLights[i])) {
            MWARN("Не удалось добавить точечный источник света в систему освещения.");
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
    }

    return true;
}

bool SimpleScene::PopulateRenderPacket(Camera *CurrentCamera, f32 aspect, FrameData &rFrameData, RenderPacket &packet)
{
    // ЗАДАЧА: Кэшируйте это где-нибудь, чтобы нам не приходилось проверять это каждый раз.
    for (u32 i = 0; i < packet.ViewCount; ++i) {
        auto& ViewPacket = packet.views[i];
        const auto view = ViewPacket.view;
        if (view->type == RenderView::KnownTypeSkybox) {
            if (sb) {
                // Скайбокс
                SkyboxPacketData SkyboxData = {sb};
                if (!RenderViewSystem::BuildPacket(view, *rFrameData.FrameAllocator, &SkyboxData, ViewPacket)) {
                    MERROR("Не удалось создать пакет для вида «skybox».");
                    return false;
                }
            }
            break;
        }
    }

    for (u32 i = 0; i < packet.ViewCount; ++i) {
        auto& ViewPacket = packet.views[i];
        const auto view = ViewPacket.view;
        if (view->type == RenderView::KnownTypeWorld) {
            auto forward = CurrentCamera->Forward();
            auto right = CurrentCamera->Right();
            auto up = CurrentCamera->Up();

            // ЗАДАЧА: получите поле зрения камеры, аспект и т. д.
            Frustrum f;
            f.Create(CurrentCamera->GetPosition(), forward, right, up, aspect, Math::DegToRad(45.F), 0.1F, 1000.F);

            // ПРИМЕЧАНИЕ: начните с разумного значения по умолчанию, чтобы избежать слишком большого количества перераспределений.
            DArray<GeometryRenderData> WorldGeometries;
            WorldGeometries.Reserve(512);
            rFrameData.DrawnMeshCount = 0;
            const u64& MeshCount = meshes.Length();
            for (u32 i = 0; i < MeshCount; ++i) {
                auto& m = meshes[i];
                if (m.generation != INVALID::U8ID) {
                    auto model = m.transform.GetWorld();

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
                            // auto ExtentsMin = g->extents.MinSize * model;
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
                                data.gid = g;
                                data.UniqueID = m.UniqueID;
                                WorldGeometries.PushBack(data);

                                rFrameData.DrawnMeshCount++;
                            }
                        }
                    }
                }
            }

            // World
            if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("world"), *rFrameData.FrameAllocator, &WorldGeometries, packet.views[1])) {
                MERROR("Не удалось создать пакет для представления «мира».");
                return false;
            }
        }
    }
    
    return true;
}

bool SimpleScene::AddDirectionalLight(const char* name, DirectionalLight &light)
{
    if (DirLight) {
        // ЗАДАЧА: Выполните необходимую выгрузку ресурсов.
        LightSystem::RemoveDirectional(DirLight);
    }

    // if (light) {
        if (!LightSystem::AddDirectional(&light)) {
            MERROR("SimpleScene::AddDirectionalLight - не удалось добавить направленный свет в систему освещения.");
            return false;
        }
    // }

    DirLight = &light;

    return true;
}

bool SimpleScene::AddPointLight(const char* name, PointLight &light)
{
    if (!LightSystem::AddPoint(&light)) {
        MERROR("Не удалось добавить точечный источник света в сцену (сбой добавления системы освещения, проверьте журналы).");
        return false;
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
    this->sb = &sb;
    if (state > State::Initialized) {
        if (!sb.Initialize()) {
            MERROR("Skybox не удалось инициализировать.");
            this->sb = nullptr;
            return false;
        }
    }

    if (state >= State::Loaded) {
        if (!sb.Load()) {
            MERROR("Skybox не удалось загрузить.");
            this->sb = nullptr;
            return false;
        }
    }

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
    return sb;
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
    if (!sb) {
        MWARN("Невозможно удалить скайбокс из сцены, частью которой он не является.");
        return false;
    }

    sb = nullptr;

    return true;
}

void SimpleScene::ActualUnload()
{
    if (sb) {
        if (!sb->Unload()) {
            MERROR("Не удалось выгрузить скайбокс");
        }
        sb->Destroy();
        sb = nullptr;
    }

    const u64& MeshCount = meshes.Length();
    for (u32 i = 0; i < MeshCount; ++i) {
        if (meshes[i].generation != INVALID::U8ID) {
            if (!meshes[i].Unload()) {
                MERROR("Не удалось выгрузить сетку.");
            }
        }
    }

    if (DirLight) {
        // ЗАДАЧА: Если есть ресурс для выгрузки, это должно быть сделано до следующей строки. Пример: поле, представляющее позицию/цвет
        if (!RemoveDirectionalLight()) {
            MERROR("Не удалось выгрузить/удалить направленный свет.");
        }
    }

    const u64& pLightCount = PointLights.Length();
    for (u32 i = 0; i < pLightCount; ++i) {
        // ЗАДАЧА: Если есть ресурс для выгрузки, это должно быть сделано до следующей строки. Пример: поле, представляющее позицию/цвет
        if (!LightSystem::RemovePoint(&PointLights[i])) {
            MERROR("Не удалось выгрузить/удалить точечный свет.");
        }
    }

    // Обновите состояние, чтобы показать, что сцена инициализирована.
    state = State::Unloaded;

    // Также уничтожьте сцену.
    DirLight = nullptr;
    sb = nullptr;

    if (pLightCount > 0) {
        PointLights.Clear();
    }

    if (MeshCount > 0) {
        meshes.Clear();
    }

    // MemorySystem::ZeroMem(this, sizeof(SimpleScene));
}
