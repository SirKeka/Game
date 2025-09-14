#include "debug_line3d.h"
#include "core/identifier.h"
#include "renderer/rendering_system.h"

constexpr DebugLine3D::DebugLine3D(const FVec3& Point0, const FVec3& Point1, Transform *parent) 
:
UniqueID(Identifier::AquireNewID(this)),
name(),
Point0(Point0),
Point1(Point1),
colour(FVec4::One()), // Поумолчаню белый цвет
xform(),
VertexCount(),
vertices(nullptr),
geometry()
{
    if (parent) {
        xform.SetParent(parent);
    }
    
    // UniqueID = Identifier::AquireNewID(this);
    // geo.id = INVALID_ID;
    // geo.generation = INVALID_ID_U16;
    // geo.internal_id = INVALID_ID;
}

// DebugLine3D::~DebugLine3D()
// {
//     Destroy();
// }

void DebugLine3D::Destroy()
{
    Identifier::ReleaseID(UniqueID);
}

bool DebugLine3D::Create(const FVec3 &Point0, const FVec3 &Point1, Transform *parent)
{
    UniqueID = Identifier::AquireNewID(this);
    this->Point0 = Point0;
    this->Point1 = Point1;
    colour = FVec4::One();
    xform = Transform();
    geometry = Geometry();
    if (parent) {
        xform.SetParent(parent);
    }
    return true;
}

void DebugLine3D::SetParent(Transform *parent)
{
    xform.SetParent(parent);
}

void DebugLine3D::SetColour(const FVec4 &colour)
{
    this->colour = colour;

    if (this->colour.a == 0) this->colour.a = 1.F;

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

void DebugLine3D::SetPoints(const FVec3 &Point0, const FVec3 &Point1)
{
    if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
        this->Point0 = Point0;
        this->Point1 = Point1;
        RecalculatePoints();

        // Загрузите новые данные вершин.
        RenderingSystem::GeometryVertexUpdate(&geometry, 0, VertexCount, vertices);

        geometry.generation++;

        // Установите это на ноль, чтобы мы не заблокировали себя от обновления.
        if (geometry.generation == INVALID::U16ID) {
            geometry.generation = 0;
        }
    }
}

bool DebugLine3D::Initialize()
{
    VertexCount = 2;  // Всего 2 очка за линию.
    vertices = reinterpret_cast<ColourVertex3D*>(MemorySystem::Allocate(sizeof(ColourVertex3D) * VertexCount, Memory::Array));

    RecalculatePoints();
    UpdateVertColour();

    return true;
}

bool DebugLine3D::Load()
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

bool DebugLine3D::Unload()
{
    RenderingSystem::Unload(&geometry);

    return true;
}

bool DebugLine3D::Update()
{
    return true;
}

// void *DebugLine3D::operator new(u64 size)
// {
//     return MemorySystem::Allocate(size, Memory::Resource);
// }

void DebugLine3D::UpdateVertColour()
{
    if (VertexCount && vertices) {
        for (u32 i = 0; i < VertexCount; ++i) {
            vertices[i].colour = colour;
        }
    }
}

void DebugLine3D::RecalculatePoints()
{
    vertices[0].position = FVec4(Point0, 1.F);
    vertices[1].position = FVec4(Point1, 1.F);
}
