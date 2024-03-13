#include "matrix4d.hpp"
#include "vector4d.hpp"

#include "math.hpp"
#include "core/logger.hpp"

Matrix4D::Matrix4D(f32 n11, f32 n12, f32 n13, f32 n14,
				   f32 n21, f32 n22, f32 n23, f32 n24,
				   f32 n31, f32 n32, f32 n33, f32 n34,
				   f32 n41, f32 n42, f32 n43, f32 n44)
{
	n[0][0] = n11; n[0][1] = n12; n[0][2] = n13; n[0][3] = n14;
	n[1][0] = n21; n[1][1] = n22; n[1][2] = n23; n[1][3] = n24;
	n[2][0] = n31; n[2][1] = n32; n[2][2] = n33; n[2][3] = n34;
	n[3][0] = n41; n[3][1] = n42; n[3][2] = n43; n[3][3] = n44;
}

Matrix4D::Matrix4D(const Vector4D<f32>& a, const Vector4D<f32>& b, const Vector4D<f32>& c, const Vector4D<f32>& d)
{
	n[0][0] = a.x; n[0][1] = a.y; n[0][2] = a.z; n[0][3] = a.w;
	n[1][0] = b.x; n[1][1] = b.y; n[1][2] = b.z; n[1][3] = b.w;
	n[2][0] = c.x; n[2][1] = c.y; n[2][2] = c.z; n[2][3] = c.w;
	n[3][0] = d.x; n[3][1] = d.y; n[3][2] = d.z; n[3][3] = d.w;
}

Matrix4D::Matrix4D(const Quaternion &q)
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

Matrix4D::Matrix4D(const Quaternion &q, const Vector3D<f32> &center)
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

f32 &Matrix4D::operator()(int i, int j)
{
	if((i < 1 || i > 4)) MERROR("Неверный индекс i! Должен быть от 1 до 4");
	if ((j < 1 || i > 4)) MERROR("Неверный индекс j! Должен быть от 1 до 4");
	return n[i - 1][j - 1];
}

const f32& Matrix4D::operator()(int i, int j) const
{
	if((i < 1 || i > 4)) MERROR("Неверный индекс i! Должен быть от 1 до 4");
	if ((j < 1 || i > 4)) MERROR("Неверный индекс j! Должен быть от 1 до 4");
	return n[i - 1][j - 1];
}

f32 &Matrix4D::operator()(int i)
{
    if((i < 0 || i > 15)) MERROR("Неверный индекс i! Должен быть от 0 до 15");
	return data[i];
}

const f32 &Matrix4D::operator()(int i) const
{
    if((i < 0 || i > 15)) MERROR("Неверный индекс i! Должен быть от 0 до 15");
	return data[i];
}

Vector4D<f32>& Matrix4D::operator[](int j)
{
	return *reinterpret_cast<Vector4D<f32>*>(n[j]);
}

const Vector4D<f32>& Matrix4D::operator[](int j) const
{
	return *reinterpret_cast<const Vector4D<f32>*>(n[j]);
}

/*MINLINE Matrix4D Matrix4D::MakeIdentity()
{
    return Matrix4D(1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}*/

/*MINLINE Matrix4D Matrix4D::MakeFrustumProjection(f32 fovy, f32 s, f32 n, f32 f)
{
    
}*/

/*MINLINE Matrix4D Matrix4D::MakeRevFrustumProjection(f32 fovy, f32 s, f32 n, f32 f)
{
    f32 g = 1.0F / Math::tan(fovy * 0.5F);
	f32 k = n / (n - f);

	return (Matrix4D(g / s, 0.0F, 0.0F, 0.0F,
	                  0.0F,  g,   0.0F, 0.0F,
	                  0.0F, 0.0F,  k,  -f * k,
	                  0.0F, 0.0F, 1.0F, 0.0F));
}*/

/*MINLINE Matrix4D Matrix4D::MakeInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e)
{
    f32 g = 1.0f / Math::tan(fovy * 0.5f);
	e = 1.0F - e;

	return (Matrix4D(g / s, 0.0f, 0.0f, 0.0f,
	                  0.0f,  g,   0.0f, 0.0f,
	                  0.0f, 0.0f,  e,  -n * e,
	                  0.0f, 0.0f, 1.0f, 0.0f));
}*/

/*MINLINE Matrix4D Matrix4D::MakeRevInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e)
{
    f32 g = 1.0F / Math::tan(fovy * 0.5F);

	return (Matrix4D(g / s, 0.0F, 0.0F,    0.0F,
	                  0.0F,  g,   0.0F,    0.0F,
	                  0.0F, 0.0F,  e,   n * (1.0F - e),
	                  0.0F, 0.0F, 1.0F,    0.0F));
}*/

MINLINE Matrix4D Matrix4::MakeLookAt(const Vector3D<f32> &position, const Vector3D<f32> &target, const Vector3D<f32> &up)
{
    Vector3D<f32> Z_Axis;
	Z_Axis = target - position;

    Z_Axis.Normalize();
    Vector3D<f32> X_Axis = Normalize(Cross(Z_Axis, up));
    Vector3D<f32> Y_Axis = Cross(X_Axis, Z_Axis);

    return Matrix4D(		X_Axis.x, 				Y_Axis.x, 			  -Z_Axis.x, 		0, 
							X_Axis.y, 				Y_Axis.y, 			  -Z_Axis.y, 		0, 
							X_Axis.z, 				Y_Axis.z, 			  -Z_Axis.z, 		0, 
					-Dot(X_Axis, position), -Dot(Y_Axis, position), Dot(Z_Axis, position), 1.0f);
}

MINLINE Matrix4D Matrix4::MakeTransposed(const Matrix4D &m)
{
    return Matrix4D(m(0, 0), m(1, 0), m(2, 0), m(3, 0),
					m(0, 1), m(1, 1), m(2, 1), m(3, 1),
					m(0, 2), m(1, 2), m(2, 2), m(3, 2),
					m(0, 3), m(1, 3), m(2, 3), m(3, 3));
}

/*MINLINE Matrix4D Matrix4D::MakeTranslation(const Vector3D<f32> &position)
{
    return Matrix4D(Vector4D<f32>(1.0f, 0.0f, 0.0f, 0.0f),
					Vector4D<f32>(0.0f, 1.0f, 0.0f, 0.0f),
					Vector4D<f32>(0.0f, 0.0f, 1.0f, 0.0f),
					Vector4D<f32>(position,         1.0f));
}*/

MINLINE Matrix4D Matrix4::MakeScale(const Vector3D<f32> &scale)
{
    return Matrix4D(scale.x,    0,       0,   	  0,
					   0, 	 scale.y,    0,  	  0,
					   0,       0, 	  scale.z,    0,
					   0,       0,	     0,  	 1.0f);
}

MINLINE Vector3D<f32> Matrix4::Up(const Matrix4D& m)
{
	return Normalize(Vector3D<f32>(m(0, 1), m(1, 1), m(2, 2)));
}

MINLINE Vector3D<f32> Matrix4::Down(const Matrix4D& m)
{
	return -Normalize(Vector3D<f32>(m(0, 1), m(1, 1), m(2, 2)));
}

void Matrix4D::Inverse()
{
	*this = Matrix4::MakeInverse(*this);
}

Matrix4D operator*(Matrix4D &a, Matrix4D &b)
{
	Matrix4D c {};
    for (int i = 1; i < 5; i++) {
		for (int j = 1; j < 5; j++) {
			c(i, j) = a(i, 1) * b(1, j) + 
					  a(i, 2) * b(2, j) + 
					  a(i, 3) * b(3, j) + 
					  a(i, 4) * b(4, j);
		}
	}
	return c;
}
