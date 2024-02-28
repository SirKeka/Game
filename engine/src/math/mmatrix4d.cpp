#include "mmatrix4d.hpp"
#include "mvector3d.hpp"
#include "mvector4d.hpp"

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

f32& Matrix4D::operator()(int i, int j)
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

MINLINE Matrix4D &Matrix4D::GetIdentity()
{
    return Matrix4D(1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

MINLINE Matrix4D &Matrix4D::GetOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip)
{
    //mat4 out_matrix = mat4_identity();

    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (NearClip - FarClip);

    out_matrix.data[0] = -2.0f * lr;
    out_matrix.data[5] = -2.0f * bt;
    out_matrix.data[10] = 2.0f * nf;

    out_matrix.data[12] = (left + right) * lr;
    out_matrix.data[13] = (top + bottom) * bt;
    out_matrix.data[14] = (FarClip + NearClip) * nf;
    return out_matrix;
}

/*Matrix4D Matrix4D::Inverse(const Matrix4D& M)
{
	const Vector3D& a = reinterpret_cast<const Vector3D&>(M[0]);
	const Vector3D& b = reinterpret_cast<const Vector3D&>(M[1]);
	const Vector3D& c = reinterpret_cast<const Vector3D&>(M[2]);
	const Vector3D& d = reinterpret_cast<const Vector3D&>(M[3]);

	const f32& x = M(3, 0);
	const f32& y = M(3, 1);
	const f32& z = M(3, 2);
	const f32& w = M(3, 3);

	Vector3D s = Cross(a, b);
	Vector3D t = Cross(c, d);
	Vector3D u = a * y - b * x;
	Vector3D v = c * w - d * z;

	f32 invDet = 1.0F / (Dot(s, v) + Dot(t, u));
	s *= invDet;
	t *= invDet;
	u *= invDet;
	v *= invDet;

	Vector3D r0 = Cross(b, v) + t * y;
	Vector3D r1 = Cross(v, a) - t * x;
	Vector3D r2 = Cross(d, u) + s * w;
	Vector3D r3 = Cross(u, c) - s * z;

	return Matrix4D(r0.x, r0.y, r0.z, -Dot(b, t),
		r1.x, r1.y, r1.z, Dot(a, t),
		r2.x, r2.y, r2.z, -Dot(d, s),
		r3.x, r3.y, r3.z, Dot(c, s));
}*/

Matrix4D &operator*(Matrix4D &a, Matrix4D &b)
{
	Matrix4D c;
    for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			c(i, j) = a(i, 0) * b(0, i) + 
					  a(i, 1) * b(1, i) + 
					  a(i, 2) * b(2, i) + 
					  a(i, 3) * b(3, i);
		}
	}
	return c;
}

// 00  01  02  03	00  01  02  03
// 10  11  12  13	10  11  12  13
// 20  21  22  23	20  21  22  23
// 30  31  32  33	30  31  32  33