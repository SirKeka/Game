/// @file camera.hpp
/// @author 
/// @brief 
/// @version 1.0
/// @date 2024-08-06
/// 
/// @copyright 
#pragma once

#include "math/vector3d_fwd.hpp"
#include "math/matrix4d.hpp"

/// @brief Представляет собой камеру, которую можно использовать для различных целей, 
/// особенно для рендеринга. В идеале они создаются и управляются системой камер.
class MAPI Camera
{
private:
    FVec3 position{};       // Положение этой камеры.
    FVec3 EulerRotation{};  // Вращение этой камеры с использованием углов Эйлера (тангаж, рыскание, крен).
    bool IsDirty{};         // Внутренний флаг, используемый для определения необходимости перестройки матрицы вида.
    Matrix4D ViewMatrix{};  // Матрица вида этой камеры.
public:
    /// @brief Создает новую камеру с нулевым положением и вращением по умолчанию, а также единичной матрицей вида. 
    /// В идеале для этого следует использовать систему камер, а не делать это напрямую.
    constexpr Camera() : position(), EulerRotation(), IsDirty(false), ViewMatrix(Matrix4D::MakeIdentity()) {}
    /*constexpr Camera(FVec3 position, FVec3 EulerRotation)
     : position(position), EulerRotation(EulerRotation), IsDirty(true), ViewMatrix(Matrix4D::MakeIdentity()) {}*/
    ~Camera() {}

    /// @brief Устанавливает для камеры по умолчанию нулевое вращение и положение, а также матрицу вида на идентичность.
    void Reset();

    /// @brief Получает ссылку положения камеры.
    /// @return Константная ссылка на положение камеры.
    const FVec3& GetPosition();

    /// @brief Устанавливает указанное положение камеры.
    /// @param position Положение, которое необходимо установить.
    void SetPosition(const FVec3& position);

    /// @brief Получает ссылку на вращение камеры в углах Эйлера.
    /// @return Константная ссылка на вращение камеры в углах Эйлера.
    const FVec3& GetRotationEuler();

    /// @brief Устанавливает поворот камеры в углах Эйлера.
    /// @param rotation Вращение в углах Эйлера, которое будет установлено.
    void SetRotationEuler(const FVec3& rotation);

    /// @brief Получает ссылку на матрицу вида камеры. Если камера грязная, создается, устанавливается и возвращается новая.
    /// @return Константная ссылка актуальной матрицы вида.
    const Matrix4D& GetView();

    /// @brief Возвращает копию прямого вектора камеры.
    /// @return Копия прямого вектора камеры.
    FVec3 Forward();

    /// @brief Возвращает копию обратного вектора камеры.
    /// @return Копия обратного вектора камеры.
    FVec3 Backward();

    /// @brief Возвращает копию левого вектора камеры.
    /// @return Копия левого вектора камеры.
    FVec3 Left();

    /// @brief Возвращает копию правого вектора камеры.
    /// @return Копия правого вектора камеры.
    FVec3 Right();

    /// @brief Возвращает копию верхнего вектора камеры
    /// @return Копия верхнего вектора камеры
    FVec3 Up();

    /// @brief Перемещает камеру вперед на указанную величину.
    /// @param amount Величина перемещения.
    void MoveForward(f32 amount);

    /// @brief Перемещает камеру назад на указанную величину.
    /// @param amount Величина перемещения.
    void MoveBackward(f32 amount);

    /// @brief Перемещает камеру влево на указанную величину.
    /// @param amount Величина перемещения.
    void MoveLeft(f32 amount);

    /// @brief Перемещает камеру вправо на указанную величину.
    /// @param amount Величина перемещения.
    void MoveRight(f32 amount);

    /// @brief Перемещает камеру вверх (прямо по оси Y) на указанную величину.
    /// @param amount Величина перемещения.
    void MoveUp(f32 amount);

    /// @brief Перемещает камеру вниз (прямо по оси Y) на указанную величину.
    /// @param amount Величина перемещения.
    void MoveDown(f32 amount);

    /// @brief Изменяет рыскание камеры на указанную величину.
    /// @param amount Величина регулировки.
    void Yaw(f32 amount);

    /// @brief Изменяет наклон камеры на указанную величину.
    /// @param amount Величина регулировки.
    void Pitch(f32 amount);
};
