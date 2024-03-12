#pragma once

#include "defines.hpp"
#include "vector3d.hpp"
#include "quaternion.hpp"

template<typename T> class Vector4D;

class Matrix4D
{
public:
	// TODO: SIMD
	union{
		f32 data[16];
		f32 n[4][4];
	};

public:
	Matrix4D() = default;
	Matrix4D(f32 n11, f32 n12, f32 n13, f32 n14,
			 f32 n21, f32 n22, f32 n23, f32 n24,
			 f32 n31, f32 n32, f32 n33, f32 n34,
			 f32 n41, f32 n42, f32 n43, f32 n44);
	Matrix4D(const Vector4D<f32>& a, const Vector4D<f32>& b, const Vector4D<f32>& c, const Vector4D<f32>& d);
	Matrix4D (const Quaternion& q);
	/// @brief Вычисляет матрицу поворота на основе кватерниона и пройденной центральной точки.
	/// @param q кватернион
	/// @param v вектор
	Matrix4D (const Quaternion &q, const Vector3D<f32> &center);

	f32& operator ()(int i, int j);
	const f32& operator ()(int i, int j) const;
	f32& operator ()(int i);
	const f32& operator ()(int i) const;
	Vector4D<f32>& operator [](int j);
	const Vector4D<f32>& operator [](int j) const;

	/// @brief Инвертирует текущую матрицу
	/// @return инвертированную матрицу
	void Inverse();
};

	/// @brief Умножение матриц 4x4
	/// @param a матрица 4x4
	/// @param b матрица 4x4
	/// @return результат умножения матрицы а на матрицу b
	Matrix4D operator*(Matrix4D& a, Matrix4D& b);

	namespace Matrix4
	{
		/// @brief Создает и возвращает единичную матрицу:
	/// @return новая единичная матрица
	MINLINE Matrix4D MakeIdentity()
	{
		return Matrix4D(1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f);
	}
	/// @brief Создает и возвращает матрицу ортогональной проекции. Обычно используется для рендеринга плоских или 2D-сцен.
	/// @param left левая сторона усеченного изображения.
	/// @param right правая сторона усеченного изображения.
	/// @param bottom нижняя сторона усеченного вида.
	/// @param top верхняя сторона усеченного вида.
	/// @param NearClip расстояние до ближней плоскости отсечения.
	/// @param FarClip дальнее расстояние от плоскости отсечения.
	/// @return новая матрица ортогональной проекции.
	MINLINE Matrix4D MakeOrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip)
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
	/// @brief Создает матрицу 4х4, которая представляет перспективную проекцию для усеченной видимости
	/// @param fovy вертикальное поле зрения в радианах.
	/// @param s соотношение сторон области просмотра.
	/// @param n расстояние до ближней плоскости отсечения.
	/// @param f расстояние до дальней плоскости отсечения.
	/// @return класс Matrix4D
	MINLINE Matrix4D MakeFrustumProjection(f32 fovy, f32 s, f32 n, f32 f)
	{
		f32 g = 1.0F / Math::tan(fovy * 0.5F);
		f32 k = f / (f - n);

		return (Matrix4D(g / s, 0.f,   0.f,  0.f,		// Matrix4D(g / s, 0.f, 0.f,  0.f,
		                  0.f,   g,    0.f,  0.f,		// 			 0.f,   g,  0.f,  0.f,
		                  0.f,  0.f,   -k,  -1.f,		// 	         0.f,  0.f,  k, -n * k,
		                  0.f,  0.f, -n * k, 0.f));		// 	         0.f,  0.f, 1.f,  0.f));
	}
	/// @brief Cоздает матрицу 4×4, которая представляет обратную перспективную проекцию для усеченного вида
	/// @param fovy вертикальное поле зрения в радианах.
	/// @param s соотношение сторон области просмотра.
	/// @param n расстояние до ближней плоскости отсечения.
	/// @param f расстояние до дальней плоскости отсечения.
	/// @return структура данных Matrix4D
	MINLINE Matrix4D MakeRevFrustumProjection(f32 fovy, f32 s, f32 n, f32 f)
	{
		f32 g = 1.0F / Math::tan(fovy * 0.5F);
		f32 k = n / (n - f);

		return (Matrix4D(g / s, 0.0F,  0.0F, 0.0F,
		                  0.0F,  g,    0.0F, 0.0F,
		                  0.0F, 0.0F,   k,  -f * k,
		                  0.0F, 0.0F, -1.0F, 0.0F));
	}
	/**Часто непрактично или, по крайней мере, неудобно, чтобы усеченный вид имел дальнюю плоскость, 
	 * за которой невозможно визуализировать никакие объекты. Некоторым игровым движкам 
	 * требуется нарисовать очень большой мир, в котором камера может видеть на много километров 
	 * в любом направлении, и такие объекты, как небо, солнце, луна и звездные поля, 
	 * должны быть отрисованы намного дальше, чтобы гарантировать, что они не пересекают какую-либо геометрию мира.
	 * К счастью, дальнюю плоскость можно отодвинуть буквально на бесконечное расстояние от камеры, 
	 * и это стало обычной практикой. Игровые движки по-прежнему могут выбирать объекты, находящиеся за пределами максимального 
	 * расстояния просмотра, но матрица проекции больше не будет вызывать обрезку треугольников по дальней плоскости.*/
	/// @brief Cоздает матрицу 4×4, которая представляет бесконечную перспективную проекцию для усеченного вида
	/// @param fovy вертикальное поле зрения в радианах.
	/// @param s cоотношение сторон окна просмотра
	/// @param n расстояние до ближней плоскости
	/// @param e 
	/// @return структура данных Matrix4D
	MINLINE Matrix4D MakeInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e)
	{
		f32 g = 1.0f / Math::tan(fovy * 0.5f);
		e = 1.0F - e;

		return (Matrix4D(g / s, 0.0f, 0.0f, 0.0f,
		                  0.0f,  g,   0.0f, 0.0f,
		                  0.0f, 0.0f,  e,  -n * e,
		                  0.0f, 0.0f, 1.0f, 0.0f));
	}
	/**
	 * Чтобы воспользоваться преимуществами обратной проекции, игровой движок должен выполнить 
	 * еще одну настройку, а именно изменить направление теста глубины на противоположное, 
	 * поскольку объекты, расположенные ближе к камере, теперь записывают в буфер глубины большие значения.
	*/
	/// @brief 
	/// @param fovy 
	/// @param s 
	/// @param n 
	/// @param f 
	/// @return 
	MINLINE Matrix4D MakeRevInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e)
	{
		f32 g = 1.0F / Math::tan(fovy * 0.5F);

		return (Matrix4D(g / s, 0.0F, 0.0F,    0.0F,
		                  0.0F,  g,   0.0F,    0.0F,
		                  0.0F, 0.0F,  e,   n * (1.0F - e),
		                  0.0F, 0.0F, 1.0F,    0.0F));
	}
	/// @brief Создает и возвращает матрицу просмотра или матрицу, рассматривающую цель с точки зрения позиции.
	/// @param position положение матрицы.
	/// @param target позиция, на которую нужно "смотреть".
	/// @param up восходящий вектор.
	/// @return Матрица, рассматривающая цель с точки зрения позиции.
	MINLINE Matrix4D MakeLookAt(const Vector3D<f32>& position, const Vector3D<f32>& target, const Vector3D<f32>& up);
	/// @brief Создает и возвращает значение, обратное предоставленной матрице.
	/// @param m матрица, подлежащая инвертированию
	/// @return перевернутая копия предоставленной матрицы.
	MINLINE Matrix4D MakeInverse(const Matrix4D& m)
	{
		const Vector3D<f32>& a = reinterpret_cast<const Vector3D<f32>&>(m[0]);
		const Vector3D<f32>& b = reinterpret_cast<const Vector3D<f32>&>(m[1]);
		const Vector3D<f32>& c = reinterpret_cast<const Vector3D<f32>&>(m[2]);
		const Vector3D<f32>& d = reinterpret_cast<const Vector3D<f32>&>(m[3]);

		const f32& x = m(1, 4);
		const f32& y = m(2, 4);
		const f32& z = m(3, 4);
		const f32& w = m(4, 4);

		Vector3D<f32> s = Cross(a, b);
		Vector3D<f32> t = Cross(c, d);
		Vector3D<f32> u = a * y - b * x;
		Vector3D<f32> v = c * w - d * z;

		float invDet = 1.0F / (Dot(s, v) + Dot(t, u));
		s *= invDet;
		t *= invDet;
		u *= invDet;
		v *= invDet;

		Vector3D<f32> r0 = Cross(b, v) + t * y;
		Vector3D<f32> r1 = Cross(v, a) - t * x;
		Vector3D<f32> r2 = Cross(d, u) + s * w;
		Vector3D<f32> r3 = Cross(u, c) - s * z;

		return Matrix4D(	r0.x, 	   r1.x, 	   r2.x,      r3.x,
							r0.y, 	   r1.y, 	   r2.y,      r3.y,
							r0.z, 	   r1.z, 	   r2.z,      r3.z,
						-Dot(b, t), Dot(a, t), -Dot(d, s), Dot(c, s));

		/*return (Matrix4D(r0.x, r0.y, r0.z, -Dot(b, t),
		                 r1.x, r1.y, r1.z,  Dot(a, t),
		                 r2.x, r2.y, r2.z, -Dot(d, s),
		                 r3.x, r3.y, r3.z,  Dot(c, s)));*/
	}
	/// @brief Заменяет строки матрицы на столбцы.
	/// @param m матрица, подлежащая транспонированию
	/// @return копию транспонированной матрицы
	MINLINE Matrix4D MakeTransposed(const Matrix4D& m);
	/// @brief 
	/// @param position 
	/// @return 
	MINLINE Matrix4D MakeTranslation(const Vector3D<f32>& position)
	{
		/*Matrix4D m = Matrix4::MakeIdentity();
		m(3, 2) = position.z;
		return m;*/
		return Matrix4D(	1.0f, 		0.0f, 	   0.0f,    0.0f,
							0.0f, 		1.0f, 	   0.0f,    0.0f,
							0.0f, 		0.0f, 	   1.0f,    0.0f,
	   					position.x, position.y, position.z, 1.0f);
	}
	/// @brief Возвращает матрицу масштаба, используя предоставленный масштаб.
	/// @param scale 
	/// @return 
	MINLINE Matrix4D MakeScale(const Vector3D<f32>& scale);
	MINLINE Matrix4D MakeEulerX(f32 AngleRadians);
	MINLINE Matrix4D MakeEulerY(f32 AngleRadians);
	MINLINE Matrix4D MakeEulerZ(f32 AngleRadians);
	MINLINE Matrix4D MakeEulerXYZ(f32 X_Radians, f32 Y_Radians, f32 Z_Radians);
	MINLINE Matrix4D MakeEulerXYZ(Vector3D<f32> v);
	/// @brief Возвращает вектор направленный вперед относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE Vector3D<f32> Forward(const Matrix4D& m);
	/// @brief Возвращает вектор направленный назад относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE Vector3D<f32> Backward(const Matrix4D& m);
	/// @brief Возвращает вектор направленный вверх относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE Vector3D<f32> Up(const Matrix4D& m);
	/// @brief Возвращает вектор направленный вниз относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE Vector3D<f32> Down(const Matrix4D& m);
	/// @brief Возвращает вектор направленный влево относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE Vector3D<f32> Left(const Matrix4D& m);
	/// @brief Возвращает вектор направленный вправо относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE Vector3D<f32> Right(const Matrix4D& m);
	} // namespace Matrix4D
	