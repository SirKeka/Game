#include "terrain.h"
#include "core/identifier.h"
#include "core/systems_manager.h"
#include "math/geometry_utils.h"
#include "renderer/rendering_system.h"
#include "systems/material_system.h"

enum class TerrainSkirtSide : u8 {
    Left = 0,
    Right = 1,
    Top = 2,
    Bottom = 3,
    Count = 4
};

bool Terrain::Create(Config &config)
{
    name = (MString&&)config.name;

    if (!config.TileCountX) {
        MERROR("Количество плиток x не может быть меньше единицы.");
        return false;
    }

    if (!config.TileCountZ) {
        MERROR("Количество плиток z не может быть меньше единицы.");
        return false;
    }

    if (!config.ChunkSize) {
        MERROR("Размер блока не может быть меньше единицы.");
        return false;
    }

    xform = config.xform;

    extents = Extents3D();
    origin = FVec3();

    TileCountX = config.TileCountX;
    TileCountZ = config.TileCountZ;
    TileScaleX = config.TileScaleX;
    TileScaleZ = config.TileScaleZ;

    ScaleY = config.ScaleY;

    ChunkSize = config.ChunkSize;
    // Сделайте ландшафт недействительным, чтобы он не отрисовывался до того, как будет готов.
    generation = INVALID::ID;

    // Количество уровней детализации (LOD) рассчитывается следующим образом: 
    // берется размерность, определяется, сколько раз это число можно разделить на 2, 
    // результат округляется в меньшую сторону (взятие целой части). 
    // В итоге всегда получается значение не менее 1.
    LODcount = ChunkSize < 4 ? 1 : (u32)(Math::floor(Math::log2(ChunkSize)));

    // Выделите память для фрагментов (чанков).
    ChunkCount = (TileCountX / ChunkSize) * (TileCountZ / ChunkSize);
    CreateArray(chunks, sizeof(Chunk) * ChunkCount, true);
    
    // ПРИМЕЧАНИЕ: Предусмотрите дополнительный ряд/столбец в конце, чтобы общее количество тайлов соответствовало значению `chunk_size`.
    u32 VertexStride = ChunkSize + 1;
    u32 SurfaceVertexCount = VertexStride * VertexStride;
    TotalVertexCount = SurfaceVertexCount * ChunkCount;

    // Выделяем память под все вершины ландшафта
    CreateArray(vertices, sizeof(TerrainVertex) * TotalVertexCount, true);
    // Выделяем память под все индексы от LOD 0 уровня до последнего
    TotalIndexCount = 0; 
    for (u32 i = 0, LODtileStride = 2; i < LODcount; i++, LODtileStride *= 2) {
        u32 SurfaceIndexCount = LODtileStride * LODtileStride * 6;
        TotalIndexCount += SurfaceIndexCount; 
    }
     
    CreateArray(indices, sizeof(u32) * TotalIndexCount * ChunkCount, true);
    u32* idx = indices;

    for (u32 i = 0, k = 0; i < ChunkCount; ++i) {
        Chunk& chunk = chunks[i];
        chunk.SurfaceVertexCount = SurfaceVertexCount;
        // Общее количество вершин включает боковые «юбки» (side skirts).
        chunk.TotalVertexCount = chunk.SurfaceVertexCount; // + (VertexStride * 4);

        // Присваиваем указатель на вершины из общего массива
        chunk.vertices = vertices + (SurfaceVertexCount * i);

        // 
        CreateArray(chunk.lods, sizeof(ChunkLOD) * LODcount, true);
        for (u32 j = 0; j < LODcount; ++j, ++k) {
            ChunkLOD& lod = chunk.lods[j];

            u32 LODtileStride = (j == 0 ? ChunkSize : (u32)(ChunkSize  / (j * 2))); // * (1.F / (j * 2))));
            lod.SurfaceIndexCount = (LODtileStride * LODtileStride) * 6;
            lod.TotalIndexCount = lod.SurfaceIndexCount; // + (LODtileStride * 6 * 4);
            lod.indices = idx; // CreateArray(lod.indices, sizeof(u32) * lod.TotalIndexCount, true);
            idx += lod.SurfaceIndexCount;
        }

        // Аннулируйте фрагмент.
        chunk.generation = INVALID::U16ID;
    }

    // Данные высот.
    VertexDataLength = config.VertexDataLength;
    VertexDatas = config.VertexDatas;
    config.VertexDatas = nullptr;

    MaterialCount = config.MaterialCount;
    if (MaterialCount) {
        MaterialNames = config.MaterialNames;
        config.MaterialNames = nullptr;
    } else {
        MaterialNames = nullptr;
    }

    return true;
}

void Terrain::Destroy()
{
    // Если ландшафт всё ещё загружен, сначала выгрузите его.
    if (generation != INVALID::ID) {
        if (!Unload()) {
            MERROR("Не удалось корректно выгрузить ландшафт перед его уничтожением. Подробности см. в журналах.");
        }
    }

    // ЗАДАЧА: возможно нужно убрать
    if (name) {
        name.Destroy();
    }

    if (chunks) {
        // u32 VertexStride = ChunkSize + 1;
        // u32 SurfaceVertexCount = VertexStride * VertexStride;
        DestroyArray(vertices, sizeof(TerrainVertex) * TotalVertexCount);
        vertices = nullptr;
        TotalVertexCount = 0;

        DestroyArray(indices, sizeof(u32) * TotalIndexCount);
        indices = nullptr;
        TotalVertexCount = 0;

        for (u32 i = 0; i < ChunkCount; ++i) {
            Chunk& chunk = chunks[i];
            Destroy(chunk);
        }

        DestroyArray(chunks, sizeof(Chunk) * ChunkCount);
        chunks = nullptr;
        ChunkCount = 0;
    }

    if (MaterialNames) {
        DestroyArray(MaterialNames, sizeof(MString) * MaterialCount);
        MaterialNames = nullptr;
    }

    if (VertexDatas) {
        DestroyArray(VertexDatas, sizeof(TerrainVertexData) * VertexDataLength);
        VertexDataLength = 0;
        VertexDatas = nullptr;
    }

    // ПРИМЕЧАНИЕ: Не просто обнуляйте память, потому что некоторые структуры, такие как геометрия, должны иметь недействительные идентификаторы.
    id.UniqueID = INVALID::U64ID;
    ScaleY = TileScaleX = TileScaleZ = 0;
    TileCountX = TileCountZ = VertexDataLength = 0;
    origin = FVec3();
    extents = Extents3D();
}

bool Terrain::Initialize()
{
    u32 ChunkRowCount = TileCountZ / ChunkSize;
    u32 ChunkColCount = TileCountX / ChunkSize;

    for (u32 z = 0, i = 0; z < ChunkRowCount; z++) {
        for (u32 x = 0; x < ChunkColCount; ++x, ++i) {
            // Индексы чанков по осям x/z в пределах сетки ландшафта.
            u32 ChunkOffsetX = i % ChunkColCount;
            u32 ChunkOffsetZ = i / ChunkColCount;
            CalculateGeometry(chunks[i], ChunkOffsetX, ChunkOffsetZ);
        }
    }

    id.Generate();

    return true;
}

bool Terrain::Load()
{
    id.Generate();

    for (u32 i = 0; i < ChunkCount; ++i) {
        if (!ChunkLoad(chunks[i])) {
            // Устраните последствия сбоя.
            Destroy();
            MERROR("Не удалось загрузить фрагмент ландшафта, поэтому ландшафт не может быть загружен.");
            return false;
        }
    }

    // Пометить как пригодное для отрисовки.
    generation++;

    return true;
}

bool Terrain::ChunkLoad(Chunk &chunk)
{
    // ПРИМЕЧАНИЕ: Вместо использования здесь объекта geometry, который, по сути, 
    // объединяет набор данных вершин и индексов, для ландшафтов эти данные будут обрабатываться вручную.

    auto renderer = SystemsManager::GetRenderingSystem();
    // Загрузить данные вершин.
    RenderBuffer *VertexBuffer = renderer->GetRenderbuffer(RenderBufferType::Vertex);
    u64 TotalVertexSize = sizeof(TerrainVertex) * chunk.TotalVertexCount;
    if (!VertexBuffer->Allocate(TotalVertexSize, chunk.VertexBufferOffset)) {
        MERROR("Не удалось выделить память для данных вершин фрагмента ландшафта.");
        return false;
    }

    // ЗАДАЧА: Передача значения false в данном случае приводит к ожиданию очереди; эту операцию следует перенести в другую очередь.
    if (!renderer->RenderBufferLoadRange(*VertexBuffer, chunk.VertexBufferOffset, TotalVertexSize, chunk.vertices, false)) {
        MERROR("Не удалось выгрузить данные вершин для фрагмента ландшафта.");
        return false;
    }

    // Загрузить данные индексов для всех уровней детализации (LOD).
    RenderBuffer* IndexBuffer = renderer->GetRenderbuffer(RenderBufferType::Index);
    for (u32 i = 0; i < LODcount; ++i) {
        ChunkLOD& lod = chunk.lods[i];
        u32 TotalSize = sizeof(u32) * lod.TotalIndexCount;
        if (!IndexBuffer->Allocate(TotalSize, lod.IndexBufferOffset)) {
            MERROR("Не удалось выделить память для данных индексов уровня детализации (LOD) фрагмента ландшафта.");
            return false;
        }

        // ЗАДАЧА: Передача значения false в данном случае приводит к ожиданию очереди; эту операцию следует перенести в другую очередь.
        if (!renderer->RenderBufferLoadRange(*IndexBuffer, lod.IndexBufferOffset, TotalSize, lod.indices, false)) {
            MERROR("Не удалось выгрузить данные индексов для уровня детализации (LOD) фрагмента ландшафта.");
            return false;
        }
    }

    // Создайте материал ландшафта, скопировав свойства этих материалов в новый материал ландшафта.
    char TerrainMaterialName[MATERIAL_NAME_MAX_LENGTH]{};
    MString::Format(TerrainMaterialName, "terrain_mat_%s", name.c_str());
    // ПРИМЕЧАНИЕ: хотя технически ландшафт мог бы использовать один материал, такой подход позволяет каждому фрагменту (чанку) иметь свой собственный материал.
    chunk.material = MaterialSystem::Acquire(TerrainMaterialName, MaterialCount, MaterialNames, true);
    if (!chunk.material) {
        MWARN("Failed to acquire terrain material. Using defualt instead.");
        chunk.material = MaterialSystem::GetDefaultTerrainMaterial();
    }

    // Обновите процесс генерации, чтобы обеспечить корректный рендеринг.
    chunk.generation++;

    return true;
}

bool Terrain::Unload()
{
    // Незамедлительно признать ландшафт недействительным.
    generation = INVALID::ID;

    bool HasError = false;
    // Выгрузить все чанки.
    for (u32 i = 0; i < ChunkCount; ++i) {
        Chunk& chunk = chunks[i];
        if (!ChunkUnload(chunk)) {
            MERROR("Не удалось выгрузить фрагмент ландшафта. Подробности см. в журналах.");
            HasError = true;  // Зафиксировать ошибку, но продолжить работу.
        }
    }

    return !HasError;
}

bool Terrain::ChunkUnload(Chunk &chunk)
{
    bool HasError = false;

    // Немедленно пометить фрагмент ландшафта как недействительный, чтобы прекратить его отрисовку.
    chunk.generation = INVALID::U16ID;

    // Выгрузить все ресурсы из графического процессора (GPU), но не удалять данные вершин.
    // Это позволит выгружать и повторно загружать фрагменты по мере необходимости.

    // Освободить ссылку на материал.
    MaterialSystem::Release(chunk.material->name);
    chunk.material = nullptr;

    auto renderer = SystemsManager::GetRenderingSystem();
    if (chunk.vertices) {
        // ПРИМЕЧАНИЕ: поскольку геометрия здесь не используется, необходимо вручную освободить данные вершин и индексов.
        RenderBuffer *VertexBuffer = renderer->GetRenderbuffer(RenderBufferType::Vertex);
        if (!VertexBuffer->Free(sizeof(TerrainVertex) * chunk.TotalVertexCount, chunk.VertexBufferOffset)) {
            MERROR("Ошибка при освобождении данных вершин для фрагмента ландшафта. Подробности см. в журналах.");
            HasError = true;  // Установить флаг, указывающий на возникновение ошибки.
        }
    }

    // Освободить каждый уровень детализации (LOD).
    if (chunk.lods) {
        RenderBuffer *IndexBuffer = renderer->GetRenderbuffer(RenderBufferType::Index);
        for (u32 j = 0; j < LODcount; ++j) {
            ChunkLOD& lod = chunk.lods[j];
            if (lod.indices) {
                if (!IndexBuffer->Free(sizeof(u32) * lod.TotalIndexCount, lod.IndexBufferOffset)) {
                    MERROR("Ошибка при освобождении данных вершин для фрагмента ландшафта, уровень детализации (LOD) = %u. Подробности см. в журналах.", j);
                    HasError = true;  // Укажите, что произошла ошибка.
                }
            }
        }
    }

    return !HasError;
}

bool Terrain::Update()
{
    return true;
}

void Terrain::Destroy(Chunk &chunk)
{
    // Если объект всё ещё загружен, сначала выгрузите его.
    if (chunk.generation != INVALID::U16ID) {
        if (!ChunkUnload(chunk)) {
            // Запишите в лог, но продолжайте выполнение, так как сделать всё равно ничего нельзя.
            MERROR("Не удалось выгрузить фрагмент ландшафта перед его удалением. Подробности см. в журналах.");
        }
    }

    // Удалите данные вершин.
    // if (chunk.vertices) {
    //     DestroyArray(chunk.vertices, sizeof(TerrainVertex) * chunk.TotalVertexCount);
    //     chunk.vertices = nullptr;
    //     chunk.VertexBufferOffset = 0;
    //     chunk.TotalVertexCount = 0;
    // }

    // Удалите каждый уровень детализации (LOD).
    if (chunk.lods) {
        for (u32 j = 0; j < LODcount; ++j) {
            ChunkLOD& lod = chunk.lods[j];
            if (lod.indices) {
                // DestroyArray(lod.indices, sizeof(u32) * lod.TotalIndexCount);
                lod.indices = nullptr;
            }
        }
        // Не забудьте также очистить данные уровней детализации.
        DestroyArray(chunk.lods, sizeof(ChunkLOD) * LODcount);
        chunk.lods = nullptr;
    }
}

void Terrain::CalculateGeometry(Chunk &chunk, u32 ChunkOffsetX, u32 ChunkOffsetZ)
{
    // Базовая позиция (x, z) первой вершины в пределах чанка.
    f32 ChunkBasePosX = ChunkOffsetX * ChunkSize * TileScaleX;
    f32 ChunkBasePosZ = ChunkOffsetZ * ChunkSize * TileScaleZ;

    f32 y_min = 99999.F;
    f32 y_max = -99999.F;

    // Сгенерировать данные поверхности.
    // ПРИМЕЧАНИЕ: Добавляется еще одна строка или столбец в конце, чтобы получить количество тайлов, равное chunk_size.
    u32 VertexStride = ChunkSize + 1;
    for (u32 z = 0, i = 0; z < VertexStride; ++z) {
        for (u32 x = 0; x < VertexStride; ++x, ++i) {
            TerrainVertex& v = chunk.vertices[i];
            v.position.x = ChunkBasePosX + (x * TileScaleX);
            v.position.z = ChunkBasePosZ + (z * TileScaleZ);

            // Получить глобальное смещение (x, y) в массиве тайлов ландшафта.
            // ПРИМЕЧАНИЕ: Из-за наличия дополнительного ряда и столбца вершин 
            // первый ряд/столбец этого фрагмента должен совпадать 
            // с соответствующим рядом/столбцом предыдущего фрагмента.
            i32 globalx = x + (ChunkOffsetX * (VertexStride));
            if (ChunkOffsetX > 0) {
                globalx -= ChunkOffsetX;
            }
            i32 globalz = z + (ChunkOffsetZ * (VertexStride));
            if (ChunkOffsetZ > 0) {
                globalz -= ChunkOffsetZ;
            }

            u32 GlobalTerrainIndex = globalx + (globalz * (TileCountX + 1));
            if (GlobalTerrainIndex < 0) {
                GlobalTerrainIndex = 0;
            }

            TerrainVertexData& VertData = VertexDatas[GlobalTerrainIndex];
            f32 PointHeight = VertData.height;

            v.position.y = PointHeight * ScaleY;
            y_min = MMIN(y_min, v.position.y);
            y_max = MMAX(y_max, v.position.y);

            v.colour = FVec4(1.F);  // white;
            v.normal = FVec3(0.F, 1.F, 0.F);
            v.texcoord.x = ChunkOffsetX + (f32)x;
            v.texcoord.y = ChunkOffsetZ + (f32)z;

            // ПРИМЕЧАНИЕ: Назначение весовых коэффициентов по умолчанию выполняется на основе общей высоты. 
            // Более низким значениям индекса материала соответствуют меньшие высотные отметки.
            // ПРИМЕЧАНИЕ: Для правильного смешивания они должны перекрывать минимальные и максимальные значения.
            v.MaterialWeights[0] = Math::AttenuationMinMax(-0.2F,  0.2F, PointHeight);  // mid 0
            v.MaterialWeights[1] = Math::AttenuationMinMax( 0.F,   0.3F, PointHeight);  // mid .15
            v.MaterialWeights[2] = Math::AttenuationMinMax( 0.15F, 0.9F, PointHeight);  // mid 5
            v.MaterialWeights[3] = Math::AttenuationMinMax( 0.5F,  1.2F, PointHeight);  // mid 9
        }
    }

    // Сгенерируйте данные «юбки» (skirt) для каждой стороны.
    // u32 vvi = chunk.SurfaceVertexCount;
    //  Важен порядок: левая, правая, верхняя, затем нижняя.
    // for (u8 s = 0; s < (u8)TerrainSkirtSide::Count; ++s) {
    //     // Левый
    //     for (u32 i = 0; i < VertexStride; ++i, ++vvi) {
    //         // Исходная вершина
    //         TerrainVertex *sv = nullptr;
    //         if (s == (u8)TerrainSkirtSide::Left) {
    //             sv = &chunk.vertices[i * VertexStride];
    //         } else if (s == (u8)TerrainSkirtSide::Right) {
    //             sv = &chunk.vertices[(i * VertexStride) + ChunkSize];
    //         } else if (s == (u8)TerrainSkirtSide::Top) {
    //             sv = &chunk.vertices[i];
    //         } else {  // TerrainSkirtSide::Bottom
    //             sv = &chunk.vertices[i + (VertexStride * ChunkSize)];
    //         }

    //         // Целевая вершина
    //         TerrainVertex* v = &chunk.vertices[vvi];

    //         // Скопируйте данные исходной вершины в целевую, а затем измените высоту.
    //         MemorySystem::CopyMem(v, sv, sizeof(TerrainVertex));
    //         v->position.y -= 0.1F * ScaleY;
    //     }
    // }

    // Рассчитайте границы для этого фрагмента.
    // Используйте первую вершину поверхности для определения минимальных границ.
    chunk.extents.min = chunk.vertices[0].position;
    chunk.extents.min.y = y_min;
    // Используйте последнюю вершину поверхности для определения максимальных границ.
    chunk.extents.max = chunk.vertices[chunk.SurfaceVertexCount - 1].position;
    chunk.extents.max.y = y_max;

    chunk.center = chunk.extents.Half();

    // Сгенерируйте индексы для каждого уровня детализации (LOD).
    for (u32 j = 0; j < LODcount; ++j) {
        ChunkLOD& lod = chunk.lods[j];

        // Количество вершин, на которое смещаются циклы на каждой итерации для данного уровня детализации.
        u32 LODskipRate = (1 << j);

        // Индексы поверхности. Создайте 1 набор из 6 индексов для каждого тайла.
        for (u32 row = 0, i = 0; row < ChunkSize; row += LODskipRate) {
            for (u32 col = 0; col < ChunkSize; col += LODskipRate, i += 6) {
                u32 NextRow = row + LODskipRate;
                u32 NextCol = col + LODskipRate;
                u32 v0 = (row * VertexStride) + col;
                u32 v1 = (row * VertexStride) + NextCol;
                u32 v2 = (NextRow * VertexStride) + col;
                u32 v3 = (NextRow * VertexStride) + NextCol;

                lod.indices[i + 0] = v2;
                lod.indices[i + 1] = v1;
                lod.indices[i + 2] = v0;
                lod.indices[i + 3] = v3;
                lod.indices[i + 4] = v1;
                lod.indices[i + 5] = v2;
            }
        }

        // Сгенерируйте индексы для «юбки» (skirt), начиная с конца массивов вершин и индексов.
        // u32 ii = lod.SurfaceIndexCount;
        // u32 vi = chunk.SurfaceVertexCount;

        // Порядок важен: левая, правая, верхняя, затем нижняя стороны.
        // for (u8 s = 0; s < (u8)TerrainSkirtSide::Count; ++s) {
        //     // Выполните итерацию по вершинам с учетом шага пропуска (LOD skip rate).
        //     for (u32 i = 0; i < ChunkSize; i += LODskipRate, ii += 6, vi += LODskipRate) {
        //         // Найдите 2 вершины, расположенные вдоль края поверхности.
        //         u32 v0, v1;
        //         if (s == (u8)TerrainSkirtSide::Left) {
        //             v0 = i * VertexStride;
        //             v1 = (i + LODskipRate) * VertexStride;
        //         } else if (s == (u8)TerrainSkirtSide::Right) {
        //             v0 = (i * VertexStride) + (VertexStride - 1);
        //             v1 = ((i + LODskipRate) * VertexStride) + (VertexStride - 1);
        //         } else if (s == (u8)TerrainSkirtSide::Top) {
        //             v0 = i;
        //             v1 = i + LODskipRate;
        //         } else {  // Нижняя
        //             v0 = i + (VertexStride * ChunkSize);
        //             v1 = (i + LODskipRate) + (VertexStride * ChunkSize);
        //         }

        //         // Две другие — это вершины, расположенные непосредственно под ней.
        //         u32 v2 = vi;
        //         u32 v3 = vi + LODskipRate;

        //         if (s == (u8)TerrainSkirtSide::Left || s == (u8)TerrainSkirtSide::Bottom) {
        //             // Против часовой стрелки — для левой и нижней.
        //             lod.indices[ii + 0] = v0;
        //             lod.indices[ii + 1] = v3;
        //             lod.indices[ii + 2] = v1;
        //             lod.indices[ii + 3] = v0;
        //             lod.indices[ii + 4] = v2;
        //             lod.indices[ii + 5] = v3;
        //         } else {  // Правая, верхняя
        //             // По часовой стрелке — для правой и верхней.
        //             lod.indices[ii + 0] = v0;
        //             lod.indices[ii + 1] = v1;
        //             lod.indices[ii + 2] = v2;
        //             lod.indices[ii + 3] = v1;
        //             lod.indices[ii + 4] = v3;
        //             lod.indices[ii + 5] = v2;
        //         }
        //     }

        //     // Последнюю вершину пропускаем, так как петля выше уже использует `i` и `i + 1`.
        //     vi++;
        // }
    }

    // Генерируем только на основе первого уровня детализации (LOD); остальные должны интерполироваться естественным образом по мере пропуска вершин.
    Math::Geometry::GenerateTerrainNormals(chunk.SurfaceVertexCount, chunk.vertices, chunk.lods[0].SurfaceIndexCount, chunk.lods[0].indices);
    Math::Geometry::GenerateTerrainTangents(chunk.SurfaceVertexCount, chunk.vertices, chunk.lods[0].SurfaceIndexCount, chunk.lods[0].indices);
}
