#pragma once

#include "defines.hpp"

template<typename T> class Vector3D;
template<typename T> class Vector4D;

class Matrix4D
{
protected:
	// TODO: SIMD
	f32 n[4][4];

public:
	Matrix4D() = default;
	Matrix4D(f32 n00, f32 n01, f32 n02, f32 n03,
			 f32 n10, f32 n11, f32 n12, f32 n13,
			 f32 n20, f32 n21, f32 n22, f32 n23,
			 f32 n30, f32 n31, f32 n32, f32 n33);
	Matrix4D(const Vector4D<f32>& a, const Vector4D<f32>& b, const Vector4D<f32>& c, const Vector4D<f32>& d);

	f32& operator ()(int i, int j);
	const f32& operator ()(int i, int j) const;
	Vector4D<f32>& operator [](int j);
	const Vector4D<f32>& operator [](int j) const;

	/// @brief Инвертирует текущую матрицу
	/// @return инвертированную матрицу
	Matrix4D& Inverse();

	/// @brief Создает и возвращает единичную матрицу:
	/// @return новая единичная матрица
	static Matrix4D GetIdentity();
	/// @brief Создает и возвращает матрицу ортогональной проекции. Обычно используется для рендеринга плоских или 2D-сцен.
	/// @param left левая сторона усеченного изображения.
	/// @param right правая сторона усеченного изображения.
	/// @param bottom нижняя сторона усеченного вида.
	/// @param top верхняя сторона усеченного вида.
	/// @param NearClip расстояние до ближней плоскости отсечения.
	/// @param FarClip дальнее расстояние от плоскости отсечения.
	/// @return новая матрица ортогональной проекции.
	static Matrix4D MakeOrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip);
	/// @brief Создает и возвращает матрицу перспективной проекции. Обычно используется для рендеринга 3d-сцен.
	/// @param FOV_Radians поле зрения в радианах.
	/// @param AspectRatio соотношение сторон.
	/// @param NearClip расстояние до ближней плоскости отсечения.
	/// @param FarClip расстояние до дальней плоскости отсечения.
	/// @return новая перспективная матрица.
	static Matrix4D MakePerspectiveProjection(f32 FOV_Radians, f32 AspectRatio, f32 NearClip, f32 FarClip);
	/// @brief Создает и возвращает матрицу просмотра или матрицу, рассматривающую цель с точки зрения позиции.
	/// @param position положение матрицы.
	/// @param target позиция, на которую нужно "смотреть".
	/// @param up восходящий вектор.
	/// @return Матрица, рассматривающая цель с точки зрения позиции.
	static Matrix4D MakeLookAt(const Vector3D<f32>& position, const Vector3D<f32>& target, const Vector3D<f32>& up);
	/// @brief Создает и возвращает значение, обратное предоставленной матрице.
	/// @param m матрица, подлежащая инвертированию
	/// @return перевернутая копия предоставленной матрицы.
	static Matrix4D MakeInverse(const Matrix4D& m);
	/// @brief Заменяет строки матрицы на столбцы.
	/// @param m матрица, подлежащая транспонированию
	/// @return копию транспонированной матрицы
	static Matrix4D MakeTransposed(const Matrix4D& m);
	/// @brief 
	/// @param position 
	/// @return 
	static Matrix4D MakeTranslation(const Vector3D<f32>& position);
};

	/// @brief Умножение матриц 4x4
	/// @param a матрица 4x4
	/// @param b матрица 4x4
	/// @return результат умножения матрицы а на матрицу b
	Matrix4D operator*(Matrix4D& a, Matrix4D& b);
