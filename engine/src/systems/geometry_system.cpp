#include "geometry_system.hpp"

struct geometry_reference {
    u64 ReferenceCount;
    Geometry geometry;
    bool AutoRelease;
};

GeometrySystem::GeometrySystem()
 : 
    VertexCount(), 
    vertices(), 
    IndexCount(), 
    indices(nullptr), 
    name(GEOMETRY_NAME_MAX_LENGTH), 
    MaterialName(MATERIAL_NAME_MAX_LENGTH), 
    RegisteredGeometries()
{
    // Блок массива находится после состояния. Уже выделено, поэтому просто установите указатель.
    void* ArrayBlock = this + sizeof(GeometrySystem);
    this->RegisteredGeometries = ArrayBlock;

    // Сделать недействительными все геометрии в массиве.
    u32 count = state_ptr->config.max_geometry_count;
    for (u32 i = 0; i < count; ++i) {
        this->RegisteredGeometries[i].geometry.id = INVALID_ID;
        this->RegisteredGeometries[i].geometry.InternalID = INVALID_ID;
        this->RegisteredGeometries[i].geometry.generation = INVALID_ID;
    }
}

void GeometrySystem::SetMaxGeometryCount(u32 value)
{
    MaxGeometryCount = value;
}

bool GeometrySystem::Initialize()
{
    if (MaxGeometryCount == 0) {
        MFATAL("«GeometrySystem::Initialize» — максимальное количество геометрии должно быть > 0.");
        return false;
    }

    if (!state) {
        state = new GeometrySystem();
    }

    state_ptr = state;
    state_ptr->config = config;

    if (!CreateDefaultGeometry()) {
        MFATAL("Не удалось создать геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    return true;
}

void GeometrySystem::Shutdown(void *state)
{
    state = nullptr;
}

bool GeometrySystem::CreateDefaultGeometry()
{
    return false;
}

void *GeometrySystem::operator new(u64 size)
{
    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 ArrayRequirement = sizeof(Geometry) * MaxGeometryCount;
    return LinearAllocator::Instance().Allocate(size + ArrayRequirement);
}
