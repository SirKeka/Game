#pragma once

#include "defines.hpp"
#include "math/vector2d_fwd.h"
#include "math/vector3d_fwd.h"
#include "math/vector4d_fwd.h"

template<typename> class DArray;
class Transform;

class MAPI MString
{
private:
    /// @brief Длина строки(количество символов в строке без терминального нуля).
    u32 length      {};
    /// @brief Размер строки в байтах.
    u32 size        {};
    /// @brief Указатель на память где хранится строка.
    char* str{nullptr};

public:
   constexpr MString() : length(), size(), str(nullptr) {}
   /// @brief Создает строку из двух строк
   /// @param str1 первая строка
   /// @param str2 вторая строка
   constexpr MString(const char *str1, const char *str2);
   constexpr MString(const MString &str1, const MString &str2);
   /// @brief Создает строку на основе си строки
   /// @param s си строка
   /// @param trim указывает на то нужно ли копировать управляющие символы. По умолчанию false
   constexpr MString(const char *s, bool trim = false);
   constexpr MString(const MString &s);
   constexpr MString(MString&& s);
    ~MString();

    // char operator[] (u64 i);
    char operator[] (u16 i) const;

    MString& operator= (const MString& s);
    MString& operator= (MString&& s);
    MString& operator= (const char* s);
    //MString& operator= (char c);
    //MString& operator= (MString&& s) noexcept;

    /// @brief Добавляет строку к источнику и возвращает новую строку.
    /// @param s 
    /// @return 
    MString& operator+= (const char* s);

    /// @brief Добавляет строку к источнику и возвращает новую строку.
    /// @param s строка которую нужно добавить
    /// @return ссылку на строку
    MString& operator+=(const MString& s);

    /// @brief Добавляет предоставленное целое число в источник и выводит в назначение.
    /// @param n целое число которое нужно добавить
    /// @return ссылку на строку
    MString& operator+=(i64 n);

    /// @brief Добавляет предоставленное число с плавающей запятой в исходную строку.
    /// @param f число с плавающей запятой которое нужно добавить
    /// @return ссылку на строку
    MString& Append(const char* source, f32 f);

    /// @brief Добавляет предоставленное логическое значение (как «истина» или «ложь») в источник и выводит в назначение.
    /// @param b 
    /// @return ссылку на строку
    MString& operator+=(bool b);

    /// @brief Добавляет предоставленный символ в исходный код и выводит в целевой.
    /// @param c 
    /// @return ссылку на строку
    MString& operator+=(char c);

    void Create(char* str, u64 length);

    /// @brief Зануляет внутренний указатель на строку. 
    /// ПРИМЕЧАНИЕ: Например, полезен в случае с многопоточностью, когда нужно чтобы деструктор не удалил строку до выполнения потока
    void SetNullString() { length = 0; str = nullptr; }

    /// @brief Извлекает каталог из полного пути к файлу.
    /// @tparam N количество символов в массиве
    /// @param arr массив символов в который нужно скопировать путь
    /// @param path путь к файлу
    static void DirectoryFromPath(char* dest, const char* path);

    /// @brief Извлекает имя файла (включая расширение файла) из полного пути к файлу.
    /// @param path путь к файлу
    /// @return ссылку на строку
    MString& FilenameFromPath(const char* path);

    /// @brief Извлекает имя файла (исключая расширение файла) из полного пути к файлу.
    /// @param path путь к файлу
    /// @return ссылку на строку
    static void FilenameNoExtensionFromPath(char* dest, const char* path);

    explicit operator bool() const;

    bool operator== (const MString& rhs) const;
    bool operator== (const char* s) const;

    /// @brief Получает длину заданной строки.
    /// @return Длину строки
    constexpr const u32& Length() const noexcept;

    /// @return Размер строки в байтах.
    constexpr const u32& Size() const noexcept;

    /// @brief Функция считает количество символов в строке ПРИМЕЧАНИЕ: может не совпадать с длиной строки
    /// @return Количество символов в строке
    u32 nChar();

    /// @brief Получает длину заданной строки.
    /// @param s строка в стиле си
    /// @return Длину(количество символов в строке)
    static u32 Length(const char* s);

    /// @brief Получает длину строки в символах UTF-8 (потенциально многобайтовых).
    /// @param str строка для проверки.
    /// @return Длина строки в кодировке UTF-8.
    static u32 UTF8Length(const char* str);

    /// @brief Получает байты, необходимые из массива байтов для формирования кодовой точки UTF-8, а также указывает, сколько байтов занимает текущий символ.
    /// @param bytes массив байтов для выбора.
    /// @param offset смещение в байтах для начала.
    /// @param OutCodepoint переменная для хранения кодовой точки UTF-8.
    /// @param OutAdvance указатель для хранения продвижения или количества байтов, которые занимает кодовая точка.
    /// @return True в случае успеха; в противном случае false для недопустимого/неподдерживаемого UTF-8.
    static bool BytesToCodepoint(const char* bytes, u32 offset, i32& OutCodepoint, u8& OutAdvance);
    bool BytesToCodepoint(u32 offset, i32& OutCodepoint, u8& OutAdvance);
private:
    constexpr u32 Len(const char* s, u32& size, bool DelCon = false);
public:
    //char* Copy(const char* s);
    /// @brief Добавляет к массиву символов строку
    /// @tparam N число символов в массиве
    /// @param arr массив символов
    /// @param string строка которую нужно включить в массив строк
    template<u64 N>
    static void Append(char (&arr)[N], const char* s) {
        for (u16 i = 0, j = 0; i < N; i++) {
            if (arr[i] == '\0' && s[j] != '\0') {
                arr[i] = s[j];
                j++;
            }
        }
    }

    /// @brief Добавляет к массиву символов целое число
    /// @tparam N число символов в массиве
    /// @param arr массив символов
    /// @param n число которое нужно добавить к строке
    template<u64 N>
    static void Append(char (&arr)[N], i64 n) {
        MString s;
        s.IntToString(n);
        for (u64 i = 0, j = 0; i < N; i++) {
            if (arr[i] == '\0') {
                arr[i] = s[j];
                j++;
            }
        }
    }

    /// @return строку типа си
    const char* c_str() const noexcept;

    /// @brief Срвавнивает строки между собой без учета регистра.
    /// @param string строка с которой нужно сравнить
    /// @return true, если строки равны и false, если нет 
    bool Comparei(const MString& string) const;

    /// @brief Срвавнивает строки между собой без учета регистра.
    /// @param string строка в стиле си с которой нужно сравнить
    /// @return true, если строки равны и false, если нет 
    bool Comparei(const char* string, u64 length = 0) const;

    /// @brief Сравнение строк с учетом регистра для нескольких символов.
    /// @param string строка с которой нужно сравнить данную строку
    /// @param length максимальное количество символов для сравнения.
    /// @return true, если то же самое, в противном случае false.
    bool nCompare(const MString& string, u64 length) const;

    /// @brief Сравнение строк с учетом регистра для нескольких символов.
    /// @param string строка в стиле си с которой нужно сравнить данную строку
    /// @param length максимальное количество символов для сравнения.
    /// @return true, если то же самое, в противном случае false.
    bool nCompare(const char* string, u64 length) const;

    /// @brief Сравнение строк без учета регистра для нескольких символов.
    /// @param string строка с которой нужно сравнить данную строку
    /// @param length максимальное количество символов для сравнения.
    /// @return true, если то же самое, в противном случае false.
    bool nComparei(const MString& string, u64 length) const;

    /// @brief Сравнение строк без учета регистра для нескольких символов.
    /// @param string строка в стиле си с которой нужно сравнить данную строку
    /// @param length максимальное количество символов для сравнения.
    /// @return true, если то же самое, в противном случае false.
    bool nComparei(const char* string, u64 length) const;

    static bool nComparei(const char* string1, const char* string2, u64 length);

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

    MString& IntToString(i64 n);

    /// @brief Считывает из строки число и преобразует его в целочисленное 64 битное число.
    /// @param s строка, которую нужно считать.
    /// @return целочисленное 64 битное число считанное из строки.
    static i64 ToInt(const char* s);

    /// @brief Считывает из строки число и преобразует его в целочисленное 64 битное число без знака.
    /// @param s строка, которую нужно считать.
    /// @return целочисленное 64 битное число без знака считанное из строки.
    static u64 ToUInt(const char* s);

    static bool StringToF32 (const char* string, f32(&fArray)[] , u32 aSize);
    static bool StringToF32(const char* s, f32& fn1, f32* fn2 = nullptr, f32* fn3 = nullptr, f32* fn4 = nullptr);

    static char* Copy(char* dest, const char* source, u64 Length = 0, bool DelCon = false);
    // static char* Copy(char* dest, const MString& source, u64 length = 0, bool DelCon = false);
    void Copy(const MString& source, u64 length);

    /// @brief Зануляет статический массив символов
    /// @tparam N число символов в статическом массиве
    /// @param arr ссылка на статический массив
    template<u64 N>
    static void Zero(char (&arr)[N]) {
        if ((N % 4) == 0) {
            u64 count = N / 4;
            for(u64 i = 0; i < count; i += 4) {
                if (arr[i]) {
                    arr[i] = arr[i + 1] = arr[i + 2] = arr[i + 3] = '\0';
                } else break;
            }
        } else {
            for (u64 i = 0; i < N; i++) {
                if(arr[i]) {
                    arr[i] = '\0';
                } else break;
            }
        }
    }
    // static char* Concat();

    void Trim();
    static char* Trim(char* s);

    /// @brief Функция зануляет строку
    /// @param string Строка которую нужно занулить
    //static void Zero(char* string);

    /// @brief Получает подстроку исходной строки между началом и длиной или до конца строки. 
    /// Если длина отрицательна или равна 0, переходит к концу строки. 
    /// Выполняется путем размещения нулей в строке в соответствующих точках.
    /// @param dest указатель на массив символов в который нужно скопировать
    /// @param source указатель на исходную строку с текстом в стиле си
    /// @param start индекс символа с которого нужно обрезать строку
    /// @param length длина обрезки строки
    static void Mid(char* dest, const MString& source, u32 start, i32 length);

    /// @brief Возвращает указатель на строку выделенную в куче, удаляет всю информацию о строке внутри, чтобы строка не была автоматически "удалена".
    /// ПРИМЕЧАНИЕ: во избежание утечки памяти нужно будет удалить вручную
    /// @return указатель на строку.
    char* Move();

    /// @brief Возвращает индекс первого вхождения c в строку; в противном случае -1.
    /// @param str Строка для сканирования.
    /// @param c символ, которого нужно искать.
    /// @return Индекс первого вхождения c; в противном случае -1, если не найден. 
    static i32 IndexOf(char* str, char c);

    /// @brief Возвращает индекс первого вхождения c в строку; в противном случае -1.
    /// @param str строка для сканирования.
    /// @param c символ, который нужно искать.
    /// @return Индекс первого вхождения c; в противном случае -1, если не найден.
    i32 IndexOf(char c);

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0 3.0 4.0")
    /// @param OutVector ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false. 
    static bool ToVector(char* str, FVec4& OutVector);

    bool ToFVector(FVec4& OutVector);

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str строка для анализа. Должна быть разделена пробелами (т. е. «1,0 2,0 3,0»)
    /// @param OutVector ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    static bool ToFVector(char* str, FVec3& OutVector);

    bool ToFVector(FVec3& OutVector);

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0")
    /// @param OutVector ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    static bool ToFVector(char* str, FVec2& OutVector);

    bool ToFVector(FVec2& OutVector);

    /// @brief Пытается проанализировать 32-битное число с плавающей запятой из предоставленной строки.
    /// @param str Строка для анализа. *Не* должно иметь постфикс «f».
    /// @param f ссылка на переменную типа f32(float) для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToFloat(f32& f);

    /// @brief Пытается проанализировать 64-битное число с плавающей запятой из предоставленной строки.
    /// @param str строка для анализа.
    /// @param f ссылка на переменную типа f64(double) для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToFloat(f64& f);

    /// @brief Пытается проанализировать 8-битное целое число со знаком из предоставленной строки.
    /// @param value 
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(i8& value);

    /// @brief Пытается проанализировать 8-битное целое число без знака из предоставленной строки.
    /// @param value 
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(u8& value);

    /// @brief Пытается проанализировать 16-битное целое число со знаком из предоставленной строки.
    /// @param value 
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(i16& value);

    /// @brief Пытается проанализировать 16-битное целое число без знака из предоставленной строки.
    /// @param value 
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(u16& value);

    /// @brief Пытается проанализировать 32-битное целое число со знаком из предоставленной строки.
    /// @param value 
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToInt(i32& value);

    bool ToInt(u32& value);

    /// @brief Пытается проанализировать матрицу 4х4 из предоставленной строки
    /// @param value ссылка на матрицу в которую нужно считать строку
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToMatrix(Matrix4D& value);

    ///@brief Пытается проанализировать логическое значение из предоставленной строки.
    ///«true» или «1» считаются true; все остальное false.
    ///@param str строка для анализа. «true» или «1» считаются true; все остальное false.
    ///@param b ссылка на логическое значение для записи.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToBool(/*char* str, */bool& b);

    /// @brief Пытается проанализировать преобразование из предоставленной строки. 
    /// Если строка содержит 10 элементов, вращение анализируется как кватернион. 
    /// Если она содержит 9 элементов, вращение анализируется как углы Эйлера
    /// и преобразуется в кватернион. Все остальное недопустимо.
    /// @param str строка для анализа.
    /// @param transform ссылка на преобразование для записи.
    /// @return True, если анализ прошел успешно, в противном случае false.
    static bool ToTransform(const char* str, Transform& transform);

    bool ToTransform(Transform& transform);

    /// @brief Сравнение строк с учетом регистра. True, если совпадает, в противном случае false.
    /// @param strL строка которую будем сравнивать
    /// @param strR строка с которой будем сравнивать
    /// @return True если строки одинаковые, false если хотябы на один символ различаются
    static bool Equal(const char* strL, const char* strR);

    /// @brief Сравнение строк без учета регистра. True, если совпадает, в противном случае false.
    /// @param str0 строка которую будем сравнивать
    /// @param str1 строка с которой будем сравнивать
    /// @return True если строки одинаковые, false если хотябы на один символ различаются
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
    u32 Split(char delimiter, DArray<MString>& darray, bool TrimEntries, bool IncludeEmpty) const;

    void Clear();

    /// @brief Удаляет последний символ строки.
    void DeleteLastChar();
    // void* operator new(u64 size);
    // void operator delete(void* ptr, u64 size);
    // void* operator new[](u64 size);
    // void operator delete[](void* ptr, u64 size);
private:
    constexpr char* Copy(const char* source, u64 length, bool DelCon = false);
    constexpr char* Copy(const MString& source);
    constexpr char* Concat(const char *str1, const char *str2, u64 length);

    /// @brief Проверяет размер символа.
    /// @param c константная ссылка на символ.
    /// @return возвращает размер символа в байтах.
    static u32 CheckSymbol(const char& c);
};

/// @brief Функция конкатенации строк
/// @param ls строка к которой будет присоединяться вторая
/// @param rs строка которая будет присоединяться
/// @return строку состоящую из двух других 
MString operator+(const MString& ls, const MString& rs);
MString operator+(const MString& ls, const char* rs);
