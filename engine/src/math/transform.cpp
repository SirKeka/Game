#include "transform.h"

void Transform::SetPosition(FVec3 position)
{
    this->position = position;
    IsDirty = true;
}

void Transform::SetPosition(f32 x, f32 y, f32 z)
{
    position.x = x; position.y = y; position.z = z;
    IsDirty = true;
}

void Transform::Translate(FVec3 translation)
{
    this->position += translation;
    IsDirty = true;
}

void Transform::SetRotation(Quaternion rotation)
{
    this->rotation = rotation;
    IsDirty = true;
}

void Transform::Rotate(Quaternion rotation)
{
    this->rotation *= rotation;
    IsDirty = true;
}

void Transform::SetScale(FVec3 scale)
{
    this->scale = scale;
    IsDirty = true;
}

void Transform::Scale(FVec3 scale)
{
    this->scale *= scale;
    IsDirty = true;
}

void Transform::SetPositionRotation(FVec3 position, Quaternion rotation)
{
    this->position = position;
    this->rotation = rotation;
    IsDirty = true;
}

void Transform::SetPositionRotationScale(FVec3 position, Quaternion rotation, FVec3 scale)
{
    this->position = position;
    this->rotation = rotation;
    this->scale = scale;
    IsDirty = true;
}

void Transform::TranslateRotate(FVec3 translation, Quaternion rotation)
{
    this->position += translation;
    this->rotation *= rotation;
    IsDirty = true;
}

constexpr Matrix4D &Transform::GetLocation()
{
    if (IsDirty) {
            Matrix4D tr { Matrix4D(rotation) * Matrix4D::MakeTranslation(position) };
            local = Matrix4D::MakeScale(scale) * tr;
            IsDirty = false;
        }

        return local;
}

Matrix4D Transform::GetWorld()
{
    auto m = GetLocal();
    if (parent) {
        m *= parent->GetWorld();
        // Сохраните определитель на самом нижнем уровне.
        determinant = m.Determinant();
        return m;
    }
    // Если родителя нет, сделайте это с использованием локальной матрицы.
    determinant = m.Determinant();
    return GetLocal();
}

const Matrix4D& Transform::GetLocal()
{
    if (IsDirty) {
        Matrix4D tr = Matrix4D(rotation) * Matrix4D::MakeTranslation(position);
        tr = Matrix4D::MakeScale(scale) * tr;
        local = tr;
        IsDirty = false;
    }

    return local;
}
