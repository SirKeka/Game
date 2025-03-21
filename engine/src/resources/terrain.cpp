#include "terrain.h"
#include "renderer/rendering_system.h"
#include "systems/material_system.h"
#include "math/geometry_utils.hpp"

bool Terrain::Create(Config &config)
{
    name = config.name;

    if (!config.TileCountX) {
        MERROR("Количество плиток x не может быть меньше единицы.");
        return false;
    }

    if (!config.TileCountZ) {
        MERROR("Количество плиток z не может быть меньше единицы.");
        return false;
    }

    xform = config.xform;

    // ЗАДАЧА: рассчитать на основе фактических размеров местности.
    extents = Extents3D();
    origin = FVec3();

    TileCountX = config.TileCountX;
    TileCountZ = config.TileCountZ;
    TileScaleX = config.TileScaleX;
    TileScaleZ = config.TileScaleZ;

    ScaleY = config.ScaleY;

    VertexCount = TileCountX * TileCountZ;
    vertices = reinterpret_cast<TerrainVertex*>(MemorySystem::Allocate(sizeof(TerrainVertex) * VertexCount, Memory::Array));

    VertexDataLength = VertexCount;
    // VertexDatas = reinterpret_cast<TerrainVertexData*>(MemorySystem::Allocate(sizeof(TerrainVertexData) * VertexDataLength, Memory::Array));
    // MemorySystem::CopyMem(VertexDatas, config.VertexDatas, config.VertexDataLength * sizeof(TerrainVertexData));
    VertexDatas = config.VertexDatas.MovePtr();

    IndexCount = VertexCount * 6;
    indices = reinterpret_cast<u32*>(MemorySystem::Allocate(sizeof(u32) * IndexCount, Memory::Array));

    MaterialCount = config.MaterialCount;
    if (MaterialCount) {
        MaterialNames = reinterpret_cast<char**>(MemorySystem::Allocate(sizeof(char*) * MaterialCount, Memory::Array));
        MemorySystem::CopyMem(MaterialNames, config.MaterialNames, sizeof(char*) * MaterialCount);
    } else {
        MaterialNames = nullptr;
    }

    // Сделать геометрию недействительной.
    geo.id = INVALID::ID;
    geo.InternalID = INVALID::ID;
    geo.generation = INVALID::U16ID;

    return true;
}

void Terrain::Destroy()
{
    // ЗАДАЧА: Заполните меня!

    // ЗАДАЧА: возможно нужно убрать
    if (name) {
        name.Clear();
    }

    if (vertices) {
        MemorySystem::Free(vertices, sizeof(TerrainVertex) * VertexCount, Memory::Array);
        vertices = nullptr;
    }

    if (indices) {
        MemorySystem::Free(indices, sizeof(u32) * IndexCount, Memory::Array);
        indices = nullptr;
    }

    if (MaterialNames) {
        MemorySystem::Free(MaterialNames, sizeof(char*) * MaterialCount, Memory::Array);
        MaterialNames = nullptr;
    }

    if (VertexDatas) {
        MemorySystem::Free(VertexDatas, sizeof(TerrainVertexData) * VertexDataLength, Memory::Array);
        VertexDatas = nullptr;
    }

    // ПРИМЕЧАНИЕ: Не просто обнуляйте память, потому что некоторые структуры, такие как геометрия, должны иметь недействительные идентификаторы.
    IndexCount = VertexCount = 0;
    ScaleY = TileScaleX = TileScaleZ = 0;
    TileCountX = TileCountZ = VertexDataLength = 0;
    origin = FVec3();
    extents = Extents3D();
}

bool Terrain::Initialize()
{
    // Генерировать вершины.
    for (u32 z = 0, i = 0; z < TileCountZ; z++) {
        for (u32 x = 0; x < TileCountX; ++x, ++i) {
            auto& v = vertices[i];
            v.position.x = x * TileScaleX;
            v.position.z = z * TileScaleZ;
            v.position.y = VertexDatas[i].height * ScaleY;

            v.colour = FVec4::One();    // белый;
            v.normal = FVec3(0, 1, 0);  // ЗАДАЧА: рассчитать на основе геометрии.
            v.texcoord.x = (f32)x;
            v.texcoord.y = (f32)z;

            // ПРИМЕЧАНИЕ: Назначение весов по умолчанию на основе общей высоты. Более низкие индексы ниже по высоте.
            v.MaterialWeights[0] = Math::Smoothstep(0.00F, 0.25F, VertexDatas[i].height);
            v.MaterialWeights[1] = Math::Smoothstep(0.25F, 0.50F, VertexDatas[i].height);
            v.MaterialWeights[2] = Math::Smoothstep(0.50F, 0.75F, VertexDatas[i].height);
            v.MaterialWeights[3] = Math::Smoothstep(0.75F, 1.00F, VertexDatas[i].height);
        }
    }

    // Генерация индексов.
    for (u32 z = 0, i = 0; z < TileCountZ - 1; z++) {
        for (u32 x = 0; x < TileCountX - 1; ++x, i += 6) {
            u32 v0 = (z * TileCountX) + x;
            u32 v1 = (z * TileCountX) + x + 1;
            u32 v2 = ((z + 1) * TileCountX) + x;
            u32 v3 = ((z + 1) * TileCountX) + x + 1;

            // v0, v1, v2, v2, v1, v3
            indices[i + 0] = v2;
            indices[i + 1] = v1;
            indices[i + 2] = v0;
            indices[i + 3] = v3;
            indices[i + 4] = v1;
            indices[i + 5] = v2;
        }
    }

    Math::Geometry::GenerateTerrainNormals(VertexCount, vertices, IndexCount, indices);
    Math::Geometry::GenerateTerrainTangents(VertexCount, vertices, IndexCount, indices);

    return true;
}

bool Terrain::Load()
{
    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!RenderingSystem::Load(&geo, sizeof(TerrainVertex), VertexCount,
                                  vertices, sizeof(u32), IndexCount,
                                  indices)) {
        return false;
    }

    // Скопируйте экстенты, центр и т. д.
    geo.center = origin;
    geo.extents.min = extents.min;
    geo.extents.max = extents.max;
    // ЗАДАЧА: выгрузите приращения генерации на фронтенд. Также сделайте это в GeometrySystem::Create.
    geo.generation++;

    // Создайте материал ландшафта, скопировав свойства этих материалов в новый материал ландшафта.
    char TerrainMaterialName[MATERIAL_NAME_MAX_LENGTH]{};
    MString::Format(TerrainMaterialName, "terrain_mat_%s", name.c_str());
    geo.material = MaterialSystem::Acquire(TerrainMaterialName, MaterialCount, (const char **)MaterialNames, true);
    if (!geo.material) {
        MWARN("Не удалось получить материал ландшафта. Вместо этого используется материал по умолчанию.");
        geo.material = MaterialSystem::GetDefaultTerrainMaterial();
    }

    return true;
}

bool Terrain::Unload()
{
    MaterialSystem::Release(geo.material->name);
    RenderingSystem::Unload(&geo);

    return true;
}

bool Terrain::Update()
{
    return true;
}
