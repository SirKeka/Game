#pragma once

#include "defines.hpp"

template<typename T> class Vector3D;

class Matrix3D
{
private:

	f32 n[3][3];

public:

	Matrix3D() = default;
	Matrix3D(f32 n00, f32 n01, f32 n02,
		   	 f32 n10, f32 n11, f32 n12,
			 f32 n20, f32 n21, f32 n22);
	Matrix3D(const Matrix3D& m);

	f32& operator ()(u8 i, u8 j);
	const f32& operator ()(u8 i, u8 j) const;
	Vector3D<f32>& operator [](u8 j);
	const Vector3D<f32>& operator [](u8 j) const;

	f32 Determinant();

	static Matrix3D& GetIdentity();
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
