#include "nine_slice.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "systems/geometry_system.h"

static void Gen(const char* name, NineSlice* nslice) 
{
    GeometryConfig OutConfig;
    OutConfig.VertexSize = sizeof(FVec2);
    OutConfig.VertexCount = 4 * 9;
    OutConfig.vertices = MemorySystem::Allocate(OutConfig.VertexSize * OutConfig.VertexCount, Memory::Array);
    OutConfig.IndexSize = sizeof(u32);
    OutConfig.IndexCount = 6 * 9;
    OutConfig.indices = (u32*)MemorySystem::Allocate(OutConfig.IndexSize * OutConfig.IndexCount, Memory::Array);
    MString::Copy(OutConfig.name, name, GEOMETRY_NAME_MAX_LENGTH);

    // Сгенерировать индексные данные для 9 квадратов.
    for (u32 i = 0; i < 9; ++i) {
        // Вершины
        u32 vIndex = i * 4;

        // Индексы — против часовой стрелки
        u32 iIndex = i * 6;
        OutConfig.indices[iIndex + 0] = vIndex + 2;
        OutConfig.indices[iIndex + 1] = vIndex + 1;
        OutConfig.indices[iIndex + 2] = vIndex + 0;
        OutConfig.indices[iIndex + 3] = vIndex + 3;
        OutConfig.indices[iIndex + 4] = vIndex + 0;
        OutConfig.indices[iIndex + 5] = vIndex + 1;
    }

    if (!nslice->Update((Vertex2D*)OutConfig.vertices)) {
        MERROR("Не удалось обновить NineSlice. Подробнее см. в журналах.");
    }

    
    // Получите геометрию пользовательского интерфейса из конфигурации. ПРИМЕЧАНИЕ: данные загружаются в графический процессор.
    auto geometry = nslice->geometry = GeometrySystem::Acquire(OutConfig, true);

    // Используйте те же массивы, что и для конфигурации.
    geometry->vertices = OutConfig.vertices;
    geometry->indices = OutConfig.indices;
    geometry->VertexElementSize = OutConfig.VertexSize;
    geometry->VertexCount = OutConfig.VertexCount;
    geometry->IndexElementSize = OutConfig.IndexSize;
    geometry->IndexCount = OutConfig.IndexCount;
}

NineSlice::NineSlice(const char *name, Point CornerSize, Point CornerPxSize, Point size, Point AtlasPxMin, Point AtlasPxMax, Point AtlasPxSize)
: CornerSize(CornerSize), CornerPxSize(CornerPxSize), size(size), AtlasPxMin(AtlasPxMin), AtlasPxMax(AtlasPxMax), AtlasPxSize(AtlasPxSize)
{
    Gen(name, this);
}

void NineSlice::Generate(const char *name)
{
    Gen(name, this);
}

void NineSlice::Generate(const char *name, NineSlice &nslice)
{
    Gen(name, &nslice);
}

bool NineSlice::Update(Vertex2D *vertices)
{
    // Генерация UVs.
    NineSlicePosTc pt[9];
    u8 ptIndex = 0;

    auto GenerateUvs = [this](f32 px_x, f32 px_y, f32& OutTx, f32& OutTy) {
        OutTx = (f32)px_x / AtlasPxSize.width;
        OutTy = (f32)px_y / AtlasPxSize.height;
    };
    // Сначала углы
    #pragma region Верхний левый
        GenerateUvs(AtlasPxMin.x, AtlasPxMin.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMin.x + CornerPxSize.x, AtlasPxMin.y + CornerPxSize.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = pt[ptIndex].PosyMin = 0.F;
        pt[ptIndex].PosxMax = CornerSize.x;
        pt[ptIndex].PosyMax = CornerSize.y;
    #pragma endregion
    #pragma region Верхний правый
        ptIndex++;
        GenerateUvs(AtlasPxMax.x - CornerPxSize.x, AtlasPxMin.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMax.x, AtlasPxMin.y + CornerPxSize.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = size.x - CornerSize.x;
        pt[ptIndex].PosyMin = 0.F;
        pt[ptIndex].PosxMax = size.x;
        pt[ptIndex].PosyMax = CornerSize.y;
    #pragma endregion
    #pragma region Нижний правый
        ptIndex++;
        GenerateUvs(AtlasPxMax.x - CornerPxSize.x, AtlasPxMax.y - CornerPxSize.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMax.x, AtlasPxMax.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = size.x - CornerSize.x;
        pt[ptIndex].PosyMin = size.y - CornerSize.y;
        pt[ptIndex].PosxMax = size.x;
        pt[ptIndex].PosyMax = size.y;
    #pragma endregion
    #pragma region Нижний левый
        ptIndex++;
        GenerateUvs(AtlasPxMin.x, AtlasPxMax.y - CornerPxSize.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMin.x + CornerPxSize.x, AtlasPxMax.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = 0.F;
        pt[ptIndex].PosyMin = size.y - CornerSize.y;
        pt[ptIndex].PosxMax = CornerSize.x;
        pt[ptIndex].PosyMax = size.y;
    #pragma endregion
    #pragma region Верхний центральный
        ptIndex++;
        GenerateUvs(AtlasPxMin.x + CornerPxSize.x, AtlasPxMin.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMax.x - CornerPxSize.x, AtlasPxMin.y + CornerPxSize.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = CornerSize.x;
        pt[ptIndex].PosyMin = 0.F;
        pt[ptIndex].PosxMax = size.x - CornerSize.x;
        pt[ptIndex].PosyMax = CornerSize.y;
    #pragma endregion
    #pragma region Нижний центральный
        ptIndex++;
        GenerateUvs(AtlasPxMin.x + CornerPxSize.x, AtlasPxMax.y - CornerPxSize.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMax.x - CornerPxSize.x, AtlasPxMax.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = CornerSize.x;
        pt[ptIndex].PosyMin = size.y - CornerSize.y;
        pt[ptIndex].PosxMax = size.x - CornerSize.x;
        pt[ptIndex].PosyMax = size.y;
    #pragma endregion
    #pragma region Средний левый
        ptIndex++;
        GenerateUvs(AtlasPxMin.x, AtlasPxMin.y + CornerPxSize.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMin.x + CornerPxSize.x, AtlasPxMax.y - CornerPxSize.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = 0.F;
        pt[ptIndex].PosyMin = CornerSize.y;
        pt[ptIndex].PosxMax = CornerSize.x;
        pt[ptIndex].PosyMax = size.y - CornerSize.y;
    #pragma endregion
    #pragma region Средний правый
        ptIndex++;
        GenerateUvs(AtlasPxMax.x - CornerPxSize.x, AtlasPxMin.y + CornerPxSize.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMax.x, AtlasPxMax.y - CornerPxSize.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = size.x - CornerSize.x;
        pt[ptIndex].PosyMin = CornerSize.y;
        pt[ptIndex].PosxMax = size.x;
        pt[ptIndex].PosyMax = size.y - CornerSize.y;
    #pragma endregion
    #pragma region Центр
        ptIndex++;
        GenerateUvs(AtlasPxMin.x + CornerPxSize.y, AtlasPxMin.y + CornerPxSize.y, pt[ptIndex].TxMin, pt[ptIndex].TyMin);
        GenerateUvs(AtlasPxMax.x - CornerPxSize.x, AtlasPxMax.y - CornerPxSize.y, pt[ptIndex].TxMax, pt[ptIndex].TyMax);
        // Формирование позиций.
        pt[ptIndex].PosxMin = CornerSize.x;
        pt[ptIndex].PosyMin = CornerSize.y;
        pt[ptIndex].PosxMax = size.x - CornerSize.x;
        pt[ptIndex].PosyMax = size.y - CornerSize.y;
    #pragma endregion

    bool UsingGeoVerts = false;
    if (!vertices) {
        UsingGeoVerts = true;
        vertices = (Vertex2D*)geometry->vertices;
    }
    // Обновите 9 четырёхугольников.
    for (u32 i = 0; i < 9; ++i) {
        // Вершины
        u32 vIndex = i * 4;

        vertices[vIndex + 0].position.x = pt[i].PosxMin;  // 0    3
        vertices[vIndex + 0].position.y = pt[i].PosyMin;  //
        vertices[vIndex + 0].texcoord.x = pt[i].TxMin;    //
        vertices[vIndex + 0].texcoord.y = pt[i].TyMin;    // 2    1

        vertices[vIndex + 1].position.x = pt[i].PosxMax;
        vertices[vIndex + 1].position.y = pt[i].PosyMax;
        vertices[vIndex + 1].texcoord.x = pt[i].TxMax;
        vertices[vIndex + 1].texcoord.y = pt[i].TyMax;

        vertices[vIndex + 2].position.x = pt[i].PosxMin;
        vertices[vIndex + 2].position.y = pt[i].PosyMax;
        vertices[vIndex + 2].texcoord.x = pt[i].TxMin;
        vertices[vIndex + 2].texcoord.y = pt[i].TyMax;

        vertices[vIndex + 3].position.x = pt[i].PosxMax;
        vertices[vIndex + 3].position.y = pt[i].PosyMin;
        vertices[vIndex + 3].texcoord.x = pt[i].TxMax;
        vertices[vIndex + 3].texcoord.y = pt[i].TyMin;
    }

    if (UsingGeoVerts) {
        // Загрузите новые данные вершин.
        SystemsManager::GetRenderingSystem()->GeometryVertexUpdate(geometry, 0, geometry->VertexCount, geometry->vertices);
    }

    return true;
}
