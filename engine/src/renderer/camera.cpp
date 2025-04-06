#include "camera.hpp"

void Camera::Reset()
{
    position = EulerRotation = FVec3(0, 0, 0);
    IsDirty = false;
    ViewMatrix = Matrix4D::MakeIdentity();
}

const FVec3& Camera::GetPosition()
{
    return position;
}

void Camera::SetPosition(const FVec3& position)
{
    this->position = position;
    IsDirty = true;
}

const FVec3& Camera::GetRotationEuler()
{
    return EulerRotation;
}

void Camera::SetRotationEuler(const FVec3& rotation)
{
    this->EulerRotation = rotation;
    this->EulerRotation *= Math::DegToRad();
    IsDirty = true;
}

const Matrix4D& Camera::GetView()
{
    if (IsDirty) {
        Matrix4D rotation = Matrix4D::MakeEulerXYZ(EulerRotation);
        Matrix4D translation = Matrix4D::MakeTranslation(position);

        ViewMatrix = rotation * translation;
        ViewMatrix.Inverse();

        IsDirty = false;
    }
    return ViewMatrix;
}

FVec3 Camera::Forward()
{
    return Matrix4D::Forward(GetView());
}

FVec3 Camera::Backward()
{
    return Matrix4D::Backward(GetView());
}

FVec3 Camera::Left()
{
    return Matrix4D::Left(GetView());
}

FVec3 Camera::Right()
{
    return Matrix4D::Right(GetView());
}

FVec3 Camera::Up()
{
    return Matrix4D::Up(GetView());
}

void Camera::MoveForward(f32 amount)
{
    position += (Forward() * amount);
    IsDirty = true;
}

void Camera::MoveBackward(f32 amount)
{
    position += (Backward() * amount);
    IsDirty = true;
}

void Camera::MoveLeft(f32 amount)
{
    position += (Left() * amount);
    IsDirty = true;
}

void Camera::MoveRight(f32 amount)
{
    position += (Right() * amount);
    IsDirty = true;
}

void Camera::MoveUp(f32 amount)
{
    position += (FVec3::Up() * amount);
    IsDirty = true;
}

void Camera::MoveDown(f32 amount)
{
    position += (FVec3::Down() * amount);
    IsDirty = true;
}

void Camera::Yaw(f32 amount)
{
    EulerRotation.y += amount;
    IsDirty = true;
}

void Camera::Pitch(f32 amount)
{
    EulerRotation.x += amount;

    // Зажим, чтобы избежать блокировки подвеса.
    static const f32 limit = 1.55334306f;  // 89 градусов или эквивалент deg_to_rad(89.0f);
    EulerRotation.x = MCLAMP(EulerRotation.x, -limit, limit);
    
    IsDirty = true;
}
