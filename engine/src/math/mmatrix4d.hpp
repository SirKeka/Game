#pragma once

#include "defines.hpp"

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

	/// @brief Создает и возвращает единичную матрицу:
	/// @return новая единичная матрица
	static Matrix4D& GetIdentity();
	static Matrix4D& GetOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip);
	Matrix4D Inverse(const Matrix4D& M);
};

	/// @brief Умножение матриц 4x4
	/// @param a матрица 4x4
	/// @param b матрица 4x4
	/// @return результат умножения матрицы а на матрицу b
	Matrix4D& operator*(Matrix4D& a, Matrix4D& b);
