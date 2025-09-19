#include "gizmo.h"
#include "math/geometry_utils.h"
#include "math/quaternion.h"
#include "renderer/rendering_system.h"
#include "renderer/camera.h"

// ЗАДАЧА:
// - многоосевые вращения.
// - Гизмо должно быть активным/видимым только на выбранном объекте.
// - Перед началом редактирования необходимо заранее сделать копию преобразования, чтобы можно было отменить операцию.
// - Отменить можно, нажав правую кнопку мыши во время манипуляции или нажав Esc.
// - Отмена будет выполнена позже с помощью стека отмены.

constexpr static u8 segments = 32;
constexpr static f32 radius = 1.F;

bool Gizmo::Create()
{
    mode = NONE;
    xform = Transform();

    SelectedXform = nullptr;
    // Ориентация поумолчанию.
    orientation = Local;
    // orientation = Global;

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
    mode = NONE;

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

#ifdef _DEBUG
    PlaneNormalLine.Create(FVec3(), FVec3::One());
    PlaneNormalLine.Initialize();
    PlaneNormalLine.Load();
    // пурпурный
    PlaneNormalLine.SetColour(FVec4(1.F, 0.F, 1.F, 1.F));
#endif

    return true;
}

bool Gizmo::Unload()
{
#ifdef _DEBUG
        PlaneNormalLine.Unload();
        PlaneNormalLine.Destroy();
#endif
    return true;
}

void Gizmo::Refresh()
{
    if (SelectedXform) {
            // Установите положение.
            xform.SetPosition(SelectedXform->GetPosition());
            // Если локально, установите поворот.
            if (orientation == Local) {
                xform.SetRotation(SelectedXform->GetRotation());
            } else {
                xform.SetRotation(Quaternion::Identity());
            }
        } else {
            // На данный момент, сбросьте.
            xform.SetPosition(0);
            xform.SetRotation(Quaternion::Identity());
        }
}

Gizmo::Orientation Gizmo::GetOrientation()
{
    return orientation;
}

void Gizmo::SetOrientation(Orientation orientation)
{
    this->orientation = orientation;
#if _DEBUG
    switch (this->orientation) {
        case Global:
            MTRACE("Установка редактора gizmo в положение GLOBAL.");
            break;
        case Local:
            MTRACE("Установка редактора gizmo в положение LOCAL.");
            break;
    }
#endif
    Refresh();
}

void Gizmo::SetSelectedTransform(Transform *xform)
{
    SelectedXform = xform;
    Refresh();
}

void Gizmo::Update()
{

}

void Gizmo::SetMode(Mode mode)
{
    this->mode = mode;
}

void Gizmo::InteractionBegin(Camera *camera, Ray &ray, InteractionType iType)
{
    interaction = iType;

    if (interaction == InteractionType::MouseDrag) {
        auto& data = modeData[mode];

        auto GizmoWorld = xform.GetWorld();
        FVec3 origin = xform.GetPosition();
        FVec3 PlaneDirection;
        if (mode == MOVE || mode == SCALE) {
            // Create the plane.
            if (orientation == Local || orientation == Global) {
                switch (data.CurrentAxisIndex) {
                    case 0:  // ось х
                    case 3:  // оси xy
                        PlaneDirection = VectorTransform(FVec3::Backward(), 0.F, GizmoWorld);
                        break;
                    case 1:  // ось y
                    case 6:  // xyz
                        PlaneDirection = camera->Backward();
                        break;
                    case 4:  // оси xz
                        PlaneDirection = VectorTransform(FVec3::Up(), 0.F, GizmoWorld);
                        break;
                    case 2:  // ось z
                    case 5:  // оси yz
                        PlaneDirection = VectorTransform(FVec3::Right(), 0.F, GizmoWorld);
                        break;
                    default:
                        return;
                }
            } else {
                // ЗАДАЧА: Другие ориентации.
                return;
            }
            data.InteractionPlane.Create(origin, PlaneDirection);
            data.InteractionPlaneBack.Create(origin, PlaneDirection * -1.F);

#ifdef _DEBUG
            PlaneNormalLine.SetPoints(origin, origin + PlaneDirection);
#endif

            // Получите начальную точку пересечения луча с плоскостью.
            FVec3 intersection;
            f32 distance;
            if (!Math::RaycastPlane(ray, data.InteractionPlane, intersection, distance)) {
                // Попробуйте с другого направления.
                if (!Math::RaycastPlane(ray, data.InteractionPlaneBack, intersection, distance)) {
                    return;
                }
            }
            data.InteractionStartPosition = intersection;
            data.LastInteractionPosition = intersection;
        } else if (mode == ROTATE) {
            // ПРИМЕЧАНИЕ: Взаимодействие не требуется, так как текущая ось отсутствует.
            if (data.CurrentAxisIndex == INVALID::U8ID) {
                return;
            }
            MINFO("Начало взаимодействия с поворотом");
            // Создайте плоскость.
            auto GizmoWorld = xform.GetWorld();
            auto origin = xform.GetPosition();
            FVec3 PlaneDirection;
            switch (data.CurrentAxisIndex) {
                case 0:  // x
                    PlaneDirection = VectorTransform(FVec3::Left(), 0.F, GizmoWorld);
                    break;
                case 1:  // y
                    PlaneDirection = VectorTransform(FVec3::Down(), 0.F, GizmoWorld);
                    break;
                case 2:  // z
                    PlaneDirection = VectorTransform(FVec3::Forward(), 0.F, GizmoWorld);
                    break;
            }

            data.InteractionPlane.Create(origin, PlaneDirection);
            data.InteractionPlaneBack.Create(origin, PlaneDirection * -1.F);

#ifdef _DEBUG
            PlaneNormalLine.SetPoints(origin, origin + PlaneDirection);
#endif

            // Получите начальную точку пересечения луча с плоскостью.
            FVec3 intersection;
            f32 distance;
            if (!Math::RaycastPlane(ray, data.InteractionPlane, intersection, distance)) {
                // Попробуйте с другого направления.
                if (!Math::RaycastPlane(ray, data.InteractionPlaneBack, intersection, distance)) {
                    return;
                }
            }
            data.InteractionStartPosition = intersection;
            data.LastInteractionPosition = intersection;
        }
    }
}

void Gizmo::InteractionEnd()
{
    if (interaction == InteractionType::MouseDrag) {
        if (mode == ROTATE) {
            MINFO("Завершение поворота.");
            if (orientation == Global) {
                // Сброс поворота. Будет применено к выделенному фрагменту.
                xform.SetRotation(Quaternion::Identity());
            }
        }
    }

    interaction = InteractionType::None;
}

void Gizmo::HandleInteraction(Camera *camera, Ray &ray, InteractionType iType)
{
    auto& data = modeData[mode];
    if (mode == MOVE) {
        if (iType == InteractionType::MouseDrag) {
            // ПРИМЕЧАНИЕ: Не обрабатывайте взаимодействие, если текущая ось отсутствует.
            if (data.CurrentAxisIndex == INVALID::U8ID) {
                return;
            }
            auto GizmoWorld = xform.GetWorld();

            FVec3 intersection;
            f32 distance;
            if (!Math::RaycastPlane(ray, data.InteractionPlane, intersection, distance)) {
                // Попробуйте с другого направления.
                if (!Math::RaycastPlane(ray, data.InteractionPlaneBack, intersection, distance)) {
                    return;
                }
            }
            auto diff = intersection - data.LastInteractionPosition;
            FVec3 direction;
            FVec3 translation;

            if (orientation == Local || orientation == Global) {
                // Перемещайтесь вдоль линии текущей оси.
                switch (data.CurrentAxisIndex) {
                    case 0:  // x
                        direction = VectorTransform(FVec3::Right(), 0.F, GizmoWorld);
                        // Спроецируйте разницу на направление.
                        translation = direction * Dot(diff, direction);
                        break;
                    case 1:  // y
                        direction = VectorTransform(FVec3::Up(), 0.F, GizmoWorld);
                        // Спроецируйте разницу на направление.
                        translation = direction * Dot(diff, direction);
                        break;
                    case 2:  // z
                        direction = VectorTransform(FVec3::Forward(), 0.F, GizmoWorld);
                        // Спроецируйте разницу на направление.
                        translation = direction * Dot(diff, direction);
                        break;
                    case 3:  // xy
                    case 4:  // xz
                    case 5:  // yz
                    case 6:  // xyz
                        translation = diff;
                        break;
                    default:
                        return;
                }
            } else {
                // ЗАДАЧА: Другие ориентации.
                return;
            }
            xform.Translate(translation);
            data.LastInteractionPosition = intersection;

            // Применить преобразование к выделенному фрагменту.
            if (SelectedXform) {
                SelectedXform->Translate(translation);
            }
        } else if (iType == InteractionType::MouseHover) {
            f32 dist;
            xform.IsDirty = true;
            u8 HitAxis = INVALID::U8ID;

            // Перебрать все комбинации осей. Цикл в обратном порядке, чтобы отдать приоритет комбинациям, поскольку их области попадания гораздо меньше.
            auto GizmoWorld = xform.GetWorld();
            for (i32 i = 6; i > -1; --i) {
                if (Math::RaycastOrientedExtents(data.ModeExtents[i], GizmoWorld, ray, dist)) {
                    HitAxis = i;
                    break;
                }
            }

            // Управление подсветкой.
            FVec4 y = FVec4(1.F, 1.F, 0.F, 1.F);
            FVec4 r = FVec4(1.F, 0.F, 0.F, 1.F);
            FVec4 g = FVec4(0.F, 1.F, 0.F, 1.F);
            FVec4 b = FVec4(0.F, 0.F, 1.F, 1.F);

            if (data.CurrentAxisIndex != HitAxis) {
                data.CurrentAxisIndex = HitAxis;

                // Цвета основных осей
                for (u32 i = 0; i < 3; ++i) {
                    if (i == HitAxis) {
                        data.vertices[(i * 2) + 0].colour = y;
                        data.vertices[(i * 2) + 1].colour = y;
                    } else {
                        // Вернуть неиспользуемые оси к исходным цветам.
                        data.vertices[(i * 2) + 0].colour = FVec4(0.F, 0.F, 0.F, 1.F);
                        data.vertices[(i * 2) + 0].colour.elements[i] = 1.F;
                        data.vertices[(i * 2) + 1].colour = FVec4(0.F, 0.F, 0.F, 1.F);
                        data.vertices[(i * 2) + 1].colour.elements[i] = 1.F;
                    }
                }

                // xyz
                if (HitAxis == 6) {
                    // Окрасить их все в жёлтый цвет.
                    for (u32 i = 0; i < 18; ++i) {
                        data.vertices[i].colour = y;
                    }
                } else {
                    if (HitAxis == 3) {
                        // x/y
                        // 6/7, 12/13
                        data.vertices[6].colour = y;
                        data.vertices[7].colour = y;
                        data.vertices[12].colour = y;
                        data.vertices[13].colour = y;
                    } else {
                        data.vertices[6].colour = r;
                        data.vertices[7].colour = r;
                        data.vertices[12].colour = g;
                        data.vertices[13].colour = g;
                    }

                    if (HitAxis == 4) {
                        // x/z
                        // 8/9, 16/17
                        data.vertices[8].colour = y;
                        data.vertices[9].colour = y;
                        data.vertices[16].colour = y;
                        data.vertices[17].colour = y;
                    } else {
                        data.vertices[8].colour = r;
                        data.vertices[9].colour = r;
                        data.vertices[16].colour = b;
                        data.vertices[17].colour = b;
                    }

                    if (HitAxis == 5) {
                        // y/z
                        // 10/11, 14/15
                        data.vertices[10].colour = y;
                        data.vertices[11].colour = y;
                        data.vertices[14].colour = y;
                        data.vertices[15].colour = y;
                    } else {
                        data.vertices[10].colour = g;
                        data.vertices[11].colour = g;
                        data.vertices[14].colour = b;
                        data.vertices[15].colour = b;
                    }
                }
                RenderingSystem::GeometryVertexUpdate(&data.geometry, 0, data.VertexCount, data.vertices);
            }
        }
    } else if (mode == SCALE) {
        if (iType == InteractionType::MouseDrag) {
            // ПРИМЕЧАНИЕ: Не обрабатывайте взаимодействие, если текущей оси нет.
            if (data.CurrentAxisIndex == INVALID::U8ID) {
                return;
            }
            auto GizmoWorld = xform.GetWorld();

            FVec3 intersection;
            f32 distance;
            if (!Math::RaycastPlane(ray, data.InteractionPlane, intersection, distance)) {
                // Попробуйте с другого направления.
                if (!Math::RaycastPlane(ray, data.InteractionPlaneBack, intersection, distance)) {
                    return;
                }
            }
            FVec3 direction;
            FVec3 scale;
            auto origin = xform.GetPosition();

            // Масштабируйте вдоль линии текущей оси в локальном пространстве.
            // При необходимости это будет преобразовано в глобальное значение позже.
            switch (data.CurrentAxisIndex) {
                case 0:  // x
                    direction = FVec3::Right();
                    break;
                case 1:  // y
                    direction = FVec3::Up();
                    break;
                case 2:  // z
                    direction = FVec3::Forward();
                    break;
                case 3:  // xy
                    // Объедините 2 оси, масштабируйте по обеим.
                    direction = Normalize(FVec3::Right() + FVec3::Up() * 0.5F); 
                    break;
                case 4:  // xz
                    // Объедините 2 оси, масштабируйте по обеим.
                    direction = Normalize(FVec3::Right() + FVec3::Backward() * 0.5F);
                    break;
                case 5:  // yz
                    // Объедините 2 оси, масштабируйте по обеим.
                    direction = Normalize(FVec3::Backward() + FVec3::Up() * 0.5f);
                    break;
                case 6:  // xyz
                    direction = Normalize(FVec3::One());
                    break;
                default:
                    return;
            }
            // Расстояние от начала координат в конечном итоге определяет величину масштаба.
            f32 dist = Distance(origin, intersection);

            // Получите направление пересечения с началом координат.
            auto DirectionFromOrigin = Normalize(intersection - origin);

            // Получите преобразованное направление.
            FVec3 DirectionT;
            if (orientation == Local) {
                if (data.CurrentAxisIndex < 6) {
                    DirectionT = VectorTransform(direction, 0.F, GizmoWorld);
                } else {
                    // ПРИМЕЧАНИЕ: В случае равномерного масштаба за основу берется локальный вектор вверх.
                    DirectionT = VectorTransform(FVec3::Up(), 0.F, GizmoWorld);
                }
            } else if (orientation == Global) {
                // Используйте направление как есть.
                DirectionT = direction;
            } else {
                // ЗАДАЧА: Другие ориентации.

                // Используйте направление как есть.
                DirectionT = direction;
                return;
            }

            // Определите знак величины, вычислив скалярное произведение направления к пересечению из начала координат и его знака.
            f32 d = Math::Sign(Dot(DirectionT, DirectionFromOrigin));

            // Рассчитайте разницу масштабов, взяв знаковую величину и умножив на неё непреобразованное направление.
            scale = direction * (d * dist);

            // Для глобальных преобразований вычислите обратную величину поворота и примените её к масштабу, чтобы масштабировать по абсолютным (глобальным) осям, а не по локальным.
            if (orientation == Global) {
                if (SelectedXform) {
                    Quaternion quaternion = Inverse(SelectedXform->GetRotation());
                    scale = Rotate(scale, quaternion);
                }
            }

            MTRACE("масштаб (разница): [%.4f,%.4f,%.4f]", scale.x, scale.y, scale.z);
            // Применить масштаб к выбранному объекту.
            if (SelectedXform) {
                auto CurrentScale = SelectedXform->GetScale();

                // Применить масштаб, но только к изменившимся осям.
                for (u8 i = 0; i < 3; ++i) {
                    if (scale.elements[i] != 0.0f) {
                        CurrentScale.elements[i] = scale.elements[i];
                    }
                }
                MTRACE("Применение масштаба: [%.4f,%.4f,%.4f]", CurrentScale.x, CurrentScale.y, CurrentScale.z);
                SelectedXform->SetScale(CurrentScale);
            }
            data.LastInteractionPosition = intersection;
        } else if (iType == InteractionType::MouseHover) {
            f32 dist;
            xform.IsDirty = true;
            u8 HitAxis = INVALID::U8ID;

            // Пройдите цикл по каждой комбинации осей. Цикл в обратном порядке отдаёт приоритет комбинациям, поскольку их области попадания гораздо меньше.
            auto GizmoWorld = xform.GetWorld();
            for (i32 i = 6; i > -1; --i) {
                if (Math::RaycastOrientedExtents(data.ModeExtents[i], GizmoWorld, ray, dist)) {
                    HitAxis = i;
                    break;
                }
            }

            // Управление подсветкой.
            FVec4 y = FVec4(1.F, 1.F, 0.F, 1.F);
            FVec4 r = FVec4(1.F, 0.F, 0.F, 1.F);
            FVec4 g = FVec4(0.F, 1.F, 0.F, 1.F);
            FVec4 b = FVec4(0.F, 0.F, 1.F, 1.F);

            if (data.CurrentAxisIndex != HitAxis) {
                data.CurrentAxisIndex = HitAxis;

                // Цвета основных осей
                for (u32 i = 0; i < 3; ++i) {
                    if (i == HitAxis) {
                        data.vertices[(i * 2) + 0].colour = y;
                        data.vertices[(i * 2) + 1].colour = y;
                    } else {
                        // Верните вершинам, по которым нет попаданий, их первоначальные цвета.
                        data.vertices[(i * 2) + 0].colour = FVec4(0.F, 0.F, 0.F, 1.F);
                        data.vertices[(i * 2) + 0].colour.elements[i] = 1.F;
                        data.vertices[(i * 2) + 1].colour = FVec4(0.F, 0.F, 0.F, 1.F);
                        data.vertices[(i * 2) + 1].colour.elements[i] = 1.F;
                    }
                }

                // xyz
                if (HitAxis == 6) {
                    // Сделайте их все жёлтыми.
                    for (u32 i = 0; i < 12; ++i) {
                        data.vertices[i].colour = y;
                    }
                } else {
                    // x/y - 6/7
                    if (HitAxis == 3) {
                        data.vertices[6].colour = y;
                        data.vertices[7].colour = y;
                    } else {
                        data.vertices[6].colour = r;
                        data.vertices[7].colour = g;
                    }

                    // x/z - 10/11
                    if (HitAxis == 4) {
                        data.vertices[10].colour = y;
                        data.vertices[11].colour = y;
                    } else {
                        data.vertices[10].colour = r;
                        data.vertices[11].colour = b;
                    }

                    // z/y - 8/9
                    if (HitAxis == 5) {
                        data.vertices[8].colour = y;
                        data.vertices[9].colour = y;
                    } else {
                        data.vertices[8].colour = b;
                        data.vertices[9].colour = g;
                    }
                }
                RenderingSystem::GeometryVertexUpdate(&data.geometry, 0, data.VertexCount, data.vertices);
            }
        }
    } else if (mode == ROTATE) {
        if (iType == InteractionType::MouseDrag) {
            // ПРИМЕЧАНИЕ: Взаимодействие не требуется, если текущая ось отсутствует.
            if (data.CurrentAxisIndex == INVALID::U8ID) {
                return;
            }

            // auto GizmoWorld = xform.GetWorld();
            auto origin = xform.GetPosition();
            FVec3 interactionPosition;
            f32 distance;
            if (!Math::RaycastPlane(ray, data.InteractionPlane, interactionPosition, distance)) {
                // Попробуйте с другой стороны.
                if (!Math::RaycastPlane(ray, data.InteractionPlaneBack, interactionPosition, distance)) {
                    return;
                }
            }
            FVec3 direction;

            // Получите разницу в угле между этим взаимодействием и предыдущим и используйте её в качестве угла оси для поворота.
            FVec3 v_0 = data.LastInteractionPosition - origin;
            FVec3 v_1 = interactionPosition - origin;
            f32 angle = Math::acos(Dot(Normalize(v_0), Normalize(v_1)));
            // Отсутствие угла означает отсутствие изменений, поэтому загрузитесь.
            // ПРИМЕЧАНИЕ: Также проверьте на NaN, что возможно, поскольку числа с плавающей точкой обладают уникальным свойством (x != x) обнаруживать NaN.
            if (angle == 0 || angle != angle) {
                return;
            }
            FVec3 cross = Cross(v_0, v_1);
            if (Dot(data.InteractionPlane.GetNormal(), cross) < 0) {
                angle = -angle;
            }

            auto GizmoWorld = xform.GetWorld();
            switch (data.CurrentAxisIndex) {
                case 0:  // x
                    direction = VectorTransform(FVec3::Right(), 0.F, GizmoWorld);
                    break;
                case 1:  // y
                    direction = VectorTransform(FVec3::Up(), 0.F, GizmoWorld);
                    break;
                case 2:  // z
                    direction = VectorTransform(FVec3::Backward(), 0.F, GizmoWorld);
                    break;
                default:
                    return;
            }

            Quaternion rotation = Quaternion(direction, angle, true);
            // Примените поворот к гизмо здесь, чтобы он был виден.
            xform.Rotate(rotation);
            data.LastInteractionPosition = interactionPosition;

            // Применить вращение.
            if (SelectedXform) {
                SelectedXform->Rotate(rotation);
            }

        } else if (iType == InteractionType::MouseHover) {
            f32 dist;
            FVec3 point;
            auto model = xform.GetWorld();
            u8 HitAxis = INVALID::U8ID;

            //Пройти по каждой оси.
            for (u32 i = 0; i < 3; ++i) {
                // Ориентированный диск.
                FVec3 aaNormal;
                aaNormal.elements[i] = 1.0f;
                aaNormal = VectorTransform(aaNormal, 0.F, model);
                FVec3 center = xform.GetPosition();
                if (Math::RaycastDisc(ray, center, aaNormal, radius + 0.05F, radius - 0.05F, point, dist)) {
                    HitAxis = i;
                    break;
                } else {
                    // Если нет, попробуйте с другой стороны.
                    aaNormal = aaNormal * -1.F;
                    if (Math::RaycastDisc(ray, center, aaNormal, radius + 0.05F, radius - 0.05F, point, dist)) {
                        HitAxis = i;
                        break;
                    }
                }
            }

            u32 segments2 = segments * 2;
            if (data.CurrentAxisIndex != HitAxis) {
                data.CurrentAxisIndex = HitAxis;

                // Цвета основных осей
                for (u32 i = 0; i < 3; ++i) {
                    FVec4 SetColour{0.F, 0.F, 0.F, 1.F};
                    // Жёлтый для оси столкновений; в противном случае исходный цвет.
                    if (i == HitAxis) {
                        SetColour.r = 1.F;
                        SetColour.g = 1.F;
                    } else {
                        SetColour.elements[i] = 1.F;
                    }

                    // Основная ось в центре.
                    data.vertices[(i * 2) + 0].colour = SetColour;
                    data.vertices[(i * 2) + 1].colour = SetColour;

                    // Кольцо
                    u32 RingOffset = 6 + (segments2 * i);
                    for (u32 j = 0; j < segments; ++j) {
                        data.vertices[RingOffset + (j * 2) + 0].colour = SetColour;
                        data.vertices[RingOffset + (j * 2) + 1].colour = SetColour;
                    }
                }
            }

            RenderingSystem::GeometryVertexUpdate(&data.geometry, 0, data.VertexCount, data.vertices);
        }
    }
}

void Gizmo::CreateGizmoModeNone()
{
    auto& data = modeData[NONE];
    
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
    auto& data = modeData[MOVE];

    data.CurrentAxisIndex = INVALID::U8ID;
    data.VertexCount = 18;  // 2 в строке, 3 строки + 6 строк
    data.vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * data.VertexCount, Memory::Array, true);

    FVec4 r = FVec4(1.F, 0.F, 0.F, 1.F);
    FVec4 g = FVec4(0.F, 1.F, 0.F, 1.F);
    FVec4 b = FVec4(0.F, 0.F, 1.F, 1.F);
    // x
    data.vertices[0].colour     = r;
    data.vertices[0].position.x = 0.2F;
    data.vertices[1].colour     = r;
    data.vertices[1].position.x = 2.F;

    // y
    data.vertices[2].colour     = g;
    data.vertices[2].position.y = 0.2F;
    data.vertices[3].colour     = g;
    data.vertices[3].position.y = 2.F;

    // z
    data.vertices[4].colour     = b;
    data.vertices[4].position.z = 0.2F;
    data.vertices[5].colour     = b;
    data.vertices[5].position.z = 2.F;

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

    data.ExtentsCount = 7;
    data.ModeExtents = (Extents3D*)MemorySystem::Allocate(sizeof(Extents3D) * data.ExtentsCount, Memory::Array, true);

    // Создайте поля для каждой оси.
    // x
    auto ex = &data.ModeExtents[0];
    ex->min = FVec3(0.4F, -0.2F, -0.2F);
    ex->max = FVec3(2.1F, 0.2F, 0.2F);

    // y
    ex = &data.ModeExtents[1];
    ex->min = FVec3(-0.2F, 0.4F, -0.2F);
    ex->max = FVec3(0.2F, 2.1F, 0.2F);

    // z
    ex = &data.ModeExtents[2];
    ex->min = FVec3(-0.2F, -0.2F, 0.4F);
    ex->max = FVec3(0.2F, 0.2F, 2.1F);

    // Поля для комбинированных осей.
    // x-y
    ex = &data.ModeExtents[3];
    ex->min = FVec3(0.1F, 0.1F, -0.05F);
    ex->max = FVec3(0.5F, 0.5F, 0.05F);

    // x-z
    ex = &data.ModeExtents[4];
    ex->min = FVec3(0.1F, -0.05F, 0.1F);
    ex->max = FVec3(0.5F, 0.05F, 0.5F);

    // y-z
    ex = &data.ModeExtents[5];
    ex->min = FVec3(-0.05F, 0.1F, 0.1F);
    ex->max = FVec3(0.05F, 0.5F, 0.5F);

    // xyz
    ex = &data.ModeExtents[6];
    ex->min = FVec3(-0.1F, -0.1F, -0.1F);
    ex->max = FVec3(0.1F, 0.1F, 0.1F);
}

void Gizmo::CreateGizmoModeScale()
{
    auto& data = modeData[SCALE];

    data.CurrentAxisIndex = INVALID::U8ID;
    data.VertexCount = 12;  // 2 в строке, 3 строки + 3 строки
    data.vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * data.VertexCount, Memory::Array, true);

    FVec4 r = FVec4(1.F, 0.F, 0.F, 1.F);
    FVec4 g = FVec4(0.F, 1.F, 0.F, 1.F);
    FVec4 b = FVec4(0.F, 0.F, 1.F, 1.F);

    // x
    data.vertices[0].colour     = r;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[1].colour     = r;
    data.vertices[1].position.x = 2.F;

    // y
    data.vertices[2].colour     = g;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[3].colour     = g;
    data.vertices[3].position.y = 2.F;

    // z
    data.vertices[4].colour     = b;  // Первая вершина находится в начале координат, позиция не требуется.
    data.vertices[5].colour     = b;
    data.vertices[5].position.z = 2.F;

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

    data.ExtentsCount = 7;
    data.ModeExtents = (Extents3D*)MemorySystem::Allocate(sizeof(Extents3D) * data.ExtentsCount, Memory::Array, true);

    // Создайте поля для каждой оси.
    // x
    auto ex = &data.ModeExtents[0];
    ex->min = FVec3(0.4F, -0.2F, -0.2F);
    ex->max = FVec3(2.1F, 0.2F, 0.2F);

    // y
    ex = &data.ModeExtents[1];
    ex->min = FVec3(-0.2F, 0.4F, -0.2F);
    ex->max = FVec3(0.2F, 2.1F, 0.2F);

    // z
    ex = &data.ModeExtents[2];
    ex->min = FVec3(-0.2F, -0.2F, 0.4F);
    ex->max = FVec3(0.2F, 0.2F, 2.1F);

    // Поля для комбинированных осей.
    // x-y
    ex = &data.ModeExtents[3];
    ex->min = FVec3(0.1F, 0.1F, -0.05F);
    ex->max = FVec3(0.5F, 0.5F, 0.05F);

    // x-z
    ex = &data.ModeExtents[4];
    ex->min = FVec3(0.1F, -0.05F, 0.1F);
    ex->max = FVec3(0.5F, 0.05F, 0.5F);

    // y-z
    ex = &data.ModeExtents[5];
    ex->min = FVec3(-0.05F, 0.1F, 0.1F);
    ex->max = FVec3(0.05F, 0.5F, 0.5F);

    // xyz
    ex = &data.ModeExtents[6];
    ex->min = FVec3(-0.1F, -0.1F, -0.1F);
    ex->max = FVec3(0.1F, 0.1F, 0.1F);
}

void Gizmo::CreateGizmoModeRotate()
{
    auto& data = modeData[ROTATE];

    data.VertexCount = 12 + (segments * 2 * 3);  // 2 в строке, 3 строки + 3 строки
    data.vertices = (ColourVertex3D*)MemorySystem::Allocate(sizeof(ColourVertex3D) * data.VertexCount, Memory::Array, true);

    FVec4 red = FVec4(1.F, 0.F, 0.F, 1.F);
    FVec4 green = FVec4(0.F, 1.F, 0.F, 1.F);
    FVec4 blue = FVec4(0.F, 0.F, 1.F, 1.F);

    // Начните с центра, нарисуйте малые оси.
    // x
    data.vertices[0].colour     = red;  // Первая вершина находится в начале координат, положение не требуется.
    data.vertices[1].colour     = red;
    data.vertices[1].position.x = 0.2F;

    // y
    data.vertices[2].colour     = green;  // Первая вершина находится в начале координат, положение не требуется.
    data.vertices[3].colour     = green;
    data.vertices[3].position.y = 0.2F;

    // z
    data.vertices[4].colour     = blue;  // Первая вершина находится в начале координат, положение не требуется.
    data.vertices[5].colour     = blue;
    data.vertices[5].position.z = 0.2F;

    // Для каждой оси создайте точки по окружности.
    u32 j = 6;
    // x
    for (u32 i = 0; i < segments; ++i, j += 2) {
        // По 2 точки за раз, чтобы сформировать линию.
        f32 theta = (f32)i / segments * M_2PI;
        data.vertices[j].position.y = radius * Math::cos(theta);
        data.vertices[j].position.z = radius * Math::sin(theta);
        data.vertices[j].colour = red;

        theta = (f32)((i + 1) % segments) / segments * M_2PI;
        data.vertices[j + 1].position.y = radius * Math::cos(theta);
        data.vertices[j + 1].position.z = radius * Math::sin(theta);
        data.vertices[j + 1].colour = red;
    }

    // y
    for (u32 i = 0; i < segments; ++i, j += 2) {
        // По 2 точки за раз, чтобы сформировать линию.
        f32 theta = (f32)i / segments * M_2PI;
        data.vertices[j].position.x = radius * Math::cos(theta);
        data.vertices[j].position.z = radius * Math::sin(theta);
        data.vertices[j].colour = green;

        theta = (f32)((i + 1) % segments) / segments * M_2PI;
        data.vertices[j + 1].position.x = radius * Math::cos(theta);
        data.vertices[j + 1].position.z = radius * Math::sin(theta);
        data.vertices[j + 1].colour = green;
    }

    // z
    for (u32 i = 0; i < segments; ++i, j += 2) {
        // По 2 точки за раз, чтобы сформировать линию.
        f32 theta = (f32)i / segments * M_2PI;
        data.vertices[j].position.x = radius * Math::cos(theta);
        data.vertices[j].position.y = radius * Math::sin(theta);
        data.vertices[j].colour = blue;

        theta = (f32)((i + 1) % segments) / segments * M_2PI;
        data.vertices[j + 1].position.x = radius * Math::cos(theta);
        data.vertices[j + 1].position.y = radius * Math::sin(theta);
        data.vertices[j + 1].colour = blue;
    }
    // ПРИМЕЧАНИЕ: Механизм вращения использует диски, а не экстенты, поэтому в этом режиме они не нужны.
}
