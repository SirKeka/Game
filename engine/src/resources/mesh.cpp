#include "mesh.hpp"
#include "systems/resource_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/job_systems.hpp"

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

bool Mesh::LoadFromResource(const char *ResourceName)
{
    generation = INVALID::U8ID;
    MeshLoadParams params;
    params.ResourceName = ResourceName;
    params.OutMesh = this;

    Job::Info job { MeshLoadJobStart, MeshLoadJobSuccess, MeshLoadJobFail, &params, sizeof(MeshLoadParams), sizeof(MeshLoadParams) };
    JobSystem::Submit(job);

    return true;
}

void Mesh::Unload()
{
    for (u32 i = 0; i < GeometryCount; ++i) {
        GeometrySystem::Instance()->Release(geometries[i]);
    }

    MemorySystem::Free(geometries, sizeof(GeometryID*) * GeometryCount, Memory::Array);
    geometries = nullptr;
    generation = INVALID::U8ID; // Для верности сделайте геометрию недействительной, чтобы она не пыталась визуализироваться.
    GeometryCount = 0;
    transform = Transform();
}
