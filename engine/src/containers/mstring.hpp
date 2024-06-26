#pragma once

#include "defines.hpp"
#include "math/vector4d.hpp"
//#include "containers/darray.hpp"

template<typename> class DArray;

class MAPI MString
{
private:
    u16 length = 0;
    char* str = nullptr;

public:
   constexpr MString();
   // constexpr MString(u16 length);
   constexpr MString(const char* s);
   constexpr MString(const MString& s);
   constexpr MString(MString&& s);
    ~MString();

    // char operator[] (u64 i);
    const char operator[] (u16 i) const;

    MString& operator= (const MString& s);
    MString& operator= (const char* s);
    //MString& operator= (char c);
    //MString& operator= (MString&& s) noexcept;

    explicit operator bool() const;

    bool operator== (const MString& rhs) const;
    bool operator== (const char* s) const;

    /// @brief Получает длину заданной строки.
    /// @return Длину(количество символов в строке)
    constexpr u16 Length() const noexcept;
    /// @brief Получает длину заданной строки.
    /// @param s строка в стиле си
    /// @return Длину(количество символов в строке)
    static const u16 Length(const char* s);
private:
    u16 Len(const char* s);
public:
    //char* Copy(const char* s);

    /// @return строку типа си
    const char* c_str() const noexcept;
    /// @brief Срвавнивает строки между собой без учета регистра.
    /// @param string строка с которой нужно сравнить
    /// @return true, если строки равны и false, если нет 
    const bool Comparei(const MString& string) const;
    /// @brief Срвавнивает строки между собой без учета регистра.
    /// @param string строка в стиле си с которой нужно сравнить
    /// @return true, если строки равны и false, если нет 
    const bool Comparei(const char* string) const;
    /// @brief Копирует строку
    /// @param s строка которую нужно скопировать
    /// @return указатель на копию строки
    static char* Duplicate(const char* s);
    // Выполняет форматирование строки для определения заданной строки формата и параметров.
    static i32 Format(char* dest, const char* format, ...);
    /// @brief Выполняет форматирование переменной строки для определения заданной строки формата и va_list.
    /// @param dest определяет место назначения для отформатированной строки.
    /// @param format отформатируйте строку, которая должна быть отформатирована.
    /// @param va_list cписок переменных аргументов.
    /// @return размер записываемых данных.
    static i32 FormatV(char* dest, const char* format, char* va_list);

    static void Copy(char* dest, const char* source);
    static void Copy(char* dest, const MString& source);
    void nCopy(const MString& source, u64 length);
    static void nCopy(char* dest, const char* source, u64 Length);
    static void nCopy(char* dest, const MString& source, u64 length); 
private:
    char* Copy(const char* source, u64 length);
    char* Copy(const MString& source);
public:
    void Trim();
    static char* Trim(char* s);

    /// @brief Функция зануляет строку
    /// @param string Строка которую нужно занулить
    static void Zero(char* string);

    /// @brief Получает подстроку исходной строки между началом и длиной или до конца строки. 
    /// Если длина отрицательна или равна 0, переходит к концу строки. 
    /// 
    /// Выполняется путем размещения нулей в строке в соответствующих точках.
    /// @param dest указатель на массив символов в который нужно скопировать
    /// @param source указатель на исходную строку с текстом в стиле си
    /// @param start индекс символа с которого нужно обрезать строку
    /// @param length длина обрезки строки
    static void Mid(char* dest, const MString& source, i32 start, i32 length);
    /// @brief Возвращает индекс первого вхождения c в строку; в противном случае -1.
    /// @param str Строка для сканирования.
    /// @param c Персонаж, которого нужно искать.
    /// @return Индекс первого вхождения c; в противном случае -1, если не найден. 
    static i32 IndexOf(char* str, char c);
    /// @brief Возвращает индекс первого вхождения c в строку; в противном случае -1.
    /// @param str Строка для сканирования.
    /// @param c Персонаж, которого нужно искать.
    /// @return Индекс первого вхождения c; в противном случае -1, если не найден.
    i32 IndexOf(char c);
    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str Строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0 3.0 4.0")
    /// @param OutVector A pointer to the vector to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false. 
    static bool ToVector4D(char* str, Vector4D<f32>& OutVector);
    bool ToVector4D(Vector4D<f32>& OutVector);
    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str Строка для анализа. Должна быть разделена пробелами (т. е. «1,0 2,0 3,0»)
    /// @param OutVector Ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToVector3D(char* str, Vector3D<f32>& OutVector);
    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str Строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0")
    /// @param OutVector A pointer to the vector to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToVector2D(char* str, Vector2D<f32>& OutVector);
    /// @brief Пытается проанализировать 32-битное число с плавающей запятой из предоставленной строки.
    /// @param str Строка для анализа. *Не* должно иметь постфикс «f».
    /// @param f A pointer to the float to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToFloat(char* str, f32& f);
    /// @brief Пытается проанализировать 64-битное число с плавающей запятой из предоставленной строки.
    /// @param str Строка для анализа.
    /// @param f A pointer to the float to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToFloat(char* str, f64& f);
    ///@brief Пытается проанализировать 8-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i8& i);
    ///@brief Пытается проанализировать 16-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i16& i);
    ///@brief Пытается проанализировать 32-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i32& i);
    ///@brief Пытается проанализировать 64-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i64& i);
    ///@brief Пытается проанализировать 8-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u8& u);
    ///@brief Пытается проанализировать 16-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u16& u);
    ///@brief Пытается проанализировать 32-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u32& u);
    ///@brief Пытается проанализировать 64-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u64& u);
    ///@brief Пытается проанализировать логическое значение из предоставленной строки.
    ///«true» или «1» считаются true; все остальное false.
    ///@param str Строка для анализа. «true» или «1» считаются true; все остальное false.
    ///@param b Указатель на логическое значение для записи.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToBool(/*char* str, */bool& b);
    /// @brief Сравнение строк с учетом регистра. True, если совпадает, в противном случае false.
    /// @param strL 
    /// @param strR 
    /// @return
    static bool Equal(const char* strL, const char* strR);
    /// @brief Сравнение строк без учета регистра. True, если совпадает, в противном случае false.
    /// @param str0 
    /// @param str1 
    /// @return 
    static bool Equali(const char* str0, const char* str1);
    /// @brief Разбивает заданную строку по предоставленному разделителю 
    /// и сохраняет в предоставленном массиве. При необходимости обрезает каждую запись. 
    /// ПРИМЕЧАНИЕ: Для каждой записи происходит выделение строки, которая должна быть освобождена вызывающей стороной.
    /// @param str строка, которую нужно разделить.
    /// @param delimiter символ, по которому нужно разделить.
    /// @param darray массив строк для хранения записей.
    /// @param TrimEntries обрезает каждую запись, если это true.
    /// @param IncludeEmpty указывает, следует ли включать пустые записи.
    /// @return Количество записей, полученных в результате операции разделения.
    static u32 Split(const char* str, char delimiter, DArray<MString>& darray, bool TrimEntries, bool IncludeEmpty);
    /// @brief Разбивает заданную строку по предоставленному разделителю 
    /// и сохраняет в предоставленном массиве. При необходимости обрезает каждую запись. 
    /// ПРИМЕЧАНИЕ: Для каждой записи происходит выделение строки, которая должна быть освобождена вызывающей стороной.
    /// @param delimiter символ, по которому нужно разделить.
    /// @param darray массив строк для хранения записей.
    /// @param TrimEntries обрезает каждую запись, если это true.
    /// @param IncludeEmpty указывает, следует ли включать пустые записи.
    /// @return Количество записей, полученных в результате операции разделения.
    u32 Split(char delimiter, DArray<MString>& darray, bool TrimEntries, bool IncludeEmpty);

    void Clear();
    // void* operator new(u64 size);
    // void operator delete(void* ptr, u64 size);
    // void* operator new[](u64 size);
    // void operator delete[](void* ptr, u64 size);
};

