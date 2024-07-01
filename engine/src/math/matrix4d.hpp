#pragma once

#include "defines.hpp"
#include "vector3d_fwd.hpp"
#include "vector4d_fwd.hpp"
#include "quaternion.hpp"

template<typename T> class Vector4D;

class MAPI Matrix4D
{
public:
	// TODO: SIMD
	union{
		f32 data[16];
		f32 n[4][4];
	};

public:
	constexpr Matrix4D() : data() {}
	constexpr Matrix4D(f32 n11, f32 n12, f32 n13, f32 n14,
			 		   f32 n21, f32 n22, f32 n23, f32 n24,
					   f32 n31, f32 n32, f32 n33, f32 n34,
					   f32 n41, f32 n42, f32 n43, f32 n44);
	constexpr Matrix4D(const FVec4& a, const FVec4& b, const FVec4& c, const FVec4& d);
	Matrix4D (const Quaternion& q);
	/// @brief Вычисляет матрицу поворота на основе кватерниона и пройденной центральной точки.
	/// @param q кватернион
	/// @param v вектор
	Matrix4D (const Quaternion &q, const FVec3 &center);

	f32& operator ()(int i, int j);
	const f32& operator ()(int i, int j) const;
	f32& operator ()(int i);
	const f32& operator ()(int i) const;
	FVec4& operator [](int j);
	const FVec4& operator [](int j) const;
	Matrix4D& operator=(const Matrix4D& m);

	/// @brief Инвертирует текущую матрицу
	/// @return инвертированную матрицу
	void Inverse();
	/// @brief Создает и возвращает единичную матрицу:
	/// @return новая единичная матрица
	static Matrix4D& Identity();
	/// @brief Создает и возвращает матрицу ортогональной проекции. Обычно используется для рендеринга плоских или 2D-сцен.
	/// @param left левая сторона усеченного изображения.
	/// @param right правая сторона усеченного изображения.
	/// @param bottom нижняя сторона усеченного вида.
	/// @param top верхняя сторона усеченного вида.
	/// @param NearClip расстояние до ближней плоскости отсечения.
	/// @param FarClip дальнее расстояние от плоскости отсечения.
	/// @return новая матрица ортогональной проекции.
	void OrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip) {
		Matrix4D m;

    	f32 lr = 1.0f / (left - right);
    	f32 bt = 1.0f / (bottom - top);
    	f32 nf = 1.0f / (NearClip - FarClip);

    	m(1, 1) = -2.0f * lr;
    	m(2, 2) = -2.0f * bt;
    	m(3, 3) =  2.0f * nf;

    	m(4, 1) = (left + right) * lr;
    	m(4, 2) = (top + bottom) * bt;
    	m(4, 3) = (FarClip + NearClip) * nf;
		m(4, 4) = 1.0f;
	}

	/// @brief Создает и возвращает единичную матрицу:
	/// @return новая единичная матрица
	static constexpr Matrix4D MakeIdentity() {
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
	static MINLINE Matrix4D MakeOrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip) {
		Matrix4D m;

    	f32 lr = 1.0f / (left - right);
    	f32 bt = 1.0f / (bottom - top);
    	f32 nf = 1.0f / (NearClip - FarClip);

    	m(1, 1) = -2.0f * lr;
    	m(2, 2) = -2.0f * bt;
    	m(3, 3) =  2.0f * nf;

    	m(4, 1) = (left + right) * lr;
    	m(4, 2) = (top + bottom) * bt;
    	m(4, 3) = (FarClip + NearClip) * nf;
		m(4, 4) = 1.0f;
    	return m;
	}
	/// @brief Создает матрицу 4х4, которая представляет перспективную проекцию для усеченной видимости
	/// @param fovy вертикальное поле зрения в радианах.
	/// @param s соотношение сторон области просмотра.
	/// @param n расстояние до ближней плоскости отсечения.
	/// @param f расстояние до дальней плоскости отсечения.
	/// @return класс Matrix4D
	static MINLINE Matrix4D MakeFrustumProjection(f32 fovy, f32 s, f32 n, f32 f) {
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
	static MINLINE Matrix4D MakeRevFrustumProjection(f32 fovy, f32 s, f32 n, f32 f) {
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
	static MINLINE Matrix4D MakeInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e) {
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
	static MINLINE Matrix4D MakeRevInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e) {
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
	static MINLINE Matrix4D MakeLookAt(const FVec3& position, const FVec3& target, const FVec3& up);
	/// @brief Создает и возвращает значение, обратное предоставленной матрице.
	/// @param m матрица, подлежащая инвертированию
	/// @return перевернутая копия предоставленной матрицы.
	static MINLINE Matrix4D MakeInverse(const Matrix4D& m) {
		const FVec3& a = reinterpret_cast<const FVec3&>(m[0]);
		const FVec3& b = reinterpret_cast<const FVec3&>(m[1]);
		const FVec3& c = reinterpret_cast<const FVec3&>(m[2]);
		const FVec3& d = reinterpret_cast<const FVec3&>(m[3]);

		const f32& x = m(1, 4);
		const f32& y = m(2, 4);
		const f32& z = m(3, 4);
		const f32& w = m(4, 4);

		FVec3 s = Cross(a, b);
		FVec3 t = Cross(c, d);
		FVec3 u = a * y - b * x;
		FVec3 v = c * w - d * z;

		float invDet = 1.0F / (Dot(s, v) + Dot(t, u));
		s *= invDet;
		t *= invDet;
		u *= invDet;
		v *= invDet;

		FVec3 r0 = Cross(b, v) + t * y;
		FVec3 r1 = Cross(v, a) - t * x;
		FVec3 r2 = Cross(d, u) + s * w;
		FVec3 r3 = Cross(u, c) - s * z;

		return Matrix4D(	r0.x, 	   r1.x, 	   r2.x,      r3.x,
							r0.y, 	   r1.y, 	   r2.y,      r3.y,
							r0.z, 	   r1.z, 	   r2.z,      r3.z,
						-Dot(b, t), Dot(a, t), -Dot(d, s), Dot(c, s));
	}
	/// @brief Заменяет строки матрицы на столбцы.
	/// @param m матрица, подлежащая транспонированию
	/// @return копию транспонированной матрицы
	static MINLINE Matrix4D MakeTransposed(const Matrix4D& m);
	/// @brief 
	/// @param position 
	/// @return 
	static constexpr Matrix4D MakeTranslation(const FVec3& position) {
		return Matrix4D(	1.0f, 		0.0f, 	   0.0f,    0.0f,
							0.0f, 		1.0f, 	   0.0f,    0.0f,
							0.0f, 		0.0f, 	   1.0f,    0.0f,
	   					position.x, position.y, position.z, 1.0f);
	}
	/// @brief Возвращает матрицу масштаба, используя предоставленный масштаб.
	/// @param scale 
	/// @return 
	static constexpr Matrix4D MakeScale(const FVec3& scale) {
		return Matrix4D (scale.x,  0.f,     0.f,   0.f,
						   0.f,  scale.y,   0.f,   0.f,
						   0.f,    0.f,   scale.z, 0.f,
						   0.f,    0.f,     0.f,   1.f);
	}
};

	/// @brief Умножение матриц 4x4
	/// @param a матрица 4x4
	/// @param b матрица 4x4
	/// @return результат умножения матрицы а на матрицу b
	MAPI Matrix4D operator*(Matrix4D& a, Matrix4D& b);

	namespace Matrix4
	{
	

	MINLINE Matrix4D MakeEulerX(f32 AngleRadians)
	{
		Matrix4D m = Matrix4D::MakeIdentity();
		f32 c = Math::cos(AngleRadians);
    	f32 s = Math::sin(AngleRadians);

		m(5)  = c;  //m(2, 2) = c;
		m(6)  = s;  //m(2, 3) = s;
		m(9)  = -s; //m(3, 2) = -s;
		m(10) = c;  //m(3, 3) = -c;

    	return m;
	}

	MAPI MINLINE Matrix4D MakeEulerY(f32 AngleRadians)
	{
		Matrix4D m = Matrix4D::MakeIdentity();
		f32 c = Math::cos(AngleRadians);
    	f32 s = Math::sin(AngleRadians);

		m(0)  = c;  //m(1, 1) =  c;
		m(2)  = -s; //m(1, 3) = -s;
		m(8)  = s;  //m(3, 1) =  s;
		m(10) = c;  //m(3, 3) =  c;

    	return m;
	}

	MAPI MINLINE Matrix4D MakeEulerZ(f32 AngleRadians)
	{
		Matrix4D m = Matrix4D::MakeIdentity();
		f32 c = Math::cos(AngleRadians);
    	f32 s = Math::sin(AngleRadians);

		m(0) = c;//m(1, 1) =  c;
		m(1) = s;//m(1, 2) =  s;
		m(4) = -s;//m(2, 1) = -s;
		m(5) = c;//m(2, 2) =  c;

    	return m;
	}

	MAPI MINLINE Matrix4D MakeEulerXYZ(f32 X_Radians, f32 Y_Radians, f32 Z_Radians)
	{
		Matrix4D rx = MakeEulerX(X_Radians);
		Matrix4D ry = MakeEulerY(Y_Radians);
		Matrix4D rz = MakeEulerZ(Z_Radians);
		Matrix4D m = rx * ry;
		m = m * rz;

    	return m;
	}

	MAPI MINLINE Matrix4D MakeEulerXYZ(FVec3 v)
	{
		Matrix4D rx = MakeEulerX(v.x);
		Matrix4D ry = MakeEulerY(v.y);
		Matrix4D rz = MakeEulerZ(v.z);
		Matrix4D m = rx * ry;
		m = m * rz;

   		return m;
	}
	/// @brief Возвращает вектор направленный вперед относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE FVec3 Forward(const Matrix4D& m)
	{
		return -Normalize(FVec3(m(2), m(6), m(10)));
	}
	/// @brief Возвращает вектор направленный назад относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE FVec3 Backward(const Matrix4D& m)
	{
		return Normalize(FVec3(m(2), m(6), m(10)));
	}
	/// @brief Возвращает вектор направленный вверх относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE FVec3 Up(const Matrix4D& m);
	/// @brief Возвращает вектор направленный вниз относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE FVec3 Down(const Matrix4D& m);
	/// @brief Возвращает вектор направленный влево относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE FVec3 Left(const Matrix4D& m)
	{
		return -Normalize(FVec3(m(0), m(4), m(8)));
	}
	/// @brief Возвращает вектор направленный вправо относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	MINLINE FVec3 Right(const Matrix4D& m)
	{
		return Normalize(FVec3(m(0), m(4), m(8)));
	}
	} // namespace Matrix4D
	
	// 0  1  2  3
	// 4  5  6  7
	// 8  9 10 11
	//12 13 14 15