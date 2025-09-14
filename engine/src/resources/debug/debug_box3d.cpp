#include "debug_box3d.h"
#include "renderer/rendering_system.h"

// DebugBox3D::~DebugBox3D()
// {
//     Destroy();
// }

void DebugBox3D::Destroy()
{
    Identifier::ReleaseID(UniqueID);
}

void DebugBox3D::SetParent(Transform *parent)
{
    xform.SetParent(parent);
}

void DebugBox3D::SetColour(const FVec4 &colour)
{
    if (!this->colour.a) {
        this->colour.a = 1.F;
    }

    this->colour = colour;
    
    if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
        UpdateVertColour();

        // Загрузите новые данные вершин.
        RenderingSystem::GeometryVertexUpdate(&geometry, 0, VertexCount, vertices);

        geometry.generation++;

        // Установите это на ноль, чтобы мы не заблокировали себя от обновления.
        if (geometry.generation == INVALID::U16ID) {
            geometry.generation = 0;
        }
    }
}

void DebugBox3D::SetExtents(const Extents3D &extents)
{
    if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
        RecalculateExtents(extents);

        // Загрузите новые данные вершин.
        RenderingSystem::GeometryVertexUpdate(&geometry, 0, VertexCount, vertices);

        geometry.generation++;

        // Установите это на ноль, чтобы мы не заблокировали себя от обновления.
        if (geometry.generation == INVALID::U16ID) {
            geometry.generation = 0;
        }
    }
}

bool DebugBox3D::Initialize()
{
    VertexCount = 2 * 12;  // 12 линий, чтобы сделать куб.
    vertices = reinterpret_cast<ColourVertex3D*>(MemorySystem::Allocate(sizeof(ColourVertex3D) * VertexCount, Memory::Array));

    Extents3D extents;
    extents.min.x = -size.x * 0.5F;
    extents.min.y = -size.y * 0.5F;
    extents.min.z = -size.z * 0.5F;
    extents.max.x = size.x * 0.5F;
    extents.max.y = size.y * 0.5F;
    extents.max.z = size.z * 0.5F;
    RecalculateExtents(extents);

    UpdateVertColour();

    return true;
}

bool DebugBox3D::Load()
{
    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    if (!RenderingSystem::CreateGeometry(&geometry, sizeof(ColourVertex3D), VertexCount, vertices)) {
        return false;
    }

    // Отправляем геометрию в рендерер для загрузки в графический процессор.
    if (!RenderingSystem::Load(&geometry)) {
        return false;
    }

    if (geometry.generation == INVALID::U16ID) {
        geometry.generation = 0;
    } else {
        geometry.generation++;
    }
    return true;
}

bool DebugBox3D::Unload()
{
    RenderingSystem::Unload(&geometry);

    return true;
}

bool DebugBox3D::Update()
{
    return true;
}

void DebugBox3D::RecalculateExtents(const Extents3D &extents)
{
    // Линии спереди
    {
        // верх
        vertices[0].position = FVec4(extents.min.x, extents.min.y, extents.min.z, 1.F);
        vertices[1].position = FVec4(extents.max.x, extents.min.y, extents.min.z, 1.F);
        // право
        vertices[2].position = FVec4(extents.max.x, extents.min.y, extents.min.z, 1.F);
        vertices[3].position = FVec4(extents.max.x, extents.max.y, extents.min.z, 1.F);
        // низ
        vertices[4].position = FVec4(extents.max.x, extents.max.y, extents.min.z, 1.F);
        vertices[5].position = FVec4(extents.min.x, extents.max.y, extents.min.z, 1.F);
        // слева
        vertices[6].position = FVec4(extents.min.x, extents.min.y, extents.min.z, 1.F);
        vertices[7].position = FVec4(extents.min.x, extents.max.y, extents.min.z, 1.F);
    }
    // линии сзади
    {
        // верх
        vertices[8].position = FVec4(extents.min.x, extents.min.y, extents.max.z, 1.F);
        vertices[9].position = FVec4(extents.max.x, extents.min.y, extents.max.z, 1.F);
        // право
        vertices[10].position = FVec4(extents.max.x, extents.min.y, extents.max.z, 1.F);
        vertices[11].position = FVec4(extents.max.x, extents.max.y, extents.max.z, 1.F);
        // низ
        vertices[12].position = FVec4(extents.max.x, extents.max.y, extents.max.z, 1.F);
        vertices[13].position = FVec4(extents.min.x, extents.max.y, extents.max.z, 1.F);
        // слева
        vertices[14].position = FVec4(extents.min.x, extents.min.y, extents.max.z, 1.F);
        vertices[15].position = FVec4(extents.min.x, extents.max.y, extents.max.z, 1.F);
    }

    // верхние соединительные линии
    {
        // слева
        vertices[16].position = FVec4(extents.min.x, extents.min.y, extents.min.z, 1.F);
        vertices[17].position = FVec4(extents.min.x, extents.min.y, extents.max.z, 1.F);
        // справа
        vertices[18].position = FVec4(extents.max.x, extents.min.y, extents.min.z, 1.F);
        vertices[19].position = FVec4(extents.max.x, extents.min.y, extents.max.z, 1.F);
    }
    // нижние соединительные линии
    {
        // слева
        vertices[20].position = FVec4(extents.min.x, extents.max.y, extents.min.z, 1.F);
        vertices[21].position = FVec4(extents.min.x, extents.max.y, extents.max.z, 1.F);
        // справа
        vertices[22].position = FVec4(extents.max.x, extents.max.y, extents.min.z, 1.F);
        vertices[23].position = FVec4(extents.max.x, extents.max.y, extents.max.z, 1.F);
    }
}

void DebugBox3D::UpdateVertColour()
{
    if (VertexCount && vertices) {
        for (u32 i = 0; i < VertexCount; ++i) {
            vertices[i].colour = colour;
        }
    }
}
