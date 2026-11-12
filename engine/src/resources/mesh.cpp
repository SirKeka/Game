#include "mesh.h"
#include "systems/resource_system.h"
#include "systems/geometry_system.h"
#include "systems/job_system.h"
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
    auto MeshParams = (MeshLoadParams*)params;

    // Это также обрабатывает загрузку GPU. Не может быть джобифицировано, пока рендерер не станет многопоточным.
    GeometryConfig* configs = MeshParams->MeshRes.data.Data();
    MeshParams->OutMesh->GeometryCount = MeshParams->MeshRes.data.Length();
    CreateArray(MeshParams->OutMesh->geometries, sizeof(Geometry*) * MeshParams->OutMesh->GeometryCount, true);
    auto geosys = GeometrySystem::Instance();
    for (u32 i = 0; i < MeshParams->OutMesh->GeometryCount; ++i) {
        Geometry* geometry = MeshParams->OutMesh->geometries[i] = geosys->Acquire(configs[i], true);
        geosys->CreateGeometry(geometry);

        // Рассчитайте геометрические размеры.
        Extents3D& LocalExtents = MeshParams->OutMesh->geometries[i]->extents;
        auto verts = (Vertex3D*)configs[i].vertices;
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
        Extents3D& GlobalExtents = MeshParams->OutMesh->extents;

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
    auto MeshParams = (MeshLoadParams*)params;

    MERROR("Не удалось загрузить сетку '%s'.", MeshParams->ResourceName);

    ResourceSystem::Unload(MeshParams->MeshRes);
}

/// @brief Вызывается, когда начинается задание по загрузке сетки.
/// @param params Параметры загрузки сетки.
/// @param ResultData Результирующие данные передаются в обратный вызов завершения.
/// @return Истина при успешном выполнении задания; в противном случае ложь.
bool MeshLoadJobStart(void* params, void* ResultData) {
    auto LoadParams = (MeshLoadParams*)params;
    bool result = ResourceSystem::Load(LoadParams->ResourceName, nullptr, LoadParams->MeshRes);

    // ПРИМЕЧАНИЕ: Параметры нагрузки также используются здесь в качестве результирующих данных, теперь заполняется только поле mesh_resource.
    MemorySystem::CopyMem(ResultData, LoadParams, sizeof(MeshLoadParams));

    return result;
}

static bool Load(const char *ResourceName, Mesh* OutMesh)
{
    OutMesh->generation = INVALID::U8ID;
    MeshLoadParams params {
        .ResourceName = ResourceName,
        .OutMesh = OutMesh
    };

    JobInfo job = JobSystem::CreateJobInfo (MeshLoadJobStart, MeshLoadJobSuccess, MeshLoadJobFail, &params, sizeof(MeshLoadParams), sizeof(MeshLoadParams));
    JobSystem::Submit(job);

    return true;
}

constexpr Mesh::Mesh(MeshConfig &config)
 : name((MString&&)config.name), 
 ParentName((MString&&)config.ParentName), 
 ResourceName((MString&&)config.ResourceName), 
 id(INVALID::ID), 
 generation(), 
 GeometryCount(), 
 geometries(nullptr), 
 transform() {}

constexpr Mesh::Mesh(const Mesh &mesh) : name(mesh.name), ParentName(mesh.ParentName), ResourceName(mesh.ResourceName), id(mesh.id), generation(mesh.generation), GeometryCount(mesh.GeometryCount), geometries(), transform(mesh.transform)
{
    CreateArray(geometries, sizeof(void*) * GeometryCount);
    for (u64 i = 0; i < GeometryCount; i++) {
        geometries[i] = mesh.geometries[i];
    }
}

bool Mesh::Create(MeshConfig &config)
{
    name          = (MString&&)config.name;
    ParentName    = (MString&&)config.ParentName;
    ResourceName  = (MString&&)config.ResourceName;
    transform     = config.transform;
    GeometryCount = config.GeometryCount;
    CreateArray(geometries, sizeof(void*) * GeometryCount);

    for (size_t i = 0; i < GeometryCount; i++) {
        geometries[i] = GeometrySystem::Acquire(config.GConfigs[i], true);
    }

    return true;
}

bool Mesh::Initialize()
{
    if (ResourceName) {
        return true;
    }
    return false;
}

bool Mesh::Load()
{
    id.Generate();

    if (ResourceName) {
        return ::Load(ResourceName.c_str(), this);
    } else {
        for (u32 i = 0; i < GeometryCount; ++i) {
            GeometrySystem::CreateGeometry(geometries[i]);
            generation = 0;
        }
    }

    return true;
}

bool Mesh::Unload()
{
    for (u32 i = 0; i < GeometryCount; ++i) {
        GeometrySystem::Instance()->Release(geometries[i]);
    }

    DestroyArray(geometries, sizeof(Geometry*) * GeometryCount);
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

    if (name) {
        name.Destroy();
    }
    if (ResourceName) {
        ResourceName.Destroy();
    }
    if (ParentName) {
        ParentName.Destroy();
    }

    return true;
}
