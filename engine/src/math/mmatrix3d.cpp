#include "mmatrix3d.hpp"
#include "mvector3d.hpp"

	Matrix3D::Matrix3D(f32 n00, f32 n01, f32 n02,
					   f32 n10, f32 n11, f32 n12,
					   f32 n20, f32 n21, f32 n22)
	{
		n[0][0] = n00; n[0][1] = n01; n[0][2] = n02;
		n[1][0] = n10; n[1][1] = n11; n[1][2] = n12;
		n[2][0] = n20; n[2][1] = n21; n[2][2] = n22;
	}

    Matrix3D::Matrix3D(const Matrix3D &m)
    {
		for (u64 i = 0; i < 3; i++) {
			for (u64 j = 0; j < 3; j++) {
				n[i][j] = m.n[i][j];
			}
		}
	}

    f32& Matrix3D::operator()(u8 i, u8 j)
	{
		return (n[i][j]); //� ����� ���� n[j][i]
	}

	const f32& Matrix3D::operator()(u8 i, u8 j) const
	{
		return (n[i][j]); //� ����� ���� n[j][i]
	}

	Vector3D<f32>& Matrix3D::operator[](u8 j)
	{
		return (*reinterpret_cast<Vector3D<f32>*>(n[j]));
	}

	const Vector3D<f32>& Matrix3D::operator[](u8 j) const
	{
		return (*reinterpret_cast<const Vector3D<f32>*>(n[j]));
	}

    MINLINE f32 Matrix3D::Determinant()
	{
		return (n[0][0] * (n[1][1] * n[2][2] - n[1][2] * n[2][1])
			  + n[0][1] * (n[1][2] * n[2][0] - n[1][0] * n[2][2])
			  + n[0][2] * (n[1][0] * n[2][1] - n[1][1] * n[2][0]));
	}

    MINLINE Matrix3D Matrix3D::MakeIdentity()
    {
        return Matrix3D(1.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 1.0f);
    }

    MINLINE Matrix3D Inverse(const Matrix3D& M)
	{
		const Vector3D<f32>& a = M[0];
		const Vector3D<f32>& b = M[1];
		const Vector3D<f32>& c = M[2];

		Vector3D<f32> r0 = Cross(b, c);
		Vector3D<f32> r1 = Cross(c, a);
		Vector3D<f32> r2 = Cross(a, b);

		f32 invDet = 1.0F / Dot(r2, c);

		return (Matrix3D(r0.x * invDet, r0.y * invDet, r0.z * invDet,
						 r1.x * invDet, r1.y * invDet, r1.z * invDet,
						 r2.x * invDet, r2.y * invDet, r2.z * invDet));
	}

	MINLINE Matrix3D MakeRotationX(f32 t)
	{
		f32 c = Math::cos(t);
		f32 s = Math::sin(t);

		return Matrix3D(1.0F, 0.0F, 0.0F,
						0.0F, c, -s,
						0.0F, s, c);
	}

	MINLINE Matrix3D MakeRotationY(f32 t)
	{
		f32 c = Math::cos(t);
		f32 s = Math::sin(t);
		return Matrix3D(c, 0.0F, s,
						0.0F, 1.0F, 0.0F,
						-s, 0.0F, c);
	}

	MINLINE Matrix3D MakeRotationZ(f32 t)
	{
		f32 c = Math::cos(t);
		f32 s = Math::sin(t);

		return Matrix3D(c, -s, 0.0F,
						s, c, 0.0F,
						0.0F, 0.0F, 1.0F);
	}

	MINLINE Matrix3D operator*(const Matrix3D& A, const Matrix3D& B)
	{
		return (Matrix3D(A(0, 0) * B(0, 0) + A(0, 1) * B(1, 0) + A(0, 2) * B(2, 0),
						 A(0, 0) * B(0, 1) + A(0, 1) * B(1, 1) + A(0, 2) * B(2, 1),
						 A(0, 0) * B(0, 2) + A(0, 1) * B(1, 2) + A(0, 2) * B(2, 2),
						 A(1, 0) * B(0, 0) + A(1, 1) * B(1, 0) + A(1, 2) * B(2, 0),
						 A(1, 0) * B(0, 1) + A(1, 1) * B(1, 1) + A(1, 2) * B(2, 1),
						 A(1, 0) * B(0, 2) + A(1, 1) * B(1, 2) + A(1, 2) * B(2, 2),
						 A(2, 0) * B(0, 0) + A(2, 1) * B(1, 0) + A(2, 2) * B(2, 0),
						 A(2, 0) * B(0, 1) + A(2, 1) * B(1, 1) + A(2, 2) * B(2, 1),
						 A(2, 0) * B(0, 2) + A(2, 1) * B(1, 2) + A(2, 2) * B(2, 2)));
	}

	MINLINE Vector3D<f32> operator *(const  Matrix3D& M, const Vector3D<f32>& v)
	{
		return (Vector3D<f32>(M(0, 0) * v.x + M(0, 1) * M(0, 2) * v.z,
			             M(1, 0) * v.x + M(1, 1) * M(1, 2) * v.z,
			             M(2, 0) * v.x + M(2, 1) * M(2, 2) * v.z));
    }

	MINLINE f32 Determinant(const Matrix3D& M)
	{
		return (M(0, 0) * (M(1, 1) * M(2, 2) - M(1, 2) * M(2, 1))
			  + M(0, 1) * (M(1, 2) * M(2, 0) - M(1, 0) * M(2, 2))
			  + M(0, 2) * (M(1, 0) * M(2, 1) - M(1, 1) * M(2, 0)));
	}

	MINLINE Matrix3D MakeRotation(f32 t, const Vector3D<f32>& a)
	{
		f32 c = Math::cos(t);
		f32 s = Math::sin(t);
		f32 d = 1.0F - c;

		f32 x = a.x * d;
		f32 y = a.y * d;
		f32 z = a.z * d;
		f32 axay = x * a.y;
		f32 axaz = x * a.z;
		f32 ayaz = y * a.z;

		return (Matrix3D(c + x * a.x, axay - s * a.z, axaz + s * a.y,
						 axay + s * a.z, c + y * a.y, ayaz - s * a.x,
						 axaz - s * a.y, ayaz + s * a.x, c + z * a.z));
	}

	MINLINE Matrix3D MakeReflection(const Vector3D<f32>& a)
	{
		f32 x = a.x * -2.0F;
		f32 y = a.y * -2.0F;
		f32 z = a.z * -2.0F;
		f32 axay = x * a.y;
		f32 axaz = x * a.z;
		f32 ayaz = y * a.z;

		return (Matrix3D(x * a.x + 1.0F, axay, axaz,
						 axay, y * a.y + 1.0F, ayaz,
						 axaz, ayaz, z * a.z + 1.0F));
	}

	MINLINE Matrix3D MakeInvolution(const Vector3D<f32>& a)
	{
		f32 x = a.x * 2.0F;
		f32 y = a.y * 2.0F;
		f32 z = a.z * 2.0F;
		f32 axay = x * a.y;
		f32 axaz = x * a.z;
		f32 ayaz = y * a.z;

		return (Matrix3D(x * a.x - 1.0F, axay, axaz,
						 axay, y * a.y - 1.0F, ayaz,
						 axaz, ayaz, z * a.z - 1.0F));
	}

	MINLINE Matrix3D MakeScale(f32 sx, f32 sy, f32 sz)
	{
		return Matrix3D();
	}

	MINLINE Matrix3D MakeScale(f32 s, const Vector3D<f32>& a)
	{
		s -= 1.0F;
		f32 x = a.x * s;
		f32 y = a.y * s;
		f32 z = a.z * s;
		f32 axay = x * a.y;
		f32 axaz = x * a.z;
		f32 ayaz = y * a.z;

		return (Matrix3D(x * a.x + 1.0F, axay, axaz,
						 axay, y * a.y + 1.0F, ayaz,
						 axaz, ayaz, z * a.z + 1.0F));
	}

	MINLINE Matrix3D MakeSkew(f32 t, const Vector3D<f32>& a, const Vector3D<f32>& b)
	{
		t = Math::tan(t);
		f32 x = a.x * t;
		f32 y = a.y * t;
		f32 z = a.z * t;

		return (Matrix3D(x * b.x + 1.0F, x * b.y, x * b.z,
						 y * b.x, y * b.y + 1.0F, y * b.z,
						 z * b.x, z * b.y, z * b.z + 1.0F));
	}
	