#include "debug_box3d.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"

void DebugBox3D::Destroy()
{
    id = INVALID::ID;
}

void DebugBox3D::SetParent(Transform *parent)
{
    xform.SetParent(parent);
}

void DebugBox3D::SetPoints(FVec4 *points)
{
    if (points) {
        if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
            // for (u32 i = 0; i < 8; ++i) {
            //     vertices[i].position = points[i];
            // }

            // Линии фронта
            {
                // верхняя
                vertices[0].position = points[2];
                vertices[1].position = points[3];
                // правая
                vertices[2].position = points[1];
                vertices[3].position = points[2];
                // нижняя
                vertices[4].position = points[0];
                vertices[5].position = points[1];
                // левая
                vertices[6].position = points[3];
                vertices[7].position = points[0];
            }
            // задние линии
            {
                // верхняя
                vertices[8].position = points[6];
                vertices[9].position = points[7];
                // правая
                vertices[10].position = points[5];
                vertices[11].position = points[6];
                // нижняя
                vertices[12].position = points[4];
                vertices[13].position = points[5];
                // левая
                vertices[14].position = points[7];
                vertices[15].position = points[4];
            }

            // Верхние соединительные линии
            {
                // слева
                vertices[16].position = points[3];
                vertices[17].position = points[7];
                // справа
                vertices[18].position = points[2];
                vertices[19].position = points[6];
            }
            // Нижние соединительные линии
            {
                // слева
                vertices[20].position = points[0];
                vertices[21].position = points[4];
                // справа
                vertices[22].position = points[1];
                vertices[23].position = points[5];
            }
            IsDirty = true;
        }
    }
}

void DebugBox3D::RenderFramePrepare(const FrameData &rFrameData)
{
    // Загрузить новые данные вершин.
    SystemsManager::GetRenderingSystem()->GeometryVertexUpdate(&geometry, 0);

    geometry.generation++;

    // Обнулить это значение, чтобы не заблокировать возможность обновления.
    if (geometry.generation == INVALID::U16ID) {
        geometry.generation = 0;
    }
    IsDirty = false;
}

void DebugBox3D::SetColour(FVec4 colour)
{
    if (!this->colour.a) {
        this->colour.a = 1.F;
    }

    this->colour = colour;
    
    if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
        UpdateVertColour();
        IsDirty = true;
    }
}

void DebugBox3D::SetExtents(const Extents3D &extents)
{
    if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
        RecalculateExtents(extents);
        IsDirty = true;
    }
}

bool DebugBox3D::Initialize()
{
    VertexCount = 2 * 12;  // 12 линий, чтобы сделать куб.
    vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * VertexCount, Memory::Array);

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
    id.Generate();

    auto rsys = SystemsManager::GetRenderingSystem();

    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    geometry.VertexElementSize = sizeof(ColourVertex3D);
    geometry.VertexCount = VertexCount;
    geometry.vertices = vertices;

    if (!rsys->CreateGeometry(&geometry)) {
        return false;
    }

    // Отправляем геометрию в рендерер для загрузки в графический процессор.
    if (!rsys->Load(&geometry)) {
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
    SystemsManager::GetRenderingSystem()->Unload(&geometry);

    id = INVALID::ID;

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
