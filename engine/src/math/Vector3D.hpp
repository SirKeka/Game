#pragma once

template<typename T>
struct Vector3D
{
	T elements[3];

    // Первый элемент.
    union { T x, r, s, u; };

    // Второй элемент
    union { T y, g, t, v; };

	// Третий элемент
	union { T z, b, p, w; };

	Vector3D() = default;
	Vector3D(T a, T b, T c);

	T& operator [] (int i);
	const T& operator [](int i) const;
	Vector3D& operator +=(const Vector3D& v);
	Vector3D& operator -=(const Vector3D& v);
	Vector3D& operator *=(float s);
	Vector3D& operator /=(float s);
};

template<typename T>
inline Vector3D<T> operator +(const Vector3D<T>& a, const Vector3D<T>& b);

template<typename T>
inline Vector3D<T> operator -(const Vector3D<T>& a, const Vector3D<T>& b);

template<typename T>
inline Vector3D<T> operator *(const Vector3D<T>& v, float s);

template<typename T>
inline Vector3D<T> operator /(const Vector3D<T>& v, float s);

template<typename T>
inline Vector3D<T> operator -(const Vector3D<T>& v);

template<typename T>
inline float Magnitude(const Vector3D<T> v);

template<typename T>
inline Vector3D<T> Normalize(const Vector3D<T>& v);

template<typename T>
inline float Dot(const Vector3D<T>& a, const Vector3D<T>& b);

template<typename T>
inline Vector3D<T> Cross(const Vector3D<T>& a, const Vector3D<T>& b);

template<typename T>
inline float ScalarTripleProduct(const Vector3D<T>& a, const Vector3D<T>& b, const Vector3D<T>& c);

template<typename T>
inline Vector3D<T> Project(const Vector3D<T>& a, const Vector3D<T>& b);

template<typename T>
inline Vector3D<T> Reject(const Vector3D<T>& a, const Vector3D<T>& b);

