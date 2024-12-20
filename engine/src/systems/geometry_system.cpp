#include "geometry_system.hpp"
#include "material_system.hpp"
#include "renderer/rendering_system.hpp"
#include "math/geometry_utils.hpp"
#include "memory/linear_allocator.hpp"
#include <new>

struct GeometryReference {
    u64 ReferenceCount;
    GeometryID gid;
    bool AutoRelease;
    // GeometryReference() : ReferenceCount(), gid(), AutoRelease() {}
};

GeometrySystem* GeometrySystem::state = nullptr;

GeometrySystem::GeometrySystem(u32 MaxGeometryCount, GeometryReference* RegisteredGeometries)
    : 
    MaxGeometryCount(MaxGeometryCount),
    DefaultGeometry("DefaultGeometry"),
    Default2dGeometry("Default2dGeometry"),
    RegisteredGeometries(RegisteredGeometries)
{   
    // Сделать недействительными все геометрии в массиве.
    for (u32 i = 0; i < MaxGeometryCount; ++i) {
        RegisteredGeometries[i].gid.id = INVALID::ID;
        RegisteredGeometries[i].gid.InternalID = INVALID::ID;
        RegisteredGeometries[i].gid.generation = INVALID::U16ID;
    }
}

bool GeometrySystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<GeometrySystemConfig*>(config);

    if (pConfig->MaxGeometryCount == 0) {
        MFATAL("«GeometrySystem::Initialize» — максимальное количество геометрии должно быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 ArrayRequirement = sizeof(GeometryReference) * pConfig->MaxGeometryCount;
    MemoryRequirement = sizeof(GeometrySystem) + ArrayRequirement;

    if (!memory) {
        return true;
    }

    if (!state) {
        u8* PtrGeometrySystem = reinterpret_cast<u8*>(memory);
        auto ArrayBlock = reinterpret_cast<GeometryReference*>(PtrGeometrySystem + sizeof(GeometrySystem));
        state = new(PtrGeometrySystem) GeometrySystem(pConfig->MaxGeometryCount, ArrayBlock);
    }

    if (!state->CreateDefaultGeometries()) {
        MFATAL("Не удалось создать геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    return true;
}

void GeometrySystem::Shutdown()
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

    u32 VertexCount = xSegmentCount * ySegmentCount * 4; // 4 вершины на сегмент
    u32 IndexCount  = xSegmentCount * ySegmentCount * 6; // 6 индексов на сегмент
    GeometryConfig config;
    config.VertexSize = sizeof(Vertex3D); 
    config.VertexCount = VertexCount;  
    config.vertices = MemorySystem::Allocate(VertexCount * config.VertexSize, Memory::Array, true);
    config.IndexSize = sizeof(u32);
    config.IndexCount = IndexCount;
    config.indices = MemorySystem::Allocate(IndexCount * config.IndexSize, Memory::Array, true);
    config.CopyNames(
        name, // ? name : DEFAULT_GEOMETRY_NAME,
        MaterialName // ? MaterialName : DEFAULT_MATERIAL_NAME
    );

    // ЗАДАЧА: При этом создаются дополнительные вершины, но мы всегда можем дедуплицировать их позже.
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
            Vertex3D* v = reinterpret_cast<Vertex3D*>(config.vertices);

            v[vOffset + 0].position.x = MinX;
            v[vOffset + 0].position.y = MinY;
            v[vOffset + 0].texcoord.x = MinUVx;
            v[vOffset + 0].texcoord.y = MinUVy;

            v[vOffset + 1].position.x = MaxX;
            v[vOffset + 1].position.y = MaxY;
            v[vOffset + 1].texcoord.x = MaxUVx;
            v[vOffset + 1].texcoord.y = MaxUVy;

            v[vOffset + 2].position.x = MinX;
            v[vOffset + 2].position.y = MaxY;
            v[vOffset + 2].texcoord.x = MinUVx;
            v[vOffset + 2].texcoord.y = MaxUVy;

            v[vOffset + 3].position.x = MaxX;
            v[vOffset + 3].position.y = MinY;
            v[vOffset + 3].texcoord.x = MaxUVx;
            v[vOffset + 3].texcoord.y = MinUVy;


            // Создание индексов
            u32 iOffset = ((y * xSegmentCount) + x) * 6;
            u32* indcs = reinterpret_cast<u32*>(config.indices);
            indcs[iOffset + 0] = vOffset + 0;
            indcs[iOffset + 1] = vOffset + 1;
            indcs[iOffset + 2] = vOffset + 2;
            indcs[iOffset + 3] = vOffset + 0;
            indcs[iOffset + 4] = vOffset + 3;
            indcs[iOffset + 5] = vOffset + 1;
        }
    }

    return config;
}

GeometryConfig GeometrySystem::GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 TileX, f32 TileY, const char *name, const char *MaterialName)
{
    if (width == 0) {
        MWARN("Ширина должна быть ненулевой. По умолчанию один.");
        width = 1.0f;
    }
    if (height == 0) {
        MWARN("Высота должна быть ненулевой. По умолчанию один.");
        height = 1.0f;
    }
    if (depth == 0) {
        MWARN("Глубина должна быть ненулевой. По умолчанию один.");
        depth = 1;
    }
    if (TileX == 0) {
        MWARN("TileX не должен быть нулевым. По умолчанию один.");
        TileX = 1.0f;
    }
    if (TileY == 0) {
        MWARN("TileY не должно быть нулевым. По умолчанию один.");
        TileY = 1.0f;
    }

    f32 HalfWidth = width * 0.5f;
    f32 HalfHeight = height * 0.5f;
    f32 HalfDepth = depth * 0.5f;

    GeometryConfig config;
    config.VertexSize = sizeof(Vertex3D);
    config.VertexCount = 4 * 6; // 4 вершины на сторону, 6 сторон
    config.vertices = MemorySystem::Allocate(config.VertexCount * config.VertexSize, Memory::Array, true);
    config.IndexSize = sizeof(u32);
    config.IndexCount = 6 * 6; // 6 индексов на каждой стороне, 6 сторон
    config.indices = MemorySystem::Allocate(config.IndexCount * config.IndexSize, Memory::Array, true);
    config.CopyNames(name, MaterialName);
    // config.center = FVec3();
    config.MinExtents = FVec3(-HalfWidth, -HalfHeight, -HalfDepth);
    config.MaxExtents = FVec3(HalfWidth, HalfHeight, HalfDepth);

    f32 MinUVx = 0.f;
    f32 MinUVy = 0.f;
    f32 MaxUVx = TileX;
    f32 MaxUVy = TileY;

    auto vertex  = reinterpret_cast<Vertex3D*>(config.vertices);
    // Передняя поверхность
    vertex[0].position.Set(-HalfWidth, -HalfHeight,  HalfDepth);
    vertex[0].texcoord  = FVec2(MinUVx, MinUVy);
    vertex[0].normal.Set(0.F,  0.F,  1.F);
    vertex[1].position.Set(HalfWidth,  HalfHeight,  HalfDepth);
    vertex[1].texcoord  = FVec2(MaxUVx, MaxUVy);
    vertex[1].normal.Set(0.F,  0.F,  1.F);
    vertex[2].position.Set(-HalfWidth,  HalfHeight,  HalfDepth);
    vertex[2].texcoord  = FVec2(MinUVx, MaxUVy);
    vertex[2].normal.Set(0.F,  0.F,  1.F);
    vertex[3].position.Set(HalfWidth, -HalfHeight,  HalfDepth);
    vertex[3].texcoord  = FVec2(MaxUVx, MinUVy);
    vertex[3].normal.Set(0.F,  0.F,  1.F);
    // Задняя поверхность
    vertex[4].position.Set(HalfWidth, -HalfHeight, -HalfDepth);
    vertex[4].texcoord  = FVec2(MinUVx, MinUVy);
    vertex[4].normal.Set(0.F,  0.F, -1.F);
    vertex[5].position.Set(-HalfWidth,  HalfHeight, -HalfDepth);
    vertex[5].texcoord  = FVec2(MaxUVx, MaxUVy);
    vertex[5].normal.Set(0.F,  0.F, -1.F);
    vertex[6].position.Set(HalfWidth,  HalfHeight, -HalfDepth);
    vertex[6].texcoord  = FVec2(MinUVx, MaxUVy);
    vertex[6].normal.Set(0.F,  0.F, -1.F);
    vertex[7].position.Set(-HalfWidth, -HalfHeight, -HalfDepth);
    vertex[7].texcoord  = FVec2(MaxUVx, MinUVy);
    vertex[7].normal.Set(0.F,  0.F, -1.F);
    // Левая поверхность
    vertex[8].position.Set(-HalfWidth, -HalfHeight, -HalfDepth);
    vertex[8].texcoord  = FVec2(MinUVx, MinUVy);
    vertex[8].normal.Set(-1.F,  0.F,  0.F);
    vertex[9].position.Set(-HalfWidth,  HalfHeight,  HalfDepth);
    vertex[9].texcoord  = FVec2(MaxUVx, MaxUVy);
    vertex[9].normal.Set(-1.F,  0.F,  0.F);
    vertex[10].position.Set(-HalfWidth,  HalfHeight, -HalfDepth);
    vertex[10].texcoord = FVec2(MinUVx, MaxUVy);
    vertex[10].normal.Set(-1.F,  0.F,  0.F);
    vertex[11].position.Set(-HalfWidth, -HalfHeight,  HalfDepth);
    vertex[11].texcoord = FVec2(MaxUVx, MinUVy);
    vertex[11].normal.Set(-1.F,  0.F,  0.F);
    // Правая поверхность
    vertex[12].position.Set(HalfWidth, -HalfHeight,  HalfDepth);
    vertex[12].texcoord = FVec2(MinUVx, MinUVy);
    vertex[12].normal.Set(1.F,  0.F,  0.F);
    vertex[13].position.Set(HalfWidth,  HalfHeight, -HalfDepth);
    vertex[13].texcoord = FVec2(MaxUVx, MaxUVy);
    vertex[13].normal.Set(1.F,  0.F,  0.F);
    vertex[14].position.Set(HalfWidth,  HalfHeight,  HalfDepth);
    vertex[14].texcoord = FVec2(MinUVx, MaxUVy);
    vertex[14].normal.Set(1.F,  0.F,  0.F);
    vertex[15].position.Set(HalfWidth, -HalfHeight, -HalfDepth);
    vertex[15].texcoord = FVec2(MaxUVx, MinUVy);
    vertex[15].normal.Set(1.F,  0.F,  0.F);
    // Нижняя поверхность
    vertex[16].position.Set(HalfWidth, -HalfHeight,  HalfDepth);
    vertex[16].texcoord = FVec2(MinUVx, MinUVy);
    vertex[16].normal.Set(0.F, -1.F,  0.F);
    vertex[17].position.Set(-HalfWidth, -HalfHeight, -HalfDepth);
    vertex[17].texcoord = FVec2(MaxUVx, MaxUVy);
    vertex[17].normal.Set(0.F, -1.F,  0.F);
    vertex[18].position.Set(HalfWidth, -HalfHeight, -HalfDepth);
    vertex[18].texcoord = FVec2(MinUVx, MaxUVy);
    vertex[18].normal.Set(0.F, -1.F,  0.F);
    vertex[19].position.Set(-HalfWidth, -HalfHeight,  HalfDepth);
    vertex[19].texcoord = FVec2(MaxUVx, MinUVy);
    vertex[19].normal.Set(0.F, -1.F,  0.F);
    // Верхняя поверхность
    vertex[20].position.Set(-HalfWidth,  HalfHeight,  HalfDepth);
    vertex[20].texcoord = FVec2(MinUVx, MinUVy);
    vertex[20].normal.Set(0.F,  1.F,  0.F);
    vertex[21].position.Set(HalfWidth,  HalfHeight, -HalfDepth);
    vertex[21].texcoord = FVec2(MaxUVx, MaxUVy);
    vertex[21].normal.Set(0.F,  1.F,  0.F);
    vertex[22].position.Set(-HalfWidth,  HalfHeight, -HalfDepth);
    vertex[22].texcoord = FVec2(MinUVx, MaxUVy);
    vertex[22].normal.Set(0.F,  1.F,  0.F);
    vertex[23].position.Set(HalfWidth,  HalfHeight,  HalfDepth);
    vertex[23].texcoord = FVec2(MaxUVx, MinUVy);
    vertex[23].normal.Set(0.F,  1.F,  0.F);

    u32* indices = reinterpret_cast<u32*>(config.indices);
    for (u32 i = 0; i < 6; ++i) {
        u32 VOffset = i * 4;
        u32 IOffset = i * 6;
        indices[IOffset + 0] = VOffset + 0;
        indices[IOffset + 1] = VOffset + 1;
        indices[IOffset + 2] = VOffset + 2;
        indices[IOffset + 3] = VOffset + 0;
        indices[IOffset + 4] = VOffset + 3;
        indices[IOffset + 5] = VOffset + 1;
    }

    Math::Geometry::CalculateTangents(config.VertexCount, vertex, config.IndexCount, indices);

    return config;
}

bool GeometrySystem::CreateGeometry(GeometryConfig &config, GeometryID *gid)
{
    if (!config.VertexCount || !config.vertices) {
        MERROR("VulkanAPI::CreateGeometry требует данных вершин, но они не были предоставлены. VertexCount=%d, vertices=%p", config.VertexCount, config.vertices);
        return false;
    }

    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!RenderingSystem::Load(gid, config.VertexSize, config.VertexCount, config.vertices, config.IndexSize, config.IndexCount, config.indices)) {
        // Сделайте запись недействительной.
        state->RegisteredGeometries[gid->id].ReferenceCount = 0;
        state->RegisteredGeometries[gid->id].AutoRelease = false;
        gid->id = INVALID::ID;
        gid->generation = INVALID::U16ID;
        gid->InternalID = INVALID::ID;

        return false;
    }

    // Копирование экстентов, центра и т.д.
    gid->center = config.center;
    gid->extents.MinSize = config.MinExtents;
    gid->extents.MaxSize = config.MaxExtents;

    // Получить материал
    if (MString::Length(config.MaterialName) > 0) {
        gid->material = MaterialSystem::Acquire(config.MaterialName);
        if (!gid->material) {
            gid->material = MaterialSystem::GetDefaultMaterial();
        }
    }

    return true;
}

void GeometrySystem::DestroyGeometry(GeometryID *gid)
{
    RenderingSystem::Unload(gid);

    gid->id = INVALID::ID; 
    gid->InternalID = INVALID::ID;
    gid->generation = INVALID::U16ID; 
    MemorySystem::SetMemory(gid->name, 0, GEOMETRY_NAME_MAX_LENGTH);
    if (gid->material && MString::Length(gid->material->name) > 0) {
    MaterialSystem::Release(gid->material->name);
    gid->material = nullptr;
    }
}

bool GeometrySystem::CreateDefaultGeometries()
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
    if (!RenderingSystem::Load(&state->DefaultGeometry, sizeof(Vertex3D), 4, verts, sizeof(u32), 6, indices)) {
        MFATAL("Не удалось создать геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    // Получите материал по умолчанию.
    state->DefaultGeometry.material = MaterialSystem::GetDefaultMaterial();

    // Создайте 2d-геометрию по умолчанию.
    Vertex2D verts2d[4]{};
    verts2d[0].position.x = -0.5 * f;  // 0    3
    verts2d[0].position.y = -0.5 * f;  //
    verts2d[0].texcoord.x = 0.0f;      //
    verts2d[0].texcoord.y = 0.0f;      // 2    1

    verts2d[1].position.y = 0.5 * f;
    verts2d[1].position.x = 0.5 * f;
    verts2d[1].texcoord.x = 1.0f;
    verts2d[1].texcoord.y = 1.0f;

    verts2d[2].position.x = -0.5 * f;
    verts2d[2].position.y = 0.5 * f;
    verts2d[2].texcoord.x = 0.0f;
    verts2d[2].texcoord.y = 1.0f;

    verts2d[3].position.x = 0.5 * f;
    verts2d[3].position.y = -0.5 * f;
    verts2d[3].texcoord.x = 1.0f;
    verts2d[3].texcoord.y = 0.0f;

    // Индексы (ПРИМЕЧАНИЕ: против часовой стрелки)
    u32 indices2d[6] = {2, 1, 0, 3, 0, 1};

    state->Default2dGeometry.InternalID = 0;
    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!RenderingSystem::Load(&state->Default2dGeometry, sizeof(Vertex2D), 4, verts2d, sizeof(u32), 6, indices2d)) {
        MFATAL("Не удалось создать 2D-геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    // Получите материал по умолчанию.
    state->Default2dGeometry.material = MaterialSystem::GetDefaultMaterial();

    return true;
}

GeometryID *GeometrySystem::Acquire(u32 id)
{
    if (id != INVALID::ID && state->RegisteredGeometries[id].gid.id != INVALID::ID) {
        state->RegisteredGeometries[id].ReferenceCount++;
        return &state->RegisteredGeometries[id].gid;
    }

    // ПРИМЕЧАНИЕ. Следует ли вместо этого возвращать геометрию по умолчанию?
    MERROR("GeometrySystem::Acquire не может загрузить неверный идентификатор геометрии. Возвращение nullptr.");
    return nullptr;
}

GeometryID *GeometrySystem::Acquire(GeometryConfig& config, bool AutoRelease)
{
    GeometryID* g = nullptr;
    for (u32 i = 0; i < state->MaxGeometryCount; ++i) {
        if (state->RegisteredGeometries[i].gid.id == INVALID::ID) {
            // Поиск пустого слота.
            state->RegisteredGeometries[i].AutoRelease = AutoRelease;
            state->RegisteredGeometries[i].ReferenceCount = 1;
            g = &state->RegisteredGeometries[i].gid;
            g->id = i;
            break;
        }
    }

    if (!g) {
        MERROR("Невозможно получить свободный слот для геометрии. Измените конфигурацию, чтобы освободить больше места. Возвращение nullptr.");
        return nullptr;
    }

    if (!CreateGeometry(config, g)) {
        MERROR("Не удалось создать геометрию. Возвращение nullptr.");
        return nullptr;
    }
    //state->RegisteredGeometries = buf; // Возвращаем слетевший указатель присваивая ему значение сохраненного ранее адреса
    return g;
}

void GeometrySystem::Release(GeometryID *gid)
{
    if (gid && gid->id != INVALID::ID) {
        GeometryReference* ref = &state->RegisteredGeometries[gid->id];

        // Возьмите копию ID;
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

    MWARN("GeometrySystem::Release не может освободить неверный идентификатор геометрии. Ничего не было сделано.");
}

GeometryID *GeometrySystem::GetDefault()
{
    return &state->DefaultGeometry;

    MFATAL("GeometrySystem::GetDefault вызывается перед инициализацией системы. Возвращение nullptr.");
    return nullptr;
}

GeometryID *GeometrySystem::GetDefault2D()
{
    return &state->Default2dGeometry;
}

void GeometryConfig::Dispose()
{
    if (vertices) {
        MemorySystem::Free(vertices, VertexSize * VertexCount, Memory::Array);
    }
    if (indices) {
        MemorySystem::Free(indices, IndexSize * IndexCount, Memory::Array);
    }
    MemorySystem::ZeroMem(this, sizeof(GeometryConfig));
}

void GeometryConfig::CopyNames(const char *name, const char *MaterialName)
{
    const char* n = name ? name : DEFAULT_GEOMETRY_NAME;
    const char* mn = MaterialName ? MaterialName : DEFAULT_MATERIAL_NAME;

    for (u64 i = 0, j = 0; i < 256;) {
        if(n[i]) {
            this->name[i] = n[i];
            i++;
        }
        if(mn[j]) {
            this->MaterialName[j] = mn[j];
            j++;
        }
        if (!n[i] && !mn[j]) {
            this->name[i] = this->MaterialName[j] = 0;
            break;
        }
    }
}
