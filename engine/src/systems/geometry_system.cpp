#include "geometry_system.h"
#include "material_system.h"
#include "core/systems_manager.h"
#include "math/geometry_utils.h"
#include "memory/linear_allocator.h"
#include "renderer/rendering_system.h"
#include "resources/geometry.h"

struct GeometryReference {
    u64 ReferenceCount;
    Geometry geometry;
    bool AutoRelease;
};

GeometrySystem::GeometrySystem(u32 MaxGeometryCount, GeometryReference* RegisteredGeometries)
    : 
    MaxGeometryCount(MaxGeometryCount),
    DefaultGeometry("DefaultGeometry"),
    Default2dGeometry("Default2dGeometry"),
    RegisteredGeometries(RegisteredGeometries)
{   
    // Сделать недействительными все геометрии в массиве.
    for (u32 i = 0; i < MaxGeometryCount; ++i) {
        RegisteredGeometries[i].geometry.id = INVALID::ID;
        RegisteredGeometries[i].geometry.generation = INVALID::U16ID;
    }
}

GeometrySystem* GeometrySystem::Initialize(u64& MemoryRequirement, void* memory, GeometrySystemConfig config)
{
    if (config.MaxGeometryCount == 0) {
        MFATAL("«GeometrySystem::Initialize» — максимальное количество геометрии должно быть > 0.");
        return nullptr;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 ArrayRequirement = sizeof(GeometryReference) * config.MaxGeometryCount;
    MemoryRequirement = sizeof(GeometrySystem) + ArrayRequirement;

    if (!memory) {
        return nullptr;
    }

    if (!state) {
        u8* PtrGeometrySystem = reinterpret_cast<u8*>(memory);
        auto ArrayBlock = reinterpret_cast<GeometryReference*>(PtrGeometrySystem + sizeof(GeometrySystem));
        state = new(PtrGeometrySystem) GeometrySystem(config.MaxGeometryCount, ArrayBlock);
    }

    if (!state->CreateDefaultGeometries()) {
        MFATAL("Не удалось создать геометрию по умолчанию. Приложение не может быть продолжено.");
        return nullptr;
    }

    return state;
}

GeometryConfig GeometrySystem::GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 TileX, f32 TileY, const char *name, const char *MaterialName)
{
    if (width == 0) {
        MWARN("Ширина(width) должна быть ненулевой. По умолчанию один.");
        width = 1.F;
    }
    if (height == 0) {
        MWARN("Высота(height) должна быть ненулевой. По умолчанию один.");
        height = 1.F;
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
        TileX = 1.F;
    }
    if (TileY == 0) {
        MWARN("TileY не должен быть нулевым. По умолчанию один.");
        TileY = 1.F;
    }

    u32 VertexCount = xSegmentCount * ySegmentCount * 4; // 4 вершины на сегмент
    u32 IndexCount  = xSegmentCount * ySegmentCount * 6; // 6 индексов на сегмент
    GeometryConfig config;
    config.VertexSize = sizeof(Vertex3D); 
    config.VertexCount = VertexCount;  
    config.vertices = MemorySystem::Allocate(VertexCount * config.VertexSize, Memory::Array, true);
    config.IndexSize = sizeof(u32);
    config.IndexCount = IndexCount;
    config.indices = reinterpret_cast<u32*>(MemorySystem::Allocate(IndexCount * config.IndexSize, Memory::Array, true));
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
        MWARN("Ширина должна быть ненулевой. По умолчанию 1.");
        width = 1.F;
    }
    if (height == 0) {
        MWARN("Высота должна быть ненулевой. По умолчанию 1.");
        height = 1.F;
    }
    if (depth == 0) {
        MWARN("Глубина должна быть ненулевой. По умолчанию 1.");
        depth = 1;
    }
    if (TileX == 0) {
        MWARN("TileX не должен быть нулевым. По умолчанию 1.");
        TileX = 1.F;
    }
    if (TileY == 0) {
        MWARN("TileY не должно быть нулевым. По умолчанию 1.");
        TileY = 1.F;
    }

    f32 HalfWidth = width * 0.5F;
    f32 HalfHeight = height * 0.5F;
    f32 HalfDepth = depth * 0.5F;

    GeometryConfig config;
    config.VertexSize = sizeof(Vertex3D);
    config.VertexCount = 4 * 6; // 4 вершины на сторону, 6 сторон
    config.vertices = MemorySystem::Allocate(config.VertexCount * config.VertexSize, Memory::Array, true);
    config.IndexSize = sizeof(u32);
    config.IndexCount = 6 * 6; // 6 индексов на каждой стороне, 6 сторон
    config.indices = (u32*)MemorySystem::Allocate(config.IndexCount * config.IndexSize, Memory::Array, true);
    config.CopyNames(name, MaterialName);
    // config.center = FVec3();
    config.MinExtents = FVec3(-HalfWidth, -HalfHeight, -HalfDepth);
    config.MaxExtents = FVec3(HalfWidth, HalfHeight, HalfDepth);

    f32 MinUVx = 0.F;
    f32 MinUVy = 0.F;
    f32 MaxUVx = TileX;
    f32 MaxUVy = TileY;

    auto vertex  = (Vertex3D*)config.vertices;
    // Передняя поверхность
    vertex[0].position.SetXYZ(-HalfWidth, -HalfHeight,  HalfDepth);
    vertex[0].texcoord.Set(MinUVx, MinUVy);
    vertex[0].normal.SetXYZ(0.F,  0.F,  1.F);
    vertex[1].position.SetXYZ(HalfWidth,  HalfHeight,  HalfDepth);
    vertex[1].texcoord.Set(MaxUVx, MaxUVy);
    vertex[1].normal.SetXYZ(0.F,  0.F,  1.F);
    vertex[2].position.SetXYZ(-HalfWidth,  HalfHeight,  HalfDepth);
    vertex[2].texcoord.Set(MinUVx, MaxUVy);
    vertex[2].normal.SetXYZ(0.F,  0.F,  1.F);
    vertex[3].position.SetXYZ(HalfWidth, -HalfHeight,  HalfDepth);
    vertex[3].texcoord.Set(MaxUVx, MinUVy);
    vertex[3].normal.SetXYZ(0.F,  0.F,  1.F);
    // Задняя поверхность
    vertex[4].position.SetXYZ(HalfWidth, -HalfHeight, -HalfDepth);
    vertex[4].texcoord.Set(MinUVx, MinUVy);
    vertex[4].normal.SetXYZ(0.F,  0.F, -1.F);
    vertex[5].position.SetXYZ(-HalfWidth,  HalfHeight, -HalfDepth);
    vertex[5].texcoord.Set(MaxUVx, MaxUVy);
    vertex[5].normal.SetXYZ(0.F,  0.F, -1.F);
    vertex[6].position.SetXYZ(HalfWidth,  HalfHeight, -HalfDepth);
    vertex[6].texcoord.Set(MinUVx, MaxUVy);
    vertex[6].normal.SetXYZ(0.F,  0.F, -1.F);
    vertex[7].position.SetXYZ(-HalfWidth, -HalfHeight, -HalfDepth);
    vertex[7].texcoord.Set(MaxUVx, MinUVy);
    vertex[7].normal.SetXYZ(0.F,  0.F, -1.F);
    // Левая поверхность
    vertex[8].position.SetXYZ(-HalfWidth, -HalfHeight, -HalfDepth);
    vertex[8].texcoord.Set(MinUVx, MinUVy);
    vertex[8].normal.SetXYZ(-1.F,  0.F,  0.F);
    vertex[9].position.SetXYZ(-HalfWidth,  HalfHeight,  HalfDepth);
    vertex[9].texcoord.Set(MaxUVx, MaxUVy);
    vertex[9].normal.SetXYZ(-1.F,  0.F,  0.F);
    vertex[10].position.SetXYZ(-HalfWidth,  HalfHeight, -HalfDepth);
    vertex[10].texcoord.Set(MinUVx, MaxUVy);
    vertex[10].normal.SetXYZ(-1.F,  0.F,  0.F);
    vertex[11].position.SetXYZ(-HalfWidth, -HalfHeight,  HalfDepth);
    vertex[11].texcoord.Set(MaxUVx, MinUVy);
    vertex[11].normal.SetXYZ(-1.F,  0.F,  0.F);
    // Правая поверхность
    vertex[12].position.SetXYZ(HalfWidth, -HalfHeight,  HalfDepth);
    vertex[12].texcoord.Set(MinUVx, MinUVy);
    vertex[12].normal.SetXYZ(1.F,  0.F,  0.F);
    vertex[13].position.SetXYZ(HalfWidth,  HalfHeight, -HalfDepth);
    vertex[13].texcoord.Set(MaxUVx, MaxUVy);
    vertex[13].normal.SetXYZ(1.F,  0.F,  0.F);
    vertex[14].position.SetXYZ(HalfWidth,  HalfHeight,  HalfDepth);
    vertex[14].texcoord.Set(MinUVx, MaxUVy);
    vertex[14].normal.SetXYZ(1.F,  0.F,  0.F);
    vertex[15].position.SetXYZ(HalfWidth, -HalfHeight, -HalfDepth);
    vertex[15].texcoord.Set(MaxUVx, MinUVy);
    vertex[15].normal.SetXYZ(1.F,  0.F,  0.F);
    // Нижняя поверхность
    vertex[16].position.SetXYZ(HalfWidth, -HalfHeight,  HalfDepth);
    vertex[16].texcoord.Set(MinUVx, MinUVy);
    vertex[16].normal.SetXYZ(0.F, -1.F,  0.F);
    vertex[17].position.SetXYZ(-HalfWidth, -HalfHeight, -HalfDepth);
    vertex[17].texcoord.Set(MaxUVx, MaxUVy);
    vertex[17].normal.SetXYZ(0.F, -1.F,  0.F);
    vertex[18].position.SetXYZ(HalfWidth, -HalfHeight, -HalfDepth);
    vertex[18].texcoord.Set(MinUVx, MaxUVy);
    vertex[18].normal.SetXYZ(0.F, -1.F,  0.F);
    vertex[19].position.SetXYZ(-HalfWidth, -HalfHeight,  HalfDepth);
    vertex[19].texcoord.Set(MaxUVx, MinUVy);
    vertex[19].normal.SetXYZ(0.F, -1.F,  0.F);
    // Верхняя поверхность
    vertex[20].position.SetXYZ(-HalfWidth,  HalfHeight,  HalfDepth);
    vertex[20].texcoord.Set(MinUVx, MinUVy);
    vertex[20].normal.SetXYZ(0.F,  1.F,  0.F);
    vertex[21].position.SetXYZ(HalfWidth,  HalfHeight, -HalfDepth);
    vertex[21].texcoord.Set(MaxUVx, MaxUVy);
    vertex[21].normal.SetXYZ(0.F,  1.F,  0.F);
    vertex[22].position.SetXYZ(-HalfWidth,  HalfHeight, -HalfDepth);
    vertex[22].texcoord.Set(MinUVx, MaxUVy);
    vertex[22].normal.SetXYZ(0.F,  1.F,  0.F);
    vertex[23].position.SetXYZ(HalfWidth,  HalfHeight,  HalfDepth);
    vertex[23].texcoord.Set(MaxUVx, MinUVy);
    vertex[23].normal.SetXYZ(0.F,  1.F,  0.F);

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

GeometryConfig GeometrySystem::GenerateSphereSmoothConfig(u16 rings, u16 segments, f32 radius)
{
    if (!rings) {
        rings = 16;
    }
    if (!segments) {
        segments = 32;
    }
    if (!radius) {
        radius = 1.F;
    }
    
    f32 x, y, z, xy;                            // позиция вершины
    f32 nx, ny, nz, lengthInv = 1.F / radius;   // нормаль вершины
    f32 s, t;                                   // текстурные координаты вершины

    f32 sectorStep = 2 * M_PI / segments;
    f32 stackStep = M_PI / rings;
    f32 sectorAngle, stackAngle;
    
    u32 k1, k2, m = 0;

    GeometryConfig config;
    config.VertexSize = sizeof(Vertex3D);
    config.VertexCount = (rings - 1) * segments + 2; 
    config.vertices = MemorySystem::Allocate(config.VertexCount * config.VertexSize, Memory::Array, true);
    config.IndexSize = sizeof(u32);
    config.IndexCount = ((rings - 2) * (segments << 1) + (segments << 1)) * 3; 
    config.indices = (u32*)MemorySystem::Allocate(config.IndexCount * config.IndexSize, Memory::Array, true);
    
    auto vertices = (Vertex3D*)config.vertices;
    // сгенерировать список индексов треугольников сферы против часовой стрелки
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    for(u64 i = 0; i <= segments; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;  // от π/2 до -π/2
        xy = radius * Math::cos(stackAngle);    // r * cos(u)
        z  = radius * Math::sin(stackAngle);    // r * sin(u)

        k1 = i * (rings + 1);     // начало текущего сегмента
        k2 = k1 + rings + 1;      // начало следующего сегмента

        // добавьте (rings+1) вершин на каждый стек
        // первые и последние вершины имеют одинаковое положение и нормаль, но разные текстурные координаты
        for(u64 j = 0; j <= rings; ++j, ++k1, ++k2) {
            auto& vertex = vertices[j];
            sectorAngle = j * sectorStep;           // от 0 до 2π

            // позиция вершины (x, y, z)
            x = xy * Math::cos(sectorAngle);    // r * cos(u) * cos(v)
            y = xy * Math::sin(sectorAngle);    // r * cos(u) * sin(v)
            vertex.position.SetXYZ(x, y, z);

            // нормализованная нормаль вершины (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertex.normal.SetXYZ(nx, ny, nz);

            // координаты текстуры вершины (s, t) варьируются в пределах [0, 1]
            s = (f32)j / rings;
            t = (f32)i / segments;
            vertex.texcoord.Set(s, t);

            // 2 треугольника на сектор, исключая первый и последний стаканы
            // k1 => k2 => k1+1
            if(i != 0) {
                config.indices[m++] = k1;
                config.indices[m++] = k2;
                config.indices[m++] = k1 + 1;
            }

            // k1+1 => k2 => k2+1
            if(i != (segments - 1)) {
                config.indices[m++] = k1 + 1;
                config.indices[m++] = k2;
                config.indices[m++] = k2 + 1;
            }

            // сохранять индексы для линий
            // вертикальные линии для всех стаканов, k1 => k2
            // lineIndices.push_back(k1);
            // lineIndices.push_back(k2);
            // if(i != 0) { // горизонтальные линии, кроме первого стакана, k1 => k+1
            //     lineIndices.push_back(k1);
            //     lineIndices.push_back(k1 + 1);
            // }
        }
    }
    return config;
}

void GeometrySystem::DestroyGeometry(Geometry *geometry)
{
    SystemsManager::GetRenderingSystem()->Unload(geometry);

    geometry->id = INVALID::ID; 
    geometry->generation = INVALID::U16ID; 
    MemorySystem::ZeroMem(geometry->name, GEOMETRY_NAME_MAX_LENGTH);
    if (geometry->material && MString::Length(geometry->material->name) > 0) {
    MaterialSystem::Release(geometry->material->name);
    geometry->material = nullptr;
    }
}

bool GeometrySystem::CreateDefaultGeometries()
{
    Vertex3D verts[4]{};
    const f32 f = 10.F;

    verts[0].position.x = -0.5 * f;  // 0    3
    verts[0].position.y = -0.5 * f;  //

    verts[1].position.y = 0.5 * f;
    verts[1].position.x = 0.5 * f;
    verts[1].texcoord.x = 1.F;
    verts[1].texcoord.y = 1.F;

    verts[2].position.x = -0.5 * f;
    verts[2].position.y =  0.5 * f;
    verts[2].texcoord.y =  1.F;

    verts[3].position.x =  0.5 * f;
    verts[3].position.y = -0.5 * f;
    verts[3].texcoord.x =  1.F;

    u32 indices[6] = {0, 1, 2, 0, 3, 1};

    auto renderer = SystemsManager::GetRenderingSystem();
    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    Geometry& dg = state->DefaultGeometry;
    dg.VertexElementSize = sizeof(Vertex3D);
    dg.VertexCount = 4;
    dg.vertices = verts;
    dg.IndexElementSize = sizeof(u32);
    dg.IndexCount = 6;
    dg.indices = indices;

    if (!renderer->CreateGeometry(&dg)) {
        MFATAL("Не удалось создать геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    if (!renderer->Load(&state->DefaultGeometry)) {
        MFATAL("Не удалось загрузить геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }
    

    // Получите материал по умолчанию.
    state->DefaultGeometry.material = MaterialSystem::GetDefaultMaterial();

    // Создайте 2d-геометрию по умолчанию.
    Vertex2D verts2d[4]{};
    verts2d[0].position.x = -0.5 * f;  // 0    3
    verts2d[0].position.y = -0.5 * f;  //

    verts2d[1].position.y = 0.5 * f;
    verts2d[1].position.x = 0.5 * f;
    verts2d[1].texcoord.x = 1.F;
    verts2d[1].texcoord.y = 1.F;

    verts2d[2].position.x = -0.5 * f;
    verts2d[2].position.y = 0.5 * f;
    verts2d[2].texcoord.y = 1.F;

    verts2d[3].position.x = 0.5 * f;
    verts2d[3].position.y = -0.5 * f;
    verts2d[3].texcoord.x = 1.F;

    // Индексы (ПРИМЕЧАНИЕ: против часовой стрелки)
    u32 indices2d[6] = {2, 1, 0, 3, 0, 1};

    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    Geometry& d2dg = state->Default2dGeometry;
    d2dg.VertexElementSize = sizeof(Vertex2D);
    d2dg.VertexCount = 4;
    d2dg.vertices = verts2d;
    d2dg.IndexElementSize = sizeof(u32);
    d2dg.IndexCount = 6;
    d2dg.indices = indices2d;

    if (!renderer->CreateGeometry(&d2dg)) {
        MFATAL("Не удалось создать 2D-геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    if (!renderer->Load(&d2dg)) {
        MFATAL("Не удалось загрузить 2D-геометрию по умолчанию. Приложение не может быть продолжено.");
        return false;
    }
    

    // Получите материал по умолчанию.
    d2dg.material = MaterialSystem::GetDefaultUiMaterial();

    return true;
}

Geometry *GeometrySystem::Acquire(u32 id)
{
    if (id != INVALID::ID && state->RegisteredGeometries[id].geometry.id != INVALID::ID) {
        state->RegisteredGeometries[id].ReferenceCount++;
        return &state->RegisteredGeometries[id].geometry;
    }

    // ПРИМЕЧАНИЕ: Следует ли вместо этого возвращать геометрию по умолчанию?
    MERROR("GeometrySystem::Acquire не может загрузить неверный идентификатор геометрии. Возвращение nullptr.");
    return nullptr;
}

Geometry *GeometrySystem::Acquire(GeometryConfig& config, bool AutoRelease)
{
    Geometry* g = nullptr;
    for (u32 i = 0; i < state->MaxGeometryCount; ++i) {
        if (state->RegisteredGeometries[i].geometry.id == INVALID::ID) {
            // Поиск пустого слота.
            state->RegisteredGeometries[i].AutoRelease = AutoRelease;
            state->RegisteredGeometries[i].ReferenceCount = 1;
            g = &state->RegisteredGeometries[i].geometry;
            g->id = i;
            break;
        }
    }

    if (!g) {
        MERROR("Невозможно получить свободный слот для геометрии. Измените конфигурацию, чтобы освободить больше места. Возвращение nullptr.");
        return nullptr;
    }

    // if (!config.VertexCount || !config.vertices) {
    //     MERROR("RenderingSystem::Load требуются данные о вершинах, но они не были предоставлены. VertexCount=%d, vertices=%p", VertexCount, vertices);
    //     return false;
    // }

    // Сделайте идентификаторы недействительными. ПРИМЕЧАНИЕ: Не делайте идентификатор геометрии недействительным! На этом этапе у него должен быть действительный идентификатор, и его отмена может привести к хаосу.
    g->generation = INVALID::U16ID;

    // Сделайте копию данных вершин.
    g->VertexCount = config.VertexCount;
    g->VertexElementSize = config.VertexSize;
    g->vertices = config.vertices ? config.vertices : MemorySystem::Allocate(g->VertexElementSize * g->VertexCount, Memory::Renderer);
    // MemorySystem::CopyMem(this->vertices, vertices, VertexSize * VertexCount);

    g->IndexCount = config.IndexCount;
    g->IndexElementSize = config.IndexSize;
    g->indices = nullptr;
    // Если есть, сделайте копию данных индекса.
    if (g->IndexElementSize && g->IndexCount) {
        g->indices = config.indices ? config.indices : (u32*)MemorySystem::Allocate(g->IndexElementSize * g->IndexCount, Memory::Renderer);
        // MemorySystem::CopyMem(this->indices, indices, IndexSize * IndexCount);
    }

    // Копирование экстентов, центра и т.д.
    g->center = config.center;
    g->extents.min = config.MinExtents;
    g->extents.max = config.MaxExtents;

    // ЗАДАЧА: Сначала получить указатель на метриал и заполнить его имя, а при загрузке получить остальные данные
    // Получить материал
    if (MString::Length(config.MaterialName) > 0) {
        g->material = MaterialSystem::Acquire(config.MaterialName);
        if (!g->material) {
            g->material = MaterialSystem::GetDefaultMaterial();
        }
    }

    return g;
}

bool GeometrySystem::CreateGeometry(Geometry *geometry)
{
    if (!geometry) {
        MERROR("GeometrySystem::CreateGeometry требует действительного указателя на геометрию.");
        return false;
    }
    auto renderer = SystemsManager::GetRenderingSystem();
    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!renderer->CreateGeometry(geometry)) {
        MERROR("GeometrySystem::CreateGeometry - не удалось создать геометрию.");
        // Признать запись недействительной.
        state->RegisteredGeometries[geometry->id].ReferenceCount = 0;
        state->RegisteredGeometries[geometry->id].AutoRelease = false;
        geometry->id = INVALID::ID;
        geometry->generation = INVALID::U16ID;
        return false;
    }

    if (!renderer->Load(geometry)) {
        MERROR("GeometrySystem::CreateGeometry - не удалось создать геометрию.");
        // Сделайте запись недействительной.
        state->RegisteredGeometries[geometry->id].ReferenceCount = 0;
        state->RegisteredGeometries[geometry->id].AutoRelease = false;
        geometry->id = INVALID::ID;
        geometry->generation = INVALID::U16ID;

        return false;
    }

    geometry->generation++;

    return true;
}

bool GeometrySystem::CreateGeometry(u32 id)
{
    return CreateGeometry(&state->RegisteredGeometries[id].geometry);
}

void GeometrySystem::Release(Geometry *gid)
{
    if (gid && gid->id != INVALID::ID) {
        GeometryReference* ref = &state->RegisteredGeometries[gid->id];

        // Возьмите копию ID;
        if (ref->geometry.id == gid->id) {
            if (ref->ReferenceCount > 0) {
                ref->ReferenceCount--;
            }

            // Также удаляем идентификатор геометрии.
            if (ref->ReferenceCount < 1 && ref->AutoRelease) {
                DestroyGeometry(&ref->geometry);
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

Geometry *GeometrySystem::GetDefault()
{
    return &state->DefaultGeometry;

    MFATAL("GeometrySystem::GetDefault вызывается перед инициализацией системы. Возвращение nullptr.");
    return nullptr;
}

Geometry *GeometrySystem::GetDefault2D()
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
