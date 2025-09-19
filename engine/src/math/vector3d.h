#pragma once

#include "math.h"
#include "vector2d.h"

template<typename T> struct Vector4D;
struct Matrix4D;

template<typename T>
struct Vector3D
{
public:
	union {
        // Массив x, y, z
        T elements[3];
        struct {
            // Первый элемент.
			union {T x, r;};
            // Второй элемент.
			union {T y, g;};
			// Третий элемент.
			union {T z, b;};
        };
    };

	constexpr explicit Vector3D() 					  noexcept : x(),      y(),      z()      {}
	/// @brief Заполняет все кординаты вектора одним значением
	/// @param value значение, которое заполнит все координаты вектора
	constexpr Vector3D(T value) 			  		  noexcept : x(value), y(value), z(value) {}
	constexpr Vector3D(T x, T y, T z = 0) 	  noexcept : x(x),     y(y),     z(z)     {}
	constexpr explicit Vector3D(const Vector2D<T>& v) noexcept : x(v.x),   y(v.y),   z()      {}
	constexpr Vector3D(const Vector4D<T>& v) noexcept : x(v.x),   y(v.y),   z(v.z)   {}

	/// @brief нулевой вектор
    /// @return (0, 0, 0)
    /*static constexpr Vector3D& Zero() noexcept {
		return Vector3D();
	}*/
    /// @brief единичный вектор
    /// @return (1, 1, 1)
    MINLINE static constexpr Vector3D One() noexcept {
		return Vector3D(1, 1, 1);
	}
    /// @brief Вектор указывающий вверх
    /// @return (0, 1, 0)
    MINLINE static constexpr Vector3D Up() noexcept {
		return Vector3D(0, 1, 0);
	}
    /// @brief Вектор указывающий вниз
    /// @return (0, -1, 0)
    MINLINE static constexpr Vector3D Down() noexcept {
    	return Vector3D(0, -1, 0);
	}
    /// @brief Вектор указывающий налево
    /// @return (-1, 0, 0)
    MINLINE static constexpr Vector3D Left() noexcept {
    	return Vector3D(-1, 0, 0);
	}
    /// @brief Вектор указывающий направо
    /// @return (1, 0, 0)
    MINLINE static constexpr Vector3D Right() noexcept {
    	return Vector3D(1, 0, 0);
	}
	/// @brief Вектор указывающий вперед
	/// @return (0, 0, -1)
	MINLINE static constexpr Vector3D Forward() noexcept {
    	return Vector3D(0, 0, -1);
	}
	/// @brief Вектор указывающий назад
	/// @return (0, 0, 1)
	MINLINE static constexpr Vector3D Backward() noexcept {
    	return Vector3D(0, 0, 1);
	}

	T& operator [] (int i);
	const T& operator [](int i) const;
	MINLINE Vector3D& operator +=(const Vector3D& v) {
		x += v.x; y += v.y; z += v.z; return *this;
	}
	MINLINE Vector3D& operator +=(const T s) {
		x += s;
		y += s;
		z += s;
		return *this;
	}
	MINLINE Vector3D& operator -=(const Vector3D& v) {
		x -= v.x;
		y -= v.y;
		z -= z.y;
		return *this;
	}
	MINLINE Vector3D& operator -=(const T s) {
		x -= s;
		y -= s;
		z -= s;
		return *this;
	}
	MINLINE Vector3D& operator *=(const Vector3D& v) {
		x *= v.x;
		y *= v.y;
		z *= v.y;
		return *this;
	}
	MINLINE Vector3D& operator *=(const T s) {
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}
	MINLINE Vector3D& operator /=(const Vector3D& v) {
		x /= v.x;
		y /= v.y;
		z /= z.y;
		return *this;
	}
	MINLINE Vector3D& operator /=(const T s) {
		x /= s;
		y /= s;
		z /= s;
		return *this;
	}
	MINLINE Vector3D& operator-() {
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}
	//explicit operator bool() const;
	bool operator==(const Vector3D& v) const {
		if (Math::abs(x - v.x) > M_FLOAT_EPSILON) {
			return false;
		}
        if (Math::abs(y - v.y) > M_FLOAT_EPSILON) {
            return false;
        }
		if (Math::abs(z - v.z) > M_FLOAT_EPSILON) {
			return false;
		}
		return true;
	}

	void Set(f32 x, f32 y, f32 z) {
		this->x = x; this->y = y; this->z = z;
	}

	Vector3D& Normalize() {
    	return *this /= VectorLenght<T>(*this);
	}

	/// @brief Преобразует vec3 значений rgb [0.0-1.0] в целые значения rgb [0-255].
	/// @param v Вектор значений rgb [0.0-1.0], который нужно преобразовать.
	/// @param OutR Указатель для хранения значения красного.
	/// @param OutG Указатель для хранения значения зеленого.
	/// @param OutB Указатель для хранения значения синего.
	MINLINE void ToRGBu32(u32& OutR, u32& OutG, u32& OutB) {
	    OutR = r * 255;
	    OutG = g * 255;
	    OutB = b * 255;
	}

};

template<typename T>
MINLINE Vector3D<T> operator +(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template<typename T>
MINLINE Vector3D<T> operator -(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<typename T>
MINLINE Vector3D<T> operator *(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x * b.x, a.y * b.y, a.z * b.z);
}

/// @brief Умножение на скаляр
/// @tparam T тип компонентов вектора и скаляра
/// @param v вектор
/// @param s скаляр
/// @return Копию вектора умноженного на скаляр
template<typename T>
MINLINE Vector3D<T> operator *(const Vector3D<T>& v, T s)
{
	return Vector3D<T>(v.x * s, v.y * s, v.z * s);
}

// template<typename T>
// MINLINE Vector3D<T> operator *(const Vector3D<T>& v, const Matrix4D& m)
// {
// 	return Vector3D<T>(
//         v.x * m.data[0] + v.y * m.data[4] + v.z * m.data[8]  + m.data[12],
//         v.x * m.data[1] + v.y * m.data[5] + v.z * m.data[9]  + m.data[13],
//         v.x * m.data[2] + v.y * m.data[6] + v.z * m.data[10] + m.data[14]);
// }

template<typename T>
MINLINE Vector3D<T> operator /(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x / b.x, a.y / b.y, a.z / b.z);
}

/// @brief Деление на скаляр
/// @tparam T тип компонентов вектора
/// @param v вектор
/// @param s скаляр
/// @return Копию вектора раделенного на скаляр
template<typename T>
MINLINE Vector3D<T> operator /(const Vector3D<T>& v, T s)
{
	return Vector3D<T>(v.x / s, v.y / s, v.z / s);
}

template<typename T>
MINLINE Vector3D<T> operator -(const Vector3D<T>& v)
{
	return Vector3D<T>(-v.x, -v.y, -v.z);
}

/// @brief Возвращает квадрат длины предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемого квадрата длины
/// @param v вектор
/// @return длина в квадрате.
template<typename T>
MINLINE T VectorLenghtSquared(const Vector3D<T>& v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

/// @brief Возвращает длину предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемой длины
/// @param v вектор
/// @return длина вектора
template<typename T>
MINLINE T VectorLenght(const Vector3D<T>& v)
{
	return Math::sqrt(VectorLenghtSquared(v));
}

template<typename T>
MINLINE Vector3D<T> Normalize(const Vector3D<T>& v)
{
	return v / VectorLenght(v);
}

/// @brief Скалярное произведение предоставленных векторов. Обычно используется для расчета разницы направлений.
/// @tparam T тип коммпонентов векторов и возвращаемого значения
/// @param a первый вектор
/// @param b второй вектор
/// @return результат скалярного произведения векторов
template<typename T>
MINLINE T Dot(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return a.x * b.x + a.y *b.y + a.z * b.z;
}

/// @brief Вычисляет и возвращает перекрестное произведение предоставленных векторов. Перекрестное произведение - это новый вектор, который ортогонален обоим предоставленным векторам.
/// @tparam T ти компонентов векторов
/// @param a первый вектор
/// @param b второй вектор
/// @return вектор который ортогонален вектору а и b
template<typename T>
MINLINE Vector3D<T> Cross(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.y * b.z - a.z * b.y,
	 				   a.z * b.x - a.x * b.z,
					   a.x * b.y - a.y * b.x);
}

template<typename T>
MINLINE T Distance(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return VectorLenght(a - b);
}

/// @brief Возвращает квадрат расстояния между вектором а и вектором b. Менее трудоёмкий вызов, чем неквадратная версия из-за использования квадратного корня.
/// @tparam T тип коммпонентов векторов и возвращаемого значения
/// @param a вектор
/// @param b вектор
/// @return Квадрат расстояния между вектороми а и b.
template<typename T>
MINLINE T DistanceSquared(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return VectorLenghtSquared(a - b);
}

template<typename T>
MINLINE T ScalarTripleProduct(const Vector3D<T>& a, const Vector3D<T>& b, const Vector3D<T>& c)
{
	return Dot(Cross(a, b), c);
}

template<typename T>
MINLINE Vector3D<T>& Project(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(b * (Dot(a, b) / Dot(b, b)));
}

template<typename T>
MINLINE Vector3D<T>& Reject(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a - b * (Dot(a, b) / Dot(b, b)));
}

template<typename T>
/// @brief Преобразует целочисленные значения RGB [0-255] в vec3 значений с плавающей точкой [0.0-1.0]
/// @param r Красное значение [0-255].
/// @param g Зеленое значение [0-255].
/// @param b Синее значение [0-255].
/// @param OutV Указатель для хранения вектора значений с плавающей точкой.
/// @return 
MINLINE void RGBu32ToFVec3(u32 r, u32 g, u32 b, Vector3D<T>& v) 
{
	v /= 255.F;
}

template<typename T>
MINLINE Vector3D<T> Min(const Vector3D<T>& vl, const Vector3D<T>& vr) 
{
	return Vector3D<T>(
		MMIN(vl.x, vr.y),
		MMIN(vl.y, vr.y),
		MMIN(vl.z, vr.z));
}

template<typename T>
MINLINE Vector3D<T> Max(const Vector3D<T>& vl, const Vector3D<T>& vr) 
{
	return Vector3D<T>(
		MMAX(vl.x, vr.y),
		MMAX(vl.y, vr.y),
		MMAX(vl.z, vr.z));
}