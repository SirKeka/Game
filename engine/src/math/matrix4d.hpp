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
    	f32 t0  = m(10)  * m(15);
    	f32 t1  = m(14)  * m(11);
    	f32 t2  = m(6)   * m(15);
    	f32 t3  = m(14)  * m(7) ;
    	f32 t4  = m(6)   * m(11);
    	f32 t5  = m(10)  * m(7) ;
    	f32 t6  = m(2)   * m(15);
    	f32 t7  = m(14)  * m(3) ;
    	f32 t8  = m(2)   * m(11);
    	f32 t9  = m(10)  * m(3) ;
    	f32 t10 = m(2)   * m(7) ;
    	f32 t11 = m(6)   * m(3) ;
    	f32 t12 = m(8)   * m(13);
    	f32 t13 = m(12)  * m(9) ;
    	f32 t14 = m(4)   * m(13);
    	f32 t15 = m(12)  * m(5) ;
    	f32 t16 = m(4)   * m(9) ;
    	f32 t17 = m(8)   * m(5) ;
    	f32 t18 = m(0)   * m(13);
    	f32 t19 = m(12)  * m(1) ;
    	f32 t20 = m(0)   * m(9) ;
    	f32 t21 = m(8)   * m(1) ;
    	f32 t22 = m(0)   * m(5) ;
    	f32 t23 = m(4)   * m(1) ;

    	Matrix4D o {};

		o(0) = (t0 * m(5) + t3 * m(9) + t4  * m(13)) - (t1 * m(5) + t2 * m(9) + t5  * m(13));
    	o(1) = (t1 * m(1) + t6 * m(9) + t9  * m(13)) - (t0 * m(1) + t7 * m(9) + t8  * m(13));
   		o(2) = (t2 * m(1) + t7 * m(5) + t10 * m(13)) - (t3 * m(1) + t6 * m(5) + t11 * m(13));
    	o(3) = (t5 * m(1) + t8 * m(5) + t11 * m(9))  - (t4 * m(1) + t9 * m(5) + t10 * m(9));

    	f32 d = 1.0f / (m(0) * o(0) + m(4) * o(1) + m(8) * o(2) + m(12) * o(3));

    	o(0) = d * o(0);
    	o(1) = d * o(1);
    	o(2) = d * o(2);
    	o(3) = d * o(3);
		o(4) = d  * ((t1  * m(4)  + t2  * m(8)  + t5  * m(12)) - (t0  * m(4)  + t3  * m(8)  + t4  * m(12)));
    	o(5) = d  * ((t0  * m(0)  + t7  * m(8)  + t8  * m(12)) - (t1  * m(0)  + t6  * m(8)  + t9  * m(12)));
    	o(6) = d  * ((t3  * m(0)  + t6  * m(4)  + t11 * m(12)) - (t2  * m(0)  + t7  * m(4)  + t10 * m(12)));
    	o(7) = d  * ((t4  * m(0)  + t9  * m(4)  + t10 * m(8))  - (t5  * m(0)  + t8  * m(4)  + t11 * m(8)));
    	o(8) = d  * ((t12 * m(7)  + t15 * m(11) + t16 * m(15)) - (t13 * m(7)  + t14 * m(11) + t17 * m(15)));
    	o(9) = d  * ((t13 * m(3)  + t18 * m(11) + t21 * m(15)) - (t12 * m(3)  + t19 * m(11) + t20 * m(15)));
    	o(10) = d * ((t14 * m(3)  + t19 * m(7)  + t22 * m(15)) - (t15 * m(3)  + t18 * m(7)  + t23 * m(15)));
    	o(11) = d * ((t17 * m(3)  + t20 * m(7)  + t23 * m(11)) - (t16 * m(3)  + t21 * m(7)  + t22 * m(11)));
    	o(12) = d * ((t14 * m(10) + t17 * m(14) + t13 * m(6))  - (t16 * m(14) + t12 * m(6)  + t15 * m(10)));
    	o(13) = d * ((t20 * m(14) + t12 * m(2)  + t19 * m(10)) - (t18 * m(10) + t21 * m(14) + t13 * m(2)));
    	o(14) = d * ((t18 * m(6)  + t23 * m(14) + t15 * m(2))  - (t22 * m(14) + t14 * m(2)  + t19 * m(6)));
    	o(15) = d * ((t22 * m(10) + t16 * m(2)  + t21 * m(6))  - (t20 * m(6)  + t23 * m(10) + t17 * m(2)));

    	return o;
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
	