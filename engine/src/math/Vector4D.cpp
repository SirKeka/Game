#include "Vector4D.h"

Vector4D::Vector4D(float a, float b, float c, float d)
{
	x = a;
	y = b;
	z = c;
	w = d;
}

float& Vector4D::operator[](int i)
{
	return ((&x)[i]);
}

const float& Vector4D::operator[](int i) const
{
	return ((&x)[i]);
}

Vector4D& Vector4D::operator*=(float s)
{
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return (*this);
}

Vector4D& Vector4D::operator/=(float s)
{
	s = 1.0f / s;
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return (*this);
}

Vector4D& Vector4D::operator+=(const Vector4D& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return (*this);
}

Vector4D& Vector4D::operator-=(const Vector4D& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
	return (*this);
}

