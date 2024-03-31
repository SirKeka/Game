#include "geometry_system.hpp"
#include "material_system.hpp"
#include "renderer/renderer.hpp"
#include "memory/linear_allocator.hpp"

struct GeometryReference {
    u64 ReferenceCount;
    GeometryID gid;
    bool AutoRelease;
};

u32 GeometrySystem::MaxGeometryCount;
GeometrySystem* GeometrySystem::state = nullptr;

GeometrySystem::GeometrySystem()
 : 
    VertexCount(), 
    vertices(), 
    IndexCount(), 
    indices(nullptr), 
    name(/*GEOMETRY_NAME_MAX_LENGTH*/), 
    MaterialName(/*MATERIAL_NAME_MAX_LENGTH*/), 
    RegisteredGeometries()
{
    // Блок массива находится после состояния. Уже выделено, поэтому просто установите указатель.
    void* ArrayBlock = this + sizeof(GeometrySystem);
    this->RegisteredGeometries = reinterpret_cast<GeometryReference*>(ArrayBlock);

    // Сделать недействительными все геометрии в массиве.
    u32 count = this->MaxGeometryCount;
    for (u32 i = 0; i < count; ++i) {
        // this->RegisteredGeometries[i].geometry.id = INVALID_ID;
        // this->RegisteredGeometries[i].geometry.InternalID = INVALID_ID;
        // this->RegisteredGeometries[i].geometry.generation = INVALID_ID;
        this->RegisteredGeometries[i].gid = GeometryID();
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

GeometryConfig GeometrySystem::GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 TileX, f32 TileY, const char *name, const char *MaterialName)
{
    if (width == 0) {
        MWARN("Ширина(width) должна быть ненулевой. По умолчанию один.");
        width = 1.0f;
    }
    if (height == 0) {
        MWARN("Высота(height) должна быть ненулевой. По умолчанию один.");
        height = 1.0f;
    }
    if (xSegmentCount < 1) {
        MWARN("Число сегментов по оси x(xSegmentCount) должно быть положительным. По умолчанию один.");
        xSegmentCount = 1;
    }
    if (ySegmentCount < 1) {
        MWARN("Число сегментов по оси y(ySegmentCount) должно быть положительным. По умолчанию один.");
        ySegmentCount = 1;
    }

    if (TileX == 0) {
        MWARN("TileX не должен быть нулевым. По умолчанию один.");
        TileX = 1.0f;
    }
    if (TileY == 0) {
        MWARN("TileY не должен быть нулевым. По умолчанию один.");
        TileY = 1.0f;
    }

    GeometryConfig config;
    config.VertexCount = xSegmentCount * ySegmentCount * 4;  // 4 вершины на сегмент
    config.vertices = MMemory::TAllocate<Vertex3D>(config.VertexCount, MEMORY_TAG_ARRAY);
    config.IndexCount = xSegmentCount * ySegmentCount * 6;  // 6 индексов на сегмент
    config.indices = MMemory::TAllocate<u32>(config.IndexCount, MEMORY_TAG_ARRAY);

    // TODO: При этом создаются дополнительные вершины, но мы всегда можем дедуплицировать их позже.
    f32 SegWidth = width / xSegmentCount;
    f32 SegHeight = height / ySegmentCount;
    f32 HalfWidth = width * 0.5f;
    f32 HalfHeight = height * 0.5f;
    for (u32 y = 0; y < ySegmentCount; ++y) {
        for (u32 x = 0; x < xSegmentCount; ++x) {
            // Создание вершин
            f32 MinX = (x * SegWidth) - HalfWidth;
            f32 MinY = (y * SegHeight) - HalfHeight;
            f32 MaxX = MinX + SegWidth;
            f32 MaxY = MinY + SegHeight;
            f32 MinUVx = (x / (f32)xSegmentCount) * TileX;
            f32 MinUVy = (y / (f32)ySegmentCount) * TileY;
            f32 MaxUVx = ((x + 1) / (f32)xSegmentCount) * TileX;
            f32 MaxUVy = ((y + 1) / (f32)ySegmentCount) * TileY;

            u32 vOffset = ((y * xSegmentCount) + x) * 4;
            Vertex3D* v0 = &config.vertices[vOffset + 0];
            Vertex3D* v1 = &config.vertices[vOffset + 1];
            Vertex3D* v2 = &config.vertices[vOffset + 2];
            Vertex3D* v3 = &config.vertices[vOffset + 3];

            v0->position.x = MinX;
            v0->position.y = MinY;
            v0->texcoord.x = MinUVx;
            v0->texcoord.y = MinUVy;

            v1->position.x = MaxX;
            v1->position.y = MaxY;
            v1->texcoord.x = MaxUVx;
            v1->texcoord.y = MaxUVy;

            v2->position.x = MinX;
            v2->position.y = MaxY;
            v2->texcoord.x = MinUVx;
            v2->texcoord.y = MaxUVy;

            v3->position.x = MaxX;
            v3->position.y = MinY;
            v3->texcoord.x = MaxUVx;
            v3->texcoord.y = MinUVy;

            // Создание индексов
            u32 iOffset = ((y * xSegmentCount) + x) * 6;
            config.indices[iOffset + 0] = vOffset + 0;
            config.indices[iOffset + 1] = vOffset + 1;
            config.indices[iOffset + 2] = vOffset + 2;
            config.indices[iOffset + 3] = vOffset + 0;
            config.indices[iOffset + 4] = vOffset + 3;
            config.indices[iOffset + 5] = vOffset + 1;
        }
    }

    if (name && MString::Length(name) > 0) {
        MString::nCopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    } else {
        MString::nCopy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if (MaterialName && MString::Length(MaterialName) > 0) {
        MString::nCopy(config.MaterialName, MaterialName, MATERIAL_NAME_MAX_LENGTH);
    } else {
        MString::nCopy(config.MaterialName, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}

bool GeometrySystem::CreateGeometry(GeometryConfig config, GeometryID *gid)
{
    if (!config.VertexCount || !config.vertices) {
        MERROR("VulkanAPI::CreateGeometry требует данных вершин, но они не были предоставлены. VertexCount=%d, vertices=%p", config.VertexCount, config.vertices);
        return false;
    }

    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!Renderer::Load(gid, config.VertexCount, config.vertices, config.IndexCount, config.indices)) {
        // Сделайте запись недействительной.
        this->RegisteredGeometries[gid->id].ReferenceCount = 0;
        this->RegisteredGeometries[gid->id].AutoRelease = false;
        // g->id = INVALID_ID;
        // g->generation = INVALID_ID;
        // g->InternalID = INVALID_ID;

        return false;
    }

    // Получить материал
    if (MString::Length(config.MaterialName) > 0) {
        gid->material = MaterialSystem::Instance()->Acquire(config.MaterialName);
        if (!gid->material) {
            gid->material = MaterialSystem::Instance()->GetDefaultMaterial();
        }
    }

    return true;
}

void GeometrySystem::DestroyGeometry(GeometryID *gid)
{
    Renderer::Unload(gid);
    gid->Destroy();
}

bool GeometrySystem::CreateDefaultGeometry()
{
    Vertex3D verts[4] {};
    const f32 f = 10.0f;

    verts[0].position.x = -0.5 * f;  // 0    3
    verts[0].position.y = -0.5 * f;  //
    verts[0].texcoord.x = 0.0f;      //
    verts[0].texcoord.y = 0.0f;      // 2    1

    verts[1].position.y = 0.5 * f;
    verts[1].position.x = 0.5 * f;
    verts[1].texcoord.x = 1.0f;
    verts[1].texcoord.y = 1.0f;

    verts[2].position.x = -0.5 * f;
    verts[2].position.y = 0.5 * f;
    verts[2].texcoord.x = 0.0f;
    verts[2].texcoord.y = 1.0f;

    verts[3].position.x = 0.5 * f;
    verts[3].position.y = -0.5 * f;
    verts[3].texcoord.x = 1.0f;
    verts[3].texcoord.y = 0.0f;

    u32 indices[6] = {0, 1, 2, 0, 3, 1};

    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!Renderer::Load(&DefaultGeometry, 4, verts, 6, indices)) {
        MFATAL("Не удалось создать геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    // Получите материал по умолчанию.
    this->DefaultGeometry.material = MaterialSystem::Instance()->GetDefaultMaterial();

    return true;
}

GeometryID *GeometrySystem::Acquire(u32 id)
{
    if (id != INVALID_ID && this->RegisteredGeometries[id].gid.id != INVALID_ID) {
        this->RegisteredGeometries[id].ReferenceCount++;
        return &this->RegisteredGeometries[id].gid;
    }

    // ПРИМЕЧАНИЕ. Следует ли вместо этого возвращать геометрию по умолчанию?
    MERROR("GeometrySystem::AcquireByID не может загрузить неверный идентификатор геометрии. Возвращение nullptr.");
    return 0;
}

GeometryID *GeometrySystem::Acquire(GeometryConfig config, bool AutoRelease)
{
    GeometryID* g = nullptr;
    for (u32 i = 0; i < this->MaxGeometryCount; ++i) {
        if (this->RegisteredGeometries[i].gid.id == INVALID_ID) {
            // Поиск пустого слота.
            this->RegisteredGeometries[i].AutoRelease = AutoRelease;
            this->RegisteredGeometries[i].ReferenceCount = 1;
            g = &this->RegisteredGeometries[i].gid;
            g->id = i;
            break;
        }
    }

    if (!g) {
        MERROR("Невозможно получить свободный слот для геометрии. Измените конфигурацию, чтобы освободить больше места. Возвращение nullptr.");
        return 0;
    }

    if (!CreateGeometry(config, g)) {
        MERROR("Не удалось создать геометрию. Возвращение nullptr.");
        return 0;
    }

    return g;
}

void GeometrySystem::Release(GeometryID *gid)
{
    if (gid && gid->id != INVALID_ID) {
        GeometryReference* ref = &this->RegisteredGeometries[gid->id];

        // Возьмите копию ID;
        u32 id = gid->id;
        if (ref->gid.id == gid->id) {
            if (ref->ReferenceCount > 0) {
                ref->ReferenceCount--;
            }

            // Также удаляем идентификатор геометрии.
            if (ref->ReferenceCount < 1 && ref->AutoRelease) {
                DestroyGeometry(&ref->gid);
                ref->ReferenceCount = 0;
                ref->AutoRelease = false;
            }
        } else {
            MFATAL("Несоответствие идентификатора геометрии. Проверьте логику регистрации, так как этого никогда не должно произойти.");
        }
        return;
    }

    MWARN("GeometrySystem::Acquire не может освободить неверный идентификатор геометрии. Ничего не было сделано.");
}

GeometryID *GeometrySystem::GetDefault()
{
    //if (this) {
        return &this->DefaultGeometry;
    //}

    MFATAL("GeometrySystem::GetDefault вызывается перед инициализацией системы. Возвращение nullptr.");
    return 0;
}

void *GeometrySystem::operator new(u64 size)
{
    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 ArrayRequirement = sizeof(Geometry) * MaxGeometryCount;
    return LinearAllocator::Instance().Allocate(size + ArrayRequirement);
}
