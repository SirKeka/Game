#pragma once

#include "defines.hpp"
#include "math/vector4d.hpp"

class MAPI MString
{
private:
    char* str;

    u64 lenght;

public:
    MString();
    MString(u64 lenght);
    MString(const char* s);
    MString(const MString& s);
    MString(MString&& s);
    ~MString();

    MString& operator= (const MString& s);
    MString& operator= (const char* s);
    //MString& operator= (char c);
    //MString& operator= (MString&& s) noexcept;

    explicit operator bool() const;

    bool operator== (const MString& rhs);

    /// @param str константная строка
    /// @return длину(количество символов в строке)
    u64 Length();
    static u64 Length(const char* s);
    //char* Copy(const char* s);

    /// @return строку типа си
    const char* c_str() const noexcept;

    // Выполняет форматирование строки для определения заданной строки формата и параметров.
    static i32 Format(char* dest, const char* format, ...);
    /// @brief Выполняет форматирование переменной строки для определения заданной строки формата и va_list.
    /// @param dest определяет место назначения для отформатированной строки.
    /// @param format отформатируйте строку, которая должна быть отформатирована.
    /// @param va_list cписок переменных аргументов.
    /// @return размер записываемых данных.
    static i32 FormatV(char* dest, const char* format, char* va_list);

    static char* Copy(char* dest, const char* source);
    static char* nCopy(char* dest, const char* source, i64 length);

    MString& Trim();
    static char* Trim(char* s);

    static void Mid(char* dest, const char* source, i32 start, i32 length);
    /// @brief Возвращает индекс первого вхождения c в строку; в противном случае -1.
    /// @param str Строка для сканирования.
    /// @param c Персонаж, которого нужно искать.
    /// @return Индекс первого вхождения c; в противном случае -1, если не найден. 
    static i32 IndexOf(char* str, char c);
 
    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str Строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0 3.0 4.0")
    /// @param OutVector A pointer to the vector to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false. 
    static bool ToVector4D(char* str, Vector4D<f32>* OutVector);

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str Строка для анализа. Должна быть разделена пробелами (т. е. «1,0 2,0 3,0»)
    /// @param OutVector Ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToVector3D(char* str, Vector3D<f32>* OutVector);

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str Строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0")
    /// @param OutVector A pointer to the vector to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToVector2D(char* str, Vector2D<f32>* OutVector);

    /// @brief Пытается проанализировать 32-битное число с плавающей запятой из предоставленной строки.
    /// @param str Строка для анализа. *Не* должно иметь постфикс «f».
    /// @param f A pointer to the float to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToFloat(char* str, f32* f);

    /// @brief Пытается проанализировать 64-битное число с плавающей запятой из предоставленной строки.
    /// @param str Строка для анализа.
    /// @param f A pointer to the float to write to.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToFloat(char* str, f64* f);

    ///@brief Пытается проанализировать 8-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i8* i);

    ///@brief Пытается проанализировать 16-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i16* i);

    ///@brief Пытается проанализировать 32-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i32* i);

    ///@brief Пытается проанализировать 64-битное целое число со знаком из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param i Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(char* str, i64* i);

    ///@brief Пытается проанализировать 8-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u8* u);

    ///@brief Пытается проанализировать 16-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u16* u);

    ///@brief Пытается проанализировать 32-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u32* u);

    ///@brief Пытается проанализировать 64-битное целое число без знака из предоставленной строки.
    ///@param str Строка для анализа.
    ///@param u Указатель на int, в который осуществляется запись.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToUInt(char* str, u64* u);

    ///@brief Пытается проанализировать логическое значение из предоставленной строки.
    ///«true» или «1» считаются true; все остальное false.
    ///@param str Строка для анализа. «true» или «1» считаются true; все остальное false.
    ///@param b A pointer to the boolean to write to.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToBool(char* str, bool* b);

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

    void Destroy();
};

//MAPI bool operator== (const char*   lhs, const string& rhs);
//MAPI bool operator== (const string& lhs, const char*   rhs);
