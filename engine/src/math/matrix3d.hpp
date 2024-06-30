#pragma once

#include "defines.hpp"

template<typename T> class Vector3D;

class Matrix3D
{
private:

	f32 n[3][3];

public:

	constexpr Matrix3D() : n{} {}
	constexpr Matrix3D(f32 n00, f32 n01, f32 n02,
		   	 		   f32 n10, f32 n11, f32 n12,
					   f32 n20, f32 n21, f32 n22) : n{ { n00, n01, n02 }, { n10, n11, n12 }, { n20, n21, n22 } } {}
	constexpr Matrix3D(const Matrix3D& m)  : n{ { m(0, 0), m(0, 1), m(0, 2) }, { m(1, 0), m(1, 1), m(1, 2) }, { m(2, 0), m(2, 1), m(2, 2) } } {}

	f32& operator ()(u8 i, u8 j);
	const f32& operator ()(u8 i, u8 j) const;
	Vector3D<f32>& operator [](u8 j);
	const Vector3D<f32>& operator [](u8 j) const;

	f32 Determinant();

	static MINLINE Matrix3D MakeIdentity();
};

Matrix3D operator *(const Matrix3D& A, const Matrix3D& B);
Vector3D<f32> operator *(const  Matrix3D& M, const Vector3D<f32>& v);

Matrix3D Inverse(const Matrix3D& M);
Matrix3D MakeRotationX(f32 t);
Matrix3D MakeRotationY(f32 t);
Matrix3D MakeRotationZ(f32 t);
Matrix3D MakeRotation(f32 t, const Vector3D<f32>& a);
Matrix3D MakeReflection(const Vector3D<f32>& a);
Matrix3D MakeInvolution(const Vector3D<f32>& a);
Matrix3D MakeScale(f32 sx, f32 sy, f32 sz);
Matrix3D MakeScale(f32 s, const Vector3D<f32>& a);
Matrix3D MakeSkew(f32 t, const Vector3D<f32>& a, const Vector3D<f32>& b);
