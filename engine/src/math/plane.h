#pragma once

#include "vector3d_fwd.h"

struct MAPI Plane
{
	f32 x, y, z, distance;

	constexpr Plane() : x(), y(), z(), distance() {}

	constexpr Plane(float x, float y, float z, float d) : x(x), y(y), z(z), distance(d) {}

	constexpr Plane(const FVec3& n, float d) : x(n.x), y(n.y), z(n.z), distance(d) {}

	constexpr Plane(const FVec3& p1, const FVec3& norm) : x(norm.x), y(norm.y), z(norm.z), distance(Dot(norm, p1)) {}

	/// @brief Создает плоскость.
	/// @param p1 константная ссылка на вектор, который определяет положение точки.
	/// @param norm константная ссылка на нормализованный вектор.
	void Create(const FVec3& p1, const FVec3& norm);

	/// @return нормаль плоскости.
	const FVec3& GetNormal() const {
		return (reinterpret_cast<const FVec3&>(x));
	}

	/// @brief Получает знаковое расстояние между плоскостью p и предоставленной позицией.
	/// @param position постоянная ссыллка на позицию.
	/// @return Знаковое расстояние от точки до плоскости.
	f32 SignedDistance(const FVec3& position);

	/// @brief Указывает, пересекает ли плоскость p сферу, построенную через центр и радиус.
	/// @param center постоянная ссылка на позицию, представляющую центр сферы.
	/// @param radius Радиус сферы.
	/// @return true, если сфера пересекает плоскость; в противном случае false.
	bool IntersectsSphere(const FVec3& center, f32 radius);

	/// @brief Указывает, пересекает ли плоскость p выровненный по оси ограничивающий прямоугольник, созданный с помощью центра и экстентов.
	/// @param center постоянная ссылка на позицию, представляющую центр выровненного по оси ограничивающего прямоугольника.
	/// @param extents Половинные экстенты выровненного по оси ограничивающего прямоугольника.
	/// @return true, если выровненный по оси ограничивающий прямоугольник пересекает плоскость; в противном случае false.
	bool IntersectsAABB(const FVec3& center, const FVec3& extents);
};

MINLINE f32 Dot(const Plane& f, const FVec3& v)
{
	return (f.x * v.x + f.y * v.y + f.z * v.z);
}

/*f32 Dot(const Plane& f, const Point3D& p)
{
	return (f.x * p.x + f.y * p.y + f.z * p.z + f.distance);
}*/
