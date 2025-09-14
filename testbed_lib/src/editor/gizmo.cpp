#include "gizmo.h"
#include "renderer/rendering_system.h"

bool Gizmo::Create()
{
    mode = None;
    xform = Transform();

    // Инициализируйте значения по умолчанию для всех режимов.
    for (u32 i = 0; i < MAX + 1; ++i) {
        modeData[i].VertexCount = 0;
        modeData[i].vertices = nullptr;

        modeData[i].IndexCount = 0;
        modeData[i].indices = nullptr;
    }

    return true;
}

void Gizmo::Destroy()
{

}

bool Gizmo::Initialize()
{
    mode = None;

    CreateGizmoModeNone();
    CreateGizmoModeMove();
    CreateGizmoModeScale();
    CreateGizmoModeRotate();

    return true;
}

bool Gizmo::Load()
{
    for (u32 i = 0; i < MAX + 1; ++i) {
        if (!RenderingSystem::CreateGeometry(&modeData[i].geometry, sizeof(ColourVertex3D), modeData[i].VertexCount, modeData[i].vertices)) {
            MERROR("Не удалось создать тип геометрии гизмо: '%u'", i);
            return false;
        }
        if (!RenderingSystem::Load(&modeData[i].geometry)) {
            MERROR("Не удалось загрузить тип геометрии гизмо: '%u'", i);
            return false;
        }
        if (modeData[i].geometry.generation == INVALID::U16ID) {
            modeData[i].geometry.generation = 0;
        } else {
            modeData[i].geometry.generation++;
        }
    }

    return true;
}

bool Gizmo::Unload()
{
    return true;
}

void Gizmo::Update()
{

}

void Gizmo::ModeSet(Mode mode)
{
    this->mode = mode;
}

void Gizmo::CreateGizmoModeNone()
{
    auto& data = modeData[None];
    
    data.VertexCount = 6;  // 2 в строке, 3 строки
    data.vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * data.VertexCount, Memory::Array, true);
    FVec4 grey = FVec4(0.5F, 0.5F, 0.5F, 1.F);

    // x
    data.vertices[0].colour     = grey;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[1].colour     = grey;
    data.vertices[1].position.x = 1.F;

    // y
    data.vertices[2].colour     = grey;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[3].colour     = grey;
    data.vertices[3].position.y = 1.F;

    // z
    data.vertices[4].colour     = grey;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[5].colour     = grey;
    data.vertices[5].position.z = 1.F;
}

void Gizmo::CreateGizmoModeMove()
{
    auto& data = modeData[Move];

    data.VertexCount = 18;  // 2 в строке, 3 строки + 6 строк
    data.vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * data.VertexCount, Memory::Array, true);

    FVec4 r = FVec4(1.F, 0.F, 0.F, 1.F);
    FVec4 g = FVec4(0.F, 1.F, 0.F, 1.F);
    FVec4 b = FVec4(0.F, 0.F, 1.F, 1.F);
    // x
    data.vertices[0].colour     = r;
    data.vertices[0].position.x = 0.2F;
    data.vertices[1].colour     = r;
    data.vertices[1].position.x = 1.F;

    // y
    data.vertices[2].colour     = g;
    data.vertices[2].position.y = 0.2F;
    data.vertices[3].colour     = g;
    data.vertices[3].position.y = 1.F;

    // z
    data.vertices[4].colour     = b;
    data.vertices[4].position.z = 0.2F;
    data.vertices[5].colour     = b;
    data.vertices[5].position.z = 1.F;

    // x "box" lines
    data.vertices[6].colour     = r;
    data.vertices[6].position.x = 0.4F;
    data.vertices[7].colour     = r;
    data.vertices[7].position.x = 0.4F;
    data.vertices[7].position.y = 0.4F;

    data.vertices[8].colour     = r;
    data.vertices[8].position.x = 0.4F;
    data.vertices[9].colour     = r;
    data.vertices[9].position.x = 0.4F;
    data.vertices[9].position.z = 0.4F;

    // y "box" lines
    data.vertices[10].colour     = g;
    data.vertices[10].position.y = 0.4F;
    data.vertices[11].colour     = g;
    data.vertices[11].position.y = 0.4F;
    data.vertices[11].position.z = 0.4F;

    data.vertices[12].colour     = g;
    data.vertices[12].position.y = 0.4F;
    data.vertices[13].colour     = g;
    data.vertices[13].position.y = 0.4F;
    data.vertices[13].position.x = 0.4F;

    // z "box" lines
    data.vertices[14].colour     = b;
    data.vertices[14].position.z = 0.4F;
    data.vertices[15].colour     = b;
    data.vertices[15].position.z = 0.4F;
    data.vertices[15].position.y = 0.4F;

    data.vertices[16].colour     = b;
    data.vertices[16].position.z = 0.4F;
    data.vertices[17].colour     = b;
    data.vertices[17].position.z = 0.4F;
    data.vertices[17].position.x = 0.4F;
}

void Gizmo::CreateGizmoModeScale()
{
    auto& data = modeData[Scale];

    data.VertexCount = 12;  // 2 per line, 3 lines + 3 lines
    data.vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * data.VertexCount, Memory::Array, true);

    FVec4 r = FVec4(1.F, 0.F, 0.F, 1.F);
    FVec4 g = FVec4(0.F, 1.F, 0.F, 1.F);
    FVec4 b = FVec4(0.F, 0.F, 1.F, 1.F);

    // x
    data.vertices[0].colour     = r;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[1].colour     = r;
    data.vertices[1].position.x = 1.F;

    // y
    data.vertices[2].colour     = g;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[3].colour     = g;
    data.vertices[3].position.y = 1.F;

    // z
    data.vertices[4].colour     = b;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[5].colour     = b;
    data.vertices[5].position.z = 1.F;

    // внешняя линия x/y
    data.vertices[6].position.x = 0.8F;
    data.vertices[6].colour     = r;
    data.vertices[7].position.y = 0.8F;
    data.vertices[7].colour     = g;

    // внешняя линия z/y
    data.vertices[8].position.z = 0.8F;
    data.vertices[8].colour     = b;
    data.vertices[9].position.y = 0.8F;
    data.vertices[9].colour     = g;

    // внешняя линия x/z
    data.vertices[10].position.x = 0.8F;
    data.vertices[10].colour    = r;
    data.vertices[11].position.z = 0.8F;
    data.vertices[11].colour    = b;
}

void Gizmo::CreateGizmoModeRotate()
{
    auto& data = modeData[Rotate];
    const u8 segments = 32;
    const f32 radius = 1.F;

    data.VertexCount = 12 + (segments * 2 * 3);  // 2 в строке, 3 строки + 3 строки
    data.vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * data.VertexCount, Memory::Array, true);

    FVec4 r = FVec4(1.F, 0.F, 0.F, 1.F);
    FVec4 g = FVec4(0.F, 1.F, 0.F, 1.F);
    FVec4 b = FVec4(0.F, 0.F, 1.F, 1.F);

    // Начните с центра, нарисуйте малые оси.
    // x
    data.vertices[0].colour     = r;  // Первая вершина находится в начале координат, положение не требуется.
    data.vertices[1].colour     = r;
    data.vertices[1].position.x = 0.2F;

    // y
    data.vertices[2].colour     = g;  // Первая вершина находится в начале координат, положение не требуется.
    data.vertices[3].colour     = g;
    data.vertices[3].position.y = 0.2F;

    // z
    data.vertices[4].colour     = b;  // Первая вершина находится в начале координат, положение не требуется.
    data.vertices[5].colour     = b;
    data.vertices[5].position.z = 0.2F;

    // Для каждой оси создайте точки по окружности.
    u32 j = 6;
    // z
    for (u32 i = 0; i < segments; ++i, j += 2) {
        // По 2 точки за раз, чтобы сформировать линию.
        f32 theta = (f32)i / segments * M_2PI;
        data.vertices[j].position.x = radius * Math::cos(theta);
        data.vertices[j].position.y = radius * Math::sin(theta);
        data.vertices[j].colour = b;

        theta = (f32)((i + 1) % segments) / segments * M_2PI;
        data.vertices[j + 1].position.x = radius * Math::cos(theta);
        data.vertices[j + 1].position.y = radius * Math::sin(theta);
        data.vertices[j + 1].colour = b;
    }

    // y
    for (u32 i = 0; i < segments; ++i, j += 2) {
        // По 2 точки за раз, чтобы сформировать линию.
        f32 theta = (f32)i / segments * M_2PI;
        data.vertices[j].position.x = radius * Math::cos(theta);
        data.vertices[j].position.z = radius * Math::sin(theta);
        data.vertices[j].colour = g;

        theta = (f32)((i + 1) % segments) / segments * M_2PI;
        data.vertices[j + 1].position.x = radius * Math::cos(theta);
        data.vertices[j + 1].position.z = radius * Math::sin(theta);
        data.vertices[j + 1].colour = g;
    }

    // x
    for (u32 i = 0; i < segments; ++i, j += 2) {
        // По 2 точки за раз, чтобы сформировать линию.
        f32 theta = (f32)i / segments * M_2PI;
        data.vertices[j].position.y = radius * Math::cos(theta);
        data.vertices[j].position.z = radius * Math::sin(theta);
        data.vertices[j].colour = r;

        theta = (f32)((i + 1) % segments) / segments * M_2PI;
        data.vertices[j + 1].position.y = radius * Math::cos(theta);
        data.vertices[j + 1].position.z = radius * Math::sin(theta);
        data.vertices[j + 1].colour = r;
    }
}
