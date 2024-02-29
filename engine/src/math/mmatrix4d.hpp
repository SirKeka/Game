#pragma once

#include "defines.hpp"

#include "quaternion.hpp"

template<typename T> class Vector3D;
template<typename T> class Vector4D;

class Matrix4D
{
protected:
	// TODO: SIMD
	f32 n[4][4];

public:
	Matrix4D() = default;
	Matrix4D(f32 n00, f32 n01, f32 n02, f32 n03,
			 f32 n10, f32 n11, f32 n12, f32 n13,
			 f32 n20, f32 n21, f32 n22, f32 n23,
			 f32 n30, f32 n31, f32 n32, f32 n33);
	Matrix4D(const Vector4D<f32>& a, const Vector4D<f32>& b, const Vector4D<f32>& c, const Vector4D<f32>& d);
	Matrix4D (Quaternion& q);
	/// @brief Вычисляет матрицу поворота на основе кватерниона и пройденной центральной точки.
	/// @param q кватернион
	/// @param v вектор
	Matrix4D (const Quaternion &q, const Vector3D<f32> &center);

	f32& operator ()(int i, int j);
	const f32& operator ()(int i, int j) const;
	Vector4D<f32>& operator [](int j);
	const Vector4D<f32>& operator [](int j) const;

	/// @brief Инвертирует текущую матрицу
	/// @return инвертированную матрицу
	//Matrix4D& Inverse();

	/// @brief Создает и возвращает единичную матрицу:
	/// @return новая единичная матрица
	static MINLINE Matrix4D MakeIdentity();
	/// @brief Создает и возвращает матрицу ортогональной проекции. Обычно используется для рендеринга плоских или 2D-сцен.
	/// @param left левая сторона усеченного изображения.
	/// @param right правая сторона усеченного изображения.
	/// @param bottom нижняя сторона усеченного вида.
	/// @param top верхняя сторона усеченного вида.
	/// @param NearClip расстояние до ближней плоскости отсечения.
	/// @param FarClip дальнее расстояние от плоскости отсечения.
	/// @return новая матрица ортогональной проекции.
	static MINLINE Matrix4D MakeOrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 NearClip, f32 FarClip);
	/// @brief Создает матрицу 4х4, которая представляет перспективную проекцию для усеченной видимости
	/// @param fovy вертикальное поле зрения в радианах.
	/// @param s соотношение сторон области просмотра.
	/// @param n расстояние до ближней плоскости отсечения.
	/// @param f расстояние до дальней плоскости отсечения.
	/// @return класс Matrix4D
	static MINLINE Matrix4D MakeFrustumProjection(f32 fovy, f32 s, f32 n, f32 f);
	/// @brief Cоздает матрицу 4×4, которая представляет обратную перспективную проекцию для усеченного вида
	/// @param fovy вертикальное поле зрения в радианах.
	/// @param s соотношение сторон области просмотра.
	/// @param n расстояние до ближней плоскости отсечения.
	/// @param f расстояние до дальней плоскости отсечения.
	/// @return структура данных Matrix4D
	static MINLINE Matrix4D MakeRevFrustumProjection(f32 fovy, f32 s, f32 n, f32 f);
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
	static MINLINE Matrix4D MakeInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e);
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
	static MINLINE Matrix4D MakeRevInfiniteProjection(f32 fovy, f32 s, f32 n, f32 e);
	/// @brief Создает и возвращает матрицу просмотра или матрицу, рассматривающую цель с точки зрения позиции.
	/// @param position положение матрицы.
	/// @param target позиция, на которую нужно "смотреть".
	/// @param up восходящий вектор.
	/// @return Матрица, рассматривающая цель с точки зрения позиции.
	static MINLINE Matrix4D MakeLookAt(const Vector3D<f32>& position, const Vector3D<f32>& target, const Vector3D<f32>& up);
	/// @brief Создает и возвращает значение, обратное предоставленной матрице.
	/// @param m матрица, подлежащая инвертированию
	/// @return перевернутая копия предоставленной матрицы.
	static MINLINE Matrix4D MakeInverse(const Matrix4D& m);
	/// @brief Заменяет строки матрицы на столбцы.
	/// @param m матрица, подлежащая транспонированию
	/// @return копию транспонированной матрицы
	static MINLINE Matrix4D MakeTransposed(const Matrix4D& m);
	/// @brief 
	/// @param position 
	/// @return 
	static MINLINE Matrix4D MakeTranslation(const Vector3D<f32>& position);
	/// @brief Возвращает матрицу масштаба, используя предоставленный масштаб.
	/// @param scale 
	/// @return 
	static MINLINE Matrix4D MakeScale(const Vector3D<f32>& scale);
	static MINLINE Matrix4D MakeEulerX(f32 AngleRadians);
	static MINLINE Matrix4D MakeEulerY(f32 AngleRadians);
	static MINLINE Matrix4D MakeEulerZ(f32 AngleRadians);
	static MINLINE Matrix4D MakeEulerXYZ(f32 X_Radians, f32 Y_Radians, f32 Z_Radians);
	/// @brief Возвращает вектор направленный вперед относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	static MINLINE Vector3D<f32> Forward(const Matrix4D& m);
	/// @brief Возвращает вектор направленный назад относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	static MINLINE Vector3D<f32> Backward(const Matrix4D& m);
	/// @brief Возвращает вектор направленный вверх относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	static MINLINE Vector3D<f32> Up(const Matrix4D& m);
	/// @brief Возвращает вектор направленный вниз относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	static MINLINE Vector3D<f32> Down(const Matrix4D& m);
	/// @brief Возвращает вектор направленный влево относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	static MINLINE Vector3D<f32> Left(const Matrix4D& m);
	/// @brief Возвращает вектор направленный вправо относительно предоставленной матрицы.
	/// @param m матрица 4х4, на основе которой строится вектор.
	/// @return трехкомпонентный вектор направления.
	static MINLINE Vector3D<f32> Right(const Matrix4D& m);
};

	/// @brief Умножение матриц 4x4
	/// @param a матрица 4x4
	/// @param b матрица 4x4
	/// @return результат умножения матрицы а на матрицу b
	Matrix4D operator*(Matrix4D& a, Matrix4D& b);
