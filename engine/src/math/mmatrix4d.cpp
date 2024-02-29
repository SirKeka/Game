#include "mmatrix4d.hpp"
#include "mvector3d.hpp"
#include "mvector4d.hpp"

#include "mmath.hpp"

Matrix4D::Matrix4D(f32 n00, f32 n01, f32 n02, f32 n03, 
				   f32 n10, f32 n11, f32 n12, f32 n13,
				   f32 n20, f32 n21, f32 n22, f32 n23,
				   f32 n30, f32 n31, f32 n32, f32 n33)
{
	n[0][0] = n00; n[0][1] = n10; n[0][2] = n20; n[0][3] = n30;
	n[1][0] = n01; n[1][1] = n11; n[1][2] = n21; n[1][3] = n31;
	n[2][0] = n02; n[2][1] = n12; n[2][2] = n22; n[2][3] = n32;
	n[3][0] = n03; n[3][1] = n13; n[3][2] = n23; n[3][3] = n33;
}

Matrix4D::Matrix4D(const Vector4D<f32>& a, const Vector4D<f32>& b, const Vector4D<f32>& c, const Vector4D<f32>& d)
{
	n[0][0] = a.x; n[0][1] = a.y; n[0][2] = a.z; n[0][3] = a.w;
	n[1][0] = b.x; n[1][1] = b.y; n[1][2] = b.z; n[1][3] = b.w;
	n[2][0] = c.x; n[2][1] = c.y; n[2][2] = c.z; n[2][3] = c.w;
	n[3][0] = d.x; n[3][1] = d.y; n[3][2] = d.z; n[3][3] = d.w;
}

Matrix4D::Matrix4D(Quaternion &q)
{
	// https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix
	q.Normalize();
	
	n[0][0] = 1.0f - 2.0f * q.y * q.y - 2.0f * q.z * q.z;
    n[0][1] = 2.0f * q.x * q.y - 2.0f * q.z * q.w;
    n[0][2] = 2.0f * q.x * q.z + 2.0f * q.y * q.w;
	n[0][3] = 0.0f;

    n[1][0] = 2.0f * q.x * q.y + 2.0f * q.z * q.w;
    n[1][1] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z;
    n[1][2] = 2.0f * q.y * q.z - 2.0f * q.x * q.w;
	n[1][3] = 0.0f;

    n[2][0] = 2.0f * q.x * q.z - 2.0f * q.y * q.w;
    n[2][1] = 2.0f * q.y * q.z + 2.0f * q.x * q.w;
    n[2][2] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;
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
	return n[j][i];
}

const f32& Matrix4D::operator()(int i, int j) const
{
	return n[j][i];
}

Vector4D<f32>& Matrix4D::operator[](int j)
{
	return *reinterpret_cast<Vector4D<f32>*>(n[j]);
}

const Vector4D<f32>& Matrix4D::operator[](int j) const
{
	return *reinterpret_cast<const Vector4D<f32>*>(n[j]);
}

MINLINE Matrix4D Matrix4D::MakeIdentity()
{
    return Matrix4D(1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

MINLINE Matrix4D Matrix4D::MakeOrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip)
{
    Matrix4D m {};

    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (NearClip - FarClip);

    m(0, 0) = -2.0f * lr;
    m(1, 1) = -2.0f * bt;
    m(2, 2) =  2.0f * nf;

    m(3, 0) = (left + right) * lr;
    m(3, 1) = (top + bottom) * bt;
    m(3, 2) = (FarClip + NearClip) * nf;
    return m;
}

MINLINE Matrix4D Matrix4D::MakeFrustumProjection(f32 fovy, f32 s, f32 n, f32 f)
{
    f32 g = 1.0F / Math::tan(fovy * 0.5f);
	f32 k = f / (f - n);

	return (Matrix4D(g / s, 0.0f, 0.0f, 0.0f,
	                  0.0f,  g,   0.0f, 0.0f,
	                  0.0f, 0.0f,  k,  -n * k,
	                  0.0f, 0.0f, 1.0f, 0.0f));
}

MINLINE Matrix4D Matrix4D::MakeRevFrustumProjection(f32 fovy, f32 s, f32 n, f32 f)
{
    f32 g = 1.0F / Math::tan(fovy * 0.5F);
	f32 k = n / (n - f);

	return (Matrix4D(g / s, 0.0F, 0.0F, 0.0F,
	                  0.0F,  g,   0.0F, 0.0F,
	                  0.0F, 0.0F,  k,  -f * k,
	                  0.0F, 0.0F, 1.0F, 0.0F));
}

MINLINE Matrix4D Matrix4D::MakeInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e)
{
    f32 g = 1.0f / Math::tan(fovy * 0.5f);
	e = 1.0F - e;

	return (Matrix4D(g / s, 0.0f, 0.0f, 0.0f,
	                  0.0f,  g,   0.0f, 0.0f,
	                  0.0f, 0.0f,  e,  -n * e,
	                  0.0f, 0.0f, 1.0f, 0.0f));
}

MINLINE Matrix4D Matrix4D::MakeRevInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e)
{
    f32 g = 1.0F / Math::tan(fovy * 0.5F);

	return (Matrix4D(g / s, 0.0F, 0.0F,    0.0F,
	                  0.0F,  g,   0.0F,    0.0F,
	                  0.0F, 0.0F,  e,   n * (1.0F - e),
	                  0.0F, 0.0F, 1.0F,    0.0F));
}

MINLINE Matrix4D Matrix4D::MakeLookAt(const Vector3D<f32> &position, const Vector3D<f32> &target, const Vector3D<f32> &up)
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

MINLINE Matrix4D Matrix4D::MakeInverse(const Matrix4D &m)
{
    const Vector3D<f32>& a = reinterpret_cast<const Vector3D<f32>&>(m[0]);
	const Vector3D<f32>& b = reinterpret_cast<const Vector3D<f32>&>(m[1]);
	const Vector3D<f32>& c = reinterpret_cast<const Vector3D<f32>&>(m[2]);
	const Vector3D<f32>& d = reinterpret_cast<const Vector3D<f32>&>(m[3]);

	const f32& x = m(3, 0);
	const f32& y = m(3, 1);
	const f32& z = m(3, 2);
	const f32& w = m(3, 3);

	Vector3D<f32> s = Cross(a, b);
	Vector3D<f32> t = Cross(c, d);
	Vector3D<f32> u = a * y - b * x;
	Vector3D<f32> v = c * w - d * z;

	f32 invDet = 1.0F / (Dot(s, v) + Dot(t, u));
	s *= invDet;
	t *= invDet;
	u *= invDet;
	v *= invDet;

	Vector3D<f32> r0 = Cross(b, v) + t * y;
	Vector3D<f32> r1 = Cross(v, a) - t * x;
	Vector3D<f32> r2 = Cross(d, u) + s * w;
	Vector3D<f32> r3 = Cross(u, c) - s * z;

	return Matrix4D(r0.x, r0.y, r0.z, -Dot(b, t),
					r1.x, r1.y, r1.z,  Dot(a, t),
					r2.x, r2.y, r2.z, -Dot(d, s),
					r3.x, r3.y, r3.z,  Dot(c, s));
}

MINLINE Matrix4D Matrix4D::MakeTransposed(const Matrix4D &m)
{
    return Matrix4D(m(0, 0), m(1, 0), m(2, 0), m(3, 0),
					m(0, 1), m(1, 1), m(2, 1), m(3, 1),
					m(0, 2), m(1, 2), m(2, 2), m(3, 2),
					m(0, 3), m(1, 3), m(2, 3), m(3, 3));
}

MINLINE Matrix4D Matrix4D::MakeTranslation(const Vector3D<f32> &position)
{
    return Matrix4D(Vector4D<f32>(1.0f, 0.0f, 0.0f, 0.0f),
					Vector4D<f32>(0.0f, 1.0f, 0.0f, 0.0f),
					Vector4D<f32>(0.0f, 0.0f, 1.0f, 0.0f),
					Vector4D<f32>(position,         1.0f));
}

MINLINE Matrix4D Matrix4D::MakeScale(const Vector3D<f32> &scale)
{
    return Matrix4D(scale.x,    0,       0,   	  0,
					   0, 	 scale.y,    0,  	  0,
					   0,       0, 	  scale.z,    0,
					   0,       0,	     0,  	 1.0f);
}

MINLINE Matrix4D Matrix4D::MakeEulerX(f32 AngleRadians)
{
	Matrix4D m = MakeIdentity();
	f32 c = Math::cos(AngleRadians);
    f32 s = Math::sin(AngleRadians);

	m(1, 1) = c;
	m(1, 2) = s;
	m(2, 1) = -c;
	m(2, 2) = -s;

    return m;
}

MINLINE Matrix4D Matrix4D::MakeEulerY(f32 AngleRadians)
{
	Matrix4D m = MakeIdentity();
	f32 c = Math::cos(AngleRadians);
    f32 s = Math::sin(AngleRadians);

	m(0, 0) = c;
	m(0, 2) = -s;
	m(2, 0) = s;
	m(2, 2) = c;

    return m;
}

MINLINE Matrix4D Matrix4D::MakeEulerZ(f32 AngleRadians)
{
    Matrix4D m = MakeIdentity();
	f32 c = Math::cos(AngleRadians);
    f32 s = Math::sin(AngleRadians);

	m(0, 0) = c;
	m(0, 1) = s;
	m(1, 0) = -s;
	m(1, 2) = c;

    return m;
}

MINLINE Matrix4D Matrix4D::MakeEulerXYZ(f32 X_Radians, f32 Y_Radians, f32 Z_Radians)
{
    Matrix4D rx = MakeEulerX(X_Radians);
	Matrix4D ry = MakeEulerY(Y_Radians);
	Matrix4D rz = MakeEulerZ(Z_Radians);
	Matrix4D m = rx * ry;
	m = m * rz;

    return m;
}

MINLINE Vector3D<f32> Matrix4D::Forward(const Matrix4D &m)
{
    return -Normalize(Vector3D<f32>(m(0, 2), m(1, 2), m(2, 2)));
}

MINLINE Vector3D<f32> Matrix4D::Backward(const Matrix4D &m)
{
    return Normalize(Vector3D<f32>(m(0, 2), m(1, 2), m(2, 2)));
}

MINLINE Vector3D<f32> Matrix4D::Up(const Matrix4D& m)
{
	return Normalize(Vector3D<f32>(m(0, 1), m(1, 1), m(2, 2)));
}

MINLINE Vector3D<f32> Matrix4D::Left(const Matrix4D &m)
{
    return -Normalize(Vector3D<f32>(m(0, 0), m(1, 0), m(2, 0)));
}

MINLINE Vector3D<f32> Down(const Matrix4D& m)
{
	return -Normalize(Vector3D<f32>(m(0, 1), m(1, 1), m(2, 2)));
}

MINLINE Vector3D<f32> Right(const Matrix4D& m)
{
	return Normalize(Vector3D<f32>(m(0, 0), m(1, 0), m(2, 0)));
}

//  00 01 02 03
//  04 05 06 07
//  08 09 10 11
//  12 13 14 15

/*MINLINE Matrix4D &Matrix4D::Inverse()
{
	return MakeInverse(*this);
}*/

Matrix4D operator*(Matrix4D &a, Matrix4D &b)
{
	Matrix4D c;
    for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			c(i, j) = a(i, 0) * b(0, j) + 
					  a(i, 1) * b(1, j) + 
					  a(i, 2) * b(2, j) + 
					  a(i, 3) * b(3, j);
		}
	}
	return c;
}
