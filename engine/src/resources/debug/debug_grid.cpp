#include "debug_grid.h"
#include "renderer/rendering_system.h"

// DebugGrid::~DebugGrid()
// {
//     Destroy();
// }

void DebugGrid::Destroy()
{
    Identifier::ReleaseID(UniqueID);
}

bool DebugGrid::Initialize()
{
    vertices = reinterpret_cast<ColourVertex3D*>(MemorySystem::Allocate(sizeof(ColourVertex3D) * VertexCount, Memory::Array));

    // Длины линий сетки — это количество пробелов в противоположном направлении.
    i32 LineLength0 = TileCountDim1 * TileScale;
    i32 LineLength1 = TileCountDim0 * TileScale;
    i32 LineLength2 = LineLength0 > LineLength1 ? LineLength0 : LineLength1;

    // f32 Max0 = TileCountDim0 * TileScale;
    // f32 Min0 = -Max0;
    // f32 Max1 = TileCountDim1 * TileScale;
    // f32 Min1 = -Max1;

    u32 ElementIndex0, ElementIndex1, ElementIndex2;

    switch (orientation) {
        default:
        case Orientation::XZ:
            ElementIndex0 = 0;  // x
            ElementIndex1 = 2;  // z
            ElementIndex2 = 1;  // y
            break;
        case Orientation::XY:
            ElementIndex0 = 0;  // x
            ElementIndex1 = 1;  // y
            ElementIndex2 = 2;  // z
            break;
        case Orientation::YZ:
            ElementIndex0 = 1;  // y
            ElementIndex1 = 2;  // z
            ElementIndex2 = 0;  // x
            break;
    }

    // Первая линия оси
    vertices[0].position.elements[ElementIndex0] = -LineLength1;
    vertices[0].position.elements[ElementIndex1] = 0;
    vertices[1].position.elements[ElementIndex0] = LineLength1;
    vertices[1].position.elements[ElementIndex1] = 0;
    vertices[0].colour.elements[ElementIndex0] = 1.F;
    vertices[0].colour.a = 1.F;
    vertices[1].colour.elements[ElementIndex0] = 1.F;
    vertices[1].colour.a = 1.F;

    // Вторая линия оси
    vertices[2].position.elements[ElementIndex0] = 0;
    vertices[2].position.elements[ElementIndex1] = -LineLength0;
    vertices[3].position.elements[ElementIndex0] = 0;
    vertices[3].position.elements[ElementIndex1] = LineLength0;
    vertices[2].colour.elements[ElementIndex1] = 1.F;
    vertices[2].colour.a = 1.F;
    vertices[3].colour.elements[ElementIndex1] = 1.F;
    vertices[3].colour.a = 1.F;

    if (UseThirdAxis) {
        // Третья линия оси
        vertices[4].position.elements[ElementIndex0] = 0;
        vertices[4].position.elements[ElementIndex2] = -LineLength2;
        vertices[5].position.elements[ElementIndex0] = 0;
        vertices[5].position.elements[ElementIndex2] = LineLength2;
        vertices[4].colour.elements[ElementIndex2] = 1.F;
        vertices[4].colour.a = 1.F;
        vertices[5].colour.elements[ElementIndex2] = 1.F;
        vertices[5].colour.a = 1.F;
    }

    FVec4 AltLineColour = FVec4(1.F, 1.F, 1.F, 0.5F);
    // вычислить 4 линии за раз, по 2 в каждом направлении, мин/макс.
    i32 j = 1;

    u32 StartIndex = UseThirdAxis ? 6 : 4;

    for (u32 i = StartIndex; i < VertexCount; i += 8) {
        // Первая линия (макс)
        vertices[i + 0].position.elements[ElementIndex0] = j * TileScale;
        vertices[i + 0].position.elements[ElementIndex1] = LineLength0;
        vertices[i + 0].colour = AltLineColour;
        vertices[i + 1].position.elements[ElementIndex0] = j * TileScale;
        vertices[i + 1].position.elements[ElementIndex1] = -LineLength0;
        vertices[i + 1].colour = AltLineColour;

        // Вторая линия (мин)
        vertices[i + 2].position.elements[ElementIndex0] = -j * TileScale;
        vertices[i + 2].position.elements[ElementIndex1] = LineLength0;
        vertices[i + 2].colour = AltLineColour;
        vertices[i + 3].position.elements[ElementIndex0] = -j * TileScale;
        vertices[i + 3].position.elements[ElementIndex1] = -LineLength0;
        vertices[i + 3].colour = AltLineColour;

        // Третья линия (макс)
        vertices[i + 4].position.elements[ElementIndex0] = -LineLength1;
        vertices[i + 4].position.elements[ElementIndex1] = -j * TileScale;
        vertices[i + 4].colour = AltLineColour;
        vertices[i + 5].position.elements[ElementIndex0] = LineLength1;
        vertices[i + 5].position.elements[ElementIndex1] = -j * TileScale;
        vertices[i + 5].colour = AltLineColour;

        // Четвертая линия (мин)
        vertices[i + 6].position.elements[ElementIndex0] = -LineLength1;
        vertices[i + 6].position.elements[ElementIndex1] = j * TileScale;
        vertices[i + 6].colour = AltLineColour;
        vertices[i + 7].position.elements[ElementIndex0] = LineLength1;
        vertices[i + 7].position.elements[ElementIndex1] = j * TileScale;
        vertices[i + 7].colour = AltLineColour;

        j++;
    }

    geometry.InternalID = INVALID::ID;

    return true;
}

bool DebugGrid::Load()
{
    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!RenderingSystem::Load(&geometry, sizeof(ColourVertex3D), VertexCount, vertices, 0, 0, nullptr)) {
        return false;
    }
    return true;
}

bool DebugGrid::Unload()
{
    RenderingSystem::Unload(&geometry);

    Identifier::ReleaseID(UniqueID);

    return true;
}

bool DebugGrid::Update()
{
    return true;
}
