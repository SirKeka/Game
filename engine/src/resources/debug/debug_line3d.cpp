#include "debug_line3d.h"
#include "core/identifier.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"

DebugLine3D::DebugLine3D(FVec3 Point0, FVec3 Point1, Transform *parent) 
:
id(),
name(),
Point0(Point0),
Point1(Point1),
colour(FVec4::One()), // Поумолчаню белый цвет
xform(),
IsDirty(true),
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
    id = INVALID::ID;
}

bool DebugLine3D::Create(FVec3 Point0, FVec3 Point1, Transform *parent)
{
    id.Generate();
    this->Point0 = Point0;
    this->Point1 = Point1;
    colour = FVec4::One();
    xform = Transform();
    IsDirty = true;
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

void DebugLine3D::SetColour(FVec4 colour)
{
    this->colour = colour;

    if (this->colour.a == 0) this->colour.a = 1.F;

    if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
        UpdateVertColour();
        IsDirty = true;
    }
    
}

void DebugLine3D::SetPoints(FVec3 Point0, FVec3 Point1)
{
    if (geometry.generation != INVALID::U16ID && VertexCount && vertices) {
        this->Point0 = Point0;
        this->Point1 = Point1;
        RecalculatePoints();
        IsDirty = true;
    }
}

bool DebugLine3D::Initialize()
{
    VertexCount = 2;  // Всего 2 очка за линию.
    CreateArray(vertices, sizeof(ColourVertex3D) * VertexCount);

    RecalculatePoints();
    UpdateVertColour();

    return true;
}

bool DebugLine3D::Load()
{
    id.Generate();
    // Отправьте геометрию в рендерер для загрузки в графический процессор.
    auto rsys = SystemsManager::GetRenderingSystem();

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

void DebugLine3D::RenderFramePrepare(const FrameData &rFrameData)
{
    if (!IsDirty) {
        return;
    }
    
    // Загрузите новые данные вершин.
    SystemsManager::GetRenderingSystem()->GeometryVertexUpdate(&geometry, 0, false);

    geometry.generation++;

    // Установите это на ноль, чтобы мы не заблокировали себя от обновления.
    if (geometry.generation == INVALID::U16ID) {
        geometry.generation = 0;
    }

    IsDirty = false;
}

bool DebugLine3D::Unload()
{
    SystemsManager::GetRenderingSystem()->Unload(&geometry);

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
