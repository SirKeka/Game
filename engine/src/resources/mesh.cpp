#include "mesh.hpp"
#include "systems/resource_system.h"
#include "systems/geometry_system.h"
#include "systems/job_systems.hpp"
#include "core/identifier.hpp"

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
    MeshParams->OutMesh->geometries = MemorySystem::TAllocate<GeometryID*>(Memory::Array, MeshParams->OutMesh->GeometryCount, true);
    for (u32 i = 0; i < MeshParams->OutMesh->GeometryCount; ++i) {
        MeshParams->OutMesh->geometries[i] = GeometrySystem::Instance()->Acquire(configs[i], true);
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
        geometries = reinterpret_cast<GeometryID**>(MemorySystem::Allocate(sizeof(GeometryID*), Memory::Array));
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

    MemorySystem::Free(geometries, sizeof(GeometryID*) * GeometryCount, Memory::Array);
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
