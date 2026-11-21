#pragma once

#include "containers/darray.h"

enum class jcf_token_type {
    Unknown,
    Whitespace,
    Comment,
    Identifier,
    OperatorEqual,
    OperatorMinus,
    OperatorPlus,
    OperatorSlash,
    OperatorAsterisk,
    OperatorDot,
    StringLiteral,
    NumericLiteral,
    Boolean,
    CurlyBraceOpen,
    CurlyBraceClose,
    BracketOpen,
    BracketClose,
    Newline,
    EOF
};

enum class jcf_property_type 
{
    // ЗАДАЧА: Хотим ли мы поддерживать типы undefined/null? Если да, то выберите один из них и используйте только его — не нужно определять оба.
    UNKNOWN,
    INT,
    FLOAT,
    STRING,
    OBJECT,
    ARRAY,
    BOOLEAN,
};

enum class jcf_object_type 
{
    OBJECT,
    ARRAY
};

struct jcf_property;

// Объект — это сущность, которая может содержать свойства. 
// К объектам относятся как типы «объект», так и типы «массив». 
// Эти типы идентичны, за исключением одного ключевого различия: 
// у объекта свойства должны иметь имена, тогда как у массива свойства — безымянные.
struct jcf_object 
{
    jcf_object_type type;
    DArray<jcf_property> properties;
};

// Псевдоним для обозначения массивов JCF, которые, по сути, представляют собой объекты JCF, содержащие свойства без имён.
using jcf_array = jcf_object;

// Представляет значение свойства для свойства jcf.
union jcf_property_value 
{
    // 64-битное целое число со знаком.
    i64 i;
    // 32-битное число с плавающей запятой.
    f32 f;
    // Строковое значение.
    const char* s;
    // Массив или объект.
    jcf_object o;
    // Логическое значение.
    bool b;
};

// Представляет отдельное свойство объекта или массива JCF.
struct jcf_property 
{
    // Тип свойства.
    jcf_property_type type;
    // Имя свойства. Если свойство принадлежит массиву, это поле должно быть равно null.
    MString name;
    // Значение свойства.
    jcf_property_value value;
};

// Представляет иерархию объектов JCF.
struct jcf_tree 
{
    // Корневой объект, который должен присутствовать всегда.
    jcf_object root;
};

struct jcf_token 
{
    jcf_token_type type;
    u32 start;
    u32 end;
#ifdef MOON_DEBUG
    const char* content;
#endif
};

class MAPI jcf_parser 
{
public:
    /// @brief Создает парсер jcf. Обратите внимание: как правило, 
    /// рекомендуется использовать функции jcf_tree_from_string() и 
    /// jcf_tree_to_string() вместо ручного вызова этого парсера, 
    /// поскольку они также берут на себя очистку объекта парсера.
    /// @return true в случае успеха, иначе false
    constexpr jcf_parser() : FileContent(), position(), tokens() {}

    /// @brief Уничтожает переданный парсер.
    ~jcf_parser() = default;

    /// @brief Использует указанный парсер для разбиения предоставленной исходной строки на токены. Обратите внимание: вместо этого рекомендуется использовать функцию jcf_tree_from_string().
    /// @param source Константный указатель на исходную строку, подлежащую разбиению на токены.
    /// @return true в случае успеха, иначе false
    bool Tokenize(const char* source);

    /// @brief Использует переданный парсер для построения `jcf_tree` на основе ранее полученных токенов. Это означает, 
    /// что для корректной работы данной функции необходимо предварительно успешно выполнить вызов `jcf_parser_tokenize()`. 
    /// Рекомендуется использовать вместо этого функцию `jcf_tree_from_string()`.
    /// @param OutThree Указатель для хранения созданного jcf_tree.
    /// @return true в случае успеха, иначе false
    bool Parse(jcf_tree& OutThree);

    /// @brief Принимает предоставленный исходный код, разбивает его на токены, а затем выполняет синтаксический разбор для создания дерева объектов `jcf_object`.
    /// @param source Указатель на исходную строку, которую необходимо разбить на токены и проанализировать. Обязательный параметр.
    /// @param OutTree Указатель для хранения созданного jcf_tree.
    /// @return true в случае успеха, иначе false
    bool TreeFromString(const char* source, jcf_tree& OutTree);

    /// @brief Принимает переданный jcf_tree и записывает его в строку в формате jcf.
    /// @param tree Указатель на используемый jcf_tree.
    /// @return true в случае успеха, иначе false
    const char* TreeToString(jcf_tree& tree);

    /// @brief Выполняет операции по очистке указанного дерева, освобождая занимаемые им память и ресурсы.
    /// @param tree Указатель на дерево, подлежащее очистке.
    void TreeCleanup(jcf_tree& tree);

    /// @brief Добавляет безымянное 64-битное целое число со знаком в предоставленный массив.
    /// @param array Указатель на массив, в который нужно добавить свойство.
    /// @param value Значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false
    template <typename T>
    bool ArrayAddValue(jcf_array& array, T value);

private:
    MString FileContent;
    u32 position;

    DArray<jcf_token> tokens;

    void PushToken(jcf_token& token);
}
