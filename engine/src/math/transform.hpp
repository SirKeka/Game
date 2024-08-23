#pragma once
#include "vector3d_fwd.hpp"
#include "quaternion.hpp"
#include "matrix4d.hpp"

/// @brief Представляет преобразование объекта в мире.
/// Преобразования могут иметь родителя, собственное преобразование которого затем учитывается. 
/// ПРИМЕЧАНИЕ: Его свойства следует редактировать не напрямую, а для обеспечения правильной генерации матрицы.
class MAPI Transform {
    FVec3 position;         // Положение в мире.
    Quaternion rotation;    // Вращение в мире.
    FVec3 scale;            // Масштаб в мире.
    bool IsDirty;           // Указывает, изменилось ли положение, вращение или масштаб, что указывает на необходимость пересчета локальной матрицы.
    Matrix4D local;         // Матрица локального преобразования, обновляемая при изменении положения, поворота или масштаба.

    Transform* parent;      // Указатель на родительское преобразование, если оно назначено. Также может быть нулевым.
public:
    /// @brief Создает новое преобразование, используя нулевой вектор для положения, 
    /// идентификационный кватернион для вращения и один вектор для масштаба. 
    /// Также имеет нулевой родительский элемент. По умолчанию отмечено как dirty.
    constexpr Transform() 
    : 
    position(), 
    rotation(0.f, 0.f, 0.f, 1.f), 
    scale(FVec3::One()), 
    IsDirty(false), 
    local(Matrix4D::MakeIdentity()), 
    parent(nullptr) 
    {}
    /// @brief Создает преобразование из заданной позиции.
    /// Использует нулевое вращение и единичный масштаб.
    /// @param position позиция, которую нужно использовать.
    constexpr Transform(const FVec3& position) 
    : 
    position(position), 
    rotation(0.f, 0.f, 0.f, 1.f), 
    scale(FVec3::One()), 
    IsDirty(true), 
    local(Matrix4D::MakeIdentity()), 
    parent(nullptr) 
    {}
    /// @brief Создает преобразование из заданного поворота.
    /// Использует нулевое положение и единичный масштаб.
    /// @param rotation используемое вращение.
    constexpr Transform(const Quaternion& rotation) 
    :  
    position(), 
    rotation(rotation), 
    scale(FVec3::One()), 
    IsDirty(true), 
    local(Matrix4D::MakeIdentity()), 
    parent(nullptr) 
    {}
    /// @brief Создает преобразование из заданной позиции и поворота.
    /// Использует нулевое положение и единичный масштаб.
    /// @param position позиция, которую нужно использовать.
    /// @param rotation используемое вращение.
    constexpr Transform(const FVec3& position, const Quaternion& rotation)
    :
    position(position), 
    rotation(rotation), 
    scale(FVec3::One()), 
    IsDirty(true), 
    local(Matrix4D::MakeIdentity()), 
    parent(nullptr) 
    {}
    /// @brief Создает преобразование из заданной позиции и поворота.
    /// Использует нулевое положение.
    /// @param position позиция, которую нужно использовать.
    /// @param rotation используемое вращение.
    /// @param scale масштаб
    constexpr Transform(const FVec3& position, const Quaternion& rotation, const FVec3& scale)
    :
    position(position), 
    rotation(rotation), 
    scale(scale), 
    IsDirty(true), 
    local(Matrix4D::MakeIdentity()), 
    parent(nullptr) 
    {}
    ~Transform() {}

    /// @brief Возвращает указатель на родительский элемент предоставленного преобразования.
    /// @return Указатель на родительское преобразование.
    MINLINE Transform* GetParent();
    /// @brief Устанавливает родителя предоставленного преобразования.
    /// @param parent Указатель на родительское преобразование.
    void SetParent(Transform* parent);
    /// @brief Возвращает позицию данного преобразования.
    /// @return константную ссылку на позицию
    constexpr FVec3& GetPosition();
    /// @brief Устанавливает положение данного преобразования.
    /// @param position позиция, которую необходимо установить.
    void SetPosition(const FVec3& position);
    /// @brief Применяет преобразование к данному преобразованию. Это не то же самое, что SetPosition.
    /// @param translation Перевод, который необходимо применить.
    void Translate(const FVec3& translation);
    /// @brief Возвращает вращение данного преобразования.
    /// @return константную ссылку на кватернион
    constexpr Quaternion& GetRotation();
    /// @brief Устанавливает вращение данного преобразования.
    void SetRotation(const Quaternion& rotation);
    /// @brief Применяет вращение к данному преобразованию. Не то же самое, что SetRotation.
    /// @param rotation Применяемое вращение.
    void Rotate(const Quaternion& rotation);
    /// @brief Возвращает масштаб данного преобразования.
    /// @return константную ссылку на вектор отвечающий за масштаб
    constexpr FVec3& GetScale();
    /// @brief Устанавливает масштаб данного преобразования.
    /// @param scale масштаб
    void SetScale(const FVec3& scale);
    /// @brief Применяет масштаб к данному преобразованию. Не то же самое, что SetScale.
    /// @param scale Применяемый масштаб.
    void Scale(const FVec3& scale);
    /// @brief Устанавливает положение и вращение данного преобразования.
    /// @param position позиция, которую необходимо установить.
    /// @param rotation вращение, которое необходимо установить.
    void SetPositionRotation(FVec3 position, Quaternion rotation);
    /// @brief Устанавливает положение, вращение и масштаб данного преобразования.
    /// @param position позиция, которую необходимо установить.
    /// @param rotation вращение, которое необходимо установить.
    /// @param position масштаб, который необходимо установить.
    void SetPositionRotationScale(FVec3 position, Quaternion rotation, FVec3 scale);
    /// @brief Применяет перемещение и вращение к данному преобразованию.
    /// @param translation Перевод, который необходимо применить.
    /// @param rotation Применяемое вращение.
    void TranslateRotate(FVec3 translation, Quaternion rotation);
    /// @brief Извлекает матрицу локального преобразования из предоставленного преобразования.
    /// Автоматически пересчитывает матрицу, если она загрязнена. В противном случае возвращается уже рассчитанное.
    /// @return константную ссылку на матрицу локального преобразования
    constexpr Matrix4D& GetLocation();
    /// @brief Получает мировую матрицу данного преобразования, проверяя его родительский элемент (если он есть) и умножая его на локальную матрицу.
    /// @return мировую матрицу
    Matrix4D GetWorld();
private:
    const Matrix4D& GetLocal();
};