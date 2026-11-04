#pragma once

#include "math/vector2d_fwd.h"
#include "math/vector3d_fwd.h"
#include "math/vector4d_fwd.h"

#define S_Fmt "%.*s"
#define S_Arg(s) (s).Length(), (s).Data()
#define SU_Arg(su) (su).size, (su).data

template<typename> class DArray;
struct Transform;
struct Substring;

class MAPI MString
{
public:
   constexpr MString() : size(), capacity(hb_size), heapbuf() {}
   constexpr MString(u32 size);

   /// @brief Оборачивает массив символов в класс ПРИМЕЧАНИЕ: только для строк с количеством символов больше или равно 16
   /// @param size размер строки в байтах
   /// @param str  указатель на массив символов
   constexpr MString(u32 size, char* str) : size(size), capacity(size < hb_size ? hb_size : size), heapbuf(size - 1, str) {}

   /// @brief Создает строку из двух строк
   /// @param str1 первая строка
   /// @param str2 вторая строка
   constexpr MString(const char *str1, const char *str2);
   constexpr MString(const MString &str1, const MString &str2);

   /// @brief Создает строку на основе си строки
   /// @param s си строка
   /// @param trim указывает на то нужно ли копировать управляющие символы. По умолчанию false
   constexpr MString(const char *s, bool trim = false);

   constexpr MString(const char* s, u32 length); 
   constexpr MString(const MString &s);
   constexpr MString(MString&& s);
   constexpr MString(const Substring& ss);
    ~MString();

    // char operator[] (u64 i);

    char operator[] (u32 i) const;

    MString& operator= (const MString& s);
    MString& operator= (MString&& s);
    MString& operator= (const char* s);
    MString& operator= (Substring sb);
    // const char* operator= (MString& s);
    //MString& operator= (char c);

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
    
    explicit operator bool() const;
    
    bool operator== (const MString& rhs) const;
    bool operator== (const char* s) const;
    bool operator<(const MString& rs) const;
    bool operator>(const MString& rs) const;

    void Create(char* str, u64 length);

    /// @brief Очищает строку, но не убирает выделение, памяти
    void Clear();

    /// @brief Зануляет внутренний указатель на строку. 
    /// ПРИМЕЧАНИЕ: Например, полезен в случае с многопоточностью, когда нужно чтобы деструктор не удалил строку до выполнения потока
    void Null() { size = 0; heapbuf.length = 0; heapbuf.str = nullptr; }

    const char* Data(u32* size = nullptr) const;

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

    /// @brief Получает длину заданной строки.
    /// @return Длину строки
    // constexpr u32 UTF8Length();

    /// @return Длину строки
    constexpr u32 Length() const;

    /// @return Размер строки в байтах.
    /// @brief Функция считает количество символов в строке ПРИМЕЧАНИЕ: может не совпадать с длиной строки
    /// @return Количество символов в строке
    // u32 nChar();

    /// @return Размер строки в байтах, если строка < hb_size, то возвращает длину строки
    MINLINE constexpr u32 Size() const { return size; }

    /// @brief Получает длину заданной строки.
    /// @param s строка в стиле си
    /// @param UTF8 указывает что строка является UTF-8. По умолчанию false.
    /// @param size используется для структур с текстом который не заканчивается терминальным нулем. Например Substring. По умолчанию 0;
    /// @return Длину(количество символов в строке)
    static u32 Length(const char* s, bool UTF8 = false, u32 size = 0);

    /// @brief Получает байты, необходимые из массива байтов для формирования кодовой точки UTF-8, а также указывает, сколько байтов занимает текущий символ.
    /// @param bytes массив байтов для выбора.
    /// @param offset смещение в байтах для начала.
    /// @param OutCodepoint переменная для хранения кодовой точки UTF-8.
    /// @param OutAdvance указатель для хранения продвижения или количества байтов, которые занимает кодовая точка.
    /// @return True в случае успеха; в противном случае false для недопустимого/неподдерживаемого UTF-8.
    static bool BytesToCodepoint(const char* bytes, u32 offset, i32& OutCodepoint, u8& OutAdvance);
    bool BytesToCodepoint(u32 offset, i32& OutCodepoint, u8& OutAdvance) const;
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
    const char* c_str() const {
        return capacity <= hb_size ? stackbuf : heapbuf.str;
    }

    /// @brief Сравнение строк с учетом регистра.
    /// @param strL строка которую нужно сравнить
    /// @param strR строка с которой нужно сравнить
    /// @param length максимальное количество символов для сравнения. ПРИМЕЧАНИЕ: если длина указана 0, то сравнивает всю строку целиком
    /// @return true, если строки равны и false, если нет
    static bool Compare(const char* strL, const char* strR, u64 length = 0);

    /// @brief Срвавнивает строки между собой без учета регистра.
    /// @param cstringL строка которую нужно сравнить
    /// @param cstringR строка с которой нужно сравнить
    /// @param length максимальное количество символов для сравнения. ПРИМЕЧАНИЕ: если длина указана 0, то сравнивает всю строку целиком
    /// @return true, если строки равны и false, если нет
    static bool Comparei(const char* сstringL, const char* cstringR, u64 length = 0);

    /// @brief Сравнение строк с учетом регистра.
    /// @param cstring строка в стиле си с которой нужно сравнить данную строку
    /// @param length максимальное количество символов для сравнения. ПРИМЕЧАНИЕ: если длина указана 0, то сравнивает всю строку целиком
    /// @return true, если то же самое, в противном случае false.
    bool Compare(const char* cstring, u64 length) const;

    /// @brief Сравнение строк с учетом регистра.
    /// @param string строка с которой нужно сравнить данную строку
    /// @param length максимальное количество символов для сравнения. ПРИМЕЧАНИЕ: если длина указана 0, то сравнивает всю строку целиком
    /// @return true, если то же самое, в противном случае false.
    bool Compare(const MString& string, u64 length) const;

    /// @brief Срвавнивает строки между собой без учета регистра.
    /// @param string строка с которой нужно сравнить
    /// @return true, если строки равны и false, если нет
    bool Comparei(const MString& string) const;

    /// @brief Срвавнивает строки между собой без учета регистра.
    /// @param string строка в стиле си с которой нужно сравнить
    /// @return true, если строки равны и false, если нет 
    bool Comparei(const char* string, u64 length = 0) const;

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
    static bool ToInt(const char* s, i64& num, u32 length = 0);

    static bool ToInt(const char* s, i32& num, u32 length = 0);

    /// @brief Считывает из строки число и преобразует его в целочисленное 32 битное число без знака.
    /// @param str строка, которую нужно считать.
    /// @param num целочисленное 32 битное число без знака считанное из строки.
    /// @return true если успешно, иначе false.
    static bool ToInt(const char* str, u32& num, u32 length = 0);

    static bool ToF32(const char* str, f32& num, u32 length = 0);

    /// @brief Считывает из строки число и преобразует его в целочисленное 64 битное число без знака.
    /// @param s строка, которую нужно считать.
    /// @return целочисленное 64 битное число без знака считанное из строки.
    static bool ToInt(const char* s, u64& num, u32 length = 0);

    static bool StringToF32 (const char* string, f32(&fArray)[] , u32 aSize, u32 length = 0);
    static bool StringToF32(const char* s, f32& num, u32 length = 0);

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
    static char* Trim(char* s, u64* length = nullptr);

    /// @brief Функция зануляет строку
    /// @param string Строка которую нужно занулить
    //static void Zero(char* string);

    /// @brief Получает подстроку исходной строки между началом и длиной или до конца строки. 
    /// Если длина равна 0, переходит к концу строки. 
    /// @param start индекс символа с которого нужно обрезать строку
    /// @param length длина обрезки строки, если длина равна нулю
    /// @return Подстроку с уазанной длиной
    Substring Mid(u32 start, u32 length = 0);

    void Mid(const MString& source, u32 start, i32 length);

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

    /// @brief Возвращает индекс первого вхождения c в строку иначе -1.
    /// @param str Строка для сканирования.
    /// @param c символ, которого нужно искать.
    /// @return Индекс первого вхождения c иначе -1, если не найден. 
    static i32 IndexOf(const char* str, char c, u32 length = 0);

    /// @brief Возвращает индекс первого вхождения c в строку иначе -1.
    /// @param str строка для сканирования.
    /// @param c символ, который нужно искать.
    /// @return Индекс первого вхождения c иначе -1, если не найден.
    i32 IndexOf(char c) {
        return MString::IndexOf(pData(), c, Length());
    }

    /// @brief Попытки извлечь длину массива из данной строки. Например: строка 'sampler2D[4]' вернет True и установит ArrayLength в 4.
    /// @param ArrayLength Указатель для хранения длины, если извлечение прошло успешно. 
    /// @return true, если длина массива была найдена и разобрана иначе false.
    bool ParseArrayLength(u32& ArrayLength);

    bool ToInt(u32& value);

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0 3.0 4.0")
    /// @param OutVector ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно иначе false. 
    static bool ToVector(char* str, FVec4& OutVector);

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str строка для анализа. Должна быть разделена пробелами (т. е. «1,0 2,0 3,0»)
    /// @param OutVector ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    static bool ToFVector(char* str, FVec3& OutVector);

    bool ToF32(f32& value) { return ToF32(c_str(), value); }
    bool ToFVector(FVec3& OutVector) { return StringToF32(pData(), OutVector.elements, 3); }
    bool ToFVector(FVec4& OutVector) { return StringToF32(pData(), OutVector.elements, 4); }

    /// @brief Пытается проанализировать вектор из предоставленной строки.
    /// @param str строка для анализа. Должна быть разделена пробелами (т. е. "1.0 2.0")
    /// @param OutVector ссылка на вектор для записи.
    /// @return True, если синтаксический анализ прошел успешно; в противном случае false.
    static bool ToFVector(char* str, FVec2& OutVector);

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

    /// @brief Сравнение строк без учета регистра.
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
    u32 Split(char delimiter, DArray<MString>& darray, bool TrimEntries, bool IncludeEmpty);

    /// @brief Разбивает заданную строку по предоставленному разделителю 
    /// и сохраняет в предоставленном массиве. При необходимости обрезает каждую запись. 
    /// ПРИМЕЧАНИЕ: Для каждой записи происходит выделение строки, которая должна быть освобождена вызывающей стороной.
    /// @param delimiter символ, по которому нужно разделить.
    /// @param darray массив строк для хранения записей.
    /// @param TrimEntries обрезает каждую запись, если это true.
    /// @param IncludeEmpty указывает, следует ли включать пустые записи.
    /// @return Количество записей, полученных в результате операции разделения.
    u32 Split(char delimiter, DArray<MString>& darray, bool TrimEntries, bool IncludeEmpty) const;

    /// @brief Разбивает заданную строку по предоставленному разделителю 
    /// и сохраняет в предоставленном массиве. При необходимости обрезает каждую запись. 
    /// ПРИМЕЧАНИЕ: Для каждой записи происходит выделение строки, которая должна быть освобождена вызывающей стороной.
    /// @param delimiter символ, по которому нужно разделить.
    /// @param darray массив подстрок для хранения записей.
    /// @param TrimEntries обрезает каждую запись, если это true.
    /// @param IncludeEmpty указывает, следует ли включать пустые записи.
    /// @return Количество записей, полученных в результате операции разделения.
    u32 Split(char delimiter, DArray<Substring>& darray, bool TrimEntries, bool IncludeEmpty);

    /// @brief Удаляет строку
    void Destroy();

    /// @brief Копирует заданную часть строки
    /// @param position 
    /// @param length 
    /// @return копию заданной части
    MString CopyPart(u32 position, u32 length) const;

    void InsertChar(u32 position, char ch);

    /// @brief Удаляет символы из начала строки
    /// @param count количество удаляемых символов
    void RemoveFirstChar(u32 count = 1);

    /// @brief Удаляет символы с конца строки.
    /// @param count количество удаляемых сиволов из конца строки
    void RemoveLastChar(u32 count = 1);

    /// @brief Удаляет заданную часть строки
    /// @param position позиция с которой нужно начать удаление
    /// @param length длина которую нужно удалить
    void RemovePart(u32 position, u32 length);

    void* operator new(u64 size);
    void* operator new[](u64 size);
    void operator delete(void* ptr, u64 size);
    void operator delete[](void* ptr, u64 size);

private:
    u32 size;           // Текущий размер данных 
    u32 capacity;       // Количество байт зарезервированных в памяти на данный момент
    // bool utf8;          // Указывает является ли строка UTF8
    union {
        struct {
            u32 length; // Размер строки в символах .
            char* str;  // Указатель на память где хранятся данные строки.
        } heapbuf;
        char stackbuf[sizeof(heapbuf)];
    };

    static constexpr u32 hb_size = sizeof(heapbuf);

    /// @brief Копирует строку
    /// @param source указатель на строку, которую нужно скопировать
    /// @param length длина строки с терминальным нулем ПРИМЕЧАНИЕ: для строк длиной <16 без терминального нуля
    /// @param DelCon 
    constexpr void Copy(const char* source, u64 size, u64 length, bool DelCon = false);
    // constexpr char* Copy(const MString& source);
    constexpr char* Concat(const char *str1, const char *str2, u32 length1, u32 length2);
    // constexpr auto& CopyHeapbuf(auto& heapbuf);
    constexpr char* pData(u32 *length = nullptr);

    /// @brief Проверяет размер символа.
    /// @param c константная ссылка на символ.
    /// @return возвращает размер символа в байтах.
    static u32 CheckSymbol(const char& c);
};

/// @brief Функция конкатенации строк
/// @param ls строка к которой будет присоединяться вторая
/// @param rs строка которая будет присоединяться
/// @return строку состоящую из двух других 
MINLINE MString operator+(const MString& ls, const MString& rs) { return MString(ls, rs); }
MString operator+(const MString& ls, const char* rs);
MString operator+(const char* ls, const MString& rs);

/// @brief Подстрока, являющаяся частью другой строки. ПРИМЕЧАНИЕ: будет удалена, если основная строка будет уничтожена. Может не содержать \0
struct Substring
{
    u32 size{};   // Размер строки в байтах.
    u32 length{}; // Размер строки в символах.
    char* data{}; // Указатель на массив символов без терминального нуля

    explicit operator bool() const;
    
    bool Comparei(const char* str, u64 length = 0ULL) { return MString::Comparei(data, str, length ? length : size); }

    /// @brief Возвращает индекс первого вхождения c в строку иначе -1.
    /// @param c символ, который нужно искать.
    /// @return Индекс первого вхождения c иначе -1, если не найден.
    i32 IndexOf(char c) { return MString::IndexOf(data, c, size); }

    Substring Mid(u32 start, u32 length = 0) const;

    /// @brief Попытки извлечь длину массива из данной строки. Например: строка 'sampler2D[4]' вернет True и установит ArrayLength в 4.
    /// @param ArrayLength Указатель для хранения длины, если извлечение прошло успешно. 
    /// @return true, если длина массива была найдена и разобрана иначе false.
    bool ParseArrayLength(u32& ArrayLength);

    u32 Split(char delimiter, DArray<Substring>& darray, bool TrimEntries, bool IncludeEmpty);

    ///@brief Пытается проанализировать логическое значение из предоставленной строки.
    ///«true» или «1» считаются true; все остальное false.
    ///@param str строка для анализа. «true» или «1» считаются true; все остальное false.
    ///@param b ссылка на логическое значение для записи.
    ///@return True, если синтаксический анализ прошел успешно; в противном случае false.
    bool ToBool(bool& b);

    bool ToInt      (       i8& value);
    bool ToInt      (       u8& value);
    bool ToInt      (      i16& value);
    bool ToInt      (      u16& value);
    bool ToInt      (      i32& value);
    bool ToInt      (      u32& value) { return MString::ToInt      (data, value, size); }
    bool ToF32      (      f32& value) { return MString::ToF32      (data, value, size); }
    bool ToFVector  (    FVec2& value) { return MString::StringToF32(data, value.elements, 2, length); }
    bool ToFVector  (    FVec3& value) { return MString::StringToF32(data, value.elements, 3, length); }
    bool ToFVector  (    FVec4& value) { return MString::StringToF32(data, value.elements, 4, length); }
    bool ToMatrix   ( Matrix4D& value);
    bool ToTransform(Transform& value) { return MString::ToTransform(data, value); }
};
