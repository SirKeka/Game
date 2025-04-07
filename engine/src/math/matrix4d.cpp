#include "matrix4d.h"

#include "math.h"
#include "core/logger.hpp"

constexpr Matrix4D::Matrix4D(f32 n11, f32 n12, f32 n13, f32 n14, f32 n21, f32 n22, f32 n23, f32 n24, f32 n31, f32 n32, f32 n33, f32 n34, f32 n41, f32 n42, f32 n43, f32 n44)
: data{n11, n12, n13, n14, n21, n22, n23, n24, n31, n32, n33, n34, n41, n42, n43, n44}
{}

constexpr Matrix4D::Matrix4D(const FVec4& a, const FVec4& b, const FVec4& c, const FVec4& d)
: data{a.x, a.y, a.z, a.w, b.x, b.y, b.z, b.w, c.x, c.y, c.z, c.w, d.x, d.y, d.z, d.w}
{}

constexpr Matrix4D::Matrix4D(const Quaternion &q) : data()
{
	// https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix
	Quaternion nq = Normalize(q);
	
	n[0][0] = 1.0f - 2.0f * nq.y * nq.y - 2.0f * nq.z * nq.z;
    n[0][1] = 2.0f * nq.x * nq.y - 2.0f * nq.z * nq.w;
    n[0][2] = 2.0f * nq.x * nq.z + 2.0f * nq.y * nq.w;
	n[0][3] = 0.0f;

    n[1][0] = 2.0f * nq.x * nq.y + 2.0f * nq.z * nq.w;
    n[1][1] = 1.0f - 2.0f * nq.x * nq.x - 2.0f * nq.z * nq.z;
    n[1][2] = 2.0f * nq.y * nq.z - 2.0f * nq.x * nq.w;
	n[1][3] = 0.0f;

    n[2][0] = 2.0f * nq.x * nq.z - 2.0f * nq.y * nq.w;
    n[2][1] = 2.0f * nq.y * nq.z + 2.0f * nq.x * nq.w;
    n[2][2] = 1.0f - 2.0f * nq.x * nq.x - 2.0f * nq.y * nq.y;
	n[2][3] = 0.0f;

	n[3][0] = 0.0f; n[3][1] = 0.0f; n[3][2] = 0.0f; n[3][3] = 1.0f;
}

Matrix4D::Matrix4D(const Quaternion &q, const FVec3 &center)
{
	n[0][0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
	n[0][1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
	n[0][2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
	n[0][3] = center.x - center.x * n[0][0] - center.y * n[0][1] - center.z * n[0][2];

	n[1][0] = 2.0f * ((q.x * q.y) - (q.z * q.w));
	n[1][1] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
	n[1][2] = 2.0f * ((q.y * q.z) + (q.x * q.w));
	n[1][3] = center.y - center.x * n[1][0] - center.y * n[1][1] - center.z * n[1][2];

	n[2][0] = 2.0f * ((q.x * q.z) + (q.y * q.w));
	n[2][1] = 2.0f * ((q.y * q.z) - (q.x * q.w));
	n[2][2] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
	n[2][3] = center.z - center.x * n[2][0] - center.y * n[2][1] - center.z * n[2][2];

	n[3][0] = 0.0f; n[3][1] = 0.0f; n[3][2] = 0.0f; n[3][3] = 1.0f;
}

constexpr Matrix4D::Matrix4D(const Matrix4D& m) : data()
{
	for (u64 i = 0; i < 16; i++) {
		data[i] = m.data[i];
	}
}

f32 &Matrix4D::operator()(u8 i, u8 j)
{
	if ((i < 1 || i > 4)) MERROR("Неверный индекс i! Должен быть от 1 до 4");
	if ((j < 1 || i > 4)) MERROR("Неверный индекс j! Должен быть от 1 до 4");
	return n[i - 1][j - 1];
}

const f32& Matrix4D::operator()(u8 i, u8 j) const
{
	if ((i < 1 || i > 4)) MERROR("Неверный индекс i! Должен быть от 1 до 4");
	if ((j < 1 || i > 4)) MERROR("Неверный индекс j! Должен быть от 1 до 4");
	return n[i - 1][j - 1];
}

f32 &Matrix4D::operator()(u8 i)
{
    if ((i < 0 || i > 15)) MERROR("Неверный индекс i! Должен быть от 0 до 15");
	return data[i];
}

const f32 &Matrix4D::operator()(u8 i) const
{
    if ((i < 0 || i > 15)) MERROR("Неверный индекс i! Должен быть от 0 до 15");
	return data[i];
}

FVec4& Matrix4D::operator[](u8 j)
{
	return *reinterpret_cast<FVec4*>(n[j]);
}

const FVec4& Matrix4D::operator[](u8 j) const
{
	return *reinterpret_cast<const FVec4*>(n[j]);
}

Matrix4D &Matrix4D::operator=(const Matrix4D &m)
{
	for (u64 i = 0; i < 16; i++) {
		data[i] = m.data[i];
	}
	
    return *this;
}

MINLINE Matrix4D Matrix4D::MakeLookAt(const FVec3 &position, const FVec3 &target, const FVec3 &up)
{
    FVec3 Z_Axis { target - position };

    Z_Axis.Normalize();
    FVec3 X_Axis = Normalize(Cross(Z_Axis, up));
    FVec3 Y_Axis = Cross(X_Axis, Z_Axis);

    return Matrix4D(		X_Axis.x, 				Y_Axis.x, 			  -Z_Axis.x, 		0, 
							X_Axis.y, 				Y_Axis.y, 			  -Z_Axis.y, 		0, 
							X_Axis.z, 				Y_Axis.z, 			  -Z_Axis.z, 		0, 
					-Dot(X_Axis, position), -Dot(Y_Axis, position), Dot(Z_Axis, position), 1.0f);
}

/*MINLINE Matrix4D Matrix4D::MakeTranslation(const FVec3 &position)
{
    return Matrix4D(FVec4(1.0f, 0.0f, 0.0f, 0.0f),
					FVec4(0.0f, 1.0f, 0.0f, 0.0f),
					FVec4(0.0f, 0.0f, 1.0f, 0.0f),
					FVec4(position,         1.0f));
}*/

void Matrix4D::Inverse()
{
	*this = Matrix4D::MakeInverse(*this);
}

void Matrix4D::Identity()
{
	// data[1] = data[2] = data[3] = data[4] = data[6] = data[7] = data[8] = data[9] = data[11] = data[12] = data[12] = data[13] = data[14] = 0.F;
	*this = Matrix4D();
	data[0] = data[5] = data[10] = data[15] = 1.f;
}

Matrix4D operator*(Matrix4D a, const Matrix4D &b)
{
	return a *= b;
}
