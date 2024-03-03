#include "quaternion.hpp"

#include "mvector3d.hpp"

Quaternion::Quaternion(f32 a, f32 b, f32 c, f32 s)
{
	x = a; 
	y = b; 
	z = c;
	w = s;
}

Quaternion::Quaternion(const Vector3D<f32>& v, f32 s)
{
	x = v.x; 
	y = v.y; 
	z = v.z;
	w = s;
}

Quaternion::Quaternion(const Vector3D<f32> &axis, f32 angle, bool normalize)
{
	const f32 HalfAngle = 0.5f * angle;
    f32 s = Math::sin(HalfAngle);
    f32 c = Math::cos(HalfAngle);



	x = s * axis.x;
	y = s * axis.y;
	z = s * axis.z;
	w = c;

    if (normalize) this->Normalize();
}

Quaternion::Quaternion(const Quaternion &q)
{
	x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;
}

Quaternion::Quaternion(Quaternion &&q)
{
	x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;

	q.x = 0;
	q.y = 0;
	q.z = 0;
	q.w = 0;
}

Quaternion &Quaternion::operator-()
{
	x *= -1;
	y *= -1;
	z *= -1;
	w *= -1;

    return *this;
}

Quaternion &Quaternion::operator=(const Quaternion &q)
{
    x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;
	return *this;
}

Vector3D<f32> &Quaternion::GetVectorPart(void)
{
	return reinterpret_cast<Vector3D<f32>&>(x);
}

const Vector3D<f32>& Quaternion::GetVectorPart(void) const
{
	return reinterpret_cast<const Vector3D<f32>&>(x);
}

Matrix3D Quaternion::GetRotationMatrix(void)
{
	f32 x2 = x * x;
	f32 y2 = y * y;
	f32 z2 = z * z;
	f32 xy = x * y;
	f32 xz = x * z;
	f32 yz = y * z;
	f32 wx = w * x;
	f32 wy = w * y;
	f32 wz = w * z;

	return Matrix3D(1.0F - 2.0F * (y2 + z2), 2.0F * (xy - wz), 2.0F * (xz + wy),
					2.0F * (xy + wz), 1.0F - 2.0F * (x2 + z2), 2.0F * (yz - wx),
					2.0F * (xz - wy), 2.0F * (yz + wx), 1.0F - 2.0F * (x2 + y2));
}

void Quaternion::SetRotationMatrix(const Matrix3D& m)
{
	f32 m00 = m(0,0);
	f32 m11 = m(1,1);
	f32 m22 = m(2,2);
	f32 sum = m00 + m11 + m22;

	if (sum > 0.0F)
	{
		w = Math::sqrt(sum + 1.0F) * 0.5F;
		f32 f = 0.25F / w;

		x = (m(2,1) - m(1,2)) * f;
		y = (m(0,2) - m(2,0)) * f;
		z = (m(1,0) - m(0,1)) * f;
	}
	else if ((m00 > m11) && (m00 > m22))
	{
		x = Math::sqrt(m00 - m11 - m22 + 1.0F) * 0.5F;
		f32 f = 0.25F / x;

		y = (m(1,0) + m(0,1)) * f;
		z = (m(0,2) + m(2,0)) * f;
		w = (m(2,1) - m(1,2)) * f;
	}
	else if (m11 > m22)
	{
		y = Math::sqrt(m11 - m00 - m22 + 1.0F) * 0.5F;
		f32 f = 0.25F / y;

		x = (m(1,0) + m(0,1)) * f;
		z = (m(2,1) + m(1,2)) * f;
		w = (m(0,2) - m(2,0)) * f;
    }
    else
	{
		z = Math::sqrt(m22 - m00 - m11 + 1.0F) * 0.5F;
		f32 f = 0.25F / z;

		x = (m(0,2) + m(2,0)) * f;
		y = (m(2,1) + m(1,2)) * f;
		w = (m(1,0) - m(0,1)) * f;
	}
}

Quaternion& Quaternion::Normalize()
{
	f32 n = Normal(*this);
    x /= n;
	y /= n; 
	z /= n; 
	w /= n;
	return *this;
}

MINLINE Quaternion Quaternion::Identity()
{
    return Quaternion(0.f, 0.0f, 0.0f, 1.0f);
}

MINLINE Quaternion Conjugate(const Quaternion &q)
{
    return Quaternion(-q.x, -q.y, -q.z, q.w);
}

MINLINE Quaternion Inverse(const Quaternion &q)
{
    return Normalize(Conjugate(q));
}

MINLINE f32 Dot(const Quaternion &q1, const Quaternion &q2)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

Quaternion operator*(const Quaternion& q1, const Quaternion& q2)
{
	return Quaternion(q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
					  q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
					  q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
					  q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z);
}

MINLINE Quaternion SLERP(const Quaternion& q1, const Quaternion& q2, f32 percentage)
{
    // Source: https://en.wikipedia.org/wiki/Slerp
    // Допустимыми вращениями являются только единичные кватернионы.
    // Нормализуйте, чтобы избежать неопределенного поведения.
    Quaternion n1 = Normalize(q1);
    Quaternion n2 = Normalize(q2);

    // Вычислите косинус угла между двумя векторами.
    f32 dot = Dot(q1, q2);

    // Если скалярное произведение отрицательно, cферическая линейная интерполяция 
	// не пойдет по более короткому пути. Обратите внимание, что q1 и -q1 эквивалентны, 
	// когда отрицание применяется ко всем четырем компонентам. Исправьте, перевернув один кватернион.
    if (dot < 0.0f) {
        n1 = -n1;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // Если входные данные слишком близки для комфорта, 
		// линейно интерполируйте и нормализуйте результат.
        return Normalize(Quaternion(n1.x + ((n2.x - n1.x) * percentage),
						  			n1.y + ((n2.y - n1.y) * percentage),
            			  			n1.z + ((n2.z - n1.z) * percentage),
            			  			n1.w + ((n2.w - n1.w) * percentage)));
    }

    // Поскольку точка находится в диапазоне [0, DOT_THRESHOLD], acos безопасен
    f32 theta0 = Math::acos(dot);         // theta0 = угол между входными векторами
    f32 theta = theta0 * percentage;      // theta = угол между n1 и результатом
    f32 SinTheta = Math::sin(theta);     // вычислите это значение только один раз
    f32 SinTheta0 = Math::sin(theta0);   // вычислите это значение только один раз

    f32 s0 = Math::cos(theta) - dot * SinTheta / SinTheta0;  // == sin(theta0 - theta) / sin(theta0)
    f32 s1 = SinTheta / SinTheta0;

    return Quaternion((n1.x * s0) + (n2.x * s1),
        			  (n1.y * s0) + (n2.y * s1),
        			  (n1.z * s0) + (n2.z * s1),
        			  (n1.w * s0) + (n2.w * s1));
}