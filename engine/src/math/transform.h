#pragma once
#include "vector3d_fwd.h"
#include "quaternion.h"
#include "matrix4d.h"

/// @brief Представляет преобразование объекта в мире.
/// Преобразования могут иметь родителя, собственное преобразование которого затем учитывается. 
/// ПРИМЕЧАНИЕ: Его свойства следует редактировать не напрямую, а для обеспечения правильной генерации матрицы.
struct MAPI Transform {
    FVec3 position;         // Положение в мире.
    Quaternion rotation;    // Вращение в мире.
    FVec3 scale;            // Масштаб в мире.
    Matrix4D local;         // Матрица локального преобразования, обновляемая при изменении положения, поворота или масштаба.
    Transform* parent;      // Указатель на родительское преобразование, если оно назначено. Также может быть нулевым.
    f32 determinant;
    
    bool IsDirty;           // Указывает, изменилось ли положение, вращение или масштаб, что указывает на необходимость пересчета локальной матрицы.
public:
    /// @brief Создает новое преобразование, используя нулевой вектор для положения, 
    /// идентификационный кватернион для вращения и один вектор для масштаба. 
    /// Также имеет нулевой родительский элемент. По умолчанию отмечено как dirty.
    constexpr Transform(Transform* parent = nullptr) : position(), rotation(0.F, 0.F, 0.F, 1.F), scale(FVec3::One()), local(Matrix4D::MakeIdentity()), parent(parent), determinant(), IsDirty(false) {}

    /// @brief Создает преобразование из заданной позиции.
    /// Использует нулевое вращение и единичный масштаб.
    /// @param position позиция, которую нужно использовать.
    constexpr Transform(FVec3 position, Transform* parent = nullptr): position(position), rotation(0.F, 0.F, 0.F, 1.F), scale(FVec3::One()), local(Matrix4D::MakeIdentity()), parent(parent), determinant(), IsDirty(true) {}

    /// @brief Создает преобразование из заданного поворота.
    /// Использует нулевое положение и единичный масштаб.
    /// @param rotation используемое вращение.
    constexpr Transform(Quaternion rotation, Transform* parent = nullptr) : position(), rotation(rotation), scale(FVec3::One()), local(Matrix4D::MakeIdentity()), parent(parent), determinant(), IsDirty(true) {}
    
    /// @brief Создает преобразование из заданной позиции и поворота.
    /// Использует нулевое положение и единичный масштаб.
    /// @param position позиция, которую нужно использовать.
    /// @param rotation используемое вращение.
    constexpr Transform(FVec3 position, Quaternion rotation, Transform* parent = nullptr) : position(position), rotation(rotation), scale(FVec3::One()), local(Matrix4D::MakeIdentity()), parent(parent), determinant(), IsDirty(true) {}
    
    /// @brief Создает преобразование из заданной позиции и поворота.
    /// Использует нулевое положение.
    /// @param position позиция, которую нужно использовать.
    /// @param rotation используемое вращение.
    /// @param scale масштаб
    constexpr Transform(FVec3 position, Quaternion rotation, FVec3 scale, Transform* parent = nullptr) : position(position), rotation(rotation), scale(scale), local(Matrix4D::MakeIdentity()), parent(parent), determinant(), IsDirty(true) {}
    ~Transform() {}

    /// @brief Возвращает указатель на родительский элемент предоставленного преобразования.
    /// @return Указатель на родительское преобразование.
    MINLINE Transform* GetParent() { return parent; }

    /// @brief Устанавливает родителя предоставленного преобразования.
    /// @param parent Указатель на родительское преобразование.
    void SetParent(Transform* parent = nullptr) { this->parent = parent; }

    /// @brief Возвращает позицию данного преобразования.
    /// @return константную ссылку на позицию
    constexpr FVec3 GetPosition() { return position; }

    /// @brief Устанавливает положение данного преобразования.
    /// @param position позиция, которую необходимо установить.
    void SetPosition(FVec3 position);

    /// @brief Устанавливает положение данного преобразования.
    /// @param x позиция x, которую необходимо установить.
    /// @param y позиция y, которую необходимо установить.
    /// @param z позиция z, которую необходимо установить.
    void SetPosition(f32 x, f32 y, f32 z = 0.F);

    /// @brief Применяет преобразование к данному преобразованию. Это не то же самое, что SetPosition.
    /// @param translation Перевод, который необходимо применить.
    void Translate(FVec3 translation);

    /// @brief Возвращает вращение данного преобразования.
    /// @return константную ссылку на кватернион
    constexpr Quaternion GetRotation() { return rotation; }

    /// @brief Устанавливает вращение данного преобразования.
    void SetRotation(Quaternion rotation);

    /// @brief Применяет вращение к данному преобразованию. Не то же самое, что SetRotation.
    /// @param rotation Применяемое вращение.
    void Rotate(Quaternion rotation);

    /// @brief Возвращает масштаб данного преобразования.
    /// @return константную ссылку на вектор отвечающий за масштаб
    constexpr FVec3 GetScale() { return scale; }

    /// @brief Устанавливает масштаб данного преобразования.
    /// @param scale масштаб
    void SetScale(FVec3 scale);

    /// @brief Применяет масштаб к данному преобразованию. Не то же самое, что SetScale.
    /// @param scale Применяемый масштаб.
    void Scale(FVec3 scale);

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
