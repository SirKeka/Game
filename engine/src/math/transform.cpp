#include "transform.hpp"

MINLINE Transform *Transform::GetParent()
{
    return parent;
}

void Transform::SetParent(Transform *parent)
{
    this->parent = parent;
}

constexpr FVec3 &Transform::GetPosition()
{
    return position;
}

void Transform::SetPosition(const FVec3& position)
{
    this->position = position;
    IsDirty = true;
}

void Transform::Translate(const FVec3& translation)
{
    this->position += translation;
    IsDirty = true;
}

constexpr Quaternion &Transform::GetRotation()
{
    return rotation;
}

void Transform::SetRotation(const Quaternion &rotation)
{
    this->rotation = rotation;
    IsDirty = true;
}

void Transform::Rotate(const Quaternion &rotation)
{
    this->rotation *= rotation;
    IsDirty = true;
}

constexpr FVec3 &Transform::GetScale()
{
    return scale;
}

void Transform::SetScale(const FVec3 &scale)
{
    this->scale = scale;
    IsDirty = true;
}

void Transform::Scale(const FVec3 &scale)
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
            Matrix4D tr = Matrix4D(rotation) * Matrix4D::MakeTranslation(position);
            local = Matrix4D::MakeScale(scale) * tr;
            IsDirty = false;
        }

        return local;
}
