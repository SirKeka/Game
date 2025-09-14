#include "mesh.h"
#include "systems/resource_system.h"
#include "systems/geometry_system.h"
#include "systems/job_systems.hpp"
#include "core/identifier.h"

// Также используется как ResultData из задания.
struct MeshLoadParams {
    const char* ResourceName;
    Mesh* OutMesh;
    MeshResource MeshRes{};
};

/// @brief Вызывается при успешном завершении задания.
/// @param params Параметры, переданные из задания после завершения.
void MeshLoadJobSuccess(void* params) {
    auto MeshParams = reinterpret_cast<MeshLoadParams*>(params);

    // Это также обрабатывает загрузку GPU. Не может быть джобифицировано, пока рендерер не станет многопоточным.
    auto configs = MeshParams->MeshRes.data.Data();
    MeshParams->OutMesh->GeometryCount = MeshParams->MeshRes.data.Length();
    MeshParams->OutMesh->geometries = reinterpret_cast<Geometry**>(MemorySystem::Allocate(sizeof(Geometry*) * MeshParams->OutMesh->GeometryCount, Memory::Array, true));
    for (u32 i = 0; i < MeshParams->OutMesh->GeometryCount; ++i) {
        MeshParams->OutMesh->geometries[i] = GeometrySystem::Instance()->Acquire(configs[i], true);

        // Рассчитайте геометрические размеры.
        auto& LocalExtents = MeshParams->OutMesh->geometries[i]->extents;
        auto verts = reinterpret_cast<Vertex3D*>(configs[i].vertices);
        for (u32 v = 0; v < configs[i].VertexCount; ++v) {
            // Мин
            if (verts[v].position.x < LocalExtents.min.x) {
                LocalExtents.min.x = verts[v].position.x;
            }
            if (verts[v].position.y < LocalExtents.min.y) {
                LocalExtents.min.y = verts[v].position.y;
            }
            if (verts[v].position.z < LocalExtents.min.z) {
                LocalExtents.min.z = verts[v].position.z;
            }

            // Maкс
            if (verts[v].position.x > LocalExtents.max.x) {
                LocalExtents.max.x = verts[v].position.x;
            }
            if (verts[v].position.y > LocalExtents.max.y) {
                LocalExtents.max.y = verts[v].position.y;
            }
            if (verts[v].position.z > LocalExtents.max.z) {
                LocalExtents.max.z = verts[v].position.z;
            }
        }

        // Рассчитайте общие размеры для сетки, получив размеры для каждой подсетки.
        auto& GlobalExtents = MeshParams->OutMesh->extents;

        // Мин.
        if (LocalExtents.min.x < GlobalExtents.min.x) {
            GlobalExtents.min.x = LocalExtents.min.x;
        }
        if (LocalExtents.min.y < GlobalExtents.min.y) {
            GlobalExtents.min.y = LocalExtents.min.y;
        }
        if (LocalExtents.min.z < GlobalExtents.min.z) {
            GlobalExtents.min.z = LocalExtents.min.z;
        }

        // Макс.
        if (LocalExtents.max.x > GlobalExtents.max.x) {
            GlobalExtents.max.x = LocalExtents.max.x;
        }
        if (LocalExtents.max.y > GlobalExtents.max.y) {
            GlobalExtents.max.y = LocalExtents.max.y;
        }
        if (LocalExtents.max.z > GlobalExtents.max.z) {
            GlobalExtents.max.z = LocalExtents.max.z;
        }
    }
    MeshParams->OutMesh->generation++;

    MTRACE("Успешно загружена сетка '%s'.", MeshParams->ResourceName);

    ResourceSystem::Unload(MeshParams->MeshRes);
}

/// @brief Вызывается при сбое задания.
/// @param params Параметры, передаваемые при сбое задания.
void MeshLoadJobFail(void* params) {
    auto MeshParams = reinterpret_cast<MeshLoadParams*>(params);

    MERROR("Не удалось загрузить сетку '%s'.", MeshParams->ResourceName);

    ResourceSystem::Unload(MeshParams->MeshRes);
}

/// @brief Вызывается, когда начинается задание по загрузке сетки.
/// @param params Параметры загрузки сетки.
/// @param ResultData Результирующие данные передаются в обратный вызов завершения.
/// @return Истина при успешном выполнении задания; в противном случае ложь.
bool MeshLoadJobStart(void* params, void* ResultData) {
    auto LoadParams = reinterpret_cast<MeshLoadParams*>(params);
    bool result = ResourceSystem::Load(LoadParams->ResourceName, eResource::Type::Mesh, nullptr, LoadParams->MeshRes);

    // ПРИМЕЧАНИЕ: Параметры нагрузки также используются здесь в качестве результирующих данных, теперь заполняется только поле mesh_resource.
    MemorySystem::CopyMem(ResultData, LoadParams, sizeof(MeshLoadParams));

    return result;
}

static bool Load(const char *ResourceName, Mesh* OutMesh)
{
    OutMesh->generation = INVALID::U8ID;
    MeshLoadParams params;
    params.ResourceName = ResourceName;
    params.OutMesh = OutMesh;

    Job::Info job { MeshLoadJobStart, MeshLoadJobSuccess, MeshLoadJobFail, &params, sizeof(MeshLoadParams), sizeof(MeshLoadParams) };
    JobSystem::Submit(job);

    return true;
}

bool Mesh::Create(Config& config)
{
    this->config = static_cast<Config&&>(config);
    this->generation = INVALID::U8ID;

    return true;
}

bool Mesh::Initialize()
{
    if (config.ResourceName) {
        return true;
    } else {
        // Просто проверяю конфигурацию.
        if (!config.GConfigs) {
            return false;
        }

        GeometryCount = config.GeometryCount;
        geometries = reinterpret_cast<Geometry**>(MemorySystem::Allocate(sizeof(Geometry*), Memory::Array));
    }
    return true;
}

bool Mesh::Load()
{
    UniqueID = Identifier::AquireNewID(this);

    if (config.ResourceName) {
        return ::Load(config.ResourceName.c_str(), this);
    } else {
        if (!config.GConfigs) {
            return false;
        }

        for (u32 i = 0; i < config.GeometryCount; ++i) {
            geometries[i] = GeometrySystem::Acquire(config.GConfigs[i], true);
            generation = 0;

            // Очистите выделения для конфигурации геометрии.
            // ЗАДАЧА: Сделайте это во время выгрузки/уничтожения
            config.GConfigs[i].Dispose();
        }
    }

    return true;
}

bool Mesh::Unload()
{
    for (u32 i = 0; i < GeometryCount; ++i) {
        GeometrySystem::Instance()->Release(geometries[i]);
    }

    MemorySystem::Free(geometries, sizeof(Geometry*) * GeometryCount, Memory::Array);
    geometries = nullptr;
    generation = INVALID::U8ID; // Для верности сделайте геометрию недействительной, чтобы она не пыталась визуализироваться.
    GeometryCount = 0;
    transform = Transform();
    return true;
}

bool Mesh::Destroy()
{
    if (geometries) {
        if (!this->Unload()) {
            MERROR("Mesh::Destroy() - не удалось выгрузить сетку.");
            return false;
        }
    }

    // if (name) {
    //     name.Clear();
    // }

    if (config.name) {
        config.name.Clear();
    }
    if (config.ResourceName) {
        config.ResourceName.Clear();
    }
    if (config.ParentName) {
        config.ParentName.Clear();
    }

    return true;
}
